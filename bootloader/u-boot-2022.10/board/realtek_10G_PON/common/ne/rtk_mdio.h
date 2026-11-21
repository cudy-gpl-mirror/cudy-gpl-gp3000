/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 */
#ifndef __RTK_MDIO_H__
#define __RTK_MDIO_H__

#if (defined(CONFIG_TARGET_PRESIDIO_ASIC) || defined(CONFIG_TARGET_SATURN_ASIC) || defined(CONFIG_TARGET_VENUS) || defined(CONFIG_TARGET_RTK_TAURUS) || defined(CONFIG_TARGET_RTK_ELNATH)) && !defined(CONFIG_RTK_V7_FPGA_PLATFORM)
#define INTERNAL_PHY_EXIST 1
#endif

#if defined(CONFIG_TARGET_PRESIDIO_ASIC)
#define CA_NI_MDIO_REG_BASE 0xF4338
#else
#if defined(CONFIG_TARGET_VENUS)
#define CA_NI_MDIO_REG_BASE 0xF4339
#elif defined(CONFIG_TARGET_RTK_TAURUS)
#define CA_NI_MDIO_REG_BASE 0xF4600
#elif defined(CONFIG_TARGET_RTK_ELNATH)
#define CA_NI_MDIO_REG_BASE 0xF4600
#elif defined(CONFIG_TARGET_RTK_MERCURY)
#define CA_NI_MDIO_REG_BASE 0xF4333
#else
#define CA_NI_MDIO_REG_BASE 0xD000B
#endif
#endif

typedef volatile union {
	struct {
		ca_uint32 reserved : 2; /* bits  1:0 */
		ca_uint32 reg_off : 5; /* bits  6:2 */
		ca_uint32 phy_addr : 5; /* bits 11:7 */
		ca_uint32 reg_base : 20; /* bits 31:12 */
	} bf;
	ca_uint32 wrd;
} NI_MDIO_OPER_T;

#define __MDIO_WR_FLAG				(0)
#define __MDIO_RD_FLAG				(1)
#define __MDIO_ACCESS_TIMEOUT			(1000000)
#define CA_MDIO_ADDR_MIN			(1)
#define CA_MDIO_ADDR_MAX			(31)

#define __MDIO_ADDR_CHK(addr) \
	do { \
		if ((addr) < CA_MDIO_ADDR_MIN || (addr) > CA_MDIO_ADDR_MAX) { \
			return CA_E_PARAM; \
		} \
	} while (0)

#define CA_ASSERT(x) \
			do { \
				if (!(x)) return CA_E_PARAM; \
			} while (0)

ca_status_t ca_mdio_write (
		CA_IN		ca_uint32_t 			addr,
		CA_IN		ca_uint32_t 			st_code,
		CA_IN		ca_uint32_t 			offset,
		CA_IN		ca_uint32_t 			waddr,
		CA_IN		ca_uint16_t 			data);

ca_status_t ca_mdio_read (
		CA_IN       ca_uint32_t             addr,
		CA_IN       ca_uint32_t             st_code,
		CA_IN       ca_uint32_t             offset,
		CA_IN       ca_uint32_t             waddr,
		CA_OUT      ca_uint16_t             *data);

int rtk_ne_miiphy_read(struct mii_dev *bus, int addr, int devad, int reg);
int rtk_ne_miiphy_write(struct mii_dev *bus, int addr, int devad, int reg, u16 value);

#endif /* __RTK_MDIO_H__ */
