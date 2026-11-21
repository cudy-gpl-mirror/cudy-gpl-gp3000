#include <linux/delay.h>
#include <linux/hw_random.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/iopoll.h>

#include <soc/cortina/rtl8277c_registers.h>

/* Reg offset */
#define RNG_CTRL      0x00
#define RNG_RETURN0   0x1c
#define RNG_RESULTR   0x28
#define RNG_ST_RETURN 0x40
#define RNG_INT       0x54
#define RNG_INT_EN    0x64

/* INT bit offset */
#define INT_PARITY  BIT(2)
#define INT_ST_ERR  BIT(3)

#define OUT_READY   BIT(0)
#define MAX_POLL_CNT 0xffff

#define RNG_SRST      BIT(0)
#define RNG_CALI_SRST BIT(3)

typedef volatile union {
	struct {
		u32 rng_srst         : 1 ; /*bit 0*/
		u32 clk_hf_sel       : 1 ; /*bit 1*/
		u32 corrector_bypass : 1 ; /*bit 2*/
		u32 rng_cali_srst    : 1 ; /*bit 3*/
		u32 rvd0             : 1 ; /*bit 4*/
		u32 corrector_imode  : 1 ; /*bit 5*/
		u32 dbg0_sel         : 5 ; /*bit 10:6*/
		u32 dbg1_sel         : 5 ; /*bit 15:11*/
		u32 lfsr_bypass_1    : 1 ; /*bit 16*/
		u32 lfsr_mode        : 1 ; /*bit 17*/
		u32 st_en            : 1 ; /*bit 18*/
		u32 mode             : 1 ; /*bit 19*/
		u32 rvd1             : 10; /*bit 29:20*/
		u32 opt1             : 1 ; /*bit 30*/
		u32 lfsr_mode2       : 1 ; /*bit 31*/
	} bf;
	u32 wrd;
} RNG_CTRL_t;

struct rtk_trng_ctrl {
	struct device *dev;
	void __iomem *reg;
	int irq;
	RNG_CTRL_t ctrl_reg_v;

	/* properties */
	unsigned int is_self_test_en:1;
	unsigned int is_lfsr_bypass:1;
	unsigned int is_corrector_bypass:1;

	atomic_t in_reset;
};

static int rtk_trng_wait_ready(struct hwrng *rng, bool wait)
{
	struct rtk_trng_ctrl *trng = (struct rtk_trng_ctrl *)rng->priv;
	int ready;

	ready = OUT_READY & readl(trng->reg + RNG_RETURN0);
	if (!ready && wait) {
		readl_poll_timeout_atomic(trng->reg + RNG_RETURN0, ready,
			ready & OUT_READY, 3, 1500);
		ready &= OUT_READY;
	}

	return ready;
}

static int rtk_trng_read(struct hwrng *rng, void *buf, size_t max,
                         bool wait)
{
	struct rtk_trng_ctrl *trng = (struct rtk_trng_ctrl *)rng->priv;
	int retval = 0;

	//check wether we're in reset
	//TRNG needs to be reseted when RNG_int_parity is asserted
	while (atomic_read(&trng->in_reset) != 0) {
		if (!wait)
			return 0;
		cpu_relax();
	}

	while (max >= sizeof(u32)) {
		if (!rtk_trng_wait_ready(rng, wait))
			break;

		*(u32 *)buf = readl(trng->reg + RNG_RESULTR);

		retval += sizeof(u32);
		buf += sizeof(u32);
		max -= sizeof(u32);
	}

	return retval || !wait ? retval : -EIO;
}

static struct hwrng rtk_trng_hwrng = {
	.name = "rtk-trng",
	.read = rtk_trng_read,
	.quality = 1000,
};

static void rtk_trng_reset_assert(struct rtk_trng_ctrl *t, int mode)
{
	struct device *dev = t->dev;

	if (!mode)
		return;

	dev_dbg(dev, "assert Thor RNG reset: %s %s\n",
	        (mode & RNG_SRST)?"Software":"",
	        (mode & RNG_CALI_SRST)?"Calibration":"");

	atomic_inc(&t->in_reset);
	t->ctrl_reg_v.wrd = readl(t->reg + RNG_CTRL);

	if (mode & RNG_SRST)
		t->ctrl_reg_v.bf.rng_srst = 0x1;

	if (mode & RNG_CALI_SRST)
		t->ctrl_reg_v.bf.rng_cali_srst = 0x1;

	writel(t->ctrl_reg_v.wrd, t->reg + RNG_CTRL);
}

static void rtk_trng_reset_deassert(struct rtk_trng_ctrl *t, int mode)
{
	struct device *dev = t->dev;

	if (!mode)
		return;

	dev_dbg(dev, "deassert Thor RNG reset: %s %s\n",
	        (mode & RNG_SRST)?"Software":"",
	        (mode & RNG_CALI_SRST)?"Calibration":"");

	t->ctrl_reg_v.wrd = readl(t->reg + RNG_CTRL);

	if (mode & RNG_SRST)
		t->ctrl_reg_v.bf.rng_srst = 0x0;

	if (mode & RNG_CALI_SRST)
		t->ctrl_reg_v.bf.rng_cali_srst = 0x0;

	writel(t->ctrl_reg_v.wrd, t->reg + RNG_CTRL);
	atomic_dec(&t->in_reset);
}

static irqreturn_t rtk_trng_irq_handler(int irq, void *arg)
{
	struct rtk_trng_ctrl *t = arg;
	u32 val;

	val = readl(t->reg+RNG_INT);
	if (val & INT_PARITY) {
		//It needs to use soft-reset (RNG_CTRL: 0x200), if RNG_int_parity is asserted.
		rtk_trng_reset_assert(t, RNG_SRST);
		rtk_trng_reset_deassert(t, RNG_SRST);
	} else if (val & INT_ST_ERR) {
		//Maybe we should info user that a self test error has occured.
		dev_info(t->dev, "Self test error has been found (0x%x)\n", readl(t->reg+RNG_ST_RETURN) & 0xf);
		//clear asserted errors
		writel(0x0, t->reg+RNG_ST_RETURN);
	}

	writel(val, t->reg+RNG_INT);

	return IRQ_HANDLED;
}

static int rtk_trng_init(struct rtk_trng_ctrl *t)
{
	struct device *dev = t->dev;
	GLOBAL_GLOBAL_CONFIG_t reg_v;
	void __iomem *reg;


	//disable TRNG power down
	reg = ioremap(GLOBAL_GLOBAL_CONFIG, 4);
	if (IS_ERR(reg)) {
		dev_err(dev, "GLOBAL CONFIG Reg for Thor RNG power down control mapped failed\n");
		return PTR_ERR(reg);
	}

	reg_v.wrd = readl(reg);
	reg_v.bf.trng_pd = 0;
	writel(reg_v.wrd, reg);
	iounmap(reg);

	//assert reset
	rtk_trng_reset_assert(t, RNG_SRST | RNG_CALI_SRST);
	usleep_range(1000, 2000);
	//deassert reset
	rtk_trng_reset_deassert(t, RNG_SRST | RNG_CALI_SRST);

	return 0;
}

static int rtk_trng_setup(struct rtk_trng_ctrl *t)
{
	struct device *dev = t->dev;

	//RNG_ctrl (offset=0x200) : 0x0004_8000 (disable self-test : 0x0000_8000)
	t->ctrl_reg_v.wrd = readl(t->reg + RNG_CTRL);

	dev_info(dev, "Thor RNG setup with self test %s\n", t->is_self_test_en?"enabled":"disabled");
	if (t->is_self_test_en) {
		t->ctrl_reg_v.bf.st_en = 0x1;
		t->ctrl_reg_v.bf.opt1 = 0x1;
	} else {
		t->ctrl_reg_v.bf.st_en = 0x0;
		t->ctrl_reg_v.bf.opt1 = 0x0;
	}

	if (t->is_lfsr_bypass) {
		t->ctrl_reg_v.bf.lfsr_bypass_1 = 0x1;
		dev_info(dev, "Thor RNG setup with LFSR bypass\n");
	} else {
		t->ctrl_reg_v.bf.lfsr_bypass_1 = 0x0;
	}

	if (t->is_corrector_bypass) {
		t->ctrl_reg_v.bf.corrector_bypass = 0x1;
		dev_info(dev, "Thor RNG setup with corrector bypass\n");
	} else {
		t->ctrl_reg_v.bf.corrector_bypass = 0x0;
	}

	writel(t->ctrl_reg_v.wrd, t->reg + RNG_CTRL);

	//enable interrupt
	writel(0x0000000f, t->reg+RNG_INT_EN);

	return 0;
}

static ssize_t rtk_trng_mode_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
	struct rtk_trng_ctrl *trng = dev_get_drvdata(dev);
	ssize_t ret = 0;

	ret += sprintf(buf + ret, "Thor RNG running with:\n");
	ret += sprintf(buf + ret, "Self-Test: %s\n", trng->is_self_test_en?"enabled":"disabled");
	ret += sprintf(buf + ret, "LFSR Bypass: %s\n", trng->is_lfsr_bypass?"enabled":"disabled");
	ret += sprintf(buf + ret, "Corrector Bypass: %s\n", trng->is_corrector_bypass?"enabled":"disabled");

	return ret;
}


static ssize_t rtk_trng_mode_store(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t count)
{
	struct rtk_trng_ctrl *trng = dev_get_drvdata(dev);
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;

	dev_info(trng->dev, "mode setting via sysfs is not yet implemented\n");

	return count;
}

static DEVICE_ATTR(mode, 0664, rtk_trng_mode_show,
                   rtk_trng_mode_store);


static int rtk_trng_data_get(struct rtk_trng_ctrl *trng, u32 *data)
{
	int i = 0;


	//check RNG ready
	while (i < MAX_POLL_CNT) {
		if (OUT_READY & readl(trng->reg + RNG_RETURN0))
			goto DATA_READ;
		else
			i++;
	}

	dev_err(trng->dev, "Thor RNG polling reaches limit (%d)", MAX_POLL_CNT);
	return -EBUSY;

DATA_READ:
	//get data from
	*data = readl(trng->reg + RNG_RESULTR);

	if (!*data) {
		dev_err(trng->dev, "Thor RNG result is not available!");
		return -ENODATA;
	}

	return 0;
}

static ssize_t rtk_trng_data_show(struct device *dev,
                                  struct device_attribute *attr,
                                  char *buf)
{
	struct rtk_trng_ctrl *trng = dev_get_drvdata(dev);
	ssize_t cnt = 0;
	int ret = 0;
	u32 val = 0;

	ret = rtk_trng_data_get(trng, &val);
	if (ret < 0)
		cnt += sprintf(buf + cnt, "trng data is not ready\n");
	else
		cnt += sprintf(buf + cnt, "trng data: %x\n", val);

	return cnt;
}
static DEVICE_ATTR(data, 0444, rtk_trng_data_show, NULL);

static struct attribute *rtk_trng_attributes[] = {
	&dev_attr_mode.attr,
	&dev_attr_data.attr,
	NULL
};

static const struct attribute_group rtk_trng_attr_group = {
	.attrs = rtk_trng_attributes,
};

static int rtk_trng_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct resource *reg_res;
	struct rtk_trng_ctrl *trng;
	int ret;

	trng = devm_kzalloc(dev, sizeof(struct rtk_trng_ctrl),
	                    GFP_KERNEL);
	if (!trng)
		return -ENOMEM;

	trng->dev = dev;
	atomic_set(&trng->in_reset, 0);

	/* get TRNG reg base addr */
	reg_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!reg_res)
		return -EINVAL;

	trng->reg = devm_ioremap_resource(dev, reg_res);
	if (IS_ERR(trng->reg))
		return PTR_ERR(trng->reg);

	dev_info(dev, "resource - %pr mapped at 0x%pK\n", reg_res, trng->reg);

	trng->irq = platform_get_irq(pdev, 0);
	if (trng->irq < 0)
		return trng->irq;

	dev_info(dev, "Thor RNG irq is %d\n", trng->irq);
	ret = devm_request_irq(dev, trng->irq, rtk_trng_irq_handler,
	                       IRQF_NO_THREAD, "rtk-trng", trng);
	if (ret) {
		dev_err(dev, "Failed to request irq(%d)\n", ret);
		return ret;
	}

	trng->is_self_test_en = of_property_read_bool(np, "self-test");
	trng->is_lfsr_bypass = of_property_read_bool(np, "lfsr-bypass");
	trng->is_corrector_bypass = of_property_read_bool(np, "corrector_bypass");

	/* alloc lock */
#if 0 //so far we don't need lock
	lock = devm_kzalloc(dev, sizeof(*lock), GFP_KERNEL);
	if (!lock)
		return -ENOMEM;

	spin_lock_init(lock);
	trng->lock = lock;
#endif
	ret = sysfs_create_group(&trng->dev->kobj,
	                         &rtk_trng_attr_group);
	if (ret) {
		dev_err(dev, "failed to register sysfs\n");
		return ret;
	}

	dev_set_drvdata(dev, trng);

	ret = rtk_trng_init(trng);
	if (ret) {
		dev_err(dev, "Failed to init Thor RNG\n");
		return ret;
	}

	rtk_trng_setup(trng);

	rtk_trng_hwrng.priv = (unsigned long)trng;
	ret = devm_hwrng_register(dev, &rtk_trng_hwrng);
	if (ret) {
		dev_err(dev, "hwrng registration failed\n");
		return ret;
	} else
		dev_info(dev, "hwrng registered\n");

	return 0;
}

static int rtk_trng_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	GLOBAL_GLOBAL_CONFIG_t reg_v;
	void __iomem *reg;

	sysfs_remove_group(&pdev->dev.kobj, &rtk_trng_attr_group);
	devm_hwrng_unregister(dev, &rtk_trng_hwrng);

	//power down TRNG
	reg = ioremap(GLOBAL_GLOBAL_CONFIG, 4);
	if (IS_ERR(reg)) {
		dev_err(dev, "GLOBAL CONFIG Reg for Thor RNG power down control mapped failed\n");
		return PTR_ERR(reg);
	}

	reg_v.wrd = readl(reg);
	reg_v.bf.trng_pd = 1;
	writel(reg_v.wrd, reg);
	iounmap(reg);

	dev_info(dev, "Power down Thor RNG\n");

	return 0;
}

static const struct of_device_id rtk_trng_of_match[] = {
	{ .compatible = "realtek,trng", },
	{ },
};
MODULE_DEVICE_TABLE(of, rtk_trng_of_match);

static struct platform_driver rtk_trng_driver = {
	.probe = rtk_trng_probe,
	.remove = rtk_trng_remove,
	.driver = {
		.name = "rtk-trng",
		.of_match_table = rtk_trng_of_match,
	},
};

module_platform_driver(rtk_trng_driver);

MODULE_DESCRIPTION("Realtek Thor RNG Driver for Taurus");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:rtk-trng");
