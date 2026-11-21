#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <soc/cortina/registers.h>
#include <sound/rtl8277x_snd_audio.h>
#include <linux/delay.h>
#include "rtl8277x_snd_audio_reg.h"

#if defined(CONFIG_ARCH_REALTEK_TAURUS) || defined(CONFIG_ARCH_REALTEK_9607F)  || defined(CONFIG_ARCH_REALTEK_MERCURY)
#define SSP_PINCTRL
#define DRV_NAME	"ssp-pinctrl"
#endif

static int rtl8277x_pinctrl_probe(struct platform_device *pdev)
{

	//platform_driver_register(&rtl8277x_pinctrl_driver);

	return 0;

fail:
	return -ENODEV;
}

static int rtl8277x_pinctrl_remove(struct platform_device *pdev)
{
	//platform_driver_unregister(&rtl8277x_pinctrl_driver);
	
	return 0;
}

static const struct of_device_id rtl8277x_pinctrl_match[] = {
	{ .compatible = "realtek,ssp_pinctrl", },
	{ },
};
MODULE_DEVICE_TABLE(of, rtl8277x_pinctrl_match);

static struct platform_driver rtl8277x_pinctrl_driver = {
	.probe = rtl8277x_pinctrl_probe,
	.remove = rtl8277x_pinctrl_remove,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = of_match_ptr(rtl8277x_pinctrl_match),
	},
};
module_platform_driver(rtl8277x_pinctrl_driver);

int rtl8277x_pinctrl_init(void)
{
	printk("%s\n", __func__);
	return platform_driver_register(&rtl8277x_pinctrl_driver);
}


void rtl8277x_pinctrl_exit(void)
{
	printk("%s\n", __func__);
	platform_driver_unregister(&rtl8277x_pinctrl_driver);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("rtl8277x AUDIO PIN CTRL driver");
