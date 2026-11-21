// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Realtek
 *
 * GPIO Driver for Realtek of SoCs
 */

#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <linux/compat.h>
#include <linux/compiler.h>
#include <linux/delay.h>

/* GPIO Register Map */
#define REALTEK_GPIO_CFG	0x00
#define REALTEK_GPIO_OUT	0x04
#define REALTEK_GPIO_IN		0x08
#define REALTEK_GPIO_LVL	0x0C
#define REALTEK_GPIO_EDGE	0x10
#define REALTEK_GPIO_BOTHEDGE	0x14
#define REALTEK_GPIO_IE		0x18
#define REALTEK_GPIO_INT	0x1C
#define REALTEK_GPIO_STAT	0x20

struct realtek_gpio_bank {
	void __iomem *base;
	void __iomem *gpio_mux;
};

#ifdef CONFIG_DM_GPIO
static int rtk_gpio_direction_input(struct udevice *dev, unsigned int offset)
{
	struct realtek_gpio_bank *priv = dev_get_priv(dev);

	setbits_32(priv->gpio_mux, BIT(offset));
	setbits_32(priv->base, BIT(offset));
	mdelay(1);

	return 0;
}

static int
rtk_gpio_direction_output(struct udevice *dev, unsigned int offset, int value)
{
	struct realtek_gpio_bank *priv = dev_get_priv(dev);

	setbits_32(priv->gpio_mux, BIT(offset));
	clrbits_32(priv->base, BIT(offset));
	if (value)
		setbits_32(priv->base + REALTEK_GPIO_OUT, BIT(offset));
	else
		clrbits_32(priv->base + REALTEK_GPIO_OUT, BIT(offset));

	return 0;
}

static int rtk_gpio_get_value(struct udevice *dev, unsigned int offset)
{
	struct realtek_gpio_bank *priv = dev_get_priv(dev);

	return (readl(priv->base + REALTEK_GPIO_IN) & BIT(offset)) ? 1 : 0;
}

static int rtk_gpio_set_value(struct udevice *dev, unsigned int offset, int value)
{
	struct realtek_gpio_bank *priv = dev_get_priv(dev);

	if (value)
		setbits_32(priv->base + REALTEK_GPIO_OUT, BIT(offset));
	else
		clrbits_32(priv->base + REALTEK_GPIO_OUT, BIT(offset));

	return 0;
}

static int rtk_gpio_get_function(struct udevice *dev, unsigned int offset)
{
	struct realtek_gpio_bank *priv = dev_get_priv(dev);

	if (readl(priv->base) & BIT(offset))
		return GPIOF_INPUT;
	else
		return GPIOF_OUTPUT;
}

static const struct dm_gpio_ops gpio_realtek_ops = {
	.direction_input = rtk_gpio_direction_input,
	.direction_output = rtk_gpio_direction_output,
	.get_value = rtk_gpio_get_value,
	.set_value = rtk_gpio_set_value,
	.get_function = rtk_gpio_get_function,
};

static int rtk_gpio_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct realtek_gpio_bank *priv = dev_get_priv(dev);

	priv->base = dev_remap_addr_index(dev, 0);
	if (!priv->base)
		return -EINVAL;
	priv->gpio_mux = dev_remap_addr_index(dev, 1);
	if (!priv->gpio_mux)
		return -EINVAL;

	uc_priv->gpio_count = dev_read_u32_default(dev, "ngpios", 32);
	uc_priv->bank_name = dev->name;

	debug("Done Realtek GPIO init\n");
	return 0;
}

static const struct udevice_id rtk_gpio_ids[] = {
	{.compatible = "realtek,rtk-gpio"},
	{}
};

U_BOOT_DRIVER(realtek_gpio) = {
	.name = "realtek-gpio",
	.id = UCLASS_GPIO,
	.ops = &gpio_realtek_ops,
	.probe = rtk_gpio_probe,
	.priv_auto = sizeof(struct realtek_gpio_bank),
	.of_match = rtk_gpio_ids,
};
#endif /* CONFIG_DM_GPIO */
