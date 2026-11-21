/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 */

#ifndef __RTK_TEE_SUPPORT_H
#define __RTK_TEE_SUPPORT_H
#define RTK_SEC_SERVICE_UUID \
                 { 0x1d90f7f0, 0xa93c, 0x4e39,  \
                 { 0x8d, 0x94, 0x13, 0xb7, 0xc2, 0x42, 0xcf, 0x46 } }
#define PTA_INVOKE_RTK_CMD_AES256				0x0
#define PTA_INVOKE_RTK_CMD_PRNG					0xA

/* CONFIG_RTK_OPTEE_VERIFIED_BOOT */
#define RTK_VERIFY_BOOT_SERVICE_UUID \
                 { 0x0384333f, 0xa93c, 0x4e39,  \
                 { 0x8d, 0x94, 0x13, 0xb7, 0xc2, 0x42, 0xcf, 0x46 } }
#define PTA_INVOKE_RTK_BL33_DECRYPT_AES256		0x0
#define PTA_INVOKE_RTK_BL33_RSA_PSS_VERIFY		0x1

#endif

