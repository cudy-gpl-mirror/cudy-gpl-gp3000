/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 */

#ifndef __OSAL_H__
#define __OSAL_H__
///--
#include <osal_type.h>

#include <asm/io.h>
#include <linux/delay.h>
//stantard
#include <string.h> /* strcmp() ... */
#include <stdlib.h> /* strtol() ... */
///--#include <toolchain/gcc.h>

/* for pe zephyr, please modify it to uboot */
#if 1 ///--
#define PHYS_TO_UNCACHE_VIRT(x)                      (unsigned char *)phys_to_virt((unsigned int) x)    
#define PHYS_TO_CACHE_VIRT(x)                        (unsigned char *)phys_to_virt((unsigned int) x)
#define PHYS_TO_VIRT(x)                              PHYS_TO_UNCACHE_VIRT(x)
#define VIRT_TO_PHYS(x)                              virt_to_phys((unsigned int) x)

#define DMA_MAP(p,l)
#define DMA_UNMAP(p,l)

#define PRINTF(...)                                 ((void)0)
#define PRINTF_DEBUG(...)                           ((void)0)
#define MDELAY(MS)                                   mdelay(MS)
#define CA_INIT_DATE "20240828"

#define DCACHE_INV(ADDR_START, ADDR_END) (invalidate_dcache_range(ADDR_START, ADDR_END))
#define DCACHE_WB(ADDR_START, ADDR_END) (flush_dcache_range(ADDR_START, ADDR_END))
#else
//stantard
#include <string.h>	/* strcmp() ... */
#include <stdlib.h>	/* strtol() ... */
#include <toolchain/gcc.h>

/* for pe zephyr, please modify it to uboot */                     
#define PHYS_TO_UNCACHE_VIRT(x)						 (unsigned char *)phys_to_uncached_virt((unsigned int) x)	
#define PHYS_TO_CACHE_VIRT(x)						 (unsigned char *)phys_to_cached_virt((unsigned int) x)
#define PHYS_TO_VIRT(x)                              PHYS_TO_UNCACHE_VIRT(x)
#define VIRT_TO_PHYS(x)                              virt_to_phys((unsigned int) x)

#define DMA_MAP(p,l)
#define DMA_UNMAP(p,l)

#define PRINTF 										 printk
#define PRINTF_DEBUG
#define MDELAY(MS)                                   k_busy_wait(MS)
#define CA_INIT_DATE "20240828"

#define DCACHE_INV(ADDR_START, ADDR_END) (cache_data_range(ADDR_START, (ADDR_END - ADDR_START), K_CACHE_INVD))
#define DCACHE_WB(ADDR_START, ADDR_END) (cache_data_range(ADDR_START, (ADDR_END - ADDR_START), K_CACHE_WB))
#endif

#endif /* __CA_OSAL_H__ */
