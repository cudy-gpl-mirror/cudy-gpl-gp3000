// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Elnath Packet Engine Reset Controller Driver
 *
 * Based on reset-simple.c
 *
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/spinlock.h>
#include <soc/cortina/registers.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

#define NUM_CPUS				3

#define to_elnath_pe_reset_attr(x) container_of(x, struct elnath_pe_reset_attribute, attr)
#define kobj_to_elnath_pe_reset_cpu(x) container_of(x, struct elnath_pe_reset_cpu, kobj)

struct elnath_pe_reset_priv;

struct elnath_pe_reset_cpu {
	struct kobject kobj;
	unsigned int cpu_id;
	u32 duration_ms;
	struct elnath_pe_reset_priv *priv;
};

struct elnath_pe_reset_priv {
	spinlock_t			lock;
	struct kref refcount;
	struct reset_controller_dev	rcdev;
	struct clk_bulk_data *clks;
	struct elnath_pe_reset_cpu cpus[];
};

struct elnath_pe_reset_attribute {
	struct attribute attr;
	ssize_t (*show)(struct elnath_pe_reset_cpu *cpu,
			struct elnath_pe_reset_attribute *attr, char *buf);
	ssize_t (*store)(struct elnath_pe_reset_cpu *cpu,
			 struct elnath_pe_reset_attribute *attr,
			 const char *buf, size_t count);
};

static int elnath_pe_reset_assert(struct reset_controller_dev *rcdev,
			       unsigned long cpu_id)
{
	struct elnath_pe_reset_priv *priv = container_of(rcdev,
							struct elnath_pe_reset_priv,
							rcdev);
	unsigned long flags;
	void __iomem *block_reset_addr;
	GLOBAL_BLOCK_RESET_t blk_rst_reg;

	if(cpu_id > (NUM_CPUS - 1))
	{
		dev_err(rcdev->dev, "Invalid cpu id %ld.\n", cpu_id);
		return -EINVAL;
	}

	block_reset_addr = ioremap(GLOBAL_BLOCK_RESET, 4);

	spin_lock_irqsave(&priv->lock, flags);

	blk_rst_reg.wrd = readl(block_reset_addr);
	switch (cpu_id) {
		case 0:
			blk_rst_reg.bf.reset_pe0 = 1;
			blk_rst_reg.bf.reset_rcpu0 = 1;
			break;
		case 1:
			blk_rst_reg.bf.reset_pe1 = 1;
			blk_rst_reg.bf.reset_rcpu1 = 1;
			break;
		case 2:
			blk_rst_reg.bf.reset_doe = 1;
			blk_rst_reg.bf.reset_rcpu2 = 1;
			break;
	}
	writel(blk_rst_reg.wrd, block_reset_addr);

	if(__clk_get_enable_count(priv->clks[cpu_id].clk))
		clk_bulk_disable_unprepare(1, &priv->clks[cpu_id]);

	spin_unlock_irqrestore(&priv->lock, flags);

	iounmap(block_reset_addr);

	return 0;
}

static int elnath_pe_reset_deassert(struct reset_controller_dev *rcdev,
					unsigned long cpu_id)
{
	struct elnath_pe_reset_priv *priv = container_of(rcdev,
						     struct elnath_pe_reset_priv,
						     rcdev);
	unsigned long flags;
	void __iomem *block_reset_addr;
	GLOBAL_BLOCK_RESET_t blk_rst_reg;

	if(cpu_id > (NUM_CPUS - 1))
	{
		dev_err(rcdev->dev, "Invalid cpu id %ld.\n", cpu_id);
 		return -EINVAL;
	}

 	block_reset_addr = ioremap(GLOBAL_BLOCK_RESET, 4);

	spin_lock_irqsave(&priv->lock, flags);

	if(!__clk_get_enable_count(priv->clks[cpu_id].clk))
		clk_bulk_prepare_enable(1, &priv->clks[cpu_id]);

	blk_rst_reg.wrd = readl(block_reset_addr);
	switch (cpu_id) {
		case 0:
			blk_rst_reg.bf.reset_pe0 = 0;
			break;
		case 1:
			blk_rst_reg.bf.reset_pe1 = 0;
			break;
		case 2:
			blk_rst_reg.bf.reset_doe = 0;
			break;
	}
	writel(blk_rst_reg.wrd, block_reset_addr);
	mdelay(1);

	switch (cpu_id) {
		case 0:
			blk_rst_reg.bf.reset_rcpu0 = 0;
			break;
		case 1:
			blk_rst_reg.bf.reset_rcpu1 = 0;
			break;
		case 2:
			blk_rst_reg.bf.reset_rcpu2 = 0;
			break;
	}
	writel(blk_rst_reg.wrd, block_reset_addr);

	spin_unlock_irqrestore(&priv->lock, flags);

	iounmap(block_reset_addr);

	return 0;
}

static int reset_elnath_pe_reset(struct reset_controller_dev *rcdev,
			      unsigned long cpu_id)
{
	struct elnath_pe_reset_priv *priv = container_of(rcdev,
						     struct elnath_pe_reset_priv,
						     rcdev);
	int ret;

	ret = elnath_pe_reset_assert(rcdev, cpu_id);

	if(ret < 0)
		return ret;

	if (priv->cpus[cpu_id].duration_ms > 0)
		msleep(priv->cpus[cpu_id].duration_ms);

	ret = elnath_pe_reset_deassert(rcdev, cpu_id);

	return ret;
}

static void elnath_pe_reset_free_priv(struct kref *ref)
{
	struct elnath_pe_reset_priv *priv = container_of(ref,
					struct elnath_pe_reset_priv, refcount);

	kfree(priv);
}

static void elnath_pe_reset_release_kobj(struct kobject *kobj)
{
	struct elnath_pe_reset_cpu *cpu;

	cpu = kobj_to_elnath_pe_reset_cpu(kobj);

	kref_put(&cpu->priv->refcount, elnath_pe_reset_free_priv);
}

static ssize_t elnath_pe_reset_attr_show(struct kobject *kobj,
				    struct attribute *attr,
				    char *buf)
{
	struct elnath_pe_reset_attribute *attribute;
	struct elnath_pe_reset_cpu *cpu;

	attribute = to_elnath_pe_reset_attr(attr);
	cpu = kobj_to_elnath_pe_reset_cpu(kobj);

	if (!attribute->show)
		return -EIO;

	return attribute->show(cpu, attribute, buf);
}

static ssize_t elnath_pe_reset_attr_store(struct kobject *kobj,
				     struct attribute *attr,
				     const char *buf, size_t len)
{
	struct elnath_pe_reset_attribute *attribute;
	struct elnath_pe_reset_cpu *cpu;

	attribute = to_elnath_pe_reset_attr(attr);
	cpu = kobj_to_elnath_pe_reset_cpu(kobj);

	if (!attribute->store)
		return -EIO;

	return attribute->store(cpu, attribute, buf, len);
}

static ssize_t control_store(struct elnath_pe_reset_cpu *cpu,
			     struct elnath_pe_reset_attribute *attr,
			     const char *buf, size_t count)
{
	char action[10];
	char *eol;

	strncpy(action, buf, min(count, sizeof(action)));
	action[sizeof(action) - 1] = '\0';
	eol = strrchr(action, '\n');
	if (eol)
		*eol = '\0';

	if (!strcmp("reset", action))
		reset_elnath_pe_reset(&cpu->priv->rcdev, cpu->cpu_id);
	else if (!strcmp("assert", action))
		elnath_pe_reset_assert(&cpu->priv->rcdev, cpu->cpu_id);
	else if (!strcmp("deassert", action))
		elnath_pe_reset_deassert(&cpu->priv->rcdev, cpu->cpu_id);
	else
		return -EINVAL;

	return count;
}

static ssize_t duration_ms_show(struct elnath_pe_reset_cpu *cpu,
				struct elnath_pe_reset_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%d\n", cpu->duration_ms);
}

static ssize_t duration_ms_store(struct elnath_pe_reset_cpu *cpu,
				 struct elnath_pe_reset_attribute *attr,
				 const char *buf, size_t count)
{
	if (kstrtou32(buf, 0, &cpu->duration_ms) < 0)
		return -EINVAL;

	return count;
}

static struct elnath_pe_reset_attribute control_attribute = __ATTR_WO(control);
static struct elnath_pe_reset_attribute duration_attribute = __ATTR_RW(duration_ms);

static struct attribute *elnath_pe_reset_attrs[] = {
	&control_attribute.attr,
	&duration_attribute.attr,
	NULL
};

static const struct sysfs_ops elnath_pe_reset_sysfs_ops = {
	.show = elnath_pe_reset_attr_show,
	.store = elnath_pe_reset_attr_store,
};

static struct kobj_type elnath_pe_reset_ktype = {
	.release = elnath_pe_reset_release_kobj,
	.sysfs_ops = &elnath_pe_reset_sysfs_ops,
	.default_attrs = elnath_pe_reset_attrs,
};

static int elnath_pe_reset_create_sysfs(struct elnath_pe_reset_cpu *cpu)
{
	int ret;

	ret = kobject_init_and_add(&cpu->kobj, &elnath_pe_reset_ktype,
				   &cpu->priv->rcdev.dev->kobj, "cpu%d",
				   cpu->cpu_id);

	kref_get(&cpu->priv->refcount); /* kobject part of private structure */

	if (ret) {
		kobject_put(&cpu->kobj);
		return ret;
	}

	kobject_uevent(&cpu->kobj, KOBJ_ADD);

	return 0;
}

static void elnath_pe_reset_destroy_sysfs(struct elnath_pe_reset_cpu *cpu)
{
	kobject_put(&cpu->kobj);
}

const struct reset_control_ops elnath_pe_reset_ops = {
	.assert		= elnath_pe_reset_assert,
	.deassert	= elnath_pe_reset_deassert,
    .reset		= reset_elnath_pe_reset,
};

static const struct of_device_id elnath_pe_reset_dt_ids[] = {
	{ .compatible = "rtk,elnath-pe-reset" },
	{ /* sentinel */ },
};

static void elnath_pe_reset_free_cpu(struct elnath_pe_reset_cpu *cpu)
{
	elnath_pe_reset_destroy_sysfs(cpu);
}

static int elnath_pe_reset_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct elnath_pe_reset_priv *priv;
	struct elnath_pe_reset_cpu *cpu;
	int ret;
	int i;

	priv = devm_kzalloc(dev, sizeof(*priv) + sizeof(*cpu) * NUM_CPUS, GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	ret = clk_bulk_get_all(dev, &priv->clks);
	if (ret != NUM_CPUS){
		dev_err(dev, "num_offload_clks with num_cpus not match\n");
		return -EINVAL;
	}

	spin_lock_init(&priv->lock);
	priv->rcdev.owner = THIS_MODULE;
	priv->rcdev.ops = &elnath_pe_reset_ops;
	priv->rcdev.of_node = dev->of_node;
	priv->rcdev.dev = dev;
	priv->rcdev.nr_resets = NUM_CPUS;

	cpu = priv->cpus;

	kref_init(&priv->refcount);

	for (i = 0; i < NUM_CPUS; i++) {
		of_property_read_u32(np, "duration-ms", &cpu->duration_ms);
		cpu->cpu_id = i;
		cpu->priv = priv;
		ret = elnath_pe_reset_create_sysfs(cpu);
		if (ret)
			goto rollback;
		cpu++;
	}

	for(i = 0; i < NUM_CPUS; i++)
	{
		elnath_pe_reset_assert(&priv->rcdev, i);
	}

	ret = reset_controller_register(&priv->rcdev);
	platform_set_drvdata(pdev, priv);

	if (ret)
	{
		dev_err(dev, "Failed to register elnath pe reset controller!\n");
		return ret;
	}

	return 0;

rollback:
	while (cpu >= priv->cpus)
		elnath_pe_reset_free_cpu(cpu--);

	kref_put(&priv->refcount, elnath_pe_reset_free_priv);

	return ret;
}

static int elnath_pe_reset_remove(struct platform_device *pdev)
{
	struct elnath_pe_reset_priv *priv = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < NUM_CPUS; i++)
		elnath_pe_reset_free_cpu(&priv->cpus[i]);

	reset_controller_unregister(&priv->rcdev);
	kref_put(&priv->refcount, elnath_pe_reset_free_priv);

	return 0;
}

static struct platform_driver elnath_pe_reset_driver = {
	.probe	= elnath_pe_reset_probe,
	.remove	= elnath_pe_reset_remove,
	.driver = {
		.name		= "elnath-pe-reset",
		.of_match_table	= elnath_pe_reset_dt_ids,
	},
};
module_platform_driver(elnath_pe_reset_driver);

MODULE_DESCRIPTION("Elnath Reset Packet Engine Controller Driver");
MODULE_LICENSE("GPL");