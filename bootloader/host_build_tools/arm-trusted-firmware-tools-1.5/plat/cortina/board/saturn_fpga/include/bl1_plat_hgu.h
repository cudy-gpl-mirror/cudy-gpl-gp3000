/*
 * Copyright (c) 2017, Cortina Access. All rights reserved.
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

#ifndef __BL1_PLAT_HGU_H__
#define __BL1_PLAT_HGU_H__

#define SATURN_CPU_TYPE_SFU     0
#define SATURN_CPU_TYPE_SFP     1
#define SATURN_CPU_TYPE_HGU     2
#define SATURN_CPU_TYPE_BACKUP  3


typedef volatile union {
    struct {
        ca_uint32_t msg_data:            24;
        ca_uint32_t msg_type:            4 ; /* bits 2:2 */
        ca_uint32_t irq_ack_allow:       1 ;
        ca_uint32_t irq_ack_clear:       1 ;
        ca_uint32_t irq_allow:           1 ;
        ca_uint32_t irq_request:         1 ;
    } bf ;
    ca_uint32_t     wrd ;
} saturn_ipc_msg_t;

/* define some IPC MSG TYPES */
#define HGU_IPC_MSG_TYPE_SET_PC_AND_JUMP 	3
#define HGU_IPC_MSG_TYPE_BAD_MESSAGE 		4
#define HGU_IPC_MSG_TYPE_EAXI_READY 		7

/* define some BAD MESSAGE IDs which we pass via msg_data field */
#define HGU_IPC_BAD_MESSAGE_ID_INVALID_PC_JUMP_ADDR             1
#define HGU_IPC_BAD_MESSAGE_ID_INVALID_MESSAGE_TYPE             2

/* define some bit masks for the IPC MSG structure above */
#define HGU_IPC_MSG_DATA_MASK 0x00ffffff


uint8_t plat_saturn_in_hgu_mode();
uint8_t plat_saturn_in_backup_boot();

void plat_saturn_uart1_enable();
void plat_saturn_eaxi_init();
void plat_saturn_hgu_boot_main();

#endif /* !__BL1_PLAT_HGU_H__ */
