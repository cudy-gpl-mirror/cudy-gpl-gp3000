/*
 * Copyright (c) 2011 The Chromium OS Authors.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MIPS_CACHE_H__
#define __MIPS_CACHE_H__

/*
 * The maximum L1 data cache line size on MIPS seems to be 128 bytes.  We use
 * that as a default for aligning DMA buffers unless the board config has
 * specified another cache line size.
 */
#ifdef CONFIG_SYS_CACHELINE_SIZE
#define ARCH_DMA_MINALIGN	CONFIG_SYS_CACHELINE_SIZE
#else
#define ARCH_DMA_MINALIGN	128
#endif

#ifndef CONFIG_SYS_MIPS_CACHE_MODE
# ifdef CONFIG_SMP
#  define CONFIG_SYS_MIPS_CACHE_MODE CONF_CM_CACHABLE_COW
# else
#  define CONFIG_SYS_MIPS_CACHE_MODE CONF_CM_CACHABLE_NONCOHERENT
# endif
#endif

#ifdef __ASSEMBLY__

#if defined(CONFIG_CPU_MIPS32) || defined(CONFIG_CPU_MIPS64)
	.macro setup_c0_config set
	.set    push
	mfc0	t0, CP0_CONFIG
	ori	t0, CONF_CM_CMASK
	xori	t0, CONF_CM_CMASK
	ori	t0, \set
	mtc0	t0, CP0_CONFIG
	.set pop
	.endm
#else
	.macro setup_c0_config set
	.endm
#endif

#endif

#endif /* __MIPS_CACHE_H__ */
