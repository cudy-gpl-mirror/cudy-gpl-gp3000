/*
 * Copyright (c) 2015-2018, Cortina Access. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __VENUS_GIC3_H__
#define __VENUS_GIC3_H__

#define ARM_IRQ_SEC_PHY_TIMER		29

#define ARM_IRQ_SEC_SGI_0		8
#define ARM_IRQ_SEC_SGI_1		9
#define ARM_IRQ_SEC_SGI_2		10
#define ARM_IRQ_SEC_SGI_3		11
#define ARM_IRQ_SEC_SGI_4		12
#define ARM_IRQ_SEC_SGI_5		13
#define ARM_IRQ_SEC_SGI_6		14
#define ARM_IRQ_SEC_SGI_7		15

/*
 * List of secure interrupts are deprecated, but are retained only to support
 * legacy configurations.
 */
#define ARM_G1S_IRQS			ARM_IRQ_SEC_PHY_TIMER,		\
					ARM_IRQ_SEC_SGI_1,		\
					ARM_IRQ_SEC_SGI_2,		\
					ARM_IRQ_SEC_SGI_3,		\
					ARM_IRQ_SEC_SGI_4,		\
					ARM_IRQ_SEC_SGI_5,		\
					ARM_IRQ_SEC_SGI_7

#define ARM_G0_IRQS			ARM_IRQ_SEC_SGI_0,		\
					ARM_IRQ_SEC_SGI_6

/*
 * Define a list of Group 1 Secure and Group 0 interrupt properties as per GICv3
 * terminology. On a GICv2 system or mode, the lists will be merged and treated
 * as Group 0 interrupts.
 */
#define ARM_G1S_IRQ_PROPS(grp) \
	INTR_PROP_DESC(ARM_IRQ_SEC_PHY_TIMER, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_1, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_2, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_3, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_4, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_5, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_7, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE)

#define ARM_G0_IRQ_PROPS(grp) \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_0, PLAT_SDEI_NORMAL_PRI, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(ARM_IRQ_SEC_SGI_6, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE)



/*
 * Define a list of Group 1 Secure and Group 0 interrupts as per GICv3
 * terminology. On a GICv2 system or mode, the lists will be merged and treated
 * as Group 0 interrupts.
 */
#define PLAT_ARM_G1S_IRQS		ARM_G1S_IRQS,			\
					FVP_IRQ_TZ_WDOG,		\
					FVP_IRQ_SEC_SYS_TIMER

#define PLAT_ARM_G0_IRQS		ARM_G0_IRQS

#define PLAT_ARM_G1S_IRQ_PROPS(grp) \
	ARM_G1S_IRQ_PROPS(grp), \
	INTR_PROP_DESC(FVP_IRQ_TZ_WDOG, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL), \
	INTR_PROP_DESC(FVP_IRQ_SEC_SYS_TIMER, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_LEVEL)

#define PLAT_ARM_G0_IRQ_PROPS(grp)	ARM_G0_IRQ_PROPS(grp)


unsigned int plat_arm_get_cluster_core_count(u_register_t mpidr);
void plat_arm_gic_driver_init(void);
void plat_arm_gic_init(void);
void plat_arm_gic_cpuif_enable(void);
void plat_arm_gic_cpuif_disable(void);
void plat_arm_gic_redistif_on(void);
void plat_arm_gic_redistif_off(void);
void plat_arm_gic_pcpu_init(void);
void plat_arm_gic_save(void);
void plat_arm_gic_resume(void);

#endif /* __VENUS_GICv3_H__ */
