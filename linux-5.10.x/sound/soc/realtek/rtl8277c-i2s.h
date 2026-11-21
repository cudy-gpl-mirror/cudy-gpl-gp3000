/*
 * rtl8197d-i2s.h  --  ALSA Soc Audio Layer
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    24th Feb 2012   Initial version.
 */

#ifndef RTL277C_I2S_H_
#define RTL277C_I2S_H_

/* GLB_REGS */
#define GLOBAL_GPIO_MUX_4_8		BIT(8)
#define GLOBAL_GPIO_MUX_4_9		BIT(9)
#define GLOBAL_GPIO_MUX_4_10		BIT(10)
#define GLOBAL_GPIO_MUX_4_11		BIT(11)
/* PSDS REGS */
#define PSDS_MODE_SDS_MODE_S0_SET	(0x1a)
#define PSDS_MODE_SDS_MODE_S0_ILLEGAL	(0x1f)
#define PSDS_CH0_ABILTY_SDS01_SDS0_ABLITY_SET	(0x5cf4)

extern void __iomem		*IIS_BASE;

#define IISCR		(IIS_BASE + 0x00)	/* IIS Interface Control Register */
typedef volatile union {
	struct {
		unsigned int iis_en            :  1 ; /* bits 0:0 */
		unsigned int tx_act            :  2 ; /* bits 1:2 */
		unsigned int audio_mono        :  2 ; /* bits 3:4 */
		unsigned int edge_sw           :  1 ; /* bits 5:5 */
		unsigned int rsrvd1            :  1 ; /* bits 6:6 */
		unsigned int loop_back         :  1 ; /* bits 7:7 */
		unsigned int format            :  2 ; /* bits 8:9 */
		unsigned int daclrswap         :  1 ; /* bits 10:10 */
		unsigned int sck_swap          :  1 ; /* bits 11:11 */
		unsigned int byte_swap         :  1 ; /* bits 12:12 */
		unsigned int rsrvd2            :  2 ; /* bits 13:14 */
		unsigned int debug_switch      :  2 ; /* bits 15:16 */
		unsigned int rsrvd3            :  1 ; /* bits 17:17 */
		unsigned int burst_size        :  5 ; /* bits 18:22 */
		unsigned int rx_enable         :  1 ; /* bits 23:23 */
		unsigned int tx_enable         :  1 ; /* bits 24:24 */
		unsigned int rx_rstn           :  1 ; /* bits 25:25 */
		unsigned int tx_rstn           :  1 ; /* bits 26:26 */
		unsigned int mute              :  1 ; /* bits 27:27 */
		unsigned int slave_mode        :  1 ; /* bits 28:28 */
		unsigned int wl                :  2 ; /* bits 29:30 */
		unsigned int sw_rstn           :  1 ; /* bits 31:31 */
	} bf ;
	unsigned int     wrd ;
} IISCR_t;

#define TX_PAGE_PTR	(IIS_BASE + 0x04)	/* TX Page Pointer Register */
typedef volatile union {
	struct {
		unsigned int rsrvd            	   :  2 ; /* bits 0:1 */
		unsigned int page_ptr_tx           :  30 ; /* bits 2:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_TX_PAGE_PTR_t;

#define RX_PAGE_PTR	(IIS_BASE + 0x08)	/* RX Page Pointer Register */
typedef volatile union {
	struct {
		unsigned int rsrvd            	   :  2 ; /* bits 0:1 */
		unsigned int page_ptr_rx           :  30 ; /* bits 2:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_RX_PAGE_PTR_t;

#define IIS_SETTING	(IIS_BASE + 0x0C)	/* IIS Page size and Sampling rate setting  Register */
typedef volatile union {
	struct {
		unsigned int page_szie             :  12 ; /* bits 0:11 */
		unsigned int page_num              :  2 ; /* bits 12:13 */
		unsigned int sr	                   :  4 ; /* bits 14:17 */
		unsigned int clk_switch	           :  1 ; /* bits 18:18 */
		unsigned int rsrvd	           :  13 ; /* bits 19:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_SETTING_t;

#define IIS_TX_IMR	(IIS_BASE + 0x10)	/* IIS TX Interrupt Mask Register */
typedef volatile union {
	struct {
		unsigned int p0okie_tx             :  1 ; /* bits 0:0 */
		unsigned int p1okie_tx             :  1 ; /* bits 1:1 */
		unsigned int p2okie_tx             :  1 ; /* bits 2:2 */
		unsigned int p3okie_tx             :  1 ; /* bits 3:3 */
		unsigned int p0unva_ie_tx          :  1 ; /* bits 4:4 */
		unsigned int p1unva_ie_tx          :  1 ; /* bits 5:5 */
		unsigned int p2unva_ie_tx          :  1 ; /* bits 6:6 */
		unsigned int p3unva_ie_tx          :  1 ; /* bits 7:7 */
		unsigned int fifo_empty_ie_tx      :  1 ; /* bits 8:8 */
		unsigned int rsrvd	           :  23 ; /* bits 9:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_TX_IMR_t;

#define IIS_TX_ISR	(IIS_BASE + 0x14)	/* IIS TX Interrupt Status Register */
typedef volatile union {
	struct {
		unsigned int p0okip_tx             :  1 ; /* bits 0:0 */
		unsigned int p1okip_tx             :  1 ; /* bits 1:1 */
		unsigned int p2okip_tx             :  1 ; /* bits 2:2 */
		unsigned int p3okip_tx             :  1 ; /* bits 3:3 */
		unsigned int p0unva_ip_tx          :  1 ; /* bits 4:4 */
		unsigned int p1unva_ip_tx          :  1 ; /* bits 5:5 */
		unsigned int p2unva_ip_tx          :  1 ; /* bits 6:6 */
		unsigned int p3unva_ip_tx          :  1 ; /* bits 7:7 */
		unsigned int fifo_empty_ip_tx      :  1 ; /* bits 8:8 */
		unsigned int rsrvd	           :  23 ; /* bits 9:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_TX_ISR_t;

#define IIS_RX_IMR	(IIS_BASE + 0x18)	/* IIS RX Interrupt Mask Register */
typedef volatile union {
	struct {
		unsigned int p0okie_rx             :  1 ; /* bits 0:0 */
		unsigned int p1okie_rx             :  1 ; /* bits 1:1 */
		unsigned int p2okie_rx             :  1 ; /* bits 2:2 */
		unsigned int p3okie_rx             :  1 ; /* bits 3:3 */
		unsigned int p0unva_ie_rx          :  1 ; /* bits 4:4 */
		unsigned int p1unva_ie_rx          :  1 ; /* bits 5:5 */
		unsigned int p2unva_ie_rx          :  1 ; /* bits 6:6 */
		unsigned int p3unva_ie_rx          :  1 ; /* bits 7:7 */
		unsigned int fifo_empty_ie_rx      :  1 ; /* bits 8:8 */
		unsigned int rsrvd	           :  23 ; /* bits 9:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_RX_IMR_t;

#define IIS_RX_ISR	(IIS_BASE + 0x1C)	/* IIS RX Interrupt Status Register */
typedef volatile union {
	struct {
		unsigned int p0okip_rx             :  1 ; /* bits 0:0 */
		unsigned int p1okip_rx             :  1 ; /* bits 1:1 */
		unsigned int p2okip_rx             :  1 ; /* bits 2:2 */
		unsigned int p3okip_rx             :  1 ; /* bits 3:3 */
		unsigned int p0unva_ip_rx          :  1 ; /* bits 4:4 */
		unsigned int p1unva_ip_rx          :  1 ; /* bits 5:5 */
		unsigned int p2unva_ip_rx          :  1 ; /* bits 6:6 */
		unsigned int p3unva_ip_rx          :  1 ; /* bits 7:7 */
		unsigned int fifo_empty_ip_rx      :  1 ; /* bits 8:8 */
		unsigned int rsrvd	           :  23 ; /* bits 9:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_RX_ISR_t;

#define IIS_TX_P0OWN	(IIS_BASE + 0x20)	/* IIS TX Page 0 Own bit */
typedef volatile union {
	struct {
		unsigned int rsrvd	          :  31 ; /* bits 0:30 */
		unsigned int p0own_tx	          :  1 ; /* bits 31:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_TX_P0OWN_t;

#define IIS_TX_P1OWN	(IIS_BASE + 0x24)	/* IIS TX Page 1 Own bit */
typedef volatile union {
	struct {
		unsigned int rsrvd	          :  31 ; /* bits 0:30 */
		unsigned int p1own_tx	          :  1 ; /* bits 31:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_TX_P1OWN_t;

#define IIS_TX_P2OWN	(IIS_BASE + 0x28)	/* IIS TX Page 2 Own bit */
typedef volatile union {
	struct {
		unsigned int rsrvd	          :  31 ; /* bits 0:30 */
		unsigned int p2own_tx	          :  1 ; /* bits 31:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_TX_P2OWN_t;

#define IIS_TX_P3OWN	(IIS_BASE + 0x2C)	/* IIS TX Page 3 Own bit */
typedef volatile union {
	struct {
		unsigned int rsrvd	          :  31 ; /* bits 0:30 */
		unsigned int p3own_tx	          :  1 ; /* bits 31:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_TX_P3OWN_t;

#define IIS_RX_P0OWN	(IIS_BASE + 0x30)	/* IIS RX Page 0 Own bit */
typedef volatile union {
	struct {
		unsigned int rsrvd	          :  31 ; /* bits 0:30 */
		unsigned int p0own_rx	          :  1 ; /* bits 31:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_RX_P0OWN_t;

#define IIS_RX_P1OWN	(IIS_BASE + 0x34)	/* IIS RX Page 1 Own bit */
typedef volatile union {
	struct {
		unsigned int rsrvd	          :  31 ; /* bits 0:30 */
		unsigned int p1own_rx	          :  1 ; /* bits 31:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_RX_P1OWN_t;

#define IIS_RX_P2OWN	(IIS_BASE + 0x38)	/* IIS RX Page 2 Own bit */
typedef volatile union {
	struct {
		unsigned int rsrvd	          :  31 ; /* bits 0:30 */
		unsigned int p2own_rx	          :  1 ; /* bits 31:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_RX_P2OWN_t;

#define IIS_RX_P3OWN	(IIS_BASE + 0x3C)	/* IIS RX Page 3 Own bit */
typedef volatile union {
	struct {
		unsigned int rsrvd	          :  31 ; /* bits 0:30 */
		unsigned int p3own_rx	          :  1 ; /* bits 31:31 */
	} bf ;
	unsigned int     wrd ;
} IIS_RX_P3OWN_t;

/* IISCR */
#define DACLRSWAP	BIT(10)
#define IIS_FORMAT_I2S	(BIT(8)|BIT(9))
#define IIS_LOOP_BACK	BIT(7)
#define IIS_WL_16BIT	BIT(6)
#define IIS_EDGE_N	BIT(5)
#define IIS_MODE_MONO	BIT(4)
#define IIS_TXRXACT	BIT(2)
#define IIS_ENABLE	BIT(0)

/* IIS_TX_ISR */
#define IIS_TX_P0OK	BIT(0)
#define IIS_TX_P1OK	BIT(1)
#define IIS_TX_P2OK	BIT(2)
#define IIS_TX_P3OK	BIT(3)
#define IIS_TX_PAGEUNAVA	BIT(4)
#define IIS_TX_FIFO_EMPTY	BIT(5)

/* IIS_RX_ISR */
#define IIS_RX_P0OK	BIT(0)
#define IIS_RX_P1OK	BIT(1)
#define IIS_RX_P2OK	BIT(2)
#define IIS_RX_P3OK	BIT(3)
#define IIS_RX_PAGEUNAVA	BIT(4)
#define IIS_RX_FIFO_FULL	BIT(5)

/* Interrupt Mask Type */
#define P0OK_TX		0
#define P1OK_TX		1
#define P2OK_TX		2
#define P3OK_TX		3
#define TPUA		4
#define TFEM		5
#define P0OK_RX		6
#define P1OK_RX		7
#define P2OK_RX		8
#define P3OK_RX		9
#define RPUA		10
#define RFFU		11

/* clock sources */
#define RTL8197D_CLKSRC_PCLK 0
#define RTL8197D_CLKSRC_MPLL 1

/* Clock dividers */
#define RTL8197D_DIV_MCLK	0
#define RTL8197D_DIV_BCLK	1
#define RTL8197D_DIV_PRESCALER	2

/* prescaler */
#define RTL8197D_PRESCALE(a,b) \
	(((a - 1) << RTL8197D_IISPSR_INTSHIFT) | ((b - 1) << RTL8197D_IISPSR_EXTSHFIT))

extern struct snd_soc_dai_driver rtl8277c_i2s_dai;
#endif /*RTL8197DI2S_H_*/
