///--
#include <nic.h>

#define compiler_barrier()

/* ne variable */
static uint32 ni_pkt_buf_len;
static uint32 ni_pkt_buf_mask;
static uint32 ni_buf_phy_addr;
static uint32 ni_buf_size;
static uint32 ni_rx_desc_size;
static uint32 ni_tx_desc_size_depth;
static uint32 virt_txq_dma_desc;
static uint32 virt_txq_dma_desc_end;

/* sw pool variable */
static uint32 ni_pktbuf_cnt = 0;
static uint32 ni_pktbuf_pool[PE_NI_HAL_PKT_BUF_COUNT] = {0};

/* nic management var */
static rtk_ni_intf_t g_ni_intf;

//sw pool operation
static void rtk_ni_pktbuf_free(uint8 *p_pkt_buf)
{
	ni_pktbuf_pool[ni_pktbuf_cnt] = p_pkt_buf;
	ni_pktbuf_cnt++;
}

static uint8 *rtk_ni_pktbuf_get()
{
	if (ni_pktbuf_cnt == 0) {
		return NULL;
	}

	ni_pktbuf_cnt--;
	return (uint8 *)ni_pktbuf_pool[ni_pktbuf_cnt];
}

//refill
static uint8 rtk_ni_feed_buff_cnt(int refill_cnt)
{
	uint8 *p_pkt_buf;
	uint8 ret = RE_OK;
	int i;
	//skip check (PE_NI_HAL_CHECK_EQM_READY_PUSH(PE_NI_HAL_CPU_EQID0))
	for (i = 0; i < refill_cnt; i++) {
		p_pkt_buf = rtk_ni_pktbuf_get();
		if (unlikely(p_pkt_buf == NULL)) {
			//PRINTF("no buf\n");
			ret = RE_ERR_BUSY;
			break;
		}

		PE_NI_HAL_QM_PUSH_PHY_ADDR(PE_NI_HAL_CPUPORT_ID, PE_NI_HAL_CPU_EQID0, VIRT_TO_PHYS(p_pkt_buf));
		g_ni_intf.allocated_buff++;			
	}
	return ret;
}

static uint8 rtk_ni_rx_buff_init()
{
	int i;
	uint8 *p_pkt_buf = 0;

	g_ni_intf.allocated_buff = 0;
	p_pkt_buf = (uint8 *)PE_NI_HAL_PKT_BUF_BASE;

	PRINTF("p_pkt_buf base %p PE_NI_HAL_PKT_BUF_BASE %p\n", p_pkt_buf, (PE_NI_HAL_PKT_BUF_BASE));
	for (i = 0; i < PE_NI_HAL_PKT_BUF_COUNT; i++)		
	{
		rtk_ni_pktbuf_free(p_pkt_buf);
		p_pkt_buf += PE_NI_HAL_PKT_BUF_LEN;
	}

	//refill to eq pool 2496
	rtk_ni_feed_buff_cnt(PE_NI_HAL_EQ_POOL_SIZE);
	return 0;
}

/* nic init */
static ca_uint8_t __l3qm_get_buffer_size_index(ca_uint32_t buffer_size)
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
		case 16383: /* mercury */
			index = 7;
			break;
		default:
			PRINTF("%s: buffer_size=%d is not defined!!\n", __func__, buffer_size);
			index = 0;
			break;
	}
	return index;
}

static void rtk_nic_init()
{
	uint32 value;
	uint32 i, reg_off;
	DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS_t dma_access;
	QM_QM_EQ_PROFILE0_t eq_profile;
	QM_QM_DEST_PORT0_EQ_CFG_t dest_port_eq_cfg;
	QM_QM_CFG0_EQ0_t cfg0_eq;
	QM_QM_CFG1_EQ0_t cfg1_eq;
	QM_QM_CFG2_EQ0_t cfg2_eq;
	DMA_SEC_DMA_LSO_VP_HDRA_CFG_t	hdra_cfg;
	dma_txdesc_t *curr_desc;

	/*=================== For RX ==========================*/
	/*Configure#2 for EQ, set buffer size, FBM mode*/
	cfg2_eq.wrd = RTK_NE_REG_READ(QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * PE_NI_HAL_CPU_EQID0));
	cfg2_eq.bf.buffer_size = __l3qm_get_buffer_size_index(PE_NI_HAL_PKT_BUF_LEN);
	RTK_NE_REG_WRITE(QM_QM_CFG2_EQ0 + (QM_QM_CFG2_EQ0_STRIDE * PE_NI_HAL_CPU_EQID0), cfg2_eq.wrd);

	/*Configure#4 for EQ, axi_top 0, 0x4 : ACE, 0x1 Non-Cache
	 */
	RTK_NE_REG_WRITE(QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * PE_NI_HAL_CPU_EQID0), PE_NI_HAL_EQ_POOL0_CFG4);
	RTK_NE_REG_WRITE(QM_QM_CFG4_EQ0 + (QM_QM_CFG4_EQ0_STRIDE * PE_NI_HAL_CPU_EQID1), PE_NI_HAL_EQ_POOL1_CFG4); 
	/* ==> need to align with Configure#4*/
	QM_QM_AXI_ATTRIBUTE_ACCESS_t	attrib_cus;
	QM_QM_AXI_ATTRIBUTE_DATA0_t	attrib_data0_cus;

	attrib_data0_cus.wrd = PE_NI_HAL_EQ_POOL0_AXI_ATTRIB;
	RTK_NE_REG_WRITE(QM_QM_AXI_ATTRIBUTE_DATA0, attrib_data0_cus.wrd);
	attrib_cus.bf.access = 1;
	attrib_cus.bf.rbw = 1;
	attrib_cus.bf.ADDR = PE_NI_HAL_CPU_EQID0;
	RTK_NE_REG_WRITE(QM_QM_AXI_ATTRIBUTE_ACCESS, attrib_cus.wrd);
	
	attrib_data0_cus.wrd = PE_NI_HAL_EQ_POOL1_AXI_ATTRIB;
	RTK_NE_REG_WRITE(QM_QM_AXI_ATTRIBUTE_DATA0, attrib_data0_cus.wrd);
	attrib_cus.bf.access = 1;
	attrib_cus.bf.rbw = 1;
	attrib_cus.bf.ADDR = PE_NI_HAL_CPU_EQID1;
	RTK_NE_REG_WRITE(QM_QM_AXI_ATTRIBUTE_ACCESS, attrib_cus.wrd);

	/* Set profile setting of EPP0 FIFO  */
	{
		value = QM_QM_CPU_EPP_FIFO_CFG_profile0_dft & ~(0x1FFFFF<<8);
		value |= ((PE_NI_HAL_EPP0_DESC_CNT_PER_EPP/4)&0x1FFFFF)<<8;
		RTK_NE_REG_WRITE(QM_QM_CPU_EPP_FIFO_CFG_profile0 + (QM_QM_CPU_EPP_FIFO_CFG_profile0_STRIDE * PE_NI_HAL_EPP_PROFILE_ID),
			value);
	}

	/*Set QM RX first and rest headroom*/
	PRINTF("RTK_NI_DEFAULT_RX_HEADROOM %d \n", RTK_NI_DEFAULT_RX_HEADROOM);
	value = RTK_NI_DEFAULT_RX_HEADROOM / 16;
	RTK_NE_REG_WRITE(QM_QM_DEST_PORT0_PKT_BUF_CFG + (QM_QM_DEST_PORT0_PKT_BUF_CFG_STRIDE * PE_NI_HAL_CPUPORT_ID), value);

	/* Set profile index of EPP0 FIFO  */
	for (i = 0; i < COS_NUM; i++) {
		RTK_NE_REG_WRITE(QM_QM_CPU_EPP_FIFO0_0_CFG + (QM_QM_CPU_EPP_FIFO0_0_CFG_STRIDE * PE_NI_HAL_CPUPORT_ID) + (4 * i),
			PE_NI_HAL_EPP_PROFILE_ID);
	}

	/* Set EPP0 FIFO start addr */
	value = PE_NI_HAL_EPP0_BASE;
	for (i = 0; i < COS_NUM; i++) {
		RTK_NE_REG_WRITE(QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START +
			QM_QM_CPUEPP_POINTER_CPUEPP64_FIFO_PADDR_START_STRIDE * (PE_NI_HAL_CPUPORT_ID * COS_NUM + i),
			value);
		value += (PE_NI_HAL_EPP0_DESC_CNT_PER_EPP * PE_NI_HAL_EPP0_DESC_PER_EPP * PE_NI_HAL_EPP0_DESC_SIZE);
	}

	/* set sw epp base */
	g_ni_intf.ni_epp_base =(uint32) PHYS_TO_VIRT(PE_NI_HAL_EPP0_BASE);

	/* Confiure interrupts - disable arm interrupt */
	RTK_NE_REG_WRITE(QM_QM_EPP64_P3_0_INTERRUPT_EN, 0x00FFFFFF);

	/*=================== For TX ==========================*/
	/*TX without LSO & HEADER A, default header a by VP*/
	hdra_cfg.wrd = RTK_NE_REG_READ(DMA_SEC_DMA_LSO_VP_HDRA_CFG + PE_NI_HAL_DMA_LSO_CPU_VP_ID * DMA_SEC_DMA_LSO_VP_STRIDE);
	hdra_cfg.bf.hdra_ldpid = PE_NI_HAL_TX_TEST_DEFAULT_LDPID;
	hdra_cfg.bf.hdra_febypass = 1;
	RTK_NE_REG_WRITE(DMA_SEC_DMA_LSO_VP_HDRA_CFG + PE_NI_HAL_DMA_LSO_CPU_VP_ID * DMA_SEC_DMA_LSO_VP_STRIDE, hdra_cfg.wrd);

	/* set tx dma desc base addr */
	virt_txq_dma_desc = PHYS_TO_VIRT(PE_NI_HAL_TXQ_DMA_DESC_PHYS);
	virt_txq_dma_desc_end = virt_txq_dma_desc + sizeof(dma_txdesc_t) * PE_NI_HAL_TXQ_DMA_DESC_COUNT;

	{
		/*=================== For TX ==========================*/
		/* Set DMA_LSO_VP_TXQ_BASE_DEPTH					   */
		/* base address bit [4:31] should be 16-byte alignment */
		/* now only use 1 TXQ								   */
		value = (PE_NI_HAL_TXQ_DMA_DESC_PHYS + PE_NI_HAL_TXQ_DMA_DESC_DEPTH); 
		RTK_NE_REG_WRITE(DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA0 + 
			(DMA_SEC_DMA_LSO_VP_STRIDE * PE_NI_HAL_DMA_LSO_CPU_VP_ID), value);	
		RTK_NE_REG_WRITE(DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_DATA1 + 
			(DMA_SEC_DMA_LSO_VP_STRIDE * PE_NI_HAL_DMA_LSO_CPU_VP_ID), 0);
		dma_access.wrd = 0;
		dma_access.bf.txq_id = PE_NI_HAL_TXQ_DMA_TXQ_ID;
		dma_access.bf.rbw = 1;
		dma_access.bf.access = 1;
		RTK_NE_REG_WRITE(DMA_SEC_DMA_LSO_VP_TXQ_BASE_DEPTH_ACCESS + 
			(DMA_SEC_DMA_LSO_VP_STRIDE * PE_NI_HAL_DMA_LSO_CPU_VP_ID), dma_access.wrd);
	}

	/* fill tx desc */
	for(i = 0; i < PE_NI_HAL_TXQ_DMA_DESC_COUNT; i++)
	{
		curr_desc = (dma_txdesc_t *) ((uint32) virt_txq_dma_desc + PE_NI_HAL_TXQ_DMA_DESC_SIZEOF * i);
		curr_desc->word0.wrd = curr_desc->word1.wrd = 0;
		/*optimize code for not using lso & header_a*/
		curr_desc->word1.bf_direct_tx_lan.dest = PE_NI_HAL_TX_TEST_DEFAULT_LDPID;
		curr_desc->word1.bf_direct_tx_lan.cos = 0;
		curr_desc->word1.bf_direct_tx_lan.chk_sel = 1;	//auto
		curr_desc->word1.bf_direct_tx_lan.mode = 1; 	//direct tx
		curr_desc->word1.bf_direct_tx_lan.direct = 0;	//to lan
		curr_desc->word1.bf_direct_tx_lan.aft_fib_map = 0;
		curr_desc->word1.bf_direct_tx_lan.hp0 = curr_desc->word1.bf_direct_tx_lan.hp1 = 1;
	}
	PRINTF("phys_txq_dma_desc=0x%x virt_txq_dma_desc=0x%x virt_txq_dma_desc_end=0x%x PE_NI_HAL_TXQ_DMA_DESC_COUNT=%d\n", PE_NI_HAL_TXQ_DMA_DESC_PHYS, virt_txq_dma_desc, virt_txq_dma_desc_end, PE_NI_HAL_TXQ_DMA_DESC_COUNT);

	/* refill rx buf to sw pool & eq pool */
	rtk_ni_rx_buff_init();
	return;
}

/*
 * Dump the NI information
 */
static void rtk_nic_init_dump(void) {
	uint32 val;
	uint32 phyAddr_end;

	PRINTF("Rx desc settings: \n");
	PRINTF("PE_NI_HAL_EPP0_DESC_CNT_PER_EPP(rx_desc_size per epp) = %d \n", PE_NI_HAL_EPP0_DESC_CNT_PER_EPP );
	phyAddr_end = (PE_NI_HAL_EPP0_BASE+PE_NI_HAL_RE0_EPP0_SIZE*COS_NUM)-1;
	PRINTF("PE_NI_HAL_EPP0_BASE = 0x%x ~ 0x%x , virtual 0x%x\n", PE_NI_HAL_EPP0_BASE, phyAddr_end, g_ni_intf.ni_epp_base );

	PRINTF("Tx desc settings: \n");
	PRINTF("PE_NI_HAL_TXQ_DMA_DESC_COUNT(tx_desc_size per epp) = %d \n", PE_NI_HAL_TXQ_DMA_DESC_COUNT );
	phyAddr_end = (PE_NI_HAL_TXQ_DMA_DESC_PHYS + PE_NI_HAL_TXQ_DMA_DESC_SIZE)-1;
	PRINTF("PE_NI_HAL_TXQ_DMA_DESC_PHYS = 0x%x ~ 0x%x \n", PE_NI_HAL_TXQ_DMA_DESC_PHYS, phyAddr_end);

	PRINTF("PE_NI_TX_PKT_BUT_START = 0x%x, virtual = 0x%x \n", PE_NI_TX_PKT_BUT_START, PHYS_TO_VIRT(PE_NI_TX_PKT_BUT_START));

	PRINTF("Rx pkt buffer settings: \n");
	phyAddr_end = (PE_NI_HAL_RE0_PKT_BUF_BASE + PE_NI_HAL_PKT_BUF_LEN * PE_NI_HAL_PKT_BUF_COUNT)-1;
	PRINTF("PKT_BUF_CNT=%d, physical from(pkt len alignment %u) 0x%x ~ 0x%x, virtual from 0x%x ~ 0x%x\n", 
			PE_NI_HAL_PKT_BUF_COUNT, PE_NI_HAL_PKT_BUF_LEN, PE_NI_HAL_RE0_PKT_BUF_BASE, phyAddr_end, PE_NI_HAL_PKT_BUF_BASE, PE_NI_HAL_PKT_BUF_END-1);

	PRINTF(" allocated_buff %d \n", g_ni_intf.allocated_buff);
	PRINTF(" no_tx_descptor %d \n", g_ni_intf.no_tx_descptor);

	PRINTF(" tx_rd_idx %d \n", (uint32)PE_NI_HAL_TX_DMA_RD_IDX(PE_NI_HAL_TXQ_DMA_TXQ_ID));
	PRINTF("HW tx_wrt_idx %d, SW tx_wrt_idx %d\n", (uint32)PE_NI_HAL_TX_DMA_WR_IDX(PE_NI_HAL_TXQ_DMA_TXQ_ID), (uint32)g_ni_intf.tx_wrt_idx);
	PRINTF(" sw_tx_done_idx %d \n", (uint32)g_ni_intf.sw_tx_done_idx);
}

//nic test
extern void rtk_ne_init(void);
static void rtk_pe_10g_sfu_ni_init(void)
{
	PRINTF("[10F_SFU] ne is initializing ...\n");
	rtk_ne_init();

	PRINTF("[10F_SFU] nic is initializing ...\n");
	ni_buf_phy_addr = RTK_NI_DEFAULT_PKT_BUF_BASE;
	ni_buf_size	= RTK_NI_DEFAULT_PKT_BUF_SIZE;
	PRINTF("ni_buf_phy_addr 0x%x ni_buf_size %u(0x%x)", ni_buf_phy_addr, ni_buf_size, ni_buf_size);
	ni_pkt_buf_len	= RTK_NI_DEFAULT_PKT_BUF_LEN;
	ni_pkt_buf_mask	= RTK_NI_DEFAULT_PKT_BUF_MASK;
	ni_rx_desc_size = RTK_NI_DEFAULT_RX_DESC_SIZE;
	ni_tx_desc_size_depth = RTK_NI_DEFAULT_NI_TX_DESC_SIZE_SHIFT;

	rtk_nic_init();
	rtk_nic_init_dump();

	return RE_OK;
}

static uint8 rtk_pe_ni_rx_wfo_for_10g_sfu(uint32 count, rtk_ni_buf_t *rx_array) 
{
	uint8 cos = 0;
	uint8 *rx_phy_addr;
	uint8 *rx_virt_addr;
	volatile uint32 epp_wt_idx;
	uint32 epp_rd_idx;
	uint32 pkt_len = 0;
	uint8 hdr_len = 0;
	volatile uint32 refill_cnt = 0;
	volatile uint8 *epp_desc_ptr;
	uint32 rx_status1;
	uint32  voq_bit;
	int i;
	cpu_epp_fifo_cmd_64bit_t epp_fifo_cmd_64;
	HEADER_A_T hdr_a = {0};
	uint32 val = 0;

	/*the following are MODE 0 : use EPP0~EPP7*/
	rx_status1 = PE_NI_HAL_GET_EPP_STATUS0;
	voq_bit = 1 << ((PE_NI_HAL_CPUPORT_ID * COS_NUM) + (COS_NUM - 1));

	for (i = 0; i < COS_NUM; i++) {
		if ((rx_status1 & voq_bit) != 0) {
			cos = (COS_NUM - i - 1);
			break;
		}
		voq_bit = voq_bit >> 1;
	}
	if (unlikely(i == COS_NUM)) {
		return RE_ERR_NO_RESOURCE;
	}

	//For init, do not remove
	rx_array->buf_len = 0;
	epp_rd_idx = g_ni_intf.rx_rd_idx[cos];

	epp_wt_idx = (uint32) PE_NI_HAL_RX_EEP_WR_IDX(cos);

	if (unlikely(epp_rd_idx == epp_wt_idx)) {
		return RE_ERR_NO_RESOURCE;
	}
	
	do {
		epp_desc_ptr = (uint8 *) (((uint32)g_ni_intf.ni_epp_base +
					PE_NI_HAL_EPP0_FIFO_SIZE_PER_EPP * cos + epp_rd_idx));

		//inv to prevent arm data prefetch
		DCACHE_INV(epp_desc_ptr, epp_desc_ptr + sizeof(cpu_epp_fifo_cmd_64bit_t));
		epp_fifo_cmd_64 = *((cpu_epp_fifo_cmd_64bit_t *) epp_desc_ptr);
		rx_phy_addr = (uint8 *) (epp_fifo_cmd_64.bits.phy_addr << L3QM_PHY_ADDR_SHIFT);
		rx_virt_addr = PHYS_TO_VIRT(rx_phy_addr);
		g_ni_intf.allocated_buff--;

		//parsing hdr-a to get sport
		DCACHE_INV(rx_virt_addr, rx_virt_addr + sizeof(HEADER_A_T));
		val = *(uint32*)(rx_virt_addr + 4);
		hdr_a.bits32[1] = ENDIAN_CONVERT(val);
		PRINTF_DEBUG("\t[HDR-A] lspid 0x%x pkt_len 0x%x \n", hdr_a.bits.lspid, hdr_a.bits.pkt_size);

		//set rx-array
		rx_array->sport = hdr_a.bits.lspid;
		PRINTF_DEBUG("\trx_array->sport 0x%x \n", rx_array->sport);

		if  (likely(epp_fifo_cmd_64.bits.sop == 1)) {
			PRINTF_DEBUG("\t epp_desc_ptr 0x%x rx_phy_addr 0x%x virt 0x%x \n", epp_desc_ptr, rx_phy_addr, rx_virt_addr);
			rx_virt_addr += RTK_NI_DEFAULT_RX_HEADROOM; /* skip the head room which is preallocated by software */
			pkt_len = epp_fifo_cmd_64.bits.packet_size;
			rx_array->buf_len = pkt_len - RTK_NI_DEFAULT_HEADERA_HEADERCPU_LEN;	/*Remove HEADER 8 bytes and HEADER CPU 8 bytes*/
			hdr_len = RTK_NI_DEFAULT_HEADERA_HEADERCPU_LEN;
			rx_virt_addr += hdr_len;
			rx_array->buf_addr = rx_virt_addr;
			PRINTF_DEBUG("epp_rd_idx %d, pktlen %d, rx_virt_addr %x hdr_len %d\n", epp_rd_idx, pkt_len, rx_virt_addr, hdr_len);
		} else {
			/* Didn't get sop buffer yet and show the error!!*/
			PRINTF("%s doesn't get buffer with sop flag at first (buf %p)??\n", __func__, rx_phy_addr);
			return RE_ERR_WRONG_CONTENT;
		}

		PE_NI_INR_RING(epp_rd_idx, PE_NI_HAL_EPP0_DESC_SIZE64, PE_NI_HAL_EPP0_FIFO_SIZE_PER_EPP);

		if (unlikely(epp_fifo_cmd_64.bits.eop == 0)) {
			/*for the current NI RX driver, we just support MAX 4 incoming buffers*/
			PRINTF("%s the packets are over size (remaining %d)??\n", __func__, pkt_len);
			return RE_ERR_WRONG_CONTENT;
		}
		
		rx_array++;	
		refill_cnt++;
		count--;
		if (count) {
			rx_array->buf_len = 0;	
		}

		//invalidate
		#if 0
		compiler_barrier();
		if (((epp_rd_idx & 31) == 0) || (epp_rd_idx == epp_wt_idx) || (count == 0))
		{
			DCACHE_INV(epp_desc_ptr, epp_desc_ptr + sizeof(cpu_epp_fifo_cmd_64bit_t));
		}
		#else
		//inv to prevent swap
		DCACHE_INV(epp_desc_ptr, epp_desc_ptr + sizeof(cpu_epp_fifo_cmd_64bit_t));
		DCACHE_INV(rx_virt_addr, rx_virt_addr + sizeof(HEADER_A_T));
		#endif
	} while ((epp_rd_idx != epp_wt_idx) && (count != 0));

	PE_NI_HAL_RX_EEP_RD_IDX_UPDATE(cos, epp_rd_idx);
	g_ni_intf.rx_rd_idx[cos] = epp_rd_idx;

	compiler_barrier();
	//refill rx buffer
	rtk_ni_feed_buff_cnt(refill_cnt);

	return RE_OK;
}

static inline uint32 _rtk_pe_ni_tx_fast_get_free_descriptor_for_10g_sfu() 
{
	uint32 tx_wrt_idx;
	uint32 tx_rd_idx;
	uint32 free_desc;
	uint32 v_addr = 0;
	volatile dma_txdesc_t *curr_desc;

	tx_wrt_idx = g_ni_intf.tx_wrt_idx;
	tx_rd_idx = (uint32) PE_NI_HAL_TX_DMA_RD_IDX(PE_NI_HAL_TXQ_DMA_TXQ_ID);

	/* need to recycle xmit buffers before over-write it*/
#if 0
	while (g_ni_intf.sw_tx_done_idx != tx_rd_idx) 
	{
		curr_desc = (dma_txdesc_t *) ((uint32) virt_txq_dma_desc + PE_NI_HAL_TXQ_DMA_DESC_SIZEOF * g_ni_intf.sw_tx_done_idx);
		//PRINTF_DEBUG("fbm %d addr %x\n", curr_desc->word1.bf.fbm, curr_desc->word0.wrd);
		v_addr = PHYS_TO_VIRT(curr_desc->word0.wrd);
		rtk_ni_pktbuf_free((uint8 *)(v_addr & RTK_NI_DEFAULT_PKT_BUF_MASK));
		PE_NI_INR_RING(g_ni_intf.sw_tx_done_idx, 1, PE_NI_TX_DESC_SIZE);
	}
#endif

	if (tx_wrt_idx >= tx_rd_idx) {
		free_desc = PE_NI_TX_DESC_SIZE - tx_wrt_idx - 1 + tx_rd_idx;
	} else {
		free_desc = tx_rd_idx - tx_wrt_idx - 1;
	}

	return free_desc;
}

static inline uint8 _rtk_pe_ni_tx_pkt_fast_for_10g_sfu(rtk_ni_buf_t *tx_array)
{
	uint32 tx_wrt_idx = g_ni_intf.tx_wrt_idx;
	volatile dma_txdesc_t  *curr_desc;
	uint32 v_addr = 0;
	
	PRINTF_DEBUG("tx_wrt_idx %d buf0 0x%x len0 %u\n", tx_wrt_idx, tx_array->buf_addr, tx_array->buf_len);

	curr_desc = (dma_txdesc_t *) ((uint32) virt_txq_dma_desc + PE_NI_HAL_TXQ_DMA_DESC_SIZEOF * tx_wrt_idx);
	DCACHE_INV(curr_desc, curr_desc + sizeof(dma_txdesc_t));
	v_addr = PHYS_TO_VIRT(curr_desc->word0.wrd);
	if (v_addr != 0) {
		rtk_ni_pktbuf_free((uint8 *)(v_addr & RTK_NI_DEFAULT_PKT_BUF_MASK));
	}
	DCACHE_INV(curr_desc, curr_desc + sizeof(dma_txdesc_t));
	PRINTF_DEBUG("\t curr_desc 0x%x\n", curr_desc);

	curr_desc->word0.wrd = (uint32) VIRT_TO_PHYS(tx_array->buf_addr);
	curr_desc->word1.bf_direct_tx_lan.buf_len = tx_array->buf_len;
	curr_desc->word1.bf_direct_tx_lan.dest = tx_array->dport;

	PRINTF_DEBUG("\t %d: tx_desc0.wrd %x, tx_desc1.wrd %x\n", tx_wrt_idx, curr_desc->word0.wrd, curr_desc->word1.wrd);
	compiler_barrier();

	//write back
	DCACHE_WB(curr_desc, curr_desc + sizeof(dma_txdesc_t));
	PE_NI_INR_RING(tx_wrt_idx,  1, PE_NI_TX_DESC_SIZE);

	g_ni_intf.tx_wrt_idx = tx_wrt_idx;
	PE_NI_HAL_TX_DMA_WR_IDX_UPDATE(PE_NI_HAL_TXQ_DMA_TXQ_ID, tx_wrt_idx);
	return RE_OK;
}

static uint8 rtk_pe_ni_tx_wfo_for_10g_sfu(uint32 count, rtk_ni_buf_t *tx_array) 
{
	int i = count;
	rtk_ni_buf_t * tx_pkt = tx_array;
	uint32 free_desc = 0;

	while ( (i>0) && (tx_pkt->buf_len) ) {	

		while (free_desc < MIN_NI_TX_DESC_NEED) {
			free_desc = _rtk_pe_ni_tx_fast_get_free_descriptor_for_10g_sfu();
			if (free_desc < MIN_NI_TX_DESC_NEED) {
				g_ni_intf.no_tx_descptor++;
			}
		}

		compiler_barrier();
		_rtk_pe_ni_tx_pkt_fast_for_10g_sfu(tx_pkt);

		tx_pkt++;
		i--;
		free_desc = free_desc - MIN_NI_TX_DESC_NEED;
	}

	return RE_OK;
}

/* open to user */
uint32 rtk_ni_nic_init(void)
{
	rtk_pe_10g_sfu_ni_init();
	return RE_OK;
}

uint32 rtk_ni_nic_rx(uint8 *pkt_buf, uint32 *pkt_len, uint32 *sport)
{
	rtk_ni_buf_t rx_array[1] = {0};
	uint32 retpkt = RE_ERR;
	uint32 _pkt_len = 0;

	//setting len
	_pkt_len = *pkt_len;

	//get pkt buf
	retpkt = rtk_pe_ni_rx_wfo_for_10g_sfu(1, rx_array);
	if (retpkt != RE_OK) {
		return RE_ERR_NO_PKT;
	}

	//check length
	if (_pkt_len < rx_array->buf_len) {
		rtk_ni_pktbuf_free(rx_array->buf_addr);
		return RE_ERR_LENGTH;
	}

	compiler_barrier();
	//copy to user pkt buf
	//cache invalidate, copy, then writeback
	DCACHE_INV(rx_array->buf_addr, rx_array->buf_addr + rx_array->buf_len * sizeof(uint8));
	memcpy(pkt_buf, rx_array->buf_addr, rx_array->buf_len);
	DCACHE_WB(pkt_buf, pkt_buf + rx_array->buf_len * sizeof(uint8));

	//invalidate to prevent swap
	DCACHE_INV(rx_array->buf_addr, rx_array->buf_addr + rx_array->buf_len * sizeof(uint8));
	DCACHE_INV(pkt_buf, pkt_buf + rx_array->buf_len * sizeof(uint8));

	//set pkt len & source port
	*pkt_len = rx_array->buf_len;
	*sport = rx_array->sport;

	//free buffer
	rtk_ni_pktbuf_free(rx_array->buf_addr);

	return RE_OK;
}

uint32 rtk_ni_nic_tx(uint8 *pkt_buf, uint32 *pkt_len, uint32 dport)
{
	rtk_ni_buf_t tx_array[1] = {0};
	uint32 _pkt_len = 0;

	//len check
	_pkt_len = *pkt_len;
	if (_pkt_len > RTK_NI_DEFAULT_PKT_BUF_LEN) {
		PRINTF("error; len over size, max pkt buf len is %u\n", RTK_NI_DEFAULT_PKT_BUF_LEN);
		return RE_ERR_LENGTH;
	}

	//get buf addr from sw pool & copy pkt buf content from user
	tx_array->buf_addr = rtk_ni_pktbuf_get();

	//cache invalidate, copy, then writeback
	DCACHE_INV(pkt_buf, pkt_buf + _pkt_len * sizeof(uint8));
	memcpy(tx_array->buf_addr, pkt_buf, _pkt_len);
	DCACHE_WB(tx_array->buf_addr, tx_array->buf_addr + _pkt_len * sizeof(uint8));

	//invalidate to prevent swap
	DCACHE_INV(tx_array->buf_addr, tx_array->buf_addr + _pkt_len * sizeof(uint8));
	DCACHE_INV(pkt_buf, pkt_buf + _pkt_len * sizeof(uint8));

	//set len
	tx_array->buf_len = _pkt_len;

	//set dport
	tx_array->dport = dport;

	//tx
	rtk_pe_ni_tx_wfo_for_10g_sfu(1, tx_array);

	return RE_OK;
}

#if 1 //for test
static void dump_pkt_content(uint8 *pkt_buf, uint32 pkt_len)
{
	uint32 i = 0;
	uint32 rawCnt = 0;

	for (i = 0; i < pkt_len; i++)
	{
		PRINTF("%02x ", pkt_buf[i]);
		rawCnt++;
		if(rawCnt % 16 == 0) {
			PRINTF("\n\t");
		}
	}
	PRINTF("\n\t");
}

void rtk_pe_ni_rx_wfo_for_10g_sfu_test(void)
{
	uint8  test_pkt_buf[RTK_NI_DEFAULT_PKT_BUF_LEN] = {0};
	uint32 test_pkt_len = 0;
	uint32 retpkt = RE_ERR;
	uint32 dport = 1;
	uint32 sport = 0;

	//init ne
	rtk_ni_nic_init();

	//do nic rx tx
	do {
		//receive data
		test_pkt_len = RTK_NI_DEFAULT_PKT_BUF_LEN;
		compiler_barrier();
		retpkt = rtk_ni_nic_rx(&test_pkt_buf[0], &test_pkt_len, &sport);
		compiler_barrier();

		if (retpkt == RE_OK)
		{
			//dump rx pkt
			//dump_pkt_content(&test_pkt_buf[0], TEST_PKT_LEN);

			//tx
			rtk_ni_nic_tx(&test_pkt_buf[0], &test_pkt_len, dport);
		}
	} while (1);
}
#endif
