// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Driver for Cortina Access SPI Controllers
 *
 * This driver is inspired by:
 * spi-bcm2835.c, Copyright (C) 2015 Martin Sperl
 */

#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/spi/spi.h>
#include <linux/iopoll.h>

/* SPI register offsets */
#define CORTINA_SPI_CLK			0x00
#define CORTINA_SPI_CFG			0x04
#define CORTINA_SPI_MODE		0x08
#define CORTINA_SPI_CTRL		0x0C
#define CORTINA_SPI_CA0			0x10
#define CORTINA_SPI_CA1			0x14
#define CORTINA_SPI_CA2			0x18
#define CORTINA_SPI_WDAT1		0x1C
#define CORTINA_SPI_WDAT0		0x20
#define CORTINA_SPI_RDAT1		0x24
#define CORTINA_SPI_RDAT0		0x28
#define CORTINA_SPI_IE0			0x2C
#define CORTINA_SPI_INT0		0x30
#define CORTINA_SPI_IE1			0x34
#define CORTINA_SPI_INT1		0x38
#define CORTINA_SPI_STAT		0x3C

/* Bitfields in CORTINA_SPI_CFG */
#define CORTINA_SPI_CFG_CC		0x00000001
#define CORTINA_SPI_CFG_WR		0x00000002
#define CORTINA_SPI_CFG_MW_OFF		2
#define CORTINA_SPI_CFG_SS_OFF		8
#define CORTINA_SPI_CFG_SC_OFF		16
#define CORTINA_SPI_CFG_PSD_OFF		24

/* Bitfields in CORTINA_SPI_MODE */
#define CORTINA_SPI_MODE_CPOL		0x00000001
#define CORTINA_SPI_MODE_CPHA		0x00000002
#define CORTINA_SPI_MODE_CMDS		0x00000004
#define CORTINA_SPI_MODE_ISAM		0x00000008

/* Bitfields in CORTINA_SPI_CTRL */
#define CORTINA_SPI_CTRL_DONE		0x00000001
#define CORTINA_SPI_CTRL_START		0x00000080

/* Bitfields in CORTINA_SPI_IE/CORTINA_SPI_INT */
#define CORTINA_SPI_INT_INT		0x00000001

#define CORTINA_SPI_FIFO_SIZE		8
#define CORTINA_SPI_NUM_CS		8  /* raise as necessary */
#define CORTINA_SPI_MODE_BITS	(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_NO_CS)

#define DRV_NAME	"spi-cortina"

/* define polling limits */
static unsigned int polling_limit_us = 30;
module_param(polling_limit_us, uint, 0664);
MODULE_PARM_DESC(polling_limit_us,
		 "time in us to run a transfer in polling mode\n");

struct cfg_mode {
	u32 cfg;
	u32 mode;
	u32 spi_mode;
};

/**
 * struct cortina_spi - Cortina Access SPI controller
 * @regs: base address of register map
 * @clk: core clock, divided to calculate serial clock
 * @clk_hz: core clock cached speed
 * @irq: interrupt, signals TX FIFO empty or RX FIFO ¾ full
 * @ctlr: SPI controller reverse lookup
 * @spi: pointer handling spi device for debug
 * @xfer: pointer handling spi transfer for debug
 * @tx_buf: pointer whence next transmitted byte is read
 * @rx_buf: pointer where next received byte is written
 * @tx_len: remaining bytes to transmit
 * @rx_len: remaining bytes to receive
 * @prepare_cm: precalculated CFG/MODE registers value for ->prepare_message()
 * @debugfs_dir: the debugfs directory - neede to remove debugfs when
 *      unloading the module
 * @count_transfer_polling: count of how often polling mode is used
 * @count_transfer_irq: count of how often interrupt mode is used
 * @count_transfer_polling: count of how often atomic mode is used
 * @chip_select: SPI slave currently selected
 *	(used by cortina_spi_dma_tx_done() to write @clear_rx_cs)
 * @hw_cs_num:
 */
struct cortina_spi {
	void __iomem *regs;
	struct clk *clk;
	unsigned long clk_hz;
	int irq;
	struct spi_controller *ctlr;
	struct spi_device *spi;
	struct spi_transfer *xfer;
	const u8 *tx_buf;
	u8 *rx_buf;
	int tx_len;
	int rx_len;
	struct cfg_mode prepare_cm[CORTINA_SPI_NUM_CS];

	struct dentry *debugfs_dir;
	u64 count_transfer_polling;
	u64 count_transfer_irq;
	u64 count_transfer_atomic;

	u8 chip_select;

	u8 hw_cs_num;
};

static inline int __cortina_spi_setup(struct spi_device *spi);

#if defined(CONFIG_DEBUG_FS)
static void cortina_debugfs_create(struct cortina_spi *cs,
				   const char *dname)
{
	char name[64];
	struct dentry *dir;

	/* get full name */
	snprintf(name, sizeof(name), "spi-cortina-%s", dname);

	/* the base directory */
	dir = debugfs_create_dir(name, NULL);
	cs->debugfs_dir = dir;

	/* the counters */
	debugfs_create_u64("count_transfer_polling", 0444, dir,
			   &cs->count_transfer_polling);
	debugfs_create_u64("count_transfer_irq", 0444, dir,
			   &cs->count_transfer_irq);
	debugfs_create_u64("count_transfer_atomic", 0444, dir,
			   &cs->count_transfer_atomic);
}

static void cortina_debugfs_remove(struct cortina_spi *cs)
{
	debugfs_remove_recursive(cs->debugfs_dir);
	cs->debugfs_dir = NULL;
}
#else
static void cortina_debugfs_create(struct cortina_spi *cs,
				   const char *dname)
{
}

static void cortina_debugfs_remove(struct cortina_spi *cs)
{
}
#endif /* CONFIG_DEBUG_FS */

static inline u32 cortina_rd(struct cortina_spi *cs, unsigned int reg)
{
	u32 val = readl(cs->regs + reg);

	dev_dbg(&cs->ctlr->dev, "R, reg=0x%02x, val=0x%08x\n", reg, val);

	return val;
}

static inline void cortina_wr(struct cortina_spi *cs, unsigned int reg, u32 val)
{
	dev_dbg(&cs->ctlr->dev, "W, reg=0x%02x, val=0x%08x\n", reg, val);

	writel(val, cs->regs + reg);
}

static inline void cortina_flush_rx_fifo(struct cortina_spi *cs)
{
	cortina_wr(cs, CORTINA_SPI_RDAT0, 0);
	cortina_wr(cs, CORTINA_SPI_RDAT1, 0);
}

static inline void cortina_flush_tx_fifo(struct cortina_spi *cs)
{
	cortina_wr(cs, CORTINA_SPI_CA0, 0);
	cortina_wr(cs, CORTINA_SPI_CA1, 0);
}

static inline void cortina_rd_rx_fifo(struct cortina_spi *cs)
{
	int rx_len = cs->spi->bits_per_word / 8;
	int loop, i;
	u32 val;

	if (!cs->rx_len)
		return;

	if (!cs->rx_buf) {
		cs->rx_len -= rx_len;
		return;
	}

REMAINDER:
	val = cortina_rd(cs, rx_len > 4 ? CORTINA_SPI_RDAT1 : CORTINA_SPI_RDAT0);
	loop = ((rx_len % 4) ? : 4) - 1;
	for (i = loop; i >= 0; i--)
		*cs->rx_buf++ = (u8)((val >> (i * 8)) & 0xFF);

	cs->rx_len -= (loop + 1);

	if (rx_len > 4) {
		rx_len = 4;
		goto REMAINDER;
	}
}

static inline void cortina_wr_tx_fifo(struct cortina_spi *cs)
{
	int tx_len = cs->spi->bits_per_word / 8;
	u32 val;
	int i;

	if (!cs->tx_len)
		return;

	if (!cs->tx_buf) {
		cs->tx_len -= tx_len;
		return;
	}

	for (i = 0; i < tx_len; i++) {
		if ((i % 4) == 0)
			val = 0;

		val |= ((u32)(*cs->tx_buf++)) << (24 - 8 * (i % 4));

		if ((i & 3) == 3)
			cortina_wr(cs, CORTINA_SPI_CA0 + 4 * (i / 4), val);
	}
	if (i % 4)
		cortina_wr(cs, CORTINA_SPI_CA0 + 4 * (i / 4), val);

	cs->tx_len -= tx_len;
}

static void cortina_spi_reset_hw(struct cortina_spi *cs)
{
	cortina_wr(cs, CORTINA_SPI_IE0, 0);

	cortina_flush_rx_fifo(cs);
	cortina_flush_tx_fifo(cs);
}

static irqreturn_t cortina_spi_interrupt(int irq, void *dev_id)
{
	struct cortina_spi *cs = dev_id;
	struct spi_device *spi = cs->spi;
	struct spi_transfer *xfer = cs->xfer;
	int len = xfer->len;

	cortina_rd_rx_fifo(cs);

	if (cs->rx_len) {
		cortina_wr_tx_fifo(cs);
		cortina_wr(cs, CORTINA_SPI_CTRL, CORTINA_SPI_CTRL_START);
	} else {
		cortina_spi_reset_hw(cs);

		if (xfer->tx_buf && xfer->rx_buf) {
			dev_dbg(&spi->dev, "len=%d tx=[%*phD] rx=[%*phD]\n",
				len, len, xfer->tx_buf, len, xfer->rx_buf);
		} else if (xfer->tx_buf) {
			dev_dbg(&spi->dev, "len=%d tx=[%*phD]\n",
				len, len, xfer->tx_buf);
		} else if (xfer->rx_buf) {
			dev_dbg(&spi->dev, "len=%d rx=[%*phD]\n",
				len, len, xfer->rx_buf);
		}

		spi_finalize_current_transfer(cs->ctlr);
	}

	return IRQ_HANDLED;
}

static int cortina_spi_transfer_one_irq(struct spi_controller *ctlr,
					struct spi_device *spi,
					struct spi_transfer *xfer)
{
	struct cortina_spi *cs = spi_controller_get_devdata(ctlr);

	/* update usage statistics */
	cs->count_transfer_irq++;

	cortina_wr_tx_fifo(cs);

	cortina_wr(cs, CORTINA_SPI_IE0, CORTINA_SPI_INT_INT);

	cortina_wr(cs, CORTINA_SPI_CTRL, CORTINA_SPI_CTRL_START);

	/* signal that we need to wait for completion */
	return 1;
}

static int cortina_spi_transfer_one_poll(struct spi_controller *ctlr,
					 struct spi_device *spi,
					 struct spi_transfer *xfer)
{
	struct cortina_spi *cs = spi_controller_get_devdata(ctlr);
	void __iomem *addr = cs->regs + CORTINA_SPI_CTRL;
	unsigned long timeout_us = 2000;
	int len = xfer->len;
	u32 reg;
	int ret;

	/* update usage statistics */
	if (spi->mode & SPI_ATOMIC)
		cs->count_transfer_atomic++;
	else
		cs->count_transfer_polling++;

	while (cs->rx_len) {
		cortina_wr_tx_fifo(cs);

		cortina_wr(cs, CORTINA_SPI_CTRL, CORTINA_SPI_CTRL_START);

		if (spi->mode & SPI_ATOMIC)
			ret = readl_poll_timeout_atomic(addr, reg, (reg & 1), 1, timeout_us);
		else
			ret = readl_poll_timeout(addr, reg, (reg & 1), 3, timeout_us);
		if (ret) {
			dev_err(&spi->dev, "poll timeout %lu\n", timeout_us);
			return ret;
		}

		cortina_wr(cs, CORTINA_SPI_CTRL, CORTINA_SPI_CTRL_DONE);

		cortina_rd_rx_fifo(cs);
	}

	cortina_spi_reset_hw(cs);

	if (xfer->tx_buf && xfer->rx_buf) {
		dev_dbg(&spi->dev, "len=%d tx=[%*phD] rx=[%*phD]\n",
			len, len, xfer->tx_buf, len, xfer->rx_buf);
	} else if (xfer->tx_buf) {
		dev_dbg(&spi->dev, "len=%d tx=[%*phD]\n",
			len, len, xfer->tx_buf);
	} else if (xfer->rx_buf) {
		dev_dbg(&spi->dev, "len=%d rx=[%*phD]\n",
			len, len, xfer->rx_buf);
	}

	return 0;
}

static int cortina_spi_transfer_one(struct spi_controller *ctlr,
				    struct spi_device *spi,
				    struct spi_transfer *xfer)
{
	struct cortina_spi *cs = spi_controller_get_devdata(ctlr);
	unsigned long spi_hz, cdiv;
	unsigned long hz_per_byte, byte_limit;

	if (!xfer->bits_per_word) {
		if (unlikely(xfer->bits_per_word != spi->bits_per_word))
			dev_warn(&spi->dev, "driver not support dynamic bits_per_word %d-%d\n",
				 xfer->bits_per_word, spi->bits_per_word);
	}

	/* set clock */
	spi_hz = xfer->speed_hz ? xfer->speed_hz : spi->max_speed_hz;

	if (spi_hz >= cs->clk_hz / 2) {
		cdiv = 2; /* clk_hz/2 is the fastest we can go */
	} else if (spi_hz) {
		/* CDIV must be a multiple of two */
		cdiv = DIV_ROUND_UP(cs->clk_hz, spi_hz);
		cdiv += (cdiv % 2);

		if (cdiv >= 65536)
			cdiv = 0; /* 0 is the slowest we can go */
	} else {
		cdiv = 0; /* 0 is the slowest we can go */
	}
	xfer->effective_speed_hz = cdiv ? (cs->clk_hz / cdiv) : (cs->clk_hz / 65536);

	cortina_wr(cs, CORTINA_SPI_CLK, cdiv / 2 - 1);

	cs->spi = spi;
	cs->xfer = xfer;
	cs->tx_buf = xfer->tx_buf;
	cs->rx_buf = xfer->rx_buf;
	cs->tx_len = xfer->len;
	cs->rx_len = xfer->len;

	/* Calculate the estimated time in us the transfer runs.  Note that
	 * there is 1 idle clocks cycles after each byte getting transferred
	 * so we have 9 cycles/byte.  This is used to find the number of Hz
	 * per byte per polling limit.  E.g., we can transfer 1 byte in 30 us
	 * per 300,000 Hz of bus clock.
	 */
	hz_per_byte = polling_limit_us ? (9 * 1000000) / polling_limit_us : 0;
	byte_limit = hz_per_byte ? xfer->effective_speed_hz / hz_per_byte : 1;

	/* run in polling mode for short transfers */
	if (xfer->len < byte_limit)
		return cortina_spi_transfer_one_poll(ctlr, spi, xfer);

	/* run in interrupt-mode */
	return cortina_spi_transfer_one_irq(ctlr, spi, xfer);
}

static int cortina_spi_prepare_message(struct spi_controller *ctlr,
				       struct spi_message *msg)
{
	struct spi_device *spi = msg->spi;
	struct cortina_spi *cs = spi_controller_get_devdata(ctlr);

	if (spi->mode != cs->prepare_cm[spi->chip_select].spi_mode) {
		if (__cortina_spi_setup(spi))
			return -EINVAL;
	}

	cs->chip_select = spi->chip_select;

	cortina_wr(cs, CORTINA_SPI_CFG, cs->prepare_cm[cs->chip_select].cfg);
	cortina_wr(cs, CORTINA_SPI_MODE, cs->prepare_cm[cs->chip_select].mode);

	return 0;
}

static inline int __cortina_spi_setup(struct spi_device *spi)
{
	struct spi_controller *ctlr = spi->controller;
	struct cortina_spi *cs = spi_controller_get_devdata(ctlr);
	u8 bits_per_word;
	u32 val;

	if (spi->chip_select >= CORTINA_SPI_NUM_CS) {
		dev_err(&spi->dev, "only %d chip-selects supported\n",
			CORTINA_SPI_NUM_CS);
		return -EINVAL;
	}

	if (spi->bits_per_word % 8) {
		dev_err(&spi->dev, "not support %d bits_per_word\n",
			spi->bits_per_word);
		return -EINVAL;
	}

	if (spi->bits_per_word)
		bits_per_word = spi->bits_per_word - 1;
	else
		bits_per_word = 7;

	val = CORTINA_SPI_CFG_CC | CORTINA_SPI_CFG_WR;
	if (!(spi->mode & SPI_NO_CS || spi->cs_gpiod)) {
		if (spi->mode & SPI_CS_HIGH)
			val |= BIT(spi->chip_select) << CORTINA_SPI_CFG_MW_OFF;
		else
			val |= BIT(spi->chip_select) << CORTINA_SPI_CFG_SS_OFF;
	}
	cs->prepare_cm[spi->chip_select].cfg = val + (bits_per_word << CORTINA_SPI_CFG_SC_OFF);

	val = 0;
	if (spi->mode & SPI_CPOL)
		val |= CORTINA_SPI_MODE_CPOL;
	if (spi->mode & SPI_CPHA)
		val |= CORTINA_SPI_MODE_CPHA;
	if (spi->controller_data)
		val |= CORTINA_SPI_MODE_ISAM;
	cs->prepare_cm[spi->chip_select].mode = val | CORTINA_SPI_MODE_CMDS;

	cs->prepare_cm[spi->chip_select].spi_mode = spi->mode;

	return 0;
}

static int cortina_spi_setup(struct spi_device *spi)
{
	return __cortina_spi_setup(spi);
}

static int cortina_spi_probe(struct platform_device *pdev)
{
	struct spi_controller *ctlr;
	struct cortina_spi *cs;
	int err;

	ctlr = devm_spi_alloc_master(&pdev->dev, sizeof(*cs));
	if (!ctlr)
		return -ENOMEM;

	platform_set_drvdata(pdev, ctlr);

	ctlr->use_gpio_descriptors = true;
	ctlr->mode_bits = CORTINA_SPI_MODE_BITS;
//	ctlr->bits_per_word_mask = SPI_BPW_MASK(8);
	ctlr->num_chipselect = CORTINA_SPI_NUM_CS;
	ctlr->setup = cortina_spi_setup;
	ctlr->transfer_one = cortina_spi_transfer_one;
	ctlr->prepare_message = cortina_spi_prepare_message;
	ctlr->dev.of_node = pdev->dev.of_node;

	cs = spi_controller_get_devdata(ctlr);
	cs->ctlr = ctlr;

	cs->hw_cs_num = (u8)(long)of_device_get_match_data(&pdev->dev);
	dev_info(&pdev->dev, "SPI HW CS %u\n", cs->hw_cs_num);

	cs->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(cs->regs))
		return PTR_ERR(cs->regs);

	cs->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(cs->clk))
		return dev_err_probe(&pdev->dev, PTR_ERR(cs->clk),
				     "could not get clk\n");

	cs->irq = platform_get_irq(pdev, 0);
	if (cs->irq <= 0)
		return cs->irq ? cs->irq : -ENODEV;

	clk_prepare_enable(cs->clk);
	cs->clk_hz = clk_get_rate(cs->clk);

	err = devm_request_irq(&pdev->dev, cs->irq, cortina_spi_interrupt, 0,
			       dev_name(&pdev->dev), cs);
	if (err) {
		dev_err(&pdev->dev, "could not request IRQ: %d\n", err);
		goto out_clk_disable;
	}

	err = spi_register_controller(ctlr);
	if (err) {
		dev_err(&pdev->dev, "could not register SPI controller: %d\n",
			err);
		goto out_clk_disable;
	}

	cortina_debugfs_create(cs, dev_name(&pdev->dev));

	return 0;

out_clk_disable:
	clk_disable_unprepare(cs->clk);

	return err;
}

static int cortina_spi_remove(struct platform_device *pdev)
{
	struct spi_controller *ctlr = platform_get_drvdata(pdev);
	struct cortina_spi *cs = spi_controller_get_devdata(ctlr);

	cortina_debugfs_remove(cs);

	spi_unregister_controller(ctlr);

	cortina_spi_reset_hw(cs);

	clk_disable_unprepare(cs->clk);

	return 0;
}

static void cortina_spi_shutdown(struct platform_device *pdev)
{
	int ret;

	ret = cortina_spi_remove(pdev);
	if (ret)
		dev_err(&pdev->dev, "failed to shutdown\n");
}

static const struct of_device_id cortina_spi_match[] = {
	{ .compatible = "realtek,rtl_v2-spi", .data = (const void *)8 },
	{ .compatible = "cortina,ca77xx-spi", .data = (const void *)5 },
	{}
};
MODULE_DEVICE_TABLE(of, cortina_spi_match);

static struct platform_driver cortina_spi_driver = {
	.driver		= {
		.name		= DRV_NAME,
		.of_match_table	= cortina_spi_match,
	},
	.probe		= cortina_spi_probe,
	.remove		= cortina_spi_remove,
	.shutdown	= cortina_spi_shutdown,
};
module_platform_driver(cortina_spi_driver);

MODULE_DESCRIPTION("SPI controller driver for Cortina Access");
MODULE_LICENSE("GPL");
