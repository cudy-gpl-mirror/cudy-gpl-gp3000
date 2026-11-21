ifeq ($(CONFIG_TARGET_RTK_TAURUS), y)
PLATFORM_CPPFLAGS += -I ./board/realtek_10G_PON/common/ne
PLATFORM_CPPFLAGS += -I ./board/realtek_10G_PON/common/ne/taurus/include/ne
PLATFORM_CPPFLAGS += -I ./board/realtek_10G_PON/common/ne/taurus/include/nic
PLATFORM_CPPFLAGS += -I ./board/realtek_10G_PON/common/ne/taurus/include/osal
PLATFORM_CPPFLAGS += -I ./board/realtek_10G_PON/common/ne/taurus/include/reg
obj-y += taurus/
endif

ifeq ($(CONFIG_TARGET_RTK_ELNATH), y)
PLATFORM_CPPFLAGS += -I ./board/realtek_10G_PON/common/ne
PLATFORM_CPPFLAGS += -I ./board/realtek_10G_PON/common/ne/elnath/include/ne
PLATFORM_CPPFLAGS += -I ./board/realtek_10G_PON/common/ne/elnath/include/nic
PLATFORM_CPPFLAGS += -I ./board/realtek_10G_PON/common/ne/elnath/include/osal
PLATFORM_CPPFLAGS += -I ./board/realtek_10G_PON/common/ne/elnath/include/reg
obj-y += elnath/
endif

obj-$(CONFIG_RTK_NE) += realtek_ne.o rtk_mdio.o rtk_phy.o