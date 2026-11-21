/*
 * rtl819x-pcm.c  --  ALSA Soc Audio Layer
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    24th Feb 2012   Initial version.
 *    4th May 2012    add capture support
 *    6th Nov 2013    add mono channel support
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <asm/dma.h>
#include <linux/kconfig.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/version.h>
#include <linux/swab.h>
#include "rtl8277c-pcm.h"
#include "rtl8277c-i2s.h"

#define IIS_PAGE_NUM							4
#define IIS_FIRST_TX								1
#define IIS_FIRST_RX								2
#define BUFFER_BYTES_MAX 				((1920*40)/10)*4*2
#define I2S_DMA_ADDRESS_MASK 		0xFFFFFFFF

int iis_counter = 0;
int iis_first_start;
int iis_channels;
int iis_sample_rate;
int iis_SR = 5; /* default is 48K */
int iis_44100_enabled = 0;
int iis_be_le = 0; /* 0:be, 1:le */
int iis_bits = 0; /* 0:16bits, 1:24bit */
int iis_rxNextpage = 0;
int iis_txNextpage = 0;
int isPlaying = 0;
int isCapture = 0;

struct snd_pcm_substream *tx_substream;
struct snd_pcm_substream *rx_substream;

static unsigned long IISChanTxPage[4] = {IIS_TX_P0OK, IIS_TX_P1OK, IIS_TX_P2OK, IIS_TX_P3OK};
static unsigned long IISChanRxPage[4] = {IIS_RX_P0OK, IIS_RX_P1OK, IIS_RX_P2OK, IIS_RX_P3OK};

static int iis_txpage[2];
static int iis_rxpage[2];

static dma_addr_t iis_tx_data;
static dma_addr_t iis_rx_data;

/*--------------------------------------
	for i2s hook module used
--------------------------------------*/

static dma_addr_t iis_TxHook_pa;
static dma_addr_t iis_RxHook_pa;
void *iis_TxHook_va=NULL;
void *iis_RxHook_va=NULL;

void (*_ext_i2s_tx_page_done_hook)(int, void *) = NULL;
void (*_ext_i2s_rx_page_done_hook)(int, void *) = NULL;
void (*_ext_i2s_set_path_hook)(void) = NULL;
void (*_ext_i2s_rx_prepare_stage_hook)(int, int, void *, void *, int, int) = NULL;

static const struct snd_pcm_hardware rtl8277c_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED |
				  SNDRV_PCM_INFO_BLOCK_TRANSFER |
				  SNDRV_PCM_INFO_MMAP |
				  SNDRV_PCM_INFO_MMAP_VALID,
	.formats		= SNDRV_PCM_FMTBIT_S16_BE | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
	.rate_min = 8000,
	.rate_max = 48000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= BUFFER_BYTES_MAX,
	.period_bytes_min	= (BUFFER_BYTES_MAX/48)/2,
	.period_bytes_max	= (BUFFER_BYTES_MAX/2),
	.periods_min		= 2,
	.periods_max		= 4,
	.fifo_size		= 0,
};

struct rtl8277c_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t dma_start;
	dma_addr_t dma_pos;
	dma_addr_t dma_end;
	struct rtl8277c_pcm_dma_params *params;
};


static void rtl8277c_pcm_enqueue(struct snd_pcm_substream *substream);
/* rtl8277c i2s controller code */
static void rtl8277c_i2s_trx_start(int direction)
{
	IISCR_t iiscr;

	iiscr.wrd = rtlRegRead(IISCR);
	if (iis_channels == 1) {
		iiscr.bf.audio_mono = 2;
	} else {
		iiscr.bf.audio_mono = 0;
	}

	if (iis_be_le == 1) {
		iiscr.bf.byte_swap = 0;
	} else {
		iiscr.bf.byte_swap = 1;
	}

	if (iis_bits == 1) {
		iiscr.bf.wl = 1;
	} else if (iis_bits == 2) {
		iiscr.bf.wl = 2;
	} else {
		iiscr.bf.wl = 0;
	}

	iiscr.bf.tx_act = 3;
	iiscr.bf.iis_en = 1;
	/* set brust size */
	iiscr.bf.burst_size = 0x7;

	if(!_ext_i2s_set_path_hook) {
		if (direction == SNDRV_PCM_STREAM_PLAYBACK) {
			iiscr.bf.tx_enable = 1;
			iiscr.bf.tx_rstn = 1;
			isPlaying = 1;
		} else {
			iiscr.bf.rx_enable = 1;
			iiscr.bf.rx_rstn = 1;
			isCapture = 1;
		}
		rtlRegWrite(IISCR, iiscr.wrd);
	}
	else{
			iiscr.bf.tx_enable = 1;
			iiscr.bf.tx_rstn = 1;
			isPlaying = 1;
			iiscr.bf.rx_enable = 1;
			iiscr.bf.rx_rstn = 1;
			isCapture = 1;
			rtlRegWrite(IISCR, iiscr.wrd);
	}
}

static void rtl8277c_i2s_tx_stop(void)
{
	/* stop TX */
	IISCR_t iiscr;
	IIS_TX_ISR_t iis_isr_t;

	iiscr.wrd = rtlRegRead(IISCR);
	iiscr.bf.tx_enable = 0;
	iiscr.bf.tx_rstn = 0;
	rtlRegWrite(IISCR, iiscr.wrd);

	iis_isr_t.wrd = rtlRegRead(IIS_TX_ISR);
	iis_isr_t.bf.p0okip_tx = 1;
	iis_isr_t.bf.p1okip_tx = 1;
	iis_isr_t.bf.p2okip_tx = 1;
	iis_isr_t.bf.p3okip_tx = 1;
	iis_isr_t.bf.p0unva_ip_tx = 1;
	iis_isr_t.bf.fifo_empty_ip_tx = 1;
	rtlRegWrite(IIS_TX_ISR, iis_isr_t.wrd);

	iis_txpage[0] = 0;
	isPlaying = 0;
}

static void rtl8277c_i2s_rx_stop(void)
{
	/* stop RX */
	IISCR_t iiscr;
	IIS_RX_ISR_t iis_isr_r;

	iiscr.wrd = rtlRegRead(IISCR);
	iiscr.bf.rx_enable = 0;
	iiscr.bf.rx_rstn = 0;
	rtlRegWrite(IISCR, iiscr.wrd);

	iis_isr_r.wrd = rtlRegRead(IIS_RX_ISR);
	iis_isr_r.bf.p0okip_rx = 1;
	iis_isr_r.bf.p1okip_rx = 1;
	iis_isr_r.bf.p2okip_rx = 1;
	iis_isr_r.bf.p3okip_rx = 1;
	iis_isr_r.bf.p0unva_ip_rx = 1;
	iis_isr_r.bf.fifo_empty_ip_rx = 1;
	rtlRegWrite(IIS_RX_ISR, iis_isr_r.wrd);

	iis_rxpage[0] = 0;
	isCapture = 0;
}

static void rtl8277c_i2s_trx_stop(void)
{
	IISCR_t iiscr;

	rtl8277c_i2s_tx_stop();
	rtl8277c_i2s_rx_stop();
	iiscr.wrd = rtlRegRead(IISCR);
	iiscr.bf.iis_en = 0;
	rtlRegWrite(IISCR, iiscr.wrd);
}


static void iis_ISR(unsigned int iis_txisr, unsigned int iis_rxisr)
{
	unsigned int i;
	struct rtl8277c_runtime_data *prtd;

	/* page0/page1/page2/page3 */
	for (i=0; i < IIS_PAGE_NUM; i++) {
		if (iis_txisr & IISChanTxPage[iis_txpage[0]]) {
			if(_ext_i2s_tx_page_done_hook)
            {
               _ext_i2s_tx_page_done_hook((int)iis_txpage[0], iis_TxHook_va);
                rtlRegWrite(IIS_TX_P0OWN + 4*iis_txpage[0], BIT(31) );
                iis_txNextpage = (iis_txpage[0] +1 ) % IIS_PAGE_NUM;
                iis_txpage[0] = (iis_txpage[0] +1 ) % IIS_PAGE_NUM;
				//printk(" t af %x \n", iis_txpage[0]);
            }
			else {
				iis_txNextpage = (iis_txpage[0] +1 ) % IIS_PAGE_NUM;
				if (tx_substream)
					iis_txpage[0] = (iis_txpage[0] +1 ) % IIS_PAGE_NUM;
				if(tx_substream) {
					snd_pcm_period_elapsed(tx_substream);
					if(tx_substream) {
						prtd=tx_substream->runtime->private_data;
						prtd->dma_loaded--;
						rtl8277c_pcm_enqueue(tx_substream);
					}
				}
			}
		} else {
			if (isPlaying == 0) {
				if ( iis_txisr & IISChanTxPage[0] )
				{
					printk("T P0\n");
					iis_txNextpage= 1;
				}else if ( iis_txisr & IISChanTxPage[1])
				{
					printk("T P1\n");
					iis_txNextpage= 2;
				}else if ( iis_txisr & IISChanTxPage[2])
				{
					printk("T P2\n");
					iis_txNextpage= 3;
				}else if ( iis_txisr & IISChanTxPage[3])
				{
					printk("T P3\n");
					iis_txNextpage= 0;
				}
			}
		}

		if (iis_rxisr & IISChanRxPage[iis_rxpage[0]]) {

			if(_ext_i2s_rx_page_done_hook)
            {
                _ext_i2s_rx_page_done_hook((int)iis_rxpage[0], iis_RxHook_va);
                rtlRegWrite(IIS_RX_P0OWN + 4*iis_rxpage[0], BIT(31) );
                iis_rxNextpage = (iis_rxpage[0] +1 ) % IIS_PAGE_NUM;
                iis_rxpage[0] = (iis_rxpage[0]+1) % IIS_PAGE_NUM;
            }
            else {
				iis_rxNextpage = (iis_rxpage[0]+1) % IIS_PAGE_NUM;
				if (rx_substream)
					iis_rxpage[0] = (iis_rxpage[0]+1) % IIS_PAGE_NUM;
				if(rx_substream) {
					snd_pcm_period_elapsed(rx_substream);
					if(rx_substream) {
						prtd=rx_substream->runtime->private_data;
						prtd->dma_loaded--;
						rtl8277c_pcm_enqueue(rx_substream);
					}
				}
			}
		} else {
			if (isCapture == 0)  {
				if ( iis_rxisr & IISChanRxPage[0] )
				{
					printk("R P0\n");
					iis_rxNextpage= 1;
				}else if ( iis_rxisr & IISChanRxPage[1])
				{
					printk("R P1\n");
					iis_rxNextpage= 2;
				}else if ( iis_rxisr & IISChanRxPage[2])
				{
					printk("R P2\n");
					iis_rxNextpage= 3;
				}else if ( iis_rxisr & IISChanRxPage[3])
				{
					printk("R P3\n");
					iis_rxNextpage= 0;
				}
			}
		}
	}
	return;
}



static enum irqreturn iis_dma_interrupt(int irq,  void *dev)
{
	IIS_TX_ISR_t iis_isr_t;
	IIS_RX_ISR_t iis_isr_r;


	if ((iis_isr_t.wrd = rtlRegRead(IIS_TX_ISR)) | (iis_isr_r.wrd = rtlRegRead(IIS_RX_ISR))) {
		rtlRegWrite(IIS_TX_ISR, iis_isr_t.wrd);
		rtlRegWrite(IIS_RX_ISR, iis_isr_r.wrd);

		if ((iis_isr_t.wrd & 0x0F) | (iis_isr_r.wrd & 0x0F)) {	/* TOK and ROK only */

			iis_ISR(iis_isr_t.wrd & 0x0F, iis_isr_r.wrd & 0x0F);
		}
	}

   	return (IRQ_HANDLED);
}

/* rtl8277c_pcm_enqueue
 *
 * place a dma buffer onto the queue for the dma system
 * to handle.
*/
void rtl8277c_pcm_enqueue(struct snd_pcm_substream *substream)
{
	struct rtl8277c_runtime_data *prtd = substream->runtime->private_data;
	dma_addr_t pos = prtd->dma_pos;
	dma_addr_t start = prtd->dma_start;

	int dma_page;
	void __iomem *iis_p0own;
	unsigned int pg_own;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		iis_p0own = IIS_TX_P0OWN;
	else
		iis_p0own = IIS_RX_P0OWN;

	while (prtd->dma_loaded < prtd->dma_limit) {
		unsigned long len = prtd->dma_period;

		if ((pos + len) > prtd->dma_end) {
			len  = prtd->dma_end - pos;
			pr_info(KERN_INFO "%s: corrected dma len %ld\n",__func__, len);
		}
		/* dma_page = (pos-start) / prtd->dma_period; */
		if ((pos-start) < prtd->dma_period)
			dma_page = 0;
		else if (((pos-start) < 2*prtd->dma_period))
			dma_page = 1;
		else if (((pos-start) < 3*prtd->dma_period))
			dma_page = 2;
		else if (((pos-start) < 4*prtd->dma_period))
			dma_page = 3;
		else
			dma_page = 0;

		pg_own = rtlRegRead(iis_p0own + 4*dma_page);

		if (pg_own == 0) {
			/* page own by cpu */
			rtlRegWrite(iis_p0own + 4*dma_page, BIT(31));
			prtd->dma_loaded++;
			pos += prtd->dma_period;
			if (pos >= prtd->dma_end)
				pos = prtd->dma_start;
		} else {
			break;
		}
	}

	prtd->dma_pos = pos;
}

static int rtl8277c_pcm_hw_params(struct snd_soc_component *component, struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rtl8277c_runtime_data *prtd = runtime->private_data;
	unsigned long totbytes = params_buffer_bytes(params);
	int stream_direction = substream->stream;
	iis_bits = 0;

	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_BE:
			iis_be_le = 0;
			break;
		case SNDRV_PCM_FORMAT_S16_LE:
			iis_be_le = 1;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			iis_bits = 1;
			iis_be_le = 1;
			break;
		case SNDRV_PCM_FORMAT_S24_BE:
			iis_bits = 1;
			iis_be_le = 0;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			iis_bits = 2;
			iis_be_le = 1;
			break;
		case SNDRV_PCM_FORMAT_S32_BE:
			iis_bits = 2;
			iis_be_le = 0;
			break;
		default:
			return -EINVAL;
	}

	iis_channels = params_channels(params);
	iis_sample_rate = params_rate(params);
	iis_44100_enabled = 0;
	if (iis_sample_rate == 8000) {
		iis_SR = 0;
	} else if (iis_sample_rate == 12000 || iis_sample_rate == 11025) {
		iis_SR = 1;
		if (iis_sample_rate == 11025)
			iis_44100_enabled = 1;
	} else if (iis_sample_rate == 16000) {
		iis_SR = 2;
	} else if(iis_sample_rate == 22050) {
		iis_SR = 3;
	} else if (iis_sample_rate == 32000) {
		iis_SR = 4;
	} else if (iis_sample_rate == 48000 || iis_sample_rate == 44100) {
		iis_SR = 5;
		if(iis_sample_rate == 44100)
			iis_44100_enabled = 1;
	} else if (iis_sample_rate == 64000) {
		iis_SR = 6;
	} else if (iis_sample_rate == 96000|| iis_sample_rate == 88200) {
		iis_SR = 7;
		if(iis_sample_rate == 88200)
			iis_44100_enabled = 1;
	} else if (iis_sample_rate == 192000 || iis_sample_rate == 176400) {
		iis_SR = 8;
		if(iis_sample_rate == 176400)
			iis_44100_enabled = 1;
	} else if (iis_sample_rate == 384000) {
		iis_SR = 9;
	} else {
		printk("iis_sample_rate=%d err\n", iis_sample_rate);
		return -EINVAL;
	}

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	runtime->dma_bytes = totbytes;
	if ((isPlaying == 0 && isCapture == 0)) {
		iis_txNextpage = 0;
		iis_rxNextpage = 0;
	}

	spin_lock_irq(&prtd->lock);
	prtd->dma_loaded = 0;
	prtd->dma_limit = runtime->hw.periods_min;
	prtd->dma_period = params_period_bytes(params);
	prtd->dma_start = runtime->dma_addr;
	if (stream_direction == SNDRV_PCM_STREAM_PLAYBACK) {
		/* playback */
		prtd->dma_pos = prtd->dma_start + (iis_txNextpage*prtd->dma_period);
		iis_txpage[0] = iis_txNextpage;
	}else {
		prtd->dma_pos = prtd->dma_start + (iis_rxNextpage*prtd->dma_period);
		iis_rxpage[0] = iis_rxNextpage;
	}
	prtd->dma_end = prtd->dma_start + totbytes;
	spin_unlock_irq(&prtd->lock);

	return 0;
}


static int rtl8277c_pcm_hw_free(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	/* TODO - do we need to ensure DMA flushed */
	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}

static int rtl8277c_pcm_prepare(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rtl8277c_runtime_data *prtd = substream->runtime->private_data;
	int period_bytes = frames_to_bytes(runtime, runtime->period_size);
	int ret = 0;
	int iis_page_size;
	int iis_page_number;
	unsigned int buffsize=BUFFER_BYTES_MAX/2+32;
	IIS_SETTING_t iis_set;


	/* channel needs configuring for mem=>device, increment memory addr,
	 * sync to pclk, half-word transfers to the IIS-FIFO. */
	iis_page_size = period_bytes;
	iis_page_number = runtime->periods;
	iis_set.wrd = rtlRegRead(IIS_SETTING);
	iis_set.bf.page_szie = (iis_page_size / 4) - 1;
	iis_set.bf.page_num = iis_page_number - 1;
	iis_set.bf.sr = iis_SR;
	iis_set.bf.clk_switch = iis_44100_enabled;

	rtlRegWrite(IIS_SETTING, iis_set.wrd);	/* set page size */

	 if(_ext_i2s_rx_prepare_stage_hook)
	 {
			if(iis_RxHook_va == NULL)
			{
				ret = dma_coerce_mask_and_coherent(substream->pcm->card->dev, DMA_BIT_MASK(32));
				if (ret) {
					dev_err(substream->pcm->card->dev, "No usable DMA configuration\n");
				}
				iis_RxHook_va = dma_alloc_coherent(substream->pcm->card->dev, buffsize, &iis_RxHook_pa, GFP_KERNEL);
				if (iis_RxHook_va == NULL)
					dev_err(substream->pcm->card->dev, "[%s] [%d] allocate dma coherent failed\n", __FUNCTION__, __LINE__);
				else {
					memset((void *)iis_RxHook_va, 0 , buffsize);
					printk("[%s] [%d] allocate rx hook pa: [0x%llx],  va=[0x%llx], size =[0x%x]\n",
									__FUNCTION__, __LINE__,  iis_RxHook_pa,  (long long unsigned int)iis_RxHook_va, buffsize);
				}
			}
			if(iis_TxHook_va == NULL)
			{
					ret = dma_coerce_mask_and_coherent(substream->pcm->card->dev, DMA_BIT_MASK(32));
					if (ret) {
						dev_err(substream->pcm->card->dev, "No usable DMA configuration\n");
					}
					iis_TxHook_va = dma_alloc_coherent(substream->pcm->card->dev, buffsize, &iis_TxHook_pa, GFP_KERNEL);
					if (iis_TxHook_va == NULL)
						dev_err(substream->pcm->card->dev, "[%s] [%d] allocate dma coherent failed\n", __FUNCTION__, __LINE__);
					else {
						memset((void *)iis_TxHook_va, 0 , buffsize);
						printk("[%s] [%d] allocate tx hook pa: [0x%llx],  va=[0x%llx], size =[0x%x]\n",
										__FUNCTION__, __LINE__,  iis_TxHook_pa,  (long long unsigned int)iis_TxHook_va, buffsize);
					}
			}

			if( (iis_TxHook_va != NULL) &&  ( iis_RxHook_va != NULL  )   )
			{
				_ext_i2s_rx_prepare_stage_hook(period_bytes,
																   iis_page_number,
																   iis_TxHook_va,
																   iis_RxHook_va,
																   buffsize,
																   buffsize);
				rtlRegWrite(TX_PAGE_PTR,(unsigned int)(iis_TxHook_pa) & I2S_DMA_ADDRESS_MASK);
				rtlRegWrite(RX_PAGE_PTR,(unsigned int)(iis_RxHook_pa) & I2S_DMA_ADDRESS_MASK);

			}else {
				dev_err(substream->pcm->card->dev, "[%s] [%d] allocate dma coherent fail for tx/rx hook buffer\n", __FUNCTION__, __LINE__);
			}
            iis_txpage[0]=0;
            iis_rxpage[0]=0;

			/*force tx/rx use 4 pages*/
			rtlRegWrite(IIS_TX_P0OWN,BIT(31));
			rtlRegWrite(IIS_TX_P1OWN,BIT(31));
			rtlRegWrite(IIS_TX_P2OWN,BIT(31));
			rtlRegWrite(IIS_TX_P3OWN,BIT(31));
			rtlRegWrite(IIS_RX_P0OWN,BIT(31));
			rtlRegWrite(IIS_RX_P1OWN,BIT(31));
			rtlRegWrite(IIS_RX_P2OWN,BIT(31));
			rtlRegWrite(IIS_RX_P3OWN,BIT(31));
     }
	else {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			rtlRegWrite(TX_PAGE_PTR, (unsigned int) (runtime->dma_addr) & 0xffffffff);
			if (isPlaying == 0 && isCapture == 0) {
				rtlRegWrite(IIS_TX_P0OWN,0);
				rtlRegWrite(IIS_TX_P1OWN,0);
				rtlRegWrite(IIS_TX_P2OWN,0);
				rtlRegWrite(IIS_TX_P3OWN,0);
			}
		} else {
			rtlRegWrite(RX_PAGE_PTR, (unsigned int) (runtime->dma_addr) & 0xffffffff);
			if (isPlaying == 0 && isCapture == 0) {
				rtlRegWrite(IIS_RX_P0OWN,0);
				rtlRegWrite(IIS_RX_P1OWN,0);
				rtlRegWrite(IIS_RX_P2OWN,0);
				rtlRegWrite(IIS_RX_P3OWN,0);
			}

			/* flush the DMA channel */
			prtd->dma_loaded = 0;
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				prtd->dma_pos = prtd->dma_start + (iis_txpage[0]*prtd->dma_period);
			} else {
				prtd->dma_pos = prtd->dma_start + (iis_rxpage[0]*prtd->dma_period);
			}
		}
	}
	return ret;
}

static int rtl8277c_pcm_trigger(struct snd_soc_component *component, struct snd_pcm_substream *substream, int cmd)
{
	struct rtl8277c_runtime_data *prtd = substream->runtime->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;
	unsigned int sample_rate = runtime->rate;
	unsigned int channels = runtime->channels;
	unsigned int sample_bits = runtime->sample_bits;
	unsigned int t = 0;

	spin_lock(&prtd->lock);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		prtd->state |= ST_RUNNING;
		/*
		 * TX and RX are not independent,they are enabled at the
		 * same time, even if only one side is running. So, we
		 * need to configure both of them at the time when the first
		 * stream is opened.
		 *
		 * CPU DAI:slave mode.
		 */
		iis_counter ++;
		if(!_ext_i2s_rx_page_done_hook)
		{
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				rtl8277c_pcm_enqueue(substream);
				tx_substream = substream;

			} else {
				rtl8277c_pcm_enqueue(substream);
				rx_substream = substream;
			}

			memset(substream->dma_buffer.area, 0, BUFFER_BYTES_MAX/2+32);


			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				rtl8277c_i2s_trx_start(SNDRV_PCM_STREAM_PLAYBACK);
			} else {
				rtl8277c_i2s_trx_start(SNDRV_PCM_STREAM_CAPTURE);
			}
		}
		else{
			 rtl8277c_i2s_trx_start(SNDRV_PCM_STREAM_PLAYBACK);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		iis_counter --;

		t = sample_rate * channels * sample_bits / 8;
		if (t > 0) {
			t = 4 * 1000 * prtd->dma_period / t;// 4 period time
			if (t < 20)
				t = 20;
		} else
			t = 80;
		mdelay(t);

		if(!_ext_i2s_rx_prepare_stage_hook)
		{
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				tx_substream = NULL;
				rtl8277c_i2s_tx_stop();
			} else {
				rx_substream = NULL;
				rtl8277c_i2s_rx_stop();
			}
			if (iis_counter == 0) {
				rtl8277c_i2s_trx_stop();
			}
		}
		break;

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock(&prtd->lock);

	return ret;
}

static snd_pcm_uframes_t
rtl8277c_pcm_pointer(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rtl8277c_runtime_data *prtd = runtime->private_data;
	unsigned long res;
	snd_pcm_uframes_t ret;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		res = iis_rxpage[0]*prtd->dma_period;
	else
		res = iis_txpage[0]*prtd->dma_period;

	/* we seem to be getting the odd error from the pcm library due
	 * to out-of-bounds pointers. this is maybe due to the dma engine
	 * not having loaded the new values for the channel before being
	 * callled... (todo - fix )
	 */

	if (res >= snd_pcm_lib_buffer_bytes(substream)) {
		if (res == snd_pcm_lib_buffer_bytes(substream))
			res = 0;
	}

	ret = bytes_to_frames(substream->runtime, res);

	return ret;
}

static int rtl8277c_pcm_open(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rtl8277c_runtime_data *prtd;

	snd_soc_set_runtime_hwparams(substream, &rtl8277c_pcm_hardware);

	prtd = kzalloc(sizeof(struct rtl8277c_runtime_data), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&prtd->lock);

	runtime->private_data = prtd;
	return 0;
}

static int rtl8277c_pcm_close(struct snd_soc_component *component, struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct rtl8277c_runtime_data *prtd = runtime->private_data;

	unsigned int sample_rate = runtime->rate;
	unsigned int channels = runtime->channels;
	unsigned int sample_bits = runtime->sample_bits;
	unsigned int t=0;

	if (!prtd){
		pr_info("rtl8277c_pcm_close called with prtd == NULL\n");
	}
	else{
		kfree(prtd);
	}
	t = sample_rate * channels * sample_bits / 8;

	if (t>0) {
		t = 4 * 1000 * prtd->dma_period / t;// 4 period time
		if (t < 20)
			t = 20;
	} else
		t = 80;
	mdelay(t);

	return 0;
}

static int rtl8277c_pcm_mmap(struct snd_soc_component *component,
			  struct snd_pcm_substream *substream,
			  struct vm_area_struct *vma)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    unsigned long size, offset;
    int ret;
    /*prevent mmap from cached value*/
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    size = vma->vm_end - vma->vm_start;
    offset = vma->vm_pgoff << PAGE_SHIFT;
    ret = remap_pfn_range(vma, vma->vm_start,
            (runtime->dma_addr + offset) >> PAGE_SHIFT,
            size, vma->vm_page_prot);

	return ret;

}

static int rtl8277c_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = rtl8277c_pcm_hardware.buffer_bytes_max;
	int ret;
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	size = BUFFER_BYTES_MAX/2+32;

	ret = dma_coerce_mask_and_coherent(pcm->card->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_dbg(pcm->card->dev, "No usable DMA configuration\n");
		//return -ENOMEM;
	}
	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {

		buf->area = dma_alloc_coherent(pcm->card->dev, size,	&buf->addr, GFP_KERNEL);

		iis_tx_data = (dma_addr_t) buf->addr;
		if (buf->area == NULL)
			printk("Allocate dma coherence for tx path failed\n");
		else
			printk("Allocate dma coherence for tx path sucess, VA=[0x%px], PA=[0x%llx]", buf->area, (long long unsigned int)iis_tx_data);
	} else {
		buf->area = dma_alloc_coherent(pcm->card->dev, size, &buf->addr, GFP_KERNEL);
		iis_rx_data = (dma_addr_t) buf->addr;
		if (buf->area == NULL)
			printk("Allocate dma coherence for rx path  failed\n");
		else
			printk("Allocate dma coherence for rx path sucess, VA=[0x%px], PA=[0x%llx]\n", buf->area,  (long long unsigned int)iis_rx_data);
	}

	if (!buf->area)
		return -ENOMEM;
	buf->bytes = size;
	iis_txpage[0] = 0;
	iis_rxpage[0] = 0;
	iis_rxNextpage = 0;
	iis_txNextpage = 0;
	rtlRegWrite(IIS_RX_P0OWN, 0);
	rtlRegWrite(IIS_RX_P1OWN, 0);
	rtlRegWrite(IIS_RX_P2OWN, 0);
	rtlRegWrite(IIS_RX_P3OWN, 0);
	rtlRegWrite(IIS_TX_P0OWN, 0);
	rtlRegWrite(IIS_TX_P1OWN, 0);
	rtlRegWrite(IIS_TX_P2OWN, 0);
	rtlRegWrite(IIS_TX_P3OWN, 0);

	return 0;
}

static void rtl8277c_pcm_free_dma_buffers(struct snd_soc_component *component, struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_coherent(pcm->card->dev, buf->bytes,
		      buf->area, buf->addr);
		buf->area = NULL;
	}
}

static u64 rtl8277c_pcm_dmamask = DMA_BIT_MASK(32);

static int rtl8277c_pcm_new(struct snd_soc_component *component, struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret = 0;

	if(card->dev != NULL) {
		if (!card->dev->dma_mask) {
			card->dev->dma_mask = &rtl8277c_pcm_dmamask;
		}
		if (!card->dev->coherent_dma_mask) {
			card->dev->coherent_dma_mask = DMA_BIT_MASK(32);
		}
	}

	if(pcm) {
		if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
			ret = rtl8277c_pcm_preallocate_dma_buffer(pcm,
				SNDRV_PCM_STREAM_PLAYBACK);
			if (ret)
				goto out;
		}

		if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
			ret = rtl8277c_pcm_preallocate_dma_buffer(pcm,
				SNDRV_PCM_STREAM_CAPTURE);
			if (ret)
				goto out;
		}
	}
out:
	return ret;
}

static const struct snd_soc_component_driver rtl8277c_pcm_component = {
	.open		= rtl8277c_pcm_open,
	.close		= rtl8277c_pcm_close,
	.hw_params	= rtl8277c_pcm_hw_params,
	.hw_free	= rtl8277c_pcm_hw_free,
	.prepare	= rtl8277c_pcm_prepare,
	.trigger	= rtl8277c_pcm_trigger,
	.pointer	= rtl8277c_pcm_pointer,
	.pcm_construct	= rtl8277c_pcm_new,
	.pcm_destruct	= rtl8277c_pcm_free_dma_buffers,
	.mmap		= rtl8277c_pcm_mmap,
};

int rtl8277c_soc_platform_init(struct device *dev, int irq)
{
	request_irq(irq, iis_dma_interrupt, 0, "iis_dma", NULL);
	return devm_snd_soc_register_component(dev, &rtl8277c_pcm_component, NULL, 0);
}

void rtl8277c_soc_platform_exit(struct device *dev)
{
	snd_soc_unregister_component(dev);
}

EXPORT_SYMBOL(_ext_i2s_rx_page_done_hook);
EXPORT_SYMBOL(_ext_i2s_tx_page_done_hook);
EXPORT_SYMBOL(_ext_i2s_set_path_hook);
EXPORT_SYMBOL(_ext_i2s_rx_prepare_stage_hook);

MODULE_DESCRIPTION("Realtek I2S DMA module");
MODULE_LICENSE("GPL");
