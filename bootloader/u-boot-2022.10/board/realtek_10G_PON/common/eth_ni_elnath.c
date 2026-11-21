// SPDX-License-Identifier: GPL-2.0+

/*
 * Copyright (C) 2022 Realtek Semiconductor Corp.
 *
 * Ethernet MAC Driver for Taurus based SOC
 */

#include <common.h>
#include <phy.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include "eth_ni.h"

int aal_mode_sync(unsigned int port, aal_sds_mode_t mode,
		  struct phy_device *phy)
{
	NI_HV_PT_PORT_STATIC_CFG_t	port_static_cfg;
	NI_HV_PT_PORT_GLB_CFG_t	port_glb_cfg;
	int rc;

	aal_sds_speed_t speed = AAL_SDS_SPEED_DEFAULT;
	//printf("%s(%d): port=%d mode=%d\n", __func__, __LINE__, port, mode);

	if (phy)
		speed = _to_aal_speed(phy->speed);

	//printf("%s(%d): speed=%d\n", __func__, __LINE__, speed);

	if (speed < 0)
		return -1;

	if (port <= NI_PORT_6) {
		int type = GE_MAC_INTF_GMII;

		if (phy->speed < SPEED_1000)
			type = GE_MAC_INTF_MII;

		port_static_cfg.wrd = CORTINA_CA_REG_READ((NI_HV_PT_PORT_STATIC_CFG +
							   APB0_NI_HV_PT_STRIDE * port));
		port_static_cfg.bf.int_cfg = type;
		CORTINA_CA_REG_WRITE(port_static_cfg.wrd,
				     (NI_HV_PT_PORT_STATIC_CFG + APB0_NI_HV_PT_STRIDE * port));

		port_glb_cfg.wrd = CORTINA_CA_REG_READ((NI_HV_PT_PORT_GLB_CFG +
							APB0_NI_HV_PT_STRIDE * port));
		port_glb_cfg.bf.speed = (phy->speed == SPEED_10) ? 1 : 0;
		port_glb_cfg.bf.duplex = (phy->duplex == DUPLEX_HALF) ? 1 : 0;
		CORTINA_CA_REG_WRITE(port_glb_cfg.wrd,
				     (NI_HV_PT_PORT_GLB_CFG + APB0_NI_HV_PT_STRIDE * port));
	}

	return 0;
}

void aal_set_active_port(unsigned int port)
{
	NI_HV_GLB_INTF_RST_CONFIG_t  intf_rst_config;
	NI_HV_GLB_STATIC_CFG_t static_cfg;

	intf_rst_config.wrd = CORTINA_CA_REG_READ(NI_HV_GLB_INTF_RST_CONFIG);

	switch (port) {
	case NI_PORT_0:
		intf_rst_config.bf.intf_rst_p0 = 0;
		intf_rst_config.bf.mac_rx_rst_p0 = 0;
		intf_rst_config.bf.mac_tx_rst_p0 = 0;
		break;

	case NI_PORT_1:
		intf_rst_config.bf.intf_rst_p1 = 0;
		intf_rst_config.bf.mac_rx_rst_p1 = 0;
		intf_rst_config.bf.mac_tx_rst_p1 = 0;
		break;

	case NI_PORT_2:
		intf_rst_config.bf.intf_rst_p2 = 0;
		intf_rst_config.bf.mac_rx_rst_p2 = 0;
		intf_rst_config.bf.mac_tx_rst_p2 = 0;
		break;

	case NI_PORT_3:
		intf_rst_config.bf.intf_rst_p3 = 0;
		intf_rst_config.bf.mac_tx_rst_p3 = 0;
		intf_rst_config.bf.mac_rx_rst_p3 = 0;
		break;

	case NI_PORT_4:
		intf_rst_config.bf.intf_rst_p4 = 0;
		intf_rst_config.bf.mac_tx_rst_p4 = 0;
		intf_rst_config.bf.mac_rx_rst_p4 = 0;
		break;

	case NI_PORT_5:
		intf_rst_config.bf.intf_rst_p5 = 0;
		intf_rst_config.bf.mac_tx_rst_p5 = 0;
		intf_rst_config.bf.mac_rx_rst_p5 = 0;
		break;

	case NI_PORT_6:
		intf_rst_config.bf.intf_rst_p6 = 0;
		intf_rst_config.bf.mac_tx_rst_p6 = 0;
		intf_rst_config.bf.mac_rx_rst_p6 = 0;
		break;	

	default:
		return;
	}

	CORTINA_CA_REG_WRITE(intf_rst_config.wrd, NI_HV_GLB_INTF_RST_CONFIG);

	/* Only one GMAC can connect to CPU */
	static_cfg.wrd = CORTINA_CA_REG_READ(NI_HV_GLB_STATIC_CFG);
	static_cfg.bf.port_to_cpu = port;
	static_cfg.bf.txmib_mode = 1;
	static_cfg.bf.rxmib_mode = 1;

	CORTINA_CA_REG_WRITE(static_cfg.wrd, NI_HV_GLB_STATIC_CFG);
}
