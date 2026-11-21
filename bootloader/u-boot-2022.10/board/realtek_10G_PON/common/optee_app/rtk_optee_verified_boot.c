// SPDX-License-Identifier: GPL-2.0+
/*
 *
 */
#include <common.h>
#include <log.h>
#include <malloc.h>
#include <linux/compiler.h>
#include <tee.h>
#include <hexdump.h>
#include <image.h>
#include <uboot_aes.h>

#include "rtk_optee_support.h"

#define RTK_TA_SERVICE_UUID RTK_VERIFY_BOOT_SERVICE_UUID
static int open_session_ta_rtk_tee(struct udevice *dev, u32 *session_id)
{
	const struct tee_optee_ta_uuid uuid = RTK_TA_SERVICE_UUID;
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

/************************************************************************************
 * uImage decrypt
 ************************************************************************************/
static int tee_crypto_aes256_bl33_decrypt(const void *src_addr, void *dst_addr, u32 len)
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

	ret = open_session_ta_rtk_tee(dev, &session_id);
	if (ret < 0) {
		printf("Cannot open session with PTA  UUID.ret 0x%X\n", ret);
		return -1;
	}

	/* Allocate shared input and output buffers for TA */
	ret = tee_shm_register(dev, (void *)src_addr, len, 0x0, &shm_input);
	if (ret < 0) {
		printf("Cannot register input shared memory 0x%X\n", ret);
		goto error;
	}

	ret = tee_shm_register(dev, (void *)dst_addr, len, 0x0, &shm_output);
	if (ret < 0) {
		printf("Cannot register output shared memory 0x%X\n", ret);
		goto error;
	}
	//printf("%s %d: addr(%p) , size(%ld)\n",  __func__, __LINE__, shm_input->addr, shm_input->size);
	//printf("%s %d: addr(%p) , size(%ld)\n",  __func__, __LINE__, shm_output->addr, shm_output->size);
	//print_hex_dump("raw data: ", DUMP_PREFIX_ADDRESS, 16, 1, shm_output->addr,shm_output->size, true);

	memset(shm_output->addr, 0, shm_output->size);

	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INOUT;
	param[0].u.value.a = 1;	//0: enc, 1: dec

	param[1].u.memref.shm = shm_input;
	param[1].u.memref.size = shm_input->size;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;

	param[2].u.memref.shm = shm_output;
	param[2].u.memref.size = shm_output->size;
	param[2].attr = TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT;

	param[3].attr = TEE_PARAM_ATTR_TYPE_NONE;

	arg_func.func = PTA_INVOKE_RTK_BL33_DECRYPT_AES256;
	arg_func.session = session_id;

	ret = tee_invoke_func(dev, &arg_func, 4, param);

	if (ret < 0)
		printf("Error. tee_invoke_func with PTA UUID. ret(0x%X)\n", ret);

error:
	/* Free shared memory */
	if (shm_input)
		tee_shm_free(shm_input);
	if (shm_output)
		tee_shm_free(shm_output);

	/* Close session */
	ret = tee_close_session(dev, session_id);
	if (ret < 0)
		printf("Cannot close session with PTA RTK SERVICE_UUID. ret(0x%X)\n", ret);

	return ret;
}


int rtk_image_aes_decrypt(struct image_cipher_info *info,
		      const void *cipher, size_t cipher_len,
		      void **data, size_t *size)
{
	*data = malloc(cipher_len);
	if (!*data) {
		printf("Can't allocate memory to decrypt\n");
		return -ENOMEM;
	}
	*size = info->size_unciphered;

	tee_crypto_aes256_bl33_decrypt(cipher, *data, cipher_len);

	return 0;
}
/*******************************************************
 * Verify Image
 ********************************************************/
#include <u-boot/rsa.h>

#define TEE_ALG_RSASSA_PKCS1_PSS_MGF1_SHA256    0x70414930
struct rtk_pta_key_info {
	uint32_t algo;
	uint32_t salt_size;
	uint32_t info_size;
	uint8_t info[];
};
int rtk_tee_rsa_verify_key(struct image_sign_info *info,
			  void *prop, const uint8_t *sig,
			  const uint32_t sig_len, const uint8_t *hash,
			  const uint32_t key_len)
{
	struct udevice *dev = NULL;
	struct tee_shm *shm_in0 = NULL, *shm_in1 = NULL, *shm_in2 = NULL ;
	struct tee_invoke_arg arg_func = { 0 };
	struct tee_param param[4] = { 0 };
	u32 session_id;
	u32 flags = 0;
	int ret;

	struct rtk_pta_key_info key_info;

	struct checksum_algo *checksum = info->checksum;

	int hash_len;

	hash_len = checksum->checksum_len;
	key_info.algo = TEE_ALG_RSASSA_PKCS1_PSS_MGF1_SHA256;
/**********************************************************
 *   u-boot-2022 required, if use u-boot mkimg to sign and enc
 *   salt_size = sig_len - hash_len -2;
 *   rsa2048,sha256=>  222 = 256 - 32 -2 ;
 **********************************************************/
	key_info.salt_size = sig_len - hash_len -2;
	/* Get tee device */
	dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!dev) {
		printf("Cannot get OP-TEE device\n");
		return -1;
	}

	ret = open_session_ta_rtk_tee(dev, &session_id);
	if (ret < 0) {
		printf("Cannot open session with PTA RTK SERVICE_UUID.ret 0x%X\n", ret);
		return -1;
	}

	/* Allocate shared input and output buffers for TA */
	ret = tee_shm_register(dev,  &key_info, sizeof(key_info) , flags, &shm_in0);
	if (ret < 0) {
		printf("Cannot register input shared memory 0x%X\n", ret);
		goto error;
	}

	ret = tee_shm_register(dev,  (void *)hash, hash_len, flags, &shm_in1);
	if (ret < 0) {
		printf("Cannot register output shared memory 0x%X\n", ret);
		goto error;
	}

	ret = tee_shm_register(dev,  (void *)sig, sig_len, flags, &shm_in2);
	if (ret < 0) {
		printf("Cannot register output shared memory 0x%X\n", ret);
		goto error;
	}

	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = 0;

	param[1].u.memref.shm = shm_in0;
	param[1].u.memref.size = shm_in0->size;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;

	param[2].u.memref.shm = shm_in1;
	param[2].u.memref.size = shm_in1->size;
	param[2].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;

	param[3].u.memref.shm = shm_in2;
	param[3].u.memref.size = shm_in2->size;
	param[3].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;


	arg_func.func = PTA_INVOKE_RTK_BL33_RSA_PSS_VERIFY;
	arg_func.session = session_id;

	ret = tee_invoke_func(dev, &arg_func, 4, param);

	if (ret < 0)
		printf("Error. tee_invoke_func with PTA RTK SERVICE_UUID. ret(0x%X)\n", ret);


	if( arg_func.ret){
		printf("%s %d: 0x%x, 0x%x\n", __func__, __LINE__, arg_func.ret, arg_func.ret_origin);
		return -1;
	}
//print_hex_dump("return data: ", DUMP_PREFIX_ADDRESS, 16, 1, shm_output->addr,shm_output->size, true);

      error:
	/* Free shared memory */
	if (shm_in0)
		tee_shm_free(shm_in0);

	if (shm_in1)
		tee_shm_free(shm_in1);
	if (shm_in2)
		tee_shm_free(shm_in2);


	/* Close session */
	ret = tee_close_session(dev, session_id);
	if (ret < 0)
		printf("Cannot close session with PTA RTK SERVICE_UUID. ret(0x%X)\n", ret);

	return ret;

}

