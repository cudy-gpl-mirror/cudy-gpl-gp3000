/*
 * Copyright (c) 2015-2016, Cortina Access. All rights reserved.
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

#ifndef __ARM_IO_STORAGE_H__
#define __ARM_IO_STORAGE_H__

typedef struct 
{
        uintptr_t *dev_handle;
        uintptr_t image_spec;
        int (*check)(const uintptr_t spec);
}
plat_io_policy_t;

int update_fip_spec(uint8_t boot_device, uint8_t boot_partition, plat_io_policy_t *policy);
#ifdef CORTINA_MULTIBOOT_SUPPORT
void plat_set_pinstrap_fip_device(void);
int plat_set_next_otp_fip_device();
#endif /* CORTINA_MULTIBOOT_SUPPORT */

#ifdef CORTINA_MULTIBOOT_ONE_SHOT_SUPPORT
#define OSB_2_BIT_BL1_SIG		0x00000002
#define OSB_2_BIT_BL2_SIG		0x00000001
#define OSB_2_BIT_DONE_SIG		0x00000003
#define OSB_2_BIT_SIG_BLEN		2
#define OSB_2_BIT_SIG_MASK		0x3

#define OSB_24_BIT_BL1_SIG		0x0062736F	/* ascii "osb" */
#define OSB_24_BIT_BL2_SIG		0x0062734F	/* ascii "Osb" */
#define OSB_24_BIT_DONE_SIG		0x0042534F	/* ascii "OSB" */
#define OSB_24_BIT_SIG_BLEN		24
#define OSB_24_BIT_SIG_MASK		0x00FFFFFF
#endif /* CORTINA_MULTIBOOT_ONE_SHOT_SUPPORT */

#endif /* __ARM_IO_STORAGE_H__ */

