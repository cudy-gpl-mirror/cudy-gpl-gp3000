/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __CA_NI_H__
#define __CA_NI_H__
#define CA_NI_USE_BUILD_SKB 0       // must be 0
#define CA_NI_ALLOC_SCATTER_SIZE         75

#define  CA_ASSERT_RET(x, ret)    \
    do{ \
        if (!(x))  { \
            printk("%s() _%d_: assert %s fail\n", __func__, __LINE__, #x); \
            return ret; \
        } \
    }while(0)

//extern ca_uint8_t ca_ni_use_qm_eq_refill_int;
//extern ca_uint8_t ca_ni_use_qm_eq_refill_tasklet;
//extern ca_uint8_t ca_ni_use_build_skb;
extern ca_uint8_t ca_ni_cpu64_use_fbm;
extern ca_uint8_t ca_ni_alloc_scatter;
extern uint32 ca_aal_debug;

#endif