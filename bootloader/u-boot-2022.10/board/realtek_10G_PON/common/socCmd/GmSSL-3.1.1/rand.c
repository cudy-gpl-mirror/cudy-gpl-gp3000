/*
 *  Copyright 2014-2022 The GmSSL Project. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the License); you may
 *  not use this file except in compliance with the License.
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 */
#include <common.h>
#include <command.h>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gmssl/rand.h>
#include <gmssl/error.h>

#define RAND_MAX_BUF_SIZE 4096

/* USEu-boot Pseudo random number generation */
#include <asm/io.h>
extern unsigned int rand(void);
extern void srand(unsigned int seed);

int rand_bytes(uint8_t *buf, size_t len)
{
	unsigned long val;
	unsigned int rest;
	val =  get_ticks();
	srand((int)val);
	
	//printf("timer 0x%llx\n", val);
	rest = len;
	while(1){
		val = rand();
		//printf("rand 0x%x\n", val);
		if(rest > 4) {
			memcpy(buf, &val, 4);
			rest -= 4;	
			buf += 4;
		} else {
			memcpy(buf, &val, rest);
			return 1;
		}
	}

}
#if 0
int rand_bytes(uint8_t *buf, size_t len)
{
	FILE *fp;
	if (!buf) {
		error_print();
		return -1;
	}
	if (len > RAND_MAX_BUF_SIZE) {
		error_print();
		return -1;
	}
	if (!len) {
		return 0;
	}

	if (!(fp = fopen("/dev/urandom", "rb"))) {
		error_print();
		return -1;
	}
	if (fread(buf, 1, len, fp) != len) {
		error_print();
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 1;
}
#endif
