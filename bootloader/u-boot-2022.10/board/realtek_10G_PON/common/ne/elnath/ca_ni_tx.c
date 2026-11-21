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
 * ca_ni_tx.c: Cortina NI Tx handler
 *
 */
#include <osal_soc.h>
#include <osal_type.h>
#include <osal.h>
#include <aal_common.h>
#include <aal_ni.h>
#include <aal_scfg.h>


static void ca_ni_dma_lso_init_startup_config(void)
{
	#if 0
	DMA_SEC_DMA_GLB_CCI_MAP_t glb_cci_map;

	ca_ni_chip_revision = ca_ni_get_chip_revision();
	PRINTF( "%s: ca_ni_chip_revision=%d\n", __func__, ca_ni_chip_revision);

	aal_scfg_read(CFG_ID_DMA_LSO_ACE_TEST, 1, &dma_lso_ace_test);
	glb_cci_map.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_GLB_CCI_MAP);
	if(dma_lso_ace_test){
		PRINTF( "%s: This chip not support DMALSO ACE enable!!\n", __func__);
		glb_cci_map.bf.cci_map = 2;
	}else{
		glb_cci_map.bf.cci_map = 0;
	}
	CA_DMA_LSO_REG_WRITE(glb_cci_map.wrd, DMA_SEC_DMA_GLB_CCI_MAP);
	PRINTF( "%s: aal_scfg_read(CFG_ID_DMA_LSO_ACE_TEST), dma_lso_ace_test=%d\n", __func__, dma_lso_ace_test);

	aal_scfg_read(CFG_ID_NE_CA_TX_BY_START_XMIT, 1, &ca_tx_by_start_xmit);
	PRINTF( "%s: aal_scfg_read(CFG_ID_NE_CA_TX_BY_START_XMIT), ca_tx_by_start_xmit=%d\n", __func__, ca_tx_by_start_xmit);

        aal_scfg_read(CFG_ID_PON_VOQ_MODE, 1, &puc_voq_mode);
        PRINTF( "%s: aal_scfg_read(CFG_ID_PON_VOQ_MODE), puc_voq_mode=%d\n", __func__, puc_voq_mode);
	#else
	DMA_SEC_DMA_GLB_CCI_MAP_t glb_cci_map;

	#if 0
	aal_scfg_read(CFG_ID_DMA_LSO_ACE_TEST, 1, &dma_lso_ace_test);
	#endif
	ca_uint8_t dma_lso_ace_test = CFG_ID_DMA_LSO_ACE_TEST;
	glb_cci_map.wrd = CA_DMA_LSO_REG_READ(DMA_SEC_DMA_GLB_CCI_MAP);
	if(dma_lso_ace_test){
		PRINTF( "%s: This chip not support DMALSO ACE enable!!\n", __func__);
		glb_cci_map.bf.cci_map = 2;
	}else{
		glb_cci_map.bf.cci_map = 0;
	}
	CA_DMA_LSO_REG_WRITE(glb_cci_map.wrd, DMA_SEC_DMA_GLB_CCI_MAP);
	#endif
}

#if 0
void ca_ni_init_tx_dma_lso(struct platform_device *pdev)
#else
void ca_ni_init_tx_dma_lso(void)
#endif
{
	#if 0
	int i, j, wptr;
	ni_info_t *ni =	&ni_info_data;
	u32 rptr_addr, wptr_addr;
	u32 dma_tx_base_addr;
	dma_addr_t tx_desc_paddr[CA_NI_DMA_LSO_TXQ_NUM];
	dma_txdesc_t *tx_desc[CA_NI_DMA_LSO_TXQ_NUM];
	dma_addr_t tx_buf_paddr[CA_NI_DMA_LSO_TXQ_NUM];
	dma_txbuf_t *tx_buf[CA_NI_DMA_LSO_TXQ_NUM];
	dma_rptr_t rptr_reg;
	dma_txdesc_t *curr_desc_reset;
	u8 skip_mem_alloc = FALSE;

	/* because dma_alloc_coherent() need a non-NULL device */
	struct device *dev = &(pdev->dev);

	spin_lock_init(&ca_ni_send_packet_lock);

	ca_ni_dma_lso_init_startup_config();

	/* There are 12 virtual ports for DMA LSO, 4 for CPUs linux kernel, 4 for PEs */
	/* each virtual port has NI_DMA_LSO_TXQ_NUM=8 queues */
	for (j = 0; j < ca_ni_dma_lso_cpu_vp_num; j++){

		dma_tx_base_addr = DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + (j * DMA_SEC_DMA_LSO_VP_STRIDE);
		rptr_addr = DMA_SEC_DMA_LSO_VP_TXQ0_RPTR + (j * DMA_SEC_DMA_LSO_VP_STRIDE);
		wptr_addr = DMA_SEC_DMA_LSO_VP_TXQ0_WPTR + (j * DMA_SEC_DMA_LSO_VP_STRIDE);

		skip_mem_alloc = FALSE;
		for(i = 0; i< CA_NI_CPU_CORE_END; i++) {
			if(j == ca_ni_dmalso_vp_sel[i])
				break;
		}

		if((i == CA_NI_CPU_CORE_END) || (i > CA_NI_CPU_CORE_ARM_END))
			skip_mem_alloc = TRUE;

#if !defined(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE)
			skip_mem_alloc = FALSE;
#endif

		for (i = 0; i < CA_NI_DMA_LSO_TXQ_NUM; i++) {

			if(skip_mem_alloc) {
				tx_buf[i] = NULL;
				tx_buf_paddr[i] = 0;
			}else {
				if(dma_lso_ace_test) {		
					tx_buf[i] = ca_malloc_trace(CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txbuf_t), GFP_KERNEL|GFP_DMA, true);
					tx_buf_paddr[i] = virt_to_phys(tx_buf[i]);
				}else {
					tx_buf[i] = dma_alloc_coherent(dev, CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txbuf_t), &tx_buf_paddr[i], GFP_KERNEL|GFP_DMA);
					if (!tx_buf[i]) {
						PRINTF("%s: TX %d dma_alloc_coherent() for tx_buf fail !!!\n", __func__, i);
						dma_free_coherent(dev, CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txbuf_t), tx_buf, tx_buf_paddr[i]);
						return;
					}
				}
			}

			ni->swtxq[j][i].buf_base = tx_buf[i];
			ni->swtxq[j][i].buf_dma_addr = tx_buf_paddr[i];

			if(skip_mem_alloc) {
				tx_desc[i] = NULL;
				tx_desc_paddr[i] = 0;
			}else {
				if(dma_lso_ace_test) {
					if(l3qm_dmalso_sram_enable[j] & (1 << i)) {
						
						tx_desc_paddr[i] = (sram_start_phy_addr+sram_cur_used_size);
						tx_desc[i] = (dma_txdesc_t *) sram_cur_virtual_addr;
						
						PRINTF("dmalso txring[%d][%d] sramPhyStart = 0x%x, sramvirStart = 0x%llx, tx_dexc = %p\n", j, i, (sram_start_phy_addr+sram_cur_used_size), sram_cur_virtual_addr, tx_desc[i]);
						
						sram_cur_used_size += TXDESCSIZE_PER_VOQ;
						sram_cur_virtual_addr += TXDESCSIZE_PER_VOQ;
					} else {
						tx_desc[i] = ca_malloc_trace(CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txdesc_t), GFP_KERNEL|GFP_DMA, true);
						tx_desc_paddr[i] = virt_to_phys(tx_desc[i]);
					}
				}else {
					tx_desc[i] = dma_alloc_coherent(dev, CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txdesc_t), &tx_desc_paddr[i], GFP_KERNEL|GFP_DMA);
					if (!tx_desc[i]) {
						PRINTF("%s: TX %d dma_alloc_coherent() for tx_desc fail !!!\n", __func__, i);
						dma_free_coherent(dev, CA_NI_DMA_LSO_TXDESC_NUM * sizeof(dma_txdesc_t), tx_desc, tx_desc_paddr[i]);
						return;
					}
				}
			}

			ni->swtxq[j][i].rptr_reg = rptr_addr + (8 * i);      /* the stride to next queue is 8 */
			ni->swtxq[j][i].wptr_reg = wptr_addr + (8 * i);	     /* the stride to next queue is 8 */
			rptr_reg.wrd = aal_ni_get_tx_read_ptr(ni->swtxq[j][i].rptr_reg);
			if(rptr_reg.bf.rptr!=0)
			{
				PRINTF("[WARNING]initial dma lso vp %d q %d read pointer is not 0",j,i);
			}
			ni->swtxq[j][i].rptr = rptr_reg.bf.rptr;
			ni->swtxq[j][i].rptr_prev = 0;
			ni->swtxq[j][i].rptr_rep_times = 0;
			
			ni->swtxq[j][i].wptr = 0;
			ni->swtxq[j][i].finished_idx = 0;
			ni->swtxq[j][i].total_desc_num = skip_mem_alloc ? 0 : CA_NI_DMA_LSO_TXDESC_NUM;
			ni->swtxq[j][i].skb_recycle_per_pkt_cnt = 0;

			spin_lock_init(&ni->swtxq[j][i].lock);


			/* the buffer address points to LSO para+Header A */
			ni->swtxq[j][i].desc_base = tx_desc[i];
			ni->swtxq[j][i].desc_dma_addr = tx_desc_paddr[i];
			if(tx_desc[i]) {
				tx_desc[i]->word0.wrd = tx_buf_paddr[i];

				//reset buffer length prevent invalid buffer address cause debug dump call trace
				for (wptr = 0; wptr < ni->swtxq[j][i].total_desc_num; wptr++) {
					curr_desc_reset = ni->swtxq[j][i].desc_base + wptr;
					if(curr_desc_reset){
						curr_desc_reset->word1.wrd = 0;
					}
				}
			}

			aal_ni_set_dma_lso_base_depth_addr(j, i, tx_desc_paddr[i]);

			if (ca_ni_debug & NI_DBG_DUMP_TX) {
				PRINTF("%s: ni->swtxq[%d][%d].buf_base=%p, ni->swtxq[%d][%d].buf_dma_addr=%p\n",
						__func__, j, i, ni->swtxq[j][i].buf_base, j, i, &ni->swtxq[j][i].buf_dma_addr);
				PRINTF("%s: ni->swtxq[%d][%d].desc_base=%p, ni->swtxq[%d][%d].desc_dma_addr=%p\n",
						__func__, j, i, ni->swtxq[j][i].desc_base, j, i, &ni->swtxq[j][i].desc_dma_addr);
			}
		}
	}

	ca_ni_init_tx_dma_lso_interrupts(pdev);

#if defined(CONFIG_REALTEK_IPC2RCPU)
	ca_ni_init_tx_dma_lso_ipc();
#endif
	#else
	ca_ni_dma_lso_init_startup_config();
	#endif
}