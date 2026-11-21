/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2016, Cortina Access. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <platform.h>
#include <platform_oid.h>
#include <stdint.h>
#include <string.h>
#include <platform_def.h>
#ifdef CORTINA_OTP_SUPPORT
#include <cortina_otp_reg.h>
#include INC_CORTINA_PLATFORM_OTP_DATA_HEADER
#include <cortina_otp.h>
#endif /* CORTINA_OTP_SUPPORT */

/* Weak definition may be overridden in specific platform */
#pragma weak plat_match_rotpk
#pragma weak plat_get_nv_ctr
#pragma weak plat_set_nv_ctr

/* SHA256 algorithm */
#define SHA256_BYTES			32

/* ROTPK locations */
#define ARM_ROTPK_REGS_ID		1
#define ARM_ROTPK_DEVEL_RSA_ID		2

#if !ARM_ROTPK_LOCATION_ID
  #error "ARM_ROTPK_LOCATION_ID not defined"
#endif

#ifdef CORTINA_OTP_SUPPORT
#if (ARM_ROTPK_LOCATION_ID == ARM_ROTPK_DEVEL_RSA_ID)
#error "Embedded BL image ROTPK_HASH no longer supported. Use ARM_ROTPK_LOCATION=regs instead to fetch ROTPK_HASH from OTP"
#endif /* ARM_ROTPK_LOCATION_ID == ARM_ROTPK_DEVEL_RSA_ID */
#endif /* CORTINA_OTP_SUPPORT */

static const unsigned char rotpk_hash_hdr[] =		\
		"\x30\x31\x30\x0D\x06\x09\x60\x86\x48"	\
		"\x01\x65\x03\x04\x02\x01\x05\x00\x04\x20";
static const unsigned int rotpk_hash_hdr_len = sizeof(rotpk_hash_hdr) - 1;
static unsigned char rotpk_hash_der[sizeof(rotpk_hash_hdr) - 1 + SHA256_BYTES];

#if (ARM_ROTPK_LOCATION_ID == ARM_ROTPK_DEVEL_RSA_ID)
static const unsigned char arm_devel_rotpk_hash[] =	\
		"\xB0\xF3\x82\x09\x12\x97\xD8\x3A"	\
		"\x37\x7A\x72\x47\x1B\xEC\x32\x73"	\
		"\xE9\x92\x32\xE2\x49\x59\xF6\x5E"	\
		"\x8B\x4A\x4A\x46\xD8\x22\x9A\xDA";
#endif

/*
 * Return the ROTPK hash in the following ASN.1 structure in DER format:
 *
 * AlgorithmIdentifier  ::=  SEQUENCE  {
 *     algorithm         OBJECT IDENTIFIER,
 *     parameters        ANY DEFINED BY algorithm OPTIONAL
 * }
 *
 * DigestInfo ::= SEQUENCE {
 *     digestAlgorithm   AlgorithmIdentifier,
 *     digest            OCTET STRING
 * }
 */
int plat_get_rotpk_info(void *cookie, void **key_ptr, unsigned int *key_len,
			unsigned int *flags)
{
	uint8_t *dst;
#ifdef CORTINA_OTP_SUPPORT
#if (ARM_ROTPK_LOCATION_ID == ARM_ROTPK_REGS_ID)
	CORTINA_OTP_DRV_t * pOtpDrv;
	int rc;
	uint8_t HASH_ROT_available = 0;
#endif /* (ARM_ROTPK_LOCATION_ID == ARM_ROTPK_REGS_ID) */
#endif /* CORTINA_OTP_SUPPORT */

	assert(key_ptr != NULL);
	assert(key_len != NULL);
	assert(flags != NULL);
#ifdef CORTINA_OTP_SUPPORT
#if (ARM_ROTPK_LOCATION_ID == ARM_ROTPK_REGS_ID)
	pOtpDrv = cortina_otp_get_drv_instance(0);
	assert(pOtpDrv != NULL);
#endif /* (ARM_ROTPK_LOCATION_ID == ARM_ROTPK_REGS_ID) */
#endif /* CORTINA_OTP_SUPPORT */

	/* Copy the DER header */
	memcpy(rotpk_hash_der, rotpk_hash_hdr, rotpk_hash_hdr_len);
	dst = (uint8_t *)&rotpk_hash_der[rotpk_hash_hdr_len];

#if (ARM_ROTPK_LOCATION_ID == ARM_ROTPK_DEVEL_RSA_ID)
	memcpy(dst, arm_devel_rotpk_hash, SHA256_BYTES);
#elif (ARM_ROTPK_LOCATION_ID == ARM_ROTPK_REGS_ID)
#ifdef CORTINA_OTP_SUPPORT

	/* Has the Hash of ROT been programmed? */
	rc = cortina_otp_shadow_field_is_programmed(pOtpDrv, 
					 (uint32_t) OTP_DATA_hash_of_ROT_public_key_BSTART, 
					 (uint32_t) OTP_DATA_hash_of_ROT_public_key_BLEN, 
					 &HASH_ROT_available);
	/* something went wrong */
	if (rc) {
                TEST_INFO("Hash of ROTPK not programmed - something wrong\n"); 
                return 1;
        }

#if IMAGE_BL1
	// check OTP fixed param auth_override bit0, if 0, force BL1 to skip auth on BL2
	uint8_t auth_override = 0;
	cortina_otp_shadow_field_is_programmed(pOtpDrv,
					  (uint32_t) OTP_DATA_auth_override_BSTART,
					  (uint32_t) OTP_DATA_auth_override_BLEN,
					  &auth_override);
	if (auth_override & 1) {
          TEST_INFO("Skipping auth due to auth_override %d\n", auth_override);
          *flags = ROTPK_NOT_DEPLOYED;
	  return 0;
	}

#endif

	if(HASH_ROT_available == 0)
	 {
		/* ROTPK field is not programmed */
                 TEST_INFO("Hash of ROTPK not programmed\n");
                 *flags = ROTPK_NOT_DEPLOYED;
                 return 0;
	 }
	/* copy Hash of ROT Public Key from Secure OTP Shadow Memory */
	/* src and dst data is already byte aligned */
	cortina_otp_shadow_get_field(pOtpDrv, 
				(uint32_t) OTP_DATA_hash_of_ROT_public_key_BSTART, 
				(uint32_t) OTP_DATA_hash_of_ROT_public_key_BLEN, 
				(uintptr_t) dst);
#else
#error NO_CORTINA_OTP_SUPPORT_FOR_ROTPK_HASH
#endif /* CORTINA_OTP_SUPPORT */
#endif /* (ARM_ROTPK_LOCATION_ID == ARM_ROTPK_DEVEL_RSA_ID) */

	*key_ptr = (void *)rotpk_hash_der;
	*key_len = (unsigned int)sizeof(rotpk_hash_der);
	*flags = ROTPK_IS_HASH;
        TEST_INFO("set flag ROTPK_IS_HASH\n");
	return 0;
}

/*
 * Return the non-volatile counter value stored in the platform. The cookie
 * will contain the OID of the counter in the certificate.
 *
 * Return: 0 = success, Otherwise = error
 */
int plat_get_nv_ctr(void *cookie, unsigned int *nv_ctr)
{
#ifdef CORTINA_OTP_SUPPORT
	const char *oid;
	CORTINA_OTP_DRV_t * pOtpDrv;

	assert(cookie != NULL);
	assert(nv_ctr != NULL);
	pOtpDrv = cortina_otp_get_drv_instance(0);
	assert(pOtpDrv != NULL);

	oid = (const char *)cookie;
	if (strcmp(oid, TRUSTED_FW_NVCOUNTER_OID) == 0) {
		*nv_ctr = cortina_otp_shadow_get_trusted_min_ver(pOtpDrv);
	} else if (strcmp(oid, NON_TRUSTED_FW_NVCOUNTER_OID) == 0) {
		*nv_ctr = cortina_otp_shadow_get_non_trusted_min_ver(pOtpDrv);
	} else {
		return 1;
	}

	return 0;
#else /* CORTINA_OTP_SUPPORT */
	return 1;
#endif /* CORTINA_OTP_SUPPORT */
}

/*
 * Store a new non-volatile counter value. 
 *
 * Return: 0 = success, Otherwise = error
 */
int plat_set_nv_ctr(void *cookie, unsigned int nv_ctr)
{
#ifdef CORTINA_OTP_SUPPORT

	const char *oid;
	CORTINA_OTP_DRV_t * pOtpDrv;
#if IMAGE_BL1
	uint8_t bl1_otp_counter_allow_update = 0;
#endif /* IMAGE_BL1 */

	assert(cookie != NULL);
	pOtpDrv = cortina_otp_get_drv_instance(0);
	assert(pOtpDrv != NULL);

        oid = (const char *)cookie;
#if IMAGE_BL1
	/* check to see if we should write in BL1 */
	cortina_otp_shadow_field_is_programmed(pOtpDrv, 
					 (uint32_t) OTP_DATA_allow_bl1_nv_cntr_writes_BSTART, 
					 (uint32_t) OTP_DATA_allow_bl1_nv_cntr_writes_BLEN, 
					 &bl1_otp_counter_allow_update);
	/* We are in BL1 and update is not allowed.
	 * Therefore ignore update for now. it will be done later in BL2 instead */
	if(bl1_otp_counter_allow_update != 1) {
                TEST_INFO("bl1_otp_counter_allow_update is 0 not to update\n");
                return 0;
        }
	NOTICE("BL1: Update NV counter\n");
#endif /* IMAGE_BL1 */
        if (strcmp(oid, TRUSTED_FW_NVCOUNTER_OID) == 0) {
                TEST_INFO("to call cortina_otp_update_trusted_min_ver with nv_ctr %d\n", nv_ctr); 
                return cortina_otp_update_trusted_min_ver(pOtpDrv, nv_ctr);
        } else if (strcmp(oid, NON_TRUSTED_FW_NVCOUNTER_OID) == 0) {
                TEST_INFO("to call cortina_otp_update_non_trusted_min_ver with nv_ctr %d\n", nv_ctr);
                return cortina_otp_update_non_trusted_min_ver(pOtpDrv, nv_ctr);
        } else {
                TEST_INFO("plat_set_nv_ctr oid wrong\n");
                return 1;
        }
#else /* CORTINA_OTP_SUPPORT */
	return 1;
#endif /* CORTINA_OTP_SUPPORT */
}
