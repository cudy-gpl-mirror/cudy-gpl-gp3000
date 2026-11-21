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

#ifndef __BL1_CHECKPOINT_HOTFIX_H__
#define __BL1_CHECKPOINT_HOTFIX_H__


/* 16 bit aligned */
/* this is table of 8 hotfix checkpoint row entries.
 * each hotfix checkpoint row contains two columns.
 * the first column contains the checkpoint id (0 to 127), and a flag for hotfix code in OTP shadow or other SRAM space
 * the second column contains the coresponding hotfix offset (0 to 255) from the start 
 * of either Shadow OTP region # 2 space or a special memory address space.  
 * The special memory address space is, by default, the starting of SRAM free space, or
 * it can be hotfixed to point to anywhere in memory.
 * ID 0 is for un-defined. ID 127 is for invalid. 
 * If a hotfix code is not in OTP shadw, the hotfix code is followed by a signature to indicate the hotfix code is loaded. 
 * The hotfix code's entry point if in OTP shadow is caculated as OTP region 2 base + offset * 8.
 * If it is not in OTP shadw, the entry is caculated as hotfix code space base + offset * 32 + 4 (4 byte signature)
 * This allow hotfix code to be much bigger than the code otherwise in OTP shadow which has a very limited space.
 * The checkpoint returns 0 if hotfix code is not found or found but not matching the signature (not loaded in the hotfix special mem).
 * If the hotfix code is invoked, the checkpoint returns 1. The return status can be used to implement replacement checkpoints.
 * In BL1 code, a total of 126 valid checkpoints can be inserted, only upto 8 checkpoints can be activated, that is, provided with
 * their hotfix code either in OTP or in an other mem space which should be loaded in a way.
 * Such as, to load hotfix code into the other mem, a regular hotfix from OTP may be used. 
 * The above approach allows ROM code to be patched using
 * hotfix code found in either Shadow OTP space or other memory space.
 */

/* Reserved Tail Protected Region Start - HW Protected fixed region layout */
#define BL1_HOTFIX_CHECKPOINT_NUM_ENTRIES               8

#define BL1_HOTFIX_CHECKPOINT_ID_BLEN                   5
#define BL1_HOTFIX_CHECKPOINT_OFFSET_BLEN               7 
#define BL1_HOTFIX_CHECKPOINT_ENTRY_BLEN                (BL1_HOTFIX_CHECKPOINT_ID_BLEN + BL1_HOTFIX_CHECKPOINT_OFFSET_BLEN) 

#ifndef __ASSEMBLY__

typedef enum {
  CHECKPOINT_ID_UNDEF = 0,
  CHECKPOINT_ID_WATHDOG = 1,
  CHECKPOINT_ID_UART_INIT = 2,
  CHECKPOINT_ID_OTP_COPY = 3,
  CHECKPOINT_ID_NFLASH_INIT = 4,
  CHECKPOINT_ID_NFLASH_INIT_CONTINUE = 5,
  CHECKPOINT_ID_NFLASH_AFTER = 6,
  CHECKPOINT_ID_SFLASH_INIT = 7,
  CHECKPOINT_ID_SFLASH_INIT_CONTINUE = 8,
  CHECKPOINT_ID_SFLASH_AFTER = 9,
  CHECKPOINT_ID_MMC_INIT_BEFORE = 10,
  CHECKPOINT_ID_MMC_INIT = 11,
  CHECKPOINT_ID_MMC_AFTER = 12,
  CHECKPOINT_ID_BEFORE_BL2 = 13,
  CHECKPOINT_ID_AUTH_FAIL = 14,
  CHECKPOINT_ID_MULTIBOOT_NEXT_DEVICE = 15,
  CHECKPOINT_ID_SNANDFLASH_INIT = 16,
  CHECKPOINT_ID_SNANDFLASH_INIT_CONTINUE = 17,
  CHECKPOINT_ID_SNANDFLASH_AFTER = 18,
  CHECKPOINT_ID_HGU_EAXI_MASTER_INIT_BEFORE = 19,
  CHECKPOINT_ID_HGU_EAXI_MASTER_AFTER = 20,
#if defined(TEST_OTP) || defined(TEST_ON_CORTINA_ASIC_SIM)
  CHECKPOINT_ID_TEST1 = 28,
  CHECKPOINT_ID_TEST2 = 29,
  CHECKPOINT_ID_TEST3 = 30,
#endif
  CHECKPOINT_ID_INVALID = 0x1f
} BL1_hotfix_checkpoint_id_t;

#ifdef IMAGE_BL1
#define BL1_HOTFIX_CHECKPOINT(id, data) cortina_bl1_hotfix_checkpoint(id, data)
unsigned int cortina_bl1_hotfix_checkpoint(uint32_t id, void *data);
#else /* IMAGE_BL1 */
#define BL1_HOTFIX_CHECKPOINT(id,data)
#endif /* IMAGE_BL1 */

#endif /* __ASSEMBLY__ */

#endif /* __BL1_CHECKPOINT_HOTFIX_H__ */
