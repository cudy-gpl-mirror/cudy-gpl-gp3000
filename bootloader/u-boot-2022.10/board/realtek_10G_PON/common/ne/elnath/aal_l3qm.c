
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
/*
 * aal_l3qm.c: Hardware Abstraction Layer for Layer-3 Queue Manager to access hardware regsiters
 *
 */
#include <osal_soc.h>
#include <osal.h>
#include <aal_common.h>
#include <aal_port.h>
#include <aal_ni.h>
#include <aal_l3_te_cb.h>
#include <aal_l3qm.h>
#include <ca_aal_proc.h>
#include <aal_scfg.h>
#include <ca_ni.h>

#if defined(CONFIG_NE_PON)
#include "aal_pon.h"
#endif

#define CA_L3QM_MAGIC_NUMBER        0xDEADBEEF

#define CA_L3QM_AXI_TOP_DDR_NONCACHE_BIT        1
#define CA_L3QM_AXI_TOP_DDR_COHERENT_BIT        4

#define CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT        300

#define CA_L3QM_AXI_ATTRIBUTE_EQ_BASE       0       /* EQ0-15 */

#define CA_L3QM_AXI_ATTRIBUTE_CPU256_BASE   16      /* CPU256 port 0-31 */
#define CA_L3QM_AXI_ATTRIBUTE_CPU_BASE      48      /* CPU port 0-7 */

#define CA_L3QM_CPU_EPP_FIFO_SIZE_UINT      4       /* 4-FIFO entries */
#define CA_L3QM_CPU_REFILL_THS_BITS         8       /* 255 * 8(unit) = 2040 buffers */

int DQ_bid_start=0; 


#define EQ_POOL_SRAM_BUFFER_SIZE (512)          // 512B per SRAM buffer
#define EQ_POOL_SRAM_WAN_BUFFER_SIZE (128)      // 128,256,512B per SRAM buffer // ref CFG_ID_L3QM_WAN_SRAM_BUFFER_SIZE
#define EQ_POOL_SRAM_WAN_NUMBER (300)           // 300*128B                     // ref CFG_ID_L3QM_WAN_SRAM_BUFFER_COUNT
#define EQ_POOL_SRAM_SIZE   (0x800)                 // per each pool
#define EQ_POOL_DDR_SIZE    (0x100000)

#define EQ_PROFILE_DQ_LAN_ID        (12)                    /* PROFILE id == POOL id*/
#define EQ_PROFILE_DQ_WAN_ID        (13)                    /* PROFILE id == POOL id*/
#define EQ_PROFILE_DQ_SHARE_ID  (14)                    /* Shared by all NI ports */

#define EQ_BUFFER_ID_MAX                        (12288) /* HW spec*/
#define EQ_BUFFER_ID_NI_DRAM_MAX            (9000)  /* noncache reserved mem size, 2KB page*/

/* user defined constants should get from startup configs */
ca_uint8_t  l3qm_magic_marker_len = 0;          /* length of magic marker */
ca_uint8_t  l3qm_desc_size = 4;                 /* size of RX descriptor */
ca_uint8_t  l3qm_desc_per_epp = 1;              /* number of RX descriptor per EPP */
ca_uint32_t l3qm_pe0_1_port_count = 1;          /* PE0/PE1 port count */
ca_uint8_t  l3qm_pe0_1_voq_per_port = 8;            /* PE0/PE1 VoQ per port */
ca_uint32_t l3qm_pe0_1_epp_per_voq = 32;            /* PE0/PE1 EPP per VoQ */
ca_uint32_t l3qm_pe0_1_pool_bid_count = 128;        /* PE0/PE1 pool BID count */

ca_uint8_t  l3qm_eq0_sram_eq1_dram = 1;         /* EQ0 SRAM and EQ1 DRAM Mode*/
ca_uint8_t  l3qm_eq_share_pool_id = 1;          /* EQ0 SRAM and EQ1 DRAM Mode*/
ca_uint32_t l3qm_eq_sram_wan_buffer_size = EQ_POOL_SRAM_WAN_BUFFER_SIZE;
ca_uint32_t l3qm_eq_sram_wan_buffer_count = EQ_POOL_SRAM_WAN_NUMBER;
ca_uint32_t l3qm_eq_sram_wan_total_size = (EQ_POOL_SRAM_WAN_NUMBER*EQ_POOL_SRAM_WAN_BUFFER_SIZE);

ca_uint8_t  l3qm_qos_auto_threhsold_voq_num = 0;
ca_uint8_t  l3qm_qos_auto_threhsold_voq_prvt_buf_cnt = 0;

ca_uint32_t     l3qm_cpu_port_count = 6;                    /* CPU port count */
ca_uint8_t      l3qm_cpu_voq_per_port = 8;                      /* CPU VoQ per port */
ca_uint32_t     l3qm_cpu_epp_per_voq = 128;                     /* CPU EPP per VoQ */
ca_uint32_t     l3qm_cpu_pool_bid_count = 1500;                 /* CPU pool BID count */
ca_uint8_t  l3qm_cpu_cache_line_size = 64;          /* CPU cache line size */
ca_uint32_t l3qm_eq_buffer_align_size = 128;        /* EQ buffer align size */

ca_uint8_t  l3qm_deepq_test_with_sram = 0;          /* use sram to test DEEPQ function */
ca_uint8_t  l3qm_ace_test = 0;              /* do QM ACE test do not do dma sync by software */
ca_uint8_t  l3qm_main_hash_bm_test = 1;         /* do Main Hash Bench Mark test */ /*RTK always turn on this (sram mode)*/

ca_uint8_t  l3qm_cpu_port_used_by_pe[CA_NI_NUM_OF_CPU];
ca_uint32_t     epp64_paddr_start[8][8] = {0};
//EXPORT_SYMBOL(epp64_paddr_start);

ca_uint_t       sram_start_virtual_addr = 0xffffffff;   /* 64bit virtual address */
ca_uint_t       sram_cur_virtual_addr = 0xffffffff;             /* 64bit virtual address */
ca_uint32_t sram_start_phy_addr = 0x80000000;               /* must be 0x80000000 */
ca_uint8_t  l3qm_cpuepp_sram_enable[CFG_ID_RTK_CPUEPP_SRAM_MODE_ENABLE_LEN] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
ca_uint8_t  l3qm_dmalso_sram_enable[CFG_ID_RTK_DMALSO_SRAM_MODE_ENABLE_LEN] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

/* caculated from defined constants */
ca_uint8_t      l3qm_epp_size = 0;      /* EPP size in byte */

ca_uint32_t     l3qm_eq_profile_pe0_1_total_epp_count = 0;      /* PE0/PE1 total EPP count */

ca_uint32_t     l3qm_epp_profile_cpu_total_epp_count = 0;       /* CPU total EPP count */

ca_uint32_t l3qm_cpu_port_head_room_first = 32;     /* head room first for CPU port */

/* EQ(QM_CFG3/4_EQ%d) startup configures */
ca_uint32_t l3qm_eq_cfg3_values[CFG_ID_L3QM_EQ_CFG3_VALUES_LEN] =
{
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1a008017, 0x1a008017, 0x0, 0x0, 0x0, 0x0, 0x10, 0x10
};

ca_uint32_t     l3qm_eq_cfg4_values[CFG_ID_L3QM_EQ_CFG4_VALUES_LEN] =
{
    0x0, 0x1, 0x0, 0x1, 0x0, 0x1, 0x1, 0x1, 0x4, 0x4, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1
};

/* QM_AXI_ATTRIBUTE for EQ 0-15 */
ca_uint32_t l3qm_axi_attrib_eq_values[CFG_ID_L3QM_AXI_ATTRIB_EQ_VALUES_LEN] =
{
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x04000010, 0x04000010
};

/* QM_AXI_ATTRIBUTE for CPU EPP */
ca_uint32_t     l3qm_axi_attrib_cpu_epp_values[CFG_ID_L3QM_AXI_ATTRIB_CPU_EPP_VALUES_LEN] =
{
    0x12008060, 0x12008060, 0x12008060, 0x12008060, 0x12008060, 0x12008060, 0x12008060, 0x12008060
};

/* configuration parameters for PE0 need to be got from start config */
ca_uint8_t      l3qm_eq_profile_pe0_id = 0;                     /* EQ profile 0 */
ca_uint8_t      l3qm_eq_profile_pe0_rule = 1;                   /* use pool 0 for header */
ca_uint8_t      l3qm_eq_profile_pe0_use_fbm = 1;                /* use FBM pool */
ca_uint8_t      l3qm_eq_profile_pe0_pool0_eq_id = 1;            /* QM_EQ_PROFILE0.eqid0=1 */
ca_uint32_t     l3qm_eq_profile_pe0_pool0_bid_start = 0;        /* strating buffer id (must start from 0) */
ca_uint8_t      l3qm_eq_profile_pe0_pool0_fbm_pool_id = 0;      /* in SRAM */
ca_uint32_t l3qm_eq_profile_pe0_pool0_physical_start_address = 0xffffffff;  /* pool0 physical start address */
ca_uint8_t      l3qm_eq_profile_pe0_pool1_eq_id = 2;            /* QM_EQ_PROFILE1.eqid0=2 */
ca_uint32_t     l3qm_eq_profile_pe0_pool1_buf_count = 128;        /* number of  buffers for PE0 pool 1 */
ca_uint32_t     l3qm_eq_profile_pe0_pool1_bid_start = 128;       /* starting buffer id */
ca_uint32_t     l3qm_eq_profile_pe0_pool1_buf_sz = 2048;        /* pool 1 buffer size(payload) */
ca_uint8_t      l3qm_eq_profile_pe0_pool1_fbm_pool_id = 1;      /* in DDR */
ca_uint32_t l3qm_eq_profile_pe0_pool1_physical_start_address = 0xffffffff;  /* pool1 physical start address */
ca_uint8_t      l3qm_epp_profile_pe0_id = 0;                    /* EPP (RX FIFO) profile id */
ca_uint32_t     l3qm_epp_profile_pe0_sz = 0;                    /* RX FIFO size per entry, 4B */
ca_uint32_t     l3qm_epp_profile_pe0_cn = 0;                    /* 8 queues, 64 entries per queue */
ca_uint32_t     l3qm_epp_profile_pe0_start_addresses[8] = {0xffffffff};       /* VoQs 0-7 for PE0  */
ca_uint32_t l3qm_epp_profile_pe0_map_mode = 0;      /* QM_CPU_EPP0_CFG map_mode=0 */

/* configuration parameters for PE1 need to be got from start config */
ca_uint8_t      l3qm_eq_profile_pe1_id = 1;                     /* EQ profile 1 */
ca_uint8_t      l3qm_eq_profile_pe1_rule = 1;                   /* use pool 0 for header */
ca_uint8_t      l3qm_eq_profile_pe1_use_fbm = 1;                /* use FBM pool */
ca_uint8_t      l3qm_eq_profile_pe1_pool0_eq_id = 3;            /* QM_EQ_PROFILE1.eqid0=3 */
ca_uint32_t     l3qm_eq_profile_pe1_pool0_buf_count = 128;        /* 64 buffers for PE1 pool 0 */
ca_uint32_t     l3qm_eq_profile_pe1_pool0_bid_start = 256;      /* strating buffer id */
ca_uint32_t     l3qm_eq_profile_pe1_pool0_buf_sz = 256;         /* pool 0 header buffer size (B) */
ca_uint8_t      l3qm_eq_profile_pe1_pool0_fbm_pool_id = 1;      /* in SRAM */
ca_uint32_t l3qm_eq_profile_pe1_pool0_physical_start_address = 0xffffffff;  /* pool0 physical start address */
ca_uint8_t      l3qm_eq_profile_pe1_pool1_eq_id = 4;            /* QM_EQ_PROFILE1.eqid1=4 */
ca_uint32_t     l3qm_eq_profile_pe1_pool1_buf_count = 128;        /* 64 buffers for PE1 pool 1 */
ca_uint32_t     l3qm_eq_profile_pe1_pool1_bid_start = 384;   /* starting buffer id */
ca_uint32_t     l3qm_eq_profile_pe1_pool1_buf_sz = 2048;        /* pool 1 buffer size(payload) */
ca_uint8_t      l3qm_eq_profile_pe1_pool1_fbm_pool_id = 0;      /* in DDR */
ca_uint32_t l3qm_eq_profile_pe1_pool1_physical_start_address = 0xffffffff;  /* pool1 physical start address */
ca_uint32_t     l3qm_eq_profile_pe1_id_ports = 19;              /* LPID=19, CPU port 3 */
ca_uint8_t      l3qm_epp_profile_pe1_id = 1;                    /* EPP (RX FIFO) profile id */
ca_uint32_t     l3qm_epp_profile_pe1_sz = 0;                    /* RX FIFO size per entry, 4B */
ca_uint32_t     l3qm_epp_profile_pe1_cn = 0;                    /* 8 queues, 64 entries per queue */
ca_uint32_t     l3qm_epp_profile_pe1_start_addresses[8] = {0xffffffff};       /* VoQs 0-7 for PE1  */
ca_uint32_t     l3qm_epp_profile_pe1_map_mode = 0;              /* QM_CPU_EPP1_CFG map_mode=0 */

/* configuration parameters for CPU need to be got from start config */
ca_uint8_t      l3qm_eq_profile_cpu_id = 4;                     /* EQ profile 2 */
ca_uint8_t      l3qm_eq_profile_cpu_rule = 0;                   /* use pool 0 then pool 1 */
ca_uint8_t      l3qm_eq_profile_cpu_use_fbm = 0;                /* do not use FBM pool */
ca_uint8_t      l3qm_eq_profile_cpu_pool0_eq_id = 8;            /* QM_EQ_PROFILE2.eqid0=5 */
ca_uint32_t     l3qm_eq_profile_cpu_pool0_buf_count = 1500;        /* 1024 buffers for CPU pool 0 */
ca_uint32_t     l3qm_eq_profile_cpu_pool0_bid_start = 0;     /* strating buffer id */
ca_uint32_t     l3qm_eq_profile_cpu_pool0_buf_sz = 2048;        /* pool 0 buffer size */
ca_uint8_t      l3qm_eq_profile_cpu_pool0_fbm_pool_id = 0;      /* SRAM */
ca_uint32_t l3qm_eq_profile_cpu_pool0_physical_start_address = 0xffffffff;  /* pool0 physical start address */
ca_uint8_t      l3qm_eq_profile_cpu_pool1_eq_id = 9;            /* QM_EQ_PROFILE2.eqid0=6 */
ca_uint32_t     l3qm_eq_profile_cpu_pool1_buf_count = 1500;        /* 2048 buffers for CPU pool 1 */
ca_uint32_t     l3qm_eq_profile_cpu_pool1_bid_start = 1500;     /* starting buffer id */
ca_uint32_t     l3qm_eq_profile_cpu_pool1_buf_sz = 2048;        /* pool 1 buffer size */
ca_uint8_t      l3qm_eq_profile_cpu_pool1_fbm_pool_id = 0;      /* SRAM */
ca_uint32_t l3qm_eq_profile_cpu_pool1_physical_start_address = 0xffffffff;  /* pool1 physical start address */
ca_uint8_t      l3qm_epp_profile_cpu_id = 2;                            /* EPP (RX FIFO) profile id */

ca_uint32_t     l3qm_epp_profile_cpu_sz = 0;                    /* RX FIFO size per entry, 8B */
ca_uint32_t     l3qm_epp_profile_cpu_cn = 256 * 8 * 6;          /* CPU ports 0-5, 6 ports, 8 queues per port, 256 entries per queue */

ca_uint32_t     l3qm_epp_profile_cpu_map_mode = 0;              /* QM_CPU_EPP1_CFG map_mode=0 */

/* configuration parameters for Deep Queue need to be got from start config, only use pool0 and no EPP profile */
ca_uint8_t      l3qm_eq_profile_dq_id = 3;                     /* EQ profile 3 */
ca_uint8_t      l3qm_eq_profile_dq_rule = 0;                   /* use pool 0 then pool 1 */
ca_uint8_t      l3qm_eq_profile_dq_pool0_eq_id = 8;            /* QM_EQ_PROFILE3.eqid0=0, eqid1=0 */
ca_uint32_t     l3qm_eq_profile_dq_pool0_buf_count = 4096;        /* 8192 buffers for DQ pool 0 */
ca_uint32_t     l3qm_eq_profile_dq_pool0_bid_start = 7608;     /* strating buffer id */
ca_uint32_t     l3qm_eq_profile_dq_pool0_buf_sz = 2048;        /* pool 0 buffer size */
ca_uint32_t     l3qm_eq_profile_dq_pool0_start_physical_addr = 0xffffffff;      /* QM_CFG0_EQ0.phy_addr_start = DDR addess of DQ EQ pool */
ca_uint8_t      l3qm_eq_profile_dq_pool0_fbm_pool_id = 7;      /* DDR (pushed into FBM) */
ca_uint8_t      l3qm_eq_profile_dq_pool1_eq_id = 9;            /* QM_EQ_PROFILE3.eqid0=0, eqid1=0 */
ca_uint32_t     l3qm_eq_profile_dq_pool1_buf_count = 9000;        /* 8192 buffers for DQ pool 0 */
ca_uint32_t     l3qm_eq_profile_dq_pool1_bid_start = 11704;    /* strating buffer id */
ca_uint32_t     l3qm_eq_profile_dq_pool1_buf_sz = 2048;        /* pool 0 buffer size */
ca_uint32_t     l3qm_eq_profile_dq_pool1_start_physical_addr = 0xffffffff;      /* QM_CFG0_EQ0.phy_addr_start = DDR addess of DQ EQ pool */
ca_uint8_t      l3qm_eq_profile_dq_pool1_fbm_pool_id = 7;      /* DDR (pushed into FBM) */

/* configuration parameters for main hash table need to be got from start config */
ca_uint32_t l3fe_main_hash_overflow_hash_fib_length = 32;       /* FIB length of main hash overflow hash table */
ca_uint32_t l3fe_main_hash_overflow_hash_fib_count  = 32;       /* number of FIB of main hash overflow hash table */
ca_uint32_t l3fe_main_hash_default_hash_fib_length  = 32;       /* FIB length of main hash default hash table */
ca_uint32_t l3fe_main_hash_default_hash_fib_count   = 32;       /* number of FIB of main hash default hash table */
ca_uint32_t l3fe_main_hash_action_cache_fib_length  = 32;       /* FIB length of main hash action cache table */
ca_uint32_t l3fe_main_hash_action_cache_fib_count   = 2048;     /* number of FIB of main hash action cache table */
ca_uint32_t l3fe_main_hash_table_entry_length   = 4;        /* length of main hash table entry */
ca_uint32_t l3fe_main_hash_table_entry_count    = 65536;    /* count of main hash table entry */
ca_uint32_t l3fe_main_hash_action_table_entry_length= 32;       /* length of main hash action table entry */
ca_uint32_t l3fe_main_hash_action_table_entry_count = 65536;    /* count of main hash action table entry */

uint8_t           l3fe_main_hash_flow_cache_fib_internal_sram_mode = 0;
uint8_t           l3fe_main_hash_flow_default_fib_internal_reg_mode = 0;

uint32_t        sram_dts_conf_size = 0;                             /* HW support 256KB - dtsi define 240KB are avaliable for NE */
uint32_t        sram_cur_used_size = 0;                             /* init as BL2 16B are reserved in dtsi*/

/* SRAM/DRR addresses calculated from startup config */
/* virtual address should be 64-bit long */
/* SRAM/DRR addresses calculated from startup config */
/* virtual addresses in DDR non-cache area should be 64-bit long */
ca_uint_t     l3qm_eq_profile_dq_pool0_buff_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_dq_pool1_buff_start_addr = 0xffffffff;
ca_uint_t     l3fe_main_hash_table_start_addr = 0xffffffff;
ca_uint_t     l3fe_main_action_table_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_pe0_pool1_buff_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_pe1_pool1_buff_start_addr = 0xffffffff;

/* physical addresses in DDR non-cache area should be 32-bit long */
ca_uint32_t     l3qm_eq_profile_dq_pool0_buff_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_dq_pool1_buff_phy_addr = 0xffffffff;
ca_uint32_t     l3fe_main_hash_table_phy_addr = 0xffffffff;
ca_uint32_t     l3fe_main_action_table_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_pe0_pool1_buff_phy_addr = 0xffffffff;
ca_uint32_t     l3qm_eq_profile_pe1_pool1_buff_phy_addr = 0xffffffff;

/* virtual addresses in DDR cache coherent area should be 64-bit long */
ca_uint_t     l3qm_eq_profile_cpu_pool0_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu_pool1_start_addr = 0xffffffff;
ca_uint_t     l3qm_epp_profile_cpu_rx_fifo_start_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu_pool0_phy_addr = 0xffffffff;
ca_uint_t     l3qm_eq_profile_cpu_pool1_phy_addr = 0xffffffff;
ca_uint_t     l3qm_epp_profile_cpu_rx_fifo_phy_addr = 0xffffffff;

/* virtual addresses in SRAM area should be 64-bit long */
ca_uint_t     l3fe_main_hash_overflow_hash_fib_start_addr = 0xffffffff;
ca_uint_t     l3fe_main_hash_default_hash_fib_start_addr = 0xffffffff;
ca_uint_t     l3fe_main_hash_action_cache_fib_start_addr = 0xffffffff;

/* physical address in SRAM area should be 32-bit long */
ca_uint32_t     l3fe_main_hash_overflow_hash_fib_phy_addr = 0xffffffff;
ca_uint32_t     l3fe_main_hash_default_hash_fib_phy_addr = 0xffffffff;
ca_uint32_t     l3fe_main_hash_action_cache_fib_phy_addr = 0xffffffff;

//spinlock_t l3qm_epp64_inte_lock;

ca_uint8_t l3qm_jumbo_buf_packing_size = 0;
ca_uint8_t l3qm_jumbo_buf_pingpong_eqs = 0;
ca_uint8_t l3qm_ldpid_to_pon_port_map_offset = 0;

aal_l3qm_eq_cfg_t l3qm_eq_cfg_cache[QM_QM_CFG2_EQ0_COUNT];
//EXPORT_SYMBOL(l3qm_eq_cfg_cache);

ca_uint32_t l3qm_eq_poo0_lan_port_free_buf = 0;
ca_uint32_t l3qm_eq_poo0_wan_port_free_buf = 0;
extern ca_uint8_t  l3te_voq_profile_sel[__AAL_L3_TE_CB_PORT_NUM][8];
extern ca_uint8_t  l3te_port_profile_sel[__AAL_L3_TE_CB_PORT_NUM];

void aal_l3qm_init_jumbo_buf(void)
{
    ca_uint32_t reg_off;
    QM_QM_BUFFER_CNTL_t buf_cntl;
    QM_QM_EQ_PROFILE0_t eq_profile;
    NI_HV_GLB_SPARE_t ni_glb_stor_cfg;

    buf_cntl.wrd = CA_NE_REG_READ(QM_QM_BUFFER_CNTL);
    buf_cntl.bf.jumbo_buffer_en = l3qm_jumbo_buf_packing_size;
    buf_cntl.bf.alternate_eq_en = l3qm_jumbo_buf_pingpong_eqs;
    CA_NE_REG_WRITE(buf_cntl.wrd, QM_QM_BUFFER_CNTL);

    /* when alternate EQ is enabled DeepQ should be rule=1 */
    if (buf_cntl.bf.alternate_eq_en) {
        reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * l3qm_eq_profile_dq_id);
        eq_profile.wrd = CA_NE_REG_READ(reg_off);
        eq_profile.bf.rule = 1;
        CA_NE_REG_WRITE(eq_profile.wrd, reg_off);
        aal_l3qm_load_eq_config();
    }

    ni_glb_stor_cfg.wrd = CA_NE_REG_READ(NI_HV_GLB_SPARE);
    ni_glb_stor_cfg.wrd &= 0xFF00;
    ni_glb_stor_cfg.wrd |= (0x55);          //p10_p13 tm to ni fifo threshold
        
    if(l3qm_jumbo_buf_packing_size) {
        ca_uint32_t bypassLen = 0;

        if(l3qm_jumbo_buf_packing_size==1)
            bypassLen = 2048;
        else if(l3qm_jumbo_buf_packing_size==2)
            bypassLen = 4096;
        else //if(l3qm_jumbo_buf_packing_size==3)
            bypassLen = 16384;

        // qm_bypass_pktlen(N) = packet size(M) + 4B CRC + 8B hdr_a + 8B hdr_cpu
        bypassLen -= 24;
        
        ni_glb_stor_cfg.wrd |= (1<<31);         //qm_jbbuf_en
        ni_glb_stor_cfg.wrd |= (bypassLen<<16); //qm_bypass_pktlen
    }

    CA_NE_REG_WRITE(ni_glb_stor_cfg.wrd, NI_HV_GLB_SPARE);

}

void aal_l3qm_init_upper_ldpid_map(void)
{
    // QM_QM_UPPER_LDPID_MAP is removed.
}

void aal_l3qm_dump_registers(void)
{
        uint32     reg;
        uint32     value;

        PRINTF("====================================L3 QM=================================\n");
        for (reg = QM_QM_REVISION; reg <= QM_QM_SPARE_STS; reg+=4) {
                value = CA_NE_REG_READ(reg);
                PRINTF("register=0x%x, value=0x%x\n", reg, value);
        }
}

int aal_l3qm_get_ace_test(void)
{
    return l3qm_ace_test;
}

static void aal_l3qm_get_axi_attrib(int entry, QM_QM_AXI_ATTRIBUTE_DATA0_t *attrib_data0, QM_QM_AXI_ATTRIBUTE_DATA1_t *attrib_data1)
{
        int i;
        QM_QM_AXI_ATTRIBUTE_ACCESS_t    attrib;

    attrib.wrd = 0;
        attrib.bf.access = 1;
        attrib.bf.rbw = 0;
        attrib.bf.ADDR = entry;
    CA_NE_REG_WRITE(attrib.wrd, QM_QM_AXI_ATTRIBUTE_ACCESS);
        for (i = 0; i < CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT; i++) {
                attrib.wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_ACCESS);
                if (!attrib.bf.access)
                        break;
        }
        if (i == CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT) {
                PRINTF("%s: CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT at entry=%d!!!\n", __func__, entry);
                return;
        }

        attrib_data0->wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_DATA0);
        attrib_data1->wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_DATA1);

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("[%d] data0=0x%x, data1=0x%x\n", entry, attrib_data0->wrd, attrib_data1->wrd);
        PRINTF("     qos=%d, cache=%d, snoop=%d, bar=%d, domain=%d, user=%d, prot=%d\n", attrib_data0->bf.qos, attrib_data0->bf.cache, attrib_data0->bf.snoop,
                attrib_data0->bf.bar, attrib_data0->bf.domain, attrib_data0->bf.user, attrib_data0->bf.prot);

        PRINTF("     axi_top_bit=0x%x, ace_cmd=%d\n", attrib_data0->bf.axi_top_bit | (attrib_data1->bf.axi_top_bit << 6), attrib_data0->bf.ace_cmd);
    }
}

int aal_l3qm_get_cpuepp_axi_attrib(int cpu_port)
{
    QM_QM_AXI_ATTRIBUTE_DATA0_t attrib_data0;
    QM_QM_AXI_ATTRIBUTE_DATA1_t attrib_data1;

    aal_l3qm_get_axi_attrib(CA_L3QM_AXI_ATTRIBUTE_CPU_BASE + cpu_port, &attrib_data0, &attrib_data1);

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: cpu_port=%d, axi_top_bit=%d\n", __func__,  cpu_port, attrib_data0.bf.axi_top_bit | (attrib_data1.bf.axi_top_bit << 6));
    }
    return attrib_data0.bf.axi_top_bit | (attrib_data1.bf.axi_top_bit << 6);
}

int aal_l3qm_get_eq_pool_axi_top_8bit(int eqid)
{
    QM_QM_CFG4_EQ0_t    cfg4_eq;

    cfg4_eq.wrd = CA_NE_REG_READ(QM_QM_CFG4_EQ0 + eqid * QM_QM_CFG4_EQ0_STRIDE);

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: eqid=%d, axi_top_bit=%d\n", __func__,  eqid, cfg4_eq.bf.axi_top_bit);
    }

    return cfg4_eq.bf.axi_top_bit;
}

int aal_l3qm_get_cpu_port_head_room_first(void)
{
    return l3qm_cpu_port_head_room_first;
}

int aal_l3qm_eq_get_cpu_rule(void)
{
    return l3qm_eq_profile_cpu_rule;
}

int aal_l3qm_eq_get_cpu_use_fbm(void)
{
        return l3qm_eq_profile_cpu_use_fbm;
}

int aal_l3qm_eq_get_cpu_pool0_buf_size(void)
{
    return l3qm_eq_profile_cpu_pool0_buf_sz;
}

int aal_l3qm_eq_get_desc_per_epp(void)
{
    return l3qm_desc_per_epp;
}

int aal_l3qm_eq_get_desc_size(void)
{
    return l3qm_desc_size;
}

int aal_l3qm_eq_get_max_write_ptr(void)
{
    return (l3qm_cpu_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp);
}

int aal_l3qm_eq_get_cpu_pool_bid_count(void)
{
    return l3qm_cpu_pool_bid_count;
}

void aal_l3qm_eq_get_cpu_pool0_1_start_addr(ca_uint_t *cpu_pool0_start_addr, ca_uint_t *cpu_pool1_start_addr)
{
    *cpu_pool0_start_addr = l3qm_eq_profile_cpu_pool0_start_addr;
    *cpu_pool1_start_addr = l3qm_eq_profile_cpu_pool1_start_addr;
}

void aal_l3qm_eq_get_cpu_pool0_1_eq_id(ca_uint8_t *cpu_pool0_eq_id, ca_uint8_t *cpu_pool1_eq_id)
{
    *cpu_pool0_eq_id = l3qm_eq_profile_cpu_pool0_eq_id;
    *cpu_pool1_eq_id = l3qm_eq_profile_cpu_pool1_eq_id;
}

void aal_l3qm_eq_get_cpu_pool0_info(ca_uint_t *virt_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
       *virt_start_addr = l3qm_eq_profile_cpu_pool0_start_addr;
       *entry_size = l3qm_eq_profile_cpu_pool0_buf_sz;
       *entry_count = l3qm_eq_profile_cpu_pool0_buf_count;
       *fbm_pool_id = l3qm_eq_profile_cpu_pool0_fbm_pool_id;
}

void aal_l3qm_eq_get_cpu_pool1_info(ca_uint_t *virt_start_addr, ca_uint32_t *entry_size, ca_uint32_t *entry_count, ca_uint8_t *fbm_pool_id)
{
        *virt_start_addr = l3qm_eq_profile_cpu_pool1_start_addr;
        *entry_size = l3qm_eq_profile_cpu_pool1_buf_sz;
        *entry_count = l3qm_eq_profile_cpu_pool1_buf_count;
        *fbm_pool_id = l3qm_eq_profile_cpu_pool1_fbm_pool_id;
}

void aal_l3qm_get_l3fe_main_hash_action_info(aal_l3qm_l3fe_main_hash_info_t *l3fe_main_hash_info)
{
        l3fe_main_hash_info->overflow_action_start_virt_addr = l3fe_main_hash_overflow_hash_fib_start_addr;
        l3fe_main_hash_info->default_action_start_virt_addr  = l3fe_main_hash_default_hash_fib_start_addr;
        l3fe_main_hash_info->action_cache_start_virt_addr    = l3fe_main_hash_action_cache_fib_start_addr;
        l3fe_main_hash_info->hash_table_start_virt_addr      = l3fe_main_hash_table_start_addr;
        l3fe_main_hash_info->action_table_start_virt_addr    = l3fe_main_action_table_start_addr;
        l3fe_main_hash_info->overflow_action_start_phy_addr  = l3fe_main_hash_overflow_hash_fib_phy_addr;
        l3fe_main_hash_info->default_action_start_phy_addr   = l3fe_main_hash_default_hash_fib_phy_addr;
        l3fe_main_hash_info->action_cache_start_phy_addr     = l3fe_main_hash_action_cache_fib_phy_addr;
        l3fe_main_hash_info->hash_table_start_phy_addr       = l3fe_main_hash_table_phy_addr;
        l3fe_main_hash_info->action_table_start_phy_addr     = l3fe_main_action_table_phy_addr;
        l3fe_main_hash_info->overflow_hash_fib_length        = l3fe_main_hash_overflow_hash_fib_length;
        l3fe_main_hash_info->overflow_hash_fib_count         = l3fe_main_hash_overflow_hash_fib_count;
        l3fe_main_hash_info->default_hash_fib_length         = l3fe_main_hash_default_hash_fib_length;
        l3fe_main_hash_info->default_hash_fib_count      = l3fe_main_hash_default_hash_fib_count;
        l3fe_main_hash_info->action_cache_fib_length         = l3fe_main_hash_action_cache_fib_length;
        l3fe_main_hash_info->action_cache_fib_count      = l3fe_main_hash_action_cache_fib_count;
        l3fe_main_hash_info->table_entry_length              = l3fe_main_hash_table_entry_length;
        l3fe_main_hash_info->table_entry_count               = l3fe_main_hash_table_entry_count;
        l3fe_main_hash_info->action_table_entry_length       = l3fe_main_hash_action_table_entry_length;
        l3fe_main_hash_info->action_table_entry_count        = l3fe_main_hash_action_table_entry_count;
    l3fe_main_hash_info->main_hash_bm_test = l3qm_main_hash_bm_test;
}

void aal_l3qm_set_l3fe_main_hash(ca_uint_t main_hash_table_saddr, ca_uint32_t main_hash_table_paddr,
                    ca_uint_t main_hash_action_saddr, ca_uint32_t main_hash_action_paddr)
{
    l3fe_main_hash_table_start_addr = main_hash_table_saddr;
    l3fe_main_action_table_start_addr = main_hash_action_saddr;
    l3fe_main_hash_table_phy_addr = main_hash_table_paddr;
    l3fe_main_action_table_phy_addr = main_hash_action_paddr;
}

void aal_l3qm_set_rx_read_ptr(ca_uint16_t cpu_port, ca_uint8_t voq, ca_uint32_t value)
{
    QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_t  rdptr;

    rdptr.wrd = 0;
    rdptr.bf.rptr = value;

    CA_NE_REG_WRITE(rdptr.wrd, QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR + (cpu_port * l3qm_cpu_voq_per_port + voq) * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_STRIDE);
}

ca_uint32_t aal_l3qm_get_rx_read_ptr(ca_uint16_t cpu_port, ca_uint8_t voq)
{
        QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_t      rdptr;

    /* 64 instances 8 CPU ports each port has 8 VOQs */
    rdptr.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR + (cpu_port * l3qm_cpu_voq_per_port + voq) * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_STRIDE);

    return rdptr.bf.rptr;
}

ca_uint32_t aal_l3qm_get_rx_write_ptr(ca_uint16_t cpu_port, ca_uint8_t voq, ca_uint16_t *count)
{
    QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR_t  wrptr;
    ca_uint32_t rdptr;
    int epp_size, size;

    /* 64 instances 8 CPU ports each port has 8 VOQs */
    wrptr.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR + (cpu_port * l3qm_cpu_voq_per_port + voq) * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR_STRIDE);

    rdptr = aal_l3qm_get_rx_read_ptr(cpu_port, voq);

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: wrptr.bf.wptr=%d, rdptr=%d\n", __func__, wrptr.bf.wptr, rdptr);
    }

    /* take care of wraping around case */
    epp_size = l3qm_desc_size * l3qm_desc_per_epp;
    if (wrptr.bf.wptr >= rdptr) {
        *count = (wrptr.bf.wptr - rdptr) / epp_size;
    }
    else {
        size = aal_l3qm_eq_get_max_write_ptr();
        *count = ((size - rdptr) + wrptr.bf.wptr) / epp_size;
        if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: max write pointer=%d\n", __func__, size);
        }
    }

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: wrptr.bf.wptr=%d, rdptr=%d, count=%d\n", __func__, wrptr.bf.wptr, rdptr, *count);
    }

    return wrptr.bf.wptr;
}


ca_uint32_t aal_l3qm_get_rx_start_addr(ca_uint16_t cpu_port, ca_uint8_t voq)
{

    return epp64_paddr_start[cpu_port][voq];
    
/*  QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_t paddr_start;

    paddr_start.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START + (cpu_port * l3qm_cpu_voq_per_port + voq) *
                       QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE);

        if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: cpu_port=%d, voq=%d, paddr_start.bf.phy_addr=0x%x\n", __func__, cpu_port, voq, paddr_start.bf.phy_addr);
    }

    return paddr_start.bf.phy_addr;*/
}
//EXPORT_SYMBOL(aal_l3qm_get_rx_start_addr);


ca_uint32_t aal_l3qm_get_rx_status0(void)
{
    /* get CPU EPP FIFO (VoQ#31-0) status */
    QM_QM_CPU_EPP_STATUS0_t status0;

    status0.wrd = CA_NE_REG_READ(QM_QM_CPU_EPP_STATUS0);

    return status0.bf.status;
}


ca_uint32_t aal_l3qm_get_rx_status1(void)
{
        /* get CPU EPP FIFO (VoQ#63-32) status */
        QM_QM_CPU_EPP_STATUS1_t status1;

        status1.wrd = CA_NE_REG_READ(QM_QM_CPU_EPP_STATUS1);

        return status1.bf.status;
}

ca_uint16_t aal_l3qm_get_inactive_bid_cntr(int eqid)
{
    int j;
    QM_QM_EQM_PA_REQ0_t eqm_pa_req;

    eqm_pa_req.wrd = 0;
    for (j = 0; j < L3QM_INACTIVE_BID_CNTR; j++) {
        eqm_pa_req.wrd = CA_NE_REG_READ(QM_QM_EQM_PA_REQ0 + QM_QM_EQM_PA_REQ0_STRIDE * eqid);
        if (eqm_pa_req.bf.req) {
            break;
        }
    }

    if (j == L3QM_INACTIVE_BID_CNTR) {
        if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: EQM is not ready for push physical address!!!\n", __func__);
        }
        return 0;
    }

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: eqid=%d, bid_cntr=%d\n", __func__, eqid, eqm_pa_req.bf.inactive_bid_cntr);
    }
    return eqm_pa_req.bf.inactive_bid_cntr;
}

void aal_l3qm_load_eq_config(void)
{
    /* load the configuration */
    CA_NE_REG_WRITE(0x05102013, QM_QM_HDM_WRITE_PROTECTION);
    CA_NE_REG_WRITE(0xffff, QM_QM_EQ_CFG_LOAD);
    CA_NE_REG_WRITE(0, QM_QM_EQ_CFG_LOAD);
    CA_NE_REG_WRITE(0, QM_QM_HDM_WRITE_PROTECTION);
}

void aal_l3qm_load_epp_config(void)
{
}

ca_uint32_t aal_l3qm_check_cpu_push_ready(int cpu_port)
{
    int i;
    QM_QM_CPU_PUSH_RDY0_t   cpu_push_rdy;

    /* check ready bit */
    for (i = 0; i < L3QM_CPU_PUSH_RDY_TIMEOUT; i++) {
        cpu_push_rdy.wrd = CA_NE_REG_READ(QM_QM_CPU_PUSH_RDY0 + (QM_QM_CPU_PUSH_RDY0_STRIDE * cpu_port));
        if (cpu_push_rdy.bf.rdy) {
            break;
        }
    }
    if (i == L3QM_CPU_PUSH_RDY_TIMEOUT) {
        if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: EQM is not ready for push physical address!!!\n", __func__);
        }
        return 0;
    }
    return 1;
}


void aal_l3qm_set_cpu_push_paddr(int cpu_port, int eqid, ca_uint32_t phy_addr)
{
    QM_QM_CPU_PUSH_PADDR0_t cpu_push_paddr;

    cpu_push_paddr.bf.addr = phy_addr >> CA_L3QM_PHY_ADDR_SHIFT;
    cpu_push_paddr.bf.eqid = eqid;

    if (unlikely(ca_aal_debug & AAL_DBG_L3QM)) {
        PRINTF("%s: cpu_port=%d, eqid=%d, phy_addr=0x%x, cpu_push_paddr=0x%x\n", __func__, cpu_port, eqid, phy_addr, cpu_push_paddr.wrd);
    }

#if defined(CONFIG_LUNA_G3_SERIES)
    CA_NE_REG_WRITE_ASM(cpu_push_paddr.wrd, QM_QM_CPU_PUSH_PADDR0 + (QM_QM_CPU_PUSH_PADDR0_STRIDE * cpu_port));
#else
    CA_NE_REG_WRITE(cpu_push_paddr.wrd, QM_QM_CPU_PUSH_PADDR0 + (QM_QM_CPU_PUSH_PADDR0_STRIDE * cpu_port));
#endif

}

void aal_l3qm_set_epp_axi_attrib(int entry, ca_uint32_t data0)
{
    int i;
    QM_QM_AXI_ATTRIBUTE_ACCESS_t    attrib;
    QM_QM_AXI_ATTRIBUTE_DATA0_t attrib_data0;

    attrib_data0.wrd = data0;
    CA_NE_REG_WRITE(attrib_data0.wrd, QM_QM_AXI_ATTRIBUTE_DATA0);

    attrib.bf.access = 1;
    attrib.bf.rbw = 1;
    attrib.bf.ADDR = entry;
    CA_NE_REG_WRITE(attrib.wrd, QM_QM_AXI_ATTRIBUTE_ACCESS);
    for (i = 0; i < CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT; i++) {
        attrib.wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_ACCESS);
        if (!attrib.bf.access)
            break;
    }
    if (i == CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT) {
        PRINTF("%s: CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT!!!\n", __func__);
        return;
    }

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: attrib_data0.wrd=0x%x, QM_QM_AXI_ATTRIBUTE_DATA0=0x%x\n", __func__, attrib_data0.wrd, QM_QM_AXI_ATTRIBUTE_DATA0);
    }
}

#define AAL_L3QM_MAX_BID    16384
typedef struct aal_l3qm_bid_entry {
    ca_uint8_t  eqid;
    ca_uint16_t bid_start;
    ca_uint16_t bid_count;
} aal_l3qm_bid_entry_t;
static int aal_l3qm_bid_entry_count = 0;
static aal_l3qm_bid_entry_t bid_entry[QM_QM_CFG1_EQ0_COUNT];
static void aal_l3qm_add_bid(ca_uint8_t eqid, ca_uint16_t bid_start, ca_uint16_t bid_count)
{
    int i;

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: aal_l3qm_bid_entry_count=%d\n", __func__, aal_l3qm_bid_entry_count);
    }
    if (aal_l3qm_bid_entry_count >= QM_QM_CFG1_EQ0_COUNT) {
        PRINTF("%s: aal_l3qm_bid_entry_count=%d bid table is full!!\n", __func__, aal_l3qm_bid_entry_count);
        return;
    }

    /* find the index for new added bid */
    i = 0;
    while (i < aal_l3qm_bid_entry_count) {
        if (bid_start < bid_entry[i].bid_start) {
            if (ca_aal_debug & AAL_DBG_L3QM) {
                PRINTF("%s: aal_l3qm_bid_entry_count - i=%d\n", __func__, aal_l3qm_bid_entry_count - i);
            }
            memcpy(&(bid_entry[i+1]), &(bid_entry[i]), (aal_l3qm_bid_entry_count - i) * sizeof(aal_l3qm_bid_entry_t));
            break;
        }
        i++;
    }
    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: i=%d, eqid=%d, bid_start=%d, bid_count=%d\n", __func__, i, eqid, bid_start, bid_count);
    }
    bid_entry[i].eqid = eqid;
    bid_entry[i].bid_start = bid_start;
    bid_entry[i].bid_count = bid_count;
    aal_l3qm_bid_entry_count++;
}
ca_status_t aal_l3qm_check_bid_start(ca_device_id_t device_id)
{
    int i;
    ca_uint16_t next_bid_start;
    ca_uint16_t total_bid_count;
    QM_QM_CFG1_EQ0_t cfg1_eq;

    next_bid_start = 0;

    aal_l3qm_bid_entry_count = 0;
    memset(&(bid_entry[0]), 0, sizeof(bid_entry));

    for (i = 0; i < QM_QM_CFG1_EQ0_COUNT; i++) {
        cfg1_eq.wrd = CA_NE_REG_READ(QM_QM_CFG1_EQ0 + i * QM_QM_CFG1_EQ0_STRIDE);
        if (cfg1_eq.bf.total_buffer_num == 0)       /* skip the eqid not in use */
            continue;

        aal_l3qm_add_bid(i, cfg1_eq.bf.bid_start, cfg1_eq.bf.total_buffer_num);
    }
#if 0//!defined(CONFIG_LUNA_G3_SERIES)
    PRINTF("%s: aal_l3qm_bid_entry_count=%d\n", __func__, aal_l3qm_bid_entry_count);
    total_bid_count = 0;
    for (i = 0; i < aal_l3qm_bid_entry_count; i++) {
        PRINTF("[%d]: eqid=%d bid_start=%d bid_count=%d\n", i, bid_entry[i].eqid, bid_entry[i].bid_start, bid_entry[i].bid_count);
        PRINTF("next_bid_start=%d\n", next_bid_start);

        /* check whether the bid range overlapped */
        if (next_bid_start == 0) {
            next_bid_start = bid_entry[i].bid_start + bid_entry[i].bid_count;
            total_bid_count += bid_entry[i].bid_count;
            continue;
        }
        if (bid_entry[i].bid_start < next_bid_start) {
            PRINTF("%s: bid overlap at eqid=%d, bid_entry[%d].bid_start=%d, next_bid_start=%d!!\n",
                    __func__, bid_entry[i].eqid, i, bid_entry[i].bid_start, next_bid_start);
            BUG();
        }
        next_bid_start = bid_entry[i].bid_start + bid_entry[i].bid_count;
        total_bid_count += bid_entry[i].bid_count;

        PRINTF("total_bid_count=%d\n", total_bid_count);
        if (total_bid_count > AAL_L3QM_MAX_BID) {
            PRINTF("%s: total bid count %d exceed %d!!\n", __func__, total_bid_count, AAL_L3QM_MAX_BID);
            BUG();
        }
    }
#endif
    return CA_E_OK;
}

ca_status_t aal_l3qm_check_epps(ca_device_id_t device_id)
{
    int i, j, size;
    ca_uint8_t *virt_addr;

    /* check magic number among CPU EPPs */
    size = l3qm_cpu_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;
    for (j = 0; j < ca_ni_cpu_cpu_port_count; j++) {
        for (i = 0; i < l3qm_cpu_voq_per_port; i++) {
            virt_addr = (ca_uint8_t *)(l3qm_epp_profile_cpu_rx_fifo_start_addr + (i * size) + (j * l3qm_cpu_voq_per_port * size));
            if (ca_aal_debug & AAL_DBG_L3QM) {
                PRINTF("%s: CPU RX FIFO start address(%d)=0x%p\n", __func__, i, virt_addr + (size - l3qm_magic_marker_len));
            }
            if (aal_l3qm_check_magic_number(virt_addr + (size - l3qm_magic_marker_len)) != CA_E_OK) {
                PRINTF("%s: magic number check failed at CPU EPP (%d, %d)!!!\n", __func__, j, i);
                return CA_E_ERROR;
            }
        }
    }
    PRINTF("Magic number among CPU EPPs are OK\n");
    
    return CA_E_OK;
}

static void aal_l3qm_init_qm_axi_attrib(void)
{
    int i;
    ca_uint32_t offset;
    QM_QM_AXI_ATTRIBUTE_DATA0_t data0;
    QM_AXIM4_CONFIG_t   axim4_cfg;

    /* AXIM2 */
    axim4_cfg.wrd = CA_NE_REG_READ(QM_AXIM4_CONFIG);
    axim4_cfg.bf.axi_write_outtrans_num = 0xf;
    axim4_cfg.bf.axi_read_outtrans_num = 0xf;
    axim4_cfg.bf.write_cacheline_trans_en = 0;
    axim4_cfg.bf.read_cacheline_trans_en = 0;
    CA_NE_REG_WRITE(axim4_cfg.wrd, QM_AXIM4_CONFIG);

    /* EQ 0-15 */
    for (i = 0; i < CFG_ID_L3QM_AXI_ATTRIB_EQ_VALUES_LEN; i++) {
        offset = CA_L3QM_AXI_ATTRIBUTE_EQ_BASE + i;

        /* if QM ACE not turned on, ignore the startup configure settings if ace_cmd=1 or axi_top_bit=2 */
        data0.wrd = l3qm_axi_attrib_eq_values[i];
        if (!l3qm_ace_test) {
            if (data0.bf.axi_top_bit == 2 || data0.bf.ace_cmd == 1)
                data0.wrd = 0;
        }
        aal_l3qm_set_epp_axi_attrib(offset, data0.wrd);

        if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: EQ[%d] offset=0x%x, data0.wrd=0x%x, l3qm_axi_attrib_eq_values=0x%x\n",
                __func__, i, offset, data0.wrd, l3qm_axi_attrib_eq_values[i]);
        }
    }

    /* CPU EPP port 0-7 */
    for (i = 0; i < CFG_ID_L3QM_AXI_ATTRIB_CPU_EPP_VALUES_LEN; i++) {
        offset = CA_L3QM_AXI_ATTRIBUTE_CPU_BASE + i;

        /* if QM ACE not turned on, ignore the startup configure settings if ace_cmd=1 or axi_top_bit=2 */
        data0.wrd = l3qm_axi_attrib_cpu_epp_values[i];
        if (!l3qm_ace_test) {
            if (data0.bf.axi_top_bit == 2 || data0.bf.ace_cmd == 1)
                data0.wrd = 0;
        }

        aal_l3qm_set_epp_axi_attrib(offset, data0.wrd);

                if (ca_aal_debug & AAL_DBG_L3QM) {
                        PRINTF("%s: CPU EPP[%d] offset=0x%x, data0.wrd=0x%x, l3qm_axi_attrib_cpu_epp_values=0x%x\n",
                __func__, i, offset, data0.wrd, l3qm_axi_attrib_cpu_epp_values[i]);
                }
    }
}

/* Sending packets to CPU port through L3QM - Transmitting */
/* Configure CPU EPP FIFO for receiving packets for CPU */
void aal_l3qm_init_cpu_epp(void)
{
    int i, j, size;
    ca_uint32_t reg_off;
    QM_QM_CPU_EPP0_CFG_t cpu_epp0_cfg;
    QM_QM_CPU_EPP_FIFO_CFG_profile0_t cpu_epp_fifo_cfg_profile;
    QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_t cpu_epp64_fifo_paddr_start;
    ca_uint8_t *virt_addr;
    QM_QM_EPP_t epp_ctrl_reg;

    /* CPU EPP Port Configuration Register */
    /* Select per-destionation VOQ-to-EPP-FIFO-Ring map-mode */
    for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
        reg_off = QM_QM_CPU_EPP0_CFG + (QM_QM_CPU_EPP0_CFG_STRIDE * i);
        cpu_epp0_cfg.wrd = CA_NE_REG_READ(reg_off);
        cpu_epp0_cfg.bf.map_mode = l3qm_epp_profile_cpu_map_mode;
        CA_NE_REG_WRITE(cpu_epp0_cfg.wrd, reg_off);
    }

    /* CPU EPP Interrupt Coalescing Timer Configuration Register */
    /* Select coalescing Timer-base */
    CA_NE_REG_WRITE(0, QM_QM_CPU_EPP_CT_CFG);

    /* CPU EPP FIFO Configuration Register */
    /* only one EPP profile is used for CPU, the size is for one VoQ */
    reg_off = QM_QM_CPU_EPP_FIFO_CFG_profile0 + (QM_QM_CPU_EPP_FIFO_CFG_profile0_STRIDE * l3qm_epp_profile_cpu_id);
    cpu_epp_fifo_cfg_profile.wrd = CA_NE_REG_READ(reg_off);
    cpu_epp_fifo_cfg_profile.bf.size = l3qm_cpu_epp_per_voq / CA_L3QM_CPU_EPP_FIFO_SIZE_UINT;   /* the unit is 4 FIFO entries */
    cpu_epp_fifo_cfg_profile.bf.timer_ths = 0xf;
    if ((ca_ni_napi_budget / 16) > 0)
        cpu_epp_fifo_cfg_profile.bf.high_ths = (ca_ni_napi_budget / 16) + 1;    /* Interrupt to CPU is generated when EPP-FIFO accumulates at least ca_napi_budget entries */
    else
        cpu_epp_fifo_cfg_profile.bf.high_ths = 2;               /* Interrupt to CPU is generated when EPP-FIFO accumulates at least 16 entries */
    CA_NE_REG_WRITE(cpu_epp_fifo_cfg_profile.wrd, reg_off);

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: l3qm_cpu_epp_per_voq=%d\n", __func__, l3qm_cpu_epp_per_voq);
    }

    /* CPU EPP FIFO Starting address Register */
    /* Select one of 4 profiles for EPP-FIFO size, occupancy-high-threshold and time-threshold for CPU ports 0-5 */
    for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
        for (j = 0; j < l3qm_cpu_voq_per_port; j++) {
            /* select profile 2 */
            reg_off = QM_QM_CPU_EPP_FIFO0_0_CFG + (QM_QM_CPU_EPP_FIFO0_0_CFG_STRIDE * i) + (4 * j);
            CA_NE_REG_WRITE(l3qm_epp_profile_cpu_id, reg_off);
        }
    }
    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: l3qm_epp_profile_cpu_id=%d\n", __func__, l3qm_epp_profile_cpu_id);
    }

    /* fill the physical start address for CPU EPPs */
    #if 0
    size = l3qm_cpu_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len;
        for (j = 0; j < ca_ni_cpu_cpu_port_count; j++) {
                for (i = 0; i < l3qm_cpu_voq_per_port; i++) {
                        reg_off = QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START + (i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE) +
                (j * l3qm_cpu_voq_per_port * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE);
                        cpu_epp64_fifo_paddr_start.bf.phy_addr = l3qm_epp_profile_cpu_rx_fifo_phy_addr + (i * size) + (j * l3qm_cpu_voq_per_port * size);
               // if reserved mem is not enough, we get dynamic one.
               if((cpu_epp64_fifo_paddr_start.bf.phy_addr+size) > (l3qm_epp_profile_cpu_rx_fifo_phy_addr+ca_ne_ddr_coherent_reserved_size)){
                ca_uint_t *ptr = ca_malloc_trace(size, GFP_KERNEL|GFP_DMA, true);
                cpu_epp64_fifo_paddr_start.bf.phy_addr = VIRT_TO_PHYS(ptr);
                PRINTF("%s: port %d cos %d alloc 0x%px phy addr 0x%x\n", __func__, j, i, ptr, cpu_epp64_fifo_paddr_start.bf.phy_addr);
               }
            
                        CA_NE_REG_WRITE(cpu_epp64_fifo_paddr_start.wrd, reg_off);

                        epp64_paddr_start[j][i] = cpu_epp64_fifo_paddr_start.bf.phy_addr;

                        if (ca_aal_debug & AAL_DBG_L3QM) {
                PRINTF("%s: CPU RX FIFO register offset(%d, %d)=0x%x\n", __func__, j, i, reg_off);
                                PRINTF("%s: CPU RX FIFO start address(%d,%d)=0x%x\n", __func__, j, i, cpu_epp64_fifo_paddr_start.bf.phy_addr);
                PRINTF("%s: size=%d, offset=%d\n", __func__, size, (i * size) + (j * l3qm_cpu_voq_per_port * size));
                        }
            virt_addr = (ca_uint8_t *)(l3qm_epp_profile_cpu_rx_fifo_start_addr + (i * size) + (j * l3qm_cpu_voq_per_port * size) + (size - l3qm_magic_marker_len));
            aal_l3qm_insert_magic_number(virt_addr);
                }
        }
    #endif

    
    epp_ctrl_reg.wrd = CA_NE_REG_READ(QM_QM_EPP);
    

    if(L3QM_EPP_BIT_MODE == 1)
    {
        epp_ctrl_reg.bf.cmd_mode = 1;
        PRINTF( "%s: EPP is 64-bit mode\n", __func__);
    }
    else
    {
        epp_ctrl_reg.bf.cmd_mode = 0;
        PRINTF("%s: EPP is 32-bit mode\n", __func__);
    }
    
    CA_NE_REG_WRITE(epp_ctrl_reg.wrd, QM_QM_EPP);



    aal_l3qm_load_epp_config();
}

void aal_l3qm_axi_reo_rd_init(ca_device_id_t device_id)
{
    QM_AXI_REO_AXI_REO_RD_ORIG_ID_t orig_id;
    QM_AXI_REO_AXI_REO_RD_NEW_ID_t new_id;
    QM_AXI_REO_AXI_REO_RD_TOP_ADDR0_t top_addr;
    QM_AXI_REO_AXI_REO_RD_TOP_ADDR_MASK0_t top_addr_mask;
    QM_AXI_REO_AXI_REO_RD_NEW_ID0_t new_id_0;

    /* AXI reoder for deepq read*/
    orig_id.wrd = 0;
    orig_id.bf.axi_id = 15;

    new_id.wrd = 0;
    new_id.bf.axi_id = 12;
    new_id.bf.axi_id_valid = 1;

    top_addr.bf.id_addr = 0x10000000;
    top_addr_mask.bf.id_mask = 0x10000000;

    new_id_0.wrd = 0;
    new_id_0.bf.axi_id = 13;
    new_id_0.bf.axi_id_valid = 1;

    CA_NE_AXI_REO_REG_WRITE(orig_id.wrd, QM_AXI_REO_AXI_REO_RD_ORIG_ID);
    CA_NE_AXI_REO_REG_WRITE(new_id.wrd, QM_AXI_REO_AXI_REO_RD_NEW_ID);
    CA_NE_AXI_REO_REG_WRITE(top_addr.wrd, QM_AXI_REO_AXI_REO_RD_TOP_ADDR0);
    CA_NE_AXI_REO_REG_WRITE(top_addr_mask.wrd, QM_AXI_REO_AXI_REO_RD_TOP_ADDR_MASK0);
    CA_NE_AXI_REO_REG_WRITE(new_id_0.wrd, QM_AXI_REO_AXI_REO_RD_NEW_ID0);
}


void aal_l3qm_axi_reo_wr_init(ca_device_id_t device_id)
{
    QM_AXI_REO_AXI_REO_WR_ORIG_ID_t orig_id;
    QM_AXI_REO_AXI_REO_WR_NEW_ID_t new_id;
    QM_AXI_REO_AXI_REO_WR_TOP_ADDR0_t top_addr;
    QM_AXI_REO_AXI_REO_WR_TOP_ADDR_MASK0_t top_addr_mask;
    QM_AXI_REO_AXI_REO_WR_NEW_ID0_t new_id_0;

    /* AXI reoder for deepq write*/
    orig_id.wrd = 0;
    orig_id.bf.axi_id = 4;

    new_id.wrd = 0;
    new_id.bf.axi_id = 12;
    new_id.bf.axi_id_valid = 1;

    top_addr.bf.id_addr = 0x10000000;
    top_addr_mask.bf.id_mask = 0x10000000;

    new_id_0.wrd = 0;
    new_id_0.bf.axi_id = 13;
    new_id_0.bf.axi_id_valid = 1;

    CA_NE_AXI_REO_REG_WRITE(orig_id.wrd, QM_AXI_REO_AXI_REO_WR_ORIG_ID);
    CA_NE_AXI_REO_REG_WRITE(new_id.wrd, QM_AXI_REO_AXI_REO_WR_NEW_ID);
    CA_NE_AXI_REO_REG_WRITE(top_addr.wrd, QM_AXI_REO_AXI_REO_WR_TOP_ADDR0);
    CA_NE_AXI_REO_REG_WRITE(top_addr_mask.wrd, QM_AXI_REO_AXI_REO_WR_TOP_ADDR_MASK0);
    CA_NE_AXI_REO_REG_WRITE(new_id_0.wrd, QM_AXI_REO_AXI_REO_WR_NEW_ID0);

    /* AXI reoder for epp write*/   
    orig_id.wrd = 0;
    orig_id.bf.axi_id = 2;

    new_id.wrd = 0;
    new_id.bf.axi_id = 8;
    new_id.bf.axi_id_valid = 1;

    top_addr.bf.id_addr = 0x10000000;
    top_addr_mask.bf.id_mask = 0x10000000;

    new_id_0.wrd = 0;
    new_id_0.bf.axi_id = 9;
    new_id_0.bf.axi_id_valid = 1;

    CA_NE_AXI_REO_REG_WRITE(orig_id.wrd, QM_AXI_REO_AXI_REO_WR_ORIG_ID + APB2_QM_AXI_REO_AXI_REO_WR_STRIDE);
    CA_NE_AXI_REO_REG_WRITE(new_id.wrd, QM_AXI_REO_AXI_REO_WR_NEW_ID + APB2_QM_AXI_REO_AXI_REO_WR_STRIDE);
    CA_NE_AXI_REO_REG_WRITE(top_addr.wrd, QM_AXI_REO_AXI_REO_WR_TOP_ADDR0 + APB2_QM_AXI_REO_AXI_REO_WR_STRIDE);
    CA_NE_AXI_REO_REG_WRITE(top_addr_mask.wrd, QM_AXI_REO_AXI_REO_WR_TOP_ADDR_MASK0 + APB2_QM_AXI_REO_AXI_REO_WR_STRIDE);
    CA_NE_AXI_REO_REG_WRITE(new_id_0.wrd, QM_AXI_REO_AXI_REO_WR_NEW_ID0 + APB2_QM_AXI_REO_AXI_REO_WR_STRIDE);
}

ca_uint8_t aal_l3qm_get_buffer_size_index(ca_uint32_t buffer_size)
{
    ca_uint8_t index;

    switch (buffer_size) {
        case 128:
            index = 0;
            break;
        case 256:
            index = 1;
            break;
        case 512:
            index = 2;
            break;
        case 1024:
            index = 3;
            break;
        case 2048:
            index = 4;
            break;
        case 4096:
            index = 5;
            break;
        case 8192:
            index = 6;
            break;
        default:
            PRINTF("%s: buffer_size=%d is not defined!!\n", __func__, buffer_size);
            index = 0;
            break;
    }
    return index;
}

#if defined(CONFIG_LUNA_G3_SERIES)
/*
 * L3TE CB re-init
*/
void aal_l3qm_pe_reload_te_cb(ca_uint32_t voq_thresh_profile_id)
{
    int i, voq;
    int eq_buffer_count;
    int cpuIdx;
    int cpuIdx_end;
    aal_l3_te_cb_free_buf_cnt_t port_buf;
    aal_l3_te_cb_free_buf_cnt_mask_t port_buf_mask;
    ca_uint32_t profileIdx;
    aal_l3_te_cb_comb_threshold_mask_t threshold_msk;
    aal_l3_te_cb_comb_threshold_t threshold;

    if(voq_thresh_profile_id == L3TE_CB_VOQ_THRESH_PROFILE_PE0) {
        eq_buffer_count     = L3QM_EQ_PROFILE_PE0_BUF_COUNT;
        cpuIdx          = L3QM_EQ_PROFILE_PE0_CPUID_START;
        cpuIdx_end      = L3QM_EQ_PROFILE_PE0_CPUID_END;
    }else if(voq_thresh_profile_id == L3TE_CB_VOQ_THRESH_PROFILE_PE1) {
        eq_buffer_count = L3QM_EQ_PROFILE_PE1_BUF_COUNT;
        cpuIdx          = L3QM_EQ_PROFILE_PE1_CPUID_START;
        cpuIdx_end      = L3QM_EQ_PROFILE_PE1_CPUID_END;
    }else if(voq_thresh_profile_id == L3TE_CB_VOQ_THRESH_PROFILE_PE2) {
        eq_buffer_count = L3QM_EQ_PROFILE_PE2_BUF_COUNT;
        cpuIdx          = L3QM_EQ_PROFILE_PE2_CPUID_START;
        cpuIdx_end      = L3QM_EQ_PROFILE_PE2_CPUID_END;
    }else {
        PRINTF("%s: not support, voq_thresh_profile_id = %d\n", __FUNCTION__, voq_thresh_profile_id);
        return;
    }

    for(i = cpuIdx; i <= cpuIdx_end; i++) {

        // port free buffer
        port_buf_mask.u32 = 0;
        memset(&port_buf, 0, sizeof(aal_l3_te_cb_free_buf_cnt_t));
        memset(&threshold, 0, sizeof(aal_l3_te_cb_comb_threshold_t));

        port_buf_mask.s.cnt1_msb = 1;
        port_buf_mask.s.cnt0_msb = 1;
        port_buf_mask.s.cnt1 = 1;
        port_buf_mask.s.cnt0 = 1;
        
        port_buf.cnt1_msb = 1;
        port_buf.cnt0_msb = 1;
        port_buf.cnt0 = eq_buffer_count;
        port_buf.cnt1 = 0;
        
            aal_l3_te_cb_port_free_buf_cnt_set(0, i, port_buf_mask, &port_buf);

        // adjust voq threshold
        profileIdx = voq_thresh_profile_id;
        threshold_msk.u32 = 0;
        threshold_msk.s.threshold_hi = 1;
        threshold_msk.s.threshold_lo = 1;
        threshold.threshold_hi = (eq_buffer_count*7) >> 3;  // x/8
        threshold.threshold_lo = (eq_buffer_count*6) >> 3;  // y/8  

        for(voq = (i<<3); voq < ((i<<3) + 8); voq++) {
            aal_l3_te_cb_voq_profile_sel_set(0, voq, profileIdx);
        }
        aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);
    
        // adjust port threshold
        profileIdx = AAL_L3TE_CB_PE1_PROFILE_SEL;
        threshold_msk.s.threshold_hi = 1;
        threshold_msk.s.threshold_lo = 1;
        threshold.threshold_hi = 16;
        threshold.threshold_lo = 2;

        aal_l3_te_cb_port_profile_sel_set(0, i, profileIdx);
        aal_l3_te_cb_port_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);
    }
}
//EXPORT_SYMBOL(aal_l3qm_pe_reload_te_cb);

/*
 * Support ldpid 0x13-0x17 used by AMSDU.
 */
static void aal_l3qm_init_empty_buffer_PEx(aal_l3qm_eq_cfg_t eq_cfg)
{
    int i, voq;
    ca_uint32_t reg_off;
    QM_QM_EQ_PROFILE0_t eq_profile;
    QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
    QM_QM_DEST_PORT0_PKT_BUF_CFG_t dest_port_pkt_buf_cfg;
    QM_QM_CFG0_EQ0_t cfg0_eq;
    QM_QM_CFG1_EQ0_t cfg1_eq;
    QM_QM_CFG2_EQ0_t cfg2_eq;
    QM_QM_CFG3_EQ0_t cfg3_eq;
       QM_QM_CFG4_EQ0_t cfg4_eq;
    int eq_profile_id = eq_cfg.profile_id;
    int eq_profile_pool0_id = eq_cfg.pool0_id;
    int eq_profile_pool1_id = eq_cfg.pool1_id;
    int eq_buffer_count = eq_cfg.buffer_count;
    int eq_buffer_size = eq_cfg.buffer_size;
    int eq_buffer_head_room = eq_cfg.buffer_head_room;
    int eq_buffer_tail_room = eq_cfg.buffer_tail_room;
    int cpuIdx = eq_cfg.cpu_idx_start;
    int cpuIdx_end = eq_cfg.cpu_idx_end;

    /* dedicated profile for each CPU core */
    
    eq_profile.wrd = 0;
    eq_profile.bf.eqp0 = eq_profile_pool0_id;
    eq_profile.bf.eqp1 = eq_profile_pool1_id;
    eq_profile.bf.rule = 0;
    reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * (eq_profile_id));
    CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: PE eq profile %d pool0/1 eq_id=%d/%d, rule=%d\n", 
            __func__, (eq_profile_id), eq_profile.bf.eqp0, eq_profile.bf.eqp1, eq_profile.bf.rule);
    }

    /* for CPU ports profile select*/
    for(i = cpuIdx; i <= cpuIdx_end; i++) {
        dest_port_eq_cfg.bf.profile_sel = eq_profile_id;
        reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * i);
        CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);

            if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: PE profile id=%d\n", __func__, eq_profile_id);
        }

        {
            reg_off = QM_QM_DEST_PORT0_PKT_BUF_CFG + (QM_QM_DEST_PORT0_PKT_BUF_CFG_STRIDE * i);
            dest_port_pkt_buf_cfg.wrd = CA_NE_REG_READ(reg_off);
            dest_port_pkt_buf_cfg.bf.head_room_first = dest_port_pkt_buf_cfg.bf.head_room_rest = eq_buffer_head_room / 16;       /* reserved 32 bytes for first fragment of packet  */
            dest_port_pkt_buf_cfg.bf.tail_room_first= dest_port_pkt_buf_cfg.bf.tail_room_rest = eq_buffer_tail_room / 16;           // 512
            CA_NE_REG_WRITE(dest_port_pkt_buf_cfg.wrd, reg_off);
        }

        if(i>=0 && i<CA_NI_NUM_OF_CPU)
            l3qm_cpu_port_used_by_pe[i] = 1;
    }

    /* configure total_buffer_num and bid_start for CPU ports*/
    /* only two EB pools are assigned to CPU */
    i = eq_profile_id;
    {

        /* enable Empty Buffer */
        cfg0_eq.wrd = 0;
        cfg0_eq.bf.eq_en = 1;
        reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * i);
        CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

        /* configure total_buffer_num and bid_start */
        cfg1_eq.wrd = 0;
        cfg1_eq.bf.total_buffer_num = eq_buffer_count;
        cfg1_eq.bf.bid_start = DQ_bid_start;    // after cpu buffer
        DQ_bid_start += cfg1_eq.bf.total_buffer_num;

        reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * i);
        CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

            if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: PE eq_id=%d, pool0 bid_start=%d\n", __func__, i, cfg1_eq.bf.bid_start);
            PRINTF("%s: PE pool0 total_buffer_num=%d\n", __func__, cfg1_eq.bf.total_buffer_num);
        }

        /* configure buffer size for Empty Buffer */
        reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * i);
        cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
        cfg2_eq.bf.cpu_eq = 1;

        /* set threshold to be (1500 - 160) / 8, regard the 160 as budget in RX routine */
        /* the budget value should not be too small otherwise the interrupts happened too frequently */
        if(cfg1_eq.bf.total_buffer_num > ((1<<CA_L3QM_CPU_REFILL_THS_BITS -1) * 8))
            cfg2_eq.bf.refill_ths = (1<<CA_L3QM_CPU_REFILL_THS_BITS -1);
        else
            cfg2_eq.bf.refill_ths = cfg1_eq.bf.total_buffer_num / 8;

        cfg2_eq.bf.refill_en = eq_cfg.fbm_en;
        cfg2_eq.bf.refill_fbm_eqid = eq_cfg.fbm_id;
        cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(eq_buffer_size);      /* 2048 bytes */
        reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * i);
        CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);
        l3qm_eq_cfg_cache[i].fbm_en=cfg2_eq.bf.refill_en;
        l3qm_eq_cfg_cache[i].fbm_id=cfg2_eq.bf.refill_fbm_eqid;
        l3qm_eq_cfg_cache[i].buffer_size=cfg2_eq.bf.buffer_size;
        l3qm_eq_cfg_cache[i].buffer_head_room=eq_buffer_head_room;

        PRINTF( "%s: i=%d, reg_off=0x%x, cfg2_eq.wrd=0x%x\n", __func__, i, reg_off, cfg2_eq.wrd);

            if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: PE pool buf_sz=%d, size_index=%d\n", __func__, eq_buffer_size, cfg2_eq.bf.buffer_size);
        }

        /* PE pool 0 */
        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * i);
        /* if QM ACE not turned on, ignore the startup configure settings if acd_cmd=1 */
        cfg3_eq.wrd = l3qm_eq_cfg3_values[i];
        if (!l3qm_ace_test && cfg3_eq.bf.acd_cmd == 1) {
            cfg3_eq.wrd = 0;
        }
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

        /* configure AXI address top 8 bits */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * i);
        cfg4_eq.wrd = 0;
        if (l3qm_ace_test) {
            cfg4_eq.bf.axi_top_bit = l3qm_eq_cfg4_values[i];
        }

            CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

    }

    {
        TE_CB_WIFI_EQ_PROFILE0_THRSH_t wifi_eq_th;
        /*if(eq_buffer_count <= 128) {*/
            wifi_eq_th.bf.hth = AAL_L3_TE_CB_THRED_DISABLE;
            wifi_eq_th.bf.lth = AAL_L3_TE_CB_THRED_DISABLE;
        /*}else {
            wifi_eq_th.bf.hth = 12;
            wifi_eq_th.bf.lth = 6;
        }*/
        CA_NE_REG_WRITE(wifi_eq_th.wrd, TE_CB_WIFI_EQ_PROFILE0_THRSH + (eq_profile_id * TE_CB_WIFI_EQ_PROFILE0_THRSH_STRIDE));
        
        /*
         * L3TE CB re-init
        */
        aal_l3qm_pe_reload_te_cb(eq_cfg.te_cb_voq_th_profile_id);
    }


    
}

static void aal_l3qm_init_empty_buffer_PE(void)
{
    aal_l3qm_eq_cfg_t eq_cfg;
    
#if defined(CONFIG_RTK_FC_WIFI_DRIVER_OFFLOAD_BY_PE)
    /* 0x12 */
    memset(&eq_cfg, 0, sizeof(eq_cfg));
    eq_cfg.cpu_idx_start = L3QM_EQ_PROFILE_PE0_CPUID_START;
    eq_cfg.cpu_idx_end = L3QM_EQ_PROFILE_PE0_CPUID_END;
    eq_cfg.profile_id = L3QM_EQ_PROFILE_PE0_ID;
    eq_cfg.fbm_en = L3QM_EQ_PROFILE_PE0_FBM_EN;
    eq_cfg.fbm_id = L3QM_EQ_PROFILE_PE0_FBM_IDX;
    eq_cfg.pool0_id = L3QM_EQ_PROFILE_PE0_POOL0_ID;
    eq_cfg.pool1_id = L3QM_EQ_PROFILE_PE0_POOL1_ID;
    eq_cfg.buffer_count = L3QM_EQ_PROFILE_PE0_BUF_COUNT;
    eq_cfg.buffer_size = L3QM_EQ_PROFILE_PE0_BUF_SIZE;
    eq_cfg.buffer_head_room = 0;
    eq_cfg.buffer_tail_room = 0;
    eq_cfg.te_cb_voq_th_profile_id = L3TE_CB_VOQ_THRESH_PROFILE_PE0;
    aal_l3qm_init_empty_buffer_PEx(eq_cfg);
#endif

#if defined(CONFIG_RTK_FC_HTTP_OFFLOAD_BY_PE) || defined(CONFIG_RTK_FC_CRYPTO_OFFLOAD_BY_PE) || defined(CONFIG_RTK_FC_PKT_QUEUE_OFFLOAD_BY_PE) || defined(CONFIG_RTK_FC_SRV6_OFFLOAD_BY_PE)
    /* 0x17 */
    memset(&eq_cfg, 0, sizeof(eq_cfg));
    eq_cfg.cpu_idx_start = L3QM_EQ_PROFILE_PE1_CPUID_START;
    eq_cfg.cpu_idx_end = L3QM_EQ_PROFILE_PE1_CPUID_END;
    eq_cfg.profile_id = L3QM_EQ_PROFILE_PE1_ID;
    eq_cfg.fbm_en = L3QM_EQ_PROFILE_PE1_FBM_EN;
    eq_cfg.fbm_id = L3QM_EQ_PROFILE_PE1_FBM_IDX;
    eq_cfg.pool0_id = L3QM_EQ_PROFILE_PE1_POOL0_ID;
    eq_cfg.pool1_id = L3QM_EQ_PROFILE_PE1_POOL1_ID;
    eq_cfg.buffer_count = L3QM_EQ_PROFILE_PE1_BUF_COUNT;
    eq_cfg.buffer_size = L3QM_EQ_PROFILE_PE1_BUF_SIZE;
    eq_cfg.buffer_head_room = l3qm_cpu_port_head_room_first;
    eq_cfg.buffer_tail_room = L3QM_CPU_PORT_TAIL_ROOM_FIRST;
    eq_cfg.te_cb_voq_th_profile_id = L3TE_CB_VOQ_THRESH_PROFILE_PE1;
    aal_l3qm_init_empty_buffer_PEx(eq_cfg);
#endif

#if defined(CONFIG_RTK_FC_WIFI_AMSDU_OFFLOAD_BY_PE) || defined(CONFIG_REALTEK_BOARD_FPGA_V8)
    /* 0x13 - 0x17 */
    memset(&eq_cfg, 0, sizeof(eq_cfg));
    eq_cfg.cpu_idx_start = L3QM_EQ_PROFILE_PE2_CPUID_START;
    eq_cfg.cpu_idx_end = L3QM_EQ_PROFILE_PE2_CPUID_END;
    eq_cfg.profile_id = L3QM_EQ_PROFILE_PE2_ID;
    eq_cfg.fbm_en = L3QM_EQ_PROFILE_PE2_FBM_EN;
    eq_cfg.fbm_id = L3QM_EQ_PROFILE_PE2_FBM_IDX;
    eq_cfg.pool0_id = L3QM_EQ_PROFILE_PE2_POOL0_ID;
    eq_cfg.pool1_id = L3QM_EQ_PROFILE_PE2_POOL1_ID;
    eq_cfg.buffer_count = L3QM_EQ_PROFILE_PE2_BUF_COUNT;
    eq_cfg.buffer_size = L3QM_EQ_PROFILE_PE2_BUF_SIZE;
    eq_cfg.buffer_head_room = l3qm_cpu_port_head_room_first;
    eq_cfg.buffer_tail_room = L3QM_CPU_PORT_TAIL_ROOM_FIRST;
    eq_cfg.te_cb_voq_th_profile_id = L3TE_CB_VOQ_THRESH_PROFILE_PE2;
    aal_l3qm_init_empty_buffer_PEx(eq_cfg);
#endif
}

static void aal_l3qm_init_empty_buffer_CPU(ca_uint8_t is_use_fbm)
{
    int i, buf_size = l3qm_eq_profile_cpu_pool0_buf_sz;
    ca_uint32_t reg_off;
    QM_QM_EQ_PROFILE0_t eq_profile;
    QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
    QM_QM_DEST_PORT0_PKT_BUF_CFG_t dest_port_pkt_buf_cfg;
    QM_QM_CFG0_EQ0_t cfg0_eq;
    QM_QM_CFG1_EQ0_t cfg1_eq;
    QM_QM_CFG2_EQ0_t cfg2_eq;
    QM_QM_CFG3_EQ0_t cfg3_eq;
    QM_QM_CFG4_EQ0_t cfg4_eq;

    /* dedicated profile for each CPU core */
    for(i = 0; i < L3QM_EQ_PROFILE_CPU_COUNT; i++) {
        eq_profile.wrd = 0;
        eq_profile.bf.eqp0 = l3qm_eq_profile_cpu_pool0_eq_id+i;
        eq_profile.bf.eqp1 = l3qm_eq_profile_cpu_pool1_eq_id;
        eq_profile.bf.rule = l3qm_eq_profile_cpu_rule;
        reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * (l3qm_eq_profile_cpu_id+i));
        CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

            if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: CPU eq profile %d pool0/1 eq_id=%d/%d, rule=%d\n", 
                __func__, (l3qm_eq_profile_cpu_id+i), eq_profile.bf.eqp0, eq_profile.bf.eqp1, l3qm_eq_profile_cpu_rule);
        }
        
    }

    /* for CPU ports profile select, profile 4/5/6/7 for CPU ports 0-3 and 4-7 */
    for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
        
        if(i>=0 && i<CA_NI_NUM_OF_CPU && l3qm_cpu_port_used_by_pe[i] )
            continue;
        
        dest_port_eq_cfg.bf.profile_sel = l3qm_eq_profile_cpu_id + (i % L3QM_EQ_PROFILE_CPU_COUNT);
        reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * i);
        CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);
    }

        if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: CPU profile id=%d\n", __func__, l3qm_eq_profile_cpu_id);
    }

    for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
        
        if(i>=0 && i<CA_NI_NUM_OF_CPU && l3qm_cpu_port_used_by_pe[i] )
            continue;
        
        reg_off = QM_QM_DEST_PORT0_PKT_BUF_CFG + (QM_QM_DEST_PORT0_PKT_BUF_CFG_STRIDE * i);
        dest_port_pkt_buf_cfg.wrd = CA_NE_REG_READ(reg_off);
        dest_port_pkt_buf_cfg.bf.head_room_first = dest_port_pkt_buf_cfg.bf.head_room_rest = l3qm_cpu_port_head_room_first / 16;        // 0
        dest_port_pkt_buf_cfg.bf.tail_room_first= dest_port_pkt_buf_cfg.bf.tail_room_rest = L3QM_CPU_PORT_TAIL_ROOM_FIRST / 16;         // 512
#if defined(L3QM_CPU_ARM_WIFI_FIX_2KB)
        if(i == L3QM_CPU_PE_WIFI_PORT_IDX) {
            // overwrite
            dest_port_pkt_buf_cfg.bf.head_room_first = dest_port_pkt_buf_cfg.bf.head_room_rest = 0;
            dest_port_pkt_buf_cfg.bf.tail_room_first= 0;
            dest_port_pkt_buf_cfg.bf.tail_room_rest = 16 / 16;
        }
#endif
        CA_NE_REG_WRITE(dest_port_pkt_buf_cfg.wrd, reg_off);
    }

    /* configure total_buffer_num and bid_start for CPU ports*/
    for (i = l3qm_eq_profile_cpu_pool0_eq_id; i < (l3qm_eq_profile_cpu_pool0_eq_id+L3QM_EQ_PROFILE_CPU_COUNT); i++) {
        ca_uint8_t cpu_idx = (i-l3qm_eq_profile_cpu_pool0_eq_id);
        if(cpu_idx>=0 && cpu_idx<CA_NI_NUM_OF_CPU && l3qm_cpu_port_used_by_pe[cpu_idx] )
            continue;
        
        /* enable Empty Buffer */
        cfg0_eq.wrd = 0;
        cfg0_eq.bf.eq_en = 1;
        reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * i);
        CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

        /* configure total_buffer_num and bid_start */
        cfg1_eq.wrd = 0;
        if((i - l3qm_eq_profile_cpu_pool0_eq_id) == L3QM_CPU_ARM_WIFI_PORT_IDX)
            cfg1_eq.bf.total_buffer_num = L3QM_CPU_ARM_WIFI_BUF_CNT;    
        else     if((i - l3qm_eq_profile_cpu_pool0_eq_id) == L3QM_CPU_ARM_WIFI_AGG_PORT_IDX)
            cfg1_eq.bf.total_buffer_num = L3QM_CPU_ARM_WIFI_AGG_BUF_CNT;    
        else
            cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_cpu_pool0_buf_count;
        cfg1_eq.bf.bid_start = DQ_bid_start;
        DQ_bid_start += cfg1_eq.bf.total_buffer_num;

        reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * i);
        CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

            if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: CPU eq_id=%d, pool0 bid_start=%d\n", __func__, i, cfg1_eq.bf.bid_start );
            PRINTF("%s: CPU pool0 total_buffer_num=%d\n", __func__, cfg1_eq.bf.total_buffer_num);
        }

        /* configure buffer size for Empty Buffer */
        reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * i);
        cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
        cfg2_eq.bf.cpu_eq = 1;

        /* set threshold to be (1500 - 160) / 8, regard the 160 as budget in RX routine */
        /* the budget value should not be too small otherwise the interrupts happened too frequently */
        if(cfg1_eq.bf.total_buffer_num > ((1<<CA_L3QM_CPU_REFILL_THS_BITS -1) * 8))
            cfg2_eq.bf.refill_ths = (1<<CA_L3QM_CPU_REFILL_THS_BITS -1);
        else
                cfg2_eq.bf.refill_ths = (cfg1_eq.bf.total_buffer_num - ca_ni_napi_budget) / 8;

        cfg2_eq.bf.refill_en = is_use_fbm;
        cfg2_eq.bf.refill_fbm_eqid = l3qm_eq_profile_cpu_pool0_fbm_pool_id;
        buf_size = (l3qm_eq_profile_cpu_pool0_buf_sz);      /* 2048 bytes */
#if defined(L3QM_CPU_ARM_WIFI_FIX_2KB)
        if((i - l3qm_eq_profile_cpu_pool0_eq_id) == L3QM_CPU_ARM_WIFI_AGG_PORT_IDX) {
            buf_size = 2048;
        }
#endif
        
        cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(buf_size);
        reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * i);
        CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);
        l3qm_eq_cfg_cache[i].fbm_en=cfg2_eq.bf.refill_en;
        l3qm_eq_cfg_cache[i].fbm_id=cfg2_eq.bf.refill_fbm_eqid;
        l3qm_eq_cfg_cache[i].buffer_size=cfg2_eq.bf.buffer_size;
        l3qm_eq_cfg_cache[i].buffer_head_room=l3qm_cpu_port_head_room_first;

        PRINTF( "%s: i=%d, reg_off=0x%x, cfg2_eq.wrd=0x%x\n", __func__, i, reg_off, cfg2_eq.wrd);

            if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: CPU pool buf_sz=%d, size_index=%d\n", __func__, buf_size, cfg2_eq.bf.buffer_size);
        }

        /* CPU pool 0 */
        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * i);
        /* if QM ACE not turned on, ignore the startup configure settings if acd_cmd=1 */
        cfg3_eq.wrd = l3qm_eq_cfg3_values[i];
        if (!l3qm_ace_test && cfg3_eq.bf.acd_cmd == 1) {
            cfg3_eq.wrd = 0;
        }
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

        /* configure AXI address top 8 bits */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * i);
        cfg4_eq.wrd = 0;
        if (l3qm_ace_test) {
            cfg4_eq.bf.axi_top_bit = l3qm_eq_cfg4_values[i];
        }

            CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

    }

    {

        /*
         * L3TE CB re-init
        */
        
        aal_l3_te_cb_free_buf_cnt_t port_buf;
        aal_l3_te_cb_free_buf_cnt_mask_t port_buf_mask;
        ca_uint32_t profileIdx = 0;
        aal_l3_te_cb_comb_threshold_mask_t threshold_msk;
        aal_l3_te_cb_comb_threshold_t threshold;

        memset(&threshold, 0, sizeof(aal_l3_te_cb_comb_threshold_t));

        for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
            
            if(i>=0 && i<CA_NI_NUM_OF_CPU && l3qm_cpu_port_used_by_pe[i] )
                continue;
        
            port_buf_mask.u32 = 0;
            memset(&port_buf, 0, sizeof(aal_l3_te_cb_free_buf_cnt_t));

            port_buf_mask.s.cnt1_msb = 1;
            port_buf_mask.s.cnt0_msb = 1;
            port_buf_mask.s.cnt1 = 1;
            port_buf_mask.s.cnt0 = 1;
            
            port_buf.cnt1_msb = 1;
            port_buf.cnt0_msb = 1;
            if((i%L3QM_EQ_PROFILE_CPU_COUNT)  == L3QM_CPU_ARM_WIFI_PORT_IDX)
                port_buf.cnt0 = L3QM_CPU_ARM_WIFI_BUF_CNT;
            #if 0
            else     if((i%L3QM_EQ_PROFILE_CPU_COUNT)  == L3QM_CPU_ARM_WIFI_AGG_PORT_IDX)
                port_buf.cnt0 = L3QM_CPU_ARM_WIFI_AGG_BUF_CNT;
            else
                port_buf.cnt0 = l3qm_eq_profile_cpu_pool0_buf_count;
            #endif
            port_buf.cnt1 = 0;
            
                aal_l3_te_cb_port_free_buf_cnt_set(0, i, port_buf_mask, &port_buf);
                

            // adjust voq threshold
            threshold_msk.u32 = 0;
            threshold_msk.s.threshold_hi = 1;
            threshold_msk.s.threshold_lo = 1;
            threshold.threshold_hi = (port_buf.cnt0*7) >> 3;    // x/8
            threshold.threshold_lo = (port_buf.cnt0*6) >> 3;    // y/8  

            aal_l3_te_cb_voq_profile_sel_get(0, i*8, &profileIdx);
            aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);
            aal_l3_te_cb_voq_threshold_profile_get(0, profileIdx, &threshold);
        
            // adjust port threshold
            threshold_msk.s.threshold_hi = 1;
            threshold_msk.s.threshold_lo = 1;
            threshold.threshold_hi = threshold.prvt << 4;   // reserved for privated buffer cnt
            threshold.threshold_lo = threshold.prvt << 3;   // reserved for privated buffer cnt

            aal_l3_te_cb_port_profile_sel_get(0, i, &profileIdx);
            aal_l3_te_cb_port_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);
        }

    }


    
}

#else

static void aal_l3qm_init_empty_buffer_CPU(void)
{
    int i;
    ca_uint32_t reg_off;
    QM_QM_EQ_PROFILE0_t eq_profile;
    QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
    QM_QM_DEST_PORT0_PKT_BUF_CFG_t dest_port_pkt_buf_cfg;
    QM_QM_CFG0_EQ0_t cfg0_eq;
    QM_QM_CFG1_EQ0_t cfg1_eq;
    QM_QM_CFG2_EQ0_t cfg2_eq;
    QM_QM_CFG3_EQ0_t cfg3_eq;
        QM_QM_CFG4_EQ0_t cfg4_eq;

    /* only one profile is assigned to CPU */
    eq_profile.wrd = 0;
    eq_profile.bf.eqp0 = l3qm_eq_profile_cpu_pool0_eq_id;
    eq_profile.bf.eqp1 = l3qm_eq_profile_cpu_pool1_eq_id;
    eq_profile.bf.rule = l3qm_eq_profile_cpu_rule;
    reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * l3qm_eq_profile_cpu_id);
    CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: CPU pool0 eq_id=%d, rule=%d\n", __func__, l3qm_eq_profile_cpu_pool0_eq_id, l3qm_eq_profile_cpu_rule);
    }

    /* for CPU ports profile select, profile 2 for CPU CPU ports 0-5 */
    for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
        dest_port_eq_cfg.bf.profile_sel = l3qm_eq_profile_cpu_id;
        reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * i);
        CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);
    }

        if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: CPU profile id=%d\n", __func__, l3qm_eq_profile_cpu_id);
    }

    for (i = 0; i < ca_ni_cpu_cpu_port_count; i++) {
        reg_off = QM_QM_DEST_PORT0_PKT_BUF_CFG + (QM_QM_DEST_PORT0_PKT_BUF_CFG_STRIDE * i);
        dest_port_pkt_buf_cfg.wrd = CA_NE_REG_READ(reg_off);
        dest_port_pkt_buf_cfg.bf.head_room_first = l3qm_cpu_port_head_room_first / 16;       /* reserved 32 bytes for first fragment of packet  */
        dest_port_pkt_buf_cfg.bf.head_room_rest = dest_port_pkt_buf_cfg.bf.head_room_first;  /* reserved 32 bytes for reset of segments of packet  */
        CA_NE_REG_WRITE(dest_port_pkt_buf_cfg.wrd, reg_off);
    }

    /* configure total_buffer_num and bid_start for CPU ports, EB pool 0-13 */
    /* only two EB pools are assigned to CPU */
    for (i = l3qm_eq_profile_cpu_pool0_eq_id; i <= l3qm_eq_profile_cpu_pool1_eq_id; i++) {

        /* enable Empty Buffer */
        cfg0_eq.wrd = 0;
        cfg0_eq.bf.eq_en = 1;
        reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * i);
        CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

        /* configure total_buffer_num and bid_start */
        cfg1_eq.wrd = 0;
        cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_cpu_pool0_buf_count;

        if (i == l3qm_eq_profile_cpu_pool0_eq_id)
            cfg1_eq.bf.bid_start = l3qm_eq_profile_cpu_pool0_bid_start;
        else
            cfg1_eq.bf.bid_start = l3qm_eq_profile_cpu_pool1_bid_start;

        reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * i);
        CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

            if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: CPU eq_id=%d, pool0 bid_start=%d\n", __func__, i, l3qm_eq_profile_cpu_pool0_bid_start);
            PRINTF("%s: CPU pool0 total_buffer_num=%d\n", __func__, cfg1_eq.bf.total_buffer_num);
        }

        /* configure buffer size for Empty Buffer */
        reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * i);
        cfg2_eq.wrd = CA_NE_REG_READ(reg_off);

        PRINTF("%s: l3qm_eq_profile_cpu_use_fbm=%d, l3qm_eq_profile_cpu_pool0_fbm_pool_id=%d, l3qm_eq_profile_cpu_pool1_fbm_pool_id=%d\n", __func__, l3qm_eq_profile_cpu_use_fbm, l3qm_eq_profile_cpu_pool0_fbm_pool_id, l3qm_eq_profile_cpu_pool1_fbm_pool_id);

        if (l3qm_eq_profile_cpu_use_fbm) {
            cfg2_eq.bf.cpu_eq = 1;

            if (i == l3qm_eq_profile_cpu_pool0_eq_id)
                cfg2_eq.bf.refill_fbm_eqid = l3qm_eq_profile_cpu_pool0_fbm_pool_id;
            else
                cfg2_eq.bf.refill_fbm_eqid = l3qm_eq_profile_cpu_pool1_fbm_pool_id;

            /* use the same threshold as non-FBM, because we wish QM REFILL interrupt happened every 160 BIDs comsumed */
            /* set threshold to be (1500 - 160) / 8, regard the 160 as budget in RX routine */
            /* the budget value should not be too small otherwise the interrupts happened too frequently */
                        cfg2_eq.bf.refill_ths = (l3qm_cpu_pool_bid_count - ca_ni_napi_budget) / 8;

            cfg2_eq.bf.refill_en = 1;
        }
        else {
            cfg2_eq.bf.cpu_eq = 1;
            cfg2_eq.bf.refill_fbm_eqid = 0;

            /* set threshold to be (1500 - 160) / 8, regard the 160 as budget in RX routine */
            /* the budget value should not be too small otherwise the interrupts happened too frequently */
                        cfg2_eq.bf.refill_ths = (l3qm_cpu_pool_bid_count - ca_ni_napi_budget) / 8;

            cfg2_eq.bf.refill_en = 0;
            cfg2_eq.bf.refill_fbm_eqid = 0;
        }

        if (i == l3qm_eq_profile_cpu_pool0_eq_id)
            cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_cpu_pool0_buf_sz);      /* 2048 bytes */
        else
            cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_cpu_pool1_buf_sz);      /* 2048 bytes */

        reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * i);
        CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

        PRINTF( "%s: i=%d, reg_off=0x%x, cfg2_eq.wrd=0x%x\n", __func__, i, reg_off, cfg2_eq.wrd);

            if (ca_aal_debug & AAL_DBG_L3QM) {
            PRINTF("%s: CPU pool buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_cpu_pool0_buf_sz, cfg2_eq.bf.buffer_size);
        }
    }

    /* CPU pool 0 */
    /* configure cache_eos/domain/snoop/cache */
    reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_cpu_pool0_eq_id);
    /* if QM ACE not turned on, ignore the startup configure settings if acd_cmd=1 */
    cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_cpu_pool0_eq_id];
    if (!l3qm_ace_test && cfg3_eq.bf.acd_cmd == 1) {
        cfg3_eq.wrd = 0;
    }
    CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

    /* configure AXI address top 8 bits */
    reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu_pool0_eq_id);
    cfg4_eq.wrd = 0;
    if (l3qm_ace_test) {
        cfg4_eq.bf.axi_top_bit = l3qm_eq_cfg4_values[l3qm_eq_profile_cpu_pool0_eq_id];
    }

    PRINTF("%s: l3qm_eq_profile_cpu_pool0_eq_id=%d, cfg4_eq.bf.axi_top_bit=%d\n", __func__, l3qm_eq_profile_cpu_pool0_eq_id, cfg4_eq.bf.axi_top_bit);
        CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

    /* CPU pool1 */
    /* configure cache_eos/domain/snoop/cache */
    reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_cpu_pool1_eq_id);
    /* if QM ACE not turned on, ignore the startup configure settings if acd_cmd=1 */
    cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_cpu_pool1_eq_id];
    if (!l3qm_ace_test && cfg3_eq.bf.acd_cmd == 1) {
        cfg3_eq.wrd = 0;
    }
    CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

    /* configure AXI address top 8 bits */
    reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_cpu_pool1_eq_id);
    cfg4_eq.wrd = 0;
    if (l3qm_ace_test) {
        cfg4_eq.bf.axi_top_bit = l3qm_eq_cfg4_values[l3qm_eq_profile_cpu_pool1_eq_id];
    }
    CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);
}
#endif

static void aal_l3qm_init_empty_buffer_DQ(void)
{
    int i;
    ca_uint32_t reg_off;
    QM_QM_EQ_PROFILE0_t eq_profile;
    QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
    QM_QM_CFG0_EQ0_t cfg0_eq;
    QM_QM_CFG1_EQ0_t cfg1_eq;
    QM_QM_CFG2_EQ0_t cfg2_eq;
    QM_QM_CFG2_EQ0_t cfg3_eq;
    QM_QM_CFG4_EQ0_t cfg4_eq;

    /* one profile for DQ or normal NI ports */
    eq_profile.wrd = 0;
    eq_profile.bf.eqp0 = l3qm_eq_profile_dq_pool0_eq_id;
    eq_profile.bf.eqp1 = l3qm_eq_profile_dq_pool1_eq_id;
    eq_profile.bf.rule = l3qm_eq_profile_dq_rule;
    reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * l3qm_eq_profile_dq_id);
    CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: DQ pool0 eq_id=%d, rule=%d\n", __func__, l3qm_eq_profile_dq_pool0_eq_id, l3qm_eq_profile_dq_rule);
    }

    dest_port_eq_cfg.wrd = 0;

    /* for CPU ports profile select, profile 0 for DQ at NI ports  */
    for (i = 0; i < QM_QM_DEST_PORT0_EQ_CFG_COUNT; i++) {
        dest_port_eq_cfg.bf.profile_sel = l3qm_eq_profile_dq_id;
        reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * (i + CA_NI_TOTAL_CPU_PORT));
        CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);
    }

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: DQ profile id=%d\n", __func__, l3qm_eq_profile_dq_id);
    }

    /* configure EB pool 0 for DQ */
    /* set phy_addr_start and enable Empty Buffer */
    cfg0_eq.wrd = 0;
    cfg0_eq.bf.eq_en = 1;
    if (l3qm_deepq_test_with_sram)
        cfg0_eq.bf.phy_addr_start = (0xc0028000) >> CA_L3QM_PHY_ADDR_SHIFT;
    else
        cfg0_eq.bf.phy_addr_start = (l3qm_eq_profile_dq_pool0_buff_phy_addr) >> CA_L3QM_PHY_ADDR_SHIFT;
    reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_dq_pool0_eq_id);
    CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: DQ pool0 buff_phy_addr=0x%x\n", __func__, l3qm_eq_profile_dq_pool0_buff_phy_addr);
    }

    /* configure total buffer number and bid_start */
    cfg1_eq.wrd = 0;
    cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_dq_pool0_buf_count;
    cfg1_eq.bf.bid_start = l3qm_eq_profile_dq_pool0_bid_start;
    reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_dq_pool0_eq_id);
    CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: DQ pool0 buf_count=%d, bid_start=%d\n", __func__, l3qm_eq_profile_dq_pool0_buf_count, l3qm_eq_profile_dq_pool0_bid_start);
    }

    /* configure buffer size for Empty Buffer */
    reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_dq_pool0_eq_id);
    cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
    cfg2_eq.bf.cpu_eq = 0;
    cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_dq_pool0_buf_sz);      /* 2048 bytes */
    cfg2_eq.bf.refill_en = 0;
    CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: DQ pool0 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_dq_pool0_buf_sz, cfg2_eq.bf.buffer_size);
    }

        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_dq_pool0_eq_id);
        cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_dq_pool0_eq_id];
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

        /* configure AXI address top 8 bits */
        /* because PE0 pool1 locates at DDR start from 0x180000000 */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_dq_pool0_eq_id);
    if (l3qm_deepq_test_with_sram)
        cfg4_eq.wrd = 0;
    else
        cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_dq_pool0_eq_id];
        CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

        /* configure EB pool 1 for DQ */
        /* set phy_addr_start and enable Empty Buffer */
        cfg0_eq.wrd = 0;
    cfg0_eq.bf.eq_en = 1;
    if (l3qm_deepq_test_with_sram)
        cfg0_eq.bf.phy_addr_start = (0xc0030000) >> CA_L3QM_PHY_ADDR_SHIFT;
    else
        cfg0_eq.bf.phy_addr_start = (l3qm_eq_profile_dq_pool1_buff_phy_addr) >> CA_L3QM_PHY_ADDR_SHIFT;
    reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * l3qm_eq_profile_dq_pool1_eq_id);
        CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                PRINTF("%s: DQ pool1 buff_phy_addr=0x%x\n", __func__, l3qm_eq_profile_dq_pool1_buff_phy_addr);
        }

        /* configure total buffer number and bid_start */
        cfg1_eq.wrd = 0;
        cfg1_eq.bf.total_buffer_num = l3qm_eq_profile_dq_pool1_buf_count;
        cfg1_eq.bf.bid_start = l3qm_eq_profile_dq_pool1_bid_start;
        reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * l3qm_eq_profile_dq_pool1_eq_id);
        CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                PRINTF("%s: DQ pool1 buf_count=%d, bid_start=%d\n", __func__,
                        l3qm_eq_profile_dq_pool1_buf_count, l3qm_eq_profile_dq_pool1_bid_start);
        }

        /* configure buffer size for Empty Buffer */
        reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * l3qm_eq_profile_dq_pool1_eq_id);
        cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
        cfg2_eq.bf.cpu_eq = 0;
        cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(l3qm_eq_profile_dq_pool1_buf_sz);      /* 2048 bytes */
        CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                PRINTF("%s: DQ pool1 buf_sz=%d, size_index=%d\n", __func__, l3qm_eq_profile_dq_pool1_buf_sz, cfg2_eq.bf.buffer_size);
        }

        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * l3qm_eq_profile_dq_pool1_eq_id);
        cfg3_eq.wrd = l3qm_eq_cfg3_values[l3qm_eq_profile_dq_pool1_eq_id];
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

    /* configure AXI address top 8 bits */
    /* because PE0 pool1 locates at DDR start from 0x180000000 */
    reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * l3qm_eq_profile_dq_pool1_eq_id);
    if (l3qm_deepq_test_with_sram)
        cfg4_eq.wrd = 0;
    else
        cfg4_eq.wrd = l3qm_eq_cfg4_values[l3qm_eq_profile_dq_pool1_eq_id];
    CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);
    
}

/* Sending packets to CPU port through L3QM - Receiving */
/* Configure Empty Buffer Pool for CPU and NI and set up the physical start address for NI ports */
static void aal_l3qm_init_empty_buffer_DQ_1(void);
void aal_l3qm_init_empty_buffer(ca_uint8_t is_use_fbm)
{
    QM_QM_ES_CTRL_t es_ctrl;

    es_ctrl.wrd = CA_NE_REG_READ(QM_QM_ES_CTRL);
    es_ctrl.bf.crc_en = 0;  // reduce 4B CRC overhead, nic rx also not consider that.
    CA_NI_REG_WRITE(es_ctrl.wrd, QM_QM_ES_CTRL);    

    //CA_NE_REG_WRITE(0xffffffff, QM_QM_BUFFER_INT_SRCE);
        
    aal_l3qm_init_empty_buffer_PE();    // init first before ARM cpu
    
    aal_l3qm_init_empty_buffer_CPU(is_use_fbm);

    //aal_l3qm_init_empty_buffer_DQ_1();

    /* must be called before aal_l3qm_load_eq_config() */

    aal_l3qm_load_eq_config();

    aal_l3qm_init_qm_axi_attrib();

    aal_l3qm_check_bid_start(0);

    //spin_lock_init(&l3qm_epp64_inte_lock);
}

void aal_l3qm_enable_rx(ca_uint8_t enable)
{
    QM_QM_RMU0_CTRL_t rmu0_ctrl;

    rmu0_ctrl.wrd = CA_NE_REG_READ(QM_QM_RMU0_CTRL);
    if (enable) {
        rmu0_ctrl.bf.rx_en = 1;
    }
    else {
        rmu0_ctrl.bf.rx_en = 0;
    }
    CA_NE_REG_WRITE(rmu0_ctrl.wrd, QM_QM_RMU0_CTRL);

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: enable=%d\n", __func__, enable);
    }
}

void aal_l3qm_enable_tx(ca_uint8_t enable)
{
    QM_QM_ES_CTRL_t es_ctrl;

    es_ctrl.wrd = CA_NE_REG_READ(QM_QM_ES_CTRL);
    if (enable) {
        es_ctrl.bf.tx_en = 1;
        es_ctrl.bf.cpu_en = 0;
        es_ctrl.bf.ni_en = 0xff;
        es_ctrl.bf.inccfg_for_pkt_cntrs = 0x2;
        es_ctrl.bf.inccfg_for_err_cntrs = 0x6;
    }
    else {
        es_ctrl.bf.tx_en = 0;
        es_ctrl.bf.cpu_en = 0;
        es_ctrl.bf.ni_en = 0;
    }
    CA_NE_REG_WRITE(es_ctrl.wrd, QM_QM_ES_CTRL);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                PRINTF("%s: enable=%d\n", __func__, enable);
        }
}

void aal_l3qm_enable_tx_cpu(ca_uint16_t cpu_port, ca_uint8_t enable)
{
        QM_QM_ES_CTRL_t es_ctrl;

        es_ctrl.wrd = CA_NE_REG_READ(QM_QM_ES_CTRL);
        if (enable) {
                es_ctrl.bf.cpu_en |= (1 << cpu_port);
        }
        else {
                es_ctrl.bf.cpu_en &= ~(1 << cpu_port);
        }
        CA_NE_REG_WRITE(es_ctrl.wrd, QM_QM_ES_CTRL);

        if (ca_aal_debug & AAL_DBG_L3QM) {
                PRINTF("%s: cpu_port=%d, enable=%d, es_ctrl.bf.cpu_en=0x%x\n", __func__, cpu_port, enable, es_ctrl.bf.cpu_en);
        }
}

void aal_l3qm_config_DWRR(void)
{
    int i;
    QM_QM_DRR_WEIGHT_BASE_t drr_weight_base;
    QM_QM_VOQ3_0_WEIGHT_RATIO_CFG0_t voq3_0_weight_ratio;
    QM_QM_VOQ7_4_WEIGHT_RATIO_CFG0_t voq7_4_weight_ratio;

    QM_QM_PORT7_0_WEIGHT_RATIO_CFG_t   port7_0_weight_ratio;
    QM_QM_PORT15_8_WEIGHT_RATIO_CFG_t   port15_8_weight_ratio;

    /* configure DRR Scheduler Weight Base */
    drr_weight_base.wrd = CA_NE_REG_READ(QM_QM_DRR_WEIGHT_BASE);

    CA_NE_REG_WRITE(drr_weight_base.wrd, QM_QM_DRR_WEIGHT_BASE);

    /* Configure Weight Ratio for VOQ */
    for (i = 0; i < QM_QM_VOQ3_0_WEIGHT_RATIO_CFG0_COUNT; i++) {
        voq3_0_weight_ratio.bf.voq0 = 1;
        voq3_0_weight_ratio.bf.voq1 = 1;
        voq3_0_weight_ratio.bf.voq2 = 1;
        voq3_0_weight_ratio.bf.voq3 = 1;
        CA_NE_REG_WRITE(voq3_0_weight_ratio.wrd, QM_QM_VOQ3_0_WEIGHT_RATIO_CFG0 + (QM_QM_VOQ3_0_WEIGHT_RATIO_CFG0_STRIDE * i));
    }
    for (i = 0; i < QM_QM_VOQ7_4_WEIGHT_RATIO_CFG0_COUNT; i++) {
        voq7_4_weight_ratio.bf.voq4 = 1;
        voq7_4_weight_ratio.bf.voq5 = 1;
        voq7_4_weight_ratio.bf.voq6 = 1;
        voq7_4_weight_ratio.bf.voq7 = 1;
        CA_NE_REG_WRITE(voq3_0_weight_ratio.wrd, QM_QM_VOQ7_4_WEIGHT_RATIO_CFG0 + (QM_QM_VOQ7_4_WEIGHT_RATIO_CFG0_STRIDE * i));
    }

    /* Configure Weight Ratio for CPU ports */
    port7_0_weight_ratio.bf.port0 = 1;
    port7_0_weight_ratio.bf.port1 = 1;
    port7_0_weight_ratio.bf.port2 = 1;
    port7_0_weight_ratio.bf.port3 = 1;
    port7_0_weight_ratio.bf.port4 = 1;
    port7_0_weight_ratio.bf.port5 = 1;
    port7_0_weight_ratio.bf.port6 = 1;
    port7_0_weight_ratio.bf.port7 = 1;
    CA_NE_REG_WRITE(port7_0_weight_ratio.wrd, QM_QM_PORT7_0_WEIGHT_RATIO_CFG);

    /* Configure Weight Ratio for NI ports */
    /* the DRR_WEIGHT_BASE*weight_ratio=128*6=768= should equal to (max packet size/2)=1514/2=757 */
    port15_8_weight_ratio.bf.port8  = 1;
    port15_8_weight_ratio.bf.port9  = 1;
    port15_8_weight_ratio.bf.port10 = 1;
    port15_8_weight_ratio.bf.port11 = 1;
    port15_8_weight_ratio.bf.port12 = 1;
    port15_8_weight_ratio.bf.port13 = 1;
    port15_8_weight_ratio.bf.port14 = 1;
    port15_8_weight_ratio.bf.port15 = 1;
    CA_NE_REG_WRITE(port15_8_weight_ratio.wrd, QM_QM_PORT15_8_WEIGHT_RATIO_CFG);
}

void aal_l3qm_enable_int_src(void)
{
    QM_QM_INT_SRCE_t    int_srcE;

    int_srcE.wrd = 0;
    int_srcE.bf.header_ecc_error_int_srcE       = 1;
    int_srcE.bf.voqinfo_ecc_error_int_srcE      = 1;
    int_srcE.bf.mmu_lkup_bid_out_of_range_int_srcE  = 1;
    int_srcE.bf.rmu0_no_buffer_drop_int_srcE    = 1;
    int_srcE.bf.rmu0_check_error_int_srcE       = 1;
    int_srcE.bf.rmu0_fifo_error_int_srcE        = 1;
    int_srcE.bf.ni_bridge_fifo_error_int_srcE   = 1;
    int_srcE.bf.dqm_fifo_error_int_srcE     = 1;
    int_srcE.bf.eqm_fifo_error_int_srcE     = 1;
    int_srcE.bf.cpuepp_fifo_error_int_srcE      = 1;
    int_srcE.bf.tmu_fifo_error_int_srcE     = 1;
    int_srcE.bf.eqm_cfg_error_int_srcE      = 1;
    int_srcE.bf.axiarb_fifo_error_int_srcE      = 1;
    int_srcE.bf.pkt_lenght_error_int_srcE       = 1;
    int_srcE.bf.fbm_returns_no_buffer_int_srcE  = 0;
    int_srcE.bf.eq_overflow_int_srcE        = 1;
    int_srcE.bf.axim_rmu0_resp_error_int_srcE   = 1;
    int_srcE.bf.axim_cpuepp_resp_error_int_srcE = 1;
    int_srcE.bf.axim_qm_int_srcE            = 1;
    CA_NE_REG_WRITE(int_srcE.wrd, QM_QM_INT_SRCE);
}

ca_uint32_t aal_l3qm_read_rx_interrupt_status(void)
{
    QM_QM_VOQ_STATUS0_t voq_status0;
    QM_QM_VOQ_STATUS1_t voq_status1;
    QM_QM_VOQ_STATUS2_t voq_status2;
    QM_QM_VOQ_STATUS3_t voq_status3;

    voq_status0.wrd = CA_NE_REG_READ(QM_QM_VOQ_STATUS0);
    voq_status1.wrd = CA_NE_REG_READ(QM_QM_VOQ_STATUS1);
    voq_status2.wrd = CA_NE_REG_READ(QM_QM_VOQ_STATUS2);
    voq_status3.wrd = CA_NE_REG_READ(QM_QM_VOQ_STATUS3);

    PRINTF("%s: voq_status0.wrd=0x%x, voq_status1.wrd=0x%x, voq_status2.wrd=0x%x, voq_status3.wrd=0x%x\n", __func__,
        voq_status0.wrd, voq_status1.wrd, voq_status2.wrd, voq_status3.wrd);

    return 0;
}

void aal_l3qm_write_rx_interrupt_status(ca_uint32_t status)
{
    QM_QM_INT_SRC_t         int_src;

    PRINTF("%s: status = 0x%x\n", __func__, status);
    int_src.wrd = status;
    CA_NE_REG_WRITE(int_src.wrd, QM_QM_INT_SRC);
}

void aal_l3qm_enable_refill_threshold_interrupt(ca_uint16_t eqid, ca_uint8_t enable)
{
    QM_QM_EQ_REFILL_INTERRUPT_EN_t refill_int_en;
    unsigned long flags;

    //spin_lock_irqsave(&l3qm_epp64_inte_lock, flags);
    
    refill_int_en.wrd = CA_NE_REG_READ(QM_QM_EQ_REFILL_INTERRUPT_EN);
    if (enable)
            refill_int_en.bf.eq_refill_en |= (1 << eqid);
    else
            refill_int_en.bf.eq_refill_en &= ~(1 << eqid);

    CA_NE_REG_WRITE(refill_int_en.wrd, QM_QM_EQ_REFILL_INTERRUPT_EN);

    //spin_unlock_irqrestore(&l3qm_epp64_inte_lock, flags);
}

void aal_l3qm_setup_refill_threshold_interrup(void)
{
    CA_NE_REG_WRITE(0, QM_QM_EQ_REFILL_INTERRUPT_EN);
#if defined(CONFIG_LUNA_G3_SERIES)
    {
        ca_uint32 i = 0;
        for(i = l3qm_eq_profile_cpu_pool0_eq_id; i < (l3qm_eq_profile_cpu_pool0_eq_id+L3QM_EQ_PROFILE_CPU_COUNT); i++)
            aal_l3qm_enable_refill_threshold_interrupt(i, 1);
    }
#else
    aal_l3qm_enable_refill_threshold_interrupt(l3qm_eq_profile_cpu_pool0_eq_id, 1);
    aal_l3qm_enable_refill_threshold_interrupt(l3qm_eq_profile_cpu_pool1_eq_id, 1);
#endif
}

void aal_l3qm_enable_cpu_epp_interrupt(ca_uint16_t cpu_port, ca_uint8_t enable)
{
    QM_QM_EPP64_P3_0_INTERRUPT_EN_t p3_0_int_en;
    QM_QM_EPP64_P7_4_INTERRUPT_EN_t p7_4_int_en;
    unsigned long flags;

    /* these registers are shared by all of CPUs */
    //spin_lock_irqsave(&l3qm_epp64_inte_lock, flags);

    if (cpu_port >= 0 && cpu_port <= 3) {
        p3_0_int_en.wrd = CA_NE_REG_READ(QM_QM_EPP64_P3_0_INTERRUPT_EN);
        switch (cpu_port) {
            case 0:
                if (enable)
                    p3_0_int_en.bf.p0_int_en = 0xff;
                else
                    p3_0_int_en.bf.p0_int_en = 0;
                break;
            case 1:
                                if (enable)
                                        p3_0_int_en.bf.p1_int_en = 0xff;
                                else
                                        p3_0_int_en.bf.p1_int_en = 0;
                                break;
            case 2:
                                if (enable)
                                        p3_0_int_en.bf.p2_int_en = 0xff;
                                else
                                        p3_0_int_en.bf.p2_int_en = 0;
                                break;
            case 3:
                                if (enable)
                                        p3_0_int_en.bf.p3_int_en = 0xff;
                                else
                                        p3_0_int_en.bf.p3_int_en = 0;
                                break;
        }
        CA_NE_REG_WRITE(p3_0_int_en.wrd, QM_QM_EPP64_P3_0_INTERRUPT_EN);
    }
    else {
        p7_4_int_en.wrd = CA_NE_REG_READ(QM_QM_EPP64_P7_4_INTERRUPT_EN);
        switch (cpu_port) {
            case 4:
                if (enable)
                    p7_4_int_en.bf.p4_int_en = 0xff;
                else
                    p7_4_int_en.bf.p4_int_en = 0;
                break;
            case 5:
                if (enable)
                    p7_4_int_en.bf.p5_int_en = 0xff;
                else
                    p7_4_int_en.bf.p5_int_en = 0;
                break;
            case 6:
                if (enable)
                    p7_4_int_en.bf.p6_int_en = 0xff;
                else
                    p7_4_int_en.bf.p6_int_en = 0;
                break;
            case 7:
                if (enable)
                    p7_4_int_en.bf.p7_int_en = 0xff;
                else
                    p7_4_int_en.bf.p7_int_en = 0;
                break;
        }
        CA_NE_REG_WRITE(p7_4_int_en.wrd, QM_QM_EPP64_P7_4_INTERRUPT_EN);
    }
    //spin_unlock_irqrestore(&l3qm_epp64_inte_lock, flags);
}

int aal_l3qm_insert_magic_number(void *addr)
{
    int i;
        ca_uint32_t *magic_number = (ca_uint32_t *)addr;

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: addr=%p, l3qm_magic_marker_len=%d\n", __func__, addr, l3qm_magic_marker_len);
    }

        for (i = 0; i < l3qm_magic_marker_len/4; i++) {
                *magic_number++ = CA_L3QM_MAGIC_NUMBER;
        }
    return l3qm_magic_marker_len;
}

ca_status_t aal_l3qm_check_magic_number(void *addr)
{
        int i;
        ca_uint32_t *magic_number = (ca_uint32_t *)addr;

        for (i = 0; i < l3qm_magic_marker_len/4; i++) {
                if (*magic_number != CA_L3QM_MAGIC_NUMBER) {
            PRINTF("%s: addr=%p, magic_number=0x%x\n", __func__, magic_number, *magic_number);
            return CA_E_ERROR;
        }
        magic_number++;
        }
        return CA_E_OK;
}

static void aal_l3qm_dump_sram_map(void)
{
    PRINTF( "%s: Dump SRAM map...\n", __func__);

    PRINTF( "[ Main hash ]\n");
    PRINTF( "overflow hash fib start addr = 0x%llx\n", (ca_uint64_t)l3fe_main_hash_overflow_hash_fib_start_addr);
    PRINTF( "overflow hash fib physical addr = 0x%x\n", l3fe_main_hash_overflow_hash_fib_phy_addr);
    PRINTF( "overflow hash fib length = %d\n", l3fe_main_hash_overflow_hash_fib_length);
    PRINTF( "overflow hash fib count = %d\n", l3fe_main_hash_overflow_hash_fib_count);

     PRINTF( "default hash fib internal reg mode = %d\n", l3fe_main_hash_flow_default_fib_internal_reg_mode);
    if(l3fe_main_hash_flow_default_fib_internal_reg_mode==0) {
        PRINTF( "default hash fib start addr = 0x%llx\n", (ca_uint64_t)l3fe_main_hash_default_hash_fib_start_addr);
        PRINTF( "default hash fib physical addr = 0x%x\n", l3fe_main_hash_default_hash_fib_phy_addr);
    } else {
        PRINTF( "default hash fib start addr = N/A (use internal reg)\n");
        PRINTF( "default hash fib physical addr = N/A (use internal reg)\n");
    }
    PRINTF( "default hash fib length = %d\n", l3fe_main_hash_default_hash_fib_length);
    PRINTF( "default hash fib count = %d\n", l3fe_main_hash_default_hash_fib_count);

    PRINTF( "hash action cache fib internal sram mode = %d\n", l3fe_main_hash_flow_cache_fib_internal_sram_mode);
    if(l3fe_main_hash_flow_cache_fib_internal_sram_mode==0) {
        PRINTF( "hash action cache fib start addr = 0x%llx\n", (ca_uint64_t)l3fe_main_hash_action_cache_fib_start_addr);
        PRINTF( "hash action cache fib physical addr = 0x%x\n", l3fe_main_hash_action_cache_fib_phy_addr);
    } else {
        PRINTF( "hash action cache fib start addr = N/A (use internal sram)\n");
        PRINTF( "hash action cache fib physical addr = N/A (use internal sram)\n");
    }
    PRINTF( "hash action cache fib length = %d\n", l3fe_main_hash_action_cache_fib_length);
    PRINTF( "hash action cache fib count = %d\n", l3fe_main_hash_action_cache_fib_count);
}

static ca_status_t aal_l3qm_create_sram_map(void *sram_base, ca_uint32_t sram_size, void *sram_phy_base)
{
    ca_uint32_t size = 0;
    ca_uint8_t *sram_start_addr = sram_base;
    ca_uint8_t *sram_phy_addr = sram_phy_base;

    // reserve at least 32B for PE bootup
    {
        size += 128;
    }

    /* Main hash overflow action */
    l3fe_main_hash_overflow_hash_fib_start_addr = (ca_uint_t)sram_start_addr + size;
    l3fe_main_hash_overflow_hash_fib_phy_addr   = (ca_uint_t)sram_phy_base + size;
    size += l3fe_main_hash_overflow_hash_fib_length * l3fe_main_hash_overflow_hash_fib_count;

    /* Magic number */
    size += aal_l3qm_insert_magic_number(sram_start_addr + size);

    /* Main hash default action */
    l3fe_main_hash_default_hash_fib_start_addr = (ca_uint_t)sram_start_addr + size;
    l3fe_main_hash_default_hash_fib_phy_addr = (ca_uint_t)sram_phy_addr + size;
    if(l3fe_main_hash_flow_default_fib_internal_reg_mode==0) {
        size += l3fe_main_hash_default_hash_fib_length * l3fe_main_hash_default_hash_fib_count;
        /* Magic number */
        size += aal_l3qm_insert_magic_number(sram_start_addr + size);
    }

    /* Main hash cache action */
    l3fe_main_hash_action_cache_fib_start_addr = (ca_uint_t)sram_start_addr + size;
    l3fe_main_hash_action_cache_fib_phy_addr = (ca_uint_t)sram_phy_addr + size;
    if(l3fe_main_hash_flow_cache_fib_internal_sram_mode==0) {
        size += l3fe_main_hash_action_cache_fib_length * l3fe_main_hash_action_cache_fib_count;
        /* Magic number */
        size += aal_l3qm_insert_magic_number(sram_start_addr + size);
    }

    PRINTF( "%s: total required SRAM size 0x%x\n", __func__, size);

    if (size > sram_size) {
        PRINTF( "%s: out of SRAM space! (required=0x%x, total=0x%x)\n", __func__, size, sram_size);
        PRINTF( "%s: NE driver initialized failed!!!\n", __func__);
    }
    //BUG_ON(size > sram_size);

    aal_l3qm_dump_sram_map();

    sram_dts_conf_size = sram_size;
    sram_cur_used_size += size;
    
    sram_start_virtual_addr = (ca_uint_t)sram_start_addr;
    sram_cur_virtual_addr = (ca_uint_t)sram_start_addr + size;
    
    PRINTF( "sram start vir addr = 0x%llx, used size = 0x%x, cur vir addr = 0x%llx\n", sram_start_virtual_addr, sram_cur_used_size, sram_cur_virtual_addr);

    return CA_E_OK;
}

static void aal_l3qm_dump_ddr_map(void)
{
#if defined(CONFIG_LUNA_G3_SERIES)
    PRINTF( "%s: Dump DDR map...\n", __func__);
    PRINTF( "[ DDR coherent ]\n");

    PRINTF( "CPU RX FIFO buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu_rx_fifo_start_addr);
    PRINTF( "CPU RX FIFO buffer physical addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu_rx_fifo_phy_addr);
    PRINTF( "CPU RX FIFO count = %d\n", l3qm_epp_profile_cpu_total_epp_count);
    PRINTF( "CPU RX FIFO size = %d\n", l3qm_epp_profile_cpu_sz);
    PRINTF( "CPU RX FIFO consumed size = %d\n", l3qm_epp_profile_cpu_total_epp_count * l3qm_epp_profile_cpu_sz);

    PRINTF( "[ DDR non-cache ]\n");

    PRINTF( "Main Hash table entry start addr = 0x%llx\n", (ca_uint64_t)l3fe_main_hash_table_start_addr);
    PRINTF( "Main Hash table entry physical addr = 0x%x\n", l3fe_main_hash_table_phy_addr);
    PRINTF( "Main Hash tabel entry length = %d\n", l3fe_main_hash_table_entry_length);
    PRINTF( "Main Hash table entry count = %d\n", l3fe_main_hash_table_entry_count);
    PRINTF( "Main Hash table consumed size = %lld\n", (ca_uint64_t)(l3fe_main_action_table_start_addr - l3fe_main_hash_table_start_addr));

    PRINTF( "Main FIB action table entry addr = 0x%llx\n", (ca_uint64_t)l3fe_main_action_table_start_addr);
    PRINTF( "Main FIB action table entry physical addr = 0x%x\n", l3fe_main_action_table_phy_addr);
    PRINTF( "Main FIB action table entry length = %d\n", l3fe_main_hash_action_table_entry_length);
    PRINTF( "Main FIB action table entry count = %d\n", l3fe_main_hash_action_table_entry_count);
    PRINTF( "Main FIB action table consumed size = %lld\n", (ca_uint64_t)(l3qm_eq_profile_dq_pool1_buff_start_addr - l3fe_main_action_table_start_addr));

    PRINTF( "DEEPQ pool1 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_dq_pool1_buff_start_addr);
    PRINTF( "DEEPQ pool1 buffer physical addr = 0x%x\n", l3qm_eq_profile_dq_pool1_buff_phy_addr);
    PRINTF( "DEEPQ pool1 buffer count = %d\n", l3qm_eq_profile_dq_pool1_buf_count);
    PRINTF( "DEEPQ pool1 buffer size = %d\n", l3qm_eq_profile_dq_pool1_buf_sz);
    PRINTF( "DEEPQ pool1 consumed size = %d\n", l3qm_eq_profile_dq_pool1_buf_count * l3qm_eq_profile_dq_pool1_buf_sz);

#else
    PRINTF( "%s: Dump DDR map...\n", __func__);
    PRINTF( "[ DDR coherent ]\n");

    PRINTF( "CPU pool0 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu_pool0_start_addr);
    PRINTF( "CPU pool0 buffer physical addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu_pool0_phy_addr);
    PRINTF( "CPU pool0 buffer count = %d\n", l3qm_eq_profile_cpu_pool0_buf_count);
    PRINTF( "CPU pool0 buffer size = %d\n", l3qm_eq_profile_cpu_pool0_buf_sz);
    PRINTF( "CPU pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_cpu_pool1_start_addr - l3qm_eq_profile_cpu_pool0_start_addr);

        PRINTF( "CPU pool1 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu_pool1_start_addr);
        PRINTF( "CPU pool1 buffer physical addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_cpu_pool1_phy_addr);
        PRINTF( "CPU pool1 buffer count = %d\n", l3qm_eq_profile_cpu_pool1_buf_count);
        PRINTF( "CPU pool1 buffer size = %d\n", l3qm_eq_profile_cpu_pool1_buf_sz);
    PRINTF( "CPU pool1 consumed size = %lld\n", (ca_uint64_t)l3qm_epp_profile_cpu_rx_fifo_start_addr - l3qm_eq_profile_cpu_pool1_start_addr);

        PRINTF( "CPU RX FIFO buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu_rx_fifo_start_addr);
        PRINTF( "CPU RX FIFO buffer physical addr = 0x%llx\n", (ca_uint64_t)l3qm_epp_profile_cpu_rx_fifo_phy_addr);
        PRINTF( "CPU RX FIFO count = %d\n", l3qm_epp_profile_cpu_total_epp_count);
        PRINTF( "CPU RX FIFO size = %d\n", l3qm_epp_profile_cpu_sz);
    PRINTF( "CPU RX FIFO consumed size = %d\n", l3qm_epp_profile_cpu_total_epp_count * l3qm_epp_profile_cpu_sz);

    PRINTF( "[ DDR non-cache ]\n");
        PRINTF( "DEEPQ pool0 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_dq_pool0_buff_start_addr);
    PRINTF( "DEEPQ pool0 buffer physical addr = 0x%x\n", l3qm_eq_profile_dq_pool0_buff_phy_addr);
        PRINTF( "DEEPQ pool0 buffer count = %d\n", l3qm_eq_profile_dq_pool0_buf_count);
        PRINTF( "DEEPQ pool0 buffer size = %d\n", l3qm_eq_profile_dq_pool0_buf_sz);
    PRINTF( "DEEPQ pool0 consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_dq_pool1_buff_start_addr - l3qm_eq_profile_dq_pool0_buff_start_addr);

        PRINTF( "DEEPQ pool1 buffer start addr = 0x%llx\n", (ca_uint64_t)l3qm_eq_profile_dq_pool1_buff_start_addr);
    PRINTF( "DEEPQ pool1 buffer physical addr = 0x%x\n", l3qm_eq_profile_dq_pool1_buff_phy_addr);
        PRINTF( "DEEPQ pool1 buffer count = %d\n", l3qm_eq_profile_dq_pool1_buf_count);
        PRINTF( "DEEPQ pool1 buffer size = %d\n", l3qm_eq_profile_dq_pool1_buf_sz);
    PRINTF( "DEEPQ pool1 consumed size = %lld\n", (ca_uint64_t)l3fe_main_hash_table_start_addr - l3qm_eq_profile_dq_pool1_buff_start_addr);

        PRINTF( "Main Hash table entry start addr = 0x%llx\n", (ca_uint64_t)l3fe_main_hash_table_start_addr);
    PRINTF( "Main Hash table entry physical addr = 0x%x\n", l3fe_main_hash_table_phy_addr);
        PRINTF( "Main Hash tabel entry length = %d\n", l3fe_main_hash_table_entry_length);
        PRINTF( "Main Hash table entry count = %d\n", l3fe_main_hash_table_entry_count);
    PRINTF( "Main Hash table consumed size = %lld\n", (ca_uint64_t)l3fe_main_action_table_start_addr - l3fe_main_hash_table_start_addr);

        PRINTF( "Main FIB action table entry addr = 0x%llx\n", (ca_uint64_t)l3fe_main_action_table_start_addr);
    PRINTF( "Main FIB action table entry physical addr = 0x%x\n", l3fe_main_action_table_phy_addr);
        PRINTF( "Main FIB action table entry length = %d\n", l3fe_main_hash_action_table_entry_length);
        PRINTF( "Main FIB action table entry count = %d\n", l3fe_main_hash_action_table_entry_count);
    PRINTF( "Main FIB action table consumed size = %lld\n", (ca_uint64_t)l3qm_eq_profile_pe0_pool1_buff_start_addr - l3fe_main_action_table_start_addr);
#endif

}

static ca_status_t aal_l3qm_create_ddr_map(void *ddr_coherent_base, void *ddr_noncache_base, void *ddr_noncache_phy_base)
{
    ca_uint32_t size = 0;
    ca_uint8_t *ddr_coherent_start_addr = ddr_coherent_base;
    ca_uint8_t *ddr_noncache_start_addr = ddr_noncache_base;
    ca_uint8_t *ddr_noncache_phy_addr = ddr_noncache_phy_base;
    
#if defined(CONFIG_LUNA_G3_SERIES)
    /* DDR coherent */
    size = 0;
    l3qm_epp_profile_cpu_rx_fifo_start_addr = (ca_uint_t)PHYS_TO_VIRT((ca_uint_t)ddr_coherent_start_addr);

    l3qm_epp_profile_cpu_rx_fifo_phy_addr = VIRT_TO_PHYS((void *)l3qm_epp_profile_cpu_rx_fifo_start_addr);
    size = l3qm_epp_profile_cpu_sz * l3qm_epp_profile_cpu_total_epp_count;
    
    PRINTF( "%s: total size of DDR coherent consumed=0x%x/0x%x\n", __func__, size, ca_ne_ddr_coherent_reserved_size);
    
    if (size > ca_ne_ddr_coherent_reserved_size) {
        PRINTF("%s: out of DDR Coherent! some EPP FIFO mem will be dynamic alloc!!!\n", __func__);  // ref: aal_l3qm_init_cpu_epp()
    }
    
    /* DDR non-cacheable */
    size = 0;

    /* Main hash */
    l3fe_main_hash_table_start_addr = (ca_uint_t)ddr_noncache_start_addr + size;
    l3fe_main_hash_table_phy_addr = (ca_uint_t)ddr_noncache_phy_addr + size;
    size += l3fe_main_hash_table_entry_count * l3fe_main_hash_table_entry_length;
       //PRINTF("%s(%d): total size of DDR non-cache consumed=0x%x/0x%x\n", __func__, __LINE__, size, ca_ne_ddr_noncache_reserved_size);

    /* Main FIB */
    {
        uint8_t extrafib_group = 0, fib_len = 32;
        #if 0
        aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_FLOW_ACTION_GRP24_SUPPORT, 1, &extrafib_group);
        #endif
        extrafib_group = CFG_ID_L3FE_MAIN_HASH_FLOW_ACTION_GRP24_SUPPORT;
        if(extrafib_group == 1) fib_len = 48;
        else if(extrafib_group == 2) fib_len = 64;

        l3fe_main_action_table_start_addr = (ca_uint_t)ddr_noncache_start_addr + size;
        l3fe_main_action_table_phy_addr = (ca_uint_t)ddr_noncache_phy_addr + size;
        size += l3fe_main_hash_action_table_entry_count * fib_len;
        l3fe_main_hash_action_table_entry_length = fib_len;
           //PRINTF("%s(%d): total size of DDR non-cache consumed=0x%x/0x%x\n", __func__, __LINE__, size, ca_ne_ddr_noncache_reserved_size);
    }
    
    /* DEEPQ pool1 buffer */
    l3qm_eq_profile_dq_pool1_buff_start_addr = (ca_uint_t)ddr_noncache_start_addr + size;
    l3qm_eq_profile_dq_pool1_buff_phy_addr = (ca_uint32_t)ddr_noncache_phy_addr + size;
    // adjust buffer count to use remaining memory
    l3qm_eq_profile_dq_pool1_buf_count = (ca_ne_ddr_noncache_reserved_size - size) / l3qm_eq_profile_dq_pool1_buf_sz;
    size += l3qm_eq_profile_dq_pool1_buf_count * l3qm_eq_profile_dq_pool1_buf_sz;
       //PRINTF("%s(%d): total size of DDR non-cache consumed=0x%x/0x%x\n", __func__, __LINE__, size, ca_ne_ddr_noncache_reserved_size);

#else

    /* DDR coherent */
    /* ddr_coherent_base is physical address */
    /* CPU RX FIFO ,  for memory shrink, Patched from Aaron/Cortina 1116 */
    l3qm_epp_profile_cpu_rx_fifo_start_addr = (ca_uint_t)PHYS_TO_VIRT((ca_uint_t)ddr_coherent_start_addr);

    l3qm_epp_profile_cpu_rx_fifo_phy_addr = VIRT_TO_PHYS((void *)l3qm_epp_profile_cpu_rx_fifo_start_addr);
    size = l3qm_epp_profile_cpu_sz * l3qm_epp_profile_cpu_total_epp_count;
    
    /* Magic number */
    size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_epp_profile_cpu_rx_fifo_start_addr + size);
    
    PRINTF("%s: total size of DDR coherent consumed=0x%x/0x%x\n", __func__, size, ca_ne_ddr_coherent_reserved_size);
    
    if (size > ca_ne_ddr_coherent_reserved_size) {
        PRINTF("%s: out of DDR Coherent! NE driver initialized failed!!!\n", __func__);
        aal_l3qm_dump_ddr_map();
    }
    //BUG_ON(size > ca_ne_ddr_coherent_reserved_size);
    
    /* DDR non-cacheable */
    /* DEEPQ pool0 buffer */
    size = 0;
    l3qm_eq_profile_dq_pool0_buff_start_addr = (ca_uint_t)ddr_noncache_start_addr + size;
    l3qm_eq_profile_dq_pool0_buff_phy_addr = (ca_uint32_t)(uintptr_t) ddr_noncache_phy_addr + size;

    if(l3qm_eq0_sram_eq1_dram == 1) {
        // eq0 use sram, so keep size as 0 and continue eq1 config.
        size = 0;
    } else {
        size = l3qm_eq_profile_dq_pool0_buf_count * l3qm_eq_profile_dq_pool0_buf_sz;
    }

    /* Magic number */
    size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_eq_profile_dq_pool0_buff_start_addr + size);
    
    /* DEEPQ pool1 buffer */
    l3qm_eq_profile_dq_pool1_buff_start_addr = (ca_uint_t)l3qm_eq_profile_dq_pool0_buff_start_addr + size;
    l3qm_eq_profile_dq_pool1_buff_phy_addr = (ca_uint32_t)l3qm_eq_profile_dq_pool0_buff_phy_addr + size;
    size = l3qm_eq_profile_dq_pool1_buf_count * l3qm_eq_profile_dq_pool1_buf_sz;

    /* Magic number */
    size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_eq_profile_dq_pool1_buff_start_addr + size);

    /* Main hash */
    l3fe_main_hash_table_start_addr = (ca_uint_t)l3qm_eq_profile_dq_pool1_buff_start_addr + size;
    l3fe_main_hash_table_phy_addr = (ca_uint_t)l3qm_eq_profile_dq_pool1_buff_phy_addr + size;
    size = l3fe_main_hash_table_entry_count * l3fe_main_hash_table_entry_length;

    /* Magic number */
    size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3fe_main_hash_table_start_addr + size);

    /* Main FIB */
    l3fe_main_action_table_start_addr = (ca_uint_t)l3fe_main_hash_table_start_addr + size;
    l3fe_main_action_table_phy_addr = (ca_uint_t)l3fe_main_hash_table_phy_addr + size;
    size = l3fe_main_hash_action_table_entry_count * l3fe_main_hash_action_table_entry_length;

    /* Magic number */
    size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3fe_main_action_table_start_addr + size);

     
    /* PE0 EQ pool1 */
    l3qm_eq_profile_pe0_pool1_buff_start_addr = (ca_uint_t)((l3fe_main_action_table_start_addr + size + l3qm_eq_buffer_align_size) /
                                l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
    l3qm_eq_profile_pe0_pool1_buff_phy_addr = (ca_uint_t)((l3fe_main_action_table_phy_addr + size + l3qm_eq_buffer_align_size) /
            l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
    size = l3qm_eq_profile_pe0_pool1_buf_count * l3qm_eq_profile_pe0_pool1_buf_sz;

        /* Magic number */
        size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_eq_profile_pe0_pool1_buff_start_addr + size);

        /* PE1 EQ pool1 */
        l3qm_eq_profile_pe1_pool1_buff_start_addr = (ca_uint_t)((l3qm_eq_profile_pe0_pool1_buff_start_addr + size + l3qm_eq_buffer_align_size) /
                                                                l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        l3qm_eq_profile_pe1_pool1_buff_phy_addr = (ca_uint_t)((l3qm_eq_profile_pe0_pool1_buff_phy_addr + size + l3qm_eq_buffer_align_size) /
                                                                l3qm_eq_buffer_align_size) * l3qm_eq_buffer_align_size;
        size = l3qm_eq_profile_pe1_pool1_buf_count * l3qm_eq_profile_pe1_pool1_buf_sz;

        /* Magic number */
        size += aal_l3qm_insert_magic_number((void *)(ca_uint_t) l3qm_eq_profile_pe1_pool1_buff_start_addr + size);
#endif

        PRINTF( "%s: total size of DDR non-cache consumed=0x%x/0x%x\n", __func__, size, ca_ne_ddr_noncache_reserved_size);

        if (size > ca_ne_ddr_noncache_reserved_size) {
                PRINTF("%s: out of DDR non-cache! NE driver initialized failed!!!\n", __func__);
        aal_l3qm_dump_ddr_map();
    }
        //BUG_ON(size > ca_ne_ddr_noncache_reserved_size);

    aal_l3qm_dump_ddr_map();

    return CA_E_OK;
}

void aal_l3qm_init_startup_config(void)
{
    /* read additional configure */
    /* do not use CFG_ID_L3QM_MAGIC_MARKER_LEN, set l3qm_magic_marker_len=0 always */
#if defined(CONFIG_LUNA_G3_SERIES)
    l3qm_desc_size = L3QM_DESC_SIZE;
    l3qm_desc_per_epp = L3QM_DESC_PER_EPP;
#else
    #if 0
    aal_scfg_read(CFG_ID_L3QM_DESC_SIZE, 1, &l3qm_desc_size);
    aal_scfg_read(CFG_ID_L3QM_DESC_PER_EPP, 1, &l3qm_desc_per_epp);
    #endif
    l3qm_desc_size = CFG_ID_L3QM_DESC_SIZE;
    l3qm_desc_per_epp = CFG_ID_L3QM_DESC_PER_EPP;
#endif
    #if 0
    aal_scfg_read(CFG_ID_L3QM_EQ0_SRAM_EQ1_DRAM, 1, &l3qm_eq0_sram_eq1_dram);

    aal_scfg_read(CFG_ID_L3QM_WAN_SRAM_BUFFER_SIZE, 4, &l3qm_eq_sram_wan_buffer_size);
    aal_scfg_read(CFG_ID_L3QM_WAN_SRAM_BUFFER_COUNT, 4, &l3qm_eq_sram_wan_buffer_count);
    l3qm_eq_sram_wan_total_size = (l3qm_eq_sram_wan_buffer_count*l3qm_eq_sram_wan_buffer_size);

    aal_scfg_read(CFG_ID_l3QM_QOS_AUTO_THRESHOLD_VOQ_NUM, 1, &l3qm_qos_auto_threhsold_voq_num);
    aal_scfg_read(CFG_ID_l3QM_QOS_AUTO_THRESHOLD_VOQ_PRVT_BUF_CNT, 1, &l3qm_qos_auto_threhsold_voq_prvt_buf_cnt);
    

    aal_scfg_read(CFG_ID_L3QM_CPU_PORT_COUNT, 4, &l3qm_cpu_port_count);
    aal_scfg_read(CFG_ID_L3QM_CPU_VOQ_PER_PORT, 1, &l3qm_cpu_voq_per_port);
    #endif
    l3qm_eq0_sram_eq1_dram = CFG_ID_L3QM_EQ0_SRAM_EQ1_DRAM;
    l3qm_eq_sram_wan_buffer_size = CFG_ID_L3QM_WAN_SRAM_BUFFER_SIZE;
    l3qm_eq_sram_wan_buffer_count = CFG_ID_L3QM_WAN_SRAM_BUFFER_COUNT;
    l3qm_eq_sram_wan_total_size = (l3qm_eq_sram_wan_buffer_count*l3qm_eq_sram_wan_buffer_size);
    l3qm_qos_auto_threhsold_voq_num = CFG_ID_l3QM_QOS_AUTO_THRESHOLD_VOQ_NUM;
    l3qm_qos_auto_threhsold_voq_prvt_buf_cnt = CFG_ID_l3QM_QOS_AUTO_THRESHOLD_VOQ_PRVT_BUF_CNT;
    l3qm_cpu_port_count = CFG_ID_L3QM_CPU_PORT_COUNT;
    l3qm_cpu_voq_per_port = CFG_ID_L3QM_CPU_VOQ_PER_PORT;
#if defined(CONFIG_LUNA_G3_SERIES)
    l3qm_cpu_epp_per_voq = L3QM_CPU_EPP_PER_VOQ;
#else
    aal_scfg_read(CFG_ID_L3QM_CPU_EPP_PER_VOQ, 4, &l3qm_cpu_epp_per_voq);
#endif
    #if 0
    aal_scfg_read(CFG_ID_L3QM_CPU_POOL_BID_COUNT, 4, &l3qm_cpu_pool_bid_count);
    aal_scfg_read(CFG_ID_L3QM_EQ_BUFFER_ALIGN_SIZE, 4, &l3qm_eq_buffer_align_size);

    aal_scfg_read(CFG_ID_L3QM_BUF_PACKING_SIZE, 1, &l3qm_jumbo_buf_packing_size);
    aal_scfg_read(CFG_ID_L3QM_JUMBO_BUF_PINGPONG_EQS, 1, &l3qm_jumbo_buf_pingpong_eqs);
    aal_scfg_read(CFG_ID_L3QM_LDPID_TO_PON_PORT_MAP_OFFSET, 1, &l3qm_ldpid_to_pon_port_map_offset);

    /* read PE 1 configure */
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_ID, 1, &l3qm_eq_profile_pe1_id);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_EQ_ID, 1, &l3qm_eq_profile_pe1_pool0_eq_id);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_pe1_pool0_buf_count);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_BUF_SZ, 4, &l3qm_eq_profile_pe1_pool0_buf_sz);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_POOL1_EQ_ID, 1, &l3qm_eq_profile_pe1_pool1_eq_id);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_ID_PORTS, 4, &l3qm_eq_profile_pe1_id_ports);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_PE1_RULE, 1, &l3qm_eq_profile_pe1_rule);

    
    /* read CPU configure */
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_ID, 1, &l3qm_eq_profile_cpu_id);
    #endif
    l3qm_cpu_pool_bid_count = CFG_ID_L3QM_CPU_POOL_BID_COUNT;
    l3qm_eq_buffer_align_size = CFG_ID_L3QM_EQ_BUFFER_ALIGN_SIZE;
    l3qm_jumbo_buf_packing_size = CFG_ID_L3QM_BUF_PACKING_SIZE;
    l3qm_jumbo_buf_pingpong_eqs = CFG_ID_L3QM_JUMBO_BUF_PINGPONG_EQS;
    l3qm_ldpid_to_pon_port_map_offset = CFG_ID_L3QM_LDPID_TO_PON_PORT_MAP_OFFSET;
    l3qm_eq_profile_pe1_id = CFG_ID_L3QM_EQ_PROFILE_PE1_ID;
    l3qm_eq_profile_pe1_pool0_eq_id = CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_EQ_ID;
    l3qm_eq_profile_pe1_pool0_buf_count = CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_BUF_COUNT;
    l3qm_eq_profile_pe1_pool0_buf_sz = CFG_ID_L3QM_EQ_PROFILE_PE1_POOL0_BUF_SZ;
    l3qm_eq_profile_pe1_pool1_eq_id = CFG_ID_L3QM_EQ_PROFILE_PE1_POOL1_EQ_ID;
    l3qm_eq_profile_pe1_id_ports = CFG_ID_L3QM_EQ_PROFILE_PE1_ID_PORTS;
    l3qm_eq_profile_pe1_rule = CFG_ID_L3QM_EQ_PROFILE_PE1_RULE;
    l3qm_eq_profile_cpu_id = CFG_ID_L3QM_EQ_PROFILE_CPU_ID;
#if defined(CONFIG_LUNA_G3_SERIES)
    l3qm_eq_profile_cpu_rule = 0;
    l3qm_eq_profile_cpu_use_fbm = 0;
    if((ca_ni_cpu64_use_fbm == 0) && (l3qm_eq_profile_cpu_rule == 0) && (CA_NI_USE_BUILD_SKB == 0) && (ca_ni_napi_budget <= (CA_NI_ALLOC_SCATTER_SIZE-11/*for jumbo*/))){
        PRINTF( "%s: nic rx WITH allocate scatter(env: use_fbm %d, eq_rule %d, build_skb %d, budget %d) \n", __func__, ca_ni_cpu64_use_fbm, l3qm_eq_profile_cpu_rule, CA_NI_USE_BUILD_SKB, ca_ni_napi_budget);
        ca_ni_alloc_scatter = 1;
    }else{
        PRINTF("\033[1;33;40m[WARNING] %s: nic rx WITHOUT allocate scatter(env: use_fbm %d, eq_rule %d, build_skb %d, budget %d)  \033[0m\n", __func__, ca_ni_cpu64_use_fbm, l3qm_eq_profile_cpu_rule, CA_NI_USE_BUILD_SKB, ca_ni_napi_budget);
        ca_ni_alloc_scatter = 0;
    }
#else
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_RULE, 1, &l3qm_eq_profile_cpu_rule);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_USE_FBM, 1, &l3qm_eq_profile_cpu_use_fbm);
#endif
    #if 0
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_EQ_ID, 1, &l3qm_eq_profile_cpu_pool0_eq_id);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_cpu_pool0_buf_count);

    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_BID_START, 4, &l3qm_eq_profile_cpu_pool0_bid_start);
    #endif
    l3qm_eq_profile_cpu_pool0_eq_id = CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_EQ_ID;
    l3qm_eq_profile_cpu_pool0_buf_count = CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_BUF_COUNT;
    l3qm_eq_profile_cpu_pool0_bid_start = CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_BID_START;
#if defined(CONFIG_LUNA_G3_SERIES)
    l3qm_cpu_pool_bid_count = l3qm_eq_profile_cpu_pool0_buf_count;
    l3qm_eq_profile_cpu_pool0_buf_sz = L3QM_EQ_PROFILE_CPU_POOL0_BUF_SZ;
#else
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_BUF_SZ, 4, &l3qm_eq_profile_cpu_pool0_buf_sz);
#endif
    #if 0
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu_pool0_fbm_pool_id);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_EQ_ID, 1, &l3qm_eq_profile_cpu_pool1_eq_id);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_cpu_pool1_buf_count);

    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_BID_START, 4, &l3qm_eq_profile_cpu_pool1_bid_start);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_BUF_SZ, 4, &l3qm_eq_profile_cpu_pool1_buf_sz);

    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_FBM_POOL_ID, 1, &l3qm_eq_profile_cpu_pool1_fbm_pool_id);
    aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU_ID, 1, &l3qm_epp_profile_cpu_id);
    aal_scfg_read(CFG_ID_L3QM_EPP_PROFILE_CPU_MAP_MODE, 4, &l3qm_epp_profile_cpu_map_mode);

    /* read DEEPQ configure */
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_ID, 1, &l3qm_eq_profile_dq_id);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_RULE, 1, &l3qm_eq_profile_dq_rule);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_EQ_ID, 1, &l3qm_eq_profile_dq_pool0_eq_id);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_BUF_COUNT, 4, &l3qm_eq_profile_dq_pool0_buf_count);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_BID_START, 4, &l3qm_eq_profile_dq_pool0_bid_start);
    aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_BUF_SZ, 4, &l3qm_eq_profile_dq_pool0_buf_sz);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_EQ_ID, 1, &l3qm_eq_profile_dq_pool1_eq_id);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_BUF_COUNT, 4, &l3qm_eq_profile_dq_pool1_buf_count);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_BID_START, 4, &l3qm_eq_profile_dq_pool1_bid_start);
        aal_scfg_read(CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_BUF_SZ, 4, &l3qm_eq_profile_dq_pool1_buf_sz);

    /* read L3FE Main hash configure */
    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_OVERFLOW_FIB_LENGTH, 4, &l3fe_main_hash_overflow_hash_fib_length);
    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_OVERFLOW_FIB_COUNT, 4, &l3fe_main_hash_overflow_hash_fib_count);
    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_DEFAULT_HASH_FIB_LENGTH, 4, &l3fe_main_hash_default_hash_fib_length);
    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_DEFAULT_HASH_FIB_COUNT, 4, &l3fe_main_hash_default_hash_fib_count);
    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_ACTION_CACHE_FIB_LENGTH, 4, &l3fe_main_hash_action_cache_fib_length);
    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_ACTION_CACHE_FIB_COUNT, 4, &l3fe_main_hash_action_cache_fib_count);
    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_TABLE_ENTRY_LENGTH, 4, &l3fe_main_hash_table_entry_length);
    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_TABLE_ENTRY_COUNT, 4, &l3fe_main_hash_table_entry_count);
    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_ACTION_TABLE_ENTRY_LENGTH, 4, &l3fe_main_hash_action_table_entry_length);
    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_ACTION_TABLE_ENTRY_COUNT, 4, &l3fe_main_hash_action_table_entry_count);

    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_DEFAULT_HASH_FIB_REG_MODE, 1, &l3fe_main_hash_flow_default_fib_internal_reg_mode);
    aal_scfg_read(CFG_ID_L3FE_MAIN_HASH_CACHE_FIB_INTER_SRAM_MODE, 1, &l3fe_main_hash_flow_cache_fib_internal_sram_mode);
    #endif
    l3qm_eq_profile_cpu_pool0_fbm_pool_id = CFG_ID_L3QM_EQ_PROFILE_CPU_POOL0_FBM_POOL_ID;
    l3qm_eq_profile_cpu_pool1_eq_id = CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_EQ_ID;
    l3qm_eq_profile_cpu_pool1_buf_count = CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_BUF_COUNT;
    l3qm_eq_profile_cpu_pool1_bid_start = CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_BID_START;
    l3qm_eq_profile_cpu_pool1_buf_sz = CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_BUF_SZ;
    l3qm_eq_profile_cpu_pool1_fbm_pool_id = CFG_ID_L3QM_EQ_PROFILE_CPU_POOL1_FBM_POOL_ID;
    l3qm_epp_profile_cpu_id = CFG_ID_L3QM_EPP_PROFILE_CPU_ID;
    l3qm_epp_profile_cpu_map_mode = CFG_ID_L3QM_EPP_PROFILE_CPU_MAP_MODE;
    l3qm_eq_profile_dq_id = CFG_ID_L3QM_EQ_PROFILE_DQ_ID;
    l3qm_eq_profile_dq_rule = CFG_ID_L3QM_EQ_PROFILE_DQ_RULE;
    l3qm_eq_profile_dq_pool0_eq_id = CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_EQ_ID;
    l3qm_eq_profile_dq_pool0_buf_count = CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_BUF_COUNT;
    l3qm_eq_profile_dq_pool0_bid_start = CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_BID_START;
    l3qm_eq_profile_dq_pool0_buf_sz = CFG_ID_L3QM_EQ_PROFILE_DQ_POOL0_BUF_SZ;
    l3qm_eq_profile_dq_pool1_eq_id = CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_EQ_ID;
    l3qm_eq_profile_dq_pool1_buf_count = CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_BUF_COUNT;
    l3qm_eq_profile_dq_pool1_bid_start = CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_BID_START;
    l3qm_eq_profile_dq_pool1_buf_sz = CFG_ID_L3QM_EQ_PROFILE_DQ_POOL1_BUF_SZ;
    l3fe_main_hash_overflow_hash_fib_length = CFG_ID_L3FE_MAIN_HASH_OVERFLOW_FIB_LENGTH;
    l3fe_main_hash_overflow_hash_fib_count = CFG_ID_L3FE_MAIN_HASH_OVERFLOW_FIB_COUNT;
    l3fe_main_hash_default_hash_fib_length = CFG_ID_L3FE_MAIN_HASH_DEFAULT_HASH_FIB_LENGTH;
    l3fe_main_hash_default_hash_fib_count = CFG_ID_L3FE_MAIN_HASH_DEFAULT_HASH_FIB_COUNT;
    l3fe_main_hash_action_cache_fib_length = CFG_ID_L3FE_MAIN_HASH_ACTION_CACHE_FIB_LENGTH;
    l3fe_main_hash_action_cache_fib_count = CFG_ID_L3FE_MAIN_HASH_ACTION_CACHE_FIB_COUNT;
    l3fe_main_hash_table_entry_length = CFG_ID_L3FE_MAIN_HASH_TABLE_ENTRY_LENGTH;
    l3fe_main_hash_table_entry_count = CFG_ID_L3FE_MAIN_HASH_TABLE_ENTRY_COUNT;
    l3fe_main_hash_action_table_entry_length = CFG_ID_L3FE_MAIN_HASH_ACTION_TABLE_ENTRY_LENGTH;
    l3fe_main_hash_action_table_entry_count = CFG_ID_L3FE_MAIN_HASH_ACTION_TABLE_ENTRY_COUNT;
    l3fe_main_hash_flow_default_fib_internal_reg_mode = CFG_ID_L3FE_MAIN_HASH_DEFAULT_HASH_FIB_REG_MODE;
    l3fe_main_hash_flow_cache_fib_internal_sram_mode = CFG_ID_L3FE_MAIN_HASH_CACHE_FIB_INTER_SRAM_MODE;

#if defined(CONFIG_LUNA_G3_SERIES)
    l3qm_cpu_port_head_room_first = L3QM_CPU_PORT_HEAD_ROOM_FIRST;
    l3qm_ace_test = L3QM_ACE_TEST;
#else
    /* read CPU port head room first */
    aal_scfg_read(CFG_ID_L3QM_CPU_PORT_HEAD_ROOM_FIRST, 4, &l3qm_cpu_port_head_room_first);

    /* read QM ACE test bit */
    aal_scfg_read(CFG_ID_L3QM_ACE_TEST, 1, &l3qm_ace_test);
#endif

    /* read Main Hash Bench Mark test */
    #if 0
    aal_scfg_read(CFG_ID_MAIN_HASH_BM_TEST, 1, &l3qm_main_hash_bm_test);
    #endif
    l3qm_main_hash_bm_test = CFG_ID_MAIN_HASH_BM_TEST;

    /* read EQ QM_CFG3/4 */
    #if 0
    aal_scfg_read(CFG_ID_L3QM_EQ_CFG3_VALUES, CFG_ID_L3QM_EQ_CFG3_VALUES_LEN * sizeof(ca_uint32_t), l3qm_eq_cfg3_values);
    aal_scfg_read(CFG_ID_L3QM_EQ_CFG4_VALUES, CFG_ID_L3QM_EQ_CFG4_VALUES_LEN * sizeof(ca_uint32_t), l3qm_eq_cfg4_values);
    #endif
    ca_uint32_t _l3qm_eq_cfg3_values[CFG_ID_L3QM_EQ_CFG3_VALUES_LEN] = CFG_ID_L3QM_EQ_CFG3_VALUES;
    ca_uint32_t _l3qm_eq_cfg4_values[CFG_ID_L3QM_EQ_CFG4_VALUES_LEN] = CFG_ID_L3QM_EQ_CFG4_VALUES;
    memcpy(l3qm_eq_cfg3_values, _l3qm_eq_cfg3_values, CFG_ID_L3QM_EQ_CFG3_VALUES_LEN * sizeof(ca_uint32_t));
    memcpy(l3qm_eq_cfg4_values, _l3qm_eq_cfg4_values, CFG_ID_L3QM_EQ_CFG4_VALUES_LEN * sizeof(ca_uint32_t));

    /* read QM AXI ATTRIBUTE */
    #if 0
    aal_scfg_read(CFG_ID_L3QM_AXI_ATTRIB_EQ_VALUES, CFG_ID_L3QM_AXI_ATTRIB_EQ_VALUES_LEN * sizeof(ca_uint32_t), l3qm_axi_attrib_eq_values);
    #endif
    ca_uint32_t _l3qm_axi_attrib_eq_values[CFG_ID_L3QM_AXI_ATTRIB_EQ_VALUES_LEN] = CFG_ID_L3QM_AXI_ATTRIB_EQ_VALUES;
    memcpy(l3qm_axi_attrib_eq_values, _l3qm_axi_attrib_eq_values, CFG_ID_L3QM_AXI_ATTRIB_EQ_VALUES_LEN * sizeof(ca_uint32_t));
    //aal_scfg_read(CFG_ID_RTK_EPP_64BIT_MODE, 1, &epp_mode);
    l3qm_epp_profile_cpu_total_epp_count = (ca_ni_cpu_cpu_port_count * l3qm_cpu_voq_per_port * l3qm_cpu_epp_per_voq);  /* CPU total EPP count */

    #if 0
    aal_scfg_read(CFG_ID_L3QM_AXI_ATTRIB_CPU_EPP_VALUES, CFG_ID_L3QM_AXI_ATTRIB_CPU_EPP_VALUES_LEN * sizeof(ca_uint32_t), l3qm_axi_attrib_cpu_epp_values);
    #endif
    ca_uint32_t _l3qm_axi_attrib_cpu_epp_values[CFG_ID_L3QM_AXI_ATTRIB_CPU_EPP_VALUES_LEN] = CFG_ID_L3QM_AXI_ATTRIB_CPU_EPP_VALUES;
    memcpy(l3qm_axi_attrib_cpu_epp_values, _l3qm_axi_attrib_cpu_epp_values, CFG_ID_L3QM_AXI_ATTRIB_CPU_EPP_VALUES_LEN * sizeof(ca_uint32_t));

    /* calculate from startup configs */
    l3qm_epp_size = (l3qm_desc_size * l3qm_desc_per_epp);   /* EPP size in byte */

    l3qm_epp_profile_pe0_sz = l3qm_epp_size;
    l3qm_epp_profile_pe0_cn = l3qm_eq_profile_pe0_1_total_epp_count;

    l3qm_epp_profile_pe1_sz = l3qm_epp_size;
    l3qm_epp_profile_pe1_cn = l3qm_eq_profile_pe0_1_total_epp_count;

    l3qm_epp_profile_cpu_sz = l3qm_epp_size;
    l3qm_epp_profile_cpu_cn = l3qm_epp_profile_cpu_total_epp_count;

    #if 0
    aal_scfg_read(CFG_ID_RTK_CPUEPP_SRAM_MODE_ENABLE, CFG_ID_RTK_CPUEPP_SRAM_MODE_ENABLE_LEN * sizeof(ca_uint8_t), l3qm_cpuepp_sram_enable);
    aal_scfg_read(CFG_ID_RTK_DMALSO_SRAM_MODE_ENABLE, CFG_ID_RTK_DMALSO_SRAM_MODE_ENABLE_LEN * sizeof(ca_uint8_t), l3qm_dmalso_sram_enable);
    #endif
    ca_uint8_t  _l3qm_cpuepp_sram_enable[CFG_ID_RTK_CPUEPP_SRAM_MODE_ENABLE_LEN] = CFG_ID_RTK_CPUEPP_SRAM_MODE_ENABLE;
    ca_uint8_t  _l3qm_dmalso_sram_enable[CFG_ID_RTK_DMALSO_SRAM_MODE_ENABLE_LEN] = CFG_ID_RTK_DMALSO_SRAM_MODE_ENABLE;
    memcpy(l3qm_cpuepp_sram_enable, _l3qm_cpuepp_sram_enable, CFG_ID_RTK_CPUEPP_SRAM_MODE_ENABLE_LEN * sizeof(ca_uint8_t));
    memcpy(l3qm_dmalso_sram_enable, _l3qm_dmalso_sram_enable, CFG_ID_RTK_DMALSO_SRAM_MODE_ENABLE_LEN * sizeof(ca_uint8_t));

    // "l3qm_eq_profile_dq_pool1_buf_count" may be adjusted by aal_l3qm_create_ddr_map() and aal_l3qm_init_DQ_pools_pool1().
}

void aal_l3qm_set_cpu_rx_fifo_addr(ca_uint_t cpu_rx_fifo_phy_addr, ca_uint_t cpu_rx_fifo_virt_addr)
{
        l3qm_epp_profile_cpu_rx_fifo_phy_addr = cpu_rx_fifo_phy_addr;
        l3qm_epp_profile_cpu_rx_fifo_start_addr = cpu_rx_fifo_virt_addr;
}

ca_uint32_t aal_l3qm_get_cpu_rx_fifo_size(void)
{
        return l3qm_epp_profile_cpu_sz * l3qm_epp_profile_cpu_cn + (l3qm_magic_marker_len * ca_ni_cpu_cpu_port_count * l3qm_cpu_voq_per_port);
}

void aal_l3qm_init_reserved_mem(void *sram_base, ca_uint32_t sram_size,
    void *sram_phy_base, void *ddr_coherent_base, void *ddr_noncache_base, void *ddr_noncache_phy_base)
{
    /* create sram map */
    aal_l3qm_create_sram_map(sram_base, sram_size, sram_phy_base);

    /* create ddr map */
    aal_l3qm_create_ddr_map(ddr_coherent_base, ddr_noncache_base, ddr_noncache_phy_base);
}

void aal_l3qm_init_voq(void)
{
    int i;
    QM_QM_SCH_CFG0_t sch_cfg;

    sch_cfg.wrd = CA_NE_REG_READ(QM_QM_SCH_CFG0);
    for (i = 0; i < QM_QM_SCH_CFG0_COUNT; i++) {

        sch_cfg.bf.voq_en = 0xff;
        CA_NE_REG_WRITE(sch_cfg.wrd, QM_QM_SCH_CFG0 + (i * QM_QM_SCH_CFG0_STRIDE));
    }
}

ca_uint32_t aal_l3qm_check_init_done(void)
{
    int i;
    QM_QM_PHY_PORT_STS_t    init_done;

        /* Check initializtion of NI block is done */
        for (i = 0; i < CA_L3QM_INIT_DONE_LOOP; i++) {
                init_done.wrd = CA_NE_REG_READ(QM_QM_PHY_PORT_STS);
                if (init_done.bf.qm_init_done)
                        break;
        }
        if (i == CA_L3QM_INIT_DONE_LOOP) {
                PRINTF("%s: initializtion of QM block is NOT done!!!\n", __func__);
        return -1;
        }
    return 0;
}

/*
 * Procedure of retrieving a buffer back to CPU:
 * 1. Disable the Empty-buffer pool which physical address is going to be
 * retrieveled from;
 * 2. Enable "buf_retrieval" by writing "1" to this field and
 * the corresponding EQ-ID;
 * 3. read "EQ_RETURN_PHY_ADDR%d_phy_addr", with valid entry indication;
 * 4. repeat "#2" if more buffers need to be retrieved;
 * 5. Write "0" to "buf_retrieval_en" to disable this function;
*/
void aal_l3qm_get_return_phy_addr_enable(int eq_id, int enable)
{
    volatile ca_uint32_t reg_off;
    QM_QM_CFG0_EQ0_t cfg0_eq;
    QM_QM_EQ_PHY_ADDR_RETURN_CTRL_t   phy_addr_return_ctrl;

    if (enable) {
        /* disable Empty Buffer Pool */
        cfg0_eq.wrd = 0;
        cfg0_eq.bf.eq_en = 0;
        reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * eq_id);
        CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

        /* Enable "buf_retieval" */
        phy_addr_return_ctrl.bf.phy_addr_retrieval_en = 1;
        phy_addr_return_ctrl.bf.eq_id = eq_id;
        CA_NE_REG_WRITE(phy_addr_return_ctrl.wrd, QM_QM_EQ_PHY_ADDR_RETURN_CTRL);
    }
    else {
        /* disable "buf_retieval" */
        phy_addr_return_ctrl.bf.phy_addr_retrieval_en = 0;
        phy_addr_return_ctrl.bf.eq_id = eq_id;
        CA_NE_REG_WRITE(phy_addr_return_ctrl.wrd, QM_QM_EQ_PHY_ADDR_RETURN_CTRL);

        /* enable Empty Buffer Pool */
        cfg0_eq.wrd = 0;
        cfg0_eq.bf.eq_en = 1;
        reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * eq_id);
        CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);
    }
}

ca_uint32_t aal_l3qm_get_return_phy_addr(int eq_id)
{
    QM_QM_EQ_PHY_ADDR_RETURN_CTRL_t   phy_addr_return_ctrl;
    QM_QM_EQ_RETURN_PHY_ADDR_t        return_phy_addr;

    /* Enable "buf_retieval" */
    phy_addr_return_ctrl.bf.phy_addr_retrieval_en = 1;
    phy_addr_return_ctrl.bf.eq_id = eq_id;
    CA_NE_REG_WRITE(phy_addr_return_ctrl.wrd, QM_QM_EQ_PHY_ADDR_RETURN_CTRL);

        /* read "EQ_RETURN_PHY_ADD%d_phy_addr" */
        return_phy_addr.wrd = CA_NE_REG_READ(QM_QM_EQ_RETURN_PHY_ADDR);

    if (ca_aal_debug & AAL_DBG_L3QM) {
        PRINTF("%s: eqid=%d, retrieval_done=%d, valid=%d\n", __func__, eq_id, return_phy_addr.bf.retrieval_done, return_phy_addr.bf.valid);
        PRINTF("%s: return_phy_addr.wrd=0x%x, phy_addr=0x%x\n", __func__, return_phy_addr.wrd, return_phy_addr.bf.phy_addr << 7);
    }

    /* check retrieval_done and valid bits */
    if (return_phy_addr.bf.retrieval_done) {
        return 0;
    }

    if (return_phy_addr.bf.valid)
        return (return_phy_addr.bf.phy_addr << CA_L3QM_PHY_ADDR_SHIFT);

        return 0;
}

/* functions for debugging print out used with iROS */
#define VOQ_INFO_ACCESS_TIMEOUT         1000000
#define HEADER_MEMORY_ACCESS_TIMEOUT    1000000
#define BID_PADDR_ACCESS_TIMEOUT        1000000
ca_status_t aal_l3qm_get_voq_info_mem(ca_device_id_t device_id, ca_uint16_t addr, QM_QM_VOQ_INFO_MEMORY_DATA0_t *data0,
                    QM_QM_VOQ_INFO_MEMORY_DATA1_t *data1, QM_QM_VOQ_INFO_MEMORY_DATA2_t *data2)
{
    int i;
    QM_QM_VOQ_INFO_MEMORY_ACCESS_t  access;

    access.bf.access = 1;
    access.bf.rbw = 0;
    access.bf.ADDR = addr;
    CA_NE_REG_WRITE(access.wrd, QM_QM_VOQ_INFO_MEMORY_ACCESS);

    for (i = 0; i < VOQ_INFO_ACCESS_TIMEOUT; i++) {
        access.wrd = CA_NE_REG_READ(QM_QM_VOQ_INFO_MEMORY_ACCESS);
        if (access.bf.access == 0) {
            break;
        }
    }
    if (i == VOQ_INFO_ACCESS_TIMEOUT) {
        PRINTF("%s: VoQ addr=%d access timeout!!!\n", __func__, addr);
        return CA_E_ERROR;
    }
    data0->wrd = CA_NE_REG_READ(QM_QM_VOQ_INFO_MEMORY_DATA0);
    data1->wrd = CA_NE_REG_READ(QM_QM_VOQ_INFO_MEMORY_DATA1);
    data2->wrd = CA_NE_REG_READ(QM_QM_VOQ_INFO_MEMORY_DATA2);

    PRINTF("[addr=%d]\n", addr);
    PRINTF("voq_header=0x%x\n", data0->bf.voq_header);

    PRINTF("voq_header=%d, voq_tail=0x%x, voq_head=0x%x, bcntr=0x%x\n",
        data1->bf.voq_header, data1->bf.voq_tail, data1->bf.voq_head, data1->bf.bcntr);

    PRINTF("bcntr=0x%x, ecc=0x%x\n", data2->bf.bcntr, data2->bf.ecc);

    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_get_voq_info_mem);

ca_status_t aal_l3qm_get_header_mem(ca_device_id_t device_id, ca_uint16_t addr, QM_QM_HEADER_MEMORY_DATA0_t *data0, QM_QM_HEADER_MEMORY_DATA1_t *data1)
{
        int i;
        QM_QM_HEADER_MEMORY_ACCESS_t      access;

        access.bf.access = 1;
        access.bf.rbw = 0;
        access.bf.ADDR = addr;
        CA_NE_REG_WRITE(access.wrd, QM_QM_HEADER_MEMORY_ACCESS);

        for (i = 0; i < HEADER_MEMORY_ACCESS_TIMEOUT; i++) {
                access.wrd = CA_NE_REG_READ(QM_QM_HEADER_MEMORY_ACCESS);
                if (access.bf.access == 0) {
                        break;
                }
        }
        if (i == HEADER_MEMORY_ACCESS_TIMEOUT) {
                PRINTF("%s: header memory addr=%d access timeout!!!\n", __func__, addr);
                return CA_E_ERROR;
        }
        data0->wrd = CA_NE_REG_READ(QM_QM_HEADER_MEMORY_DATA0);
        data1->wrd = CA_NE_REG_READ(QM_QM_HEADER_MEMORY_DATA1);

    PRINTF("[addr=%d]\n", addr);

    PRINTF("next_buffer_ptr=%d, sop=%d, eop=%d, pkt_length=%d\n", data0->bf.next_buffer_ptr, data0->bf.sop, data0->bf.eop, data0->bf.pkt_length);

    PRINTF("ecc=%d, pkt_type_ptp=%d\n", data1->bf.ecc, data1->bf.pkt_type_ptp);

        return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_get_header_mem);

ca_status_t aal_l3qm_get_bid_paddr(ca_device_id_t device_id, ca_uint16_t addr, QM_QM_BID_PADDR_LKUP_DATA_t *data)
{
        int i;
        QM_QM_BID_PADDR_LKUP_ACCESS_t      access;

        access.bf.access = 1;
        access.bf.rbw = 0;
        access.bf.ADDR = addr;
        CA_NE_REG_WRITE(access.wrd, QM_QM_BID_PADDR_LKUP_ACCESS);

        for (i = 0; i < BID_PADDR_ACCESS_TIMEOUT; i++) {
                access.wrd = CA_NE_REG_READ(QM_QM_BID_PADDR_LKUP_ACCESS);
                if (access.bf.access == 0) {
                        break;
                }
        }
        if (i == BID_PADDR_ACCESS_TIMEOUT) {
                PRINTF("%s: BID paddr addr=%d access timeout!!!\n", __func__, addr);
                return CA_E_ERROR;
        }
        data->wrd = CA_NE_REG_READ(QM_QM_BID_PADDR_LKUP_DATA);

        PRINTF("%s: paddr=0x%x\n", __func__, data->bf.phy_addr);

        return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_get_bid_paddr);

ca_status_t aal_l3qm_get_tx_rx_cntr(ca_device_id_t device_id)
{
    QM_QM_TX_PKT_CNTR_t  tx_pkt_cntr;
    QM_QM_RMU0_RX_PKT_CNTR_t        rmu0_rx_pkt_cntr;

    tx_pkt_cntr.wrd = CA_NE_REG_READ(QM_QM_TX_PKT_CNTR);
    PRINTF("%s: packtes transmitted from L3QM count=%d\n", __func__, tx_pkt_cntr.bf.cntr);

    rmu0_rx_pkt_cntr.wrd = CA_NE_REG_READ(QM_QM_RMU0_RX_PKT_CNTR);
    PRINTF("%s: packets received by L3QM count=%d\n", __func__, rmu0_rx_pkt_cntr.bf.cntr);

    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_get_tx_rx_cntr);

ca_status_t aal_l3qm_get_monitor_eq(ca_device_id_t device_id)
{
    QM_QM_MONITOR0_EQ0_t monitor0_eq0;
    QM_QM_MONITOR1_EQ0_t monitor1_eq0;
    QM_QM_MONITOR2_EQ0_t monitor2_eq0;

    monitor0_eq0.wrd = CA_NE_REG_READ(QM_QM_MONITOR0_EQ0);
    PRINTF("%s: Monitor0 EQ0, head=%d, tail=%d\n", __func__, monitor0_eq0.bf.head, monitor0_eq0.bf.tail);

    monitor1_eq0.wrd = CA_NE_REG_READ(QM_QM_MONITOR1_EQ0);
    PRINTF("%s: Monitor1 EQ0, inactive_head=%d, cntr=%d\n", __func__, monitor1_eq0.bf.inactive_head, monitor1_eq0.bf.cntr);

    monitor2_eq0.wrd = CA_NE_REG_READ(QM_QM_MONITOR2_EQ0);
    PRINTF("%s: Monitor2 EQ0, inactive_tail=%d, inactive_cntr=%d\n", __func__, monitor2_eq0.bf.inactive_tail, monitor2_eq0.bf.inactive_cntr);

    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_get_monitor_eq);

ca_status_t aal_l3qm_dump_epps(ca_device_id_t device_id)
{
    int i, j;
    ca_uint32_t reg_off;
    QM_QM_CPU_EPP0_CFG_t cpu_epp0_cfg;
    QM_QM_CPU_EPP_FIFO_CFG_profile0_t cpu_epp_fifo_cfg_profile;
    QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_t cpu_epp64_fifo_paddr_start;
    QM_QM_CPU_EPP_FIFO0_0_CFG_t cpu_epp_fifo_cfg;

    /* CPU EPP Port Configuration Register */
    /* Select per-destionation VOQ-to-EPP-FIFO-Ring map-mode */
    PRINTF("=== QM_QM_CPU_EPP0_CFG ===\n");
    for (i = 0; i < CA_NI_TOTAL_CPU_PORT; i++) {
        reg_off = QM_QM_CPU_EPP0_CFG + (QM_QM_CPU_EPP0_CFG_STRIDE * i);
        cpu_epp0_cfg.wrd = CA_NE_REG_READ(reg_off);
        PRINTF("index=%d, reg_off=0x%x, map_mode=%d\n", i, reg_off, cpu_epp0_cfg.bf.map_mode);
    }

    /* CPU EPP FIFO Configuration Register */
    PRINTF("=== QM_QM_CPU_EPP_FIFO_CFG_profile0 ===\n");
    for (i = 0; i < QM_QM_CPU_EPP_FIFO_CFG_profile0_COUNT; i++) {
        reg_off = QM_QM_CPU_EPP_FIFO_CFG_profile0 + (QM_QM_CPU_EPP_FIFO_CFG_profile0_STRIDE * i);
        cpu_epp_fifo_cfg_profile.wrd = CA_NE_REG_READ(reg_off);
        PRINTF("profile id=%d, reg_off=0x%x, size=%d\n", i, reg_off, cpu_epp_fifo_cfg_profile.bf.size);
    }

        PRINTF("=== QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START/QM_QM_CPU_EPP_FIFO0_0_CFG ===\n");
    for (j = 0; j < QM_QM_CPU_EPP_FIFO0_0_CFG_COUNT; j++) {
        for (i = 0; i < l3qm_cpu_voq_per_port; i++) {

                        reg_off = QM_QM_CPU_EPP_FIFO0_0_CFG + (QM_QM_CPU_EPP_FIFO0_0_CFG_STRIDE * j) + (4 * i);
                        cpu_epp_fifo_cfg.wrd = CA_NE_REG_READ(reg_off);

            reg_off = QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START + (i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE) +
                (j * l3qm_cpu_voq_per_port * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE);
            cpu_epp64_fifo_paddr_start.wrd = CA_NE_REG_READ(reg_off);

            PRINTF("port id=%d, voq id=%d, reg_off=0x%x, paddr_start=0x%x, profile sel=%d\n", j, i, reg_off,
                    cpu_epp64_fifo_paddr_start.bf.phy_addr, cpu_epp_fifo_cfg.bf.profile_sel);
        }
    }

    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_dump_epps);

ca_status_t aal_l3qm_dump_empty_buffers(ca_device_id_t device_id)
{
    int i;
    volatile ca_uint32_t reg_off;
    QM_QM_EQ_PROFILE0_t eq_profile;
    QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
    QM_QM_CFG0_EQ0_t cfg0_eq;
    QM_QM_CFG1_EQ0_t cfg1_eq;
    QM_QM_CFG2_EQ0_t cfg2_eq;
    QM_QM_CFG3_EQ0_t cfg3_eq;
    QM_QM_CFG4_EQ0_t cfg4_eq;

    PRINTF("=== QM_QM_EQ_PROFILE0 ===\n");
    for (i = 0; i < QM_QM_EQ_PROFILE0_COUNT; i++) {
        reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * i);
        eq_profile.wrd = CA_NE_REG_READ(reg_off);
        PRINTF("profile id=%d, reg_off=0x%x, rule=%d, eqp0=%d, eqp1=%d\n", i, reg_off, eq_profile.bf.rule, eq_profile.bf.eqp0, eq_profile.bf.eqp1);
    }

    PRINTF(" === QM_QM_DEST_PORT0_EQ_CFG ===\n");
    for (i = 0; i < QM_QM_DEST_PORT0_EQ_CFG_COUNT; i++) {
        reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * i);
        dest_port_eq_cfg.wrd = CA_NE_REG_READ(reg_off);
        PRINTF("port id=%d, reg_off=0x%x, profile_sel=%d\n", i, reg_off, dest_port_eq_cfg.bf.profile_sel);
    }

    PRINTF("=== QM_QM_CFG0_EQ0 ===\n");
    for (i = 0; i < QM_QM_CFG0_EQ0_COUNT; i++) {
        reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * i);
        cfg0_eq.wrd = CA_NE_REG_READ(reg_off);
        PRINTF("index=%d, reg_off=0x%x, phy_addr_start=0x%x, eq_en=%d\n", i, reg_off, cfg0_eq.bf.phy_addr_start << CA_L3QM_PHY_ADDR_SHIFT, cfg0_eq.bf.eq_en);
    }

    PRINTF("=== QM_QM_CFG1_EQ0 ===\n");
    for (i = 0; i < QM_QM_CFG1_EQ0_COUNT; i++) {
        reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * i);
        cfg1_eq.wrd = CA_NE_REG_READ(reg_off);
        PRINTF("index=%d, reg_off=0x%x, total_buffer_num=%d, bid_start=%d\n", i, reg_off, cfg1_eq.bf.total_buffer_num, cfg1_eq.bf.bid_start);
    }

    PRINTF("=== QM_QM_CFG2_EQ0 ===\n");
    for (i = 0; i < QM_QM_CFG2_EQ0_COUNT; i++) {
        reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * i);
        cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
        PRINTF("index=%d, reg_off=0x%x, refill_ths=%d, refill_en=%d, refill_fbm_eqid=%d, cpu_eq=%d, buffer_size=%d\n",
                i, reg_off, cfg2_eq.bf.refill_ths, cfg2_eq.bf.refill_en, cfg2_eq.bf.refill_fbm_eqid, cfg2_eq.bf.cpu_eq, cfg2_eq.bf.buffer_size);
    }

    PRINTF("=== QM_QM_CFG3_EQ0 ===\n");
    for (i = 0; i < QM_QM_CFG3_EQ0_COUNT; i++) {
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * i);
        cfg3_eq.wrd = CA_NE_REG_READ(reg_off);
        PRINTF("index=%d, reg_off=0x%x, cfg3_eq.wrd=0x%x\n", i, reg_off, cfg3_eq.wrd);
        PRINTF("qos=%d, cache=%d, snoop=%d, bar=%d, domain=%d, user=%d, acd_cmd=%d, cache_line_split=%d, cache_eos=%d\n",
            cfg3_eq.bf.qos, cfg3_eq.bf.cache, cfg3_eq.bf.snoop, cfg3_eq.bf.bar, cfg3_eq.bf.domain, cfg3_eq.bf.user,
            cfg3_eq.bf.acd_cmd, cfg3_eq.bf.cache_line_split, cfg3_eq.bf.cache_eos);
    }

    PRINTF("=== QM_QM_CFG4_EQ0 ===\n");
    for (i = 0; i < QM_QM_CFG4_EQ0_COUNT; i++) {
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * i);
        cfg4_eq.wrd = CA_NE_REG_READ(reg_off);
        PRINTF("index=%d, reg_off=0x%x, cfg4_eq.wrd=0x%x\n", i, reg_off, cfg4_eq.wrd);
    }
    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_dump_empty_buffers);

ca_status_t aal_l3qm_dump_empty_buffers_phy_addr(ca_device_id_t device_id)
{
    int i;
    ca_uint32_t phy_addr;
#if defined(CONFIG_LUNA_G3_SERIES)
    QM_QM_EQ_REFILL_INTERRUPT_EN_t refill_int_en;
    uint8 *rx_virt_addr;
    struct sk_buff *skb;
    int poolid = 0;

    /* backup/disable refill interrupt */
    refill_int_en.wrd = CA_NE_REG_READ(QM_QM_EQ_REFILL_INTERRUPT_EN);
    CA_NE_REG_WRITE(0, QM_QM_EQ_REFILL_INTERRUPT_EN);

    PRINTF("=== QM_QM_EQ_RETURN_PHY_ADDR ===\n");

    for(poolid = l3qm_eq_profile_cpu_pool0_eq_id; poolid < (l3qm_eq_profile_cpu_pool0_eq_id+L3QM_EQ_PROFILE_CPU_COUNT); poolid++) { 
        PRINTF("EQ=%d\n", poolid);
        aal_l3qm_get_return_phy_addr_enable(poolid, 1);
        i=0;
        while ((phy_addr = aal_l3qm_get_return_phy_addr(poolid)) != 0) {
            rx_virt_addr = PHYS_TO_VIRT(phy_addr);
#if defined(L3QM_CPU_ARM_WIFI_FIX_2KB)
            if(poolid == (l3qm_eq_profile_cpu_pool0_eq_id+L3QM_CPU_ARM_WIFI_AGG_PORT_IDX)) {
                skb = (struct sk_buff *)(*((ca_uint_t *)(rx_virt_addr+CA_NI_SKB_STORE_HEAD_OFFSET-L3QM_CPU_PORT_HEAD_ROOM_FIRST)));
            }else
#endif
            skb = (struct sk_buff *)(*((ca_uint_t *)(rx_virt_addr+CA_NI_SKB_STORE_HEAD_OFFSET)));
            PRINTF("EQ[%d] buffer[%d] phy_addr=0x%x skb=%px\n", poolid, i, phy_addr, skb);
            
            i++;
            dev_kfree_skb_any(skb);
        }
        aal_l3qm_get_return_phy_addr_enable(poolid, 0);

    }

    
    /* restore/enable refill interrupt */
    CA_NE_REG_WRITE(refill_int_en.wrd, QM_QM_EQ_REFILL_INTERRUPT_EN);
    
#else

    PRINTF("=== QM_QM_EQ_RETURN_PHY_ADDR ===\n");
    PRINTF("EQ=%d\n", l3qm_eq_profile_cpu_pool0_eq_id);
    aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu_pool0_eq_id, 1);
    i=0;
    while ((phy_addr = aal_l3qm_get_return_phy_addr(l3qm_eq_profile_cpu_pool0_eq_id)) != 0) {
        PRINTF("[%d] phy_addr=0x%x\n", i, phy_addr);
        i++;
    }
    aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu_pool0_eq_id, 0);

        PRINTF("EQ=%d\n", l3qm_eq_profile_cpu_pool1_eq_id);
        aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu_pool1_eq_id, 1);
        i=0;
        while ((phy_addr = aal_l3qm_get_return_phy_addr(l3qm_eq_profile_cpu_pool1_eq_id)) != 0) {
                PRINTF("[%d] phy_addr=0x%x\n", i, phy_addr);
                i++;
        }
        aal_l3qm_get_return_phy_addr_enable(l3qm_eq_profile_cpu_pool1_eq_id, 0);

#endif


    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_dump_empty_buffers_phy_addr);

ca_status_t aal_l3qm_dump_rmu_fe_drop_counter(ca_device_id_t device_id)
{
    int addr;
    QM_QM_RMU0_FE_DROP_PKT_CNTR_t   drop_cntr;
    QM_QM_RMU0_CTRL_t       ctrl;
    QM_QM_INT_SRC_t         int_src;
    QM_QM_RMU0_RX_PKT_CNTR_t    rx_pkt_cntr;
    QM_QM_RMU0_RX_PACKET_HEADER_INFO1_t header_info1;
    QM_QM_RMU0_RX_PACKET_HEADER_INFO0_t header_info0;
    QM_QM_RMU0_NO_BUF_DROP_PKT_INFO_t   no_buf_drop_info;
    QM_QM_RMU0_NO_BUF_DROP_PKT_CNTR_t   no_buf_drop_cntr;
    QM_QM_RMU0_FE_DROP_PKT_CNTR_t       fe_drop_cntr;
    QM_QM_RMU0_FIFO_STS_t           fifo_sts;
    QM_QM_VOQ_INFO_MEMORY_DATA0_t       voq_data0;
    QM_QM_VOQ_INFO_MEMORY_DATA1_t       voq_data1;
    QM_QM_VOQ_INFO_MEMORY_DATA2_t       voq_data2;
    QM_QM_HEADER_MEMORY_DATA0_t     header_data0;
    QM_QM_HEADER_MEMORY_DATA1_t     header_data1;
    #if 0
    HEADER_A_T header_a;
    #endif

    PRINTF("=== QM_QM_RMU0_FE_DROP_PKT_CNTR ===\n");
    drop_cntr.wrd = CA_NE_REG_READ(QM_QM_RMU0_FE_DROP_PKT_CNTR);
    PRINTF("cntr=%d\n", drop_cntr.wrd);

    PRINTF("=== QM_QM_RMU0_CTRL ===\n");
    ctrl.wrd = CA_NE_REG_READ(QM_QM_RMU0_CTRL);
    PRINTF("voq_drop_en=%d, rx_en=%d\n", ctrl.bf.voq_drop_en, ctrl.bf.rx_en);

    PRINTF("=== QM_QM_INT_SRC ===\n");
    int_src.wrd = CA_NE_REG_READ(QM_QM_INT_SRC);
    PRINTF("rmu0_no_buffer_drop_int_src=%d, rmu0_check_error_int_src=%d, rmu0_fifo_error_int_src=%d, pkt_lenght_error_int_src=%d, dqm_fe_drop_int_src=%d\n",
            int_src.bf.rmu0_no_buffer_drop_int_src, int_src.bf.rmu0_check_error_int_src, int_src.bf.rmu0_fifo_error_int_src,
            int_src.bf.pkt_lenght_error_int_src, int_src.bf.dqm_fe_drop_int_src);

    PRINTF("=== QM_QM_RMU0_RX_PKT_CNTR ===\n");
    rx_pkt_cntr.wrd = CA_NE_REG_READ(QM_QM_RMU0_RX_PKT_CNTR);
    PRINTF("cntr=%d\n", rx_pkt_cntr.bf.cntr);

    PRINTF("=== QM_QM_RMU0_RX_PACKET_HEADER_INFO ===\n");
    header_info1.wrd = CA_NE_REG_READ(QM_QM_RMU0_RX_PACKET_HEADER_INFO1);
    header_info0.wrd = CA_NE_REG_READ(QM_QM_RMU0_RX_PACKET_HEADER_INFO0);
    PRINTF("header_info1.wrd=0x%x, header_info0.wrd=0x%x\n", header_info1.wrd, header_info0.wrd);

    #if 0
    header_a.bits64 = ((ca_uint64_t)header_info1.wrd << 32) | header_info0.wrd;
    PRINTF("ldpid=%d, lspid=%d, pkt_size=%d, fe_bypass=%d, hdr_type=%d\n",
        header_a.bits.ldpid, header_a.bits.lspid, header_a.bits.pkt_size, header_a.bits.fe_bypass, header_a.bits.hdr_type);
    #endif

        PRINTF("=== QM_QM_RMU0_NO_BUF_DROP_PKT_INFO ===\n");
    no_buf_drop_info.wrd = CA_NE_REG_READ(QM_QM_RMU0_NO_BUF_DROP_PKT_INFO);
    PRINTF("eqid=%d, voqid=%d\n", no_buf_drop_info.bf.eqid, no_buf_drop_info.bf.voqid);

        PRINTF("=== QM_QM_RMU0_NO_BUF_DROP_PKT_CNTR ===\n");
    no_buf_drop_cntr.wrd = CA_NE_REG_READ(QM_QM_RMU0_NO_BUF_DROP_PKT_CNTR);
    PRINTF("cntr=%d\n", no_buf_drop_cntr.bf.cntr);

        PRINTF("=== QM_QM_RMU0_FE_DROP_PKT_CNTR ===\n");
    fe_drop_cntr.wrd = CA_NE_REG_READ(QM_QM_RMU0_FE_DROP_PKT_CNTR);
    PRINTF("cntr=%d\n", fe_drop_cntr.bf.cntr);

        PRINTF("=== QM_QM_RMU0_FIFO_STS ===\n");
    fifo_sts.wrd = CA_NE_REG_READ(QM_QM_RMU0_FIFO_STS);
    PRINTF("data_fifo_depth=%d, header_fifo_depth=%d, cmd_fifo_depth=%d, cmd_resp_fifo_depth=%d, axi_fifo_depth=%d\n",
        fifo_sts.bf.data_fifo_depth, fifo_sts.bf.header_fifo_depth, fifo_sts.bf.cmd_fifo_depth, fifo_sts.bf.cmd_resp_fifo_depth, fifo_sts.bf.axi_fifo_depth);

        PRINTF("=== QM_QM_VOQ_INFO_MEMORY_DATA0 ===\n");
    for (addr = 0; addr < 64; addr++) {
        aal_l3qm_get_voq_info_mem(0, addr, &voq_data0, &voq_data1, &voq_data2);
    }

        PRINTF("=== QM_QM_HEADER_MEMORY_DATA0 ===\n");
    for (addr = 0; addr < 64; addr++) {
        aal_l3qm_get_header_mem(0, addr, &header_data0, &header_data1);
    }

    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_dump_rmu_fe_drop_counter);

ca_uint32_t aal_l3qm_dump_cpu64_fifo_read_ptr(ca_device_id_t device_id)
{
    int i;
    QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_t      rdptr;

    PRINTF("=== QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR ===\n");
    for (i = 0; i < QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_COUNT; i++) {
        rdptr.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR + i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_STRIDE);
        PRINTF("[%d] rdptr=%d\n", i, rdptr.bf.rptr);
    }
    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_dump_cpu64_fifo_read_ptr);

ca_uint32_t aal_l3qm_dump_cpu64_fifo_write_ptr(ca_device_id_t device_id)
{
    int i;
    QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR_t      wrptr;

    PRINTF("=== QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR ===\n");
    for (i = 0; i < QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR_COUNT; i++) {
        wrptr.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR + i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR_STRIDE);
        PRINTF("[%d] wrptr=%d\n", i, wrptr.bf.wptr);
    }
    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_dump_cpu64_fifo_write_ptr);

ca_uint32_t aal_l3qm_dump_cpu64_fifo_paddr_start(ca_device_id_t device_id)
{
    int i;
    QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_t      paddr_start;

    PRINTF("=== QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START ===\n");
    for (i = 0; i < QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_COUNT; i++) {
        paddr_start.wrd = CA_NE_REG_READ(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START + i * QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE);
        PRINTF("[%d] phy_addr=0x%x\n", i, paddr_start.bf.phy_addr);
    }
    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_dump_cpu64_fifo_paddr_start);

ca_uint32_t aal_l3qm_dump_monitor_eq(ca_device_id_t device_id)
{
    int i;
    QM_QM_MONITOR0_EQ0_t  monitor0_eq;
    QM_QM_MONITOR1_EQ0_t  monitor1_eq;
    QM_QM_MONITOR2_EQ0_t  monitor2_eq;
    QM_QM_MONITOR3_EQ0_t  monitor3_eq;
    QM_QM_EQ0_BUF_USG_STS_t eq_cnt;

    PRINTF("=== QM_QM_MONITOR0_EQ0/1/2/3 ===\n");
    for (i = 0; i < QM_QM_MONITOR0_EQ0_COUNT; i++) {
        
        QM_QM_CFG0_EQ0_t cfg0_eq;
        cfg0_eq.wrd = CA_NE_REG_READ(QM_QM_CFG0_EQ0 + i * QM_QM_CFG0_EQ0_STRIDE);
        
        eq_cnt.bf.cnt = CA_NE_REG_READ(QM_QM_EQ0_BUF_USG_STS + i * 4);
            
        if(cfg0_eq.bf.eq_en == 0) {
            PRINTF("eq_id[%02d] -- disabled\n", i);
            continue;
        }
        
        PRINTF("eq_id[%02d]\n", i);
        monitor0_eq.wrd = CA_NE_REG_READ(QM_QM_MONITOR0_EQ0 + i * QM_QM_MONITOR0_EQ0_STRIDE);
        monitor1_eq.wrd = CA_NE_REG_READ(QM_QM_MONITOR1_EQ0 + i * QM_QM_MONITOR1_EQ0_STRIDE);
        monitor2_eq.wrd = CA_NE_REG_READ(QM_QM_MONITOR2_EQ0 + i * QM_QM_MONITOR2_EQ0_STRIDE);
        monitor3_eq.wrd = CA_NE_REG_READ(QM_QM_MONITOR3_EQ0 + i * QM_QM_MONITOR3_EQ0_STRIDE);
        
        PRINTF(" - active cntr=%d, inactive_cntr=%d, inactive_head=%d, inactive_tail=%d\n", 
            monitor1_eq.bf.cntr, monitor2_eq.bf.inactive_cntr, monitor1_eq.bf.inactive_head, monitor2_eq.bf.inactive_tail);
        PRINTF(" - active_stack_cntr=%d, inactive_stack_cntr=%d\n", 
            monitor3_eq.bf.stack_cntr, monitor3_eq.bf.inactive_stack_cntr);
        PRINTF(" - head=%d, tail=%d\n", 
            monitor0_eq.bf.head, monitor0_eq.bf.tail);
        PRINTF(" - eq_pool_statistic_cnt = %d\n", eq_cnt.bf.cnt);
    }
        

    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_l3qm_dump_monitor_eq);

static void aal_l3qm_dump_axi_attrib(int entry)
{
    int i;
        QM_QM_AXI_ATTRIBUTE_ACCESS_t    attrib;
        QM_QM_AXI_ATTRIBUTE_DATA0_t   attrib_data0;
        QM_QM_AXI_ATTRIBUTE_DATA1_t   attrib_data1;

    attrib.wrd = 0;
    attrib.bf.access = 1;
    attrib.bf.rbw = 0;
    attrib.bf.ADDR = entry;
    CA_NE_REG_WRITE(attrib.wrd, QM_QM_AXI_ATTRIBUTE_ACCESS);
    for (i = 0; i < CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT; i++) {
        attrib.wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_ACCESS);
        if (!attrib.bf.access)
            break;
    }
    if (i == CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT) {
        PRINTF("%s: CA_L3QM_AXI_ATTRIBUTE_ACCESS_TIMEOUT at entry=%d!!!\n", __func__, entry);
        return;
    }

        attrib_data0.wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_DATA0);
        attrib_data1.wrd = CA_NE_REG_READ(QM_QM_AXI_ATTRIBUTE_DATA1);

    PRINTF("[%d] data0=0x%x, data1=0x%x\n", entry, attrib_data0.wrd, attrib_data1.wrd);
    PRINTF("     qos=%d, cache=%d, snoop=%d, bar=%d, domain=%d, user=%d, prot=%d\n", attrib_data0.bf.qos, attrib_data0.bf.cache, attrib_data0.bf.snoop,
        attrib_data0.bf.bar, attrib_data0.bf.domain, attrib_data0.bf.user, attrib_data0.bf.prot);

    PRINTF("     axi_top_bit=0x%x, ace_cmd=%d\n", attrib_data0.bf.axi_top_bit | (attrib_data1.bf.axi_top_bit << 6), attrib_data0.bf.ace_cmd);
}

ca_status_t aal_l3qm_dump_axi_attribs(ca_device_id_t device_id)
{
    int i;

    PRINTF("=== EQ 0-15 AXI ATTRIBUTE ===\n");

    for (i = CA_L3QM_AXI_ATTRIBUTE_EQ_BASE; i < CA_L3QM_AXI_ATTRIBUTE_EQ_BASE+16; i++) {
        aal_l3qm_dump_axi_attrib(i);
    }

        PRINTF("=== CPUEPP-256 port 0-31 AXI ATTRIBUTE ===\n");

        for (i = CA_L3QM_AXI_ATTRIBUTE_CPU256_BASE; i < CA_L3QM_AXI_ATTRIBUTE_CPU256_BASE+32; i++) {
                aal_l3qm_dump_axi_attrib(i);
        }

        PRINTF("=== CPUEPP port 0-7 AXI ATTRIBUTE ===\n");

        for (i = CA_L3QM_AXI_ATTRIBUTE_CPU_BASE; i < CA_L3QM_AXI_ATTRIBUTE_CPU_BASE+8; i++) {
                aal_l3qm_dump_axi_attrib(i);
        }

    return CA_E_OK;
}

//EXPORT_SYMBOL(aal_l3qm_dump_axi_attrib);

static ca_uint32_t aal_l3qm_calc_jumbo_buf_count(ca_uint32_t buf_count, ca_uint32_t buf_size)
{
        ca_uint32_t jumbo_buf_size;
        ca_uint32_t jumbo_buf_count;

        jumbo_buf_count = buf_count;

        /* when Jumbo buffer mode is enabled the total buffer number should be re-cacluated */
        if (l3qm_jumbo_buf_packing_size > 0) {

                //buf_count = l3qm_eq_profile_dq_pool1_buf_count;
                //buf_size = l3qm_eq_profile_dq_pool1_buf_sz;

                jumbo_buf_size = buf_size;
                if (l3qm_jumbo_buf_packing_size == 1) {
                        jumbo_buf_size = 2048;
                }
                else if (l3qm_jumbo_buf_packing_size == 2) {
                        jumbo_buf_size = 4096;
                }
                else if (l3qm_jumbo_buf_packing_size == 3) {
                        jumbo_buf_size = 16384;
                }
                jumbo_buf_count = (buf_count * buf_size) / jumbo_buf_size;
                
          PRINTF("%s: buf_count=%d buf_size=%d jumbo_buf_count=%d\n", __func__, buf_count, buf_size, jumbo_buf_count);
        }
        return jumbo_buf_count;
}

static void aal_l3qm_init_DQ_shared_dram(int profile_index, ca_uint32_t sram_phy_start, ca_uint32_t ddr_phy_start, ca_uint32_t mode)
{
    int i, pool_id, buf_cnt, buf_size;
    ca_uint32_t reg_off;
    QM_QM_CFG0_EQ0_t cfg0_eq;
    QM_QM_CFG1_EQ0_t cfg1_eq;
    QM_QM_CFG2_EQ0_t cfg2_eq;
    QM_QM_CFG2_EQ0_t cfg3_eq;
    QM_QM_CFG4_EQ0_t cfg4_eq;
    ca_uint32_t pool_ddr_phy_start;
    aal_l3_te_cb_free_buf_cnt_t port_buf;
    aal_l3_te_cb_free_buf_cnt_mask_t port_buf_mask;
    ca_uint32_t profileIdx = 0;
    aal_l3_te_cb_comb_threshold_mask_t threshold_msk;
    aal_l3_te_cb_comb_threshold_t threshold;
    aal_l3_te_cb_threshold_t deep_q_th;

    memset(&port_buf, 0, sizeof(aal_l3_te_cb_free_buf_cnt_t));
    memset(&threshold, 0, sizeof(aal_l3_te_cb_comb_threshold_t));

    if(profile_index == EQ_PROFILE_DQ_WAN_ID) {
        aal_l3_te_cb_free_buf_cnt_mask_t port_buf_mask;

        buf_cnt = l3qm_eq_profile_dq_pool1_buf_count/3;
        buf_size = l3qm_eq_profile_dq_pool1_buf_sz;
        
        if((DQ_bid_start+buf_cnt) > EQ_BUFFER_ID_MAX)
            buf_cnt = EQ_BUFFER_ID_MAX - DQ_bid_start;


        pool_id = profile_index;    
        
        pool_ddr_phy_start = ddr_phy_start + (buf_cnt * buf_size * 1);

        port_buf_mask.u32 = 0;
        port_buf_mask.s.cnt0 = 1;
        port_buf.cnt0 = aal_l3qm_calc_jumbo_buf_count(buf_cnt, buf_size);
        for(i = (CA_NI_TOTAL_CPU_PORT+CA_NI_WAN_PORT); i < __AAL_L3_TE_CB_PORT_NUM ; i++) {
                    aal_l3_te_cb_port_free_buf_cnt_set(0, i, port_buf_mask, &port_buf);
        }
#if defined(CONFIG_RG_G3_WAN_PORT_INDEX) && (CONFIG_RG_G3_WAN_PORT_INDEX!=7)
                aal_l3_te_cb_port_free_buf_cnt_set(0, CA_NI_TOTAL_CPU_PORT+CONFIG_RG_G3_WAN_PORT_INDEX, port_buf_mask, &port_buf);
#endif
        
    } else {
        buf_cnt = l3qm_eq_profile_dq_pool1_buf_count/3;
        buf_size = l3qm_eq_profile_dq_pool1_buf_sz;
        pool_id = profile_index;

        pool_ddr_phy_start = ddr_phy_start + (buf_cnt * buf_size * 0);  
        
        port_buf_mask.u32 = 0;
        port_buf_mask.s.cnt0 = 1;
        port_buf.cnt0 = aal_l3qm_calc_jumbo_buf_count(buf_cnt, buf_size);
        for(i = (CA_NI_TOTAL_CPU_PORT); i < (CA_NI_TOTAL_CPU_PORT+CA_NI_WAN_PORT) ; i++) {
                    aal_l3_te_cb_port_free_buf_cnt_set(0, i, port_buf_mask, &port_buf);
        }
    }


    PRINTF("%s: sram_phy_start=0x%x, ddr_phy_start=0x%x\n", __func__, sram_phy_start, ddr_phy_start);
    PRINTF("%s: pool_id=%d, bid_start=%d\n", __func__, pool_id, DQ_bid_start);

    /* configure EB pool 0 for DQ */
    /* set phy_addr_start and enable Empty Buffer */
    cfg0_eq.wrd = 0;
    cfg0_eq.bf.eq_en = 1;
    cfg0_eq.bf.phy_addr_start = pool_ddr_phy_start >> CA_L3QM_PHY_ADDR_SHIFT;
    reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * pool_id);
    CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

    PRINTF("%s: cfg0_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg0_eq.wrd, reg_off);

    /* configure total buffer number and bid_start */
    cfg1_eq.wrd = 0;
    cfg1_eq.bf.total_buffer_num = aal_l3qm_calc_jumbo_buf_count(buf_cnt, buf_size);
    cfg1_eq.bf.bid_start = DQ_bid_start;
    reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * pool_id);
    CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

    PRINTF("%s: cfg1_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg1_eq.wrd, reg_off);

    /* configure buffer size for Empty Buffer */
    reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * pool_id);
    cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
    cfg2_eq.bf.cpu_eq = 0;
    cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(buf_size);      /* 512 bytes */
    cfg2_eq.bf.refill_en = 0;
    CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);
    
    PRINTF("%s: cfg2_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg2_eq.wrd, reg_off);

    /* configure cache_eos/domain/snoop/cache */
    reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * pool_id);
    cfg3_eq.wrd = 0x10;
    CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

    cfg3_eq.wrd = CA_NE_REG_READ(reg_off);
    PRINTF("%s: cfg3_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg3_eq.wrd, reg_off);

    /* configure AXI address top 8 bits */
    reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * pool_id);
    cfg4_eq.wrd = 0;
    CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

    cfg4_eq.wrd = CA_NE_REG_READ(reg_off);
    PRINTF("%s: cfg4_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg4_eq.wrd, reg_off);

    DQ_bid_start += buf_cnt;
    
    /* configure EB pool 1 for DQ */
    if(profile_index == EQ_PROFILE_DQ_LAN_ID)
    {
    
        pool_ddr_phy_start = ddr_phy_start + (buf_cnt * buf_size * 2);

        buf_cnt = l3qm_eq_profile_dq_pool1_buf_count/3;
        buf_size = l3qm_eq_profile_dq_pool1_buf_sz;

        pool_id = l3qm_eq_share_pool_id;
        /* set phy_addr_start and enable Empty Buffer */
        cfg0_eq.wrd = 0;
        cfg0_eq.bf.eq_en = 1;
        cfg0_eq.bf.phy_addr_start = pool_ddr_phy_start >> CA_L3QM_PHY_ADDR_SHIFT;
        reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * pool_id);
        CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

        PRINTF("%s: cfg0_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg0_eq.wrd, reg_off);

        /* configure total buffer number and bid_start */
        cfg1_eq.wrd = 0;
        cfg1_eq.bf.total_buffer_num = aal_l3qm_calc_jumbo_buf_count(buf_cnt, buf_size);
        cfg1_eq.bf.bid_start = DQ_bid_start;
        reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * pool_id);
        CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

        PRINTF("%s: cfg1_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg1_eq.wrd, reg_off);

        /* configure buffer size for Empty Buffer */
        reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * pool_id);
        cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
        cfg2_eq.bf.cpu_eq = 0;
            cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(buf_size);      /* 1024 bytes */
        cfg2_eq.bf.refill_en = 0;
        CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

        PRINTF("%s: cfg2_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg2_eq.wrd, reg_off);

        /* configure cache_eos/domain/snoop/cache */
        reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * pool_id);
        cfg3_eq.wrd = 0x10;
        CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

        cfg3_eq.wrd = CA_NE_REG_READ(reg_off);
        PRINTF("%s: cfg3_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg3_eq.wrd, reg_off);

        /* configure AXI address top 8 bits */
        reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * pool_id);
        cfg4_eq.wrd = 0;
        CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

        cfg4_eq.wrd = CA_NE_REG_READ(reg_off);
        PRINTF("%s: cfg4_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg4_eq.wrd, reg_off);

        DQ_bid_start += buf_cnt;    

        port_buf_mask.u32 = 0;
        port_buf_mask.s.cnt1 = 1;
        port_buf.cnt1 = aal_l3qm_calc_jumbo_buf_count(buf_cnt, buf_size);
        for(i = (CA_NI_TOTAL_CPU_PORT); i < __AAL_L3_TE_CB_PORT_NUM ; i++) {
                aal_l3_te_cb_port_free_buf_cnt_set(0, i, port_buf_mask, &port_buf);
        }

        // adjust voq threshold
        threshold_msk.u32 = 0;
        threshold_msk.s.threshold_hi = 1;
        threshold_msk.s.threshold_lo = 1;
        threshold.threshold_hi = (port_buf.cnt1*2) / 2;
        threshold.threshold_lo = (port_buf.cnt1*2) / 3; 

        // lan
        aal_l3_te_cb_voq_profile_sel_get(0, (AAL_L3QM_NI_PORT_OFFSET * 8), &profileIdx);
        aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);
        
        // wan
        aal_l3_te_cb_voq_profile_sel_get(0, (AAL_L3QM_PUC_PORT_OFFSET * 8) - 8, &profileIdx);
        aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);

        // puc low queue
        aal_l3_te_cb_voq_profile_sel_get(0, (AAL_L3QM_PUC_PORT_OFFSET * 8), &profileIdx);
        aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);

        // puc mid queue
        aal_l3_te_cb_voq_profile_sel_get(0, (AAL_L3QM_PUC_PORT_OFFSET * 8)+4, &profileIdx);
        aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);

        // puc high queue
        aal_l3_te_cb_voq_profile_sel_get(0, (AAL_L3QM_PUC_PORT_OFFSET * 8)+7, &profileIdx);
        aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);

        // adjust port threshold
        threshold_msk.s.threshold_hi = 1;
        threshold_msk.s.threshold_lo = 1;
        threshold.threshold_hi = ((port_buf.cnt1*2) / 3) * 2;
        threshold.threshold_lo = ((port_buf.cnt1*2) / 3);   

        // lan
        aal_l3_te_cb_port_profile_sel_get(0, AAL_L3QM_NI_PORT_OFFSET, &profileIdx);
        aal_l3_te_cb_port_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);

        // wan
        aal_l3_te_cb_port_profile_sel_get(0, AAL_L3QM_PUC_PORT_OFFSET - 1, &profileIdx);
        aal_l3_te_cb_port_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);
        
        // puc
        aal_l3_te_cb_port_profile_sel_get(0, AAL_L3QM_PUC_PORT_OFFSET, &profileIdx);
        aal_l3_te_cb_port_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);

        // adjust voq and port backpressure threshoold
        profileIdx = 0;
        aal_l3_te_cb_dqsch_voq_thrsh_profile_sel_get(0, (AAL_L3QM_NI_PORT_OFFSET * 8), &profileIdx);
        aal_l3_te_cb_dqsch_voq_thrsh_profile_get(0, profileIdx, &deep_q_th);
        deep_q_th.threshold_lo = aal_l3qm_calc_jumbo_buf_count(deep_q_th.threshold_lo, buf_size);;
        aal_l3_te_cb_dqsch_voq_thrsh_profile_set(0, profileIdx,&deep_q_th);

        profileIdx = 0;
        aal_l3_te_cb_dqsch_port_thrsh_profile_sel_get(0, AAL_L3QM_NI_PORT_OFFSET, &profileIdx);
        aal_l3_te_cb_dqsch_port_thrsh_profile_get(0, profileIdx, &deep_q_th);
        deep_q_th.threshold_hi = aal_l3qm_calc_jumbo_buf_count(deep_q_th.threshold_hi, buf_size);;
        aal_l3_te_cb_dqsch_port_thrsh_profile_set(0, profileIdx,&deep_q_th);
        
    }
}


static void aal_l3qm_init_DQ_pools_pool0(int profile_index, ca_uint32_t sram_phy_start, uint32_t sram_size)
{
    int i, pool_id, buf_cnt, buf_size;
    ca_uint32_t reg_off;
    QM_QM_CFG0_EQ0_t cfg0_eq;
    QM_QM_CFG1_EQ0_t cfg1_eq;
    QM_QM_CFG2_EQ0_t cfg2_eq;
    QM_QM_CFG2_EQ0_t cfg3_eq;
    QM_QM_CFG4_EQ0_t cfg4_eq;
    aal_l3_te_cb_free_buf_cnt_t port_buf;
    aal_l3_te_cb_free_buf_cnt_mask_t port_buf_mask;

    memset(&port_buf, 0, sizeof(aal_l3_te_cb_free_buf_cnt_t));

    if(profile_index == EQ_PROFILE_DQ_WAN_ID) {         
        buf_size = l3qm_eq_sram_wan_buffer_size;
        buf_cnt =  sram_size / buf_size;
        pool_id = profile_index;    

        port_buf_mask.u32 = 0;
        port_buf_mask.s.cnt0 = 1;
        port_buf.cnt0 = aal_l3qm_calc_jumbo_buf_count(buf_cnt, buf_size);

        for(i = CA_NI_TOTAL_CPU_PORT + CA_NI_WAN_PORT; i < __AAL_L3_TE_CB_PORT_NUM ; i++) {
        
                    aal_l3_te_cb_port_free_buf_cnt_set(0, i, port_buf_mask, &port_buf);
        }

        l3qm_eq_poo0_wan_port_free_buf = port_buf.cnt0;
    } else {
        buf_size = EQ_POOL_SRAM_BUFFER_SIZE;
        buf_cnt = sram_size / buf_size;
        pool_id = profile_index;

        port_buf_mask.u32 = 0;
        port_buf_mask.s.cnt0 = 1;
        port_buf.cnt0 = aal_l3qm_calc_jumbo_buf_count(buf_cnt, buf_size);

        for(i = CA_NI_TOTAL_CPU_PORT; i < (CA_NI_TOTAL_CPU_PORT+CA_NI_WAN_PORT) ; i++) {

                    aal_l3_te_cb_port_free_buf_cnt_set(0, i, port_buf_mask, &port_buf);
        }

        l3qm_eq_poo0_lan_port_free_buf = port_buf.cnt0;
    }


    PRINTF( "%s: sram_phy_start=0x%x\n", __func__, sram_phy_start);
    PRINTF( "%s: pool_id=%d, bid_start=%d\n", __func__, pool_id, DQ_bid_start);

    /* configure EB pool 0 for DQ */
    /* set phy_addr_start and enable Empty Buffer */
    cfg0_eq.wrd = 0;
    cfg0_eq.bf.eq_en = 1;
    cfg0_eq.bf.phy_addr_start = sram_phy_start >> CA_L3QM_PHY_ADDR_SHIFT;
    reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * pool_id);
    CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

    PRINTF( "%s: cfg0_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg0_eq.wrd, reg_off);

    /* configure total buffer number and bid_start */
    cfg1_eq.wrd = 0;
    cfg1_eq.bf.total_buffer_num = aal_l3qm_calc_jumbo_buf_count(buf_cnt, buf_size);
    cfg1_eq.bf.bid_start = DQ_bid_start;
    reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * pool_id);
    CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

    PRINTF( "%s: cfg1_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg1_eq.wrd, reg_off);

    /* configure buffer size for Empty Buffer */
    reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * pool_id);
    cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
    cfg2_eq.bf.cpu_eq = 0;
    cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(buf_size);      /* 512 bytes */
    cfg2_eq.bf.refill_en = 0;
    CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);
    
    PRINTF( "%s: cfg2_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg2_eq.wrd, reg_off);

    /* configure cache_eos/domain/snoop/cache */
    reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * pool_id);
    cfg3_eq.wrd = 0x10;
    CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

    cfg3_eq.wrd = CA_NE_REG_READ(reg_off);
    PRINTF( "%s: cfg3_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg3_eq.wrd, reg_off);

    /* configure AXI address top 8 bits */
    reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * pool_id);
    cfg4_eq.wrd = 0;
    CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

    cfg4_eq.wrd = CA_NE_REG_READ(reg_off);
    PRINTF( "%s: cfg4_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg4_eq.wrd, reg_off);


    DQ_bid_start += buf_cnt;
    
}

static void aal_l3qm_init_DQ_pools_pool1(ca_uint32_t ddr_phy_start)
{
    int i, pool_id, buf_cnt, buf_size;
    ca_uint32_t reg_off;
    QM_QM_CFG0_EQ0_t cfg0_eq;
    QM_QM_CFG1_EQ0_t cfg1_eq;
    QM_QM_CFG2_EQ0_t cfg2_eq;
    QM_QM_CFG2_EQ0_t cfg3_eq;
    QM_QM_CFG4_EQ0_t cfg4_eq;
    aal_l3_te_cb_free_buf_cnt_t port_buf;
    aal_l3_te_cb_free_buf_cnt_mask_t port_buf_mask;
    ca_uint32_t profileIdx = 0;
    aal_l3_te_cb_comb_threshold_mask_t threshold_msk;
    aal_l3_te_cb_comb_threshold_t threshold;
    aal_l3_te_cb_threshold_t deep_q_th;
    aal_l3_te_cb_eq_threshold_t eq_profile_th;

    memset(&port_buf, 0, sizeof(aal_l3_te_cb_free_buf_cnt_t));
    memset(&threshold, 0, sizeof(aal_l3_te_cb_comb_threshold_t));
    memset(&eq_profile_th, 0, sizeof(aal_l3_te_cb_eq_threshold_t));

    /* pool1 is shared by all NI ports */

    pool_id = l3qm_eq_share_pool_id;
    buf_cnt = l3qm_eq_profile_dq_pool1_buf_count;
    buf_size = l3qm_eq_profile_dq_pool1_buf_sz;

    if((DQ_bid_start+buf_cnt) > EQ_BUFFER_ID_MAX)
        buf_cnt = EQ_BUFFER_ID_MAX - DQ_bid_start;
    
    PRINTF( "%s: dram_phy_start=0x%x\n", __func__, ddr_phy_start);
    PRINTF( "%s: pool_id=%d, bid_start=%d\n", __func__, pool_id, DQ_bid_start);
    
    /* set phy_addr_start and enable Empty Buffer */
    cfg0_eq.wrd = 0;
    cfg0_eq.bf.eq_en = 1;
    cfg0_eq.bf.phy_addr_start = ddr_phy_start >> CA_L3QM_PHY_ADDR_SHIFT;
    reg_off = QM_QM_CFG0_EQ0 + (QM_QM_CFG0_EQ0_STRIDE * pool_id);
    CA_NE_REG_WRITE(cfg0_eq.wrd, reg_off);

    PRINTF( "%s: cfg0_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg0_eq.wrd, reg_off);

    /* configure total buffer number and bid_start */
    cfg1_eq.wrd = 0;
    cfg1_eq.bf.total_buffer_num = aal_l3qm_calc_jumbo_buf_count(buf_cnt, buf_size);
    cfg1_eq.bf.bid_start = DQ_bid_start;
    reg_off = QM_QM_CFG1_EQ0 + (QM_QM_CFG1_EQ0_STRIDE * pool_id);
    CA_NE_REG_WRITE(cfg1_eq.wrd, reg_off);

    PRINTF( "%s: cfg1_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg1_eq.wrd, reg_off);

    /* configure buffer size for Empty Buffer */
    reg_off = QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * pool_id);
    cfg2_eq.wrd = CA_NE_REG_READ(reg_off);
    cfg2_eq.bf.cpu_eq = 0;
    cfg2_eq.bf.buffer_size = aal_l3qm_get_buffer_size_index(buf_size);      /* 1024 bytes */
    cfg2_eq.bf.refill_en = 0;
    CA_NE_REG_WRITE(cfg2_eq.wrd, reg_off);

    PRINTF( "%s: cfg2_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg2_eq.wrd, reg_off);

    /* configure cache_eos/domain/snoop/cache */
    reg_off = QM_QM_CFG3_EQ0 + (QM_QM_CFG3_EQ0_STRIDE * pool_id);
    cfg3_eq.wrd = 0x10;
    CA_NE_REG_WRITE(cfg3_eq.wrd, reg_off);

    cfg3_eq.wrd = CA_NE_REG_READ(reg_off);
    PRINTF( "%s: cfg3_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg3_eq.wrd, reg_off);

    /* configure AXI address top 8 bits */
    reg_off = QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * pool_id);
    cfg4_eq.wrd = 0;
    CA_NE_REG_WRITE(cfg4_eq.wrd, reg_off);

    cfg4_eq.wrd = CA_NE_REG_READ(reg_off);
    PRINTF( "%s: cfg4_eq.wrd=0x%x, reg_off=0x%x\n", __func__, cfg4_eq.wrd, reg_off);

    DQ_bid_start += buf_cnt;    

    port_buf_mask.u32 = 0;
    port_buf_mask.s.cnt1 = 1;
    port_buf.cnt1 = aal_l3qm_calc_jumbo_buf_count(buf_cnt, buf_size);
    for(i = (CA_NI_TOTAL_CPU_PORT); i < __AAL_L3_TE_CB_PORT_NUM ; i++) {
            aal_l3_te_cb_port_free_buf_cnt_set(0, i, port_buf_mask, &port_buf);
    }


    // adjust voq threshold
    threshold_msk.u32 = 0;
    threshold_msk.s.threshold_hi = 1;
    threshold_msk.s.threshold_lo = 1;
    threshold.threshold_hi = port_buf.cnt1 / 2;
    threshold.threshold_lo = threshold.threshold_hi - 100;  
    if(l3qm_qos_auto_threhsold_voq_num) {
        threshold_msk.s.prvt = 1;
        threshold.prvt = l3qm_qos_auto_threhsold_voq_prvt_buf_cnt;
    }

    // lan
    aal_l3_te_cb_voq_profile_sel_get(0, (AAL_L3QM_NI_PORT_OFFSET * 8), &profileIdx);
    aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);
    
    // wan
#if defined(CONFIG_RTK_DEV_AP)
    threshold.threshold_hi = port_buf.cnt1;
    threshold.threshold_lo = (port_buf.cnt1 / 10) * 9;  
#endif
    aal_l3_te_cb_voq_profile_sel_get(0, (AAL_L3QM_PUC_PORT_OFFSET * 8) - 8, &profileIdx);
    aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);

    // puc low queue
    aal_l3_te_cb_voq_profile_sel_get(0, (AAL_L3QM_PUC_PORT_OFFSET * 8), &profileIdx);
    aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);

    // puc mid queue
    aal_l3_te_cb_voq_profile_sel_get(0, (AAL_L3QM_PUC_PORT_OFFSET * 8)+4, &profileIdx);
    aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);

    // puc high queue
    aal_l3_te_cb_voq_profile_sel_get(0, (AAL_L3QM_PUC_PORT_OFFSET * 8)+7, &profileIdx);
    aal_l3_te_cb_voq_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);


    // adjust profile & port threshold
    if(l3qm_qos_auto_threhsold_voq_num) {
        PRINTF("%s: qm qos auto threshold ON, voq prvt = %d voq num = %d\n", __func__, l3qm_qos_auto_threhsold_voq_prvt_buf_cnt, l3qm_qos_auto_threhsold_voq_num);
        if(aal_l3_te_cb_pool_profile_get(0, EQ_PROFILE_DQ_WAN_ID << 1, &eq_profile_th) == CA_E_OK) {
            eq_profile_th.threshold =  (l3qm_qos_auto_threhsold_voq_num+1) * l3qm_qos_auto_threhsold_voq_prvt_buf_cnt;
            aal_l3_te_cb_pool_profile_set(0, EQ_PROFILE_DQ_WAN_ID << 1, &eq_profile_th);
        }
        if(aal_l3_te_cb_pool_profile_get(0, EQ_PROFILE_DQ_LAN_ID << 1, &eq_profile_th) == CA_E_OK) {
            eq_profile_th.threshold =  (l3qm_qos_auto_threhsold_voq_num+1) * l3qm_qos_auto_threhsold_voq_prvt_buf_cnt;
            aal_l3_te_cb_pool_profile_set(0, EQ_PROFILE_DQ_LAN_ID << 1, &eq_profile_th);
        }
    }
    
    if(aal_l3_te_cb_pool_profile_get(0, EQ_PROFILE_DQ_WAN_ID << 1, &eq_profile_th) == CA_E_OK)
        threshold.threshold_hi = eq_profile_th.threshold + 200;
    else
        threshold.threshold_hi = (port_buf.cnt1 / 3) * 2;

    if(threshold.threshold_hi > buf_cnt) {
        for(i = 0; i < 1000; i++)
            PRINTF("\033[1;33m%s: FIXME!!!!!!! port threshold %d but higher than DDR buffer cnt %d \033[0m\n", __func__, threshold.threshold_hi, buf_cnt);
            ////BUG_ON((threshold.threshold_hi > buf_cnt));
    }   
    
    threshold.threshold_lo = threshold.threshold_hi - 100;


    threshold_msk.s.threshold_hi = 1;
    threshold_msk.s.threshold_lo = 1;
    
    // lan
    aal_l3_te_cb_port_profile_sel_get(0, CA_NI_TOTAL_CPU_PORT + 0, &profileIdx);
    aal_l3_te_cb_port_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);

    // wan
#if defined(CONFIG_RTK_DEV_AP)
    threshold.threshold_hi = 8;
    threshold.threshold_lo = 4;     
#endif
    aal_l3_te_cb_port_profile_sel_get(0, CA_NI_TOTAL_CPU_PORT + 7, &profileIdx);
    aal_l3_te_cb_port_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);
    
    // puc
    aal_l3_te_cb_port_profile_sel_get(0, CA_NI_TOTAL_CPU_PORT + 8, &profileIdx);
    aal_l3_te_cb_port_threshold_profile_set(0, profileIdx, threshold_msk, &threshold);

    // adjust voq and port backpressure threshoold
    profileIdx = 0;
    aal_l3_te_cb_dqsch_voq_thrsh_profile_sel_get(0, 64, &profileIdx);
    aal_l3_te_cb_dqsch_voq_thrsh_profile_get(0, profileIdx, &deep_q_th);
    deep_q_th.threshold_lo = aal_l3qm_calc_jumbo_buf_count(deep_q_th.threshold_lo, buf_size);;
    aal_l3_te_cb_dqsch_voq_thrsh_profile_set(0, profileIdx,&deep_q_th);

    profileIdx = 0;
    aal_l3_te_cb_dqsch_port_thrsh_profile_sel_get(0, 8, &profileIdx);
    aal_l3_te_cb_dqsch_port_thrsh_profile_get(0, profileIdx, &deep_q_th);
    deep_q_th.threshold_hi = aal_l3qm_calc_jumbo_buf_count(deep_q_th.threshold_hi, buf_size);;
    aal_l3_te_cb_dqsch_port_thrsh_profile_set(0, profileIdx,&deep_q_th);
}

static void aal_l3qm_init_empty_buffer_DQ_1(void)
{
    int i;
    ca_uint32_t reg_off;
    uint32_t eq_pool_sram_size = 0, rsvdSize = 0;
    QM_QM_EQ_PROFILE0_t eq_profile;
    QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
    // shared sram for DQ usage starts after hash function.
    ca_uint32_t l3fe_dq_eq0_phy_addr = (l3fe_main_hash_action_cache_fib_phy_addr);

    if(l3fe_main_hash_flow_cache_fib_internal_sram_mode==0) {
        l3fe_dq_eq0_phy_addr += l3fe_main_hash_action_cache_fib_length * l3fe_main_hash_action_cache_fib_count;
    }

    /* l3qm_eq_share_pool_id should be read from scfg */
    l3qm_eq_share_pool_id = EQ_PROFILE_DQ_SHARE_ID;

    /* profile 0 for DQ NI lan ports */
    eq_profile.wrd = 0;
    eq_profile.bf.eqp0 = EQ_PROFILE_DQ_LAN_ID;
    eq_profile.bf.eqp1 = l3qm_eq_share_pool_id;
    eq_profile.bf.rule = 0;
    reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * EQ_PROFILE_DQ_LAN_ID);
    CA_NE_REG_WRITE(eq_profile.wrd, reg_off);
    
    /* profile 1 for NI port 7 */
    eq_profile.wrd = 0;
    eq_profile.bf.eqp0 = EQ_PROFILE_DQ_WAN_ID;
    eq_profile.bf.eqp1 = l3qm_eq_share_pool_id;
    eq_profile.bf.rule = 0;
    reg_off = QM_QM_EQ_PROFILE0 + (QM_QM_EQ_PROFILE0_STRIDE * EQ_PROFILE_DQ_WAN_ID);
    CA_NE_REG_WRITE(eq_profile.wrd, reg_off);

    /* NI ports select DQ profile 0 or 1 */
    dest_port_eq_cfg.wrd = 0;
    for (i = 0; i < 8; i++) {
        if(i <= 6)  /* as LAN */
            dest_port_eq_cfg.bf.profile_sel = EQ_PROFILE_DQ_LAN_ID;
        else        /* as WAN */
            dest_port_eq_cfg.bf.profile_sel = EQ_PROFILE_DQ_WAN_ID;
        reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * (i + CA_NI_TOTAL_CPU_PORT));
        CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);
    }

    for (i = AAL_L3QM_PUC_PORT_OFFSET; i < AAL_L3QM_PORT_COUNT; i++) {
        /* PON WAN*/
        dest_port_eq_cfg.bf.profile_sel = EQ_PROFILE_DQ_WAN_ID;
        reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * i);       
        CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off); 
    }

    {
        TE_CB_WIFI_EQ_PROFILE0_THRSH_t wifi_eq_th;
        
        wifi_eq_th.bf.hth = 128;
        wifi_eq_th.bf.lth = 64;
        CA_NE_REG_WRITE(wifi_eq_th.wrd, TE_CB_WIFI_EQ_PROFILE0_THRSH + (EQ_PROFILE_DQ_LAN_ID * TE_CB_WIFI_EQ_PROFILE0_THRSH_STRIDE));
        CA_NE_REG_WRITE(wifi_eq_th.wrd, TE_CB_WIFI_EQ_PROFILE0_THRSH + (EQ_PROFILE_DQ_WAN_ID * TE_CB_WIFI_EQ_PROFILE0_THRSH_STRIDE));
    }

    PRINTF( "capsram size is %d  currect used size is %d.\n", sram_dts_conf_size, sram_cur_used_size);


    if((sram_dts_conf_size < sram_cur_used_size)) {
        eq_pool_sram_size = EQ_POOL_SRAM_SIZE;
        PRINTF("ERROR: dtb sram size is %d but currect used size %d over that\n", sram_dts_conf_size, sram_cur_used_size);
    }else {
        ca_uint32_t portidx = 0, qidx =0;

        for(portidx=0; portidx<CFG_ID_RTK_DMALSO_SRAM_MODE_ENABLE_LEN; portidx++) {
            for(qidx=0; qidx<8; qidx++) {
                if( l3qm_dmalso_sram_enable[portidx] & (1 << qidx))
                    rsvdSize += (CA_NI_DMA_LSO_TXDESC_NUM * 8 /*sizeof(dma_txdesc_t)*/);
            }
        }
        for(portidx=0; portidx<CFG_ID_RTK_CPUEPP_SRAM_MODE_ENABLE_LEN; portidx++) {
            for(qidx=0; qidx<8; qidx++) {
                if(l3qm_cpuepp_sram_enable[portidx] & (1 << qidx))
                    rsvdSize += (l3qm_cpu_epp_per_voq * l3qm_desc_size * l3qm_desc_per_epp + l3qm_magic_marker_len);
            }
        }   
        if((sram_dts_conf_size < (sram_cur_used_size+rsvdSize))) {
            //eq_pool_sram_size = EQ_POOL_SRAM_SIZE;
            PRINTF("ERROR: dtb sram size is %d but currect used size %d over that, please check SCFG!!\n", sram_dts_conf_size, (sram_cur_used_size+rsvdSize));
        }
            
    
        PRINTF( "capsram reseve some mem for epp and dmalso, currect used size is %d srvd size is %d.\n", sram_cur_used_size, rsvdSize);

        // eq_pool_sram_size: remaining  sram are used by LAN ports
        eq_pool_sram_size = ( sram_dts_conf_size - sram_cur_used_size -rsvdSize) - l3qm_eq_sram_wan_total_size;

    }

    if(l3qm_eq0_sram_eq1_dram == 0) {
        
        /* test mode */
        
        /* Init DQ private DRAM pool - LAN ports */
        aal_l3qm_init_DQ_shared_dram(EQ_PROFILE_DQ_LAN_ID, l3fe_dq_eq0_phy_addr, l3qm_eq_profile_dq_pool1_buff_phy_addr, L3QM_EQ_SHARE_MODE);

        /* Init DQ private DRAM pool - WAN ports */
        aal_l3qm_init_DQ_shared_dram(EQ_PROFILE_DQ_WAN_ID, l3fe_dq_eq0_phy_addr, l3qm_eq_profile_dq_pool1_buff_phy_addr, L3QM_EQ_SHARE_MODE);
    }else {
    
        /* Init DQ private SRAM pool - LAN ports */
        aal_l3qm_init_DQ_pools_pool0(EQ_PROFILE_DQ_LAN_ID, l3fe_dq_eq0_phy_addr, eq_pool_sram_size);
        sram_cur_used_size += eq_pool_sram_size;
        sram_cur_virtual_addr += eq_pool_sram_size;

        /* Init DQ private SRAM pool - WAN ports */
        aal_l3qm_init_DQ_pools_pool0(EQ_PROFILE_DQ_WAN_ID, l3fe_dq_eq0_phy_addr + eq_pool_sram_size, l3qm_eq_sram_wan_total_size);
        sram_cur_used_size += l3qm_eq_sram_wan_total_size;
        sram_cur_virtual_addr += l3qm_eq_sram_wan_total_size;
        
        /* Init DQ shared DRAM pool - ALL ports */
        aal_l3qm_init_DQ_pools_pool1(l3qm_eq_profile_dq_pool1_buff_phy_addr);
    }
}

void aal_l3qm_set_empty_buffer_ni_port_select_profile(ca_uint64_t wan_portmask)
{
    int i, loop_cnt=0, loop_voq=0, find=0;
    ca_uint8_t profileIdx, voq, wanport;
    ca_uint32_t reg_off;
    QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
    aal_l3_te_cb_free_buf_cnt_t cfg;
    aal_l3_te_cb_free_buf_cnt_mask_t mask;

    mask.u32 = 0;
    mask.s.cnt0 = 1;

    /* disable RX of L3QM */
    aal_l3qm_enable_rx(0);

    /* make sure all NI pkts are transmitted out */
    loop_voq = AAL_L3QM_NI_PORT_OFFSET << 3;
    do {
        find = 0;
        for(i = loop_voq; i < ((AAL_L3QM_NI_PORT_OFFSET + AAL_L3QM_NI_PORT_COUNT) << 3); i++) {
            aal_l3_te_cb_voq_buf_cnt_get(0, i, &cfg);
            if((cfg.cnt0 + cfg.cnt1) > 0) {
                loop_voq = i;
                find = 1;
                break;
            }
        }
        loop_cnt++;

        if(find == 1)
            continue;
        else
            break;
    }while(loop_cnt < CA_L3QM_INIT_DONE_LOOP);
    if(loop_cnt == CA_L3QM_INIT_DONE_LOOP)
        PRINTF("NOTICE: change QM eq selection but there are pkts still not transmitting\n");

    /* NI ports select DQ profile 0 or 1 */
    dest_port_eq_cfg.wrd = 0;

    for (i = AAL_L3QM_NI_PORT_OFFSET; i < (AAL_L3QM_NI_PORT_OFFSET+AAL_L3QM_NI_PORT_COUNT); i++) {
        wanport = ((wan_portmask >> (i-AAL_L3QM_NI_PORT_OFFSET)) & 0x1);
        dest_port_eq_cfg.bf.profile_sel = wanport ? EQ_PROFILE_DQ_WAN_ID : EQ_PROFILE_DQ_LAN_ID;
        reg_off = QM_QM_DEST_PORT0_EQ_CFG + (QM_QM_DEST_PORT0_EQ_CFG_STRIDE * i);
        CA_NE_REG_WRITE(dest_port_eq_cfg.wrd, reg_off);

        if(i == (AAL_PPORT_NI7 + AAL_L3QM_NI_PORT_OFFSET))
        {
#if defined(CONFIG_NE_PON)
            if(CA_PON_MODE_IS_EPON_FAMILY(aal_pon_mac_mode_get(0)) || CA_PON_MODE_IS_GPON_FAMILY(aal_pon_mac_mode_get(0)))
                continue;
#endif

            if (aal_l3_te_cb_port_free_buf_cnt_get(0, i , &cfg) == CA_E_OK) {
                cfg.cnt0 = ((1ULL << i) & wan_portmask) ? l3qm_eq_poo0_wan_port_free_buf : l3qm_eq_poo0_lan_port_free_buf;
                aal_l3_te_cb_port_free_buf_cnt_set(0, i, mask, &cfg);
                // adjust voq threshold selection
                profileIdx = wanport ? l3te_voq_profile_sel[AAL_L3QM_PUC_PORT_OFFSET][0] : l3te_voq_profile_sel[AAL_L3QM_NI_PORT_OFFSET][0];
                for(voq = (i<<3) ; voq < (i<<3) + 8 ; voq++) {
                    aal_l3_te_cb_voq_profile_sel_set(0, voq, profileIdx);
                }
                // adjust port threshold selection
                profileIdx = wanport ? l3te_port_profile_sel[AAL_L3QM_PUC_PORT_OFFSET] : l3te_port_profile_sel[AAL_L3QM_NI_PORT_OFFSET];
                aal_l3_te_cb_port_profile_sel_set(0, i, profileIdx);
            }
            else {
                PRINTF("NI port [%d] aal_l3_te_cb_port_free_buf_cnt_get err!\n", i);
            }
        }
    }

    /* enable RX of L3QM */
    aal_l3qm_enable_rx(1);
}
//EXPORT_SYMBOL(aal_l3qm_set_empty_buffer_ni_port_select_profile);


