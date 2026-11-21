/*
 * Copyright (c) 2017 Cortina Access Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the pulpino core
 */

#ifndef __OSAL_SOC_H_
#define __OSAL_SOC_H_

///--#include <rtl9617c_registers.h>

//tmp macro define
#define CONFIG_LUNA_G3_SERIES
#define CONFIG_CHIP_REVISION_OPENWRT 67

#define CA_SOC_VENDOR_ID(jtag_id)       ((jtag_id) & 0xFFF)
#define CA_SOC_CHIP_ID(jtag_id)         (((jtag_id) >> 12) & 0xFFFF)
//corrrect ?
#define CA_SOC_CHIP_REV(jtag_id)        (((jtag_id) >> 28) & 0xF)


#endif /* __OSAL_SOC_H_ */
