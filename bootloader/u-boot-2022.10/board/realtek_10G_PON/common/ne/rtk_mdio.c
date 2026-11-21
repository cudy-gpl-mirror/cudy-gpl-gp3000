#include <miiphy.h>
#include <asm/io.h>
#include <osal_type.h>
#include <rtk_mdio.h>
#include <realtek_ne.h>


static ca_status_t ca_mdio_write_rgmii(ca_uint32_t addr, ca_uint32_t offset, ca_uint16_t data)
{
	PER_MDIO_ADDR_t  mdio_addr;
	PER_MDIO_CTRL_t  mdio_ctrl;
	ca_uint32_t      loop_wait = __MDIO_ACCESS_TIMEOUT; /* up to 10000 cycles*/

	mdio_addr.wrd            = 0;
	mdio_addr.bf.mdio_addr   = addr;
	mdio_addr.bf.mdio_offset = offset;
	mdio_addr.bf.mdio_rd_wr  = __MDIO_WR_FLAG;
	CA_REG_WRITE(mdio_addr.wrd, PER_MDIO_ADDR);
	CA_REG_WRITE(data, PER_MDIO_WRDATA);

#if CORTINA_NI_DBG
	printf("%s: mdio_addr.wrd=0x%x\n", __func__, mdio_addr.wrd);
#endif

	mdio_ctrl.wrd          = 0;
	mdio_ctrl.bf.mdiostart = 1;
	CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);

#if CORTINA_NI_DBG
	printf("%s: phy_addr=%d, offset=%d, data=0x%x\n", __func__, addr, offset, data);
#endif

	do {
		mdio_ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);
		if (mdio_ctrl.bf.mdiodone) {
			CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);
			return CA_E_OK;
		}
	} while(--loop_wait);

	printf("%s: PHY rite timeout!!!\n", __func__);
	return CA_E_TIMEOUT;
}

static ca_status_t ca_mdio_write_rgmii_indirect(ca_uint32_t addr, ca_uint32_t offset, ca_uint32_t waddr, ca_uint16_t data)
{
	PER_MDIO_ADDR_t  mdio_addr;
	PER_MDIO_CTRL_t  mdio_ctrl;
	ca_uint32_t      loop_wait = __MDIO_ACCESS_TIMEOUT; /* up to 10000 cycles*/

	mdio_addr.wrd            = 0;
	mdio_addr.bf.mdio_addr   = addr;
	mdio_addr.bf.mdio_op     = 0;                       /* address write */
	mdio_addr.bf.mdio_offset = offset;                  /* region area */
	mdio_addr.bf.mdio_rd_wr  = __MDIO_WR_FLAG;
	mdio_addr.bf.mdio_st     = 1;                       /* indirect access */
	CA_REG_WRITE(mdio_addr.wrd, PER_MDIO_ADDR);
	CA_REG_WRITE(waddr, PER_MDIO_WRDATA);

	mdio_ctrl.wrd          = 0;
    mdio_ctrl.bf.mdiostart = 1;
	CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);

	do {
		mdio_ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);
		if (mdio_ctrl.bf.mdiodone) {
			CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);
			break;
		}
	} while(--loop_wait);
	if (loop_wait == 0) {
		printf("%s: PHY rite timeout!!!\n", __func__);
		return CA_E_TIMEOUT;
	}

	mdio_addr.wrd            = 0;
	mdio_addr.bf.mdio_addr   = addr;
	mdio_addr.bf.mdio_offset = offset;
	mdio_addr.bf.mdio_op     = 1;           /* indriect WRITE */
	mdio_addr.bf.mdio_st     = 1;	    /* indirect access */

	mdio_addr.bf.mdio_rd_wr  = __MDIO_WR_FLAG;
	CA_REG_WRITE(mdio_addr.wrd, PER_MDIO_ADDR);
	CA_REG_WRITE(data, PER_MDIO_WRDATA);

#if CORTINA_NI_DBG
	printf("%s: mdio_addr.wrd=0x%x\n", __func__, mdio_addr.wrd);
#endif

	mdio_ctrl.wrd          = 0;
	mdio_ctrl.bf.mdiostart = 1;
	CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);

#if CORTINA_NI_DBG
	printf("%s: phy_addr=%d, offset=0x%x, waddr=0x%x, data=0x%x\n", __func__, addr, offset, waddr, data);
#endif

	do {
		mdio_ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);
		if (mdio_ctrl.bf.mdiodone) {
			CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);
			return CA_E_OK;
		}
	} while(--loop_wait);

	printf("%s: PHY rite timeout!!!\n", __func__);
	return CA_E_TIMEOUT;
}

ca_status_t ca_mdio_write (
		CA_IN       ca_uint32_t             addr,
		CA_IN       ca_uint32_t             st_code,
		CA_IN       ca_uint32_t             offset,
		CA_IN       ca_uint32_t             waddr,
		CA_IN       ca_uint16_t             data)
{
#if defined(INTERNAL_PHY_EXIST)
	NI_MDIO_OPER_T	mdio_oper;
	__MDIO_ADDR_CHK(addr);

	/* the phy addr 5 is connect to RGMII */
	if (addr >= 5) {
		if (st_code == 0)
			return ca_mdio_write_rgmii_indirect(addr, offset, waddr, data);
		else
			return ca_mdio_write_rgmii(addr, offset, data);
	}
	else {
		mdio_oper.wrd = 0;
		mdio_oper.bf.reg_off = offset;
		mdio_oper.bf.phy_addr = addr;
		mdio_oper.bf.reg_base = CA_NI_MDIO_REG_BASE;
		CA_REG_WRITE(data, mdio_oper.wrd);

#if CORTINA_NI_DBG
		printf("%s: mdio_oper.wrd=0x%x, data=0x%x\n", __func__, mdio_oper.wrd, data);
#endif
		return CA_E_OK;
	}
#else
        __MDIO_ADDR_CHK(addr);

	if (st_code == 0)
		return ca_mdio_write_rgmii_indirect(addr, offset, waddr, data);
	else
		return ca_mdio_write_rgmii(addr, offset, data);
#endif
}

static ca_status_t ca_mdio_read_rgmii(ca_uint32_t addr, ca_uint32_t offset, ca_uint16_t *data)
{
	PER_MDIO_ADDR_t  mdio_addr;
	PER_MDIO_CTRL_t  mdio_ctrl;
	PER_MDIO_RDDATA_t  read_data;
	ca_uint32_t      loop_wait = __MDIO_ACCESS_TIMEOUT;

	mdio_addr.wrd            = 0;
	mdio_addr.bf.mdio_addr   = addr;
	mdio_addr.bf.mdio_offset = offset;
	mdio_addr.bf.mdio_rd_wr  = __MDIO_RD_FLAG;
	CA_REG_WRITE(mdio_addr.wrd, PER_MDIO_ADDR);

	mdio_ctrl.wrd          = 0;
	mdio_ctrl.bf.mdiostart = 1;
	CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);

	do {
		mdio_ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);
		if (mdio_ctrl.bf.mdiodone) {
			CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);
			read_data.wrd = CA_REG_READ(PER_MDIO_RDDATA);
			*data = read_data.bf.mdio_rddata;
#if CORTINA_NI_DBG
			printf("%s: phy_addr=%d, offset=%d, data=0x%x\n", __func__, addr, offset, *data);
#endif
			return CA_E_OK;
		}
	} while(--loop_wait);

	printf("%s: CA_E_TIMEOUT!!\n", __func__);
	return CA_E_TIMEOUT;
}

static ca_status_t ca_mdio_read_rgmii_indirect(ca_uint32_t addr, ca_uint32_t offset, ca_uint32_t waddr, ca_uint16_t *data)
{
	PER_MDIO_ADDR_t  mdio_addr;
	PER_MDIO_CTRL_t  mdio_ctrl;
	PER_MDIO_RDDATA_t  read_data;
	ca_uint32_t      loop_wait = __MDIO_ACCESS_TIMEOUT;

	mdio_addr.wrd            = 0;
	mdio_addr.bf.mdio_addr   = addr;
	mdio_addr.bf.mdio_op     = 0;           /* address write */
	mdio_addr.bf.mdio_offset = offset;                  /* region area */
	mdio_addr.bf.mdio_rd_wr  = __MDIO_WR_FLAG;
	mdio_addr.bf.mdio_st     = 1;           /* indirect access */
	CA_REG_WRITE(mdio_addr.wrd, PER_MDIO_ADDR);
	CA_REG_WRITE(waddr, PER_MDIO_WRDATA);
	mdio_ctrl.wrd          = 0;
	mdio_ctrl.bf.mdiostart = 1;
	CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);
	do {
		mdio_ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);
		if (mdio_ctrl.bf.mdiodone) {
			CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);
			break;
		}
	} while(--loop_wait);
	if (loop_wait == 0) {
		printf("%s: CA_E_TIMEOUT!!\n", __func__);
		return CA_E_TIMEOUT;
	}

	mdio_addr.wrd            = 0;
	mdio_addr.bf.mdio_addr   = addr;
	mdio_addr.bf.mdio_op     = 3;             /* indirect read */
	mdio_addr.bf.mdio_offset = offset;
	mdio_addr.bf.mdio_st     = 1;             /* indirect access */
	mdio_addr.bf.mdio_rd_wr  = __MDIO_RD_FLAG;
	CA_REG_WRITE(mdio_addr.wrd, PER_MDIO_ADDR);

	mdio_ctrl.wrd          = 0;
	mdio_ctrl.bf.mdiostart = 1;
	CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);

	do {
		mdio_ctrl.wrd = CA_REG_READ(PER_MDIO_CTRL);
		if (mdio_ctrl.bf.mdiodone) {
			CA_REG_WRITE(mdio_ctrl.wrd, PER_MDIO_CTRL);
			read_data.wrd = CA_REG_READ(PER_MDIO_RDDATA);
			*data = read_data.bf.mdio_rddata;
#if CORTINA_NI_DBG
			printf("%s: phy_addr=%d, offset=0x%x, waddr=0x%x, data=0x%x\n", __func__, addr, offset, waddr, *data);
#endif
			return CA_E_OK;
		}
	} while(--loop_wait);

	printf("%s: CA_E_TIMEOUT!!\n", __func__);
	return CA_E_TIMEOUT;
}

ca_status_t ca_mdio_read (
		CA_IN       ca_uint32_t             addr,
		CA_IN       ca_uint32_t             st_code,
		CA_IN       ca_uint32_t             offset,
		CA_IN       ca_uint32_t             waddr,
		CA_OUT      ca_uint16_t             *data)
{
#if defined(INTERNAL_PHY_EXIST)
	NI_MDIO_OPER_T  mdio_oper;

	CA_ASSERT(data);
	__MDIO_ADDR_CHK(addr); /* support range: 1~31*/

	/* the phy addr 5 is connect to RGMII */
	if (addr >= 5) {
		if (st_code == 0)
			return ca_mdio_read_rgmii_indirect(addr, offset, waddr, data);
		else
			return ca_mdio_read_rgmii(addr, offset, data);
	}
	else {
		mdio_oper.wrd = 0;
		mdio_oper.bf.reg_off = offset;
		mdio_oper.bf.phy_addr = addr;
		mdio_oper.bf.reg_base = CA_NI_MDIO_REG_BASE;
		*data = CA_REG_READ(mdio_oper.wrd);
		return CA_E_OK;
	}
#else
        CA_ASSERT(data);
        __MDIO_ADDR_CHK(addr); /* support range: 1~31*/

	if (st_code == 0)
		return ca_mdio_read_rgmii_indirect(addr, offset, waddr, data);
	else
		return ca_mdio_read_rgmii(addr, offset, data);
#endif
}

static inline int is_internal_phy(int addr)
{
	struct eth_device *dev = eth_get_dev_by_name(CONFIG_IDENT_STRING);
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);
	uint32 totalLanPort = rtk_ne_max_lan_port_num_get();
	int32 i;

	for (i = 0; i < totalLanPort; i++)
	{
		if (priv->lan_port_table[i].dport == -1)
			continue;

		if (priv->lan_port_table[i].phyAddr == addr 
			&& priv->lan_port_table[i].serdesMode == NE_SERDES_MODE_NONE)
			return 1;
	}

	return 0;
}

int rtk_ne_miiphy_write(struct mii_dev *bus, int addr, int devad, int reg, u16 value)
{
	int result;

	if (is_internal_phy(addr)) {
		NI_MDIO_OPER_T	mdio_oper;

		mdio_oper.wrd = 0;
		mdio_oper.bf.reg_off = reg;
		mdio_oper.bf.phy_addr = addr;
		mdio_oper.bf.reg_base = CA_NI_MDIO_REG_BASE;
		CA_REG_WRITE(value, mdio_oper.wrd);
		return 0;
	}

	addr &= 0x1f;

	if (devad == MDIO_DEVAD_NONE) {
		///--if (addr == 8)
		///--	printf("##########$$$$$ C22 WRITE PHYADDR=%u, OFFSET=%u, DATA=%u\n", addr, reg, value);
		result = ca_mdio_write(addr, 1, reg, 0, value);
	} else {
		///--if (addr == 8)
		///--	printf("##########$$$$$ C45 WRITE PHYADDR=%u, OFFSET=%u, WADDR=%u, DATA=%u\n", addr, devad, reg, value);
		result = ca_mdio_write(addr, 0, devad, reg, value);
	}
	if (result != CA_E_OK) {
		printk("%s-%d: [ERROR] MDIO set FAIL ! (addr=%u, devad=%u, reg=%u, value=%u)", __func__, __LINE__, addr, devad, reg, value);
		return -1;
	} else {
		return result;
	}
}

int rtk_ne_miiphy_read(struct mii_dev *bus, int addr, int devad, int reg)
{
	u16 value;
	int result;

	if (is_internal_phy(addr)) {
		NI_MDIO_OPER_T	mdio_oper;

		mdio_oper.wrd = 0;
		mdio_oper.bf.reg_off = reg;
		mdio_oper.bf.phy_addr = addr;
		mdio_oper.bf.reg_base = CA_NI_MDIO_REG_BASE;
		return CA_REG_READ(mdio_oper.wrd);
	}

	addr &= 0x1f;

	if (devad == MDIO_DEVAD_NONE) {
		result = ca_mdio_read(addr, 1, reg, 0, &value);
		///--if (addr == 8)
		///--	printf("##########$$$$$ C22 READ PHYADDR=%u, OFFSET=%u, DATA=%u\n", addr, reg, value);
	} else {
		result = ca_mdio_read(addr, 0, devad, reg, &value);
		///--if (addr == 8)
		///--	printf("##########$$$$$ C45 READ PHYADDR=%u, OFFSET=%u, WADDR=%u, DATA=%u\n", addr, devad, reg, value);
	}

	if (result != CA_E_OK)
	{
		printk("%s-%d: [ERROR] MDIO get FAIL ! (addr=%u, devad=%u, reg=%u)", __func__, __LINE__, addr, devad, reg);
		return -1;
	}

	return value;
}

