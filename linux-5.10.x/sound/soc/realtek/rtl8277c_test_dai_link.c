/*
 * rtl819xD_rt5621.c - Realtek machine ASoC driver.
 *
 * Author: Johnny Hsu <johnnyhsu@realtek.com>
 * Copyright 2012 Realtek Semiconductor Corp.
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
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/pcm_params.h>
#include "rtl8277c-pcm.h"

#include "rtl8277c-i2s.h"

static int rtl8277c_rt5621_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	#if 0
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	unsigned int clk = 0;
	int bfs, rfs, ret = 0;

	pr_debug("%s rate %d format %x\n", __func__, params_rate(params),
		params_format(params));

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_U8:
	case SNDRV_PCM_FORMAT_S8:
		bfs = 16;
		break;
	case SNDRV_PCM_FORMAT_U16_LE:
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_S16_BE:
	case SNDRV_PCM_FORMAT_S32_LE:
	case SNDRV_PCM_FORMAT_S32_BE:
		bfs = 32;
		break;
	default:
		return -EINVAL;
	}

	rfs = 256;

	clk = params_rate(params) * rfs;

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);

	if (ret < 0){
		return ret;
	}
	#endif
	return 0;
}

static struct snd_soc_ops rtl8277c_rt5621_ops = {
	.hw_params = rtl8277c_rt5621_hw_params,
};

static const struct snd_soc_dapm_widget rtl8277c_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Int Mic", NULL),
	SND_SOC_DAPM_INPUT("input"),
};

static const struct snd_soc_dapm_route rtl8277c_audio_map[] = {
	{"input", NULL, "Int Mic"},
};

#if 1
/*For test used, we do not connect to any codec, this dai link only for loopback test used */
SND_SOC_DAILINK_DEFS(rt5621,
	DAILINK_COMP_ARRAY(COMP_CPU("rtl8277c-iis")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("f7100000.i2s")));
#endif
static struct snd_soc_dai_link rtl8277c_rt5621_dai[] = {
	{ 
		.name = "rtl8277c_rt5621_dai",
		.stream_name = "AIF1 Playback",
		.ops = &rtl8277c_rt5621_ops,
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBS_CFS,
		SND_SOC_DAILINK_REG(rt5621),

	},
};

static struct snd_soc_card rtl8277c_rt5621 = {
	.name = "rtl8277c_to_dummy",
	.owner = THIS_MODULE,
	.dai_link = rtl8277c_rt5621_dai,
	.num_links = 1,

	.dapm_widgets = rtl8277c_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(rtl8277c_dapm_widgets),

	.dapm_routes = rtl8277c_audio_map,
	.num_dapm_routes = ARRAY_SIZE(rtl8277c_audio_map),

};

#if 0
/*for kernel 5.10 */
static struct platform_device *rtl8277c_snd_device;

static int __init rtl8277c_rt5621_modinit(void)
{
	int ret=0;
	printk("%s, %d\n", __FUNCTION__, __LINE__);
	rtl8277c_snd_device = platform_device_alloc("soc-audio", -1);
	if (!rtl8277c_snd_device)
		return -ENOMEM;

	platform_set_drvdata(rtl8277c_snd_device, &rtl8277c_rt5621);
	//rtl8277c_rt5621.dev = &rtl8277c_snd_device->dev;
	ret = platform_device_add(rtl8277c_snd_device);

	if (ret){
		printk("%s, %d, ret=%d\n", __FUNCTION__, __LINE__,ret);
		platform_device_put(rtl8277c_snd_device);
	}
	printk("%s, %d, ret=%d\n", __FUNCTION__, __LINE__,ret);
	return 0;
}
module_init(rtl8277c_rt5621_modinit);

static void __exit rtl8277c_rt5621_modexit(void)
{
	platform_device_unregister(rtl8277c_snd_device);
}
module_exit(rtl8277c_rt5621_modexit);
#else


static int rtl8277c_rt5621_dev_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &rtl8277c_rt5621;
	struct device_node *platform_node;
	int i,platform_index, ret;

	dev_dbg(&pdev->dev, "rtl8277c_rt5621_dev_probe started\n");
	platform_node = of_parse_phandle(pdev->dev.of_node, "realtek,platform", 0);
	if (!platform_node) {
		dev_err(&pdev->dev, "Property 'platform' missing or invalid\n");
		return -EINVAL;
	}

	for (i = 0; i < card->num_links; i++) {
		for(platform_index=0; platform_index< rtl8277c_rt5621_dai[i].num_platforms;platform_index++)
		{
			if (rtl8277c_rt5621_dai[i].platforms[platform_index].name)
				continue;
			rtl8277c_rt5621_dai[i].platforms[platform_index].of_node = platform_node;
		}
	}

	card->dev = &pdev->dev;
	platform_set_drvdata(pdev, card);

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(&pdev->dev, "%s snd_soc_register_card fail %d\n",
			__func__, ret);
	}
	else{
		printk("snd_soc_card's name  [%s] registered success \n", rtl8277c_rt5621.name);
	}
	return ret;
}
/* this .compatible's string  is same as dtsi device node's .compatible attribute */
static const struct of_device_id rtl8277c_rt5621_dt_match[] = {
	{
		//.compatible = "realtek,rtl819x_rt5621", },
		.compatible = "realtek, sndcard_rtl8277c_dummy", },
	{ }
};
MODULE_DEVICE_TABLE(of, rtl8277c_rt5621_dt_match);

static struct platform_driver rtl8277c_rt5621_driver = {
	.driver = {
		   .name = "rtk-rtl8277c-rt5621",
		   .of_match_table = rtl8277c_rt5621_dt_match,
	},
	.probe = rtl8277c_rt5621_dev_probe,
};

module_platform_driver(rtl8277c_rt5621_driver);

#endif

MODULE_DESCRIPTION("ASoC RT5621 driver");

MODULE_LICENSE("GPL");
