/*
 * Cortina Specific Extensions for Synopsys DW Multimedia Card Interface driver
 *
 * Copyright (C) 2020, Cortina Accss Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "dw_mmc.h"
#include "dw_mmc-pltfm.h"
#include "dw_mmc-cortina.h"

void sd_phase_override(struct dw_mci *host, u32 regs_val)
{
	struct dw_mci_cortina_priv_data *priv = host->priv;
	u32 regs = regs_val;

	regs |= (1 << PHASE_RESET_OVERRIDE);
	writel(regs, (void *)priv->sd_dll_ctrl);
	usleep_range(1000, 1500);
	regs &= ~(1 << PHASE_RESET_OVERRIDE);
	writel(regs, (void *)priv->sd_dll_ctrl);
	usleep_range(1000, 1500);
}

static int dw_mci_cortina_set_clock(struct dw_mci *host, struct mmc_ios *ios)
{
	struct dw_mci_cortina_priv_data *priv = host->priv;
	u32 bus_hz;
	u32 io_drv_val = readl((void *)priv->g_io_drv_ctrl);
	int ret;

	if (ios->clock >= 100000000 && ios->clock < 20000000) {
		ret = clk_set_rate(host->ciu_clk, 10000000);
		io_drv_val &= ~(SD_DS_MASK << SD_DS_SHIFT);
		io_drv_val |= 0x77 << SD_DS_SHIFT;
	} else if (ios->clock >= 200000000) {
		ret = clk_set_rate(host->ciu_clk, 200000000);
		io_drv_val &= ~(SD_DS_MASK << SD_DS_SHIFT);
		io_drv_val |= 0x77 << SD_DS_SHIFT;
	} else {
		ret = clk_set_rate(host->ciu_clk, 50000000);
		io_drv_val &= ~(SD_DS_MASK << SD_DS_SHIFT);
		io_drv_val |= 0x33 << SD_DS_SHIFT;
		if (!IS_ERR(priv->drv_clk)) {
			if (!IS_ERR(priv->drv_clk))
				clk_set_phase(priv->drv_clk, 0x14);
			if (!IS_ERR(priv->smpl_clk))
				clk_set_phase(priv->smpl_clk, 0);
		}
	}

	if (ret)
		dev_warn(host->dev, "failed to set rate %uHz\n", ios->clock);

	writel(io_drv_val, (void *)priv->g_io_drv_ctrl);
	usleep_range(10, 15);

	bus_hz = clk_get_rate(host->ciu_clk);
	if (bus_hz != host->bus_hz) {
		host->bus_hz = bus_hz;
		host->current_speed = 0;
	}

	dev_dbg(host->dev, "bus_hz %uHz timing = %u, clock %u, io_drv %x\n",
		host->bus_hz, ios->timing, ios->clock, io_drv_val);

	return 0;
}

static void dw_mci_cortina_set_ios(struct dw_mci *host, struct mmc_ios *ios)
{
	struct dw_mci_cortina_priv_data *priv = host->priv;

	if (ios->timing == MMC_TIMING_UHS_SDR50 ||
	    ios->timing == MMC_TIMING_UHS_SDR104) {
		set_bit(DW_MMC_CARD_NO_USE_HOLD, &host->slot->flags);
	} else {
		clear_bit(DW_MMC_CARD_NO_USE_HOLD, &host->slot->flags);
	}

	priv->ios = ios;

	dw_mci_cortina_set_clock(host, ios);
}

static int dw_mci_cortina_clk_drv_phase_tuning(struct dw_mci_slot *slot,
					       u32 opcode)
{
	struct dw_mci *host = slot->host;
	struct mmc_host *mmc = slot->mmc;
	struct dw_mci_cortina_priv_data *priv = host->priv;
	u32 regs;
	u8 phase_level;
	u8 i, j;
	u8 *good_phase;
	int tmp_range = 0, max_range = 0, begin_phase = 0,
	    mid_phase = 0, err = 0;

	if (IS_ERR(priv->drv_clk)) {
		dev_err(host->dev, "drv clk not defined\n");
		return -EIO;
	}

	if (priv->ios->clock == 200000000) {
		phase_level = 20;
	} else if (priv->ios->clock == 100000000) {
		phase_level = 40;
	} else {
		dev_err(host->dev, "%s can't support frequency %u\n",
			__func__,  priv->ios->clock);
		return -EINVAL;
	}

	good_phase = kzalloc(phase_level * sizeof(u8), GFP_KERNEL);
	if (!good_phase)
		return -ENOMEM;

	for (i = 0; i < phase_level; i++) {
		clk_set_phase(priv->drv_clk, i);

		if (!mmc_send_tuning(mmc, opcode, NULL))
			good_phase[i] = 1;
	}

	for (i = 0; i < phase_level; i++) {
		tmp_range = 0;

		for (j = 0; j < phase_level; j++) {
			u8 tmp = i + j;

			if (tmp >= phase_level)
				tmp -= phase_level;
			if (good_phase[tmp] == 1)
				tmp_range++;
			else
				break;
		}

		if (tmp_range > max_range) {
			max_range = tmp_range;
			begin_phase = i;
		}
	}

	if (max_range < 2) {
		err = -EIO;
		goto OUT;
	}

	// find middle phase
	mid_phase = begin_phase  + max_range / 2;
	if (mid_phase >= phase_level)
		mid_phase -= phase_level;

	clk_set_phase(priv->drv_clk, mid_phase);

	priv->cur_phase_drv = mid_phase;

	dev_dbg(host->dev, "drv phase %d, regs 0x%08x, begin %d, range %d\n",
		priv->cur_phase_drv, regs, begin_phase, max_range);

OUT:
	kfree(good_phase);
	return err;
}

static int dw_mci_cortina_clk_smpl_phase_tuning(struct dw_mci_slot *slot,
						u32 opcode)
{
	struct dw_mci *host = slot->host;
	struct mmc_host *mmc = slot->mmc;
	struct dw_mci_cortina_priv_data *priv = host->priv;

	u32 regs;
	u8 phase_level;
	u8 i, j;
	u8 *good_phase;
	int tmp_range = 0, max_range = 0, begin_phase = 0,
	    mid_phase = 0, err = 0;

	if (IS_ERR(priv->smpl_clk)) {
		dev_err(host->dev, "smpl clk not defined\n");
		return -EIO;
	}

	if (priv->ios->clock == 200000000) {
		phase_level = 20;
	} else if (priv->ios->clock == 100000000) {
		phase_level = 40;
	} else {
		dev_err(host->dev, "%s can't support frequency %u\n",
			__func__, priv->ios->clock);
		return -EINVAL;
	}

	good_phase = kzalloc(phase_level * sizeof(u8), GFP_KERNEL);
	if (!good_phase)
		return -ENOMEM;

	for (i = 0; i < phase_level; i++) {
		clk_set_phase(priv->smpl_clk, i);

		if (!mmc_send_tuning(mmc, opcode, NULL))
			good_phase[i] = 1;
	}

	for (i = 0; i < phase_level; i++) {
		tmp_range = 0;

		for (j = 0; j < phase_level; j++) {
			u8 tmp = i + j;

			if (tmp >= phase_level)
				tmp -= phase_level;
			if (good_phase[tmp] == 1)
				tmp_range++;
			else
				break;
		}

		if (tmp_range > max_range) {
			max_range = tmp_range;
			begin_phase = i;
		}
	}

	if (max_range < 2) {
		err = -EIO;
		goto OUT;
	}

	// find middle phase
	mid_phase = begin_phase  + max_range / 2;
	if (mid_phase >= phase_level)
		mid_phase -= phase_level;

	clk_set_phase(priv->smpl_clk, mid_phase);

	priv->cur_phase_smpl = mid_phase;

	dev_dbg(host->dev, "smpl phase %d, regs 0x%08x, begin %d, range %d\n",
		priv->cur_phase_smpl, regs, begin_phase, max_range);

OUT:
	kfree(good_phase);
	return err;
}

static int dw_mci_cortina_execute_tuning(struct dw_mci_slot *slot, u32 opcode)
{
	int ret = 0;

	ret = dw_mci_cortina_clk_smpl_phase_tuning(slot, opcode);
	if (ret) {
		dev_err(slot->host->dev, "clk_smpl_phase_tuning fail\n");
		goto tuning_error;
	}

	ret = dw_mci_cortina_clk_drv_phase_tuning(slot, opcode);
	if (ret) {
		dev_err(slot->host->dev, "clk_drv_phase_tuning fail\n");
		goto tuning_error;
	}

tuning_error:
	return ret;
}

static int dw_mci_cortina_init(struct dw_mci *host)
{
	struct dw_mci_cortina_priv_data *priv;
	struct device_node *node = host->dev->of_node;
	u32 regs = 0;

	priv  = devm_kzalloc(host->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	of_property_read_u32(node, "g_io_drv_ctrl", &regs);
	priv->g_io_drv_ctrl = ioremap(regs, 4);
	if (IS_ERR(priv->g_io_drv_ctrl)) {
		dev_err(host->dev, "%s(%d) ioremap 0x%08x fail\n",
			__func__, __LINE__, regs);
		goto ERR;
	}

	priv->smpl_clk = of_clk_get_by_name(node, "sd-smpl");
	if (IS_ERR(priv->smpl_clk)) {
		dev_err(host->dev, "%s(%d) no smpl clk\n", __func__, __LINE__);
		goto ERR;
	}

	priv->drv_clk = of_clk_get_by_name(node, "sd-drv");
	if (IS_ERR(priv->drv_clk)) {
		dev_err(host->dev, "%s(%d) no drv clk\n", __func__, __LINE__);
		goto ERR;
	}

	host->priv = priv;

	return 0;
ERR:
	iounmap(priv->g_io_drv_ctrl);

	return -EINVAL;
}

#ifdef CONFIG_PM_SLEEP
static int dw_mci_cortina_suspend(struct device *dev)
{
	return dw_mci_runtime_suspend(dev);
}

static int dw_mci_cortina_resume(struct device *dev)
{
	return dw_mci_runtime_resume(dev);
}

#else
#define dw_mci_cortina_suspend		NULL
#define dw_mci_cortina_resume		NULL
#endif /* CONFIG_PM_SLEEP */

static const struct dw_mci_drv_data ca_drv_data = {
	.init = dw_mci_cortina_init,
	.execute_tuning = dw_mci_cortina_execute_tuning,
	.set_ios = dw_mci_cortina_set_ios,
};

static const struct of_device_id dw_mci_cortina_match[] = {
	{
		.compatible = "cortina,dw-mshc",
		.data = &ca_drv_data,
	},

	{},
};
MODULE_DEVICE_TABLE(of, dw_mci_cortina_match);

static ssize_t
dw_mci_show_cmd_info(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct dw_mci_cortina_priv_data *priv = host->priv;

	priv->debug_flag = ~priv->debug_flag & 0x1;
	dev_info(host->dev, "%s debug %s\n", __func__,
		 (priv->debug_flag & 0x1) ? "on" : "off");

	return 0;
}

static DEVICE_ATTR(show_cmd_info, 0444, dw_mci_show_cmd_info, NULL);

static int dw_mci_cortina_probe(struct platform_device *pdev)
{
	const struct dw_mci_drv_data *drv_data;
	const struct of_device_id *match;
	int ret;

	if (device_create_file(&pdev->dev, &dev_attr_show_cmd_info))
		pr_warn("%s: unable to show cmd info\n", __func__);

	match = of_match_node(dw_mci_cortina_match, pdev->dev.of_node);
	drv_data = match->data;

	ret = dw_mci_pltfm_register(pdev, drv_data);
	if (ret)
		pr_err("%s: can't register dw_mmc-cortina\n", __func__);

	return ret;
}

static const struct dev_pm_ops dw_mci_cortina_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(dw_mci_cortina_suspend, dw_mci_cortina_resume)
};

static struct platform_driver dw_mci_cortina_pltfm_driver = {
	.probe		= dw_mci_cortina_probe,
	.remove		= dw_mci_pltfm_remove,
	.driver		= {
		.name		= "dwmmc_cortina",
		.of_match_table	= dw_mci_cortina_match,
		.pm		= &dw_mci_cortina_pmops,
	},
};

module_platform_driver(dw_mci_cortina_pltfm_driver);

MODULE_DESCRIPTION("Cortina Specific DW-MSHC Driver Extension");
MODULE_AUTHOR("Arthur Li <arthur.li@cortina-access.com");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:dwmmc_cortina");
