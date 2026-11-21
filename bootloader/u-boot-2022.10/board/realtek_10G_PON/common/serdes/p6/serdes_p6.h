#ifndef __SERDES_P6_H__
#define __SERDES_P6_H__
#include "../serdes_common.h"

#ifdef CONFIG_RTK_NI_SUPPORT_QXGMII_P6
void ca_ni_p6_qxgmii_init(void);
ca_int32_t ca_ni_p6_qxgmii_port_macForceAbility_set(u32 port, u32 speed);
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_USXGMII_P6
void ca_ni_p6_usxgmii_init(void);
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_XFI_P6
void ca_ni_p6_xfi_init(void);
#endif
#ifdef CONFIG_RTK_NI_SUPPORT_HSGMII_P6
u32 ca_ni_p6_hsgmii_speed_set(rtk_sds_speed_t sdsSpeed);
void ca_ni_p6_hsgmii_init(void);
#endif

#endif /* __SERDES_P6_H__ */

