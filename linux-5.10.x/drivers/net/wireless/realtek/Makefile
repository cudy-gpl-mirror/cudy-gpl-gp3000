# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the Linux Wireless network device drivers for Realtek units
#

obj-$(CONFIG_RTL8180)		+= rtl818x/
obj-$(CONFIG_RTL8187)		+= rtl818x/
obj-$(CONFIG_RTLWIFI)		+= rtlwifi/
obj-$(CONFIG_RTL8XXXU)		+= rtl8xxxu/
obj-$(CONFIG_RTW88)		+= rtw88/
obj-$(CONFIG_RTL8192CD)		+= rtl8192cd/
obj-$(CONFIG_WNIC_RECYCLED_SKB) += wnic_skb_pool/
ifneq ($(CONFIG_RTLWIFI6),)
obj-$(CONFIG_RTLWIFI6)		+= g6_wifi_driver/
else
obj-$(CONFIG_RTL8852AE)		+= g6_wifi_driver/
endif

ifneq ($(CONFIG_WFO_VIRT),)
obj-$(CONFIG_WFO_VIRT)		+= wfo_virt/
endif
ifneq ($(CONFIG_THER_CTRL),)
obj-$(CONFIG_THER_CTRL)		+= ther_ctrl/
endif
