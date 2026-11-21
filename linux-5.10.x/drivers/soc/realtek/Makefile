# Realtek Semiconductor Corp.
#
# bsp/Makefile
#     bsp Makefile
#
# Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
#


obj-y += rtk_proc.o reboot_reason.o
obj-$(CONFIG_NET) += netdev_reset.o
ifeq ($(CONFIG_RTK_PROC_GPIO_FILES),y)
obj-y += rtk_gpio.o
obj-y += pushbutton.o led-generic.o
obj-$(CONFIG_ARCH_CORTINA_G3HGU) += board_g3hgu.o
## G3HGU and VENUS use the same board_g3hgu..c
obj-$(CONFIG_ARCH_CORTINA_VENUS) += board_g3hgu.o
obj-$(CONFIG_ARCH_REALTEK_TAURUS) += board_g3hgu.o
obj-$(CONFIG_ARCH_REALTEK_9607F) += board_g3hgu.o
obj-$(CONFIG_ARCH_RTL8198F) += board_98F.o
obj-$(CONFIG_RTK_SOC_RTL8198D) += board_rtl8198d.o
obj-$(CONFIG_WPS_LED) += led-wps.o
ifeq ($(CONFIG_RTL8192CD),y)
obj-$(CONFIG_WIFI_LED_USE_SOC_GPIO) += led-wifi.o
endif
obj-$(CONFIG_E8B) += led-power.o
obj-$(CONFIG_RTL8672_SW_USB_LED)    += led-usb.o
obj-$(CONFIG_RTL9607C) += soc_rtl960xc.o
obj-$(CONFIG_RTK_SOC_RTL9607C) += board_rtl960xc.o
endif
obj-$(CONFIG_RTK_SOC_RTL9607C) += board_rtl960xc.o
obj-$(CONFIG_RTK_SOC_RTL8198D) += board_rtl8198d.o
ifndef CONFIG_CORTINA_BOARD_FPGA
obj-$(CONFIG_ARM64) += soc_arm.o
obj-$(CONFIG_ARM) += soc_arm.o
obj-$(CONFIG_ARCH_CORTINA_G3HGU) += rtk_phy_control.o
obj-$(CONFIG_ARCH_CORTINA_VENUS) += soc_arm_venus.o rtk_phy_control_venus.o rtk_spare_reg.o
obj-$(CONFIG_ARCH_REALTEK_TAURUS) += soc_arm_taurus.o rtk_phy_control_venus.o rtk_spare_reg.o
obj-$(CONFIG_ARCH_REALTEK_9607F) += soc_arm_elnath.o rtk_spare_reg.o
obj-$(CONFIG_ARCH_REALTEK_MERCURY) += soc_arm_mercury.o
endif

obj-$(CONFIG_RTK_DDR_TREF_TUNING) += rtk_tref_tuning.o
ifdef CONFIG_HOTPLUG_CPU
obj-$(CONFIG_ARCH_REALTEK_TAURUS) += rtk_taurus_pwr_ctrl.o
endif
ifdef CONFIG_PSTORE_RAM
obj-$(CONFIG_ARCH_REALTEK_TAURUS) += rtk_pstore_taurus.o
obj-$(CONFIG_ARCH_REALTEK_9607F) += rtk_pstore_taurus.o
obj-$(CONFIG_RTK_SOC_RTL9607C) += rtk_pstore_9607c.o sram.o
endif
obj-$(CONFIG_RTK_DP_DIS_SOC) += dying_gasp.o
obj-$(CONFIG_GPIO_RESET_CONTROLLER) += vbus_chain.o
