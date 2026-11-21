#obj-y += rtk_proc.o netdev_reset.o
#obj-$(CONFIG_RTK_NETLOG)		+= netlog.o
#obj-$(CONFIG_USB)	+= rtk_usb_phy.o
obj-$(CONFIG_RTK_KERNEL_OMD) += rtl_signalCapture.o
#obj-$(CONFIG_RTK_HS_UART) += hs_uart.o
ifeq ($(CONFIG_RTK_WLAN_EVENT_INDICATE),y)
ifeq ($(CONFIG_RTK_DEV_AP),y)
obj-y += rtk_wlan_event_netlink_for_dev_ap.o
else
obj-y += rtk_wlan_event_netlink.o
endif
endif

obj-y += framework_protector/

ifdef CONFIG_SYSTEM_CAPABILITY
obj-y += sys_capability.o
endif
ifdef CONFIG_UNI_CAPABILITY
EXTRA_CFLAGS += -I$(APOLLODIR)/sdk/include -I$(APOLLODIR)/sdk/system/include
obj-y += uni_capability.o
endif
obj-$(CONFIG_RTK_VBUS_CHAIN) += vbus_chain.o
obj-$(CONFIG_RTK_ARM64_WATCHPOINT_DEBUG) += rtk_arm64_data_breakpoint.o
obj-$(CONFIG_LUNA) += rtk_loopdetect.o
obj-y += sys_memory.o
obj-$(CONFIG_RTK_IPV6_ETHERNET_TUNNEL) += rtk_tunnel6_tap.o rtk_ip6_tunnel_tap.o
ifdef CONFIG_RTK_MIRROR
obj-y += rtk_mirror.o
endif