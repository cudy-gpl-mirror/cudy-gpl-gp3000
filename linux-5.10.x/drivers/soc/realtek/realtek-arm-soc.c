#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/debugfs.h>

#ifdef CONFIG_ARM_DSU_PMU
#include <linux/arm-smccc.h>
#include <soc/cortina/cortina-smc.h>

#define CA_SMC_CALL_SREG_ACTLR_READ(elx)			rtk_smc_call(CA_SVC_SREG_ACTLR_READ, elx, 0, 0, 0)
#define CA_SMC_CALL_SREG_ACTLR_WRITE(elx, val) 		rtk_smc_call(CA_SVC_SREG_ACTLR_SET, BIT(elx), val, 0, 0)

#define CLUSTERPMUEN_BIT BIT(12)
#define EL2 0x2
#define EL3 0x3
#define ACTLR_CLUSTERPMUEN_PASS 1
#define ACTLR_CLUSTERPMUEN_FAIL 0


u64 rtk_smc_call(unsigned int smc_id, unsigned long x1,unsigned long x2, \
		unsigned long x3, unsigned long x4)
{
	struct arm_smccc_res res;

	if(x1==0){
		printk("Illegal SMC call(Address 0)!\n");
		while(1);
	}

	arm_smccc_smc(smc_id, x1, x2, x3, 0, 0, 0, 0, &res);
	return res.a0;
}




static void rtk_actlr_set(void *info){
	unsigned int *result = (unsigned int *)info;
	unsigned long val;

	val = CA_SMC_CALL_SREG_ACTLR_READ(EL3) | CLUSTERPMUEN_BIT;
	CA_SMC_CALL_SREG_ACTLR_WRITE(EL3, val);
	val = CA_SMC_CALL_SREG_ACTLR_READ(EL2) | CLUSTERPMUEN_BIT;
	CA_SMC_CALL_SREG_ACTLR_WRITE(EL2, val);


	if((CA_SMC_CALL_SREG_ACTLR_READ(EL3) & CLUSTERPMUEN_BIT )
		&& (CA_SMC_CALL_SREG_ACTLR_READ(EL2) & CLUSTERPMUEN_BIT )){
		*result =1;
		return;
	}
	*result = 0;
}

static void rtk_actlr_confirm_smp(void){
	int cpu;
	unsigned int result;
	for_each_online_cpu(cpu) {
		smp_call_function_single(cpu, rtk_actlr_set, (void *)&result, 1);
		if(result ==0){
			printk("DSU_PMU: ACTLR_EL2/3 - Can't set CLUSTERPMUEN bit.\n");
			return;
		}
	}
}
#endif

static int rtk_soc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	dev_info(dev, "%s probe.\n", np->name);
#ifdef CONFIG_ARM_DSU_PMU
	rtk_actlr_confirm_smp();
#endif

	return 0;
}

static int rtk_soc_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id rtk_soc_dt_match[] = {
	{ .compatible = "realtek,arm-soc", },
	{ },
};

MODULE_DEVICE_TABLE(of, rtk_soc_dt_match);

static struct platform_driver rtk_soc_driver = {
	.driver = {
		.name		= "rtk_soc",
		.owner		= THIS_MODULE,
		.of_match_table	= rtk_soc_dt_match,
	},
	.probe = rtk_soc_probe,
	.remove	= rtk_soc_remove,
};

module_platform_driver(rtk_soc_driver);


MODULE_DESCRIPTION("Reaktek SoC driver");
MODULE_LICENSE("GPL v2");
