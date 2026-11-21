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
 * aal_ni.c: Hardware Abstraction Layer for NI driver to access hardware regsiters
 *
 */

#include <osal_soc.h>
#include <osal_type.h>
#include <osal.h>
#include <aal_common.h>
#include <aal_port.h>
#include <aal_ni.h>
#include <ca_aal_proc.h>
#include <aal_scfg.h>

#define XRAM_CFG_PROF   CPU_XRAM_CFG_PROFILE_SEPARATE_LSPID

ca_uint8_t ca_ni_cpu_cpu_port_count = CA_NI_TOTAL_CPU_PORT; 

ca_status_t aal_ni_set_dma_lso_aft_l2fib_top_vlan(int index, ca_vlan_id_t vlan_id, ca_uint8_t op);

ca_uint32_t ca_ne_ddr_noncache_reserved_size;
ca_uint32_t ca_ne_ddr_coherent_reserved_size;

extern ca_uint8_t dma_lso_ace_test;

void aal_ni_dump_registers(void)
{
	uint32    reg;
	uint32    value;

	PRINTF("=====================================NI_HV GLB================================\n");
	for (reg = NI_HV_GLB_REVISION; reg <= NI_HV_GLB_DEBUG_2; reg+=4) {
		value = CA_NI_REG_READ(reg);
		PRINTF("register=0x%x, value=0x%x\n", reg, value);
	}
	PRINTF("=====================================NI_HV PT 5================================\n");
	for (reg = NI_HV_PT_PORT_STATIC_CFG + (5 * APB0_NI_HV_PT_STRIDE); reg <= NI_HV_PT_SPARESAMPLE + ( 5 * APB0_NI_HV_PT_STRIDE); reg+=4) {
		value = CA_NI_REG_READ(reg);
		PRINTF("register=0x%x, value=0x%x\n", reg, value);
	}
	PRINTF("=====================================NI_HV XRAM================================\n");
	for (reg = NI_HV_XRAM_CPUXRAM_ADRCFG_RX; reg <= NI_HV_XRAM_DMA_BYTE_CNT; reg+=4) {
		value = CA_NI_REG_READ(reg);
		PRINTF("register=0x%x, value=0x%x\n", reg, value);
	}
}

void aal_ni_dma_lso_dump_registers(void)
{
	uint32     reg;
	uint32     value;

	PRINTF("=====================================DMA LSO GLB================================\n");
	for (reg = DMA_SEC_DMA_GLB_DMA_LSO_CTRL; reg <= DMA_SEC_DMA_GLB_DMA_SSP_SPARE1; reg+=4) {
		value = CA_DMA_LSO_REG_READ(reg);
		PRINTF("register=0x%x, value=0x%x\n", reg, value);
	}

	PRINTF("=====================================DMA LSO VP================================\n");
	for (reg = DMA_SEC_DMA_LSO_VP_CONTROL; reg <= DMA_SEC_DMA_LSO_VP_HDRA_CFG; reg+=4) {
		value = CA_DMA_LSO_REG_READ(reg);
		PRINTF("register=0x%x, value=0x%x\n", reg, value);
	}
}

static char *nirx_l3fe_demux_id_str[4] = {"L3QM", "WAN", "L2FE", "L2TM"};
void aal_ni_rx_l3fe_demux_dump_registers(void)
{
	ca_ni_rx_l3fe_demux_t           demux;
	int ldpid = 0;
	NI_HV_GLB_L3QMRX_DEMUX_CFG0_t	ni_qmrx_demux0;	/* 1: to L2TM 0: to WAN */
	NI_HV_GLB_L3QMRX_DEMUX_CFG1_t	ni_qmrx_demux1;
	aal_ni_hv_glb_internal_port_id_cfg_t	ni_internal_port_cfg;

	PRINTF("=== NIRX_L3FE_DEMUX ===\n");
	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DEMUX_CFG0);
	PRINTF("ldpid=0, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	PRINTF("ldpid=1, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	PRINTF("ldpid=2, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	PRINTF("ldpid=3, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	PRINTF("ldpid=4, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	PRINTF("ldpid=5, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	PRINTF("ldpid=6, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	PRINTF("ldpid=7, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	PRINTF("ldpid=8, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	PRINTF("ldpid=9, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	PRINTF("ldpid=10, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	PRINTF("ldpid=11, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	PRINTF("ldpid=12, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	PRINTF("ldpid=13, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	PRINTF("ldpid=14, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	PRINTF("ldpid=15, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DEMUX_CFG1);
	PRINTF("ldpid=16, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	PRINTF("ldpid=17, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	PRINTF("ldpid=18, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	PRINTF("ldpid=19, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	PRINTF("ldpid=20, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	PRINTF("ldpid=21, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	PRINTF("ldpid=22, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	PRINTF("ldpid=23, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	PRINTF("ldpid=24, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	PRINTF("ldpid=25, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	PRINTF("ldpid=26, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	PRINTF("ldpid=27, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	PRINTF("ldpid=28, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	PRINTF("ldpid=29, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	PRINTF("ldpid=30, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	PRINTF("ldpid=31, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DEMUX_CFG2);
	PRINTF("ldpid=32, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	PRINTF("ldpid=33, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	PRINTF("ldpid=34, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	PRINTF("ldpid=35, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	PRINTF("ldpid=36, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	PRINTF("ldpid=37, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	PRINTF("ldpid=38, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	PRINTF("ldpid=39, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	PRINTF("ldpid=40, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	PRINTF("ldpid=41, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	PRINTF("ldpid=42, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	PRINTF("ldpid=43, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	PRINTF("ldpid=44, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	PRINTF("ldpid=45, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	PRINTF("ldpid=46, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	PRINTF("ldpid=47, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DEMUX_CFG3);
	PRINTF("ldpid=48, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	PRINTF("ldpid=49, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	PRINTF("ldpid=50, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	PRINTF("ldpid=51, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	PRINTF("ldpid=52, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	PRINTF("ldpid=53, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	PRINTF("ldpid=54, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	PRINTF("ldpid=55, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	PRINTF("ldpid=56, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	PRINTF("ldpid=57, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	PRINTF("ldpid=58, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	PRINTF("ldpid=59, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	PRINTF("ldpid=60, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	PRINTF("ldpid=61, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	PRINTF("ldpid=62, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	PRINTF("ldpid=63, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	PRINTF("=== NIRX_L3FE_DPQ_DEMUX ===\n");
	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG0);
	PRINTF("ldpid=0, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	PRINTF("ldpid=1, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	PRINTF("ldpid=2, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	PRINTF("ldpid=3, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	PRINTF("ldpid=4, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	PRINTF("ldpid=5, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	PRINTF("ldpid=6, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	PRINTF("ldpid=7, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	PRINTF("ldpid=8, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	PRINTF("ldpid=9, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	PRINTF("ldpid=10, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	PRINTF("ldpid=11, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	PRINTF("ldpid=12, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	PRINTF("ldpid=13, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	PRINTF("ldpid=14, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	PRINTF("ldpid=15, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG1);
	PRINTF("ldpid=16, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	PRINTF("ldpid=17, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	PRINTF("ldpid=18, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	PRINTF("ldpid=19, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	PRINTF("ldpid=20, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	PRINTF("ldpid=21, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	PRINTF("ldpid=22, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	PRINTF("ldpid=23, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	PRINTF("ldpid=24, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	PRINTF("ldpid=25, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	PRINTF("ldpid=26, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	PRINTF("ldpid=27, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	PRINTF("ldpid=28, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	PRINTF("ldpid=29, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	PRINTF("ldpid=30, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	PRINTF("ldpid=31, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG2);
	PRINTF("ldpid=32, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	PRINTF("ldpid=33, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	PRINTF("ldpid=34, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	PRINTF("ldpid=35, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	PRINTF("ldpid=36, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	PRINTF("ldpid=37, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	PRINTF("ldpid=38, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	PRINTF("ldpid=39, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	PRINTF("ldpid=40, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	PRINTF("ldpid=41, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	PRINTF("ldpid=42, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	PRINTF("ldpid=43, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	PRINTF("ldpid=44, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	PRINTF("ldpid=45, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	PRINTF("ldpid=46, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	PRINTF("ldpid=47, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	demux.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG3);
	PRINTF("ldpid=48, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_0, nirx_l3fe_demux_id_str[demux.bf.ldpid_0]);
	PRINTF("ldpid=49, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_1, nirx_l3fe_demux_id_str[demux.bf.ldpid_1]);
	PRINTF("ldpid=50, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_2, nirx_l3fe_demux_id_str[demux.bf.ldpid_2]);
	PRINTF("ldpid=51, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_3, nirx_l3fe_demux_id_str[demux.bf.ldpid_3]);
	PRINTF("ldpid=52, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_4, nirx_l3fe_demux_id_str[demux.bf.ldpid_4]);
	PRINTF("ldpid=53, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_5, nirx_l3fe_demux_id_str[demux.bf.ldpid_5]);
	PRINTF("ldpid=54, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_6, nirx_l3fe_demux_id_str[demux.bf.ldpid_6]);
	PRINTF("ldpid=55, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_7, nirx_l3fe_demux_id_str[demux.bf.ldpid_7]);
	PRINTF("ldpid=56, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_8, nirx_l3fe_demux_id_str[demux.bf.ldpid_8]);
	PRINTF("ldpid=57, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_9, nirx_l3fe_demux_id_str[demux.bf.ldpid_9]);
	PRINTF("ldpid=58, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_10, nirx_l3fe_demux_id_str[demux.bf.ldpid_10]);
	PRINTF("ldpid=59, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_11, nirx_l3fe_demux_id_str[demux.bf.ldpid_11]);
	PRINTF("ldpid=60, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_12, nirx_l3fe_demux_id_str[demux.bf.ldpid_12]);
	PRINTF("ldpid=61, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_13, nirx_l3fe_demux_id_str[demux.bf.ldpid_13]);
	PRINTF("ldpid=62, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_14, nirx_l3fe_demux_id_str[demux.bf.ldpid_14]);
	PRINTF("ldpid=63, demux_id=%d, demux_id_str=%s\n", demux.bf.ldpid_15, nirx_l3fe_demux_id_str[demux.bf.ldpid_15]);

	
	PRINTF("=== NI_HV_GLB_L3QMRX_DEMUX_CFG ===\n");
	ni_qmrx_demux0.wrd = CA_NI_REG_READ(NI_HV_GLB_L3QMRX_DEMUX_CFG0);
	ni_qmrx_demux1.wrd = CA_NI_REG_READ(NI_HV_GLB_L3QMRX_DEMUX_CFG1);
	aal_ni_hv_glb_internal_port_id_cfg_get(0, &ni_internal_port_cfg);

	PRINTF("---------------------------------------\n");
	for(ldpid = 0; ldpid <=15; ldpid++) {	
		PRINTF("ldpid=%d, demux_id=%d, dest=%s\n", ldpid, (ni_qmrx_demux0.wrd>>ldpid)& 0x1, (ni_qmrx_demux0.wrd>>ldpid)& 0x1 ? "L2TM" : "WAN");
	}
	PRINTF("---------------------------------------\n");
	for(ldpid = 32; ldpid <=63; ldpid++) {	
		PRINTF("ldpid=%d, demux_id=%d, dest=%s\n", ldpid, (ni_qmrx_demux1.wrd>>(ldpid-32))& 0x1 , (ni_qmrx_demux1.wrd>>(ldpid-32))& 0x1 ? "L2TM" : "WAN");
	}
	
}

ca_status_t aal_ni_set_mac_address(ca_uint8_t port_id, ca_uint8_t *mac_addr)
{
	NI_HV_GLB_MAC_ADDR_CFG0_t mac_addr_cfg0;
	NI_HV_GLB_MAC_ADDR_CFG1_t mac_addr_cfg1;
	NI_HV_PT_PORT_STATIC_CFG_t port_static_cfg;

	/* The complete MAC address consists of {MAC_ADDR0_mac_addr0[0-3], MAC_ADDR1_mac_addr1[4],
	   PT_PORT_STATIC_CFG_mac_addr6[5]}. */
	mac_addr_cfg0.wrd = 0;
	mac_addr_cfg0.bf.mac_addr0 = (mac_addr[0]<<24) + (mac_addr[1]<<16) + (mac_addr[2]<<8) + mac_addr[3];
	CA_NI_REG_WRITE(mac_addr_cfg0.wrd, NI_HV_GLB_MAC_ADDR_CFG0);

	mac_addr_cfg1.wrd = 0;
	mac_addr_cfg1.bf.mac_addr1 = mac_addr[4];
	CA_NI_REG_WRITE(mac_addr_cfg1.wrd, NI_HV_GLB_MAC_ADDR_CFG1);

	port_static_cfg.wrd = CA_NI_REG_READ(NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * port_id);
	port_static_cfg.bf.mac_addr6 = mac_addr[5];

	CA_NI_REG_WRITE(port_static_cfg.wrd, NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * port_id);

	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: port_id=%d, mac_addr=%pM\n", __func__, port_id, mac_addr);
	}

	return CA_E_OK;
}

void aal_ni_set_tx_write_ptr(ca_uint32_t reg, ca_uint32_t value)
{
	CA_DMA_LSO_REG_WRITE(value, reg);
}

ca_uint32_t aal_ni_get_tx_read_ptr(ca_uint32_t reg)
{
	ca_uint32_t value;

	value = CA_DMA_LSO_REG_READ(reg);

	return value;
}
//EXPORT_SYMBOL(aal_ni_get_tx_read_ptr);

void aal_ni_enable_rx_interrupt(ca_uint16_t cpu_port, ca_uint8_t enable)
{
	/* cpu_port = irq_no - IRQ_GLB_QM_CPU_FIFO0_REQ - 32 */
	aal_l3qm_enable_cpu_epp_interrupt(cpu_port, enable);

	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: cpu_port=%d, enable=%d\n", __func__, cpu_port, enable);
	}
}

void aal_ni_enable_rx_interrupt_by_cpu_port(ca_uint16_t cpu_port, ca_uint8_t enable)
{
	aal_l3qm_enable_cpu_epp_interrupt(cpu_port, enable);

	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: cpu_port=%d, enable=%d\n", __func__, cpu_port, enable);
	}
}

void aal_ni_write_rx_interrupt_status(ca_uint16_t irq_no, ca_uint32_t status)
{
	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: irq_no=%d, status=%d\n", __func__, irq_no, status);
	}
}

ca_uint32_t aal_ni_read_rx_interrupt_status(ca_uint16_t irq_no)
{
	ca_uint32_t status;

	status = CA_GLB_NE_INTR_REG_READ(GLOBAL_NETWORK_ENGINE_INTERRUPT_0);

	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: irq_no=%d, status=%d\n", __func__, irq_no, status);
	}

	return status;
}

int aal_ni_has_work(int instance)
{
	return 1;   /* L3QM will check the packet available alway */
}

static void aal_ni_reset(void)
{
	int i;
	NI_HV_GLB_INIT_DONE_t    init_done;

	/* Check initializtion of NI block is done */
	for (i = 0; i < CA_NI_INIT_DONE_LOOP; i++) {
		init_done.wrd = CA_NI_REG_READ(NI_HV_GLB_INIT_DONE);
		if (init_done.bf.ni_init_done)
			break;
	}
	if (i == CA_NI_INIT_DONE_LOOP) {
		PRINTF("%s: initializtion of NI block is NOT done!!!\n", __func__);
	}

	CA_NI_REG_WRITE(0x00000000, NI_HV_GLB_INTF_RST_CONFIG);
	PRINTF( "%s: reset NI done!\n", __func__);
}

#if 0
static void aal_ni_mgmt_init_xram_mem(void)
{
	NI_HV_XRAM_CPUXRAM_ADRCFG_RX_t    cpuxram_adrcfg_rx;
	NI_HV_XRAM_CPUXRAM_ADRCFG_TX_0_t cpuxram_adrcfg_tx;
	NI_HV_XRAM_CPUXRAM_CFG_t    cpuxram_cfg;

	cpuxram_adrcfg_rx.wrd = 0;
	cpuxram_adrcfg_tx.wrd = 0;

	/* RX XRAM ADDRESS CONFIG (start and end address) */
	cpuxram_adrcfg_rx.wrd = 0;
	cpuxram_adrcfg_rx.bf.rx_top_addr = CA_NI_RX_TOP_ADDR;
	cpuxram_adrcfg_rx.bf.rx_base_addr = CA_NI_RX_BASE_ADDR;
	CA_NI_REG_WRITE(cpuxram_adrcfg_rx.wrd, NI_HV_XRAM_CPUXRAM_ADRCFG_RX);

	/* TX XRAM ADDRESS CONFIG (start and end address) */
	cpuxram_adrcfg_tx.wrd = 0;
	cpuxram_adrcfg_tx.bf.tx_top_addr = CA_NI_TX_TOP_ADDR;
	cpuxram_adrcfg_tx.bf.tx_base_addr = CA_NI_TX_BASE_ADDR;
	CA_NI_REG_WRITE(cpuxram_adrcfg_tx.wrd, NI_HV_XRAM_CPUXRAM_ADRCFG_TX_0);

	cpuxram_cfg.wrd = CA_NI_REG_READ(NI_HV_XRAM_CPUXRAM_CFG);
	cpuxram_cfg.bf.rx_0_cpu_pkt_dis = 1;
	cpuxram_cfg.bf.tx_0_cpu_pkt_dis = 1;
	CA_NI_REG_WRITE(cpuxram_cfg.wrd, NI_HV_XRAM_CPUXRAM_CFG);
	cpuxram_cfg.bf.rx_0_cpu_pkt_dis = 0;
	cpuxram_cfg.bf.tx_0_cpu_pkt_dis = 0;
	CA_NI_REG_WRITE(cpuxram_cfg.wrd, NI_HV_XRAM_CPUXRAM_CFG);
}
#endif

static void aal_ni_dma_lso_axi_reo_init(ca_device_id_t device_id)
{
	int i = 0;
	AXI_REO_RD_ORIG_ID_t orig_id;

	orig_id.wrd = 0;
	orig_id.bf.axi_id = 0xf;			// disable

	for(i = 0; i < APB2_DMA_AXI_REO_AXI_REO_RD_COUNT; i++) {
		CA_NE_AXI_REO_REG_WRITE(orig_id.wrd, DMA_AXI_REO_AXI_REO_RD_ORIG_ID + (i*APB2_DMA_AXI_REO_AXI_REO_RD_STRIDE));
	}
}

void rtk_ni_init_tx_dma_lso(void)
{
	ca_status_t ret = CA_E_OK;
	DMA_SEC_DMA_GLB_CFG_LENFIX_EN_t glb_cfg_lenfix_en;
//	DMA_SEC_DMA_GLB_CCI_MAP_t glb_cci_map;
	DMA_SEC_DMA_GLB_DMA_LSO_VLAN_TAG_TYPE0_t dma_lso_vlan_cfg;
	DMA_SEC_DMA_GLB_DMA_LSO_CTRL_t	  dma_lso_ctrl;
	DMA_SEC_DMA_AFT_CTRL_t dma_aft_ctrl;
	DMA_SEC_DMA_GLB_WIFI_PKT_DIVIDE_CFG_t dma_lso_wifi_pkt_divide_cfg;
	aal_ni_dma_lso_stream_id_tbl_cfg_t stream_tbl_cfg={0};
	aal_ni_dma_lso_lspid_map_tbl_cfg_t lspid_map_tbl_cfg={0};
	aal_ni_dma_lso_dmaaft_map_tbl_cfg_t dmaaft_map_tbl_cfg={0};
	aal_ni_dma_lso_fbm_mask_tbl_cfg_t fbm_mask_cfg={0};
	int i = 0;

	//PRINTF("Init dmalso global config\n");
	glb_cfg_lenfix_en.wrd = 0;
	glb_cfg_lenfix_en.bf.lenfix_en = 1;
	CA_DMA_LSO_REG_WRITE(glb_cfg_lenfix_en.wrd, DMA_SEC_DMA_GLB_CFG_LENFIX_EN);
#if 0	//init in ca_package with CFG_ID_DMA_LSO_ACE_TEST
	glb_cci_map.wrd = 0;
	glb_cci_map.bf.cci_map = 2;
	CA_DMA_LSO_REG_WRITE(glb_cci_map.wrd, DMA_SEC_DMA_GLB_CCI_MAP);
#endif

	dma_lso_vlan_cfg.wrd = 0;
	dma_lso_vlan_cfg.bf.enable = 1;
	dma_lso_vlan_cfg.bf.value = 0x88a8;
	CA_DMA_LSO_REG_WRITE(dma_lso_vlan_cfg.wrd, DMA_SEC_DMA_GLB_DMA_LSO_VLAN_TAG_TYPE0);

	dma_lso_wifi_pkt_divide_cfg.wrd = 0;
	dma_lso_wifi_pkt_divide_cfg.bf.wifi7_prot = 0x7777;
	dma_lso_wifi_pkt_divide_cfg.bf.wifi7_en = 1;
	dma_lso_wifi_pkt_divide_cfg.bf.wifi6_en = 1;
	CA_DMA_LSO_REG_WRITE(dma_lso_wifi_pkt_divide_cfg.wrd, DMA_SEC_DMA_GLB_WIFI_PKT_DIVIDE_CFG);

	//PRINTF("Init dmalso mapping table\n");
	for(i = 0; i < CA_NI_DMA_LSO_STREAMID_TBL_SIZE ; i ++)
		aal_ni_dma_lso_set_streamid_tbl(i , &stream_tbl_cfg);
	for(i = 0; i < CA_NI_DMA_LSO_LSPID_MAP_TBL_SIZE ; i ++){
		memset(&lspid_map_tbl_cfg, 0, sizeof(lspid_map_tbl_cfg));
		lspid_map_tbl_cfg.lspid = i + AAL_LPORT_CPU_0;
		lspid_map_tbl_cfg.en_flag = 1;
		if(i >= DMA_SEC_DMA_LSO_VP_COUNT){	//reserve entry 12~15
			lspid_map_tbl_cfg.lspid = AAL_LPORT_CPU_0;
		}
		if (i == NITX_LSPID_MAP_PE0_WIFI) {	//reserve to PE0 with tx VP9
			lspid_map_tbl_cfg.lspid = 0x19;
		}else if (i == NITX_LSPID_MAP_PE1_WIFI) {	//reserve to PE1 with tx VP10
			lspid_map_tbl_cfg.lspid = 0x1a;
		}else if (i == NITX_LSPID_MAP_PE2_AMSDU_AGG) {	//reserve to PE2 with tx VP11
			lspid_map_tbl_cfg.lspid = 0x1c;
		}else if (i == NITX_LSPID_MAP_PE2_AMSDU_NAGG) {	//reserve to PE2 with tx VP11
			lspid_map_tbl_cfg.lspid = 0x1d;
		}else if (i == NITX_LSPID_MAP_FBM_EN_POOL6) {	//reserve to ARM enable FBM
			lspid_map_tbl_cfg.lspid = 0x10;
			lspid_map_tbl_cfg.fbm_en = 1;
			lspid_map_tbl_cfg.fbm_pool_id = 6;
			lspid_map_tbl_cfg.fbm_buf_mask = 0xFFFFFFFF80;
		}else if (i == NITX_LSPID_MAP_ARM_IPSEC_DECRYPT_HWLOOKUP) {		//reserve to ARM ipsec
			lspid_map_tbl_cfg.lspid = 0xa;
		}else if (i == NITX_LSPID_MAP_ARM_IPSEC_PE_FIRST_ENCRYPT_HWLOOKUP) {	//reserve to ARM ipsec first encrypt
			lspid_map_tbl_cfg.lspid = 0x21;
		}else if (i == NITX_LSPID_MAP_PE_SRV6_DECAP) {	//reserve to PE with SRv6 decap packet lookup miss recovery
			lspid_map_tbl_cfg.lspid = 0xb;
		}else if (i >= NITX_LSPID_MAP_ARM14) {	//dynamic usage in FC, take care aal_ni_dma_lso_del_lspid_map_tbl
			memset(&lspid_map_tbl_cfg, 0, sizeof(lspid_map_tbl_cfg));
		}

		aal_ni_dma_lso_set_lspid_map_tbl(i , &lspid_map_tbl_cfg);
	}
	for(i = 0; i < CA_NI_DMA_LSO_DMAAFT_MAP_TBL_SIZE ; i ++){
		if(i < AAL_LPORT_CPU_2){	//reserve first 2 entry for dmaaft disable & lspid 0x10/0x11
			memset(&dmaaft_map_tbl_cfg, 0, sizeof(dmaaft_map_tbl_cfg));
			dmaaft_map_tbl_cfg.lspid_map = i;
			dmaaft_map_tbl_cfg.en_flag = 1;
		}else{
			memset(&dmaaft_map_tbl_cfg, 0, sizeof(dmaaft_map_tbl_cfg));
		}
		aal_ni_dma_lso_set_dmaaft_map_tbl(i , &dmaaft_map_tbl_cfg);
	}
	for(i = 0; i < CA_NI_DMA_LSO_DMAAFT_TBL_SIZE ; i ++){
		aal_ni_dma_lso_clean_aft_l2fib(i);
	}
	for(i = 0; i < CA_NI_DMA_LSO_FBM_BUF_MASK_TBL_SIZE ; i ++){
		fbm_mask_cfg.fbm_buf_mask = 0;
		aal_ni_dma_lso_get_fbm_mask_tbl(i, &fbm_mask_cfg);
	}

	//PRINTF("Padding by DMAAFT rather than LSO\n");
	dma_lso_ctrl.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_GLB_DMA_LSO_CTRL);
	dma_lso_ctrl.bf.lso_padding_en = 0; 	/* add padding data for under 64 bytes data */
	dma_lso_ctrl.bf.lspid_keep = 1; 		/* keep lspid by tx desc or hdrA */
	CA_DMA_LSO_REG_WRITE(dma_lso_ctrl.wrd, DMA_SEC_DMA_GLB_DMA_LSO_CTRL);
	dma_aft_ctrl.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_CTRL);
	dma_aft_ctrl.bf.padding_en = 1;
	dma_aft_ctrl.bf.padding_size = 64;
	CA_DMA_LSO_REG_WRITE(dma_aft_ctrl.wrd, DMA_SEC_DMA_AFT_CTRL);

	//PRINTF("Init Per CPU port VP header by-pass l2fe\n");

	for(i = 0; i < DMA_SEC_DMA_LSO_VP_COUNT ; i ++)
		aal_ni_set_dma_lso_headerA(i /*port_no*/, 0 /*ldpid*/, 1 /*febypass*/);

	PRINTF( "Init DMALSO done. (Global Cfg, Lspid Map Tbl, Dmaaft Map Tbl, FBM BUF Mask Tbl, Padding Cfg, VP Reg)\n");

	return ;
}

//EXPORT_SYMBOL(rtk_ni_init_tx_dma_lso);

void aal_ni_init_tx_dma_lso(void)
{
	int i;
	DMA_SEC_DMA_GLB_DMA_LSO_CTRL_t    dma_lso_ctrl;
	DMA_SEC_DMA_LSO_VP_CONTROL_t    vp_control;
#if !defined(CONFIG_CORTINA_BOARD_FPGA) && !defined(CONFIG_REALTEK_BOARD_FPGA) && !defined(CONFIG_REALTEK_BOARD_FPGA_V8)
	DMA_SEC_AXIM2_CONFIG_t        axim2_config;
#endif
	DMA_SEC_SS_CTRL_t ss_ctrl;
	DMA_SEC_DMA_GLB_DMA_LSO_AXI_USER_SEL0_t user_sel;
	ca_ni_dma_lso_read_user_t       read_user;

	/* DMA VP TXQ control */
	vp_control.wrd = 0;
	vp_control.bf.txq0_en = 1;
	vp_control.bf.txq1_en = 1;
	vp_control.bf.txq2_en = 1;
	vp_control.bf.txq3_en = 1;
	vp_control.bf.txq4_en = 1;
	vp_control.bf.txq5_en = 1;
	vp_control.bf.txq6_en = 1;
	vp_control.bf.txq7_en = 1;

	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: register DMA_SEC_DMA_LSO_VP_CONTROL offset=0x%x, DMA_SEC_DMA_LSO_VP_STRIDE value=%d\n",
				__func__, DMA_SEC_DMA_LSO_VP_CONTROL, DMA_SEC_DMA_LSO_VP_STRIDE);
	}
	for (i = 0; i < CA_NI_DMA_LSO_VP_NUM; i++) {
		CA_DMA_LSO_REG_WRITE(vp_control.wrd, DMA_SEC_DMA_LSO_VP_CONTROL + (i * DMA_SEC_DMA_LSO_VP_STRIDE));

		vp_control.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_CONTROL + (i * DMA_SEC_DMA_LSO_VP_STRIDE));
		PRINTF( "%s: i = %d, vp_control.wrd=0x%x\n", __func__, i, vp_control.wrd);
	}

#if !defined(CONFIG_CORTINA_BOARD_FPGA) && !defined(CONFIG_REALTEK_BOARD_FPGA) && !defined(CONFIG_REALTEK_BOARD_FPGA_V8)
	/* setup AXI configuration */
	axim2_config.wrd = 0;
	axim2_config.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_AXIM2_CONFIG);
	axim2_config.bf.axi_write_outtrans_num = 0xf;
	axim2_config.bf.axi_read_outtrans_num = 0xf;
	axim2_config.bf.axi_xbus_len    = 2;
	axim2_config.bf.write_cacheline_trans_en = 1;
	axim2_config.bf.write_cacheline_trans_size = 1;
	axim2_config.bf.read_cacheline_trans_en = 1;
	axim2_config.bf.read_cacheline_trans_size = 1;
	CA_DMA_LSO_REG_WRITE(axim2_config.wrd, DMA_SEC_AXIM2_CONFIG);

	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: register DMA_SEC_AXIM2_CONFIG offset=0x%x, axim2_config.wrd=0x%x\n", __func__, DMA_SEC_AXIM2_CONFIG, axim2_config.wrd);
	}
#endif

	/* set user pattern for DMA LSO ACE */
	user_sel.wrd=0;
	user_sel.bf.vp0_rd_pat = 1;
	user_sel.bf.vp1_rd_pat = 1;
	user_sel.bf.vp2_rd_pat = 1;
	user_sel.bf.vp3_rd_pat = 1;
	user_sel.bf.vp4_rd_pat = 1;
	user_sel.bf.vp5_rd_pat = 1;
	user_sel.bf.vp6_rd_pat = 1;
	user_sel.bf.vp7_rd_pat = 1;
	user_sel.bf.vp8_rd_pat = 1;
	user_sel.bf.vp9_rd_pat = 1;
	user_sel.bf.vp10_rd_pat = 1;
	user_sel.bf.vp11_rd_pat = 1;
	CA_DMA_LSO_REG_WRITE(user_sel.wrd, DMA_SEC_DMA_GLB_DMA_LSO_AXI_USER_SEL0);

	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: register DMA_SEC_DMA_GLB_DMA_LSO_AXI_USER_SEL0 offset=0x%x, user_sel.wrd=0x%x\n", __func__,
				DMA_SEC_DMA_GLB_DMA_LSO_AXI_USER_SEL0, user_sel.wrd);
	}

	read_user.wrd = 0;
	read_user.bf.domain = 2;    /* outer sharable */
	read_user.bf.cache = 2;
	read_user.bf.ace_cmd = 1;
	CA_DMA_LSO_REG_WRITE(read_user.wrd, DMA_SEC_DMA_GLB_AXI_USER_PAT0);
	CA_DMA_LSO_REG_WRITE(read_user.wrd, DMA_SEC_DMA_GLB_AXI_USER_PAT1);
	CA_DMA_LSO_REG_WRITE(read_user.wrd, DMA_SEC_DMA_GLB_AXI_USER_PAT2);
	CA_DMA_LSO_REG_WRITE(read_user.wrd, DMA_SEC_DMA_GLB_AXI_USER_PAT3);

	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: register DMA_SEC_DMA_GLB_AXI_USER_PAT0 offset=0x%x, read_user.wrd=0x%x\n", __func__, DMA_SEC_DMA_GLB_AXI_USER_PAT0, read_user.wrd);
		PRINTF("%s: register DMA_SEC_DMA_GLB_AXI_USER_PAT1 offset=0x%x, read_user.wrd=0x%x\n", __func__, DMA_SEC_DMA_GLB_AXI_USER_PAT1, read_user.wrd);
		PRINTF("%s: register DMA_SEC_DMA_GLB_AXI_USER_PAT2 offset=0x%x, read_user.wrd=0x%x\n", __func__, DMA_SEC_DMA_GLB_AXI_USER_PAT2, read_user.wrd);
		PRINTF("%s: register DMA_SEC_DMA_GLB_AXI_USER_PAT3 offset=0x%x, read_user.wrd=0x%x\n", __func__, DMA_SEC_DMA_GLB_AXI_USER_PAT3, read_user.wrd);
	}

	/* setup DMA_SEC_SS_CTRL */
	ss_ctrl.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_SS_CTRL);
	ss_ctrl.bf.tx_en = 1;
	CA_DMA_LSO_REG_WRITE(ss_ctrl.wrd, DMA_SEC_SS_CTRL);
	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: register DMA_SEC_SS_CTRL=0x%x, ss_ctrl.wrd=0x%x\n", __func__, DMA_SEC_SS_CTRL, ss_ctrl.wrd);
	}

	dma_lso_ctrl.wrd = 0;
	dma_lso_ctrl.bf.tx_dma_enable = 1;
	dma_lso_ctrl.bf.tx_burst_len  = 3;      /* 64*64 bits */
	dma_lso_ctrl.bf.lso_padding_en = 1;     /* add padding data for under 64 bytes data */
	CA_DMA_LSO_REG_WRITE(dma_lso_ctrl.wrd, DMA_SEC_DMA_GLB_DMA_LSO_CTRL);

	dma_lso_ctrl.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_GLB_DMA_LSO_CTRL);
	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: register DMA_SEC_DMA_GLB_DMA_LSO_CTRL offset=0x%x, dma_lso_ctrl.wrd=0x%x\n", __func__, DMA_SEC_DMA_GLB_DMA_LSO_CTRL, dma_lso_ctrl.wrd);
	}

	aal_ni_dma_lso_axi_reo_init(0);

	rtk_ni_init_tx_dma_lso();
}
//EXPORT_SYMBOL(aal_ni_init_tx_dma_lso);

static void aal_ni_set_pkt_len(void)
{
	NI_HV_GLB_PKT_LEN_CFG_t  pkt_len_cfg;

	pkt_len_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_PKT_LEN_CFG);
	pkt_len_cfg.bf.max_pkt_size = CA_NI_MAX_FRAME_SIZE;
	pkt_len_cfg.bf.min_pkt_size = CA_NI_MIN_FRAME_SIZE;
	CA_NI_REG_WRITE(pkt_len_cfg.wrd, NI_HV_GLB_PKT_LEN_CFG);
}

static void aal_ni_set_port_to_cpu(ca_uint8_t port)
{
	NI_HV_GLB_STATIC_CFG_t  static_cfg;

	static_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_STATIC_CFG);
	static_cfg.bf.port_to_cpu = port;
	static_cfg.bf.txmib_mode = 1;
	static_cfg.bf.rxmib_mode = 1;
	CA_NI_REG_WRITE(static_cfg.wrd, NI_HV_GLB_STATIC_CFG);
}

#if 0
static void aal_ni_mgmt_enable_tx_rx(void)
{
	NI_HV_XRAM_CPUXRAM_CFG_t cpuxram_cfg;

	/* Need to toggle the tx and rx cpu_pkt_dis bit */
	/* after changing Address config register.      */
	cpuxram_cfg.wrd = CA_NI_REG_READ(NI_HV_XRAM_CPUXRAM_CFG);
	cpuxram_cfg.bf.rx_0_cpu_pkt_dis = 1;
	cpuxram_cfg.bf.tx_0_cpu_pkt_dis = 1;
	CA_NI_REG_WRITE(cpuxram_cfg.wrd, NI_HV_XRAM_CPUXRAM_CFG);

	cpuxram_cfg.wrd = CA_NI_REG_READ(NI_HV_XRAM_CPUXRAM_CFG);
	cpuxram_cfg.bf.rx_0_cpu_pkt_dis = 0;
	cpuxram_cfg.bf.tx_0_cpu_pkt_dis = 0;
	cpuxram_cfg.bf.xram_mgmt_promisc_mode = 1;
	CA_NI_REG_WRITE(cpuxram_cfg.wrd, NI_HV_XRAM_CPUXRAM_CFG);
}
#endif

ca_status_t aal_ni_hv_glb_internal_port_id_cfg_get(
		CA_IN ca_device_id_t          device_id,
		CA_OUT aal_ni_hv_glb_internal_port_id_cfg_t *cfg
		)
{
	NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;

	port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
	cfg->dma_to_l3fe = (ca_uint8_t) port_id_cfg.bf.dma_to_l3fe;
	cfg->wan_rxsel = (ca_uint8_t) port_id_cfg.bf.wan_rxsel;
	cfg->l3qmrx_demux_sel = (ca_uint8_t) port_id_cfg.bf.l3qmrx_demux_sel;
	cfg->mrr_dsel = (ca_uint8_t) port_id_cfg.bf.mrr_dsel;
	cfg->mrr_ldpid = (ca_uint8_t) port_id_cfg.bf.mrr_ldpid;
	cfg->mrr_cos = (ca_uint8_t) port_id_cfg.bf.mrr_cos;
	cfg->mrr_fe_bypass = (ca_uint8_t) port_id_cfg.bf.mrr_fe_bypass;
	cfg->egr_mrr_sel = (ca_uint8_t) port_id_cfg.bf.egr_mrr_sel;

	return CA_E_OK;
}
//EXPORT_SYMBOL(aal_ni_hv_glb_internal_port_id_cfg_get);

ca_status_t aal_ni_hv_glb_internal_port_id_cfg_set(
		CA_IN ca_device_id_t          device_id,
		CA_IN aal_ni_hv_glb_internal_port_id_cfg_mask_t mask,
		CA_IN aal_ni_hv_glb_internal_port_id_cfg_t *cfg
		)
{
	NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;
	port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);

	if (mask.bf.dma_to_l3fe) {
		port_id_cfg.bf.dma_to_l3fe = cfg->dma_to_l3fe;
	}

	if (mask.bf.wan_rxsel) {
		port_id_cfg.bf.wan_rxsel = cfg->wan_rxsel;
	}

	if (mask.bf.l3qmrx_demux_sel) {
		port_id_cfg.bf.l3qmrx_demux_sel = cfg->l3qmrx_demux_sel;
	}
	
	if (mask.bf.mrr_dsel) {
		port_id_cfg.bf.mrr_dsel = cfg->mrr_dsel;
	}
	
	if (mask.bf.mrr_ldpid) {
		port_id_cfg.bf.mrr_ldpid = cfg->mrr_ldpid;
	}
	
	if (mask.bf.mrr_cos) {
		port_id_cfg.bf.mrr_cos = cfg->mrr_cos;
	}
	
	if (mask.bf.mrr_fe_bypass) {
		port_id_cfg.bf.mrr_fe_bypass = cfg->mrr_fe_bypass;
	}

	
	if (mask.bf.egr_mrr_sel) {
		port_id_cfg.bf.egr_mrr_sel = cfg->egr_mrr_sel;
	}

	CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);
	return CA_E_OK;
}
//EXPORT_SYMBOL(aal_ni_hv_glb_internal_port_id_cfg_set);

void aal_ni_init_nirx_l3fe_demux(void)
{
	ca_status_t ret;
	NI_HV_GLB_NIRX_L3FE_DEMUX_CFG2_t    l3fe_demux_cfg2;
	NI_HV_GLB_NIRX_L3FE_DEMUX_CFG3_t    l3fe_demux_cfg3;
	NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG2_t        l3fe_dpq_demux_cfg2;
	NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG3_t        l3fe_dpq_demux_cfg3;

	NI_HV_GLB_NIRX_L3FE_DEMUX_CFG0_t    l3fe_demux_cfg0;
	NI_HV_GLB_NIRX_L3FE_DEMUX_CFG1_t    l3fe_demux_cfg1;
	NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG0_t        l3fe_dpq_demux_cfg0;
	NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG1_t        l3fe_dpq_demux_cfg1;
	ca_ni_rx_l3fe_demux_t        demux;

	ca_uint8_t l3fe_to_wan_port;

	#if 0
	ret = aal_scfg_read(CFG_ID_L3FE_TO_WAN_PORT, 1, &l3fe_to_wan_port);
	if (ret != CA_E_OK) {
		PRINTF("%s: failed to read CFG_ID_L3FE_TO_WAN_PORT, ret=%d\n", __func__, ret);
		l3fe_to_wan_port = 1;
	}
	#endif
	l3fe_to_wan_port = CFG_ID_L3FE_TO_WAN_PORT;

	/* ldpid 0-15: L2FE */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* determined by start up config to go to WAN port or L3QM  */
	if (l3fe_to_wan_port)
		demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_WAN;
	else
		demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;

	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_demux_cfg0.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_demux_cfg0.wrd, NI_HV_GLB_NIRX_L3FE_DEMUX_CFG0);

	/* ldpid 16-23: L3QM (CPU) */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* ldpid 24-27: L2FE */
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 28: no assigned */
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 29: L3QM (CPUQ) */
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* ldpid 30: L2FE (SPL) */
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 31: L3QM (BH) */
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_demux_cfg1.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_demux_cfg1.wrd, NI_HV_GLB_NIRX_L3FE_DEMUX_CFG1);

	/* ldpid 32-47: L3QM */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_demux_cfg2.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_demux_cfg2.wrd, NI_HV_GLB_NIRX_L3FE_DEMUX_CFG2);

	/* ldpid 48-63: L3QM */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_demux_cfg3.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_demux_cfg3.wrd, NI_HV_GLB_NIRX_L3FE_DEMUX_CFG3);

	/* DEEPQ configure */
	/* all of ports default go through L3QM */
	demux.wrd = 0;

	/* ldpid 0-7: L3QM */
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* determined by start up config to go to WAN port or L3QM  */
	if (l3fe_to_wan_port)
		demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_WAN;
	else
		demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* ldpid 8-15: L2FE */
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L2FE;
	l3fe_dpq_demux_cfg0.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_dpq_demux_cfg0.wrd, NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG0);

	/* ldpid 16-23: L3QM (CPU) */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* ldpid 24-27: L2FE */
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L2FE;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 28: no assigned */
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 29: L3QM (CPUQ) */
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;

	/* ldpid 30: L2FE (SPL) */
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L2FE;

	/* ldpid 31: L3QM (BH) */
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_dpq_demux_cfg1.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_dpq_demux_cfg1.wrd, NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG1);

	/* ldpid 32-47: L3QM */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_dpq_demux_cfg2.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_dpq_demux_cfg2.wrd, NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG2);

	/* ldpid 48-63: L3QM */
	demux.wrd = 0;
	demux.bf.ldpid_0 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_1 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_2 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_3 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_4 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_5 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_6 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_7 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_8 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_9 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_10 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_11 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_12 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_13 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_14 = NIRX_L3FE_DEMUX_ID_L3QM;
	demux.bf.ldpid_15 = NIRX_L3FE_DEMUX_ID_L3QM;
	l3fe_dpq_demux_cfg3.wrd = demux.wrd;
	CA_NI_REG_WRITE(l3fe_dpq_demux_cfg3.wrd, NI_HV_GLB_NIRX_L3FE_DPQ_DEMUX_CFG3);
}

static void aal_ni_init_dma_to_l3fe(void)
{
	int i;
	char dma_to_l3fe[CFG_ID_NE_DMA_TO_L3FE_LEN];
	char _dma_to_l3fe[CFG_ID_NE_DMA_TO_L3FE_LEN] = CFG_ID_NE_DMA_TO_L3FE;
	NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;

	memset(&dma_to_l3fe[0], 0, CFG_ID_NE_DMA_TO_L3FE_LEN);
	#if 0
	if (aal_scfg_read(CFG_ID_NE_DMA_TO_L3FE, CFG_ID_NE_DMA_TO_L3FE_LEN, &(dma_to_l3fe[0])) == CA_E_OK) {
		port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
		for (i = 0; i < CFG_ID_NE_DMA_TO_L3FE_LEN; i++) {
			port_id_cfg.bf.dma_to_l3fe += (dma_to_l3fe[i] << i);
		}
		PRINTF("%s: port_id_cfg.bf.dma_to_l3fe=0x%x\n", __func__, port_id_cfg.bf.dma_to_l3fe);
		CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);
	}
	#endif
	memcpy(dma_to_l3fe, _dma_to_l3fe, CFG_ID_NE_DMA_TO_L3FE_LEN);
	port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
	for (i = 0; i < CFG_ID_NE_DMA_TO_L3FE_LEN; i++) {
		port_id_cfg.bf.dma_to_l3fe += (dma_to_l3fe[i] << i);
	}
	PRINTF("%s: port_id_cfg.bf.dma_to_l3fe=0x%x\n", __func__, port_id_cfg.bf.dma_to_l3fe);
	CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);
}

void aal_ni_init_ni(int port_id)
{
	NI_HV_GLB_NIRX_MISC_CFG_t nirx_misc;

	#if 0
	if (aal_scfg_read(CFG_ID_NE_CPU_CPU_PORT_COUNT, 1, &ca_ni_cpu_cpu_port_count) == CA_E_OK) {
		if (!ca_ni_cpu_cpu_port_count || ca_ni_cpu_cpu_port_count > CA_NI_TOTAL_CPU_PORT) {
			ca_ni_cpu_cpu_port_count = CA_NI_TOTAL_CPU_PORT - 2;
			PRINTF("%s: CFG_ID_NE_CPU_CPU_PORT_COUNT=%d too large!!\n", __func__, ca_ni_cpu_cpu_port_count);
		}
	}
	#endif
	ca_ni_cpu_cpu_port_count = CFG_ID_NE_CPU_CPU_PORT_COUNT;
	if (!ca_ni_cpu_cpu_port_count || ca_ni_cpu_cpu_port_count > CA_NI_TOTAL_CPU_PORT) {
		ca_ni_cpu_cpu_port_count = CA_NI_TOTAL_CPU_PORT - 2;
		PRINTF("%s: CFG_ID_NE_CPU_CPU_PORT_COUNT=%d too large!!\n", __func__, ca_ni_cpu_cpu_port_count);
	}

	aal_ni_reset();

	/* set packet length globally */
	aal_ni_set_pkt_len();

	/* Initial TX DMA LSO */
	aal_ni_init_tx_dma_lso();

	aal_ni_set_port_to_cpu(0xff);   /* only for debugging */

	aal_ni_init_nirx_l3fe_demux();

	PRINTF( "%s: ======= NI_HV_GLB_INTERNAL_PORT_ID_CFG ==============\n", __func__);
	{
		NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;
		NI_HV_GLB_L3QMRX_DEMUX_CFG0_t	ni_qmrx_demux0;	/* 1: to L2TM 0: to WAN */
		NI_HV_GLB_L3QMRX_DEMUX_CFG1_t	ni_qmrx_demux1;
		

		port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
		port_id_cfg.bf.l3qmrx_demux_sel = 0x0; 
		CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);

#if defined(CONFIG_RTL8198X_SERIES)
		ni_qmrx_demux0.bf.l3qmrx_demux_sel_enhanced0 = 0xFFFF7FFF;		/* port7 to L2TM */
#else
		ni_qmrx_demux0.bf.l3qmrx_demux_sel_enhanced0 = 0xFFFF7F7F;
#endif
		CA_NI_REG_WRITE(ni_qmrx_demux0.wrd, NI_HV_GLB_L3QMRX_DEMUX_CFG0);
		ni_qmrx_demux1.bf.l3qmrx_demux_sel_enhanced1 = 0x0;
		CA_NI_REG_WRITE(ni_qmrx_demux1.wrd, NI_HV_GLB_L3QMRX_DEMUX_CFG1);
	}

	{
		NI_HV_GLB_INTERNAL_PORT_ID_CFG_t  port_id_cfg;
		//ca_device_config_tlv_t scfg_tlv;
		ca_status_t ret = CA_E_OK;
		ca_int8_t wan_rxsel = 0;

		#if 0
		memset(&scfg_tlv, 0, sizeof(scfg_tlv));
		scfg_tlv.type = CA_CFG_ID_WAN_TO_L3FE;
		ret = ca_device_config_tlv_get(0, &scfg_tlv);
		if (CA_E_OK == ret){
			wan_rxsel = scfg_tlv.value[0] ? 0 : 2;
		}
		#endif
		wan_rxsel = 2;

		/* L3QM to L2TM for port 0-6 and to WAN for port 7 */
		port_id_cfg.wrd = CA_NI_REG_READ(NI_HV_GLB_INTERNAL_PORT_ID_CFG);
		port_id_cfg.bf.wan_rxsel = wan_rxsel;
		CA_NI_REG_WRITE(port_id_cfg.wrd, NI_HV_GLB_INTERNAL_PORT_ID_CFG);
	}

	nirx_misc.wrd = CA_NI_REG_READ(NI_HV_GLB_NIRX_MISC_CFG);
	nirx_misc.bf.use_p0_as_wan_port_en = 0;
	nirx_misc.bf.l2te_ni_txdma_l3fe_rdy_en = 1;
	nirx_misc.bf.l2te_ni_txdma_l2fe_rdy_en = 1;
	nirx_misc.bf.l2te_ni_l3felan_port_rdy_en = 1;
	nirx_misc.bf.l2te_ni_l3fewan_port_rdy_en = 1;
	nirx_misc.bf.l2te_ni_mc_rdy_en = 1;  
	CA_NI_REG_WRITE(nirx_misc.wrd, NI_HV_GLB_NIRX_MISC_CFG);

}

static void aal_ni_enable_interrupts(void)
{
	NI_HV_GLB_PORT_8_INTERRUPTE_t   port_8_inte;
	NI_HV_GLB_INTERRUPTE_t       inte;
	NI_HV_GLB_TXEM_INTERRUPTE_t       txem_inte;

	/* Enable NI interrupt for CPU port */
	port_8_inte.wrd = 0;
	port_8_inte.bf.rxcntrl_overrunIE    = 1;
	port_8_inte.bf.rxmib_cntmsb_setIE    = 1;
	port_8_inte.bf.txmib_cntmsb_setIE    = 1;
	CA_NI_REG_WRITE(port_8_inte.wrd, NI_HV_GLB_PORT_8_INTERRUPTE + (CA_NI_PPORT_CPU * NI_HV_GLB_PORT_8_INTERRUPTE_STRIDE));

	/* Enable NI global interrupts */
	inte.wrd = 0;
	inte.bf.cpuxram_rxpktE = 1;
	inte.bf.cpuxram_txpktE = 1;
	CA_NI_REG_WRITE(inte.wrd, NI_HV_GLB_INTERRUPTE);

	/* Enable NI TXEM interrupt */
	txem_inte.wrd = 0;
	txem_inte.bf.txem_ififo_ovfIE     = 1;
	txem_inte.bf.txrm_cntovfIE     = 1;
	txem_inte.bf.txmib_fifo_ovfIE     = 1;
	txem_inte.bf.rxmib_fifo_ovfIE    = 1;
	txem_inte.bf.txrm_timerexpIE    = 1;
	CA_NI_REG_WRITE(txem_inte.wrd, NI_HV_GLB_TXEM_INTERRUPTE);
}

extern void ca_ni_dma_lso_interrupt_debug_info(uint32 dump, DMA_SEC_DMA_GLB_LSO_INTERRUPT_t *dma_lso_status_p);


void aal_ni_get_dma_glb_lso_interrupt_status(void)
{
	DMA_SEC_DMA_GLB_LSO_INTERRUPT_t	dma_lso_status;
	

	dma_lso_status.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_GLB_LSO_INTERRUPT);
	PRINTF("%s: dma_lso_status.wrd=0x%x\n", __func__, dma_lso_status.wrd);
	ca_ni_dma_lso_interrupt_debug_info((ca_aal_debug & AAL_DBG_DMA_LSO_INTERRUPT), &dma_lso_status);

	CA_DMA_LSO_REG_WRITE(dma_lso_status.wrd, DMA_SEC_DMA_GLB_LSO_INTERRUPT);
}

void aal_ni_get_dma_lso_interrupt_status(ca_uint8_t dma_vp)
{
        DMA_SEC_DMA_LSO_VP_VP_INTERRUPT_t vp_int_status;
        DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT_t vp_misc_int_status;
        DMA_SEC_DMA_LSO_VP_COAL_INTERRUPT_t vp_coal_int_status;
	ca_uint32_t offset;

	/* only test for VP 10 with CPU256 */
	offset = DMA_SEC_DMA_LSO_VP_VP_INTERRUPT + dma_vp * DMA_SEC_DMA_LSO_VP_STRIDE;
	vp_int_status.wrd = CA_DMA_LSO_REG_READ(offset);
	//if (vp_int_status.bf.COAL_INTERRUPTi == 1 || vp_int_status.bf.MISC_INTERRUPTi == 1) 
	{
		/* write back to diable interrupt */
		CA_DMA_LSO_REG_WRITE(vp_int_status.wrd, offset);

		offset = DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + dma_vp * DMA_SEC_DMA_LSO_VP_STRIDE;
		vp_misc_int_status.wrd = CA_DMA_LSO_REG_READ(offset);

		/* write back to diable interrupt */
		if (vp_misc_int_status.wrd) {
			if (vp_misc_int_status.bf.l2te_te_bp) {
				if (ca_aal_debug & AAL_DBG_DMA_LSO)
					PRINTF("DMA LSO BP!!\n");
			}
			PRINTF("%s: VP[%d] vp_misc_int_status.wrd=0x%x, VP BP sts %d\n", __func__, dma_vp, vp_misc_int_status.wrd, vp_misc_int_status.bf.l2te_te_bp);
			CA_DMA_LSO_REG_WRITE(vp_misc_int_status.wrd, offset);
		}

		offset = DMA_SEC_DMA_LSO_VP_COAL_INTERRUPT + dma_vp * DMA_SEC_DMA_LSO_VP_STRIDE;
		vp_coal_int_status.wrd = CA_DMA_LSO_REG_READ(offset);

		/* write back to diable interrupt */
		if (vp_coal_int_status.wrd) {
			CA_DMA_LSO_REG_WRITE(vp_coal_int_status.wrd, offset);
		}
	}
}
//EXPORT_SYMBOL(aal_ni_get_dma_lso_interrupt_status);

static void aal_ni_enable_dma_lso_interrupts(void)
{
	int i;
	DMA_SEC_DMA_GLB_LSO_INTENABLE_t	dma_glb_lso_inten;
	DMA_SEC_DMA_LSO_VP_VP_INTENABLE_t vp_inten;
	DMA_SEC_DMA_LSO_VP_MISC_INTENABLE_t vp_misc_inten;
	DMA_SEC_DMA_LSO_VP_COAL_INTENABLE_t vp_coal_inten;

	vp_inten.wrd = 0;
	vp_inten.bf.MISC_INTERRUPTi_en = 1;
	vp_inten.bf.COAL_INTERRUPTi_en = 1;
	for (i = 0; i < CA_NI_DMA_LSO_VP_NUM; i++) {
		CA_DMA_LSO_REG_WRITE(vp_inten.wrd, DMA_SEC_DMA_LSO_VP_VP_INTENABLE + (i * DMA_SEC_DMA_LSO_VP_STRIDE));
	}

	vp_misc_inten.wrd = 0;
	vp_misc_inten.bf.txq0_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq0_des_err_en     = 1;
	vp_misc_inten.bf.txq1_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq1_des_err_en    = 1;
	vp_misc_inten.bf.txq2_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq2_des_err_en        = 1;
	vp_misc_inten.bf.txq3_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq3_des_err_en        = 1;
	vp_misc_inten.bf.txq4_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq4_des_err_en        = 1;
	vp_misc_inten.bf.txq5_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq5_des_err_en        = 1;
	vp_misc_inten.bf.txq6_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq6_des_err_en        = 1;
	vp_misc_inten.bf.txq7_pktcnt_overrun_en = 1;
	vp_misc_inten.bf.txq7_des_err_en        = 1;
	vp_misc_inten.bf.fbm_timeout_err_en     = 1;
	vp_misc_inten.bf.l2te_te_bp_en   	= 1;
	for (i = 0; i < CA_NI_DMA_LSO_VP_NUM; i++) {
		CA_DMA_LSO_REG_WRITE(vp_misc_inten.wrd, DMA_SEC_DMA_LSO_VP_MISC_INTENABLE + (i * DMA_SEC_DMA_LSO_VP_STRIDE));
	}

	vp_coal_inten.wrd = 0;
	for (i = 0; i < CA_NI_DMA_LSO_VP_NUM; i++) {
		CA_DMA_LSO_REG_WRITE(vp_coal_inten.wrd, DMA_SEC_DMA_LSO_VP_COAL_INTENABLE + (i * DMA_SEC_DMA_LSO_VP_STRIDE));
	}

	dma_glb_lso_inten.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_GLB_LSO_INTENABLE);
	dma_glb_lso_inten.bf.dma_aft_int_en = 1;
	dma_glb_lso_inten.bf.pkt_len_err_en = 1;
	dma_glb_lso_inten.bf.udp_dec_err_en = 1;
	dma_glb_lso_inten.bf.tcp_dec_err_en = 1;
	dma_glb_lso_inten.bf.ipv6_dec_err_en = 1;
	dma_glb_lso_inten.bf.ipv4_dec_err_en = 1;
	CA_DMA_LSO_REG_WRITE(dma_glb_lso_inten.wrd, DMA_SEC_DMA_GLB_LSO_INTENABLE);
}

void aal_ni_init_interrupt_cfg(void)
{

	/* Enable NI interrupts */
	aal_ni_enable_interrupts();

	/* L3QM enable interrupt source */
	aal_l3qm_enable_int_src();

	/* Enable DMA LSO interrupt */
	aal_ni_enable_dma_lso_interrupts();
}

void aal_ni_get_dma_lso_base_depth_addr(ca_uint8_t instance, ca_uint8_t txq_id)
{
	int i;
	DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS_t      txq_base_depth_access;
	ca_uint32_t     reg_off = (instance * DMA_SEC_DMA_LSO_VP_STRIDE);
	ca_uint32_t    value;

	txq_base_depth_access.wrd = 0;
	txq_base_depth_access.bf.txq_id = txq_id;
	txq_base_depth_access.bf.access = 1;
	txq_base_depth_access.bf.rbw = 0;
	CA_DMA_LSO_REG_WRITE(txq_base_depth_access.wrd, DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + reg_off);

	/* wait for complete */
	for (i = 0; i < CA_NI_DMA_LSO_BASE_DEPTH_LOOP; i++) {
		txq_base_depth_access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + reg_off);
		if (txq_base_depth_access.bf.access == 0)
			break;
	}

	if (i == CA_NI_DMA_LSO_BASE_DEPTH_LOOP) {
		PRINTF("%s: indirect access write not finished!!!\n", __func__);
	}

	value = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0 + reg_off);

	CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1 + reg_off);

	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0=0x%x, reg_off=%d, value=0x%x\n", __func__,
				DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0, reg_off, value);
		PRINTF("%s: DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1=0x%x, reg_off=%d, value=0x%x\n", __func__,
				DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1, reg_off, value);
	}

}
//EXPORT_SYMBOL(aal_ni_get_dma_lso_base_depth_addr);

void aal_ni_set_dma_lso_base_depth_addr(ca_uint8_t instance, ca_uint8_t txq_id, ca_uint32_t tx_desc_paddr)
{
	int i;
	DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0_t    txq_base_depth_data0;
	DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1_t    txq_base_depth_data1;
	DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS_t    txq_base_depth_access;
	ca_uint32_t    reg_off = (instance * DMA_SEC_DMA_LSO_VP_STRIDE);

	txq_base_depth_data0.wrd = 0;
	txq_base_depth_data0.bf.base_addr = tx_desc_paddr >> 4;        /* base address bit [4:31] should be 16-byte alignment */
	txq_base_depth_data0.bf.depth = CA_NI_DMA_LSO_TXQ_DEPTH;      /* depth=log2(1024)=10 */

	CA_DMA_LSO_REG_WRITE(txq_base_depth_data0.wrd, DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0 + reg_off);

	txq_base_depth_data1.wrd = 0;
	if(dma_lso_ace_test){
		txq_base_depth_data1.bf.base_addr = 2;   /* CCI path */
	}else{
		txq_base_depth_data1.bf.base_addr = 0;	 /* !CCI path */
	}

	CA_DMA_LSO_REG_WRITE(txq_base_depth_data1.wrd, DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1 + reg_off);

	txq_base_depth_access.wrd = 0;
	txq_base_depth_access.bf.txq_id = txq_id;
	txq_base_depth_access.bf.access = 1;
	txq_base_depth_access.bf.rbw = 1;

	CA_DMA_LSO_REG_WRITE(txq_base_depth_access.wrd, DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + reg_off);

	/* wait for complete */
	for (i = 0; i < CA_NI_DMA_LSO_BASE_DEPTH_LOOP; i++) {
		txq_base_depth_access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + reg_off);
		if (txq_base_depth_access.bf.access == 0)
			break;
	}

	if (i == CA_NI_DMA_LSO_BASE_DEPTH_LOOP) {
		PRINTF("%s: indirect access write not finished!!!\n", __func__);
	}

	aal_ni_get_dma_lso_base_depth_addr(instance, txq_id);
}
//EXPORT_SYMBOL(aal_ni_set_dma_lso_base_depth_addr);

void aal_ni_set_dma_lso_headerA(ca_uint16_t port_no, ca_uint16_t ldpid, ca_uint8_t febypass)
{
	DMA_SEC_DMA_LSO_VP_HDRA_CFG_t   hdra_cfg;

	//PRINTF("%s: port_no=%d, ldpid=%d, febypass=%d\n", __func__, port_no, ldpid, febypass);

	hdra_cfg.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_HDRA_CFG + port_no * DMA_SEC_DMA_LSO_VP_STRIDE);
	hdra_cfg.bf.hdra_ldpid = ldpid;
	hdra_cfg.bf.hdra_febypass = febypass;
	CA_DMA_LSO_REG_WRITE(hdra_cfg.wrd, DMA_SEC_DMA_LSO_VP_HDRA_CFG + port_no * DMA_SEC_DMA_LSO_VP_STRIDE);
}
//EXPORT_SYMBOL(aal_ni_set_dma_lso_headerA);

void aal_ni_clear_dma_lso_misc_interrupts(ca_uint16_t cpu_port, ca_uint16_t txq_index)
{
	DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT_t	misc_int;

	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: cpu_port=%d, txq_index=%d\n", __func__, cpu_port, txq_index);
	}
	misc_int.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + cpu_port * DMA_SEC_DMA_LSO_VP_STRIDE);
	switch (txq_index) {
		case 0:
			misc_int.bf.txq0_empty = 1;
			break;
		case 1:
			misc_int.bf.txq1_empty = 1;
			break;
		case 2:
			misc_int.bf.txq2_empty = 1;
			break;
		case 3:
			misc_int.bf.txq3_empty = 1;
			break;
		case 4:
			misc_int.bf.txq4_empty = 1;
			break;
		case 5:
			misc_int.bf.txq5_empty = 1;
			break;
		case 6:
			misc_int.bf.txq5_empty = 1;
			break;
		case 7:
			misc_int.bf.txq5_empty = 1;
			break;
		default:
			ca_printf("%s: txq_index=%d out of range!!\n", __func__, txq_index);
			return;
	}
	CA_DMA_LSO_REG_WRITE(misc_int.wrd, DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + cpu_port * DMA_SEC_DMA_LSO_VP_STRIDE);

	if (ca_aal_debug & AAL_DBG_NI) {
		misc_int.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + cpu_port * DMA_SEC_DMA_LSO_VP_STRIDE);
		PRINTF("%s: after write misc_int.wrd=0x%x\n", __func__, misc_int.wrd);
	}
}

#define CA_NI_DMA_LSO_EMPTY_TIMEOUT	200
ca_status_t aal_ni_wait_dma_lso_misc_interrupts(ca_uint16_t cpu_port, ca_uint16_t txq_index)
{
	int i;
	ca_uint8_t	empty;
	DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT_t     misc_int;

	if (ca_aal_debug & AAL_DBG_NI) {
		PRINTF("%s: cpu_port=%d, txq_index=%d\n", __func__, cpu_port, txq_index);
	}
	for (i = 0; i < CA_NI_DMA_LSO_EMPTY_TIMEOUT; i++) {
		empty = 0;
		misc_int.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + cpu_port * DMA_SEC_DMA_LSO_VP_STRIDE);
		if (ca_aal_debug & AAL_DBG_NI) {
			PRINTF("%s: misc_int.wrd=0x%x\n", __func__, misc_int.wrd);
		}
		switch (txq_index) {
			case 0:
				empty = misc_int.bf.txq0_empty;
				break;
			case 1:
				empty = misc_int.bf.txq1_empty;
				break;
			case 2:
				empty = misc_int.bf.txq2_empty;
				break;
			case 3:
				empty = misc_int.bf.txq3_empty;
				break;
			case 4:
				empty = misc_int.bf.txq4_empty;
				break;
			case 5:
				empty = misc_int.bf.txq5_empty;
				break;
			case 6:
				empty = misc_int.bf.txq6_empty;
				break;
			case 7:
				empty = misc_int.bf.txq7_empty;
				break;
			default:
				ca_printf("%s: txq_index=%d out of range!!\n", __func__, txq_index);
				return CA_E_ERROR;
		}
		if (empty) {
			break;
		}
	}

	if (i == CA_NI_DMA_LSO_EMPTY_TIMEOUT) {
		ca_printf("%s: Wait for DMA LSO MISC interrupt empty bit timeout!!\n", __func__);
		ca_printf("%s: misc_int.wrd=0x%x\n", __func__, misc_int.wrd);
		ca_printf("%s: cpu_port=%d, txq_index=%d, offset=0x%x\n", __func__,
				cpu_port, txq_index, DMA_SEC_DMA_LSO_VP_MISC_INTERRUPT + cpu_port * DMA_SEC_DMA_LSO_VP_STRIDE);
		return CA_E_ERROR;
	}
	aal_ni_clear_dma_lso_misc_interrupts(cpu_port, txq_index);
	return CA_E_OK;
}

/* for DMA AFT test, Venus only */
#define CA_NI_DMA_LSO_AFT_TIMEOUT     200
ca_status_t aal_ni_dma_lso_get_aft_l2fib(
	int index,
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t *data0,
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t *data1,
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t *data2)
{
	int i;
	DMA_SEC_DMA_AFT_L2FIB_ACCESS_t  access;

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.rbw = 0;
	access.bf.addr = index;
	CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_AFT_L2FIB_ACCESS);

	for (i = 0; i < CA_NI_DMA_LSO_AFT_TIMEOUT; i++) {
		access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_L2FIB_ACCESS);
		if (access.bf.access == 0) {
			break;
		}
	}
	if (i == CA_NI_DMA_LSO_AFT_TIMEOUT) {
		PRINTF("%s: read DMA_SEC_DMA_AFT_L2FIB_DATA timeout failed!!\n", __func__);
		return CA_E_ERROR;
	}

	data0->wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_L2FIB_DATA0);
	data1->wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_L2FIB_DATA1);
	data2->wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_L2FIB_DATA2);

	return CA_E_OK;
}
ca_status_t aal_ni_dma_lso_set_aft_l2fib(
	int index,
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t data0,
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t data1,
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t data2)
{
	int i;
	DMA_SEC_DMA_AFT_L2FIB_ACCESS_t  access;

        CA_DMA_LSO_REG_WRITE(data0.wrd, DMA_SEC_DMA_AFT_L2FIB_DATA0);
        CA_DMA_LSO_REG_WRITE(data1.wrd, DMA_SEC_DMA_AFT_L2FIB_DATA1);
        CA_DMA_LSO_REG_WRITE(data2.wrd, DMA_SEC_DMA_AFT_L2FIB_DATA2);

        access.wrd = 0;
        access.bf.access = 1;
        access.bf.rbw = 1;
        access.bf.addr = index;
        CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_AFT_L2FIB_ACCESS);

        for (i = 0; i < CA_NI_DMA_LSO_AFT_TIMEOUT; i++) {
                access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_L2FIB_ACCESS);
                if (access.bf.access == 0) {
                        break;
                }
        }
        if (i == CA_NI_DMA_LSO_AFT_TIMEOUT) {
                PRINTF("%s: write DMA_SEC_DMA_AFT_L2FIB_DATA timeout failed!!\n", __func__);
                return CA_E_ERROR;
        }
        return CA_E_OK;
}


//EXPORT_SYMBOL(aal_ni_dma_lso_get_aft_l2fib);
//EXPORT_SYMBOL(aal_ni_dma_lso_set_aft_l2fib);

ca_status_t aal_ni_set_dma_lso_aft_l2fib_top_vlan(int index, ca_vlan_id_t vlan_id, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t	data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t	data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t	data2;

	PRINTF("%s: index=%d, vlan_id=%d, op=%d\n", __func__, index, vlan_id, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	PRINTF("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	data1.bf.top_vid = vlan_id;
	PRINTF("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;		/* stacking mode */
	data2.bf.vlan_cnt = op;		/* push(1)/pop(2)/swap(3) */
	data2.bf.top_tpid_enc = 1;	/* select TPID 1 */
	PRINTF("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_inner_vlan(int index, ca_vlan_id_t vlan_id, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	PRINTF("%s: index=%d, vlan_id=%d, op=%d\n", __func__, index, vlan_id, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	data0.bf.inner_vid = (vlan_id & 0x3f);      /* bit 0-5 */
	PRINTF("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	data1.bf.inner_tpid_enc = 1;	/* TPID 1 */
	data1.bf.inner_vid = (vlan_id >> 6) & 0x3f;    /* bit 6-11 */
	PRINTF("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	data2.bf.inner_vlan_cmd = op;
	PRINTF("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_top_vlan_8021p_dei(int index, ca_uint8_t vlan_8021p_dei, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	PRINTF("%s: index=%d, vlan_8021p_dei=%d, op=%d\n", __func__, index, vlan_8021p_dei, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	PRINTF("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	data1.bf.top_dei = (vlan_8021p_dei & 0x1);
	data1.bf.top_dei_sel = op;
	data1.bf.top_802_1p = (vlan_8021p_dei & 0xe) >> 1;
	data1.bf.top_1p_sel = op;
	PRINTF("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	PRINTF("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_inner_vlan_8021p_dei(int index, ca_uint8_t vlan_8021p_dei, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	PRINTF("%s: index=%d, vlan_8021p_dei=%d, op=%d\n", __func__, index, vlan_8021p_dei, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	data0.bf.inner_dei = (vlan_8021p_dei & 0x1);
	data0.bf.inner_dei_sel = op;
	data0.bf.inner_802_1p = (vlan_8021p_dei & 0xe) >> 1;
	data0.bf.inner_1p_sel = op;
	PRINTF("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	PRINTF("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	PRINTF("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_top_tpid(int index, ca_uint8_t tpid_enc, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	PRINTF("%s: index=%d, tpid_enc=%d, op=%d\n", __func__, index, tpid_enc, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	PRINTF("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	data1.bf.top_tpid_sel = op & 1;    /* bit 0 */
	PRINTF("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	data2.bf.top_tpid_enc = tpid_enc;
	data2.bf.top_tpid_sel = (op & 2) >> 1;    /* bit 1 */
	PRINTF("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_inner_tpid(int index, ca_uint8_t tpid_enc, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	PRINTF("%s: index=%d, tpid_enc=%d, op=%d\n", __func__, index, tpid_enc, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	PRINTF("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	data1.bf.inner_tpid_enc = tpid_enc;
	data1.bf.inner_tpid_sel = op;
	PRINTF("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	PRINTF("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_set_dma_lso_aft_l2fib_pppoe(int index, ca_uint16_t session_id, ca_uint8_t op)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	PRINTF("%s: index=%d, session_id=%d, op=%d\n", __func__, index, session_id, op);

	/* read current settings first */
	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	data0.bf.pppoe_cmd = op;
	data0.bf.session_id = session_id;
	PRINTF("%s: data0.wrd=0x%x\n", __func__, data0.wrd);

	PRINTF("%s: data1.wrd=0x%x\n", __func__, data1.wrd);

	data2.bf.vlan_vld = 0;          /* stacking mode */
	PRINTF("%s: data2.wrd=0x%x\n", __func__, data2.wrd);

	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

	return CA_E_OK;
}

ca_status_t aal_ni_dma_lso_dump_aft_l2fib(int index)
{
	DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
	DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
	DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	if (aal_ni_dma_lso_get_aft_l2fib(index, &data0, &data1, &data2) != CA_E_OK)
		return CA_E_ERROR;

	PRINTF("[index=%d]\n", index);
	PRINTF("inner_vid[5:0]=%d, inner_dei=%d, inner_dei_sel=%d, inner_802_1p=%d, inner_1p_sel=%d, pppoe_cmd=%d, session_id=%d\n",
			data0.bf.inner_vid, data0.bf.inner_dei, data0.bf.inner_dei_sel, data0.bf.inner_802_1p,
			data0.bf.inner_1p_sel, data0.bf.pppoe_cmd, data0.bf.session_id);

	PRINTF("top_tpid_sel[0]=%d, top_vid=%d, top_dei=%d, top_dei_sel=%d, top_802_1p=%d, top_1p_sel=%d\n",
			data1.bf.top_tpid_sel, data1.bf.top_vid, data1.bf.top_dei, data1.bf.top_dei_sel, data1.bf.top_802_1p, data1.bf.top_1p_sel);
	PRINTF("inner_tpid_enc=%d, inner_tpid_sel=%d, inner_vid[11:6]=%d\n",
			data1.bf.inner_tpid_enc, data1.bf.inner_tpid_sel, data1.bf.inner_vid);

	PRINTF("vlan_vld=%d, vlan_cnt=%d, inner_vlan_cmd=%d, top_tpid_enc=%d, top_tpid_sel[1]=%d\n",
			data2.bf.vlan_vld, data2.bf.vlan_cnt, data2.bf.inner_vlan_cmd, data2.bf.top_tpid_enc, data2.bf.top_tpid_sel);

	return CA_E_OK;
}

ca_status_t aal_ni_dma_lso_clean_aft_l2fib(int index)
{
        DMA_SEC_DMA_AFT_L2FIB_DATA0_t   data0;
        DMA_SEC_DMA_AFT_L2FIB_DATA1_t   data1;
        DMA_SEC_DMA_AFT_L2FIB_DATA2_t   data2;

	data0.wrd = data1.wrd = data2.wrd = 0;
	aal_ni_dma_lso_set_aft_l2fib(index, data0, data1, data2);

        return CA_E_OK;
}
//EXPORT_SYMBOL(aal_ni_dma_lso_clean_aft_l2fib);

ca_status_t aal_ni_dma_lso_get_streamid_tbl(ca_uint16_t index, aal_ni_dma_lso_stream_id_tbl_cfg_t *cfg)
{
	int i;

	DMA_SEC_DMA_INDIR_TBL_ACCESS_STREAMID_TBL_CTRL_DATA_t   data;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_STREAMID_TBL_CTRL_ACCESS_t access;

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.rbw = 0;
	access.bf.address = index;
	CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_STREAMID_TBL_CTRL_ACCESS);

	/* wait for complete */
	for (i = 0; i < 100; i++) {
		access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_STREAMID_TBL_CTRL_ACCESS);
		if (access.bf.access == 0)
			break;
	}
	if (i == 100) {
		PRINTF("%s: [ATTENTION] indirect access write not finished!!!\n", __func__);
	}

	data.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_STREAMID_TBL_CTRL_DATA);
	cfg->en_flag = data.bf.en_flag;
	cfg->ldpid = data.bf.ldpid;
	cfg->cos = data.bf.cos;
	cfg->pol_id = data.bf.pol_id;
	cfg->deep_q = data.bf.deep_q;

	return CA_E_OK;
}
ca_status_t aal_ni_dma_lso_set_streamid_tbl(ca_uint16_t index, aal_ni_dma_lso_stream_id_tbl_cfg_t *cfg)
{
	int i;

	DMA_SEC_DMA_INDIR_TBL_ACCESS_STREAMID_TBL_CTRL_DATA_t   data;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_STREAMID_TBL_CTRL_ACCESS_t access;

	data.wrd = 0;
	data.bf.deep_q = cfg->deep_q;
	data.bf.pol_id = cfg->pol_id;
	data.bf.cos = cfg->cos;
	data.bf.ldpid = cfg->ldpid;
	data.bf.en_flag = cfg->en_flag;
	CA_DMA_LSO_REG_WRITE(data.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_STREAMID_TBL_CTRL_DATA);

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.rbw = 1;
	access.bf.address = index;
	CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_STREAMID_TBL_CTRL_ACCESS);

	/* wait for complete */
	for (i = 0; i < 100; i++) {
		access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_STREAMID_TBL_CTRL_ACCESS);
		if (access.bf.access == 0)
			break;
	}
	if (i == 100) {
		PRINTF("%s: [ATTENTION] indirect access write not finished!!!\n", __func__);
	}

    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_ni_dma_lso_get_streamid_tbl);
//EXPORT_SYMBOL(aal_ni_dma_lso_set_streamid_tbl);

ca_status_t aal_ni_dma_lso_get_lspid_map_tbl(ca_uint16_t index, aal_ni_dma_lso_lspid_map_tbl_cfg_t *cfg)
{
	int i;

	DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_DATA0_t   data0;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_DATA1_t   data1;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_ACCESS_t access;

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.rbw = 0;
	access.bf.address = index;
	CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_ACCESS);

	/* wait for complete */
	for (i = 0; i < 100; i++) {
		access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_ACCESS);
		if (access.bf.access == 0)
			break;
	}
	if (i == 100) {
		PRINTF("%s: [ATTENTION] indirect access write not finished!!!\n", __func__);
	}

	data0.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_DATA0);
	data1.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_DATA1);
	cfg->en_flag = data1.bf.en_flag;
	cfg->lspid = data1.bf.lspid;
	cfg->fbm_en = data1.bf.fbm_en;
	cfg->fbm_pool_id = data1.bf.fbm_pool_id;
	cfg->fbm_buf_mask = (((ca_uint64_t)data1.bf.fbm_mask) << 32) | data0.bf.fbm_mask;

	return CA_E_OK;
}
ca_status_t aal_ni_dma_lso_set_lspid_map_tbl(ca_uint16_t index, aal_ni_dma_lso_lspid_map_tbl_cfg_t *cfg)
{
	int i;

	DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_DATA0_t   data0;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_DATA1_t   data1;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_ACCESS_t access;

	data0.wrd = 0;
	data0.bf.fbm_mask = (cfg->fbm_buf_mask & 0xffffffff);	//FBM mask[31:0]
	CA_DMA_LSO_REG_WRITE(data0.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_DATA0);
	data1.wrd = 0;
	data1.bf.lspid = cfg->lspid;
	data1.bf.fbm_en = cfg->fbm_en;
	data1.bf.fbm_pool_id = cfg->fbm_pool_id;
	data1.bf.fbm_mask = (cfg->fbm_buf_mask >> 32);			//FBM mask[39:32]
	data1.bf.en_flag = cfg->en_flag;
	CA_DMA_LSO_REG_WRITE(data1.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_DATA1);

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.rbw = 1;
	access.bf.address = index;
	CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_ACCESS);

	/* wait for complete */
	for (i = 0; i < 100; i++) {
		access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_LSPID_MAP_TBL_CTRL_ACCESS);
		if (access.bf.access == 0)
			break;
	}
	if (i == 100) {
		PRINTF("%s: [ATTENTION] indirect access write not finished!!!\n", __func__);
	}

    return CA_E_OK;
}
ca_status_t aal_ni_dma_lso_add_lspid_map_tbl(ca_uint16_t *index, aal_ni_dma_lso_lspid_map_tbl_cfg_t *cfg)
{
	ca_status_t ret = CA_E_OK;
	int i = 0;
	int first_invalid_entry = CA_NI_DMA_LSO_LSPID_MAP_TBL_SIZE;
	aal_ni_dma_lso_lspid_map_tbl_cfg_t entry = {0};

	cfg->en_flag = 1;

	for (i = 0; i < CA_NI_DMA_LSO_LSPID_MAP_TBL_SIZE; i++) {
		if (aal_ni_dma_lso_get_lspid_map_tbl(i, &entry) != CA_E_OK)
			continue;
		if(memcmp(&entry, cfg, sizeof(aal_ni_dma_lso_lspid_map_tbl_cfg_t)) == 0){
			*index = i;
			return ret;
		}
		if((first_invalid_entry == CA_NI_DMA_LSO_LSPID_MAP_TBL_SIZE) && (entry.en_flag == 0))
			first_invalid_entry = i;
	}

	if(first_invalid_entry < CA_NI_DMA_LSO_LSPID_MAP_TBL_SIZE){
		ret = aal_ni_dma_lso_set_lspid_map_tbl(first_invalid_entry, cfg);
		*index = first_invalid_entry;
	}else
		ret = CA_E_ERROR;

	return ret;
}
ca_status_t aal_ni_dma_lso_del_lspid_map_tbl(ca_uint16_t index)
{
	ca_status_t ret;
	aal_ni_dma_lso_lspid_map_tbl_cfg_t entry = {0};

#if defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
	if(index < NITX_LSPID_MAP_ARM14)
		PRINTF("%s: [ATTENTION] Please review dma lso lspid mapping table reserve usage!!!\n", __func__);
#endif

	ret = aal_ni_dma_lso_set_lspid_map_tbl(index , &entry);

	return ret;
}
//EXPORT_SYMBOL(aal_ni_dma_lso_get_lspid_map_tbl);
//EXPORT_SYMBOL(aal_ni_dma_lso_set_lspid_map_tbl);
//EXPORT_SYMBOL(aal_ni_dma_lso_add_lspid_map_tbl);
//EXPORT_SYMBOL(aal_ni_dma_lso_del_lspid_map_tbl);

ca_status_t aal_ni_dma_lso_get_dmaaft_map_tbl(ca_uint16_t index, aal_ni_dma_lso_dmaaft_map_tbl_cfg_t *cfg)
{
	int i;

	DMA_SEC_DMA_INDIR_TBL_ACCESS_DMAAFT_MAP_TBL_CTRL_DATA_t   data;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_DMAAFT_MAP_TBL_CTRL_ACCESS_t access;

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.rbw = 0;
	access.bf.address = index;
	CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_DMAAFT_MAP_TBL_CTRL_ACCESS);

	/* wait for complete */
	for (i = 0; i < 100; i++) {
		access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_DMAAFT_MAP_TBL_CTRL_ACCESS);
		if (access.bf.access == 0)
			break;
	}
	if (i == 100) {
		PRINTF("%s: [ATTENTION] indirect access write not finished!!!\n", __func__);
	}

	data.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_DMAAFT_MAP_TBL_CTRL_DATA);
	cfg->en_flag = data.bf.en_flag;
	cfg->lspid_map = data.bf.lspid_remap;
	cfg->dmaaft_en = data.bf.dmaaft_en;
	cfg->dmaaft_fib = data.bf.dmaaft_fib;

	return CA_E_OK;
}
ca_status_t aal_ni_dma_lso_set_dmaaft_map_tbl(ca_uint16_t index, aal_ni_dma_lso_dmaaft_map_tbl_cfg_t *cfg)
{
	int i;

	DMA_SEC_DMA_INDIR_TBL_ACCESS_DMAAFT_MAP_TBL_CTRL_DATA_t   data;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_DMAAFT_MAP_TBL_CTRL_ACCESS_t access;

	data.wrd = 0;
	data.bf.lspid_remap = cfg->lspid_map;
	data.bf.dmaaft_en = cfg->dmaaft_en;
	data.bf.dmaaft_fib = cfg->dmaaft_fib;
	data.bf.en_flag = cfg->en_flag;
	CA_DMA_LSO_REG_WRITE(data.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_DMAAFT_MAP_TBL_CTRL_DATA);

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.rbw = 1;
	access.bf.address = index;
	CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_DMAAFT_MAP_TBL_CTRL_ACCESS);

	/* wait for complete */
	for (i = 0; i < 100; i++) {
		access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_DMAAFT_MAP_TBL_CTRL_ACCESS);
		if (access.bf.access == 0)
			break;
	}
	if (i == 100) {
		PRINTF("%s: [ATTENTION] indirect access write not finished!!!\n", __func__);
	}

    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_ni_dma_lso_get_dmaaft_map_tbl);
//EXPORT_SYMBOL(aal_ni_dma_lso_set_dmaaft_map_tbl);

ca_status_t aal_ni_dma_lso_get_fbm_mask_tbl(ca_uint16_t index, aal_ni_dma_lso_fbm_mask_tbl_cfg_t *cfg)
{
	int i;

	DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_DATA0_t   data0;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_DATA1_t   data1;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_ACCESS_t access;

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.rbw = 0;
	access.bf.address = index;
	CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_ACCESS);

	/* wait for complete */
	for (i = 0; i < 100; i++) {
		access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_ACCESS);
		if (access.bf.access == 0)
			break;
	}
	if (i == 100) {
		PRINTF("%s: [ATTENTION] indirect access write not finished!!!\n", __func__);
	}

	data0.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_DATA0);
	data1.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_DATA1);
	cfg->fbm_buf_mask = (((ca_uint64_t)data1.bf.fbm_mask) << 32) | data0.bf.fbm_mask;

	return CA_E_OK;
}
ca_status_t aal_ni_dma_lso_set_fbm_mask_tbl(ca_uint16_t index, aal_ni_dma_lso_fbm_mask_tbl_cfg_t *cfg)
{
	int i;

	DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_DATA0_t   data0;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_DATA1_t   data1;
	DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_ACCESS_t access;

	data0.wrd = 0;
	data0.bf.fbm_mask = (cfg->fbm_buf_mask & 0xffffffff);	//FBM mask[31:0]
	CA_DMA_LSO_REG_WRITE(data0.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_DATA0);
	data1.wrd = 0;
	data1.bf.fbm_mask = (cfg->fbm_buf_mask >> 32);			//FBM mask[39:32]
	CA_DMA_LSO_REG_WRITE(data1.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_DATA1);

	access.wrd = 0;
	access.bf.access = 1;
	access.bf.rbw = 1;
	access.bf.address = index;
	CA_DMA_LSO_REG_WRITE(access.wrd, DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_ACCESS);

	/* wait for complete */
	for (i = 0; i < 100; i++) {
		access.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_INDIR_TBL_ACCESS_FBM_ADDR_MASK_TBL_CTRL_ACCESS);
		if (access.bf.access == 0)
			break;
	}
	if (i == 100) {
		PRINTF("%s: [ATTENTION] indirect access write not finished!!!\n", __func__);
	}

    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_ni_dma_lso_get_fbm_mask_tbl);
//EXPORT_SYMBOL(aal_ni_dma_lso_set_fbm_mask_tbl);

ca_status_t aal_ni_dma_lso_get_dmaaft_dbg(ca_uint32_t index, ca_uint32_t *value)
{
	DMA_SEC_DMA_AFT_FPGA_DBG_SEL_t idx;

	idx.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_FPGA_DBG_SEL);
	idx.bf.idx = index;
	CA_DMA_LSO_REG_WRITE(idx.wrd, DMA_SEC_DMA_AFT_FPGA_DBG_SEL);

	*value = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_AFT_FPGA_DBG);

    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_ni_dma_lso_get_dmaaft_dbg);

ca_status_t aal_ni_dma_lso_get_wifi_divide_dbg(ca_uint32_t index, ca_uint32_t *value)
{
	DMA_SEC_DMA_GLB_WIFI_PKT_DIVIDE_CFG_t idx;

	idx.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_GLB_WIFI_PKT_DIVIDE_CFG);
	idx.bf.wifi_dbg_sel = index;
	CA_DMA_LSO_REG_WRITE(idx.wrd, DMA_SEC_DMA_GLB_WIFI_PKT_DIVIDE_CFG);

	*value = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_GLB_WIFI_PKT_DEBUG);

    return CA_E_OK;
}
//EXPORT_SYMBOL(aal_ni_dma_lso_get_wifi_divide_dbg);



