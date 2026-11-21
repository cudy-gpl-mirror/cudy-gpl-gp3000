#include <common.h>
#include <command.h>
#include <miiphy.h>
#include <phy.h>
#include <osal.h>
#include <nic.h>
#include <net.h>
#include <rtk_mdio.h>
#include <realtek_ne.h>
#ifdef REALTEK_NE_DBG
#include <aal_ni_l2.h>
#endif

#ifdef CONFIG_REALTEK_10G_PON
u8 eth_initialize_flag = 0;
static int rtk_ne_initialize_done = 0;
#endif

#ifdef CONFIG_RTK_NE_SUPPORT_HSGMII_P7
#include <aal_pon.h>

static u8 spd_to_pon_mode[6] = { ONU_PON_MAC_ETHERNET_SGMII_10M,
		ONU_PON_MAC_ETHERNET_SGMII_100M, ONU_PON_MAC_ETHERNET_SGMII_1G,
		ONU_PON_MAC_ETHERNET_SGMII_500M, ONU_PON_MAC_ETHERNET_10G,
		ONU_PON_MAC_ETHERNET_SGMII_2G5 };

static u8 port7_8221B_old_phy_speed = 4;
#endif

#if defined(CONFIG_TARGET_RTK_TAURUS)
#if 1
struct lan_port_table_entry lan_port_table_default[MAX_LAN_PORT_NUM] = {
	//dport, phyAddr, serdesMode
	{0, 1, NE_SERDES_MODE_NONE},
	{1, 2, NE_SERDES_MODE_NONE},
	{2, 3, NE_SERDES_MODE_NONE},
	{3, 4, NE_SERDES_MODE_NONE},
#ifdef CONFIG_RTK_NI_SUPPORT_QXGMII_P6
	{6, 8, NE_SERDES_MODE_QXGMII},
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_USXGMII_P6
	{6, 8, NE_SERDES_MODE_USXGMII},
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_XFI_P6
	{6, 8, NE_SERDES_MODE_XFI},
#endif
	{4, 7, NE_SERDES_MODE_HSGMII},
	{-1, -1, NE_SERDES_MODE_NONE},
	{-1, -1, NE_SERDES_MODE_NONE},
	{-1, -1, NE_SERDES_MODE_NONE},
	{-1, -1, NE_SERDES_MODE_NONE},
};
#else
struct lan_port_table_entry lan_port_table_default[MAX_LAN_PORT_NUM] = {
	//dport, phyAddr, serdesMode
	{0, 5, NE_SERDES_MODE_QXGMII},
	{1, 6, NE_SERDES_MODE_QXGMII},
	{2, 7, NE_SERDES_MODE_QXGMII},
	{3, 8, NE_SERDES_MODE_QXGMII},
	{-1, -1, NE_SERDES_MODE_NONE},
	{-1, -1, NE_SERDES_MODE_NONE},
	{-1, -1, NE_SERDES_MODE_NONE},
	{-1, -1, NE_SERDES_MODE_NONE},
	{-1, -1, NE_SERDES_MODE_NONE},
	{-1, -1, NE_SERDES_MODE_NONE},
};
#endif
#elif defined(CONFIG_TARGET_RTK_ELNATH)
struct lan_port_table_entry lan_port_table_default[MAX_LAN_PORT_NUM] = {
	//dport, phyAddr, serdesMode
	{0, 1, NE_SERDES_MODE_NONE},
	{1, 2, NE_SERDES_MODE_NONE},
	{2, 3, NE_SERDES_MODE_NONE},
	{3, 4, NE_SERDES_MODE_NONE},
#if defined(CONFIG_RTK_NE_SUPPORT_HSGMII_P7) && defined(CONFIG_RTK_NI_SUPPORT_RTL8221B)
	{7, PORT7_8221B_PHY_ID, NE_SERDES_MODE_HSGMII},
#else
	{-1, -1, NE_SERDES_MODE_NONE},
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_HSGMII_P6
	{6, 7, NE_SERDES_MODE_HSGMII},
#else
	{-1, -1, NE_SERDES_MODE_NONE},
#endif
	{-1, -1, NE_SERDES_MODE_NONE},
	{-1, -1, NE_SERDES_MODE_NONE},
	{-1, -1, NE_SERDES_MODE_NONE},
	{-1, -1, NE_SERDES_MODE_NONE},
};
#endif

#ifdef CONFIG_MULTICAST_UPGRADE //realtek, rtk
void ni_enter_promiscuous_mode(void)
{
	/* empty */
}
#endif

uint32 rtk_ne_max_lan_port_num_get(void)
{
	return MAX_LAN_PORT_NUM;
}

static uint32 rtk_ne_max_lut_num_get(void)
{
	return MAX_LUT_ENTRY_NUM;
}

static uint32 rtk_ne_lut_init(struct eth_device *dev)
{
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);

	memset(&(priv->lut_table), 0, sizeof(priv->lut_table));
	return 0;
}

static int32 rtk_ne_lut_find(struct eth_device *dev, uint8 *mac_p)
{
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);
	uint32 totalLut = rtk_ne_max_lut_num_get();
	uint32 i;
	int32 ret = -1;

	// my entry
	for (i = 0; i < totalLut ; i++)
	{
		if (priv->lut_table[i].valid == 0)
			continue;

		if (!memcmp(mac_p, &(priv->lut_table[i].mac[0]), ARP_HLEN))
			return i;
	}

	return ret;
}

static int32 rtk_ne_lut_get(struct eth_device *dev, uint8 *mac_p)
{
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);
	uint32 totalLut = rtk_ne_max_lut_num_get();
	unsigned long oldestUpdateTime = 0xffffffff;
	int32 oldestUpdateTimeIndex = -1;
	uint32 i;
	int32 ret = -1;

	// my entry
	for (i = 0; i < totalLut ; i++)
	{
		if (priv->lut_table[i].valid == 0)
			continue;

		if (!memcmp(mac_p, &(priv->lut_table[i].mac[0]), ARP_HLEN))
			return i;
	}

	// free entry
	if (ret == -1)
	{
		for (i = 0; i < totalLut ; i++)
		{
			if (priv->lut_table[i].valid == 1)
				continue;

			return i;
		}
	}

	// lru entry
	if (ret == -1)
	{
		for (i = 0; i < totalLut ; i++) {
			if (priv->lut_table[i].updateTime < oldestUpdateTime) {
				oldestUpdateTime = priv->lut_table[i].updateTime;
				oldestUpdateTimeIndex = i;
			}
		}

		if (oldestUpdateTimeIndex != -1) {
			return oldestUpdateTimeIndex;
		}
	}

	return ret;
}

static int32 rtk_ne_lut_update(struct eth_device *dev, uint8 *mac_p, uint32 sport)
{
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);
	int32 lutIndex;

	if (!mac_p)
		return -1;

	if ((lutIndex = rtk_ne_lut_get(dev, mac_p)) != -1)
	{
		memcpy(&(priv->lut_table[lutIndex].mac[0]), mac_p, ARP_HLEN);
		priv->lut_table[lutIndex].updateTime = get_timer(0);
		priv->lut_table[lutIndex].sport = sport;
		priv->lut_table[lutIndex].valid = 1;
	}
	else
	{
		printf("get LUT entry FAIL !\n");
	}
	return 0;
}

static int rtk_ne_dev_priv_init(struct realtek_ne_priv *priv)
{
	memcpy(&(priv->lan_port_table), &lan_port_table_default, sizeof(priv->lan_port_table));

	return 0;
}

static phy_interface_t rtk_ne_phy_interface_trans(RTK_NE_SERDES_MODE_T serdesMode)
{
	phy_interface_t interfaceMode;

	switch(serdesMode)
	{
		case NE_SERDES_MODE_NONE:
			interfaceMode = PHY_INTERFACE_MODE_INTERNAL;
			break;
		case NE_SERDES_MODE_HSGMII:
		case NE_SERDES_MODE_SGMII:
			interfaceMode = PHY_INTERFACE_MODE_SGMII;
			break;
		case NE_SERDES_MODE_USXGMII:
			interfaceMode = PHY_INTERFACE_MODE_USXGMII;
			break;
		case NE_SERDES_MODE_RXAUI:
			interfaceMode = PHY_INTERFACE_MODE_RXAUI;
			break;
		case NE_SERDES_MODE_RGMII:
			interfaceMode = PHY_INTERFACE_MODE_RGMII;
			break;
		case NE_SERDES_MODE_QXGMII:
			interfaceMode = PHY_INTERFACE_MODE_QSGMII;
			break;
		case NE_SERDES_MODE_FIBER:
		case NE_SERDES_MODE_XFI:
		default:
			interfaceMode = PHY_INTERFACE_MODE_NA;
			break;
	}

	return interfaceMode;
}

static int rtk_ne_dev_init(struct eth_device *dev, struct bd_info *bd)
{
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);
	uint32 totalLanPort = rtk_ne_max_lan_port_num_get();
	NI_HV_PT_PORT_STATIC_CFG_t	port_static_cfg;
	NI_HV_PT_PORT_GLB_CFG_t	port_glb_cfg;
	struct phy_device *phydev;
	struct phy_driver *drv;
	u16 phy_reg_value, phy_link, lpagb, lpa, phy_speed, phy_duplex, speed, duplex;
	char *spd, *dup;
	u16 vendor_id, chip_id;
	u32 phy_id;
	uint32 i, val;

#ifdef CONFIG_REALTEK_10G_PON
	if(rtk_ne_initialize_done == 0)
		return 0;
#endif

	for (i = 0; i < totalLanPort; i++)
	{
		if (priv->lan_port_table[i].dport == -1)
			continue;

		if (priv->lan_port_table[i].is_phyinited) {
			vendor_id = (u16) phy_read(priv->lan_port_table[i].phydev, MDIO_DEVAD_NONE, MII_PHYSID1);
			chip_id = (u16) phy_read(priv->lan_port_table[i].phydev, MDIO_DEVAD_NONE, MII_PHYSID2);
			phy_id = (vendor_id << 16) | chip_id;

			/* phy_link 0: DOWN, 1: UP */
			/* phy_speed 0: 10Mbps, 1: 100Mbps, 2: 1000Mbps, 3: 500Mbps, 4: 10Gbps, 5: 2.5Gbps */
			/* duplex 0: half duplex, 1: full duplex */
			/* read PHY link twice for refresh last PHY link status */
			phy_read(priv->lan_port_table[i].phydev, MDIO_MMD_PMAPMD, 0x1);
			phy_reg_value = (u16) phy_read(priv->lan_port_table[i].phydev, MDIO_MMD_PMAPMD, 0x1);
			phy_link = (phy_reg_value & (1 << 2)) ? 1 : 0;
			if (priv->lan_port_table[i].serdesMode == NE_SERDES_MODE_USXGMII 
				|| priv->lan_port_table[i].serdesMode == NE_SERDES_MODE_HSGMII) 
			{
				phy_reg_value = (u16) phy_read(priv->lan_port_table[i].phydev, MDIO_MMD_VEND2, 0xA434);
				phy_speed = ((phy_reg_value & (0x3 << 9)) >> (9 - 2)) | ((phy_reg_value & (0x3 << 4)) >> 4);
				duplex = (phy_reg_value & 0x8)? 1 : 0;
			} else {
				lpagb = (u16) phy_read(priv->lan_port_table[i].phydev, MDIO_DEVAD_NONE, MII_STAT1000);
				phy_reg_value = (u16) phy_read(priv->lan_port_table[i].phydev, MDIO_DEVAD_NONE, MII_CTRL1000);
				lpagb &= phy_reg_value << 2;

				lpa = (u16) phy_read(priv->lan_port_table[i].phydev, MDIO_DEVAD_NONE, MII_LPA);
				phy_reg_value = (u16) phy_read(priv->lan_port_table[i].phydev, MDIO_DEVAD_NONE, MII_ADVERTISE);
				lpa &= phy_reg_value;

				phy_speed = 0;
				phy_duplex = 0;
				if (lpagb & (3 << 10)) {  	/* 1000Mbps */
					phy_speed = 2;
					if (lpagb & (1 << 11))  /* 1000Mbps full */
						duplex = 1;
				} else if (lpa & (3 << 7)) {  	/* 100Mbps */
					phy_speed = 1;
					if (lpa & (1 << 8))  	/* 100Mbps full */
						phy_duplex = 1;
				} else if (lpa & (1 << 6)) { 	/* 10Mbps full */
					phy_duplex = 1;
				}
			}

			switch (phy_speed) {
				default:
				case 0:
					spd = "10Mbps";
					break;
				case 1:
					spd = "100Mbps";
					break;
				case 2:
					spd = "1000Mbps";
					break;
				case 3:
					spd = "500Mbps";
					break;
				case 4:
					spd = "10Gbps";
					break;
				case 5:
					spd = "2.5Gbps";
					break;
			}

			if (duplex == 1)
				dup = "full duplex";
			else
				dup = "half duplex";

			if (!phy_link)
				continue;

			phydev = priv->lan_port_table[i].phydev;
			printf("PORT %u PHYADDR %u PHYID 0x%08X %s %s\n", priv->lan_port_table[i].dport, phydev->addr, phy_id, spd, dup);

			switch (phy_id & PHY_ID_MASK) {
				case PHY_ID_RTL8221B:
				case PHY_ID_RTL8261:
				case PHY_ID_RTL8224:
					port_static_cfg.wrd = CA_REG_READ((NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * priv->lan_port_table[i].dport));
					port_static_cfg.bf.int_cfg = GE_MAC_INTF_GMII;	 /* GMII */
					CA_REG_WRITE(port_static_cfg.wrd, (NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * priv->lan_port_table[i].dport));
					break;
				case PHY_ID_RTL8214:
					port_static_cfg.wrd = CA_REG_READ((NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * priv->lan_port_table[i].dport));
					port_static_cfg.bf.int_cfg = GE_MAC_INTF_QSGMII_1000;	/* QSGMII_GE */
					CA_REG_WRITE(port_static_cfg.wrd, (NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * priv->lan_port_table[i].dport));
					break;
				case PHY_ID_RTL8211:
				case PHY_ID_RTL8211_G3_ASIC:
				case PHY_ID_RTL8201:
				case PHY_ID_RTL8211_SATURN_ASIC:
				default:
					/*
					 * Configuration for Management Ethernet
					 * Interface:
					 * - RGMII 1000 mode or RGMII 100 mode
					 * - MAC mode
					 */
					port_static_cfg.wrd = CA_REG_READ((NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * priv->lan_port_table[i].dport));
#if NE_FPGA
					port_static_cfg.bf.int_cfg = GE_MAC_INTF_GMII;
#else
					if (phy_speed == 2 /* 1000Mbps */) {
						/* port 4 connects to RGMII PHY */
						if (phydev->addr == 5)
							port_static_cfg.bf.int_cfg = GE_MAC_INTF_RGMII_1000;
						else
							port_static_cfg.bf.int_cfg = GE_MAC_INTF_GMII;
					}
					else {
						/* port 4 connects to RGMII PHY */
						if (phydev->addr == 5) {
							port_static_cfg.bf.int_cfg = GE_MAC_INTF_RGMII_100;
						}
						else {
							port_static_cfg.bf.int_cfg = GE_MAC_INTF_MII;
						}
					}
#endif
					CA_REG_WRITE(port_static_cfg.wrd, (NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * priv->lan_port_table[i].dport));
					break;
			}

#ifdef CONFIG_RTK_NE_SUPPORT_HSGMII_P7
			#ifdef CONFIG_RTK_NI_SUPPORT_RTL8221B
			if (priv->is_hsgmii_need && (priv->lan_port_table[i].serdesMode == NE_SERDES_MODE_HSGMII)
				&& (priv->lan_port_table[i].dport == NE_PORT_7) && (phy_speed <= 5)
				&& (phy_speed != port7_8221B_old_phy_speed))
			{
				ca_pon_init(0, spd_to_pon_mode[phy_speed]);
				rtk_sds_polarity_setting(SDS_P7_INV_HSO);

				// refer to ca_ni.c aal_driver_init()
				CA_REG_WRITE(0x7, L2FE_ARB_PDPID_MAP_TBL_DATA);
				CA_REG_WRITE(0xC0000007, L2FE_ARB_PDPID_MAP_TBL_ACCESS);

				port7_8221B_old_phy_speed = phy_speed;
			}
			#endif
#endif

#ifdef CONFIG_RTK_NI_SUPPORT_HSGMII_P4
			if (priv->is_hsgmii_need && priv->lan_port_table[i].serdesMode == NE_SERDES_MODE_HSGMII)
			{
				printf("Set Port 4 sds speed as %s\n", spd);
				ca_ni_p4_hsgmii_speed_set(phy_speed);
			}
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_QXGMII_P6
			if (priv->is_qxgmii_need && priv->lan_port_table[i].serdesMode == NE_SERDES_MODE_QXGMII)
			{
				printf("Set Port 6 sds speed as %s\n", spd);
				ca_ni_p6_qxgmii_port_macForceAbility_set(priv->lan_port_table[i].dport, phy_speed);
			}
			else
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_USXGMII_P6
			if (priv->is_usxgmii_need && priv->lan_port_table[i].serdesMode == NE_SERDES_MODE_USXGMII)
			{
				printf("Set Port 6 sds speed as %s\n", spd);
				ca_ni_p6_usxgmii_speed_set(phy_speed);
			}
			else
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_HSGMII_P6
			if (priv->is_hsgmii_need && priv->lan_port_table[i].serdesMode == NE_SERDES_MODE_HSGMII)
			{
				printf("Set Port 6 sds speed as %s\n", spd);
				ca_ni_p6_hsgmii_speed_set(phy_speed);
			}
			else
#endif
			{
				port_glb_cfg.wrd = CA_REG_READ((NI_HV_PT_PORT_GLB_CFG + APB0_NI_HV_PT_STRIDE * priv->lan_port_table[i].dport));
				if (phy_speed == 0)  /* 10Mbps */
					speed = 1;
				else
					speed = 0;
				if (phy_duplex == 0)  /* half duplex */
					duplex = 1;
				else
					duplex = 0;
				port_glb_cfg.bf.speed = speed;
				port_glb_cfg.bf.duplex = duplex;
				CA_REG_WRITE(port_glb_cfg.wrd, (NI_HV_PT_PORT_GLB_CFG + APB0_NI_HV_PT_STRIDE * priv->lan_port_table[i].dport));
			}

#ifdef CONFIG_RTK_NI_SUPPORT_RTL8261I
			if (priv->is_8261i_detect)
			{
				phydev = priv->lan_port_table[i].phydev;
				drv = phydev->drv;
				if ((drv->uid&drv->mask) == (PHY_ID_RTL8261&PHY_ID_MASK)) {
					rtk_ne_rtl8261_phy_sds_rx_reset(priv->lan_port_table[i].phydev);
				}
			}
#endif
		}
	}

	return 0;
}

static void rtk_ne_dev_halt(struct eth_device *netdev)
{
#ifdef CONFIG_REALTEK_10G_PON
	if(rtk_ne_initialize_done == 0)
		return;
#endif

	return;
}

static int rtk_ne_dev_send(struct eth_device *dev, void *packet, int length)
{
	uint32 totalLanPort = rtk_ne_max_lan_port_num_get();
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);
	uint8 pkt_buf[RTK_NI_DEFAULT_PKT_BUF_LEN] = {0};
	uint32 pkt_len = 0;
	uint32 ret, i;
	int32 lutIndex;

#ifdef CONFIG_REALTEK_10G_PON
	if(rtk_ne_initialize_done == 0)
		return 0;
#endif

	if ((NULL == packet) || (length > 2032))
		return -1;

	pkt_len = length;
	memcpy(&(pkt_buf[0]), (const void *)packet, length);
	if (is_broadcast_ethaddr(&pkt_buf[0]) || is_multicast_ethaddr(&pkt_buf[0]))
	{
		for (i = 0; i < totalLanPort; i++) {

			if (priv->lan_port_table[i].dport == -1)
				continue;

			if (is_broadcast_ethaddr(&pkt_buf[0]))
				priv->sw_cnt.tx_bc_pkts++;
			else if (is_multicast_ethaddr(&pkt_buf[0]))
				priv->sw_cnt.tx_mc_pkts++;

			ret |= rtk_ni_nic_tx(&pkt_buf[0], &pkt_len, priv->lan_port_table[i].dport);
		}
	}
	else if((lutIndex = rtk_ne_lut_find(dev, (uint8 *)(&packet[0]))) != -1)
	{
		priv->sw_cnt.tx_uc_pkts++;
		ret = rtk_ni_nic_tx(&pkt_buf[0], &pkt_len, priv->lut_table[lutIndex].sport);
	}
	else
	{
		priv->sw_cnt.tx_lookup_miss_pkts++;
		ret = rtk_ni_nic_tx(&pkt_buf[0], &pkt_len, PE_NI_HAL_TX_TEST_DEFAULT_LDPID);
	}

	if (ret == RE_OK)
		return 0;
	else
		return -1;
}

static int rtk_ne_dev_recv(struct eth_device *netdev)
{
	struct eth_device *dev = eth_get_dev_by_name(CONFIG_IDENT_STRING);
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);
	uint8 *data_ptr;
	uint32 pkt_len = 0;
	uint32 ret;
	uint32 sport = 0;

#ifdef CONFIG_REALTEK_10G_PON
	if(rtk_ne_initialize_done == 0)
		return 0;
#endif

	data_ptr = (uint8 *)net_rx_packets[0];
	pkt_len = min(RTK_NI_DEFAULT_PKT_BUF_LEN, (PKTSIZE_ALIGN+PKTALIGN-1));
	ret = rtk_ni_nic_rx(data_ptr, &pkt_len, &sport);
	if (ret == RE_OK) {
		rtk_ne_lut_update(netdev, &data_ptr[6], sport);
	
		PRINTF_DEBUG("ret=%d, pkt_len=%d, %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"
			, ret, pkt_len, data_ptr[0], data_ptr[1], data_ptr[2], data_ptr[3], data_ptr[4], data_ptr[5]
			, data_ptr[6], data_ptr[7], data_ptr[8], data_ptr[9], data_ptr[10], data_ptr[11]
			, data_ptr[12], data_ptr[13]);

		if (is_broadcast_ethaddr(data_ptr))
			priv->sw_cnt.rx_bc_pkts++;
		else if (is_multicast_ethaddr(data_ptr))
			priv->sw_cnt.rx_mc_pkts++;
		else
			priv->sw_cnt.rx_uc_pkts++;

		///--dump_pkt_content(data_ptr, pkt_len);
		net_process_received_packet(net_rx_packets[0], pkt_len);
	}

	return 0;
}

static void rtk_ne_setup_mac_addr(struct eth_device *dev)
{
	unsigned char mac[ARP_HLEN];
	if (eth_env_get_enetaddr("ethaddr", mac)) {
		memcpy(&(dev->enetaddr[0]), &mac[0], sizeof(dev->enetaddr));
	}
}

static int rtk_ne_internal_phy_config(struct phy_device *phydev)
{
	struct eth_device *dev = eth_get_dev_by_name(CONFIG_IDENT_STRING);
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);

	printf("Config internal PHY .. \n");
	if (!priv->is_intphy_global_config_done)
	{
		rtk_phy_global_config();
		priv->is_intphy_global_config_done = 1;
	}
	rtk_phy_config(phydev);
	///--genphy_config() will restart AN, do not do that
	///--genphy_config(phydev);
	return 0;
}

static int rtk_ne_internal_phy_probe(struct phy_device *phydev)
{
	/* disable reset behavior */
	phydev->flags = PHY_FLAG_BROKEN_RESET;
	return 0;
}

static struct phy_driver phy_internal = {
	.name = "Realtek PHY Internal",
	.uid = PHY_ID_INTERNAL,
	.mask = PHY_ID_MASK,
	.features = PHY_GBIT_FEATURES,
	.mmds = (MDIO_MMD_PMAPMD | MDIO_MMD_PCS |
		 MDIO_MMD_PHYXS | MDIO_MMD_AN |
		 MDIO_MMD_VEND1),
	.config = rtk_ne_internal_phy_config,
	.startup = genphy_startup,
	.shutdown = genphy_shutdown,
	.probe = rtk_ne_internal_phy_probe,
};

#ifdef CONFIG_RTK_NI_SUPPORT_RTL8221B
static int rtk_ne_rtl8221b_phy_config(struct phy_device *phydev)
{
	struct eth_device *dev = eth_get_dev_by_name(CONFIG_IDENT_STRING);
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);

	printf("Config RTL8221B external PHY .. \n");
	priv->is_8221b_detect = 1;
#if defined(CONFIG_RTK_NI_SUPPORT_HSGMII_P4) || defined(CONFIG_RTK_NI_SUPPORT_HSGMII_P6) || defined(CONFIG_RTK_NE_SUPPORT_HSGMII_P7)
	priv->is_hsgmii_need = 1;
#endif
	if (!priv->is_8221b_global_config_done)
	{
#ifndef CONFIG_RTK_NE_SUPPORT_HSGMII_P7
#ifdef CONFIG_RTK_NI_SUPPORT_HSGMII_P4
		ca_ni_p4_hsgmii_init();
		ca_ni_rtl8221_phy_init(phydev->addr);
		mdelay(3500);
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_HSGMII_P6
		ca_ni_p6_hsgmii_init();
		ca_ni_rtl8221_phy_init(phydev->addr);
		mdelay(3500);
#endif
#endif
		priv->is_8221b_global_config_done = 1;
	}
	///--genphy_config() will restart AN, do not do that
	///--genphy_config(phydev);
}

static int rtk_ne_rtl8221b_phy_probe(struct phy_device *phydev)
{
	/* disable reset behavior */
	phydev->flags = PHY_FLAG_BROKEN_RESET;
	return 0;
}

static struct phy_driver phy_rtl8221b = {
	.name = "Realtek PHY RTL8221B",
	.uid = PHY_ID_RTL8221B,
	.mask = PHY_ID_MASK,
	.features = PHY_GBIT_FEATURES,
	.mmds = (MDIO_MMD_PMAPMD | MDIO_MMD_PCS |
		 MDIO_MMD_PHYXS | MDIO_MMD_AN |
		 MDIO_MMD_VEND1 | MDIO_MMD_C22EXT),
	.config = rtk_ne_rtl8221b_phy_config,
	.startup = genphy_startup,
	.shutdown = genphy_shutdown,
	.probe = rtk_ne_rtl8221b_phy_probe,
};
#endif

#ifdef CONFIG_RTK_NI_SUPPORT_RTL8224B
static int rtk_ne_rtl8224_phy_config(struct phy_device *phydev)
{
	struct eth_device *dev = eth_get_dev_by_name(CONFIG_IDENT_STRING);
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);

	printf("Config RTL8224 external PHY .. \n");
	priv->is_8224b_detect = 1;
	priv->is_qxgmii_need = 1;
	if (!priv->is_8224b_global_config_done)
	{
		rtl8224_config(phydev->addr);
		ca_ni_p6_qxgmii_init();
		ca_ni_rtl8224_phy_init(phydev->addr);
		priv->is_8224b_global_config_done = 1;
	}
	///--genphy_config() will restart AN, do not do that
	///--genphy_config(phydev);
}

static int rtk_ne_rtl8224_phy_probe(struct phy_device *phydev)
{
	/* disable reset behavior */
	phydev->flags = PHY_FLAG_BROKEN_RESET;
	return 0;
}

static struct phy_driver phy_rtl8224 = {
	.name = "Realtek PHY RTL8224",
	.uid = PHY_ID_RTL8224,
	.mask = PHY_ID_MASK,
	.features = PHY_GBIT_FEATURES,
	.mmds = (MDIO_MMD_PMAPMD | MDIO_MMD_PCS |
		 MDIO_MMD_PHYXS | MDIO_MMD_AN |
		 MDIO_MMD_VEND1 | MDIO_MMD_C22EXT),
	.config = rtk_ne_rtl8224_phy_config,
	.startup = genphy_startup,
	.shutdown = genphy_shutdown,
	.probe = rtk_ne_rtl8224_phy_probe,
};
#endif

#ifdef CONFIG_RTK_NI_SUPPORT_RTL8261I
static int rtk_ne_rtl8261i_phy_config(struct phy_device *phydev)
{
	struct eth_device *dev = eth_get_dev_by_name(CONFIG_IDENT_STRING);
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);

	printf("Config RTL8261I external PHY .. \n");
	priv->is_8261i_detect = 1;
	priv->is_usxgmii_need = 1;
	
	if (!priv->is_8261i_global_config_done)
	{
		ca_ni_p6_usxgmii_init();
		priv->is_8261i_global_config_done = 1;
	}
#if 0 ///--For reduce mem usage
	rtl8261_config(phydev);
#endif
	rtk_ne_rtl8261_phy_init(phydev, 0);
	///--genphy_config() will restart AN, do not do that
	///--genphy_config(phydev);
}

static int rtk_ne_rtl8261i_phy_probe(struct phy_device *phydev)
{
	/* disable reset behavior */
	phydev->flags = PHY_FLAG_BROKEN_RESET;
	return 0;
}

static struct phy_driver phy_rtl8261i = {
	.name = "Realtek PHY RTL8261",
	.uid = PHY_ID_RTL8261,
	.mask = PHY_ID_MASK,
	.features = PHY_GBIT_FEATURES,
	.mmds = (MDIO_MMD_PMAPMD | MDIO_MMD_PCS |
		 MDIO_MMD_PHYXS | MDIO_MMD_AN |
		 MDIO_MMD_VEND1 | MDIO_MMD_VEND1 |
		 MDIO_MMD_C22EXT),
	.config = rtk_ne_rtl8261i_phy_config,
	.startup = genphy_startup,
	.shutdown = genphy_shutdown,
	.probe = rtk_ne_rtl8261i_phy_probe,
};
#endif

int rtk_ne_phy_config(struct eth_device *dev)
{
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);
	uint32 totalLanPort = rtk_ne_max_lan_port_num_get();
	struct phy_device *phydev;
	uint32 i, val;

	for (i = 0; i < totalLanPort; i++)
	{
		if (priv->lan_port_table[i].dport == -1)
			continue;

		if (!priv->lan_port_table[i].is_phyinited) {
			phydev = phy_connect(priv->bus_int, priv->lan_port_table[i].phyAddr, dev, rtk_ne_phy_interface_trans(priv->lan_port_table[i].serdesMode));
		
			if (phydev) {				
				priv->lan_port_table[i].phydev = phydev;
				printf("found %s(%x)@phyaddr %d\n", phydev->drv->name, phydev->phy_id,
					   priv->lan_port_table[i].phyAddr);
				phy_config(phydev);

				val = phy_read(priv->lan_port_table[i].phydev, MDIO_DEVAD_NONE, MII_BMCR);
				if (val & BMCR_PDOWN) {
					val &= ~BMCR_PDOWN;
					phy_write(priv->lan_port_table[i].phydev, MDIO_DEVAD_NONE, MII_BMCR, val);
				}
				priv->lan_port_table[i].is_phyinited = 1;
			}
		}
	}
	return 0;
}

void rtk_ne_ni_reset(struct eth_device *dev)
{
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);
	uint32 totalLanPort = rtk_ne_max_lan_port_num_get();
	NI_HV_GLB_INIT_DONE_t init_done;
	NI_HV_GLB_INTF_RST_CONFIG_t  intf_rst_config;
	NI_HV_GLB_STATIC_CFG_t static_cfg;
	GLOBAL_BLOCK_RESET_t glb_blk_reset;
#ifdef CONFIG_TARGET_RTK_TAURUS
	NI_HV_XGE_MAC_CFG_COM_t cfg_com;
#endif
	uint32 i;

	/* NI global resets */
	glb_blk_reset.wrd = CA_REG_READ(GLOBAL_BLOCK_RESET);
	glb_blk_reset.bf.reset_ni = 1;
	CA_REG_WRITE(glb_blk_reset.wrd, GLOBAL_BLOCK_RESET);
	/* Remove resets */
	glb_blk_reset.bf.reset_ni = 0;
	CA_REG_WRITE(glb_blk_reset.wrd, GLOBAL_BLOCK_RESET);

#ifdef CONFIG_TARGET_RTK_TAURUS
	// set XE port 6 mac enable to 0
	cfg_com.wrd = CA_REG_READ(NI_HV_XGE_MAC_CFG_COM);
	cfg_com.bf.rx_rst = 1;
	cfg_com.bf.tx_rst = 1;
	CA_REG_WRITE(cfg_com.wrd, NI_HV_XGE_MAC_CFG_COM);
#endif

	/* check the ready bit of NI module */
	for (i = 0; i < NE_READ_POLL_COUNT; i++) {
		init_done.wrd = CA_REG_READ(NI_HV_GLB_INIT_DONE);
		if (init_done.bf.ni_init_done)
			break;
	}
	if (i == NE_READ_POLL_COUNT)
		printf("%s: NI init done not ready, init_done.wrd=0x%x!!!\n", __func__, init_done.wrd);

	// Reset all
	intf_rst_config.wrd = 0;
	CA_REG_WRITE(intf_rst_config.wrd, NI_HV_GLB_INTF_RST_CONFIG);


	/* Only one GMAC can connect to CPU */
	static_cfg.wrd = CA_REG_READ(NI_HV_GLB_STATIC_CFG);
#if defined(CONFIG_RTK_NI_SUPPORT_QXGMII_P6) && !defined(CONFIG_TARGET_RTK_MERCURY)
	for (i = 0; i < totalLanPort; i++)
	{
		if (priv->lan_port_table[i].dport == -1)
			continue;

		if (!priv->is_qxgmii_need)
			break;

		if (priv->lan_port_table[i].serdesMode == NE_SERDES_MODE_QXGMII) {
			if (priv->lan_port_table[i].dport == NE_PORT_0)
				static_cfg.bf.port0_qxgmii_sel = 1;
			if (priv->lan_port_table[i].dport == NE_PORT_1)
				static_cfg.bf.port1_qxgmii_sel = 1;
			if (priv->lan_port_table[i].dport == NE_PORT_2)
				static_cfg.bf.port2_qxgmii_sel = 1;
			if (priv->lan_port_table[i].dport == NE_PORT_3)
				static_cfg.bf.port3_qxgmii_sel = 1;
		}
	}
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_XFI_P6
	for (i = 0; i < totalLanPort; i++)
	{
		if (priv->lan_port_table[i].dport == -1)
			continue;

		if (priv->lan_port_table[i].serdesMode == NE_SERDES_MODE_XFI)
		{
			ca_ni_p6_xfi_init();
		}
	}
#endif

	CA_REG_WRITE(static_cfg.wrd, NI_HV_GLB_STATIC_CFG);
}

int rtk_ni_nic_init_rest(struct eth_device *dev)
{
	rtk_ne_ni_reset(dev);
	return 0;
}

void rtk_ne_gphy_reset(void)
{
#if defined(CONFIG_TARGET_RTK_MERCURY)
	GLOBAL_GPIO_MUX_0_t global_gpio_mux_0;
	PER_GPIO0_CFG_t per_gpio0_cfg;
	PER_GPIO0_OUT_t per_gpio0_out;

	// PHY reset GPIO: Mercury GPIO0[1]:
#ifdef CONFIG_RTK_NI_SUPPORT_RTL8261I
	global_gpio_mux_0.wrd = CA_REG_READ(GLOBAL_GPIO_MUX_0);
	global_gpio_mux_0.bf.gpio0en |= 0x2;
	CA_REG_WRITE(global_gpio_mux_0.wrd, GLOBAL_GPIO_MUX_0);
	
	per_gpio0_cfg.wrd = CA_REG_READ(PER_GPIO0_CFG);
	per_gpio0_cfg.bf.iopmod &= ~(0x2);
	CA_REG_WRITE(per_gpio0_cfg.wrd, PER_GPIO0_CFG);

	per_gpio0_out.wrd = CA_REG_READ(PER_GPIO0_OUT);
	per_gpio0_out.bf.dataout |= 0x2;
	CA_REG_WRITE(per_gpio0_out.wrd, PER_GPIO0_OUT);
#endif

	// PHY reset GPIO: Mercury GPIO0[2]:
#ifdef CONFIG_RTK_NI_SUPPORT_RTL8224B
	global_gpio_mux_0.wrd = CA_REG_READ(GLOBAL_GPIO_MUX_0);
	global_gpio_mux_0.bf.gpio0en |= 0x4;
	CA_REG_WRITE(global_gpio_mux_0.wrd, GLOBAL_GPIO_MUX_0);
	
	per_gpio0_cfg.wrd = CA_REG_READ(PER_GPIO0_CFG);
	per_gpio0_cfg.bf.iopmod &= ~(0x4);
	CA_REG_WRITE(per_gpio0_cfg.wrd, PER_GPIO0_CFG);

	per_gpio0_out.wrd = CA_REG_READ(PER_GPIO0_OUT);
	per_gpio0_out.bf.dataout |= 0x4;
	CA_REG_WRITE(per_gpio0_out.wrd, PER_GPIO0_OUT);
#endif
#elif defined(CONFIG_TARGET_RTK_ELNATH)
	GLOBAL_PIN_MUX_2_t global_pin_mux_2;
	GLOBAL_GPIO_MUX_3_t global_gpio_mux_3;
	PER_GPIO3_CFG_t per_gpio3_cfg;
	PER_GPIO3_OUT_t per_gpio3_out;

	global_pin_mux_2.wrd = CA_REG_READ(GLOBAL_PIN_MUX_2);
	global_pin_mux_2.wrd = global_pin_mux_2.wrd | 0x100000;
	CA_REG_WRITE(global_pin_mux_2.wrd, GLOBAL_PIN_MUX_2);

	// PHY reset GPIO: 9615C GPIO3[17]:
	global_gpio_mux_3.wrd = CA_REG_READ(GLOBAL_GPIO_MUX_3);
	global_gpio_mux_3.bf.gpio3en |= 0x20000;
	CA_REG_WRITE(global_gpio_mux_3.wrd, GLOBAL_GPIO_MUX_3);

	per_gpio3_cfg.wrd = CA_REG_READ(PER_GPIO3_CFG);
	per_gpio3_cfg.bf.iopmod &= ~(0x20000);
	CA_REG_WRITE(per_gpio3_cfg.wrd, PER_GPIO3_CFG);

	per_gpio3_out.wrd = CA_REG_READ(PER_GPIO3_OUT);
	per_gpio3_out.bf.dataout |= 0x20000;
	CA_REG_WRITE(per_gpio3_out.wrd, PER_GPIO3_OUT);

	mdelay(500);
#elif defined(CONFIG_TARGET_RTK_TAURUS)
	GLOBAL_GPIO_MUX_4_t global_gpio_mux_4;
	PER_GPIO4_CFG_t per_gpio4_cfg;
	PER_GPIO4_OUT_t per_gpio4_out;

	// PHY reset GPIO: 9615C GPIO4[0]:
	global_gpio_mux_4.wrd = CA_REG_READ(GLOBAL_GPIO_MUX_4);
	global_gpio_mux_4.bf.gpio4en |= 0x1;
	CA_REG_WRITE(global_gpio_mux_4.wrd, GLOBAL_GPIO_MUX_4);

	per_gpio4_cfg.wrd = CA_REG_READ(PER_GPIO4_CFG);
	per_gpio4_cfg.bf.iopmod &= ~(0x1);
	CA_REG_WRITE(per_gpio4_cfg.wrd, PER_GPIO4_CFG);

	per_gpio4_out.wrd = CA_REG_READ(PER_GPIO4_OUT);
	per_gpio4_out.bf.dataout |= 0x1;
	CA_REG_WRITE(per_gpio4_out.wrd, PER_GPIO4_OUT);
#endif
	return;
}

int rtk_eth_initialize(struct bd_info *bis)
{
	struct eth_device *dev;
	struct realtek_ne_priv *priv;
	struct mii_dev *mdiodev;
	int ret;

#ifdef CONFIG_REALTEK_10G_PON
	if(rtk_ne_initialize_done == 1)
		return 0;
#endif

	dev = (struct eth_device *) malloc(sizeof *dev);
	if (NULL == dev)
		return 1;
	memset (dev, 0, sizeof *dev);

	priv = (struct realtek_ne_priv *) malloc(sizeof (*priv));
	if (NULL == priv)
		return 0;
	memset (priv, 0, sizeof *priv);

	rtk_ne_dev_priv_init(priv);

	sprintf(dev->name, CONFIG_IDENT_STRING);

	rtk_ne_setup_mac_addr(dev);

	dev->priv = priv;
	dev->init = rtk_ne_dev_init;
	dev->halt = rtk_ne_dev_halt;
	dev->send = rtk_ne_dev_send;
	dev->recv = rtk_ne_dev_recv;

	eth_register(dev);

	mdiodev = mdio_alloc();

	if (!mdiodev)
		return -ENOMEM;

	strncpy(mdiodev->name, dev->name, MDIO_NAME_LEN);
	mdiodev->read = rtk_ne_miiphy_read;
	mdiodev->write = rtk_ne_miiphy_write;
	ret = mdio_register(mdiodev);

	if (ret < 0)
		return ret;

	priv->bus_int = mdiodev;

	rtk_ne_gphy_reset();

	phy_register(&phy_internal);
#ifdef CONFIG_RTK_NI_SUPPORT_RTL8221B
	phy_register(&phy_rtl8221b);
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_RTL8224B
	phy_register(&phy_rtl8224);
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_RTL8261I
	phy_register(&phy_rtl8261i);
#endif

	rtk_ne_phy_config(dev);

	rtk_ni_nic_init();

	rtk_ni_nic_init_rest(dev);

	rtk_ne_lut_init(dev);

#ifdef CONFIG_REALTEK_10G_PON
	rtk_ne_initialize_done = 1;
#endif

	return 0;
}

#ifdef REALTEK_NE_DBG
static int rtk_ne_port_mib_get(int port, unsigned int serdesMode, aal_ni_mib_stats_t *stats_p)
{
	if (serdesMode == NE_SERDES_MODE_USXGMII || serdesMode == NE_SERDES_MODE_XFI)
		return aal_ni_xge_mac_mib_get(0, port, 0, stats_p);
	else
		return aal_ni_eth_port_mib_get(0, port, 0, stats_p);
}

static int do_ne_debug(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	struct eth_device *dev = eth_get_dev_by_name(CONFIG_IDENT_STRING);
	struct realtek_ne_priv *priv = (struct realtek_ne_priv *)(dev->priv);
	uint32 totalLanPort = rtk_ne_max_lan_port_num_get();
	uint32 totalLut = rtk_ne_max_lut_num_get();
	struct phy_device *phydev;
	struct phy_driver *drv;
	uint32 i, j;
	int dport;
	int phyAddr;
	unsigned int serdesMode;


#ifdef CONFIG_REALTEK_10G_PON
	if(rtk_ne_initialize_done == 0)
		return 0;
#endif

	printf("Port mapping table: \n");
	for (i = 0; i < totalLanPort; i++)
	{
		if (priv->lan_port_table[i].dport == -1)
			continue;

		printf("	[%d] dport=%d, phyAddr=%d, serdesMode=%s\n", i, priv->lan_port_table[i].dport, priv->lan_port_table[i].phyAddr, RTK_NE_SERDES_MODE_STR(priv->lan_port_table[i].serdesMode));
	}

	printf("LUT table: \n");
	for (i = 0; i < totalLut ; i++)
	{
		if (priv->lut_table[i].valid == 0)
			continue;

		printf("	[%d] %02X:%02X:%02X:%02X:%02X:%02X SPA=%u TIME=%u\n", i, priv->lut_table[i].mac[0], priv->lut_table[i].mac[1], priv->lut_table[i].mac[2]
			, priv->lut_table[i].mac[3], priv->lut_table[i].mac[4], priv->lut_table[i].mac[5]
			, priv->lut_table[i].sport, priv->lut_table[i].updateTime);
	}

	printf("Software counter: \n");
	printf("	rx_uc_pkts=%llu\n", priv->sw_cnt.rx_uc_pkts);
	printf("	rx_mc_pkts=%llu\n", priv->sw_cnt.rx_mc_pkts);
	printf("	rx_bc_pkts=%llu\n", priv->sw_cnt.rx_bc_pkts);
	printf("	tx_uc_pkts=%llu\n", priv->sw_cnt.tx_uc_pkts);
	printf("	tx_mc_pkts=%llu\n", priv->sw_cnt.tx_mc_pkts);
	printf("	tx_bc_pkts=%llu\n", priv->sw_cnt.tx_bc_pkts);
	printf("	tx_lookup_miss_pkts=%llu\n", priv->sw_cnt.tx_lookup_miss_pkts);

	aal_ni_mib_stats_t stats;
	for (i = 0; i < totalLanPort; i++)
	{
		if (priv->lan_port_table[i].dport == -1)
			continue;

		memset(&stats, 0, sizeof(stats));
		if (rtk_ne_port_mib_get(priv->lan_port_table[i].dport, priv->lan_port_table[i].serdesMode, &stats) == 0)
		{
			printf("Port %u MIB:\n", priv->lan_port_table[i].dport);
			for (j = 0; j < __NE_PORT_MIB_NUM; j++)
			{
				printf("	%s = %llu\n", ne_port_mib_strings[j], *(((u64 *)(&stats)) + j));
			}
		}
	}

#ifdef CONFIG_RTK_NI_SERDES
#if defined(CONFIG_RTK_NI_SUPPORT_HSGMII_P4)
	ca_ni_serdes_p4_dbg_dump();
#endif
#if defined(CONFIG_RTK_NI_SUPPORT_RXAUI_P5)
	ca_ni_serdes_p5_dbg_dump();
#endif
#if defined(CONFIG_RTK_NI_SUPPORT_QXGMII_P6) || defined(CONFIG_RTK_NI_SUPPORT_USXGMII_P6) || defined(CONFIG_RTK_NI_SUPPORT_XFI_P6)
	ca_ni_serdes_p6_dbg_dump();
#endif
#endif

#if defined(CONFIG_RTK_NI_SUPPORT_RTL8221B) && !defined(CONFIG_RTK_NE_SUPPORT_HSGMII_P7)
	if (priv->is_8221b_detect)
	{
		for (i = 0; i < totalLanPort; i++) {
			if (priv->lan_port_table[i].dport == -1)
				continue;

			phydev = priv->lan_port_table[i].phydev;
			drv = phydev->drv;
			if ((drv->uid&drv->mask) == (PHY_ID_RTL8221B&PHY_ID_MASK)) {
				rtk_ne_rtl8221_dbg_dump(phydev->addr);
			}
		}
	}
#endif

#ifdef CONFIG_RTK_NI_SUPPORT_RTL8261I
	if (priv->is_8261i_detect)
	{
		for (i = 0; i < totalLanPort; i++) {
			if (priv->lan_port_table[i].dport == -1)
				continue;

			phydev = priv->lan_port_table[i].phydev;
			drv = phydev->drv;
			if ((drv->uid&drv->mask) == (PHY_ID_RTL8261&PHY_ID_MASK)) {
				rtk_ne_rtl8261_dbg_dump(phydev);
			}
		}
	}
#endif

	return 0;
}

U_BOOT_CMD(
		ne_debug,   2,      1,      do_ne_debug,
		"dump NE debug msg \n",
		"None\n"
	  );

static int do_phy_reg(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int ret, i;
	u16 phy_addr, reg, val;

	if (argc <= 4) // C22
	{
		if (argc < 2) {
			printf("Usage:\nphy_reg_value%s\n", cmdtp->help);
			return -1;
		}

		phy_addr = simple_strtoul(argv[1], NULL, 10);

		if (phy_addr > 31) {
			printf("Usage:\nphy_reg_value%s\n", cmdtp->help);
			return -1;
		}
		if (argc == 2) {
			/* read the first 15 registers of the PHY */
			printf("PHY addr %d:\n", phy_addr);
			for (i = 0; i < 15; i++) {
				ca_mdio_read(phy_addr, 1, i, 0, &val);
				printf("Reg 0x%04X = 0x%04X\n", i, val);
			}
			return 0;
		}

		reg = simple_strtoul(argv[2], NULL, 10);

		if (argc == 3) {
			/* read cmd */
			ca_mdio_read(phy_addr, 1, reg, 0, &val);
			printf("C22 PHY addr %d Reg 0x%04X = 0x%04X\n", phy_addr, reg, val);
		} else {
			/* argc > 3*/
			/* write cmd */
			val = simple_strtoul(argv[3], NULL, 10);
			ret = ca_mdio_write(phy_addr, 1, reg, 0, val);
			if (!ret) {
				printf("C22 PHY addr %d Reg 0x%04X = 0x%04X\n", phy_addr, reg, val);
			} else {
				printf("Can't write PHY addr %d Reg 0x%04X as 0x%04X, ret = %d\n", phy_addr, reg, val, ret);
			}
		}
	}
	else if (!strcmp(argv[1], "C45")) // C45
	{
		u16 waddr;

		phy_addr = simple_strtoul(argv[2], NULL, 10);

		if (phy_addr > 31) {
			printf("Usage:\nphy_reg_value%s\n", cmdtp->help);
			return -1;
		}

		reg = simple_strtoul(argv[3], NULL, 10);

		waddr = simple_strtoul(argv[4], NULL, 10);

		if (argc == 5) {
			/* read cmd */
			ca_mdio_read(phy_addr, 0, reg, waddr, &val);
			printf("C45 PHY addr %d Reg 0x%04X waddr 0x%x = 0x%04X\n", phy_addr, reg, waddr, val);
		} else {
			/* argc > 5*/
			/* write cmd */
			val = simple_strtoul(argv[5], NULL, 10);
			ret = ca_mdio_write(phy_addr, 0, reg, waddr, val);
			if (!ret) {
				printf("C45 PHY addr %d Reg 0x%04X waddr 0x%x = 0x%04X\n", phy_addr, reg, waddr, val);
			} else {
				printf("Can't write PHY addr %d Reg 0x%04X as 0x%04X, ret = %d\n", phy_addr, reg, val, ret);
			}
		}
	}

	return 0;
}

U_BOOT_CMD(
	  phy_reg,	  6,  1,  do_phy_reg,
	  "read/write PHY register",
	  "[PHY addr] [reg_valueaddr] ([value])\n"
	  "PHY addr : 0-31\n"
	  "C45 [PHY addr] [reg_valueaddr] [waddr] ([value])\n"
	);
#endif
