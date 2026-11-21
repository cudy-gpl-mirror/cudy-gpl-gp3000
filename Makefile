#
# Lets work out what the user wants, and if they have configured us yet
#

ifeq (.config,$(wildcard .config))
include .config
else
all: config_error
endif

ifeq ($(CONFIG_PRODUCT), luna_ap_mips)
all:  show_toolchain bootloader_image linux backports subdirs romfs rcX modules modules_install image vmimg
else
ifneq (,$(filter asdk aarch,$(findstring asdk,$(CONFIG_RSDK_DIR)) $(findstring aarch,$(CONFIG_RSDK_DIR))))
ifdef CONFIG_32BIT_IMAGE
LINUXTARGET = uImage
endif
all:  show_toolchain bootloader_gpt_image tools_prepare packages_ca linux linux_voip backports packages_user subdirs romfs rcX modules_install image vmimg
else
all:  show_toolchain linux backports subdirs romfs rcX modules_install image vmimg cudy_factory
LINUXTARGET = uImage
endif
endif
ifeq ($(CONFIG_USER_XMLCONFIG), y)
PROVINCE_NAME=province
else
PROVINCE_NAME=province_boa_xml
endif
#############################################
# Do not include linux/.config 
# Linux config has the same option name 
# with u-boot
#############################################
LINUXDIR = $(CONFIG_LINUXDIR)
CA_KMOD = ca_packages
CA_SCFG_CFG = $(CA_KMOD)/ca-scfg/config
3RD_PACKAGES = 3rd_packages
CA_USER = ca_user
LIBNL_3_2_25_DIR = $(CA_USER)/libnl/libnl-3.2.25
CA_SYSROOT = sysroot-destdir
LIBUSB_1_0_21_DIR = lib/libusb-1.0.21/libusb-1.0.21
EUDEV_3_2_DIR = lib/eudev-3.2/eudev-3.2
###################################
HOST_CPU_NUM = $(shell cat /proc/cpuinfo | grep -c processor)
ROOTDIR  = $(shell pwd)
ifdef CONFIG_32BIT_IMAGE
PATH     := $(PATH):$(ROOTDIR)/tools:$(ROOTDIR)/tools/uboot_2018
else
PATH     := $(PATH):$(ROOTDIR)/tools:$(ROOTDIR)/vendors/Realtek/luna_ap_arm
endif
HOSTCC   = unset GCC_EXEC_PREFIX; cc
IMAGEDIR = $(ROOTDIR)/images
ROMFSDIR = $(ROOTDIR)/romfs
TMPFSDIR = $(ROOTDIR)/tmpfs
PKG_CONFIG=$(TMPFSDIR)/bin/pkg-config
PKG_CONFIG_EXTRAARGS=--define-variable=prefix="${TMPFSDIR}" \
	--define-variable=exec_prefix="${TMPFSDIR}" \
	--define-variable=bindir="${TMPFSDIR}/bin" 
#PKG_CONFIG_LIBDIR=$(TMPFSDIR)/lib/pkgconfig
#PKG_CONFIG_PATH=$(TMPFSDIR)/lib/pkgconfig
ROMFSINST= romfs-inst.sh
KERNEL_SRC = $(ROOTDIR)/$(LINUXDIR)
KERNEL_DIR = $(ROOTDIR)/$(LINUXDIR)
STAGING_KERNEL_BUILDDIR = $(ROOTDIR)/$(LINUXDIR)
LINUX_CONFIG  = $(ROOTDIR)/$(LINUXDIR)/.config
CONFIG_CONFIG = $(ROOTDIR)/config/.config
MODULES_CONFIG = $(ROOTDIR)/modules/.config
DEFAULT_UBINIZE_CFG_MIPS=$(ROOTDIR)/tools/ubinize-luna-eng.cfg
DEFAULT_UBINIZE_CFG_ARM=$(ROOTDIR)/tools/ubinize-g3-eng_ubi.cfg
BACKPORT_DIR = $(ROOTDIR)/backports-5.2.8-1
BACKPORT_LOCAL_SYM = local-symbols
BACKPORT_LOCAL_SYM_OLD = local-symbols.old
BACKPORT_LOCAL_SYM_UPDATED = .local-sym-updated
BACKPORT_CONFIG = $(BACKPORT_DIR)/.config
BACKPORT_AUTOCONF_H = $(BACKPORT_DIR)/backport-include/backport/autoconf.h
WIFI_DRIVER_DIR = $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/realtek/g6_wifi_driver
FW_IMG = $(WIFI_DRIVER_DIR)/phl/hal_g6/mac/fw_ax/rtl8852a/RTL8852Afw.bin
BACKPORT_RTL8192FE_NAME = rtl8192fe
BACKPORT_RTL8192CD_NAME = rtl8192cd
#### DTS PATH ###
DTS_PATH = $(LINUXDIR)/arch/arm64/boot/dts/realtek/

KERNEL_CONFIG=$(shell grep -rn CONFIG_RTK_DEV_AP $(ROOTDIR)/$(CONFIG_LINUXDIR)/.config)

ifeq ($(findstring mips-EB-4.4,$(CONFIG_RSDK_DIR)),)
SECCFLAGS = -fstack-protector-all -fPIE
SECLDFLAGS = -Wl,-z,relro -Wl,-z,now -pie
LIB_SECCFLAGS = -fstack-protector-all -fPIC
LIB_SECLDFLAGS = -Wl,-z,relro -Wl,-z,now
CHECKSEC_CONFIG = $(ROOTDIR)/checksec.mk
CHECKSEC_LIB_CONFIG = $(ROOTDIR)/checksec_lib.mk
export CHECKSEC_CONFIG CHECKSEC_LIB_CONFIG SECCFLAGS SECLDFLAGS LIB_SECCFLAGS LIB_SECLDFLAGS
endif

ifeq (aarch,$(findstring aarch,$(CONFIG_RSDK_DIR)))
STAGING_DIR = $(ROOTDIR)/staging_dir
export STAGING_DIR
endif

# Follow kernel tree
ifeq ($(CONFIG_LINUXDIR),linux-4.4.x)
KERNEL_VERSION = 4.4.140
CONF =  conf44
else
KERNEL_VERSION = 3.18.20
CONF = conf318
endif
ifeq ($(CONFIG_LINUXDIR),linux-4.14.x)
KERNEL_VERSION = 4.14.172
CONF =  conf414
endif
ifeq ($(CONFIG_LINUXDIR),linux-5.10.x)
SUBLEVEL:=$(shell grep ^SUBLEVEL  linux-5.10.x/Makefile |  cut -d" " -f3 )
KERNEL_VERSION = 5.10.$(SUBLEVEL)
CONF =  conf510
##  Force linux-5 modpost treat  "modpost: xxx undefined!" as warning.
export KBUILD_MODPOST_WARN=1
endif
$(info KERNEL_VERSION = $(KERNEL_VERSION))
export KERNEL_VERSION

LOCALVERSION=
export LOCALVERSION
#######################################################################################
ifeq ($(MAKECMDGOALS:preconfig414_%=preconfig414),preconfig414)
	ifneq ($(wildcard ./vendors/Realtek/luna/conf414/$(MAKECMDGOALS:preconfig414_%=%)),)
		CONFIG_PRODUCT=luna
	else ifneq ($(wildcard ./vendors/Realtek/luna_xdsl/conf414/$(MAKECMDGOALS:preconfig414_%=%)),)
		CONFIG_PRODUCT=luna_xdsl
	else
		$(error configuration found)
	endif
endif

ifeq ($(MAKECMDGOALS:preconfig44_%=preconfig44),preconfig44)
	ifneq ($(wildcard ./vendors/Realtek/luna/conf44/$(MAKECMDGOALS:preconfig44_%=%)),) 
		CONFIG_PRODUCT=luna
#	else ifneq ($(wildcard ./vendors/Realtek/luna_mips/conf44/$(MAKECMDGOALS:preconfig44_%=%)),) 
#		CONFIG_PRODUCT=luna_mips
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_arm/conf44/$(MAKECMDGOALS:preconfig44_%=%)),) 
		CONFIG_PRODUCT=luna_ap_arm
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_mips/conf44/$(MAKECMDGOALS:preconfig44_%=%)),) 
		CONFIG_PRODUCT=luna_ap_mips
	else ifneq ($(wildcard ./vendors/Realtek/luna_xdsl/conf44/$(MAKECMDGOALS:preconfig44_%=%)),)
		CONFIG_PRODUCT=luna_xdsl
	else
		$(error configuration found)
	endif 
endif
ifeq ($(MAKECMDGOALS:preconfig318_%=preconfig318),preconfig318)
	ifneq ($(wildcard ./vendors/Realtek/luna/conf318/$(MAKECMDGOALS:preconfig318_%=%)),)
		CONFIG_PRODUCT=luna
	else ifneq ($(wildcard ./vendors/Realtek/luna_xdsl/conf318/$(MAKECMDGOALS:preconfig318_%=%)),)
		CONFIG_PRODUCT=luna_xdsl
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_arm/conf318/$(MAKECMDGOALS:preconfig318_%=%)),)
		CONFIG_PRODUCT=luna_ap_arm
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_mips/conf318/$(MAKECMDGOALS:preconfig318_%=%)),)
		CONFIG_PRODUCT=luna_ap_mips
	else
		$(error configuration found)
	endif 
endif

ifeq ($(MAKECMDGOALS:preconfig_%=preconfig),preconfig)
	ifneq ($(wildcard ./vendors/Realtek/luna/$(CONF)/$(MAKECMDGOALS:preconfig_%=%)),)
		CONFIG_PRODUCT=luna
	else ifneq ($(wildcard ./vendors/Realtek/luna_xdsl/$(CONF)/$(MAKECMDGOALS:preconfig_%=%)),)
		CONFIG_PRODUCT=luna_xdsl
#	else ifneq ($(wildcard ./vendors/Realtek/luna_mips/$(CONF)/$(MAKECMDGOALS:preconfig_%=%)),) 
#		CONFIG_PRODUCT=luna_mips
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_arm/$(CONF)/$(MAKECMDGOALS:preconfig_%=%)),) 
		CONFIG_PRODUCT=luna_ap_arm
	else ifneq ($(wildcard ./vendors/Realtek/luna_ap_mips/$(CONF)/$(MAKECMDGOALS:preconfig_%=%)),) 
		CONFIG_PRODUCT=luna_ap_mips
	else
		$(error configuration found)
	endif 
endif

ifeq ($(MAKECMDGOALS:preconfig44_%=preconfig44),preconfig44)
$(info $(wildcard $(ROOTDIR)/vendors/Realtek/luna/conf44/$(MAKECMDGOALS:preconfig44_%=preconfig44)))
else ifeq ($(MAKECMDGOALS:preconfig318_%=preconfig318),preconfig318)
ifeq ($(CONFIG_PRODUCT), luna)
$(info $(wildcard $(ROOTDIR)/vendors/Realtek/luna/conf318/$(MAKECMDGOALS:preconfig318_%=preconfig318)))
else ifeq ($(CONFIG_PRODUCT), luna_xdsl)
$(info $(wildcard $(ROOTDIR)/vendors/Realtek/luna_xdsl/conf318/$(MAKECMDGOALS:preconfig318_%=preconfig318)))
endif
else ifeq ($(MAKECMDGOALS:preconfig_%=preconfig),preconfig)
$(info $(wildcard $(ROOTDIR)/vendors/Realtek/luna/$(CONF)/$(MAKECMDGOALS:preconfig_%=preconfig)))
endif

ifeq ($(CONFIG_PRODUCT),luna_ap_arm)  ##98f , g3lite
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
CA_TARGET = g3lite
else
CA_TARGET = g3
endif
else ##pon , g3
CA_TARGET = g3
endif
export CA_TARGET

ifeq ($(CONFIG_PRODUCT),luna_ap_arm)  ##98f
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
LINUX_DTS_DIR = $(ROOTDIR)/$(LINUXDIR)/arch/arm/boot/dts/realtek
endif
endif

$(info CONFIG_PRODUCT=$(CONFIG_PRODUCT))

CONFIG_PRODUCT ?= luna
CONFIG_VENDOR ?= Realtek
VENDDIR = $(ROOTDIR)/vendors/$(CONFIG_VENDOR)/$(CONFIG_PRODUCT)/.
DIRS    = lib user2 user 
BUSYBOXDIR_DEST = user/busybox-1.22.1
INSTALL_MOD_PATH = $(ROMFSDIR)
INSTALL_PATH = $(IMAGEDIR)
BOOTLOADER_PATH= $(ROOTDIR)/bootloader

MULTI_LANG_DL ?= 0
export MULTI_LANG_DL

########## cmake Makefile ##########
CMAKE_MAKEFILE_PATH = $(ROOTDIR)/cmake.mk
export CMAKE_MAKEFILE_PATH
####################################

ifeq (config.arch,$(wildcard config.arch))
include config.arch
ARCH_CONFIG = $(ROOTDIR)/config.arch
export ARCH_CONFIG LINUX_DTS_DIR
endif

.EXPORT_ALL_VARIABLES:

############################################################################
#
# Software Version
#
CONFIG_CU_BASEON_YUEME:=$(shell cat $(LINUXDIR)/.config | grep CONFIG_CU_BASEON_YUEME= | cut -d"=" -f2)
CONFIG_TELMEX_SERCM:=$(shell cat $(LINUXDIR)/.config | grep CONFIG_TELMEX_SERCM= | cut -d"=" -f2)
SW_DEFAULT_VER="4.2.0"
ifeq ($(CONFIG_CU_BASEON_YUEME), y)
VERSIONPKG := V01.01.0
else
ifeq ($(CONFIG_SW_VERSION), "")
SW_VERSION=$(SW_DEFAULT_VER)
else
SW_VERSION=$(CONFIG_SW_VERSION)
endif
ifeq ($(CONFIG_SWVER_YEAR), y)
TIMEFORMAT+=%Y
endif
ifeq ($(CONFIG_SWVER_MON), y)
TIMEFORMAT+=%m
endif
ifeq ($(CONFIG_SWVER_DAY), y)
TIMEFORMAT+=%d
endif
ifeq ($(CONFIG_SWVER_HOUR), y)
TIMEFORMAT+=%H
endif
ifeq ($(CONFIG_SWVER_MIN), y)
TIMEFORMAT+=%M
endif
ifneq ($(TIMEFORMAT),)
FORMAT="$(shell echo -e $(TIMEFORMAT) | tr -d '[:space:]')"
VERSIONPKG=$(SW_VERSION)"-"$(shell date +$(FORMAT)|tr -d 'e')"-"$(shell date +%s|rev|cut -c 1-6)
else
VERSIONPKG=$(SW_VERSION)
endif
endif
ifeq ($(CONFIG_TELMEX_SERCM), y)
VERSIONPKG = $(SW_VERSION)
endif
VERSIONSTR = $(VERSIONPKG)

ifdef OMCI_VERSION
VERSIONPKG := $(OMCI_VERSION)
else
ifdef CONFIG_JIO
icver:=$(shell cat .config | grep CONFIG_PRD_SIGNATURE | cut -d"=" -f2)
CHIPVENDOR:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep CHIPVENDOR | cut -d"=" -f2)
OT:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep OT | cut -d"=" -f2)
DEVICEMODE:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep DEVICEMODE | cut -d"=" -f2)
RELEASEVERION:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep $(icver) | cut -d"=" -f3)
MODELNAME:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep $(icver) | cut -d"=" -f2)
RELEASETYPE:=$(shell cat $(LINUXDIR)/luna_sdk_ver_jio | grep RELEASETYPE | cut -d"=" -f2)
VERSIONPKG := $(CHIPVENDOR)$(OT)$(DEVICEMODE)$(MODELNAME)$(R)$(RELEASETYPE)$(RELEASEVERION)
else
endif # CONFIG_JIO
endif # OMCI_VERSION

############################################################################
#
# Get the ccache
#
ifeq ($(USE_CCACHE),1)
$(info ###### use ccache #####)
CCACHE=ccache
else
$(info ##### NOT use ccache #####)
CCACHE=
endif

ifeq ($(USE_PMAKE),1)
$(info ##### use parallel make #####)
PARAL_MAKE=-j $(HOST_CPU_NUM)
else
$(info ##### NOT use parallel make #####)
PARAL_MAKE=
endif

############################################################################
#
# Get the core stuff worked out
#

MAKEARCH = $(MAKE) ARCH=$(ARCH) CROSS_COMPILE="$(CCACHE) $(CROSS_COMPILE)"
MAKEARCH_KERNEL = $(MAKEARCH)
PATH := $(RSDK)/bin:$(PATH)
SCRIPTSDIR = $(ROOTDIR)/config/scripts
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)
CROSS_HOST ?=mips-linux
CROSS_TARGET ?=mips-linux
export CROSS_HOST CROSS_TARGET  HOST_CPU_NUM RSDK
export MAKEARCH MAKEARCH_KERNEL KERNEL_SRC KERNEL_DIR ROOTDIR STAGING_KERNEL_BUILDDIR LINUXDIR IMAGEDIR
export CROSS_COMPILE SCRIPTSDIR CONFIG_SHELL ROMFSDIR TMPFSDIR VERSIONPKG VERSIONSTR INSTALL_MOD_PATH PKG_CONFIG 
#### Use CONFIG_PRODUCT to judge g3 or g3-lite
export CONFIG_PRODUCT

KDIR=$(ROOTDIR)/$(LINUXDIR)
NETDIR=$(KDIR)/drivers/net/ethernet/realtek

APOLLODIR=$(NETDIR)/rtl86900
SDKDIR=$(APOLLODIR)/sdk
APPDIR=$(SDKDIR)/src/app
export KDIR APOLLODIR SDKDIR NETDIR

KBUILD_EXTRA_SYMBOLS=$(APOLLODIR)/ca-ne/Module.symvers
KBUILD_EXTRA_SYMBOLS+=$(ROOTDIR)/ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0-venus/Module.symvers
KBUILD_EXTRA_SYMBOLS+=$(ROOTDIR)/ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0-77c/Module.symvers

ifdef CONFIG_USE_RSDK_WRAPPER
SSTRIP = $(CROSS_COMPILE)sstrip
LSTRIP = $(CROSS_COMPILE)lstrip
LSTRIPPED = $(ROMFSDIR)/.lstripped
endif #CONFIG_USE_RSDK_WRAPPER


UBOOTENVF:=$(shell echo $(CONFIG_UBOOT_ENVFILE) | cut -d "." -f1 )

##### preconfig ########################
ifeq ($(MAKECMDGOALS:preconfig44_%=preconfig44),preconfig44)
		PRECFGDIR  := $(ROOTDIR)/vendors/Realtek/$(CONFIG_PRODUCT)/conf44/$(MAKECMDGOALS:preconfig44_%=%)
endif

ifeq ($(MAKECMDGOALS:preconfig318_%=preconfig318),preconfig318)
		PRECFGDIR  := $(ROOTDIR)/vendors/Realtek/$(CONFIG_PRODUCT)/conf318/$(MAKECMDGOALS:preconfig318_%=%)
endif
export PRECFGDIR

tools_prepare:
	@echo "Host Tools Prepare......"
	@cd tools; sh ./build_tools.sh
	@echo "Host Tools Prepare......Done!"

preconfig_clean_cfg:
	rm -f .UBIFS_ARGSCFG
	rm -f $(ROOTDIR)/user2/proc_var/board_init.sh
preconfig_%: preconfig_clean_cfg
	@if [ -f $(BOOTLOADER_PATH)/u-boot-env/uboot-env-98*-eng.txt ] ; then \
		rm $(BOOTLOADER_PATH)/u-boot-env/uboot-env-98*-eng.txt; \
	fi
	@if [ -d $(VENDDIR)/$(CONF)/$* ] ; then \
		if egrep "^CONFIG_CMCC=y" $(VENDDIR)/$(CONF)/$*/config_kernel > /dev/null; then \
			if [ -f $(VENDDIR)/$(CONF)/$*/backgroup_style.css ]; then \
				cp $(VENDDIR)/$(CONF)/$*/*.css user/boa_e8/src/web_cmcc/style/;\
			fi;\
			touch $(LIBUSB_1_0_21_DIR)/aclocal.m4 $(LIBUSB_1_0_21_DIR)/configure $(LIBUSB_1_0_21_DIR)/Makefile.in;\
			touch $(JSON_C_DIR)/aclocal.m4 $(JSON_C_DIR)/configure;\
			touch $(EUDEV_3_2_DIR)/aclocal.m4 $(EUDEV_3_2_DIR)/configure $(EUDEV_3_2_DIR)/Makefile.in;\
		fi; \
    	if egrep "^CONFIG_CU_BASEON_CMCC=y" $(VENDDIR)/$(CONF)/$*/config_kernel > /dev/null; then \
			if [ -f $(VENDDIR)/$(CONF)/$*/backgroup_style.css ]; then \
				cp $(VENDDIR)/$(CONF)/$*/*.css user/boa_e8/src/web_cu/style/;\
			fi;\
			touch $(LIBUSB_1_0_21_DIR)/aclocal.m4 $(LIBUSB_1_0_21_DIR)/configure $(LIBUSB_1_0_21_DIR)/Makefile.in;\
			touch $(JSON_C_DIR)/aclocal.m4 $(JSON_C_DIR)/configure;\
			touch $(EUDEV_3_2_DIR)/aclocal.m4 $(EUDEV_3_2_DIR)/configure $(EUDEV_3_2_DIR)/Makefile.in;\
		fi; \
		cp $(VENDDIR)/$(CONF)/$*/config .config && \
		cp $(VENDDIR)/$(CONF)/$*/config_kernel $(LINUXDIR)/.config && \
		cp $(VENDDIR)/$(CONF)/$*/config_user $(CONFIG_CONFIG) ; \
		[ -f $(BACKPORT_CONFIG) ] && rm $(BACKPORT_CONFIG) ; \
		if egrep "^CONFIG_MLO_KERNEL_PATCH=y" $(VENDDIR)/$(CONF)/$*/config_kernel > /dev/null; then \
			if [ -d $(KERNEL_DIR)/drivers/net/wireless/realtek/g6_wifi_driver_v3.x.x ] && [ ! -L $(KERNEL_DIR)/drivers/net/wireless/realtek/g6_wifi_driver ]; then \
				mv $(KERNEL_DIR)/drivers/net/wireless/realtek/g6_wifi_driver $(KERNEL_DIR)/drivers/net/wireless/realtek/g6_wifi_driver_v2.x.x; \
				ln -sf $(KERNEL_DIR)/drivers/net/wireless/realtek/g6_wifi_driver_v3.x.x $(KERNEL_DIR)/drivers/net/wireless/realtek/g6_wifi_driver; \
			fi; \
		else \
			if [ -L $(KERNEL_DIR)/drivers/net/wireless/realtek/g6_wifi_driver ]; then \
				rm -rf $(KERNEL_DIR)/drivers/net/wireless/realtek/g6_wifi_driver; \
				mv $(KERNEL_DIR)/drivers/net/wireless/realtek/g6_wifi_driver_v2.x.x  $(KERNEL_DIR)/drivers/net/wireless/realtek/g6_wifi_driver; \
			fi; \
		fi; \
		if egrep "^CONFIG_RTL8852AE_BACKPORTS=y" $(VENDDIR)/$(CONF)/$*/config_kernel > /dev/null; then \
			cp $(VENDDIR)/$(CONF)/$*/config_backports-5.2.8-1 $(BACKPORT_CONFIG) ; \
		fi; \
		if [ -d $(BACKPORT_DIR) ]; then \
			[ ! -L $(BACKPORT_DIR)/drivers/net/wireless/realtek/g6_wifi_driver ] || rm $(BACKPORT_DIR)/drivers/net/wireless/realtek/g6_wifi_driver ; \
			ln -s $(KERNEL_DIR)/drivers/net/wireless/realtek/g6_wifi_driver $(BACKPORT_DIR)/drivers/net/wireless/realtek/g6_wifi_driver ; \
			[ ! -L $(BACKPORT_DIR)/drivers/net/wireless/realtek/rtl8192cd ] || rm $(BACKPORT_DIR)/drivers/net/wireless/realtek/rtl8192cd ; \
			. $(LINUX_CONFIG); if [ ! "$$CONFIG_RTK_DEV_AP" = "y" ]; then \
				ln -s $(KERNEL_DIR)/drivers/net/wireless/realtek/rtl8192cd $(BACKPORT_DIR)/drivers/net/wireless/realtek/rtl8192cd ; \
			fi; \
			if [ ! -d $(KERNEL_DIR)/drivers/net/wireless/realtek/rtl8723fu ]; then \
				mkdir -p $(KERNEL_DIR)/drivers/net/wireless/realtek/rtl8723fu; \
				touch $(KERNEL_DIR)/drivers/net/wireless/realtek/rtl8723fu/Kconfig; \
				touch $(KERNEL_DIR)/drivers/net/wireless/realtek/rtl8723fu/Makefile; \
			fi; \
		fi; \
		if [ "$(CONFIG_PRODUCT)" = "luna_ap_mips" ];then  \
			rm -f config.arch;\
			ln -s $(VENDDIR)/config.arch config.arch;\
		fi;\
		if [ "$(CONFIG_PRODUCT)" = "luna_ap_arm" ];then  \
			if egrep "^CONFIG_RSDK_DIR=.*-a17-EL-.*" $(ROOTDIR)/.config > /dev/null; then \
				echo "This is 98F." ; \
				rm -f config.arch;\
				ln -s $(VENDDIR)/config.arch config.arch;\
			else  \
				echo "This is 98XA." ; \
				rm -f config.arch;\
				ln -s $(VENDDIR)/config.arch.arm64 config.arch;\
			fi;\
		fi;\
		if [ "$(CONFIG_PRODUCT)" = "luna_ap_mips" ];then  \
			FLASH_TYPE=$$(cat .config | grep UBOOT_FLASH_TYPE) ;\
			echo $$FLASH_TYPE;\
			cd $(BOOTLOADER_PATH);\
			rm -f FLASH_TYPE* ;\
			if [ "$$FLASH_TYPE" = "UBOOT_FLASH_TYPE=SPI_NAND" ];then\
				touch FLASH_TYPE_SPI_NAND ;\
			fi;\
			if [ "$$FLASH_TYPE" = "UBOOT_FLASH_TYPE=NOR" ];then\
                                touch FLASH_TYPE_NOR ;\
                        fi;\
			cd - ;\
		fi;\
		if [ "$(CONFIG_PRODUCT)" = "luna" -o "$(CONFIG_PRODUCT)" = "luna_ap_arm" ];then  \
			find $(VENDDIR)/$(CONF)/$*/ -name 'uboot-env-*.txt' -exec cp {} $(BOOTLOADER_PATH)/u-boot-env/  \;; \
			if [ -f $(VENDDIR)/$(CONF)/$*/device_ubinize.cfg ] ; then  \
				ln -sf $(VENDDIR)/$(CONF)/$*/device_ubinize.cfg $(ROOTDIR)/tools/device_ubinize.cfg;  \
			else  \
				if egrep "^CONFIG_MIPS=y" $(VENDDIR)/$(CONF)/$*/config_kernel > /dev/null; then \
					ln -sf $(DEFAULT_UBINIZE_CFG_MIPS) $(ROOTDIR)/tools/device_ubinize.cfg; \
				else \
					ln -sf $(DEFAULT_UBINIZE_CFG_ARM) $(ROOTDIR)/tools/device_ubinize.cfg; \
				fi;\
			fi ; \
		fi ;\
		if [ "$(CONFIG_PRODUCT)" = "luna_xdsl" ];then  \
			find $(VENDDIR)/$(CONF)/$*/ -name 'uboot-env-*.txt' -exec cp {} $(BOOTLOADER_PATH)/u-boot-env/  \;; \
			if [ -f $(VENDDIR)/$(CONF)/$*/device_ubinize.cfg ] ; then  \
				ln -sf $(VENDDIR)/$(CONF)/$*/device_ubinize.cfg $(ROOTDIR)/tools/device_ubinize.cfg;  \
			else  \
				if egrep "^CONFIG_MIPS=y" $(VENDDIR)/$(CONF)/$*/config_kernel > /dev/null; then \
					ln -sf $(DEFAULT_UBINIZE_CFG_MIPS) $(ROOTDIR)/tools/device_ubinize.cfg; \
				else \
					ln -sf $(DEFAULT_UBINIZE_CFG_ARM) $(ROOTDIR)/tools/device_ubinize.cfg; \
				fi;\
			fi ; \
		fi ;\
		ln -sfn $(VENDDIR)/$(CONF)/$* preconfig; \
		[ -f $(VENDDIR)/$(CONF)/$*/run.sh ] &&  $(VENDDIR)/$(CONF)/$*/run.sh $(VENDDIR)/$(CONF)/$* ; \
		[ -f $(ROOTDIR)/ca_packages/scfg_cfg_cp.sh ] &&  $(ROOTDIR)/ca_packages/scfg_cfg_cp.sh  $(VENDDIR)/$(CONF)/$* ; \
		[ -f  $(VENDDIR)/$(CONF)/$*/rtl8198f-engboard.dts  ]  && \
			ln -sf $(VENDDIR)/$(CONF)/$*/rtl8198f-engboard.dts $(LINUX_DTS_DIR)/rtl8198f-engboard.dts ; \
		[ -f  $(VENDDIR)/$(CONF)/$*/uboot-env-98f-eng.txt  ]  && \
			ln -sf $(VENDDIR)/$(CONF)/$*/uboot-env-98f-eng.txt  $(BOOTLOADER_PATH)/u-boot-env/uboot-env-98f-eng.txt ; \
		[ -f  $(VENDDIR)/$(CONF)/$*/uboot-env-98d-eng.txt  ]  && \
			ln -sf $(VENDDIR)/$(CONF)/$*/uboot-env-98d-eng.txt  $(BOOTLOADER_PATH)/u-boot-env/uboot-env-98d-eng.txt ; \
		[ -f  $(VENDDIR)/$(CONF)/$*/config_busybox_1221 ] && cp $(VENDDIR)/$(CONF)/$*/config_busybox_1221 $(BUSYBOXDIR_DEST)/.config ; \
		if [ -f $(VENDDIR)/$(CONF)/$*/soc.h ]; then \
			cp $(VENDDIR)/$(CONF)/$*/soc.h $(LINUXDIR)/include/soc/soc.h ; \
		fi; \
		if [ -f $(VENDDIR)/$(CONF)/$*/luna_cfg.h ]; then \
			cp $(VENDDIR)/$(CONF)/$*/luna_cfg.h $(LINUXDIR)/include/soc/luna_cfg.h ; \
		else \
			echo '/* dummy file */' > $(LINUXDIR)/include/soc/luna_cfg.h ; \
		fi; \
		[ -f config/config_default.xml ] && mv config/config_default.xml config/config_default.xml.backup; \
		[ -f config/config_default_hs.xml ] && mv config/config_default_hs.xml config/config_default_hs.xml.backup; \
		cp $(VENDDIR)/$(CONF)/$*/config_default.xml config/config_default.xml ; \
		cp $(VENDDIR)/$(CONF)/$*/config_default_hs.xml config/config_default_hs.xml ; \
		[ -d config/wifi.bak ] && rm -rf config/wifi.bak; \
		[ -d config/wifi ] && mv config/wifi config/wifi.bak; \
		if egrep "^CONFIG_RTK_BOARDID=y" $(LINUXDIR)/.config > /dev/null; then \
			if [ -d $(VENDDIR)/$(CONF)/$*/wifi ]; then \
				mkdir -p config/wifi ;\
				cp -raf $(VENDDIR)/$(CONF)/$*/wifi/* config/wifi/. ;\
			fi;\
		fi;\
                [ -d config/province.bak ] && rm -rf config/province.bak; \
                [ -d config/province ] && mv config/province config/province.bak; \
		if egrep "^CONFIG_CMCC=y" $(LINUXDIR)/.config > /dev/null; then \
			if egrep "^CONFIG_USER_CONF_ON_XMLFILE=y" $(ROOTDIR)/config/.config > /dev/null; then \
				if [ -d $(VENDDIR)/$(CONF)/province_cmcc_xml ]; then \
					mkdir -p config/province ;\
					cp -raf $(VENDDIR)/$(CONF)/province_cmcc_xml/* config/province/. ;\
				fi;\
			else \
				if [ -d $(VENDDIR)/$(CONF)/province_cmcc ]; then \
					mkdir -p config/province ;\
					cp -raf $(VENDDIR)/$(CONF)/province_cmcc/* config/province/. ;\
				fi;\
			fi;\
		else \
			if egrep "^CONFIG_USER_CONF_ON_XMLFILE=y" $(ROOTDIR)/config/.config > /dev/null; then \
				if egrep "^CONFIG_CU=y" $(LINUXDIR)/.config > /dev/null; then \
					if [ -d $(VENDDIR)/$(CONF)/province_cu_xml ]; then \
						mkdir -p config/province ;\
						cp -raf $(VENDDIR)/$(CONF)/province_cu_xml/* config/province/. ;\
					fi;\
				else \
					if egrep "^CONFIG_YUEME=y" $(LINUXDIR)/.config > /dev/null; then \
						if [ -d $(VENDDIR)/$(CONF)/$(PROVINCE_NAME) ]; then \
							mkdir -p config/province ;\
							cp -raf $(VENDDIR)/$(CONF)/$(PROVINCE_NAME)/* config/province/. ;\
						fi;\
					else \
 						if egrep "^CONFIG_TELMEX=y" $(LINUXDIR)/.config > /dev/null; then \
							if [ -d $(VENDDIR)/$(CONF)/province_telmex_xml ]; then \
								mkdir -p config/province ;\
								cp -raf $(VENDDIR)/$(CONF)/province_telmex_xml/* config/province/. ;\
							fi;\
						else \
							if [ -d $(VENDDIR)/conf44/$*/province ]; then \
								mkdir -p config/province ;\
								cp -raf $(VENDDIR)/conf44/$*/province/* config/province/. ;\
							fi;\
						fi;\
					fi;\
				fi;\
			else \
        			if egrep "^CONFIG_CU=y" $(LINUXDIR)/.config > /dev/null; then \
					if [ -d $(VENDDIR)/$(CONF)/province_cu ]; then \
						mkdir -p config/province ;\
						cp -raf $(VENDDIR)/$(CONF)/province_cu/* config/province/. ;\
					fi;\
				else \
					if [ -d $(VENDDIR)/$(CONF)/$(PROVINCE_NAME) ]; then \
						mkdir -p config/province ;\
						cp -raf $(VENDDIR)/$(CONF)/$(PROVINCE_NAME)/* config/province/. ;\
					fi;\
				fi;\
        		fi;\
               	fi;\
		[ -f $(BOOTLOADER_PATH)/Flash_and_DDR_Support.txt ] && rm  $(BOOTLOADER_PATH)/Flash_and_DDR_Support.txt;  \
		[ -f $(VENDDIR)/$(CONF)/$*/Flash_and_DDR_Support.txt ] && cp  $(VENDDIR)/$(CONF)/$*/Flash_and_DDR_Support.txt  $(BOOTLOADER_PATH)/ ; \
		[ -f  $(VENDDIR)/$(CONF)/$*/config_ca_ne  ]  && cp $(VENDDIR)/$(CONF)/$*/config_ca_ne ca_packages/ca-network-engine/1.0-r0/ca-network-engine-1.0/configs/g3lite_defconfig ; \
		if [ "$(LINUXDIR)" = "linux-4.4.x" ];then  \
			echo "Configuration44 $* loaded." ; \
			touch preconfig44_$*_loaded ; \
		elif [ "$(LINUXDIR)" = "linux-4.14.x" ];then  \
			echo "Configuration414 $* loaded." ; \
			touch preconfig414_$*_loaded ; \
		elif [ "$(LINUXDIR)" = "linux-5.10.x" ];then  \
			echo "Configuration510 $* loaded." ; \
			touch preconfig510_$*_loaded ; \
		else \
			echo "Configuration318 $* loaded." ; \
			touch preconfig318_$*_loaded ; \
		fi;\
	else \
		echo "No $(CONFIG_PRODUCT) configuration found" ; \
	fi
	@# FIXME Workaround for those who can not access VOIP
	@if [ ! -d $(ROOTDIR)/rtk_voip/kernel ]; then \
		mkdir -p $(ROOTDIR)/rtk_voip/kernel;	\
		touch $(ROOTDIR)/rtk_voip/kernel/Kconfig; \
		mkdir -p $(ROOTDIR)/rtk_voip/aipc_char;	\
		touch $(ROOTDIR)/rtk_voip/aipc_char/Kconfig; \
	fi
	@# Checkout src for 98D
###	@$(MAKE) -f Makefile.98D preconfig44_$*
	@if egrep "^CONFIG_PRODUCT=luna_ap*" $(ROOTDIR)/.config > /dev/null; then \
		if [ -d $(KDIR)/drivers/net/wireless/realtek/rtl8192cd_3.4 ]; then \
			mv $(KDIR)/drivers/net/wireless/realtek/rtl8192cd $(KDIR)/drivers/net/wireless/realtek/rtl8192cd-`date +%Y%m%d_%H%M%S`; \
			ln -sf $(KDIR)/drivers/net/wireless/realtek/rtl8192cd_3.4 $(KDIR)/drivers/net/wireless/realtek/rtl8192cd; \
		fi; \
		if [ ! -d $(KDIR)/drivers/net/wireless/rtl8192cd ]; then \
			mkdir -p $(KDIR)/drivers/net/wireless/rtl8192cd; \
			ln -sf $(KDIR)/drivers/net/wireless/realtek/rtl8192cd/Kconfig-usb $(KDIR)/drivers/net/wireless/rtl8192cd/Kconfig-usb; \
		fi; \
	fi;


preconfig510_%:
	echo "CONFIG_LINUXDIR=linux-5.10.x" >> .config
	@if [ -f *_loaded ] &&  [ ! -f $@_loaded ] ; then \
		$(MAKE) clean; \
		make -C bootloader clean; \
		echo "Change to $@. Make distclean first!" ; \
		rm *_loaded; \
	fi
	make preconfig_$*;
	sed -i "s|/asdk-10.2.0.*|"$(realpath ./)"/asdk-10.2.0-a55-EL-5.10-g2.30-a96-210331\"|" .config

preconfig414_%:
	echo "CONFIG_LINUXDIR=linux-4.14.x" >> .config
	@if [ -f *_loaded ] &&  [ ! -f $@_loaded ] ; then \
		$(MAKE) clean; \
		make -C bootloader clean; \
		echo "Change to $@. Make distclean first!" ; \
		rm *_loaded; \
	fi
	make preconfig_$*;

preconfig44_%: 
	echo "CONFIG_LINUXDIR=linux-4.4.x" >> .config
	@if [ -f *_loaded ] &&  [ ! -f $@_loaded ] ; then \
		$(MAKE) clean; \
		make -C bootloader clean; \
		echo "Change to $@. Make distclean first!" ; \
		rm *_loaded; \
	fi
	make preconfig_$*;
	sed -i "s|/msdk-4.8.5.*|"$(realpath ./)"/msdk-4.8.5-mips-EB-4.4-g2.23-m32ut-190619-cmcc\"|" .config
	#sed -i 's|/msdk-4.8.5.*|'$(realpath ./)'/msdk-4.8.5-mips-EB-4.4-g2.23-m32ut-190619-cmcc\"|' .config
	@if [ A"$(shell echo $@ | grep "_spi_nor_")" != "A" ]; then \
		echo "...............flash type is NOR."; \
		cp oemconfig/linux-nor.config  oemconfig/linux.config; \
	else \
		echo "...............flash type is NAND."; \
		cp oemconfig/linux-nand.config  oemconfig/linux.config; \
	fi

preconfig318_%: 
	echo "CONFIG_LINUXDIR=linux-3.18.x" >> .config
	@if [ -f *_loaded ] &&  [ ! -f $@_loaded ] ; then \
		$(MAKE) clean; \
		make -C bootloader clean; \
		echo "Change to $@. Make distclean first!" ; \
		rm *_loaded; \
	fi
	if [ -f $(ROOTDIR)/linux-3.18.x/drivers/net/wireless/rtl8192cd/Kconfig ]; then \
		sed -i "s/.*source.*\/realtek\/rtl8192cd\/Kconfig-usb\"/\source \"drivers\/net\/wireless\/rtl8192cd\/Kconfig-usb\"/g" $(ROOTDIR)/linux-3.18.x/drivers/net/wireless/rtl8192cd/Kconfig; \
	fi
	make preconfig_$*;


### Build Package Directiory #####################
###### Linux #######
linux_dtbs:
	$(MAKEARCH_KERNEL) -j $(HOST_CPU_NUM) -C $(LINUXDIR) dtbs || exit 1
	$(MAKE) -f Makefile.image dtbimg

backports:
	. $(LINUX_CONFIG); if [ "$$CONFIG_RTL8852AE_BACKPORTS" = "y" ]; then \
		unset CC && \
		[ -d $(ROOTDIR)/include/backport ] || mkdir -p $(ROOTDIR)/include/backport; \
		ln -fs $(BACKPORT_DIR)/backport-include/backport/autoconf.h include/backport/autoconf.h; \
		if [ ! -f $(BACKPORT_DIR)/$(BACKPORT_LOCAL_SYM_UPDATED) ]; then \
			echo -n "Update backports local-symbols..."; \
			cp $(BACKPORT_DIR)/$(BACKPORT_LOCAL_SYM) $(BACKPORT_DIR)/$(BACKPORT_LOCAL_SYM_OLD); \
			cd $(BACKPORT_DIR) && sed -n "/^[\t| ]*config/p" ./drivers/net/wireless/realtek/g6_wifi_driver/Kconfig > .kconf.tmp && sed -i "s/^[\t| ]*config /CPTCFG_/" .kconf.tmp && sed -i "s/$$/&=/g" .kconf.tmp && cat .kconf.tmp >> local-symbols && rm -f .kconf.tmp && touch $(BACKPORT_LOCAL_SYM_UPDATED) && cd - ;\
			echo "done."; \
		fi && \
		$(MAKEARCH_KERNEL) KLIB=$(BACKPORT_DIR) KLIB_BUILD=$(ROOTDIR)/$(LINUXDIR) -j $(HOST_CPU_NUM) -C $(BACKPORT_DIR) || exit $$?; \
	fi

.PHONY: backports_romfs
backports_romfs:
	. $(LINUX_CONFIG); if [ "$$CONFIG_RTK_DEV_AP" = "y" ]; then \
		. $(LINUX_CONFIG); if [ "$$CONFIG_RTL8852AE_BACKPORTS" = "y" ]; then \
			if [ $(shell egrep "^CONFIG_FILE_FWIMG" $(WIFI_DRIVER_DIR)/Makefile | sed 's/[[:blank:]]//g; s/CONFIG_FILE_FWIMG=//g') = "y" ]; then \
				[ -f $(FW_IMG) ] && cp $(FW_IMG) $(ROMFSDIR)/etc/conf/rtl8852ae/; \
			fi; \
		fi; \
	fi

.PHONY: backports_driver_check
backports_driver_check:
	. $(LINUX_CONFIG); if [ "$$CONFIG_RTL8852AE_BACKPORTS" = "y" ]; then \
		. $(LINUX_CONFIG); if [ "$$CONFIG_RTK_DEV_AP" = "y" ]; then \
			sed -i "s/.*source.*"$$BACKPORT_RTL8192CD_NAME"\/Kconfig\"/\source \"drivers\/net\/wireless\/realtek\/"$$BACKPORT_RTL8192FE_NAME"\/Kconfig\"/g" $(BACKPORT_DIR)/drivers/net/wireless/realtek/Kconfig; \
			sed -i "s/"$$BACKPORT_RTL8192CD_NAME".*menuconfig/"$$BACKPORT_RTL8192FE_NAME" menuconfig/g" $(BACKPORT_DIR)/Makefile; \
			sed -i "s/"$$BACKPORT_RTL8192CD_NAME"/"$$BACKPORT_RTL8192FE_NAME"/g" $(BACKPORT_DIR)/Makefile.real; \
			sed -i "s/"$$BACKPORT_RTL8192CD_NAME"/"$$BACKPORT_RTL8192FE_NAME"/g" $(BACKPORT_DIR)/drivers/net/wireless/realtek/Makefile; \
		else \
			sed -i "s/.*source.*"$$BACKPORT_RTL8192FE_NAME"\/Kconfig\"/\source \"drivers\/net\/wireless\/realtek\/"$$BACKPORT_RTL8192CD_NAME"\/Kconfig\"/g" $(BACKPORT_DIR)/drivers/net/wireless/realtek/Kconfig; \
			sed -i "s/"$$BACKPORT_RTL8192FE_NAME".*menuconfig/"$$BACKPORT_RTL8192CD_NAME" menuconfig/g" $(BACKPORT_DIR)/Makefile; \
			sed -i "s/"$$BACKPORT_RTL8192FE_NAME"/"$$BACKPORT_RTL8192CD_NAME"/g" $(BACKPORT_DIR)/Makefile.real; \
			sed -i "s/"$$BACKPORT_RTL8192FE_NAME"/"$$BACKPORT_RTL8192CD_NAME"/g" $(BACKPORT_DIR)/drivers/net/wireless/realtek/Makefile; \
		fi; \
	fi

.PHONY: linux
linux linux_only:
	. $(LINUX_CONFIG); if [ "$$CONFIG_MLO_KERNEL_PATCH" = "y" ]; then \
                rm $(LINUXDIR)/include/linux/ieee80211.h; \
                rm $(LINUXDIR)/include/linux/rfkill.h; \
                rm $(LINUXDIR)/include/linux/stddef.h; \
                rm $(LINUXDIR)/include/linux/wireless.h; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/include/linux/ieee80211.h $(LINUXDIR)/include/linux/ieee80211.h; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/include/linux/rfkill.h $(LINUXDIR)/include/linux/rfkill.h; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/include/linux/stddef.h $(LINUXDIR)/include/linux/stddef.h; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/include/linux/wireless.h $(LINUXDIR)/include/linux/wireless.h; \
                rm $(LINUXDIR)/include/net/cfg80211.h; \
                rm $(LINUXDIR)/include/net/genetlink.h; \
                rm $(LINUXDIR)/include/net/ieee80211_radiotap.h; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/include/net/cfg80211.h $(LINUXDIR)/include/net/cfg80211.h; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/include/net/genetlink.h $(LINUXDIR)/include/net/genetlink.h; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/include/net/ieee80211_radiotap.h $(LINUXDIR)/include/net/ieee80211_radiotap.h; \
                rm $(LINUXDIR)/include/uapi/linux/genetlink.h; \
                rm $(LINUXDIR)/include/uapi/linux/nl80211.h; \
                rm $(LINUXDIR)/include/uapi/linux/rfkill.h; \
                rm $(LINUXDIR)/include/uapi/linux/stddef.h; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/include/uapi/linux/genetlink.h $(LINUXDIR)/include/uapi/linux/genetlink.h; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/include/uapi/linux/nl80211.h $(LINUXDIR)/include/uapi/linux/nl80211.h; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/include/uapi/linux/rfkill.h $(LINUXDIR)/include/uapi/linux/rfkill.h; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/include/uapi/linux/stddef.h $(LINUXDIR)/include/uapi/linux/stddef.h; \
                rm -rf $(LINUXDIR)/net/wireless; \
                rm -rf $(LINUXDIR)/net/rfkill; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/net/wireless $(LINUXDIR)/net/wireless; \
                ln -sf $(ROOTDIR)/mlo_kernel_patch/patch/net/rfkill $(LINUXDIR)/net/rfkill; \
        fi

	[ -d $(ROMFSDIR) ] || mkdir -p $(ROMFSDIR)
	[ -d $(TMPFSDIR) ] || mkdir -p $(TMPFSDIR)
	$(MAKEARCH_KERNEL) -j $(HOST_CPU_NUM) -C $(LINUXDIR) all $(LINUXTARGET) || exit 1
	[ -d $(ROOTDIR)/include ] || mkdir -p $(ROOTDIR)/include
	[ -d $(ROOTDIR)/include/linux ] || mkdir -p $(ROOTDIR)/include/linux
	ln -fs $(ROOTDIR)/$(LINUXDIR)/include/generated/autoconf.h include/linux/autoconf.h
	ln -fs $(ROOTDIR)/$(LINUXDIR)/include/generated/autoconf.h include/linux/config.h
	$(MAKEARCH_KERNEL) -C $(LINUXDIR) INSTALL_HDR_PATH=$(ROOTDIR)/$(LINUXDIR)/user_headers headers_install

linux_voip:
	if egrep "^CONFIG_RTK_VOIP_COMMON_ARCH=y" $(LINUXDIR)/.config > /dev/null; then	\
		echo $(LINUXDIR);	\
		cp $(LINUXDIR)/.config $(LINUXDIR)/.config_pe;											\
		cp $(LINUXDIR)/include/generated/autoconf.h $(LINUXDIR)/include/generated/autoconf.h_pe;	\
		sed -i "s/# CONFIG_RTK_VOIP_CODEC_ON_ARM is not set/CONFIG_RTK_VOIP_CODEC_ON_ARM=y/g" $(LINUXDIR)/.config;			\
		sed -i "/CONFIG_RTK_VOIP_CODEC_ON_ARM=y/a CONFIG_RTK_VOIP_CODEC_OBJECT_KERNEL_VERSION_5_10=y" $(LINUXDIR)/.config;	\
		sed -i "s/# CONFIG_RTK_VOIP_STANDALONE_ARCH is not set/CONFIG_RTK_VOIP_STANDALONE_ARCH=y/g" $(LINUXDIR)/.config;		\
		sed -i "s/CONFIG_RTK_VOIP_COPROCESS_DSP=y/# CONFIG_RTK_VOIP_COPROCESS_DSP is not set/g" $(LINUXDIR)/.config;			\
		sed -i "s/CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST=y//g" $(LINUXDIR)/.config;												\
		sed -i "s/# CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP is not set/# CONFIG_RTK_VOIP_IPC_ARCH is not set/g" $(LINUXDIR)/.config;	\
		sed -i "s/CONFIG_RTK_VOIP_IPC_ARCH=y/# CONFIG_RTK_VOIP_IPC_ARCH is not set/g" $(LINUXDIR)/.config;					\
		sed -i "s/CONFIG_RTK_VOIP_COPROCESS_DSP_IS_HOST=y//g" $(LINUXDIR)/.config;											\
		sed -i "s/CONFIG_RTK_VOIP_DSP_DEVICE_NR=1//g" $(LINUXDIR)/.config;													\
		sed -i "/CONFIG_RTK_VOIP_DRIVERS_PCM_LAW=y/i CONFIG_RTK_VOIP_DRIVERS_PCM=y" $(LINUXDIR)/.config;						\
		sed -i "/CONFIG_RTK_VOIP_DRIVERS_PCM_LAW=y/i CONFIG_RTK_VOIP_BUS_PCM_CH_NUM=4" $(LINUXDIR)/.config;					\
		sed -i "/CONFIG_RTK_VOIP_CON_CH_NUM=1/a CONFIG_RTK_VOIP_CON_BUSFIFO_LECBUF=y" $(LINUXDIR)/.config;					\
		sed -i "/CONFIG_RTK_VOIP_CON_BUSFIFO_LECBUF=y/a CONFIG_RTK_VOIP_CON_POLLING_PROCESS=y" $(LINUXDIR)/.config;			\
		sed -i "s/# CONFIG_RTL8686_IPC_SINGLE_CPU is not set/CONFIG_RTL8686_IPC_SINGLE_CPU=y/g" $(LINUXDIR)/.config;			\
		sed -i "s/CONFIG_RTL8686_IPC_DEBUG_MESSAGE=y/# CONFIG_RTL8686_IPC_DEBUG_MESSAGE is not set/g" $(LINUXDIR)/.config;	\
		sed -i "s/# CONFIG_RTL8686_SHM_NOTIFY is not set//g" $(LINUXDIR)/.config;											\
		sed -i "s/# CONFIG_AIPC_DSP_WATCHDOG_THREAD is not set//g" $(LINUXDIR)/.config;										\
		sed -i "s/CONFIG_RTL8686_IPC_DSP_CONSOLE=y/# CONFIG_RTL8686_IPC_DSP_CONSOLE is not set/g" $(LINUXDIR)/.config;		\
		sed -i "s/CONFIG_RTL8686_IPC_IDX_USE_DRAM=y/# CONFIG_RTL8686_IPC_IDX_USE_DRAM is not set/g" $(LINUXDIR)/.config;		\
		sed -i "s/CONFIG_RTL8686_IPC_DSP_BOOTING_ADDRESS=0x88000000/# CONFIG_RTL8686_IPC_IDX_USE_SRAM is not set/g" $(LINUXDIR)/.config;	\
		sed -i "s/CONFIG_RTL8686_IPC_DSP_CONFIG=y/# CONFIG_RTL8686_IPC_DSP_CONFIG is not set/g" $(LINUXDIR)/.config;	\
		sed -i "s/CONFIG_RTL8686_IPC_DSP_CONFIG_CON_CH_NUM=1//g" $(LINUXDIR)/.config;		\
		sed -i "s/CONFIG_RTL8686_IPC_DSP_CONFIG_MIRROR_SLIC_NR=1//g" $(LINUXDIR)/.config;	\
		sed -i "s/CONFIG_CORTINA_IPC_PCM_CFG=y/# CONFIG_CORTINA_IPC_PCM_CFG is not set/g" $(LINUXDIR)/.config;	\
		sed -i "s/CONFIG_RTL8686_IPC_RECORD_DSP_LOG=y/# CONFIG_RTL8686_IPC_RECORD_DSP_LOG is not set/g" $(LINUXDIR)/.config;	\
		cp $(LINUXDIR)/include/config/auto.conf  $(LINUXDIR)/include/config/auto.conf_pe;	\
		sed -i "s/# CONFIG_RTK_VOIP_CODEC_ON_ARM is not set/CONFIG_RTK_VOIP_CODEC_ON_ARM=y/g" $(LINUXDIR)/include/config/auto.conf;			\
		sed -i "/CONFIG_RTK_VOIP_CODEC_ON_ARM=y/a CONFIG_RTK_VOIP_CODEC_OBJECT_KERNEL_VERSION_5_10=y" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/# CONFIG_RTK_VOIP_STANDALONE_ARCH is not set/CONFIG_RTK_VOIP_STANDALONE_ARCH=y/g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_RTK_VOIP_COPROCESS_DSP=y/# CONFIG_RTK_VOIP_COPROCESS_DSP is not set/g" $(LINUXDIR)/include/config/auto.conf;		\
		sed -i "s/CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST=y//g" $(LINUXDIR)/include/config/auto.conf;												\
		sed -i "s/# CONFIG_RTK_VOIP_IPC_ARCH_IS_DSP is not set/# CONFIG_RTK_VOIP_IPC_ARCH is not set/g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_RTK_VOIP_IPC_ARCH=y/# CONFIG_RTK_VOIP_IPC_ARCH is not set/g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_RTK_VOIP_COPROCESS_DSP_IS_HOST=y//g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_RTK_VOIP_DSP_DEVICE_NR=1//g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "/CONFIG_RTK_VOIP_DRIVERS_PCM_LAW=y/i CONFIG_RTK_VOIP_DRIVERS_PCM=y" $(LINUXDIR)/include/config/auto.conf;			\
		sed -i "/CONFIG_RTK_VOIP_DRIVERS_PCM_LAW=y/i CONFIG_RTK_VOIP_BUS_PCM_CH_NUM=4" $(LINUXDIR)/include/config/auto.conf;			\
		sed -i "/CONFIG_RTK_VOIP_CON_CH_NUM=1/a CONFIG_RTK_VOIP_CON_BUSFIFO_LECBUF=y" $(LINUXDIR)/include/config/auto.conf;			\
		sed -i "/CONFIG_RTK_VOIP_CON_BUSFIFO_LECBUF=y/a CONFIG_RTK_VOIP_CON_POLLING_PROCESS=y" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/# CONFIG_RTL8686_IPC_SINGLE_CPU is not set/CONFIG_RTL8686_IPC_SINGLE_CPU=y/g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_RTL8686_IPC_DEBUG_MESSAGE=y/# CONFIG_RTL8686_IPC_DEBUG_MESSAGE is not set/g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/# CONFIG_RTL8686_SHM_NOTIFY is not set//g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/# CONFIG_AIPC_DSP_WATCHDOG_THREAD is not set//g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_RTL8686_IPC_DSP_CONSOLE=y/# CONFIG_RTL8686_IPC_DSP_CONSOLE is not set/g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_RTL8686_IPC_IDX_USE_DRAM=y/# CONFIG_RTL8686_IPC_IDX_USE_DRAM is not set/g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_RTL8686_IPC_DSP_BOOTING_ADDRESS=0x88000000/# CONFIG_RTL8686_IPC_IDX_USE_SRAM is not set/g" $(LINUXDIR)/.config;	\
		sed -i "s/CONFIG_RTL8686_IPC_DSP_CONFIG=y/# CONFIG_RTL8686_IPC_DSP_CONFIG is not set/g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_RTL8686_IPC_DSP_CONFIG_CON_CH_NUM=1//g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_RTL8686_IPC_DSP_CONFIG_MIRROR_SLIC_NR=1//g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_CORTINA_IPC_PCM_CFG=y/# CONFIG_CORTINA_IPC_PCM_CFG is not set/g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/CONFIG_RTL8686_IPC_RECORD_DSP_LOG=y/# CONFIG_RTL8686_IPC_RECORD_DSP_LOG is not set/g" $(LINUXDIR)/include/config/auto.conf;	\
		sed -i "s/#define CONFIG_RTK_VOIP_IPC_ARCH 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTK_VOIP_COPROCESS_DSP 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTK_VOIP_ILBC 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTL8686_IPC_IDX_USE_DRAM 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTL8686_IPC_DSP_BOOTING_ADDRESS 0x88000000//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTL8686_IPC_DSP_CONFIG_MIRROR_SLIC_NR 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTL8686_IPC_DSP_CONSOLE 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_CORTINA_IPC_PCM_CFG 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTL8686_IPC_DEBUG_MESSAGE 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTK_VOIP_COPROCESS_DSP_IS_HOST 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTK_VOIP_IPC_ARCH_IS_HOST 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTK_VOIP_DRIVERS_SLIC_LE9641_OWNER_ALL_DSP 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTK_VOIP_DRIVERS_SLIC_DXS101_OWNER_ALL_DSP 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTL8686_IPC_RECORD_DSP_LOG 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTL8686_IPC_DSP_CONFIG_CON_CH_NUM 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTK_VOIP_DSP_DEVICE_NR 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		sed -i "s/#define CONFIG_RTL8686_IPC_DSP_CONFIG 1//g" $(LINUXDIR)/include/generated/autoconf.h;	\
		echo "#define CONFIG_RTK_VOIP_BUS_PCM_CH_NUM 4" >> $(LINUXDIR)/include/generated/autoconf.h;	\
		echo "#define CONFIG_RTK_VOIP_DRIVERS_PCM 1"  >> $(LINUXDIR)/include/generated/autoconf.h;	\
		echo "#define CONFIG_RTK_VOIP_CON_BUSFIFO_LECBUF 1"  >> $(LINUXDIR)/include/generated/autoconf.h;	\
		echo "#define CONFIG_RTK_VOIP_CON_POLLING_PROCESS 1"  >> $(LINUXDIR)/include/generated/autoconf.h;	\
		echo "#define CONFIG_RTK_VOIP_CODEC_ON_ARM 1"  >> $(LINUXDIR)/include/generated/autoconf.h;	\
		echo "#define CONFIG_RTK_VOIP_STANDALONE_ARCH 1"  >> $(LINUXDIR)/include/generated/autoconf.h;	\
		echo "#define CONFIG_RTK_VOIP_CODEC_OBJECT_KERNEL_VERSION_5_10 1"  >> $(LINUXDIR)/include/generated/autoconf.h;	\
		echo "#define CONFIG_RTL8686_IPC_SINGLE_CPU 1"  >> $(LINUXDIR)/include/generated/autoconf.h;	\
		cp  $(LINUXDIR)/rtk_voip/module.mak $(LINUXDIR)/rtk_voip/module.mak_pe;	\
		sed -i "s/voip_kernel/voip_kernel_arm/g" $(LINUXDIR)/rtk_voip/module.mak;	\
		$(MAKEARCH_KERNEL) -C $(LINUXDIR) M=rtk_voip;	\
		cp $(LINUXDIR)/.config_pe $(LINUXDIR)/.config;	\
		cp $(LINUXDIR)/include/config/auto.conf_pe   $(LINUXDIR)/include/config/auto.conf;	\
		cp $(LINUXDIR)/include/generated/autoconf.h_pe $(LINUXDIR)/include/generated/autoconf.h;	\
		cp $(LINUXDIR)/rtk_voip/module.mak_pe $(LINUXDIR)/rtk_voip/module.mak;	\
	fi

.PHONY: voip_prepare
voip_prepare:
	@if [ -d $(ROOTDIR)/rtk_voip ] ; then \
		cd $(LINUXDIR)/drivers/char ; ln -fsn ../../../rtk_voip/aipc_char  aipc     ; cd - ; \
		cd $(LINUXDIR)              ; ln -fsn ../rtk_voip/kernel           rtk_voip ; cd - ; \
		cd $(ROOTDIR)/user          ; ln -fsn ../rtk_voip/aipc_util        aipc_util; cd - ; \
	else \
		cd $(LINUXDIR)/drivers/char ; ln -fsn ../../dummy aipc ; \
		cd $(LINUXDIR)              ; ln -fsn dummy rtk_voip   ; \
	fi


linux_clean:
	$(MAKEARCH_KERNEL) -j $(HOST_CPU_NUM) -C $(LINUXDIR) clean

linux_distclean:
	$(MAKEARCH_KERNEL) -j $(HOST_CPU_NUM) -C $(LINUXDIR) distclean

.PHONY: linux_install

	
linux_menuconfig: voip_prepare
	$(MAKEARCH_KERNEL) -C $(LINUXDIR) menuconfig
	$(MAKEARCH_KERNEL) -C $(LINUXDIR) modules_prepare
###### CA_PACKAGES #######
packages_ca:
ifneq ($(wildcard ./$(CA_KMOD)/.*),)
	$(MAKEARCH) -C $(CA_KMOD) all
else
	@echo "No $(CA_KMOD)"
endif

%_ca:
		if [ -d "$(@:_ca=)" ]; then \
			if [ -n "$(findstring ca_packages/,$@)" ]; then \
				$(MAKEARCH) -C ca_packages $(@:ca_packages/%=%); \
			else \
				$(MAKEARCH) -C $(@:_ca=); \
				fi; \
		fi	
	
	
packages_ca_clean:
ifneq ($(wildcard ./$(CA_KMOD)/.*),)
	$(MAKEARCH) -C $(CA_KMOD) clean
endif

packages_ca_install:
ifneq ($(wildcard ./$(CA_KMOD)/.*),)
	$(MAKEARCH) -C $(CA_KMOD) modules_install
endif

packages_user: 
	@if egrep "^CONFIG_RTK_USE_CA_IROS=y" $(KDIR)/.config > /dev/null; then \
	touch $(LIBNL_3_2_25_DIR)/configure.ac $(LIBNL_3_2_25_DIR)/aclocal.m4 $(LIBNL_3_2_25_DIR)/configure $(LIBNL_3_2_25_DIR)/Makefile.am $(LIBNL_3_2_25_DIR)/Makefile.in; \
	$(MAKEARCH) -C $(CA_USER) all; \
	fi

packages_user_clean:
	@if egrep "^CONFIG_RTK_USE_CA_IROS=y" $(KDIR)/.config > /dev/null; then \
	$(MAKEARCH) -C $(CA_USER) clean; \
	fi

packages_user_kmod_install:
	@if egrep "^CONFIG_RTK_USE_CA_IROS=y" $(KDIR)/.config > /dev/null; then \
	$(MAKEARCH) -C $(CA_USER) modules_install ; \
	fi

##sysroot-destdir: CA's libs, headers , binaries.....
sysroot_install:
	rm -rf ./sysroot-destdir/home/* 
	[ ! -d ./sysroot-destdir ]		|| cp -arf ./sysroot-destdir/*   $(ROMFSDIR)
	[ ! -d $(ROMFSDIR)/usr/lib ]		|| chmod -R 755  $(ROMFSDIR)/usr/lib
	[ ! -d $(ROMFSDIR)/usr/lib ]		|| rm -rf  $(ROMFSDIR)/usr/lib/*.a
	[ ! -d $(ROMFSDIR)/usr/include ]	|| rm -rf $(ROMFSDIR)/usr/include/*
	[ ! -d $(ROMFSDIR)/usr/share/doc ]	|| rm -rf $(ROMFSDIR)/usr/share/doc
	[ ! -d $(ROMFSDIR)/usr/share/man ]	|| rm -rf $(ROMFSDIR)/usr/share/man/*
	[ ! -d $(ROMFSDIR)/usr/share/terminfo ]	|| rm -rf $(ROMFSDIR)/usr/share/terminfo
	cd $(ROMFSDIR) && rm -rf config/ && ln -fsn ./var/config/ config; 

################## Menuconfig	  #######
############################################################################

#
# Config stuff,  we recall ourselves to load the new config.arch before
# running the kernel and other config scripts
#

.PHONY: config.tk config.in

config.in:
	config/mkconfig > config.in

config.tk: config.in
	$(MAKE) -C $(SCRIPTSDIR) tkparse
	ARCH=dummy $(SCRIPTSDIR)/tkparse < config.in > config.tmp
	@if [ -f /usr/local/bin/wish ];	then \
		echo '#!'"/usr/local/bin/wish -f" > config.tk; \
	else \
		echo '#!'"/usr/bin/wish -f" > config.tk; \
	fi
	cat $(SCRIPTSDIR)/header.tk >> ./config.tk
	cat config.tmp >> config.tk
	rm -f config.tmp
	echo "set defaults \"/dev/null\"" >> config.tk
	echo "set help_file \"config/Configure.help\"" >> config.tk
	cat $(SCRIPTSDIR)/tail.tk >> config.tk
	chmod 755 config.tk

.PHONY: xconfig
xconfig: config.tk
	@wish -f config.tk
	@if [ ! -f .config ]; then \
		echo; \
		echo "You have not saved your config, please re-run make config"; \
		echo; \
		exit 1; \
	 fi
	@config/setconfig defaults
	@if egrep "^CONFIG_DEFAULTS_KERNEL=y" .config > /dev/null; then \
		$(MAKE) linux_xconfig; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_MODULES=y" .config > /dev/null; then \
		$(MAKE) modules_xconfig; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_VENDOR=y" .config > /dev/null; then \
		$(MAKE) config_xconfig; \
	 fi
	@config/setconfig final

.PHONY: config
config: config.in
	@HELP_FILE=config/Configure.help \
		$(CONFIG_SHELL) $(SCRIPTSDIR)/Configure config.in
	@config/setconfig defaults
	@if egrep "^CONFIG_DEFAULTS_KERNEL=y" .config > /dev/null; then \
		$(MAKE) linux_config; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_MODULES=y" .config > /dev/null; then \
		$(MAKE) modules_config; \
	 fi
	@if egrep "^CONFIG_DEFAULTS_VENDOR=y" .config > /dev/null; then \
		$(MAKE) config_config; \
	 fi
	@config/setconfig final



##### menuconfig
config_bootloader:
	@config/setBootloaderConf NewConf

config_menuconfig:
#	$(MAKE) -C config menuconfig	
	$(MAKE) -f Makefile.sub_menuconf menuconfig_vendor

define fix_rtk_sdk_compile_shit
	[ -d $(APPDIR)/diag_rtk/bin ] || mkdir $(APPDIR)/diag_rtk/bin;
	[ -d $(APPDIR)/pkt_redirect/../lib ] || mkdir $(APPDIR)/pkt_redirect/../lib;
	[ -d $(APPDIR)/epon_oam_v1/bin ] || mkdir $(APPDIR)/epon_oam_v1/bin;
	[ -d $(APPDIR)/pon_detect/bin ] || mkdir $(APPDIR)/pon_detect/bin;
	[ -d $(APPDIR)/bin ] || mkdir $(APPDIR)/bin;
	[ -d $(APPDIR)/europa/bin ] || mkdir $(APPDIR)/europa/bin;
endef

.PHONY: menuconfig
menuconfig: 
	$(MAKE) menuconfig_phase1
	$(MAKE) menuconfig_phase2
	$(MAKE) menuconfig_phase3
	@config/setconfig final
	$(call fix_rtk_sdk_compile_shit)

.PHONY: menuconfig_phase1
menuconfig_phase1: config.in voip_prepare
	$(MAKE) -C $(SCRIPTSDIR)/lxdialog all
	@HELP_FILE=config/Configure.help \
		$(CONFIG_SHELL) $(SCRIPTSDIR)/Menuconfig config.in
	@if [ ! -f .config ]; then \
		echo; \
		echo "You have not saved your config, please re-run make config"; \
		echo; \
		exit 1; \
	 fi
	. $(LINUX_CONFIG); if [ "$$CONFIG_RTK_DEV_AP" = "y" ]; then \
		if egrep "CONFIG_RTL8852AE_BACKPORTS=y" $(LINUX_CONFIG) > /dev/null; then \
			sed -i "s/.*source.*g6_wifi_driver\/Kconfig\"/source \"drivers\/net\/wireless\/realtek\/g6_wifi_driver\/Kconfig\"/g" $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/Kconfig; \
			sed -i "s/.*source.*rtl8192cd\/Kconfig\"/\#source \"drivers\/net\/wireless\/realtek\/rtl8192cd\/Kconfig\"/g" $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/Kconfig; \
		else \
			sed -i "s/.*source.*g6_wifi_driver\/Kconfig\"/\#source \"drivers\/net\/wireless\/realtek\/g6_wifi_driver\/Kconfig\"/g" $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/Kconfig; \
			sed -i "s/.*source.*rtl8192cd\/Kconfig\"/source \"drivers\/net\/wireless\/realtek\/rtl8192cd\/Kconfig\"/g" $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/Kconfig; \
		fi; \
	fi
	@config/setconfig defaults
ifeq ($(CONFIG_PRODUCT), luna)
ifneq (,$(filter asdk aarch,$(findstring asdk,$(CONFIG_RSDK_DIR)) $(findstring aarch,$(CONFIG_RSDK_DIR))))
	@config/setBootloaderConf NewConf
	@config/setBootloaderConf UBIFSConf
else
	@if [ -f $(BOOTLOADER_PATH)/Flash_and_DDR_Support.txt ] ; then  \
		echo "have Flash_and_DDR_Support.txt" \
		[ ! -f $(BOOTLOADER_PATH)/.config ] && echo 'echo empty .confg' > $(BOOTLOADER_PATH)/.config; \
		config/setBootloaderConf UBIFSConf ;\
	fi
endif
endif
ifeq ($(CONFIG_PRODUCT), luna_xdsl)
ifeq ($(findstring asdk,$(CONFIG_RSDK_DIR)),asdk)
	@config/setBootloaderConf NewConf
endif
endif
ifeq ($(CONFIG_PRODUCT),luna_ap_arm)  ##98f
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
	echo "do menuconfig_phase1 - 98F.";
	@config/setBootloaderConf NewConf
else
	echo "do menuconfig_phase1 - 98XA.";
	@config/setBootloaderConf NewConf
	@config/setBootloaderConf UBIFSConf
endif
endif
ifeq ($(CONFIG_PRODUCT),luna_ap_mips)  ##98D
	@config/setBootloaderConf NewConf
endif

.PHONY: menuconfig_phase2
menuconfig_phase2: 
	@if egrep "^CONFIG_DEFAULTS_KERNEL=y" .config > /dev/null; then \
		$(MAKE) linux_menuconfig; \
		$(MAKE) backports_menuconfig; \
	fi
	@if egrep "^CONFIG_DEFAULTS_MODULES=y" .config > /dev/null; then \
		$(MAKE) modules_menuconfig; \
	fi
	@if egrep "^CONFIG_DEFAULTS_VENDOR=y" .config > /dev/null; then \
		$(MAKE) -f Makefile.sub_menuconf menuconfig_vendor; \
	fi	

	
.PHONY: menuconfig_phase3

menuconfig_phase3:
	@if egrep "^CONFIG_MCONF_BZBOX=y" .config > /dev/null; then \
		$(MAKE) -f Makefile.sub_menuconf menuconfig_busybox; \
	fi
	@if egrep "^CONFIG_USE_PRELOADER_PARAMETERS=y" .config > /dev/null; then \
		$(MAKE) soc_h_prepare; \
	fi

.PHONY: backports_menuconfig

backports_menuconfig: backports_driver_check
	. $(LINUX_CONFIG); if [ "$$CONFIG_RTL8852AE_BACKPORTS" = "y" ]; then \
		unset CC && \
		cd $(BACKPORT_DIR) && $(MAKEARCH_KERNEL) KLIB=$(BACKPORT_DIR) KLIB_BUILD=$(ROOTDIR)/$(LINUXDIR) menuconfig; \
	fi

bootloader_menuconfig:
	$(MAKE) -C $(BOOTLOADER_PATH) menuconfig

bootloader_clean:
	$(MAKE) -C $(BOOTLOADER_PATH) clean

###########################################################################	
### Build

bootloader_gpt_image bootloader_image:
ifneq ($(CONFIG_UBOOT_VER),"")
	$(MAKE) -C $(BOOTLOADER_PATH) all
ifeq ($(CONFIG_SDK_2TH_BL_GEN), y)
	make -f Makefile.image bimage2
	@echo "Build Second bootloader image"
	@echo "First, make bootloader_clean"
	make bootloader_clean
	$(MAKE) -C $(BOOTLOADER_PATH) all  BOOTLOADER_2_CONF=bootloader_2.conf
	make -f Makefile.image bimage2
	make bootloader_clean
endif
endif

subdirs:
	for dir in $(DIRS) ; do $(MAKEARCH) $(PARAL_MAKE) -C $$dir || exit 1 ; done

.PHONY: user
user:
	$(MAKEARCH) -C user || exit 1; done

.PHONY: modules
modules:
	. $(LINUX_CONFIG); if [ "$$CONFIG_MODULES" = "y" ]; then \
		[ -d $(LINUXDIR)/modules ] || mkdir $(LINUXDIR)/modules; \
		$(MAKEARCH_KERNEL) $(PARAL_MAKE) -C $(LINUXDIR) modules; \
	fi

.PHONY: modules_install
modules_install:
	. $(LINUX_CONFIG); if [ "$$CONFIG_MODULES" = "y" ]; then \
		[ -d $(ROMFSDIR)/lib/modules ] || mkdir -p $(ROMFSDIR)/lib/modules; \
		$(MAKEARCH_KERNEL) -C $(LINUXDIR) INSTALL_MOD_PATH=$(ROMFSDIR) modules_install INSTALL_MOD_STRIP=1; \
		. $(LINUX_CONFIG); if [ "$$CONFIG_RTL8852AE_BACKPORTS" = "y" ]; then \
			$(MAKEARCH_KERNEL) -C $(LINUXDIR) M=$(BACKPORT_DIR) INSTALL_MOD_PATH=$(ROMFSDIR) modules_install INSTALL_MOD_STRIP=1; \
		fi; \
		rm -f $(ROMFSDIR)/lib/modules/*/build; \
	fi
#ifneq ($(CONFIG_PRODUCT), luna_ap_mips)
	.  $(LINUX_CONFIG); if [ "$$CONFIG_LUNA_G3_SERIES" = "y" ]; then \
	$(MAKEARCH_KERNEL)  packages_ca_install; \
	$(MAKEARCH_KERNEL)  packages_user_kmod_install; \
	fi
	.  $(LINUX_CONFIG); \
	if [ "$$CONFIG_CMCC" = "y" ] || [ "$$CONFIG_CU_BASEON_CMCC" = "y" ]; then \
	$(MAKEARCH_KERNEL) -C user/cmcc/libnetwork_core/plugin/mac_filter modules_install; \
	$(MAKEARCH_KERNEL) -C user/cmcc/libnetwork_core/plugin/flow_counter modules_install; \
	$(MAKEARCH_KERNEL) -C user/cmcc/libnetwork_core/plugin/traffic_monitor modules_install; \
	$(MAKEARCH_KERNEL) -C user/cmcc/libnetwork_core/plugin/traffic_detail_process modules_install; \
	fi
#endif

	if egrep "^CONFIG_RTK_VOIP_COMMON_ARCH=y" $(LINUXDIR)/.config > /dev/null; then	\
		cp $(LINUXDIR)/rtk_voip/voip_kernel_arm.ko $(ROMFSDIR)/lib/modules/;	\
		$(CROSS_COMPILE)strip --strip-debug $(ROMFSDIR)/lib/modules/voip_kernel_arm.ko;	\
	fi

.PHONY: rcX
rcX: rcX_clean
	for dir in $(VENDDIR) lib user user2 ; do \
		if [ "`grep "^rcX:" $$dir/Makefile`" != "" ]; then \
			$(MAKEARCH) -C $$dir rcX || exit 1 ; \
		fi \
	done		
	for rcfile in $(ROMFSDIR)/etc/init.d/rc[0-9]*; do \
		chmod -f 755 $$rcfile; \
	done

.PHONY: rcX_clean
rcX_clean:
	rm -rf $(ROMFSDIR)/etc/init.d/rc[0-9]*


############################################################################
#
# normal make targets
#

.PHONY: romfs
romfs:
	rm -rf $(ROMFSDIR)
	$(MAKEARCH) -C $(VENDDIR) mkromfsdir
	for dir in $(DIRS) ; do $(MAKEARCH) -C $$dir romfs STRIP=echo || exit 1 ; done
	. $(LINUX_CONFIG); \
	if [ -f $(BACKPORT_CONFIG) ]; then \
		sed '/#/d' $(BACKPORT_CONFIG) | sed -e "s/CPTCFG/CONFIG/g" | xargs \
		$(MAKEARCH) KLIB=$(BACKPORT_DIR) KLIB_BUILD=$(ROOTDIR)/$(LINUXDIR) -C $(BACKPORT_DIR)/drivers/net/wireless/realtek/g6_wifi_driver romfs || exit 0; \
	elif [ ! "$$CONFIG_RTLWIFI6" = "" ]; then \
		$(MAKEARCH) KLIB_BUILD=$(ROOTDIR)/$(LINUXDIR) -C $(LINUXDIR)/drivers/net/wireless/realtek/g6_wifi_driver romfs || exit 0; \
	fi
	-find $(ROMFSDIR)/. -name CVS | xargs -r rm -rf
	-find $(ROMFSDIR)/. -name .svn -type d | xargs -r rm -rf
ifdef CONFIG_USE_RSDK_WRAPPER
ifeq ($(findstring a96,$(CONFIG_RSDK_DIR)),a96)
ifdef CONFIG_32BIT_IMAGE
	cp -R $(RSDK)/lib/*.so $(ROMFSDIR)/lib
	cp -R $(RSDK)/lib/*.so.* $(ROMFSDIR)/lib
else
	cp -R $(RSDK)/lib64/*.so $(ROMFSDIR)/lib
	cp -R $(RSDK)/lib64/*.so.* $(ROMFSDIR)/lib
endif ###32BIT
else
	cp -R $(RSDK)/lib/*.so $(ROMFSDIR)/lib
	cp -R $(RSDK)/lib/*.so.* $(ROMFSDIR)/lib
endif ##a96
endif
	if [ "$(shell ls -A $(ROOTDIR)/lib/librtk/*.so)" ]; then \
		cp -R $(ROOTDIR)/lib/librtk/*.so $(ROMFSDIR)/lib; \
	fi
	if [ "$(shell ls -A $(ROOTDIR)/lib/librtk/*.so.*)" ]; then \
		cp -R $(ROOTDIR)/lib/librtk/*.so.* $(ROMFSDIR)/lib; \
	fi
	chmod 755 $(ROMFSDIR)/lib/*.so
	rm -f $(LSTRIPPED)

	if [ -d luna_patch ]; then \
		cp -a luna_patch $(ROMFSDIR) || exit 1 ; \
	fi	
	make sysroot_install
	make backports_romfs

	# release_date                                                                                                                                                                                                                                                        
	date +"%Y-%m-%d %H:%M:%S" > release_date
	cp release_date $(ROMFSDIR)/etc

ifneq (,$(filter $(ARCH),arm arm64))
	# dbg_tool is a MIPS 32 ELF file.
	rm -f $(ROMFSDIR)/bin/dbg_tool
endif
	
#### luna image ########################################################	
progimages_fullcombined:
ifeq ($(CONFIG_PRODUCT), luna)
ifneq (,$(filter asdk aarch,$(findstring asdk,$(CONFIG_RSDK_DIR)) $(findstring aarch,$(CONFIG_RSDK_DIR))))
	make -f Makefile.image progimages_fullcombined
endif
endif
ifeq ($(CONFIG_PRODUCT),luna_ap_arm)
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
	echo "do progimages_fullcombined - 98F.";
else
	echo "do progimages_fullcombined - 98XA.";
	make -f Makefile.image progimages_fullcombined
endif
endif

progimages_loader:
ifeq ($(CONFIG_PRODUCT), luna)
ifneq (,$(filter asdk aarch,$(findstring asdk,$(CONFIG_RSDK_DIR)) $(findstring aarch,$(CONFIG_RSDK_DIR))))
	make -f Makefile.image progimages_loader
endif
endif
ifeq ($(CONFIG_PRODUCT),luna_ap_arm)
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
	echo "do progimages_loader - 98F.";
else
	echo "do progimages_loader - 98XA.";
	make -f Makefile.image progimages_loader
endif
endif

image:
ifeq ($(CONFIG_PRODUCT), luna)
	make -f Makefile.image image
else
ifeq ($(CONFIG_PRODUCT), luna_xdsl)
	make -f Makefile.image xdsl_image
else
ifeq ($(CONFIG_PRODUCT),luna_ap_arm)
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
	echo "do image - 98F.";
	make -f Makefile.image ap_image
else
	echo "do image - 98XA.";
	make -f Makefile.image image
endif
else
	make -f Makefile.image ap_image
endif
endif
endif


vmimg:
	cp oemconfig/factory.bin images/
ifeq ($(CONFIG_PRODUCT), luna_xdsl)
	make -f Makefile.image xdsl_vmimg
else
ifeq ($(CONFIG_PRODUCT),$(filter $(CONFIG_PRODUCT),luna luna_ap_arm luna_ap_mips))
ifneq (,$(filter asdk aarch,$(findstring asdk,$(CONFIG_RSDK_DIR)) $(findstring aarch,$(CONFIG_RSDK_DIR))))
	make bimage;
endif
ifeq ($(findstring 5.10,$(KERNEL_VERSION)), 5.10)
ifeq ($(findstring msdk,$(CONFIG_RSDK_DIR)),msdk) ##mips
ifeq ($(findstring CONFIG_RTK_DEV_AP=y,$(KERNEL_CONFIG)), CONFIG_RTK_DEV_AP=y)
	make -f Makefile.image ap_vmimg
	make show_toolchain
else
	make -f Makefile.image vmimg
endif
else ##arm or arm64
	make -f Makefile.image vmimg_fit
endif
else
ifeq ($(CONFIG_32BIT_IMAGE),y)
	make -f Makefile.image vmimg_fit
else
	make -f Makefile.image vmimg
endif
endif ###KERNEL_VERSION 5.10 end

else
ifeq ($(CONFIG_PRODUCT),luna_ap_arm)
ifeq ($(findstring a17,$(CONFIG_RSDK_DIR)),a17)
	echo "do vmimg - 98F.";
	make -f Makefile.image ap_vmimg
	make show_toolchain
else
	echo "do vmimg - 98XA.";
	make bimage;
	make -f Makefile.image vmimg
endif
else
	make -f Makefile.image ap_vmimg
	make show_toolchain
endif
endif
endif

cudy_factory:
ifeq ($(CONFIG_PRD_SIGNATURE),9607C)
	@if [ A"$(shell echo $@ | grep "_spi_nor_")" != "A" ]; then \
		echo "...............flash type is NOR."; \
		cp images-backup/nor/encode_uboot.img images/; \
	else \
		echo "...............flash type is NAND."; \
		cp images-backup/nand/encode_uboot.img images/; \
	fi
	./onebin -ft nand -npt large -l images/encode_uboot.img -f images/ubi_device_img.ubi 0x140000 -o images/GP1200-$$(cat $(LINUXDIR)/fwu_ver)-factory.bin
else
ifeq ($(CONFIG_PRD_SIGNATURE),9607F)
	make progimages_fullcombined
	cp $(IMAGEDIR)/programmer_images/combined_onebin_image.bin $(IMAGEDIR)/GP3000-$$(cat $(LINUXDIR)/fwu_ver)-factory.bin
endif
endif

bimage: # For only 1 bootloader
ifneq ($(CONFIG_UBOOT_VER),"")
ifneq ($(CONFIG_SDK_2TH_BL_GEN), y)
	make -f Makefile.image bimage
endif
endif


bimage2:
ifneq ($(CONFIG_UBOOT_VER),"")
	make -f Makefile.image bimage2
endif
### $(PRODUCT98F) ###########################################################
## Change Makefile.image to do detail process
98f_image:
	make -f Makefile.image 98f_image

98f_vmimg:
	make -f Makefile.image 98f_vmimg

###################################################################	
%_only:
	if [ -d "$(@:_only=)" ]; then \
                if [ -n "$(findstring user/,$@)" ]; then \
                        $(MAKEARCH) -C user $(@:user/%=%); \
                else \
                        if [ ! -f $(@:_only=)/Makefile ]; then \
                                cd $(@:_only=) && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && cd -; \
                        fi; \
                        $(MAKEARCH) -C $(@:_only=); \
                fi; \
        fi


%_romfs:
	if [ -d "$(@:_romfs=)" ]; then \
                if [ -n "$(findstring user/,$@)" ]; then \
                        $(MAKEARCH) -C user $(@:user/%=%); \
                else \
                        $(MAKEARCH) -C $(@:_romfs=) romfs; \
                fi; \
        fi

%_clean:
	if [ -d "$(@:_clean=)" ]; then \
                if [ -n "$(findstring user/,$@)" ]; then \
                        $(MAKEARCH) -C user $(@:user/%=%); \
                else \
                        $(MAKEARCH) -C $(@:_clean=) clean; \
                fi; \
        fi
############################################
## clean section

kernel_aipc_clean:
	@[ ! -d $(ROOTDIR)/rtk_voip/aipc_char ] || find $(ROOTDIR)/rtk_voip/aipc_char \
		\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name '*.symtypes' -o -name 'modules.order' \
		-o -name 'Module.markers' -o -name '.tmp_*.o.*' \) \
		-exec rm -f {} \;

kernel_voip_clean:
	@[ ! -d $(ROOTDIR)/rtk_voip/kernel ] || find $(ROOTDIR)/rtk_voip/kernel \
		\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name '*.symtypes' -o -name 'modules.order' \
		-o -name 'Module.markers' -o -name '.tmp_*.o.*' \) \
		-exec rm -f {} \;
	
modules_clean:
	[ ! -d modules ] || $(MAKEARCH) -C modules clean
	
clean: modules_clean
	for dir in $(DIRS); do $(MAKEARCH) -C $$dir clean ; done
	rm -rf $(ROMFSDIR)/*
	rm -rf $(IMAGEDIR)/*
	rm -rf $(LINUXDIR)/net/ipsec/alg/libaes $(LINUXDIR)/net/ipsec/alg/perlasm
	rm -rf $(ROOTDIR)/include
	make -C config/scripts/lxdialog/ clean
	[ ! -d $(CA_KMOD) ] || $(MAKEARCH) -C $(CA_KMOD) clean
	[ ! -d $(CA_USER) ] || $(MAKEARCH) -C $(CA_USER) clean
	make -C $(LINUXDIR) clean
	make kernel_aipc_clean kernel_voip_clean
	rm -rf $(CA_SYSROOT)
	#rm -rf $(ROOTDIR)/ca_packages/ca-scfg/1.0-r0/ca-scfg-1.0/customize_scfg.txt
ifneq (,$(filter asdk aarch,$(findstring asdk,$(CONFIG_RSDK_DIR)) $(findstring aarch,$(CONFIG_RSDK_DIR))))
	$(MAKE) -C $(BOOTLOADER_PATH) clean
endif
	if [ -f $(BACKPORT_DIR)/$(BACKPORT_LOCAL_SYM_UPDATED) ]; then \
		cd $(BACKPORT_DIR) ; mv $(BACKPORT_LOCAL_SYM_OLD) $(BACKPORT_LOCAL_SYM); rm $(BACKPORT_LOCAL_SYM_UPDATED) -f; \
	fi
	if [ -f $(BACKPORT_AUTOCONF_H) ];then \
		rm -f $(BACKPORT_AUTOCONF_H); \
	fi
	. $(LINUX_CONFIG); if [ "$$CONFIG_RTL8852AE_BACKPORTS" = "y" ]; then \
		$(MAKEARCH) KLIB=$(BACKPORT_DIR) KLIB_BUILD=$(ROOTDIR)/$(LINUXDIR) -C $(BACKPORT_DIR) clean; \
	fi
	rm -rf $(TMPFSDIR)/*

configure_user2:
	make -C user2 configure

sdk.shell_tree:
	$(MAKE) -C user/diagshell shell_tree

user_prepare:
	$(MAKE) -C user prepare

user_voip_prepare:
	$(MAKE) -C user voip_prepare

voip_h248:
	echo "flash voip set VOIP.X_CT_SERVERTYPE 2" >> $(ROOTDIR)/rtk_voip/maserati_2.1/set_start_voipapp
	chmod 775 $(ROOTDIR)/rtk_voip/maserati_2.1/set_start_voipapp
	make all

voip_sip:
	echo "flash voip set VOIP.X_CT_SERVERTYPE 0" >> $(ROOTDIR)/rtk_voip/maserati_2.1/set_start_voipapp
	chmod 775 $(ROOTDIR)/rtk_voip/maserati_2.1/set_start_voipapp
	make all

voip_soft_sip:
	echo "flash voip set VOIP.X_CT_SERVERTYPE 1" >> $(ROOTDIR)/rtk_voip/maserati_2.1/set_start_voipapp
	chmod 775 $(ROOTDIR)/rtk_voip/maserati_2.1/set_start_voipapp
	make all

show_toolchain:
	@echo "Toolchain: $(shell readlink -f $(CONFIG_RSDK_DIR))"
	mkdir -p $(TMPFSDIR)/bin
	mkdir -p $(TMPFSDIR)/include $(TMPFSDIR)/lib
	mkdir -p $(TMPFSDIR)/usr/include $(TMPFSDIR)/usr/lib
	mkdir -p $(TMPFSDIR)/usr/local/include $(TMPFSDIR)/usr/local/lib
	echo -e '#!/bin/sh\n\nPKG_CONFIG_LIBDIR=$(TMPFSDIR)/lib/pkgconfig:$(TMPFSDIR)/usr/lib/pkgconfig:$(TMPFSDIR)/usr/local/lib/pkgconfig   PKG_CONFIG_PATH=$(TMPFSDIR)/lib/pkgconfig:$(TMPFSDIR)/usr/lib/pkgconfig:$(TMPFSDIR)/usr/local/lib/pkgconfig pkg-config $(PKG_CONFIG_EXTRAARGS) "$$@"' > $(PKG_CONFIG)
	chmod +x $(PKG_CONFIG)
