#ifndef __SERDES_P5_H__
#define __SERDES_P5_H__
#include "../serdes_common.h"

#ifdef CONFIG_RTK_NI_SUPPORT_RXAUI_P5
u32 ca_ni_p5_hsgmii_rxaui_set(ca_uint16_t phySpeed);
void ca_ni_p5_rxaui_init(void);
#endif
#endif /* __SERDES_P5_H__ */

