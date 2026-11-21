/*
 * rtl8197d-i2s.c  --  ALSA Soc Audio Layer
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/info.h>
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
#include "soc/cortina/rtl8277c_registers.h"
#include "rtl8277c-pcm.h"
#include "rtl8277c-i2s.h"

#define CONFIG_I2S_SET_PINMUX	1
#define DEBUG_ENABLE 					0

void __iomem *IIS_BASE;

static const struct of_device_id realtek_i2s_of_match_table[] = {
	{ .compatible = "realtek,i2s", },
	{ },
};

#if DEBUG_ENABLE
  #define DEBUG(format, args...) printk("[%s:%d] "format, __FILE__, __LINE__, ##args)
#else
  #define DEBUG(args...)
#endif
#define I2S_PRINT(format, args...) printk("[%s:%d] "format, __FILE__, __LINE__, ##args)
#define IIS_PAGE_NUM	4
#define IIS_PAGE_SIZE	(80*3)	// 80 * 32bit, 160sample, 20ms

/*
 * Check whether CPU is the master or slave
 */
static inline int rtl8277c_snd_is_clkmaster(void)
{
	return 1; // 8197d alway master
}

/*
 * Set rtl8277c I2S DAI format
 */
static int rtl8277c_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	return 0;
}

static int rtl8277c_i2s_startup(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	DEBUG("\n");

	return 0;
}

static int rtl8277c_i2s_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	DEBUG("\n");

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_BE:
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S24_BE:
		break;
	}

	return 0;
}

static int rtl8277c_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
			       struct snd_soc_dai *dai)
{
	int ret = 0;
	
	DEBUG("\n");
	return ret;
}

/*
 * Set S3C24xx Clock source
 */
static int rtl8277c_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
	int clk_id, unsigned int freq, int dir)
{
	DEBUG("\n");
	return 0;
}

/*
 * Set S3C24xx Clock dividers
 */
static int rtl8277c_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
	int div_id, int div)
{
	DEBUG("\n");
	return 0;
}

static void rtl8277c_i2s_shutdown(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
    DEBUG ("rtl8277c_i2s_shutdown.\n");
}

static struct snd_soc_dai_ops 	rtl8277c_i2s_dai_ops = {
	.trigger	= rtl8277c_i2s_trigger,
	.hw_params	= rtl8277c_i2s_hw_params,
	.set_fmt	= rtl8277c_i2s_set_fmt,
	.startup	= rtl8277c_i2s_startup,
	.shutdown	= rtl8277c_i2s_shutdown,
	.set_clkdiv	= rtl8277c_i2s_set_clkdiv,
	.set_sysclk	= rtl8277c_i2s_set_sysclk,
};

struct snd_soc_dai_driver rtl8277c_i2s_dai = {

		.name = "rtl8277c-iis",
		.id = 0,
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,	
			.formats = SNDRV_PCM_FMTBIT_S16_BE | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,	
			.formats = SNDRV_PCM_FMTBIT_S16_BE | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &rtl8277c_i2s_dai_ops,
};
EXPORT_SYMBOL_GPL(rtl8277c_i2s_dai);

static const struct snd_soc_component_driver rtl8277c_i2s_component = {
	.name		= "rtl8277c-iis",
};

static unsigned int bypass_clk_init=0;
static int rtl8277c_iis_dev_probe(struct platform_device *pdev)
{
	void __iomem *psds_reg;
	void __iomem *glb_reg;
	int irq;
	unsigned int sds_mode_s0 = 0;
	GLOBAL_PIN_MUX_NEW_t glb_pimux_new;
	PSDS_MODE_t psds_mode;
	PSDS_MISC_CNTL_1_t psds_misc_cntl_1;
	PSDS_CH0_ABILITY_t psds_ch0_ability;
	GLOBAL_PON_CNTL_t glb_pon_cntl;
	GLOBAL_PSDS_INIT_CNTL_t glb_psds_init_cntl;
	GLOBAL_BLOCK_RESET_EXT_t glb_block_reset_ext;
	IISCR_t iiscr;
	IIS_TX_ISR_t iis_isr_t;
	IIS_RX_ISR_t iis_isr_r;
	IIS_TX_IMR_t iis_imr_t;
	IIS_RX_IMR_t iis_imr_r;
	IIS_SETTING_t iis_set;
	struct resource	*res;
	int ret = 0;

	//DEBUG("%s, %d\n", __FUNCTION__, __LINE__);

	/* get resource from device tree */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "psds_reg");
	psds_reg = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(psds_reg))
		return PTR_ERR(psds_reg);

	//printk("psds_reg %px to %px\n", (void *) res->start, psds_reg);
	
	/* get glb_reg from device tree */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,  "glb_reg");
	glb_reg = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(glb_reg))
		return PTR_ERR(glb_reg);

	//printk("glb_reg %px to %px\n", (void *) res->start, glb_reg);

	/* get i2s_reg from device tree */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "i2s_reg");
	IIS_BASE = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(IIS_BASE))
		return PTR_ERR(IIS_BASE);

	//printk("IIS_BASE %px to %px\n", (void *) res->start, IIS_BASE);

#if CONFIG_I2S_SET_PINMUX
	/* pinmux setup */
	glb_pimux_new.wrd = rtlRegRead(glb_reg + (GLOBAL_PIN_MUX_NEW - GLOBAL_JTAG_ID));
	glb_pimux_new.bf.iomux_i2s_mst_enable = 1;
	glb_pimux_new.bf.iomux_i2s_slv_enable = 0;
	glb_pimux_new.bf.iomux_jtag_enable = 0;

	glb_pimux_new.bf.iomux_biw0_enable = glb_pimux_new.bf.iomux_biw0_enable & ~(0x2);
	glb_pimux_new.bf.iomux_biw1_enable = glb_pimux_new.bf.iomux_biw1_enable & ~(0x2);

	rtlRegWrite(glb_reg + (GLOBAL_PIN_MUX_NEW - GLOBAL_JTAG_ID), glb_pimux_new.wrd);

	rtlRegMask(glb_reg + (GLOBAL_GPIO_MUX_4 - GLOBAL_JTAG_ID), 
		GLOBAL_GPIO_MUX_4_8 | GLOBAL_GPIO_MUX_4_9 | GLOBAL_GPIO_MUX_4_10 | GLOBAL_GPIO_MUX_4_11, 0);
#endif
	/* init clock */
	mdelay(100);
	psds_mode.wrd = rtlRegRead(psds_reg + (PSDS_MODE - PSDS_PON_PRBS_CONTROL));
	sds_mode_s0 = psds_mode.bf.sds_mode_s0;

	if (bypass_clk_init == 1) {

		if (sds_mode_s0 == PSDS_MODE_SDS_MODE_S0_ILLEGAL) {
			printk("%s: CLK not initialized, sds_mode_s0 : %x\n", __func__, sds_mode_s0);
			psds_mode.bf.sd_s0 = 0;
			psds_mode.bf.sds_mode_s0 = PSDS_MODE_SDS_MODE_S0_SET;
			rtlRegWrite(psds_reg + (PSDS_MODE - PSDS_PON_PRBS_CONTROL), psds_mode.wrd);
			psds_misc_cntl_1.wrd = rtlRegRead(psds_reg + (PSDS_MISC_CNTL_1 - PSDS_PON_PRBS_CONTROL));
			psds_misc_cntl_1.bf.cfg_fb_on = 1;
			rtlRegWrite(psds_reg + (PSDS_MISC_CNTL_1 - PSDS_PON_PRBS_CONTROL), psds_misc_cntl_1.wrd);
			psds_ch0_ability.wrd = rtlRegRead(psds_reg + (PSDS_CH0_ABILITY - PSDS_PON_PRBS_CONTROL));
			psds_ch0_ability.bf.sds01_sds0_ablty = PSDS_CH0_ABILTY_SDS01_SDS0_ABLITY_SET;
			rtlRegWrite(psds_reg + (PSDS_CH0_ABILITY - PSDS_PON_PRBS_CONTROL), psds_misc_cntl_1.wrd);
			printk("%s: check CLK setting 0x19c00 : 0x%x\n", __func__, 
				rtlRegRead(psds_reg + (PSDS_RGB8 - PSDS_PON_PRBS_CONTROL)));
		} else {
			printk("%s: CLK already initialized, sds_mode_s0 : %u\n", __func__, sds_mode_s0);
		}
		
		glb_pon_cntl.wrd = rtlRegRead(glb_reg + (GLOBAL_PON_CNTL - GLOBAL_JTAG_ID));
		glb_pon_cntl.bf.pdc_reset = 1;
		glb_pon_cntl.bf.puc_reset = 1;
		glb_pon_cntl.bf.ptp_rst_n = 1;
		glb_pon_cntl.bf.psds_reg_rst_n = 1;
		glb_pon_cntl.bf.pon_serdes_rst_n = 1;
		glb_pon_cntl.bf.epon_mode = 1;
		rtlRegWrite(glb_reg + (GLOBAL_PON_CNTL - GLOBAL_JTAG_ID), glb_pon_cntl.wrd);

		glb_psds_init_cntl.wrd = rtlRegRead(glb_reg + (GLOBAL_PSDS_INIT_CNTL - GLOBAL_JTAG_ID));
		glb_psds_init_cntl.bf.POW_PCIX = 1;
		rtlRegWrite(glb_reg + (GLOBAL_PSDS_INIT_CNTL - GLOBAL_JTAG_ID), glb_psds_init_cntl.wrd);
	}
	/* enable I2S */
	glb_block_reset_ext.wrd = rtlRegRead(glb_reg + (GLOBAL_BLOCK_RESET_EXT - GLOBAL_JTAG_ID));
	glb_block_reset_ext.bf.en_i2s = 1;
	glb_block_reset_ext.bf.reset_i2s = 0;
	rtlRegWrite(glb_reg + (GLOBAL_BLOCK_RESET_EXT - GLOBAL_JTAG_ID), glb_block_reset_ext.wrd);

	iiscr.wrd = rtlRegRead(IISCR);
	iiscr.bf.sw_rstn = 1;
	rtlRegWrite(IISCR, iiscr.wrd);
	mdelay(1);
	iiscr.bf.sw_rstn = 0;
	rtlRegWrite(IISCR, iiscr.wrd);
	mdelay(1);
	iiscr.bf.sw_rstn = 1;
	rtlRegWrite(IISCR, iiscr.wrd);

	iis_isr_t.wrd = 0;
	iis_isr_t.bf.p0okip_tx = 1;
	iis_isr_t.bf.p1okip_tx = 1;
	iis_isr_t.bf.p2okip_tx = 1;
	iis_isr_t.bf.p3okip_tx = 1;
	iis_isr_t.bf.p0unva_ip_tx = 1;
	iis_isr_t.bf.fifo_empty_ip_tx = 1;
	rtlRegWrite(IIS_TX_ISR, iis_isr_t.wrd);
	iis_isr_r.wrd = 0;
	iis_isr_r.bf.p0okip_rx = 1;
	iis_isr_r.bf.p1okip_rx = 1;
	iis_isr_r.bf.p2okip_rx = 1;
	iis_isr_r.bf.p3okip_rx = 1;
	iis_isr_r.bf.p0unva_ip_rx = 1;
	iis_isr_r.bf.fifo_empty_ip_rx = 1;
	rtlRegWrite(IIS_RX_ISR, iis_isr_r.wrd);

	/* set brust size */
	iiscr.wrd = rtlRegRead(IISCR);
	iiscr.bf.burst_size = 0x7; /* set 8-bits brust */
	rtlRegWrite(IISCR, iiscr.wrd);

	rtlRegWrite(IIS_TX_P0OWN, BIT(31));
	rtlRegWrite(IIS_TX_P1OWN, BIT(31));
	rtlRegWrite(IIS_TX_P2OWN, BIT(31));
	rtlRegWrite(IIS_TX_P3OWN, BIT(31));

	iis_imr_t.wrd = 0;
	iis_imr_t.bf.p0okie_tx = 1;
	iis_imr_t.bf.p1okie_tx = 1;
	iis_imr_t.bf.p2okie_tx = 1;
	iis_imr_t.bf.p3okie_tx = 1;
	rtlRegWrite(IIS_TX_IMR, iis_imr_t.wrd);
	iis_imr_r.wrd = 0;
	iis_imr_r.bf.p0okie_rx = 1;
	iis_imr_r.bf.p1okie_rx = 1;
	iis_imr_r.bf.p2okie_rx = 1;
	iis_imr_r.bf.p3okie_rx = 1;
	rtlRegWrite(IIS_RX_IMR, iis_imr_r.wrd);

	iis_set.wrd = 0;
	iis_set.bf.page_szie = 199 ;
	iis_set.bf.page_num = IIS_PAGE_NUM - 1;
	iis_set.bf.sr = 1;
	rtlRegWrite(IIS_SETTING, iis_set.wrd);	/* set page size */
	/*register platform device*/

		/* setup I2S */

	ret = snd_soc_register_component(&pdev->dev, &rtl8277c_i2s_component, &rtl8277c_i2s_dai,  1 );
	if (ret) {
		pr_err("failed to register the dai\n");
		printk("%s, %d \n", __FUNCTION__, __LINE__);
		return ret;
	}
	else{
		printk("[%s][%d] register RTL8277c_i2s_dai  success \n",__FUNCTION__, __LINE__);
	}
	irq = platform_get_irq(pdev, 0);
	printk("[%s][%d] Get i2s irq %d success \n",__FUNCTION__, __LINE__, irq);
	ret = rtl8277c_soc_platform_init(&pdev->dev, irq);
	if (ret) {
		pr_err("failed to register the dma: %d\n", ret);
		goto err;
	}

	return 0;
err:
	snd_soc_unregister_component(&pdev->dev);
	return ret;
}

static int rtl8277c_iis_dev_remove(struct platform_device *pdev)
{
	rtl8277c_soc_platform_exit(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

struct platform_driver rtl8277c_iis_driver = {
	.probe  = rtl8277c_iis_dev_probe,
	.remove = rtl8277c_iis_dev_remove,
	.driver = {
		.name = "rtl8277c-iis",
		.owner = THIS_MODULE,
		.of_match_table = realtek_i2s_of_match_table,
	},
};
module_platform_driver(rtl8277c_iis_driver);
module_param(bypass_clk_init, int, 0);
/* Module information */
MODULE_DESCRIPTION("Realtek I2S DMA module");
MODULE_LICENSE("GPL");

