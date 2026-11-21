#include <common.h>
#include <command.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/sizes.h>

#include <asm/barriers.h>

#include "ca_atu_func.h"


struct atu_param {
	u32 input;
	u32 output;
	u32 size;
};

struct atu_table {
	struct	atu_param user;
	bool	enable;
};

struct ca_atu_dev {
	/* IOMMU core code handle */

	void __iomem		*base;

	int			type;
	u32			hi_addr; /* DMA coherent high 32 bit address */
	struct atu_table	table[TLB_COUNT];
	struct atu_param	user;
	bool			keep;
	bool			coherent;
};
static void ca_atu_writel(u32 value, void *ptr)
{
	writel(value, ptr);
	printk("%p <= 0x%08x\n",  ptr, value);
}


static int ca_atu_clr(struct ca_atu_dev *atu_dev, int idx)
{
	union atu_ctrl ctrl;
	void __iomem *base = atu_dev->base;
	int type = atu_dev->type;
	int ret;
	u32 val;

	ca_atu_writel(0, base + ATU_INPUT);
	ca_atu_writel(0, base + ATU_MASK);
	ca_atu_writel(0, base + ATU_OUTPUT);

	if (type == T_AXI_STD || type == T_AXI_PLUS) {
		ca_atu_writel(0, base + ACE_CFG);
		ca_atu_writel(0, base + AXI_ARUSER);
		ca_atu_writel(0, base + AXI_AWUSER);
		if (type == T_AXI_PLUS)
			ca_atu_writel(0, base + AXI_UPPER);
	} else {
		ca_atu_writel(0, base + ACE_CFG);
		ca_atu_writel(0, base + AHB_HUSER);
		ca_atu_writel(0, base + AHB_UPPER);
	}
	ctrl.wrd = readl(base + ATU_CTRL);
	ctrl.bf.enable &= ~(0x1 << idx);
	ctrl.bf.idx = idx;
	ctrl.bf.w = 1;
	ca_atu_writel(ctrl.wrd, base + ATU_CTRL);
	ret = readl_poll_sleep_timeout(base + ATU_CTRL, val, !(val & ATU_CTRL_W),
				 1, 100000);
	if (ret)
		printf("ATU clear time out\n");

	return ret;
}

static int ca_atu_cfg(struct ca_atu_dev *atu_dev, int idx, bool enable)
{
	union atu_ctrl ctrl;
	void __iomem *base = atu_dev->base;
	int type = atu_dev->type;
	int ret;
	u32 val;

	if (enable) {
		ca_atu_writel(atu_dev->user.input, base + ATU_INPUT);
		ca_atu_writel((atu_dev->user.size - 1) ^ 0xFFFFFFFF, base + ATU_MASK);
		ca_atu_writel(atu_dev->user.output, base + ATU_OUTPUT);
		if(atu_dev->coherent ==1){
			if (type == T_AXI_STD || type == T_AXI_PLUS) {
				union axi_cfg axi;
				union user_attr aruser, awuser;

				axi.wrd = 0;
				axi.bf.arprot = AXPROT_NONSECURE_ACCESS;
				axi.bf.awprot = AXPROT_NONSECURE_ACCESS;
				axi.bf.prot_en = 1;
				axi.bf.arqos = 0;
				axi.bf.awqos = 0;
				axi.bf.qos_en = 0;
				axi.bf.arcache = AXCACHE_BUFFERABLE |
						 AXCACHE_CACHEABLE |
						 AXCACHE_READ_ALLOCATE |
						 AXCACHE_WRITE_ALLOCATE;
				axi.bf.awcache = AXCACHE_BUFFERABLE |
						 AXCACHE_CACHEABLE |
						 AXCACHE_READ_ALLOCATE |
						 AXCACHE_WRITE_ALLOCATE;
				axi.bf.cache_en = 1;
				axi.bf.user_en = 1;

				aruser.wrd = 0;
				aruser.bf.axuser = 0;
				aruser.bf.axdomain = AXDOMAIN_OUTER_SHAREABLE;

				aruser.bf.axbar = AXBAR_NORMAL_ACCESS_S;
				aruser.bf.axsnoop = ARSNOOP_READONCE;

				awuser.wrd = 0;
				awuser.bf.axuser = 0;
				awuser.bf.axdomain = AXDOMAIN_OUTER_SHAREABLE;

				awuser.bf.axbar = AXBAR_NORMAL_ACCESS_S;
				awuser.bf.axsnoop = AWSNOOP_WRITEUNIQUE;

				ca_atu_writel(axi.wrd, base + ACE_CFG);
				ca_atu_writel(aruser.wrd, base + AXI_ARUSER);
				ca_atu_writel(awuser.wrd, base + AXI_AWUSER);
				if (type == T_AXI_PLUS)
					ca_atu_writel(0, base + AXI_UPPER);
			} else {
				union ahb_cfg ahb;
				union user_attr huser;

				ahb.wrd = 0;
				ahb.bf.hprot = HPROT_PRIVILEGED_ACCESS;
				ahb.bf.prot_en = 1;
				ahb.bf.user_en = 1;

				huser.wrd = 0;
				huser.bf.axuser = 0;
				huser.bf.axdomain = AXDOMAIN_OUTER_SHAREABLE;
				huser.bf.axbar = AXBAR_NORMAL_ACCESS_S;
				huser.bf.axsnoop = AWSNOOP_WRITEUNIQUE;

				ca_atu_writel(ahb.wrd, base + ACE_CFG);
				ca_atu_writel(huser.wrd, base + AHB_HUSER);
				ca_atu_writel(0, base + AHB_UPPER);
			}
		}
	}

	ctrl.wrd = readl(base + ATU_CTRL);
	if (enable)
		ctrl.bf.enable |= (enable << idx);
	else
		ctrl.bf.enable &= ~(0x1 << idx);
	ctrl.bf.idx = idx;
	ctrl.bf.w = 1;
	ca_atu_writel(ctrl.wrd, base + ATU_CTRL);
	ret = readl_poll_sleep_timeout(base + ATU_CTRL, val, !(val & ATU_CTRL_W),
				 1, 100000);
	if (ret)
		printf("ATU configure time out\n");

	if (enable) {
		atu_dev->table[idx].user.input = atu_dev->user.input;
		atu_dev->table[idx].user.size = atu_dev->user.size;
		atu_dev->table[idx].user.output = atu_dev->user.output;

		printf( "enable(%d) input 0x%08x, size 0x%08x, output 0x%08x\n",
			idx, atu_dev->user.input, atu_dev->user.size,
			atu_dev->user.output);
	} else {
		printf("disable(%d)\n", idx);
	}
	atu_dev->table[idx].enable = enable;

	return 0;
}

void ca_atu_setup(u64 atu_reg, int type, int coherent){
	struct ca_atu_dev atu_dev = {0};
	int i;


	atu_dev.base = (void *)atu_reg;
	atu_dev.type = type;
	atu_dev.coherent = coherent;
	for (i = 0; i < TLB_COUNT; i++)
		ca_atu_clr(&atu_dev, i);
#ifdef CONFIG_TARGET_RTK_MERCURY
	/* Add an entry for HW cache coherence of whole DDR here temporarily */
	atu_dev.hi_addr = 0x4;
	atu_dev.user.input = 0;
	atu_dev.user.size = SZ_2G / SZ_64K;
	atu_dev.user.output = atu_dev.hi_addr << 16;
	ca_atu_cfg(&atu_dev, 0, true);

#else
	/* Add an entry for HW cache coherence of whole DDR here temporarily */
	atu_dev.hi_addr = 0x2;
	atu_dev.user.input = 0;
	atu_dev.user.size = SZ_2G / SZ_64K;
	atu_dev.user.output = atu_dev.hi_addr << 16;
	ca_atu_cfg(&atu_dev, 0, true);

	atu_dev.user.input = 0x18000;
	atu_dev.user.size = SZ_2G / SZ_64K;
	atu_dev.user.output = atu_dev.user.input + (atu_dev.hi_addr << 16);
	ca_atu_cfg(&atu_dev, 1, true);
#endif

}
void ca_atu_clear(u64 atu_reg, int type){
	struct ca_atu_dev atu_dev = {0};
	int i;

	atu_dev.base = (void *)atu_reg;
	atu_dev.type = type;

	for (i = 0; i < TLB_COUNT; i++)
		ca_atu_clr(&atu_dev, i);
}

