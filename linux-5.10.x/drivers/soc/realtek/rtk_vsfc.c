#include <linux/module.h>
#include <linux/init.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <soc/cortina/registers.h>
/***********************************************
 * Workaround:
 * GLOBAL_UVLO_xxx: Write need to do 2 times.
************************************************/

#define writel_2t(v,c)		({ writel(v,c); writel(v,c); })



#ifndef GLOBAL_VSFC_STATUS
#define GLOBAL_VSFC_STATUS   GLOBAL_UVLO_STATUS

typedef volatile union {
  struct {
    ca_uint32_t idle_st              :  1 ; /* bits 0:0 */
    ca_uint32_t rsrvd1               :  3 ;
    ca_uint32_t fss_detect_out       :  5 ; /* bits 8:4 */
    ca_uint32_t rsrvd2               :  3 ;
    ca_uint32_t uvlo_detect_out      :  5 ; /* bits 16:12 */
    ca_uint32_t rsrvd3               : 15 ;
  } bf ;
  ca_uint32_t     wrd ;
} GLOBAL_VSFC_STATUS_t;

#endif


#define TRIG_SEL_CPU_INTERNAL	1<<0
#define TRIG_SEL_UVLO			1<<1
#define TRIG_SEL_SPEED_SENESOR	1<<2


#define VSFC_BASE	GLOBAL_UVLO_CONTROL_GAINN
#define GLOBAL_UVLO_CONTROL_OFF		(GLOBAL_UVLO_CONTROL - VSFC_BASE)
#define GLOBAL_VSFC_STATUS_OFF		(GLOBAL_VSFC_STATUS - VSFC_BASE)


#define GLOBAL_UVLO_CONTROL_GAINN_OFF	(GLOBAL_UVLO_CONTROL_GAINN - VSFC_BASE)
#define GLOBAL_UVLO_CONTROL_GAINP_OFF	(GLOBAL_UVLO_CONTROL_GAINP - VSFC_BASE)

#define GLOBAL_UVLO_TRIG_CONTROL_OFF (GLOBAL_UVLO_TRIG_CONTROL - VSFC_BASE)

#define GLOBAL_UVLO_RECV_CTRL0_OFF (GLOBAL_UVLO_RECV_CTRL0 - VSFC_BASE)
#define GLOBAL_UVLO_RECV_CTRL1_OFF (GLOBAL_UVLO_RECV_CTRL1 - VSFC_BASE)
#define GLOBAL_UVLO_RECV_CTRL2_OFF (GLOBAL_UVLO_RECV_CTRL2 - VSFC_BASE)
#define GLOBAL_UVLO_RECV_CTRL3_OFF (GLOBAL_UVLO_RECV_CTRL3 - VSFC_BASE)
#define GLOBAL_UVLO_RECV_CTRL4_OFF (GLOBAL_UVLO_RECV_CTRL4 - VSFC_BASE)

struct realtek_vsfc {
	struct platform_device *pdev;
	void __iomem		*regs;
	GLOBAL_UVLO_CONTROL_GAINN_t gainn; /*  High to Low detect threshold */
	GLOBAL_UVLO_CONTROL_GAINP_t gainp; /* Low to High detect threshold  */
	GLOBAL_UVLO_CONTROL_t ctrl;
	GLOBAL_UVLO_TRIG_CONTROL_t	trig_ctrl;
	GLOBAL_UVLO_RECV_CTRL0_t	recv_ctrl0;
	GLOBAL_UVLO_RECV_CTRL1_t	recv_ctrl1;
	GLOBAL_UVLO_RECV_CTRL2_t	recv_ctrl2;
	GLOBAL_UVLO_RECV_CTRL3_t	recv_ctrl3;
	GLOBAL_UVLO_RECV_CTRL4_t	recv_ctrl4;
};
static struct realtek_vsfc *vsfc;


static int vsfc_init(void){
	ktime_t timeout;
	GLOBAL_VSFC_STATUS_t		status;
	GLOBAL_UVLO_CONTROL_t		ctrl;
	GLOBAL_UVLO_TRIG_CONTROL_t	trig_ctrl;

	ctrl.wrd = readl(vsfc->regs + GLOBAL_UVLO_CONTROL_OFF);
	/* disable VSFC */
	ctrl.bf.glb_en = 0;
	writel_2t(ctrl.wrd , vsfc->regs + GLOBAL_UVLO_CONTROL_OFF);

	/* Only program VSFC control registers when CTRL.glb_en is 0 and VSFC is in idle state  */
	timeout = ktime_add_ms(ktime_get(), 100);
	do {
		status.wrd = readl(vsfc->regs + GLOBAL_VSFC_STATUS_OFF);
		rmb();
	} while ((status.bf.idle_st != 1) && ktime_before(ktime_get(), timeout));


	switch (vsfc->ctrl.bf.trig_sel){
		case TRIG_SEL_CPU_INTERNAL:
			/* CASE: core wakeup and digital masking */
			ctrl.bf.trig_sel = vsfc->ctrl.bf.trig_sel;
			ctrl.bf.div_sel = vsfc->ctrl.bf.div_sel;

			writel_2t(ctrl.wrd , vsfc->regs + GLOBAL_UVLO_CONTROL_OFF);

			trig_ctrl.wrd = readl(vsfc->regs + GLOBAL_UVLO_TRIG_CONTROL_OFF);
			trig_ctrl.bf.core_min = vsfc->trig_ctrl.bf.core_min;
			trig_ctrl.bf.core_idle_div_en = vsfc->trig_ctrl.bf.core_idle_div_en;

			writel_2t(trig_ctrl.wrd, vsfc->regs + GLOBAL_UVLO_TRIG_CONTROL_OFF);

			writel_2t(vsfc->recv_ctrl0.wrd , vsfc->regs + GLOBAL_UVLO_RECV_CTRL0_OFF);

			writel_2t(vsfc->recv_ctrl1.wrd , vsfc->regs + GLOBAL_UVLO_RECV_CTRL1_OFF);
			writel_2t(vsfc->recv_ctrl2.wrd , vsfc->regs + GLOBAL_UVLO_RECV_CTRL2_OFF);
			writel_2t(vsfc->recv_ctrl3.wrd , vsfc->regs + GLOBAL_UVLO_RECV_CTRL3_OFF);
			wmb();

			ctrl.bf.glb_en = 1;
			writel_2t(ctrl.wrd , vsfc->regs + GLOBAL_UVLO_CONTROL_OFF);
			break;
		case TRIG_SEL_UVLO:
		case TRIG_SEL_SPEED_SENESOR:
			if(vsfc->gainn.bf.ca55_uvlo_reg_gainn){
				writel_2t(vsfc->gainn.wrd, vsfc->regs + GLOBAL_UVLO_CONTROL_GAINN_OFF);
			}
			if(vsfc->gainp.bf.ca55_uvlo_reg_gainp){
				vsfc->gainp.bf.ca55_uvlo_reg_pow = 0x1F;  /* When 0 the corresponding processor UVLO is kept in reset. */
				writel_2t(vsfc->gainp.wrd, vsfc->regs + GLOBAL_UVLO_CONTROL_GAINP_OFF);			
			}
			ctrl.bf.trig_sel = vsfc->ctrl.bf.trig_sel;
			ctrl.bf.div_sel = vsfc->ctrl.bf.div_sel;

			writel_2t(ctrl.wrd , vsfc->regs + GLOBAL_UVLO_CONTROL_OFF);
			writel_2t(vsfc->trig_ctrl.wrd, vsfc->regs + GLOBAL_UVLO_TRIG_CONTROL_OFF);
			writel_2t(vsfc->recv_ctrl0.wrd , vsfc->regs + GLOBAL_UVLO_RECV_CTRL0_OFF);

			writel_2t(vsfc->recv_ctrl1.wrd , vsfc->regs + GLOBAL_UVLO_RECV_CTRL1_OFF);
			writel_2t(vsfc->recv_ctrl2.wrd , vsfc->regs + GLOBAL_UVLO_RECV_CTRL2_OFF);
			writel_2t(vsfc->recv_ctrl3.wrd , vsfc->regs + GLOBAL_UVLO_RECV_CTRL3_OFF);
			wmb();
			ctrl.bf.glb_en = 1;
			writel_2t(ctrl.wrd , vsfc->regs + GLOBAL_UVLO_CONTROL_OFF);
			break;
		default:
			break;
	}
	return 0;
}


/************************Platform Operations*****************************/
/**
 * realtek_vsfc_probe - Probe call for the device.
 *
 * @pdev: handle to the platform device structure.
 * Return: 0 on success, negative error otherwise.
 *
 * It does all the memory allocation and registration for the device.
 */
static int realtek_vsfc_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;
	u32 value;

	vsfc = devm_kzalloc(&pdev->dev, sizeof(*vsfc), GFP_KERNEL);
	if (!vsfc)
		return -ENOMEM;

	vsfc->pdev = pdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	vsfc->regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (IS_ERR(vsfc->regs))
		return PTR_ERR(vsfc->regs);

	dev_info(&pdev->dev, "resource - %pr mapped at 0x%pK\n", res, vsfc->regs);

	ret = of_property_read_u32(pdev->dev.of_node, "trig_sel", &value);
	if (ret != 0){
		vsfc->ctrl.bf.trig_sel = 0;
	}else{
		vsfc->ctrl.bf.trig_sel = value;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "div_sel", &value);
	if (ret != 0){
		vsfc->ctrl.bf.div_sel = 0;
	}else{
		vsfc->ctrl.bf.div_sel = value;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "core_idle_div_en", &value);
	if (ret != 0){
		vsfc->trig_ctrl.bf.core_idle_div_en = 0;
	}else{
		vsfc->trig_ctrl.bf.core_idle_div_en = value;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "core_min", &value);
	if ((ret != 0) || (value == 0)){
		vsfc->trig_ctrl.bf.core_min = 0;
	}else{
		vsfc->trig_ctrl.bf.core_min = value - 1;
	}

	/*
	minimum FSS/UVLO trigger numbers to trigger VSFC
	Valid range: 0~4 	(Stands for 1 FSS signal ~ 5 FSS signals)
	*/

	ret = of_property_read_u32(pdev->dev.of_node, "fss_min", &value);
	if ((ret != 0) || (value == 0)){
		vsfc->trig_ctrl.bf.fss_min = 0;
	}else{
		vsfc->trig_ctrl.bf.fss_min = value - 1;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "uvlo_min", &value);
	if ((ret != 0) || (value == 0)){
		vsfc->trig_ctrl.bf.uvlo_min = 0;
	}else{
		vsfc->trig_ctrl.bf.uvlo_min = value - 1;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "det_recv_sel", &value);
	if (ret != 0){
		/* 2'b00 is an invalid setting. */
		vsfc->recv_ctrl0.bf.det_recv_sel = 1;
	}else{
		if((value >=1)&& (value <= 2))
			vsfc->recv_ctrl0.bf.det_recv_sel = value;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "recv_ctrl1", &value);
	if (ret != 0){
		vsfc->recv_ctrl1.wrd = 0x00000010;
	}else{
		vsfc->recv_ctrl1.wrd = value;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "recv_ctrl2", &value);
	if (ret != 0){
		vsfc->recv_ctrl2.wrd = 0x00000010;
	}else{
		vsfc->recv_ctrl2.wrd = value;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "recv_ctrl3", &value);
	if (ret != 0){
		vsfc->recv_ctrl3.wrd = 0x00000010;
	}else{
		vsfc->recv_ctrl3.wrd = value;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "recv_ctrl4", &value);
	if (ret != 0){
		vsfc->recv_ctrl4.wrd = 0;
	}else{
		vsfc->recv_ctrl4.wrd = value;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "control_gainn", &value);
	if (ret != 0){
		vsfc->gainn.wrd = 0;
	}else{
		vsfc->gainn.wrd = value;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "control_gainp", &value);
	if (ret != 0){
		vsfc->gainp.wrd = 0;
	}else{
		vsfc->gainp.wrd = value;
	}

	ret = vsfc_init();

	dev_info(&pdev->dev, "RTK VSFC driver init.");

	return ret;
}


static const struct of_device_id realtek_vsfc_of_match[] = {
	{ .compatible = "realtek,vsfc", },
	{ /* end of table */ }
};
MODULE_DEVICE_TABLE(of, realtek_vsfc_of_match);

/* Driver Structure */
static struct platform_driver realtek_vsfc_driver = {
	.probe		= realtek_vsfc_probe,
	.driver		= {
		.name	= "realtek-vsfc",
		.of_match_table = realtek_vsfc_of_match,
	},
};

module_platform_driver(realtek_vsfc_driver);

MODULE_DESCRIPTION("RealTek VSFC Driver");
MODULE_LICENSE("GPL");
