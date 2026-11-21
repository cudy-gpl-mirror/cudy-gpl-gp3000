// SPDX-License-Identifier: GPL-2.0

#include <linux/version.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/dma-map-ops.h>
#include <linux/arm-smccc.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <soc/cortina/cortina-smc.h>
#include <soc/cortina/cortina-soc.h>
#include "soc_arm.h"

static struct ca_soc_data soc_data;
uint32_t RTK_SOC_CHIP_ID; /* to be removed */
EXPORT_SYMBOL(RTK_SOC_CHIP_ID);

struct proc_dir_entry *ca_proc_dir; /* used by ca-ne */
EXPORT_SYMBOL(ca_proc_dir);

struct clk *cpu_daemon_clk;

u64 ca_smc_call(unsigned int smc_id, unsigned long x1, unsigned long x2,
		unsigned long x3, unsigned long x4)
{
	struct arm_smccc_res res;

	//BUG_ON(x1 == 0); /* Illegal SMC call(Address 0) */

	arm_smccc_smc(smc_id, x1, x2, x3, x4, 0, 0, 0, &res);
	return res.a0;
}
EXPORT_SYMBOL(ca_smc_call);

static int rtk_smc_buff_call(int smc_id, char *buf, size_t len)
{
	int ret = 0;
	__iomem void *smc_buf;
	phys_addr_t phyaddr;

	smc_buf = kmalloc(len, GFP_KERNEL);
	if (!smc_buf)
		return -ENOMEM;

	phyaddr = virt_to_phys(smc_buf);
	arch_sync_dma_for_cpu(phyaddr, len, DMA_BIDIRECTIONAL);
	ret = ca_smc_call(smc_id, 0, 0, phyaddr, 0);
	memcpy(buf, smc_buf, len);
	kfree(smc_buf);
	return ret;
}

int rtk_smc_gphy_patch_read(char *buf, size_t len)
{
	if (len < 1024) {
		printk(KERN_ERR "rtk_smc_gphy_patch failed! Expected Buff_SIZE is over 1024, but input 0x%zx\n", len);
		return -EINVAL;
	}

	return rtk_smc_buff_call(CA_SVC_OTP_GPHY_PATCH_READ, buf, len);
}

//#define GHPY_SMC_PATCH_TEST
#ifdef GHPY_SMC_PATCH_TEST
#define test_buff_size 1024
static void test_smc_gphy(void)
{
	char *buff = kzalloc(test_buff_size, GFP_KERNEL);

	if (buff) {
		rtk_smc_gphy_patch_read(buff, test_buff_size);
		print_hex_dump(KERN_INFO, "PHY ", DUMP_PREFIX_OFFSET, 16, 1, buff, test_buff_size, 0);
		kfree(buff);
	}
}
#endif

int plat_soc_get_chipmode(u8 *buf, int buflen)
{
	int sz;

	sz = sizeof(((struct ca_soc_data *)0)->chipmode);
	sz = min(sz, buflen);

	WARN_ON(!soc_data.chipmode);
	memcpy(buf, &soc_data.chipmode, sz);
	return 0;
}
EXPORT_SYMBOL(plat_soc_get_chipmode); /* used by ca_rtk */

#define MHZ	(1000000)

#ifdef CONFIG_ARM_DSU_PMU
#define CA_SMC_CALL_SREG_ACTLR_READ(elx) \
	rtk_smc_call(CA_SVC_SREG_ACTLR_READ, elx, 0, 0, 0)
#define CA_SMC_CALL_SREG_ACTLR_WRITE(elx, val) \
	rtk_smc_call(CA_SVC_SREG_ACTLR_SET, BIT(elx), val, 0, 0)

#define CLUSTERPMUEN_BIT BIT(12)
#define EL2 0x2
#define EL3 0x3
#define ACTLR_CLUSTERPMUEN_PASS 1
#define ACTLR_CLUSTERPMUEN_FAIL 0

static u64 rtk_smc_call(unsigned int smc_id, unsigned long x1, unsigned long x2,
		unsigned long x3, unsigned long x4)
{
	struct arm_smccc_res res;

	if (x1 == 0) {
		printk(KERN_ERR "Illegal SMC call(Address 0)!\n");
		while
			(1);
	}

	arm_smccc_smc(smc_id, x1, x2, x3, 0, 0, 0, 0, &res);
	return res.a0;
}

static void rtk_actlr_set(void *info)
{
	unsigned int *result = (unsigned int *)info;
	unsigned long val;

	val = CA_SMC_CALL_SREG_ACTLR_READ(EL3) | CLUSTERPMUEN_BIT;
	CA_SMC_CALL_SREG_ACTLR_WRITE(EL3, val);
	val = CA_SMC_CALL_SREG_ACTLR_READ(EL2) | CLUSTERPMUEN_BIT;
	CA_SMC_CALL_SREG_ACTLR_WRITE(EL2, val);

	if ((CA_SMC_CALL_SREG_ACTLR_READ(EL3) & CLUSTERPMUEN_BIT) &&
		(CA_SMC_CALL_SREG_ACTLR_READ(EL2) & CLUSTERPMUEN_BIT)) {
		*result = 1;
		return;
	}
	*result = 0;
}

static void rtk_cpu_feature_enable_dsm_pmu(void)
{
	int cpu;
	unsigned int result;

	for_each_online_cpu(cpu) {
		smp_call_function_single(cpu, rtk_actlr_set, (void *)&result, 1);
		if (result == 0) {
			printk(KERN_INFO "DSU_PMU: ACTLR_EL2/3 - Can't set CLUSTERPMUEN bit.\n");
			return;
		}
	}
}
#else
static void rtk_cpu_feature_enable_dsm_pmu(void)
{
}
#endif

__weak __init int probe_soc_venus(struct ca_soc_data *soc)
{
	return 0;
}

__weak __init int probe_soc_taurus(struct ca_soc_data *soc)
{
	return 0;
}

__weak __init int probe_soc_elnath(struct ca_soc_data *soc)
{
	return 0;
}

__weak __init int probe_soc_mercury(struct ca_soc_data *soc)
{
	return 0;
}

static __init int probe_and_setup_soc_data(void)
{
	int probed;

	soc_data.sys_clk = 25 * MHZ; /* set default */

	probed = probe_soc_venus(&soc_data) ||
		probe_soc_taurus(&soc_data) ||
		probe_soc_elnath(&soc_data) ||
		probe_soc_mercury(&soc_data);

	return probed;
}

int ca_soc_data_get(struct ca_soc_data *data)
{
	if (data == NULL)
		return -EINVAL;

	memcpy(data, &soc_data, sizeof(soc_data));
	return 0;
}
EXPORT_SYMBOL(ca_soc_data_get);

#ifdef IS_FPGA
static u8 get_chip_revision(void) { return 0; }
#else
static u8 get_chip_revision(void)
{
	unsigned int revision = 0;
	/* use Secure Monitor Call to retrieve chip revision.
	*/
	revision = CA_SMC_CALL_ROM_VERSION();
	if ((revision < 'A') || (revision == 0xffffffff)) {
		printk(KERN_ERR "\033[0;31mWarning: Can't get CHIP Revision, so use default value(A).\033[0m\n");
		printk(KERN_ERR "\033[0;31mPlease update bootloader to the recent version!\033[0m\n");
		revision = 'A';
	}
	return (u8)revision;
}
#endif

static __init int soc_arch_init(void)
{
	if (!probe_and_setup_soc_data()) /* soc_data is valid after this */
		panic("No SOC found\n");

	soc_data.chip_revision = get_chip_revision();
	rtk_cpu_feature_enable_dsm_pmu();

	RTK_SOC_CHIP_ID = soc_data.chip_id;

	#ifdef GHPY_SMC_PATCH_TEST
	test_smc_gphy();
	#endif

	return 0;
}
/* early calls */
arch_initcall(soc_arch_init);


static void __ca_jtag_id_dump(struct device *dev, struct ca_soc_data *ca_soc)
{
	dev_info(dev, "Vendor ID: 0x%03x\n", ca_soc->vendor_id);
	dev_info(dev, "Chip ID: 0x%04x\n", ca_soc->chip_id);
	dev_info(dev, "Chip Rev: %u\n", ca_soc->chip_revision);
	dev_info(dev, "Chip Mode: 0x%02x\n", ca_soc->chipmode);
}

static void __ca_clock_dump(struct device *dev, struct ca_soc_data *ca_soc)
{
	if (ca_soc->cpll != 0)
		dev_info(dev, "CPLL: %u.%02u MHZ\n", ca_soc->cpll / MHZ, (ca_soc->cpll / 10000) % 100);
	if (ca_soc->epll != 0)
		dev_info(dev, "EPLL: %u.%02u MHZ\n", ca_soc->epll / MHZ, (ca_soc->epll / 10000) % 100);
	if (ca_soc->fpll != 0)
		dev_info(dev, "FPLL: %u.%02u MHZ\n", ca_soc->fpll / MHZ, (ca_soc->fpll / 10000) % 100);
	if (ca_soc->cpu_clk != 0)
		dev_info(dev, "CPU Clock: %u.%02u MHZ\n", ca_soc->cpu_clk / MHZ, (ca_soc->cpu_clk / 10000) % 100);
	if (ca_soc->core_clk != 0)
		dev_info(dev, "Core Clock: %u.%02u MHZ\n", ca_soc->core_clk / MHZ, (ca_soc->core_clk / 10000) % 100);
	if (ca_soc->eaxi_clk != 0)
		dev_info(dev, "EAXI Clock: %u.%02u MHZ\n", ca_soc->eaxi_clk / MHZ, (ca_soc->eaxi_clk / 10000) % 100);
	if (ca_soc->lsaxi_clk != 0)
		dev_info(dev, "LSAXI Clock: %u.%02u MHZ\n", ca_soc->lsaxi_clk / MHZ, (ca_soc->lsaxi_clk / 10000) % 100);
	if (ca_soc->hsaxi_clk != 0)
		dev_info(dev, "HSAXI Clock: %u.%02u MHZ\n", ca_soc->hsaxi_clk / MHZ, (ca_soc->hsaxi_clk / 10000) % 100);
	if (ca_soc->cci_clk != 0)
		dev_info(dev, "CCI Clock: %u.%02u MHZ\n", ca_soc->cci_clk / MHZ, (ca_soc->cci_clk / 10000) % 100);
	if (ca_soc->crypto_clk != 0)
		dev_info(dev, "Crypto Clock: %u.%02u MHZ\n", ca_soc->crypto_clk / MHZ, (ca_soc->crypto_clk / 10000) % 100);
	if (ca_soc->atb_clk != 0)
		dev_info(dev, "ATB Clock: %u.%02u MHZ\n", ca_soc->atb_clk / MHZ, (ca_soc->atb_clk / 10000) % 100);
	if (ca_soc->pe_clk != 0)
		dev_info(dev, "PE Clock: %u.%02u MHZ\n", ca_soc->pe_clk / MHZ, (ca_soc->pe_clk / 10000) % 100);
	if (ca_soc->peaxi_clk != 0)
		dev_info(dev, "PE AXI Clock: %u.%02u MHZ\n", ca_soc->peaxi_clk / MHZ, (ca_soc->peaxi_clk / 10000) % 100);
}

unsigned int rtk_soc_sysclk_get_khz(void)
{
	BUG_ON(!soc_data.sys_clk);

	return (soc_data.sys_clk / 1000);
}
EXPORT_SYMBOL(rtk_soc_sysclk_get_khz);

#define REASON_MASK 0xf
static void *glbl_software;
void rtk_soc_reboot_reason_set(int reason)
{
	u32 val;

	if (glbl_software) {
		val = readl(glbl_software);
		val = (val & ~REASON_MASK) | (reason & REASON_MASK);
		writel(val, glbl_software);
	}
}
EXPORT_SYMBOL(rtk_soc_reboot_reason_set);

int rtk_soc_reboot_reason_get(void)
{
	if (unlikely(!glbl_software) && soc_data.reg_global_software)
		glbl_software = ioremap(soc_data.reg_global_software, 4);

	if (glbl_software)
		return (readl(glbl_software) & REASON_MASK);
	else
		return -1;
}



static int ca_soc_probe(struct platform_device *pdev)
{
	if (cpu_daemon_clk)
		soc_data.cpu_clk = clk_get_rate(cpu_daemon_clk);

	ca_proc_dir = proc_mkdir("driver/cortina", NULL);
	__ca_jtag_id_dump(&pdev->dev, &soc_data);
	__ca_clock_dump(&pdev->dev, &soc_data);

	return 0;
}

static const struct of_device_id ca_soc_dt_match[] = {
	{ .compatible = "cortina,venus", },
	{ .compatible = "realtek,taurus", },
	{ .compatible = "realtek,9607f", },
	{ .compatible = "realtek,mercury", },
	{ },
};

MODULE_DEVICE_TABLE(of, ca_soc_dt_match);

static struct platform_driver ca_soc_driver = {
	.driver = {
		.name		= "ca_soc",
		.owner		= THIS_MODULE,
		.of_match_table	= ca_soc_dt_match,
	},
	.probe = ca_soc_probe,
};

module_platform_driver(ca_soc_driver);

MODULE_AUTHOR("Realtek");
MODULE_DESCRIPTION("Realtek ARM based SOC driver");
MODULE_LICENSE("GPL v2");
