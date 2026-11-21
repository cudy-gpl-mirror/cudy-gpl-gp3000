// SPDX-License-Identifier: GPL-2.0+
/*
 *
 */
#include <common.h>
#include <command.h>
#include <log.h>
#include <malloc.h>
#include <memalign.h>
#include <asm/byteorder.h>
#include <linux/compiler.h>
#include <mapmem.h>
#include <tee.h>
#include <cpu_func.h>
#include <hexdump.h>
#include <rand.h>

#include "rtk_optee_support.h"

static int open_session_ta_rtk_crypto(struct udevice *dev, u32 *session_id)
{
	const struct tee_optee_ta_uuid uuid = RTK_SEC_SERVICE_UUID;
	struct tee_open_session_arg sess_arg = { 0 };
	int ret;

	/* Open session with hwrng Trusted App */
	tee_optee_ta_uuid_to_octets(sess_arg.uuid, &uuid);
	sess_arg.clnt_login = TEE_LOGIN_USER;

	ret = tee_open_session(dev, &sess_arg, 0, NULL);
	if (ret || sess_arg.ret) {
		if (!ret)
			ret = -EIO;
		return ret;
	}

	*session_id = sess_arg.session;
	return 0;
}

static int tee_crypto_aes256(u8 act, u32 src_addr, u32 dst_addr, u32 len)
{
	struct udevice *dev = NULL;
	struct tee_shm *shm_input = NULL, *shm_output = NULL;
	struct tee_invoke_arg arg_func = { 0 };
	struct tee_param param[4] = { 0 };
	u32 session_id;
	int ret;

	/* Get tee device */
	dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!dev) {
		printf("Cannot get OP-TEE device\n");
		return -1;
	}

	ret = open_session_ta_rtk_crypto(dev, &session_id);
	if (ret < 0) {
		printf("Cannot open session with PTA RTK_SEC_SERVICE_UUID.ret 0x%X\n", ret);
		return -1;
	}

	/* Allocate shared input and output buffers for TA */
	ret = tee_shm_register(dev, (void *)(ulong) src_addr, len, 0x0, &shm_input);
	if (ret < 0) {
		printf("Cannot register input shared memory 0x%X\n", ret);
		goto error;
	}

	ret = tee_shm_register(dev, (void *)(ulong) dst_addr, len, 0x0, &shm_output);
	if (ret < 0) {
		printf("Cannot register output shared memory 0x%X\n", ret);
		goto error;
	}
	//printf("%s %d: addr(%p) , size(%ld)\n",  __func__, __LINE__, shm_input->addr, shm_input->size);
	//printf("%s %d: addr(%p) , size(%ld)\n",  __func__, __LINE__, shm_output->addr, shm_output->size);
	//print_hex_dump("raw data: ", DUMP_PREFIX_ADDRESS, 16, 1, shm_output->addr,shm_output->size, true);

	memset(shm_output->addr, 0, shm_output->size);

	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[0].u.value.a = act;	//0: enc, 1: dec

	param[1].u.memref.shm = shm_input;
	param[1].u.memref.size = shm_input->size;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;

	param[2].u.memref.shm = shm_output;
	param[2].u.memref.size = shm_output->size;
	param[2].attr = TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT;

	param[3].attr = TEE_PARAM_ATTR_TYPE_NONE;

	arg_func.func = PTA_INVOKE_RTK_CMD_AES256;
	arg_func.session = session_id;

	ret = tee_invoke_func(dev, &arg_func, 4, param);

	if (ret < 0)
		printf("Error. tee_invoke_func with PTA RTK_SEC_SERVICE_UUID. ret(0x%X)\n", ret);

//printf("%s %d: 0x%x, 0x%x\n", __func__, __LINE__, arg_func.ret, arg_func.ret_origin);
//print_hex_dump("return data: ", DUMP_PREFIX_ADDRESS, 16, 1, shm_output->addr,shm_output->size, true);

      error:
	/* Free shared memory */
	if (shm_input)
		tee_shm_free(shm_input);
	if (shm_output)
		tee_shm_free(shm_output);

	/* Close session */
	ret = tee_close_session(dev, session_id);
	if (ret < 0)
		printf("Cannot close session with PTA RTK_SEC_SERVICE_UUID. ret(0x%X)\n", ret);

	return ret;
}

/********************************************************
 * RAND NUM
 *********************/
#define MAX_ENTROPY_REQ_SZ		4096
static int get_optee_rng_data(struct udevice *dev, u32 session_id, struct tee_shm *entropy_shm_pool, void *buf, size_t *size)
{
	int ret = 0;
	struct tee_invoke_arg arg = { 0 };
	struct tee_param param = { 0 };

	/* Invoke TA_CMD_GET_ENTROPY function of Trusted App */
	arg.func = PTA_INVOKE_RTK_CMD_PRNG;
	arg.session = session_id;

	/* Fill invoke cmd params */
	param.attr = TEE_PARAM_ATTR_TYPE_MEMREF_INOUT;
	param.u.memref.shm = entropy_shm_pool;
	param.u.memref.size = *size;

	ret = tee_invoke_func(dev, &arg, 1, &param);
	if (ret || arg.ret) {
		if (!ret)
			ret = -EPROTO;

		printf("Error. tee_invoke_func with PTA RTK_SEC_SERVICE_UUID. ret(0x%X)\n", ret);
		*size = 0;

		return ret;
	}

	memcpy(buf, param.u.memref.shm->addr, param.u.memref.size);
	*size = param.u.memref.size;

	return 0;
}

static int optee_rng_read(void *buf, size_t len)
{
	size_t read = 0, rng_size = 0;
	struct tee_shm *entropy_shm_pool;
	u8 *data = buf;
	struct udevice *dev = NULL;
	int ret;
	u32 session_id = 0;
	/* Get tee device */
	dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!dev) {
		printf("Cannot get OP-TEE device\n");
		return -1;
	}
	ret = open_session_ta_rtk_crypto(dev, &session_id);
	if (ret) {
		printf("can't open session: %d\n", ret);
		return ret;
	}

	ret = tee_shm_alloc(dev, MAX_ENTROPY_REQ_SZ, 0, &entropy_shm_pool);
	if (ret) {
		printf("tee_shm_alloc failed: %d\n", ret);
		goto session_close;
	}

	while (read < len) {
		rng_size = min(len - read, (size_t)MAX_ENTROPY_REQ_SZ);
		ret = get_optee_rng_data(dev, session_id, entropy_shm_pool, data, &rng_size);
		if (ret)
			goto shm_free;
		data += rng_size;
		read += rng_size;
	}

      shm_free:
	tee_shm_free(entropy_shm_pool);

      session_close:
	tee_close_session(dev, session_id);

	return ret;
}

static void do_optee_rng_read(u64 dst_addr, u32 len)
{
	void *buf = (void *)dst_addr;
	optee_rng_read(buf, len);
	print_hex_dump("    ", DUMP_PREFIX_OFFSET, 16, 1, buf, len, true);
	return;
}
/*
 * u-boot cmd
 */

static int do_tee_crypto(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	uint32_t src_addr, dst_addr, len;
	int ret = 0;

	if (strcmp(argv[1], "enc") == 0) {
		if (argc != 5)
			return CMD_RET_USAGE;

		src_addr = hextoul(argv[2], NULL);
		dst_addr = hextoul(argv[3], NULL);
		len = hextoul(argv[4], NULL);
		ret = tee_crypto_aes256(0, src_addr, dst_addr, len);
	} else if (strcmp(argv[1], "dec") == 0) {
		if (argc != 5)
			return CMD_RET_USAGE;

		src_addr = hextoul(argv[2], NULL);
		dst_addr = hextoul(argv[3], NULL);
		len = hextoul(argv[4], NULL);
		ret = tee_crypto_aes256(1, src_addr, dst_addr, len);
	} else if (strcmp(argv[1], "rand") == 0) {
		dst_addr = hextoul(argv[2], NULL);
		len = dectoul(argv[3], NULL);
		do_optee_rng_read(dst_addr, len);
	} else {
		printf("Unknow CMD\n");
		ret = -1;
	}
	return ret;
}

/***************************************************/
static char tee_crypto_help_text[] =
	"\nFor AES256 CBC:\n"
	 "\tenc  $src  $dst  $size.\n"
	"\tdec  $src  $dst  $size.\n"
	"For Random Number Generator:\n"
	"\trand  $dst $size\n" "\n"
	"Note: $src, $dst, $size => Hex.\n";

U_BOOT_CMD(tee_crypto, 5, 1, do_tee_crypto,
			"Crypto functio via TEE call", tee_crypto_help_text);
