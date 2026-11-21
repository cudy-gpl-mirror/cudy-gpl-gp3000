/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 */
#ifndef __NIC_H__
#define __NIC_H__
#include <osal_soc.h>
#include <osal.h>
///--
#include <osal_type.h>
#include <aal_common.h>

/* definition */
#define COS_NUM 8

/* pkt buf struct definetion */
typedef struct {
    uint8 *buf_addr;
    uint32 buf_len;
    uint32 sport;
    uint32 dport;
} rtk_ni_buf_t;

/* nic global menagement variable definetion */
typedef struct {
    uint32              allocated_buff;
    uint32              no_tx_descptor;
    uint32              rx_rd_idx[COS_NUM];
    uint32              tx_wrt_idx;
    uint32              sw_tx_done_idx;
    uint32              ni_epp_base;
    uint32              rx_voq_bit;
    uint32              rev[2]; //cache line aligment
} rtk_ni_intf_t;

//qm epp struct
typedef union {
    uint64 bits64;
    struct {
        uint32 bits32_h     ;
        uint32 bits32_l     ;
    } __attribute__((packed)) bits32;
    struct bit_cpu_epp_fifo_cmd_64_bit {
        uint32 eqid                         : 4; 
        uint32 sop                          : 1; 
        uint32 l4_csum_err                  : 1; 
        uint32 eop                          : 1; 
        uint32 phy_addr                     : 25;
        uint32 packet_size                  : 14;
        uint32 sw_id                            : 6; 
        uint32 rsrvd                            : 12; 
    } bits;
} cpu_epp_fifo_cmd_64bit_t;

//dma lso struct
typedef union {
        uint32 wrd;
        struct bit_038 {
                uint32 buf_addr;                   /* bits 31:0 */
        } bf;
} dma_txdesc_0_t;

typedef union {
        uint32 wrd;
        struct bit_03c {
                uint32 buf_addr_39_32      : 8;    /* bits 7:0 */
                uint32 buf_len             : 16;   /* bits 23:8 */
                uint32 hp0                 : 1;    /* bits 24 */
                uint32 fbm                 : 1;    /* bits 25 */
                uint32 fbm_pool_id         : 3;    /* bits 28:26 */
                uint32 sop_eop             : 2;    /* bits 30:29 */
                uint32 hp1                 : 1;    /* bits 31 */
        } bf;
        struct {
                uint32 fe_bypass       : 1;    /* bits 0 */
                uint32 dev_id              : 7;    /* bits 7:1 */
                uint32 buf_len             : 14;   /* bits 21:8 */
                uint32 mode                : 1;    /* bits 22, always 0 */
                uint32 dest_sel            : 1;    /* bits 23 */
                uint32 hp0                 : 1;    /* bits 24, always 1 */
                uint32 source_map          : 4;    /* bits 25:28 */
                uint32 neop                : 1;    /* bits 29 */
                uint32 nsop                : 1;    /* bits 30 */
                uint32 hp1                 : 1;    /* bits 31, always 1 */
        } bf_wifi_hwlookup;
        struct {
                uint32 stream_id           : 8;    /* bits 7:0 */
                uint32 buf_len             : 11;   /* bits 18:8 */
                uint32 chk_sel             : 3;    /* bits 21:19 */
                uint32 mode                : 1;    /* bits 22, always 1 */
                uint32 direct              : 1;    /* bits 23, always 1 */
                uint32 hp0                 : 1;    /* bits 24, always 1 */
                uint32 aft_fib_map         : 6;    /* bits 25:30 */
                uint32 hp1                 : 1;    /* bits 31, always 1 */
        } bf_direct_tx_wan;
        struct {
                uint32 rsv                 : 1;    /* bits 0 */
                uint32 dest                : 4;    /* bits 4:1 */
                uint32 cos                 : 3;    /* bits 7:5 */
                uint32 buf_len             : 11;   /* bits 18:8 */
                uint32 chk_sel             : 3;    /* bits 21:19 */
                uint32 mode                : 1;    /* bits 22, always 1 */
                uint32 direct              : 1;    /* bits 23, always 0 */
                uint32 hp0                 : 1;    /* bits 24, always 1 */
                uint32 aft_fib_map         : 6;    /* bits 25:30 */
                uint32 hp1                 : 1;    /* bits 31, always 1 */
        } bf_direct_tx_lan;
} dma_txdesc_1_t;

typedef struct {
        dma_txdesc_0_t  word0;
        dma_txdesc_1_t  word1;
} dma_txdesc_t;

//hdra
/* from G3 Header_v32.xlsm */
typedef union {
        uint32 bits32[2];
        struct header_a {
                union {

                        struct {
                                uint32 mcgid                  : 8;    /* When packet type is 2'b10(CPU), redefined as L2 learn info */
                                uint32 drop_code              : 3;    /* Drop event code, used by TE */
                                uint32 rx_pkt_type            : 2;    /* 00:UC, 01:BC, 10:MC, 11:Unknown-UC
                                                                           for l3fe->l2fe traffic, use uc(00) to disable l2 learning
                                                                           uuc(11) to enable l2 learning */
                                uint32 no_drop                : 1;    /* No-Drop flag for high priority packet */
                                uint32 mirror                 : 1;    /* Packet to be mirrored */
                                uint32 mark                   : 1;    /* Conjestion experienced in QM */
                                uint32 pol_en                 : 2;    /* 2'b00: Policer disable
                                                                           2'b01: L2 Policer enable
                                                                           2'b10: L3 Policer enable
                                                                           2'b11: both L2,L3 Policer enable
                                                                         */

                                uint32 pol_id                 : 9;    /* Used for Policing in TE/QTM */
                                uint32 pol_grp_id             : 3;    /* Policer Group ID, group 0 is NULL */
                                uint32 deep_q                 : 1;    /* Packet target to DeepQ in QM */
                                uint32 cpu_flg                : 1;    /* CPU header is included */
                        } pkt_info;

                        struct {
                                uint32 ptp_info:31;           /* Ingress PTP traffic, FE copies TS to PTP_INFO.
                                                                   Egress PTP traffic carries PTP_CMD in PTP_INFO field.
                                                                   This field is overloaded with {POL_EN,POL_GRP_ID,POL_ID,MIRROR,
                                                                   DEEP_Q,MARK,CPU_FLAG,NO_DROP, RESERVED,RX_PKT_TYPE,MCGID}fields,
                                                                   valid when HDR_TYPE is 11
                                                                 */
                                uint32 cpu_flg                : 1;    /* CPU header is included */
                        } ptp_info;
                } bits_32_63;
                uint32 cos                    : 3;    /* COS */
                uint32 ldpid                  : 6;    /* Logical Destination Port ID, Packet from MC Engine, this field carries MC_INDEX */
                uint32 lspid                  : 6;    /* Original Logical Source Port ID */
                uint32 pkt_size               : 14;   /* Packet size */
                uint32 fe_bypass              : 1;    /* Bypass L2FE and L3FE processing */
                uint32 hdr_type               : 2;    /* 00: Generic UC HDR
                                                         * 01: MC HDR
                                                         * 10: CPU bound pkt
                                                         * 11: PTP ingress or egress pkt
                                                         */
        } bits;
} HEADER_A_T;

//endian convert
#define ENDIAN_CONVERT(NUM) (((NUM & 0x000000FF) << 24) | ((NUM & 0x0000FF00) << 8) | ((NUM & 0x00FF0000) >> 8) | ((NUM & 0xFF000000) >> 24))

//ring operation
#define PE_NI_INR_RING(x, step, max) do { \
                                      x += step; \
                                      if (x >= max) \
                                          x = 0; \
                                     } while(0);

//nic reg operation
//rx qm
#define PE_NI_HAL_QM_PUSH_PHY_ADDR(cpuid, eqid, phy_addr) {\
      RTK_NE_REG_WRITE( \
         QM_QM_CPU_PUSH_PADDR0 + (QM_QM_CPU_PUSH_PADDR0_STRIDE * cpuid), \
         ((uint32) phy_addr & L3QM_PHY_ADDR_MASK) | eqid \
         ); \
      }

#define PE_NI_HAL_RX_EEP_RD_IDX(cos) \
      RTK_NE_REG_READ((uint32)QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR + \
         QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_STRIDE * (PE_NI_HAL_CPUPORT_ID * COS_NUM + cos))
#define PE_NI_HAL_RX_EEP_WR_IDX(cos) \
      RTK_NE_REG_READ((uint32)QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR + \
         QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_WRPTR_STRIDE * (PE_NI_HAL_CPUPORT_ID * COS_NUM + cos))
#define PE_NI_HAL_RX_EEP_RD_IDX_UPDATE(cos, idx) \
      RTK_NE_REG_WRITE(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR + \
         QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_RDPTR_STRIDE * (PE_NI_HAL_CPUPORT_ID * COS_NUM + cos), idx)

#define PE_NI_HAL_GET_EPP_STATUS RTK_NE_REG_READ(QM_QM_CPU_EPP_STATUS1)
#define PE_NI_HAL_GET_EPP_STATUS0 RTK_NE_REG_READ(QM_QM_CPU_EPP_STATUS0)

//tx dma lso
#define PE_NI_HAL_TX_DMA_RD_IDX(qid) \
        RTK_NE_REG_READ((uint32)DMA_SEC_DMA_LSO_VP_TXQ0_RPTR + \
            DMA_SEC_DMA_LSO_VP_STRIDE * PE_NI_HAL_DMA_LSO_CPU_VP_ID + qid * 8)
#define PE_NI_HAL_TX_DMA_RD_IDX_UPDATE(qid, idx) \
        RTK_NE_REG_WRITE((uint32)DMA_SEC_DMA_LSO_VP_TXQ0_RPTR + \
            DMA_SEC_DMA_LSO_VP_STRIDE * PE_NI_HAL_DMA_LSO_CPU_VP_ID + qid * 8, idx)         
#define PE_NI_HAL_TX_DMA_WR_IDX(qid) \
        RTK_NE_REG_READ((uint32)DMA_SEC_DMA_LSO_VP_TXQ0_WPTR + \
            DMA_SEC_DMA_LSO_VP_STRIDE * PE_NI_HAL_DMA_LSO_CPU_VP_ID + qid * 8)
#define PE_NI_HAL_TX_DMA_WR_IDX_UPDATE(qid, idx) \
        RTK_NE_REG_WRITE((uint32)DMA_SEC_DMA_LSO_VP_TXQ0_WPTR + \
            DMA_SEC_DMA_LSO_VP_STRIDE * PE_NI_HAL_DMA_LSO_CPU_VP_ID + qid * 8, idx)

//CUSOMIZE DEFINE
/*****************************************************************************
 * customise setting
 *****************************************************************************/
#define PE_NI_HAL_EQ_POOL_SIZE      (2496)
#define PE_NI_HAL_PKT_BUF_COUNT     (4096)
#define PE_NI_HAL_PKT_BUF_LEN       (ni_pkt_buf_len)
#define PE_NI_HAL_PKT_BUF_MASK      (ni_pkt_buf_mask)
#define PE_NI_HAL_CPUPORT_ID        (3)
#define PE_NI_HAL_TX_TEST_DEFAULT_LDPID (1)

#define RTK_NI_DEFAULT_PKT_BUF_BASE  (0x03500000)
#define RTK_NI_DEFAULT_PKT_BUF_SIZE  (0x00500000)
#define RTK_NI_DEFAULT_PKT_BUF_LEN  (2048)
#define RTK_NI_DEFAULT_PKT_BUF_MASK (0xFFFFF800)
#define RTK_NI_DEFAULT_RX_DESC_SIZE (512)
#define RTK_NI_DEFAULT_NI_TX_DESC_SIZE_SHIFT 11
#define RTK_NI_DEFAULT_PKT_BUF_MASK (0xFFFFF800)
#define PE_NI_TX_DESC_SIZE PE_NI_HAL_TXQ_DMA_DESC_COUNT
#define MIN_NI_TX_DESC_NEED (2)

/*****************************************************************************
 * qm setting
 *****************************************************************************/
//qm axi
#define PE_NI_HAL_EQ_POOL0_CFG4     (0) /* 0x4 : ACE, 0x1 Non-Cache */
#define PE_NI_HAL_EQ_POOL0_AXI_ATTRIB   (0)
#define PE_NI_HAL_EQ_POOL1_CFG4     (0) /* 0x4 : ACE, 0x1 Non-Cache */
#define PE_NI_HAL_EQ_POOL1_AXI_ATTRIB   (0)

//qm eq
#define PE_NI_HAL_CPU_EQID0            3
#define PE_NI_HAL_CPU_EQID1            15
#define PE_NI_HAL_EPP_PROFILE_ID       7
#define PE_NI_HAL_EQ_PROFILE_ID        3

//qm epp
#define PE_NI_HAL_EPP0_DESC_SIZE64  (8)
#define PE_NI_HAL_EPP0_DESC_PER_EPP (1)
#define PE_NI_HAL_EPP0_DESC_SIZE    (4)
#define PE_NI_HAL_EPP0_DESC_CNT_PER_EPP (ni_rx_desc_size<<1)

//HEADER LEN
#define RTK_NI_DEFAULT_RX_HEADROOM                  0
#define RTK_NI_DEFAULT_HEADERA_HEADERCPU_LEN  (16)

//phy addr mask
#define L3QM_PHY_ADDR_MASK           0xffffff80
#define L3QM_PHY_ADDR_SHIFT          7

/*****************************************************************************
 * dma lso setting
 *****************************************************************************/
#define PE_NI_HAL_TXQ_DMA_DESC_DEPTH    (ni_tx_desc_size_depth)
#define PE_NI_HAL_TXQ_DMA_DESC_COUNT    (0x1<<PE_NI_HAL_TXQ_DMA_DESC_DEPTH)
#define PE_NI_HAL_TXQ_DMA_DESC_SIZEOF   (8) //sizeof(dma_txdesc_t)
#define PE_NI_HAL_TXQ_DMA_DESC_SIZE     (PE_NI_HAL_TXQ_DMA_DESC_COUNT * PE_NI_HAL_TXQ_DMA_DESC_SIZEOF)
#define PE_NI_HAL_TXQ_DMA_DESC_PHYS (PE_NI_HAL_RE0_TXQ_DMA_DESC_BASE)
#define PE_NI_HAL_DMA_LSO_CPU_VP_ID    11
#define PE_NI_HAL_TXQ_DMA_TXQ_ID       0

/*****************************************************************************
 * PE_NI Memory Address Definition & Layout
 * Memory Layout: epp desc -> dma lso tx desc -> pkt buffer
 *****************************************************************************/
#define PE_NE_DDR_NONCACHE_BASE (ni_buf_phy_addr)
#define PE_NE_DDR_NONCACHE_SIZE (ni_buf_size)

#define PE_NI_HAL_RE0_EPP0_BASE_START PE_NE_DDR_NONCACHE_BASE
#define PE_10G_SFU_NI_RX_EPP_SIZE (PE_NI_HAL_EPP0_DESC_CNT_PER_EPP * PE_NI_HAL_EPP0_DESC_SIZE * COS_NUM)
#define PE_10G_SFU_NI_TX_DESC_SIZE (PE_NI_HAL_TXQ_DMA_DESC_SIZE)
#define PE_NI_HAL_MEM_PKT_BUF_START (PE_NI_TX_PKT_BUT_START)
#define PE_NI_HAL_RE0_TXQ_DMA_DESC_BASE (PE_NI_HAL_RE0_EPP0_BASE + PE_NI_HAL_RE0_EPP0_SIZE * COS_NUM)

//pkt buffer
#define PE_NI_TX_PKT_BUT_START (PE_NI_HAL_TXQ_DMA_DESC_PHYS + PE_NI_HAL_TXQ_DMA_DESC_SIZE)
#define PE_NI_HAL_RE0_PKT_BUF_BASE  ((PE_NI_HAL_MEM_PKT_BUF_START + (PE_NI_HAL_PKT_BUF_LEN - 1)) & PE_NI_HAL_PKT_BUF_MASK)
#define PE_NI_HAL_RE0_PKT_BUF_SIZE  (PE_NI_HAL_PKT_BUF_LEN * PE_NI_HAL_PKT_BUF_COUNT)
#define PE_NI_HAL_PKT_BUF_BASE (PHYS_TO_VIRT(PE_NI_HAL_RE0_PKT_BUF_BASE))
#define PE_NI_HAL_PKT_BUF_END  (PE_NI_HAL_PKT_BUF_BASE + PE_NI_HAL_PKT_BUF_LEN * PE_NI_HAL_PKT_BUF_COUNT)

//epp
#define PE_NI_HAL_RE0_EPP0_BASE (( PE_NI_HAL_RE0_EPP0_BASE_START + (PE_NI_HAL_EPP0_DESC_SIZE - 1)) & 0xFFFFFFF0)
#define PE_NI_HAL_RE0_EPP0_SIZE (PE_NI_HAL_EPP0_DESC_CNT_PER_EPP * PE_NI_HAL_EPP0_DESC_PER_EPP * PE_NI_HAL_EPP0_DESC_SIZE)
#define PE_NI_HAL_EPP0_BASE     (PE_NI_HAL_RE0_EPP0_BASE)
#define PE_NI_HAL_EPP0_FIFO_SIZE    (PE_NI_HAL_RE0_EPP0_SIZE)
#define PE_NI_HAL_EPP0_FIFO_SIZE_PER_EPP  (PE_NI_HAL_EPP0_FIFO_SIZE)

#endif /* __NIC_H__ */
