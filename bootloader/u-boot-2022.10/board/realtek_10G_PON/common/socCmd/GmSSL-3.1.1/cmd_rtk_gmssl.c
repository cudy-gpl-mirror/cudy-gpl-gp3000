/*
 * (C) Copyright 2001
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * Misc functions
 */
#include <common.h>
#include <command.h>
#include <cpu_func.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <gmssl/sm3.h>
#include <gmssl/sm4.h>
#include <gmssl/rand.h>

#define TEST_STR0 "helloworld"
#define TEST_STR1 "Donald Trump passed the critical threshold of 270 electoral college votes with a projected win in the state of Wisconsin making him the next US president."

void rtk_gsmml_sm3(void *str, int size, uint8_t *result){
	SM3_CTX sm3_ctx;

	sm3_init(&sm3_ctx);
	sm3_update(&sm3_ctx, str, size);
	sm3_finish(&sm3_ctx, result);
}

static int rtk_gmssl_sm3_test0(void) {
	SM3_CTX sm3_ctx;
	uint8_t buf[4096];
	size_t len;
	uint8_t dgst[32];
	int i;
	strcpy(buf, TEST_STR1);
	len = strlen(TEST_STR1);
	sm3_init(&sm3_ctx);
	sm3_update(&sm3_ctx, buf, len);
	sm3_finish(&sm3_ctx, dgst);

	for (i = 0; i < sizeof(dgst); i++) {
			printf("%02x", dgst[i]);
	}
	printf("\n");
	return 0;
}
static int rtk_gmssl_sm3_test1(void) {
	uint8_t buf[4096];
	size_t len;
	uint8_t dgst[32];
	int i;

	strcpy(buf, TEST_STR1);
	len = strlen(TEST_STR1);

	rtk_gsmml_sm3(buf, len, dgst);

	for (i = 0; i < sizeof(dgst); i++) {
			printf("%02x", dgst[i]);
	}
	printf("\n");
	return 0;
}
/*
 * SM4: 
 * 使用128位的分組
 * 固定使用128位的key
 */
/************************/

/*
 * SM4 CBC
 *
 */
int rtk_gmssl_sm4_cbc_enc(char *mbuf, uint32_t data_len, char *skey, char *iv, char *dst_buf, uint8_t dump)
{
	SM4_KEY sm4_key;
	int i;
	if (dump) {
		printf("%s %d: %d\n", __func__, __LINE__, data_len);

		printf("key: ");
		for (i = 0; i < SM4_KEY_SIZE; i++) {
				printf("0x%02X, ", skey[i]);
		}
		printf("\n");

		printf("iv: ");
		for (i = 0; i <SM4_KEY_SIZE; i++) {
				printf("0x%02X, ", iv[i]);
		}
		printf("\n");

		printf("plaintext: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", mbuf[i]);
		}
		printf("\n");
	}

	sm4_set_encrypt_key(&sm4_key, skey);
	sm4_cbc_encrypt(&sm4_key, iv, mbuf, data_len/SM4_BLOCK_SIZE, dst_buf);
	if (dump) {
		printf("ciphertext: ");
		for (i = 0; i < data_len; i++) {
			printf("%02X", dst_buf[i]);
		}
		printf("\n");
	}
	return 0;
}


int rtk_gmssl_sm4_cbc_dec(char *mbuf, uint32_t data_len, char *skey, char *iv, char *dst_buf, uint8_t dump)
{
	SM4_KEY sm4_key;
	int i;
	if (dump) {
		printf("%s %d: %d\n", __func__, __LINE__, data_len);

		printf("key: ");
		for (i = 0; i < SM4_KEY_SIZE; i++) {
				printf("0x%02X, ", skey[i]);
		}
		printf("\n");

		printf("iv: ");
		for (i = 0; i <SM4_KEY_SIZE; i++) {
				printf("0x%02X, ", iv[i]);
		}
		printf("\n");

		printf("ciphertext: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", mbuf[i]);
		}
		printf("\n");
	}

	sm4_set_decrypt_key(&sm4_key, skey);
	sm4_cbc_decrypt(&sm4_key, iv, mbuf, data_len/SM4_BLOCK_SIZE, dst_buf);

	if (dump) {
		printf("plaintext: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", dst_buf[i]);
		}
		printf("\n");
	}
	return 0;
}


static int rtk_gmssl_sm4_cbc(char *mbuf, uint32_t data_len, char *skey, char *iv)
{
		SM4_KEY sm4_key;
		char *cbuf;
		char *pbuf;
		int i;

		cbuf = malloc(data_len);
		pbuf = malloc(data_len);

printf("%s %d: %d\n", __func__, __LINE__, data_len);

		printf("key: ");
		for (i = 0; i < SM4_KEY_SIZE; i++) {
				printf("0x%02X, ", skey[i]);
		}
		printf("\n");

		printf("iv: ");
		for (i = 0; i <SM4_KEY_SIZE; i++) {
				printf("0x%02X, ", iv[i]);
		}
		printf("\n");

		printf("plaintext: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", mbuf[i]);
		}
		printf("\n");

		sm4_set_encrypt_key(&sm4_key, skey);
		sm4_cbc_encrypt(&sm4_key, iv, mbuf, data_len/SM4_BLOCK_SIZE, cbuf);

		printf("ciphertext: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", cbuf[i]);
		}
		printf("\n");
		printf("iv: ");
		for (i = 0; i <SM4_KEY_SIZE; i++) {
				printf("0x%02X, ", iv[i]);
		}
		printf("\n");

		sm4_set_decrypt_key(&sm4_key, skey);
		sm4_cbc_decrypt(&sm4_key, iv, cbuf, data_len/SM4_BLOCK_SIZE, pbuf);

		printf("decrypted: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", pbuf[i]);
		}
		printf("\n");

		free(cbuf);
		free(pbuf);

		return 0;
}

static int rtk_gmssl_sm4_cbc_test0(void) {
		SM4_KEY sm4_key;
		unsigned char key[16];
		unsigned char iv[16];
		unsigned char mbuf[32] = {
				0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
				0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
				0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
				0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
		};
		unsigned char cbuf[32] = {0};
		unsigned char pbuf[32] = {0};
		int i;

		rand_bytes(key, sizeof(key));
		rand_bytes(iv, sizeof(iv));

		printf("key: ");
		for (i = 0; i < sizeof(key); i++) {
				printf("0x%02X, ", key[i]);
		}
		printf("\n");

		printf("iv: ");
		for (i = 0; i < sizeof(iv); i++) {
				printf("0x%02X, ", iv[i]);
		}
		printf("\n");

		printf("plaintext: ");
		for (i = 0; i < sizeof(mbuf); i++) {
				printf("%02X", mbuf[i]);
		}
		printf("\n");

		sm4_set_encrypt_key(&sm4_key, key);
		sm4_cbc_encrypt(&sm4_key, iv, mbuf, sizeof(mbuf)/SM4_BLOCK_SIZE, cbuf);

		printf("ciphertext: ");
		for (i = 0; i < sizeof(cbuf); i++) {
				printf("%02X", cbuf[i]);
		}
		printf("\n");

		sm4_set_decrypt_key(&sm4_key, key);
		sm4_cbc_decrypt(&sm4_key, iv, cbuf, sizeof(cbuf)/SM4_BLOCK_SIZE, pbuf);

		printf("decrypted: ");
		for (i = 0; i < sizeof(pbuf); i++) {
				printf("%02X", pbuf[i]);
		}
		printf("\n");

		return 0;
}

extern unsigned int rand(void);
#define MAX_STR_LEN 2048
static int rtk_gmssl_sm4_cbc_test1(void) {
		SM4_KEY sm4_key;
		unsigned char key[16];
		unsigned char iv[16];
 //       unsigned char mbuf[2048] = { 0x0 };
 //       unsigned char cbuf[2048] = {0};
 //       unsigned char pbuf[2048] = {0};
 
		char *mbuf;
		char *cbuf;
		char *pbuf;

		int i;
		unsigned int data_len;

		mbuf = malloc(MAX_STR_LEN);
		cbuf = malloc(MAX_STR_LEN);
		pbuf = malloc(MAX_STR_LEN);
		

		data_len = rand() % MAX_STR_LEN;
		data_len = (data_len / 16) * 16;

printf("%s %d: %d\n", __func__, __LINE__, data_len);
		rand_bytes(mbuf, data_len);

		rand_bytes(key, sizeof(key));
		rand_bytes(iv, sizeof(iv));

		printf("key: ");
		for (i = 0; i < sizeof(key); i++) {
				printf("0x%02X, ", key[i]);
		}
		printf("\n");

		printf("iv: ");
		for (i = 0; i < sizeof(iv); i++) {
				printf("0x%02X, ", iv[i]);
		}
		printf("\n");

		printf("plaintext: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", mbuf[i]);
		}
		printf("\n");

		sm4_set_encrypt_key(&sm4_key, key);
		sm4_cbc_encrypt(&sm4_key, iv, mbuf, data_len/SM4_BLOCK_SIZE, cbuf);

		printf("ciphertext: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", cbuf[i]);
		}
		printf("\n");

		sm4_set_decrypt_key(&sm4_key, key);
		sm4_cbc_decrypt(&sm4_key, iv, cbuf, data_len/SM4_BLOCK_SIZE, pbuf);

		printf("decrypted: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", pbuf[i]);
		}
		printf("\n");

		free(mbuf);
		free(cbuf);
		free(pbuf);

		return 0;
}
static void rtk_gmssl_sm4_cbc_test2(void)
{
		char *data_p;
		unsigned int data_len;
		unsigned char key[16];
		unsigned char iv[16];
		data_len = rand() % MAX_STR_LEN;
		data_len = (data_len / SM4_BLOCK_SIZE) * SM4_BLOCK_SIZE;
		data_p = malloc(data_len);

printf("%s %d: %d\n", __func__, __LINE__, data_len);
		rand_bytes(data_p, data_len);

		rand_bytes(key, sizeof(key));
		rand_bytes(iv, sizeof(iv));
		rtk_gmssl_sm4_cbc(data_p, data_len, key, iv);
		free(data_p);
		return;
}

static void rtk_gmssl_sm4_cbc_test3(void)
{
		char *data_p;
		char *enc_buf;
		char *dec_buf;
		unsigned int data_len;
		unsigned char key[16];
		unsigned char iv[16];

		data_len = rand() % MAX_STR_LEN;
printf("%s %d: %d\n", __func__, __LINE__, data_len);
		data_len = (data_len / SM4_BLOCK_SIZE) * SM4_BLOCK_SIZE;
		data_p = malloc(data_len);
		enc_buf = malloc(data_len);
		dec_buf = malloc(data_len);

		rand_bytes(data_p, data_len);
		rand_bytes(key, sizeof(key));
		rand_bytes(iv, sizeof(iv));
		
		rtk_gmssl_sm4_cbc_enc(data_p, data_len, key, iv, enc_buf, 1);
		rtk_gmssl_sm4_cbc_dec(enc_buf, data_len, key, iv, dec_buf, 1);
		free(data_p);
		free(enc_buf);
		free(dec_buf);
		return;
}

/*
 * SM4 CTR
 *
 */
 /* encrypt will add counter to iv=> iv = iv + counter */
 int rtk_gmssl_sm4_ctr_enc(char *mbuf, uint32_t data_len, char *skey, char *iv, char *dst_buf, uint8_t dump)
{
	SM4_KEY sm4_key;
	int i;
	char ctr[16];
	if (dump) {
		printf("%s %d: %d\n", __func__, __LINE__, data_len);

		printf("key: ");
		for (i = 0; i < SM4_KEY_SIZE; i++) {
				printf("0x%02X, ", skey[i]);
		}
		printf("\n");

		printf("iv: ");
		for (i = 0; i <SM4_KEY_SIZE; i++) {
				printf("0x%02X, ", iv[i]);
		}
		printf("\n");

		printf("plaintext: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", mbuf[i]);
		}
		printf("\n");
	}
	memcpy(ctr, iv, 16);
	sm4_set_encrypt_key(&sm4_key, skey);
	sm4_ctr_encrypt(&sm4_key, ctr, mbuf, data_len, dst_buf);

	if (dump) {
		printf("ciphertext: ");
		for (i = 0; i < data_len; i++) {
			printf("%02X", dst_buf[i]);
		}
		printf("\n");
	}
	return 0;
}


int rtk_gmssl_sm4_ctr_dec(char *mbuf, uint32_t data_len, char *skey, char *iv, char *dst_buf, uint8_t dump)
{
	SM4_KEY sm4_key;
	char ctr[16];
	int i;
	if (dump) {
		printf("%s %d: %d\n", __func__, __LINE__, data_len);

		printf("key: ");
		for (i = 0; i < SM4_KEY_SIZE; i++) {
				printf("0x%02X, ", skey[i]);
		}
		printf("\n");

		printf("iv: ");
		for (i = 0; i <SM4_KEY_SIZE; i++) {
				printf("0x%02X, ", iv[i]);
		}
		printf("\n");

		printf("ciphertext: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", mbuf[i]);
		}
		printf("\n");
	}
	memcpy(ctr,iv, sizeof(ctr));
	sm4_set_encrypt_key(&sm4_key, skey);
	sm4_ctr_decrypt(&sm4_key, ctr, mbuf, data_len, dst_buf);

	if (dump) {
		printf("plaintext: ");
		for (i = 0; i < data_len; i++) {
				printf("%02X", dst_buf[i]);
		}
		printf("\n");
	}
	return 0;
}

static int rtk_gmssl_sm4_ctr_test0(void) {
		SM4_KEY sm4_key;
		unsigned char key[16];
		unsigned char iv[16];
		unsigned char ctr[16];
		unsigned char mbuf[20] = {
				0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
				0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,
				0x31,0x32,0x33,0x34,
		};
		unsigned char cbuf[20] = {0};
		unsigned char pbuf[20] = {0};
		int i;

		rand_bytes(key, sizeof(key));
		rand_bytes(iv, sizeof(iv));

		printf("key: ");
		for (i = 0; i < sizeof(key); i++) {
				printf("%02X", key[i]);
		}
		printf("\n");

		printf("ctr: ");
		for (i = 0; i < sizeof(iv); i++) {
				printf("%02X", iv[i]);
		}
		printf("\n");

		sm4_set_encrypt_key(&sm4_key, key);

		printf("sm4 ctr encrypt %zu bytes\n", sizeof(mbuf));

		printf("plaintext: ");
		for (i = 0; i < sizeof(mbuf); i++) {
				printf("%02X", mbuf[i]);
		}
		printf("\n");

		memcpy(ctr, iv, 16);
		sm4_ctr_encrypt(&sm4_key, ctr, mbuf, sizeof(mbuf), cbuf);
		
		printf("ctr: ");
		for (i = 0; i < sizeof(iv); i++) {
				printf("%02X", ctr[i]);
		}

		printf("\n");
		printf("ciphertext: ");
		for (i = 0; i < sizeof(cbuf); i++) {
				printf("%02X", cbuf[i]);
		}
		printf("\n");
		memcpy(ctr, iv, 16);
		sm4_ctr_decrypt(&sm4_key, ctr, cbuf, sizeof(cbuf), pbuf);

		printf("decrypted: ");
		for (i = 0; i < sizeof(pbuf); i++) {
				printf("%02X", pbuf[i]);
		}
		printf("\n");


		return 0;
}

static void rtk_gmssl_sm4_ctr_test1(void)
{
		char *data_p;
		char *enc_buf;
		char *dec_buf;
		unsigned int data_len;
		unsigned char key[16];
		unsigned char iv[16];

		data_len = rand() % MAX_STR_LEN;
printf("%s %d: %d\n", __func__, __LINE__, data_len);
		data_len = (data_len / SM4_BLOCK_SIZE) * SM4_BLOCK_SIZE;
		data_p = malloc(data_len);
		enc_buf = malloc(data_len);
		dec_buf = malloc(data_len);

		rand_bytes(data_p, data_len);
		rand_bytes(key, sizeof(key));
		rand_bytes(iv, sizeof(iv));
		
		rtk_gmssl_sm4_ctr_enc(data_p, data_len, key, iv, enc_buf, 1);
		rtk_gmssl_sm4_ctr_dec(enc_buf, data_len, key, iv, dec_buf, 1);
		free(data_p);
		free(enc_buf);
		free(dec_buf);
		return;
}


/************************/
static int do_gmssl(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u32 caseid;
//	u64 para=0, para1=0, para2=0,para3=0;

	if (argc <2)
		return CMD_RET_USAGE;

	if (argc == 2){
		caseid = simple_strtoul(argv[1], NULL, 10);

		switch(caseid){
		case 0:
			rtk_gmssl_sm3_test0();
			break;
		case 1:
			rtk_gmssl_sm3_test1();
			break;
		case 10:
			rtk_gmssl_sm4_cbc_test0();
			break;
		case 11:
			rtk_gmssl_sm4_cbc_test1();
			break;
		case 12:
			rtk_gmssl_sm4_cbc_test2();
			break;
		case 13:
			rtk_gmssl_sm4_cbc_test3();
			break;
		case 20:
			rtk_gmssl_sm4_ctr_test0();
			break;
		case 21:
			rtk_gmssl_sm4_ctr_test1();
			break;
		default:
			printf("Not support %d\n", caseid);
			break;
		}
	}

	return 0;
}

U_BOOT_CMD(
	gmssl ,    5,    0,     do_gmssl,
	"gmssl test",
	"\n\t\t <case>\n"
	"    - gmssl test code."
);

