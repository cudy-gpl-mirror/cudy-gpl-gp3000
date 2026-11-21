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

#define RTK_SEC_SERVICE_UUID \
                 { 0x1d90f7f0, 0xa93c, 0x4e39,  \
                 { 0x8d, 0x94, 0x13, 0xb7, 0xc2, 0x42, 0xcf, 0x46 } }

#define PTA_INVOKE_RTK_CMD_AES256 0x0

static int blob_encap_sec(u8 act, u32 src_addr, u32 dst_addr, u32 len)
{
	struct udevice *dev = NULL;
	struct tee_shm *shm_input, *shm_output;
	struct tee_open_session_arg sess_arg = {0};
	struct tee_invoke_arg arg_func = {0};
	const struct tee_optee_ta_uuid uuid = RTK_SEC_SERVICE_UUID;
	struct tee_param param[4] = {0};
	int ret;

	/* Get tee device */
	dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!dev) {
		printf("Cannot get OP-TEE device\n");
		return -1;
	}

	/* Set TA UUID */
	tee_optee_ta_uuid_to_octets(sess_arg.uuid, &uuid);

	sess_arg.clnt_login = TEE_LOGIN_USER;

	/* Open TA session */
	ret = tee_open_session(dev, &sess_arg, 0, NULL);
printf("%s %d: ret=0x%x\n", __func__, __LINE__, ret);
	if (ret < 0) {
		printf("Cannot open session with PTA Blob 0x%X\n", ret);
		return -1;
	}





	/* Allocate shared input and output buffers for TA */
	ret = tee_shm_register(dev, (void *)(ulong)src_addr, len, 0x0, &shm_input);
	if (ret < 0) {
		printf("Cannot register input shared memory 0x%X\n", ret);
		goto error;
	}

	ret = tee_shm_register(dev, (void *)(ulong)dst_addr, len, 0x0, &shm_output);
	if (ret < 0) {
		printf("Cannot register output shared memory 0x%X\n", ret);
		goto error;
	}

	printf("%s %d: addr(%p) , size(%ld)\n",  __func__, __LINE__, shm_input->addr, shm_input->size);
	printf("%s %d: addr(%p) , size(%ld)\n",  __func__, __LINE__, shm_output->addr, shm_output->size);
	//print_hex_dump("raw data: ", DUMP_PREFIX_ADDRESS, 16, 1, shm_output->addr,shm_output->size, true);

	memset(shm_output->addr, 0, shm_output->size);

	printf(" 0x%x\n", *((int *)shm_input->addr));

	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[0].u.value.a = act;//0: enc, 1: dec

	param[1].u.memref.shm	= shm_input;
	param[1].u.memref.size	= shm_input->size;
	param[1].attr		= TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;

	param[2].u.memref.shm	= shm_output;
	param[2].u.memref.size	= shm_output->size;
	param[2].attr		= TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT;


	param[3].attr		= TEE_PARAM_ATTR_TYPE_NONE;

	arg_func.func = PTA_INVOKE_RTK_CMD_ENC;
	arg_func.session = sess_arg.session;

	/* Generate DEK blob */

	//ret = tee_invoke_func(dev, &arg_func, 4, param);
	ret = tee_invoke_func(dev, &arg_func, 4, param);



	if (ret < 0)
		printf("Cannot generate Blob with PTA DEK Blob 0x%X\n", ret);


printf("%s %d: 0x%x, 0x%x\n", __func__, __LINE__, arg_func.ret, arg_func.ret_origin);
//print_hex_dump("return data: ", DUMP_PREFIX_ADDRESS, 16, 1, shm_output->addr,shm_output->size, true);

error:
	/* Free shared memory */

	tee_shm_free(shm_input);

	tee_shm_free(shm_output);
printf("%s %d\n", __func__, __LINE__);
	/* Close session */
	ret = tee_close_session(dev, sess_arg.session);
	if (ret < 0)
		printf("Cannot close session with PTA DEK Blob 0x%X\n", ret);

	return ret;
}


#define BLOB_ENCAP_SEGMENT_SIZE 0x100000
static int blob_encap_segment_sec(u8 act, u32 src_addr, u32 dst_addr, u32 len)
{
	struct udevice *dev = NULL;
	struct tee_shm *shm_input, *shm_output;
	struct tee_open_session_arg sess_arg = {0};
	struct tee_invoke_arg arg_func = {0};
	const struct tee_optee_ta_uuid uuid = RTK_SEC_SERVICE_UUID;
	struct tee_param param[4] = {0};
	int ret;
	int i;
	int seg_num;
	void *p0, *p1;
	u32 datalen= len;
	/* Get tee device */
	dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!dev) {
		printf("Cannot get OP-TEE device\n");
		return -1;
	}

	/* Set TA UUID */
	tee_optee_ta_uuid_to_octets(sess_arg.uuid, &uuid);

	sess_arg.clnt_login = TEE_LOGIN_USER;

	/* Open TA session */
	ret = tee_open_session(dev, &sess_arg, 0, NULL);
printf("%s %d: ret=0x%x\n", __func__, __LINE__, ret);
	if (ret < 0) {
		printf("Cannot open session with PTA Blob 0x%X\n", ret);
		return -1;
	}

	seg_num = len/BLOB_ENCAP_SEGMENT_SIZE;

	if(len%BLOB_ENCAP_SEGMENT_SIZE){
		seg_num = seg_num + 1;
	}

	p0 = (void *)(ulong)src_addr;
	p1 = (void *)(ulong)dst_addr;


for(i=0; i< seg_num; i++){
	datalen = BLOB_ENCAP_SEGMENT_SIZE;
	/* Allocate shared input and output buffers for TA */
	ret = tee_shm_register(dev, p0, datalen, 0x0, &shm_input);
	if (ret < 0) {
		printf("Cannot register input shared memory 0x%X\n", ret);
		goto error;
	}

	ret = tee_shm_register(dev, p1, datalen, 0x0, &shm_output);
	if (ret < 0) {
		printf("Cannot register output shared memory 0x%X\n", ret);
		goto error;
	}

	printf("%s %d: addr(%p) , size(%ld)\n",  __func__, __LINE__, shm_input->addr, shm_input->size);
	printf("%s %d: addr(%p) , size(%ld)\n",  __func__, __LINE__, shm_output->addr, shm_output->size);
	//print_hex_dump("raw data: ", DUMP_PREFIX_ADDRESS, 16, 1, shm_output->addr,shm_output->size, true);

	memset(shm_output->addr, 0, shm_output->size);

	printf(" 0x%x\n", *((int *)shm_input->addr));

	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[0].u.value.a = act;//0: enc, 1: dec

	param[1].u.memref.shm	= shm_input;
	param[1].u.memref.size	= shm_input->size;
	param[1].attr		= TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;

	param[2].u.memref.shm	= shm_output;
	param[2].u.memref.size	= shm_output->size;
	param[2].attr		= TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT;


	param[3].attr		= TEE_PARAM_ATTR_TYPE_NONE;

	arg_func.func = PTA_INVOKE_RTK_CMD_ENC;
	arg_func.session = sess_arg.session;

	/* Generate DEK blob */

	//ret = tee_invoke_func(dev, &arg_func, 4, param);
	ret = tee_invoke_func(dev, &arg_func, 4, param);



	if (ret < 0)
		printf("Cannot generate Blob with PTA DEK Blob 0x%X\n", ret);


printf("%s %d: 0x%x, 0x%x\n", __func__, __LINE__, arg_func.ret, arg_func.ret_origin);
//print_hex_dump("return data: ", DUMP_PREFIX_ADDRESS, 16, 1, shm_output->addr,shm_output->size, true);

error:
	/* Free shared memory */

	tee_shm_free(shm_input);

	tee_shm_free(shm_output);


		p0 += BLOB_ENCAP_SEGMENT_SIZE;
		p1 += BLOB_ENCAP_SEGMENT_SIZE;

}
printf("%s %d\n", __func__, __LINE__);
	/* Close session */
	ret = tee_close_session(dev, sess_arg.session);
	if (ret < 0)
		printf("Cannot close session with PTA DEK Blob 0x%X\n", ret);

	return ret;
}





static void blob_encap_rand_fill(u32 dst_addr, u32 len)
{
	int ret;
	int *p = (int *)(long)dst_addr;
	srand(get_timer(0));
	rand();

	for(ret=0; ret<(len/4);ret++){
		*(p + ret) = rand();
	}
}
/**
 * do_sec_blob() - Handle the "sec_blob" command-line command
 * @cmdtp:  Command data struct pointer
 * @flag:   Command flag
 * @argc:   Command-line argument count
 * @argv:   Array of command-line arguments
 *
 * Returns zero on success, CMD_RET_USAGE in case of misuse and negative
 * on error.
 */
static int do_sec_blob(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	uint32_t src_addr, dst_addr, len;
	int ret=0;


	if(strcmp(argv[1], "enc") == 0){
		if (argc != 5)
			return CMD_RET_USAGE;

		src_addr = hextoul(argv[2], NULL);
		dst_addr = hextoul(argv[3], NULL);
		len = dectoul(argv[4], NULL);
		ret = blob_encap_sec(0, src_addr, dst_addr, len);
	}else
	if(strcmp(argv[1], "dec") == 0){
		if (argc != 5)
			return CMD_RET_USAGE;

		src_addr = hextoul(argv[2], NULL);
		dst_addr = hextoul(argv[3], NULL);
		len = dectoul(argv[4], NULL);
		ret = blob_encap_sec(1, src_addr, dst_addr, len);
	}else
	if(strcmp(argv[1], "rand") == 0){
		dst_addr = hextoul(argv[2], NULL);
		len = dectoul(argv[3], NULL);
		blob_encap_rand_fill(dst_addr, len);
	}
	else
		if(strcmp(argv[1], "enc2") == 0){
		if (argc != 5)
			return CMD_RET_USAGE;

		src_addr = hextoul(argv[2], NULL);
		dst_addr = hextoul(argv[3], NULL);
		len = dectoul(argv[4], NULL);
		ret = blob_encap_segment_sec(0, src_addr, dst_addr, len);
	}else
	if(strcmp(argv[1], "dec2") == 0){
		if (argc != 5)
			return CMD_RET_USAGE;

		src_addr = hextoul(argv[2], NULL);
		dst_addr = hextoul(argv[3], NULL);
		len = dectoul(argv[4], NULL);
		ret = blob_encap_segment_sec(1, src_addr, dst_addr, len);
	}else{
		printf("Unknow CMD\n");
		ret = -1;
	}
	return ret;
}

/***************************************************/
static char sec_blob_help_text[] =
	"cmd src dst len            - Encapsulate and create blob of data\n"
	"                         $len bits long at address $src and\n"
	"                         store the result at address $dst.\n";

U_BOOT_CMD(
	sec_blob, 5, 1, do_sec_blob,
	"Data Enc/Dec blob encapsulation",
	sec_blob_help_text
);



#include <uboot_aes.h>
static const uint8_t aes_key[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

static const uint8_t aes_key2[] = {
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};

static uint8_t aes_iv[] = {
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
	0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF
};

static int do_aes(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	uint32_t key_addr, iv_addr, src_addr, dst_addr, len;
	uint8_t key_ptr[32], *iv_ptr, *src_ptr, *dst_ptr;
	u8 key_exp[AES256_EXPAND_KEY_LENGTH];
	u32 aes_blocks, key_len;
	int enc;

	key_len =  256 / 8;

	if (!strncmp(argv[1], "enc", 3))
		enc = 1;
	else if (!strncmp(argv[1], "dec", 3))
		enc = 0;
	else
		return CMD_RET_USAGE;

//	key_addr = aes_key;
//	iv_addr = aes_iv;
	src_addr = hextoul(argv[2], NULL);
	dst_addr = hextoul(argv[3], NULL);
	len = hextoul(argv[4], NULL);

	memcpy(key_ptr,aes_key, 32);
	iv_ptr = aes_iv;
	src_ptr = (uint8_t *)map_sysmem(src_addr, len);
	dst_ptr = (uint8_t *)map_sysmem(dst_addr, len);

	/* First we expand the key. */
	aes_expand_key(key_ptr, key_len, key_exp);

	/* Calculate the number of AES blocks to encrypt. */
	aes_blocks = DIV_ROUND_UP(len, AES_BLOCK_LENGTH);

	if (enc)
		aes_cbc_encrypt_blocks(key_len, key_exp, iv_ptr, src_ptr,
				       dst_ptr, aes_blocks);
	else
		aes_cbc_decrypt_blocks(key_len, key_exp, iv_ptr, src_ptr,
				       dst_ptr, aes_blocks);


	return 0;
}

U_BOOT_CMD(
	rtk_aes, 7, 1, do_aes,
	"AES 256 CBC encryption",
	"aes dec/enc  src dst len"
);
