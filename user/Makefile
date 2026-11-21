#
#	Makefile -- Build instructions for user level apps
#

.EXPORT_ALL_VARIABLES:

#
# Include architecture specific build rules.
#

ifndef ROOTDIR
ROOTDIR=..
endif

BACKPORT_CONFIG ?= $(ROOTDIR)/$(BACKPORT_DIR)/.config

UCLINUX_BUILD_USER=1
-include $(LINUX_CONFIG)
-include $(CONFIG_CONFIG)
-include $(ARCH_CONFIG)
-include $(MODULES_CONFIG)
-include $(BACKPORT_CONFIG)
-include $(CHECKSEC_CONFIG)

-include $(ROOTDIR)/$(BUSYBOXDIR_DEST)/.config

VEND=$(ROOTDIR)/vendors
ifdef CONFIG_USE_RSDK_WRAPPER
LDFLAGS += -L$(ROOTDIR)/lib/librtk
export LDFLAGS
endif #CONFIG_USE_RSDK_WRAPPER

ifeq ($(CONFIG_LUNA),y)
ifeq ($(CONFIG_RTL8686_SWITCH),y)
ifeq ($(CONFIG_LUNA_G3_SERIES),y)
ifeq ($(CONFIG_MULTICHIP_ONEIMAGE),y)
CFLAGS += -I$(ROOTDIR)/$(CA_SYSROOT)/usr/include/77c/ca-plat/
CFLAGS += -I$(ROOTDIR)/$(CA_SYSROOT)/usr/include/77c/cortina-api/
else
CFLAGS += -I$(ROOTDIR)/$(CA_SYSROOT)/usr/include/ca-plat/
CFLAGS += -I$(ROOTDIR)/$(CA_SYSROOT)/usr/include/cortina-api/
endif
endif
-include $(APOLLODIR)/Compiler_Flag
CFLAGS += $(PON_CFLAGS)
CFLAGS += -include $(ROOTDIR)/autoconf.h
CFLAGS += -include $(ROOTDIR)/config/autoconf.h
CFLAGS += -I$(APOLLODIR)/sdk/include
CFLAGS += -I$(APOLLODIR)/sdk/system/include
CFLAGS += -I$(APOLLODIR)/romeDriver
LDFLAGS += -L$(APOLLODIR)/object/src/app/lib
ifeq ($(CONFIG_TR142_MODULE),y)
CFLAGS += -I$(APOLLODIR)/sdk/src/app/omci_v1/API
ifeq ($(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE),y)
CFLAGS += -I$(ROOTDIR)/user/rtk_tr142/module/tr142_fc/include
else
CFLAGS += -I$(APOLLODIR)/tr142/include
endif
endif
ifeq ($(CONFIG_COMMON_RT_API),y)
ifeq ($(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE),y)
CFLAGS += -I$(APOLLODIR)/FleetConntrackDriver/include
CFLAGS += -I$(APOLLODIR)/FleetConntrackDriver/rt/include
endif #CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE
endif #CONFIG_COMMON_RT_API
ifeq ($(CONFIG_RTL_SMUX_DEV),y)
CFLAGS += -I$(NETDIR)
endif
LIBRTK = -lrtk -lpthread -lgcc_s
endif # CONFIG_RTL8686_SWITCH
ifeq ($(CONFIG_USER_STACKPROTECTOR_STRONG),y)
CFLAGS += -fstack-protector-strong
endif

ifeq ($(CONFIG_LIB_8367),y)
LIBRTK += -l8367
endif

endif #CONFIG_LUNA
LIBMIB = -lmib -pthread -lgcc_s $(LIBRTK)

ifdef CONFIG_USER_CMD_SERVER
CFLAGS += -I$(ROOTDIR)/user/commserv2/include/
LDFLAGS += -L$(ROOTDIR)/user/commserv2/lib/ 
LIBCOMMSERV = -I$(ROOTDIR)/user/commserv2/include/ -L$(ROOTDIR)/user/commserv2/lib/ -lcommserv
LIBMIB += $(LIBCOMMSERV)
endif

ifdef CONFIG_CU_BASEON_CMCC
LIBMIB += -liconv
endif


#CU: use libcap under user/ctcyueme/lxc instead of user/libcap
ifdef CONFIG_CU_BASEON_YUEME
ifeq ($(ARCH)$(CONFIG_KERNEL_4_4_x),mipsy)
ifneq (,$(findstring g2.23,$(CONFIG_RSDK_DIR)))
ARCH_KERNEL = $(ARCH)-4.4-glibc
else
ARCH_KERNEL = $(ARCH)-4.4
endif
else
ARCH_KERNEL = $(ARCH)
endif
LXC_BINARY = $(ROOTDIR)/user/ctcyueme/lxc/binary_cu/$(ARCH_KERNEL)
CFLAGS += -I$(ROOTDIR)/user/ctcyueme/lxc/libcap-2.24/libcap/include
LDFLAGS += -L$(LXC_BINARY)/lib -lcap
endif

ifdef CONFIG_00R0
LIBMIB += -lomci_api
endif

#ifeq ($(CONFIG_USER_SICHUAN_API),y)
#CFLAGS += -DCONFIG_USER_SICHUAN_API
#endif

#add for elink#
#penssl_dir = openssl-1.0.1c
openssl_dir = libssl
export openssl_dir
ifdef CONFIG_ELINKSDK_SUPPORT
ARCH_NAME=Formosa
VENDOR_NAME=realtek
export ARCH_NAME
export VENDOR_NAME
LIBMIB += -L$(ROOTDIR)/lib/ -lelinksdk 
LIBMIB += -L$(ROOTDIR)/lib/$(openssl_dir) -lcrypto
endif

ifeq ($(CONFIG_USER_WLAN_QCSAPI),y)
LIBMIB += -lqcsapi_client
LDFLAGS += -L$(ROOTDIR)/user/qcsapi  
endif

ifeq ($(CONFIG_USER_WIRELESS_TOOLS), y)
ifeq ($(CONFIG_RTK_DEV_AP),y)
ifneq ("$(wildcard $(ROOTDIR)/backports-5.2.8-1/drivers/net/wireless/realtek/rtl8192fe)","")
CFLAGS += -I$(ROOTDIR)/backports-5.2.8-1/drivers/net/wireless/realtek/rtl8192fe
endif
endif
ifeq ($(CONFIG_RTL8852AE_BACKPORTS),y)
ifneq ("$(wildcard $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/realtek/g6_wifi_driver)","")
CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/realtek/g6_wifi_driver
endif
else
ifneq ($(CONFIG_RTL8852AE)$(CONFIG_RTLWIFI6),)
ifneq ("$(wildcard $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/realtek/g6_wifi_driver)","")
CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/realtek/g6_wifi_driver
endif
else
ifneq ($(CONFIG_RTL8852CE),)
ifneq ("$(wildcard $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/realtek/g6_wifi_driver)","")
CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/realtek/g6_wifi_driver
endif
endif
endif
endif
ifneq ("$(wildcard $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/realtek/rtl8192cd)","")
CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/realtek/rtl8192cd
endif
ifneq ("$(wildcard $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/rtl8192cd)","")
CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/rtl8192cd
endif
else
ifeq ($(CONFIG_USER_RTK_MULTI_AP),y)
ifneq ("$(wildcard $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/realtek/rtl8192cd)","")
CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/realtek/rtl8192cd
endif
ifneq ("$(wildcard $(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/rtl8192cd)","")
CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/net/wireless/rtl8192cd
endif
endif
endif

ifeq ($(CONFIG_USER_DHCPV6_RECONF_MSG),y)
RECONF_LIBS = -lmd5
endif

ifeq ($(CONFIG_USER_IPTABLES_1_4_18_FAST_APPLY),y)
IPTABLES_1_4_18_CONFIG_OPT = --enable-rtk-fast-apply
endif
##### add for VoIP merging ##########
ifeq ($(CONFIG_USER_RTK_VOIP),y)
DIR_LINUX = $(ROOTDIR)/$(LINUXDIR)
DIR_USERS=$(ROOTDIR)/user
export DIR_LINUX
export DIR_USERS

export LIBVOIP
LIBVOIP = -L$(ROOTDIR)/rtk_voip/voip_manager_1.6 -lvoip_manager
CFLAGS += -I$(DIR_LINUX)
CFLAGS += -I$(ROOTDIR)/rtk_voip/kernel/include/
CFLAGS += -I$(ROOTDIR)/rtk_voip/voip_manager_1.6/includes
endif
ifeq ($(CONFIG_LIB_LIBSSL),y)
LIBSSL += -L$(TMPFSDIR)/lib
LIBMIB += $(LIBSSL) -lcrypt
endif

ifeq ($(CONFIG_LIB_UBUS),y)
CFLAGS +=-DJSONC
endif

ifeq ($(CONFIG_USER_DHCPV6_EUI64_POOL_TYPE),y)
CFLAGS += -DCONFIG_USER_DHCPV6_EUI64_POOL_TYPE
endif

#### add for nftables ###############
ifeq ($(CONFIG_USER_NFTABLES),y)
LIBMNL_CFLAGS=-I$(ROOTDIR)/lib/libmnl/include/
LIBMNL_LIBS=-L$(ROOTDIR)/lib/librtk -lmnl
LIBNFTNL_CFLAGS=-I$(ROOTDIR)/lib/libnftnl/include/
LIBNFTNL_LIBS=-L$(ROOTDIR)/lib/librtk -lnftnl

export LIBMNL_CFLAGS
export LIBMNL_LIBS
export LIBNFTNL_CFLAGS
export LIBNFTNL_LIBS
endif
####################################

######################################
#

# must run the vendor build first
#
dir_v = $(VEND)/$(CONFIG_VENDOR)/$(CONFIG_PRODUCT)/.

ifeq ($(CONFIG_LIB_CAP),y)
dir_v	+= libcap
endif

ifeq ($(CONFIG_USER_RTK_VOIP),y)
#dir_v += rtk_voip
dir_v += $(ROOTDIR)/rtk_voip/voip_manager_1.6
endif

dir_y =
dir_n =
dir_  =
dir_e =

dir_$(CONFIG_KMOD_NFBI)					+= kmod_nfbi
dir_$(CONFIG_KMOD_RXAUI)					+= kmod_rxaui
dir_$(CONFIG_KMOD_SLOADER)                                     += kmod_sloader
dir_$(CONFIG_KMOD_TAURUS_MDIO)                                     += kmod_mdio
dir_$(CONFIG_KMOD_OLT)                                     += kmod_olt
dir_$(CONFIG_KMOD_RELAYCTRL)				+= kmod_relayctrl
dir_${CONFIG_LIB_8367}						+= lib8367		# before boa
dir_$(CONFIG_USER_RTK_RTL8367)				+= rtl8367		# before boa

#dir_$(CONFIG_USER_RTK_VOIP)                 += rtk_voip	# before boa
dir_$(CONFIG_RTK_HOST_SPEEDUP)              += rtk_hostspeedup
dir_${CONFIG_LIB_EXTGPHY}	+= libextgphy
ifeq ($(CONFIG_LIB_EXTGPHY),y)
dir_${CONFIG_USER_RTK_EXTGPHY}	+= extgphy

endif

dir_$(CONFIG_USER_WLAN_QCSAPI)              += qcsapi
dir_$(CONFIG_USER_WLAN_QCSAPI)              += ip_relay_1.1

dir_$(CONFIG_USER_AGETTY_AGETTY)            += agetty
dir_$(CONFIG_USER_AT_AT)                    += at
dir_$(CONFIG_USER_AT_ATD)                   += at
dir_$(CONFIG_USER_AT_ATRUN)                 += at
ifeq ($(CONFIG_RTK_DEV_AP),y)
#dir_$(CONFIG_ELINK_SUPPORT)      	    += lib_elinksdk
endif
ifndef CONFIG_E8B
dir_$(CONFIG_USER_RTK_VOIP)                 += rtk_voip
dir_$(CONFIG_USER_CMD_SERVER) 		    += commserv2 # before boa
dir_$(CONFIG_USER_BOA_SRC_BOA)              += boa
else
ifeq ($(CONFIG_RTK_L34_FLEETCONNTRACK_ENABLE),y)
dir_$(CONFIG_USER_BOA_SRC_BOA)              += boa
else
dir_$(CONFIG_USER_BOA_SRC_BOA)              += boa_e8
endif
 
dir_$(CONFIG_USER_RTK_VOIP)                 += rtk_voip
endif
ifeq ($(CONFIG_LUNA),y)
dir_$(CONFIG_USER_BOA_SRC_BOA)              += mpctl
endif
#because boa need to call GPON or EPON APIs in librtk.a and diagshell will build it
ifeq ($(CONFIG_LUNA),y)
ifeq ($(CONFIG_USER_APOLLO_DIAGSHELL),y)
dir_v +=diagshell
ifneq ($(CONFIG_GPON_FEATURE)_$(CONFIG_EPON_FEATURE),_)
dir_y += rtkbosa
endif
endif
endif

dir_$(CONFIG_IPV6)              += sntpv6

dir_$(CONFIG_PPPOE_MONITOR) += ppp-monitor
dir_$(CONFIG_USER_NEIGHDETECT) += neighdetect

ifeq ($(CONFIG_PTM_BONDING),y)
CFLAGS += -DCONFIG_PTM_BONDING
endif

ifeq ($(CONFIG_PTM_BONDING_MASTER),y)
CFLAGS += -DCONFIG_PTM_BONDING_MASTER
endif

ifeq ($(CONFIG_PTM_BONDING_SLAVE),y)
CFLAGS += -DCONFIG_PTM_BONDING_SLAVE
endif

CFLAGS += -rdynamic -fexceptions

#cudy-app
#dir_$(CONFIG_USER_MEMTEST)   += memtester
dir_$(CONFIG_USER_OEM_FIXUP) += oem-fixup
dir_$(CONFIG_USER_HCTOOLS)   += hctools
dir_$(CONFIG_USER_BDINFO)    += bdinfo

dir_$(CONFIG_USER_SQUASHFS42)		+= squashfs4.2/squashfs-tools
dir_$(CONFIG_CMCC_IPV6_SECURITY_SUPPORT) += ip6sec
dir_$(CONFIG_USER_ZM8620_ECMMODE) += zecmapp
dir_$(CONFIG_USER_IPUTILS) += iputils

dir_$(CONFIG_USER_DBUS_CTC_IGD) 	    += ctc-igd
dir_$(CONFIG_USER_BOOTTOOLS_FLASHLOADER)    += boottools
dir_$(CONFIG_USER_BOOTTOOLS_HIMEMLOADER)    += boottools
dir_$(CONFIG_USER_BPALOGIN_BPALOGIN)        += bpalogin
dir_$(CONFIG_USER_BR2684CTL_BR2684CTL)      += br2684ctl
dir_$(CONFIG_USER_BRCFG_BRCFG)              += brcfg
dir_$(CONFIG_USER_BRCTL_BRCTL)              += bridge-utils
dir_$(CONFIG_USER_BUSYBOX_BUSYBOX)          += busybox
# Kao added 11/05/03
dir_$(CONFIG_USER_BUSYBOX_BUSYBOX100PRE3)   += busybox-1.00-pre3
# Mason Yu. for ipv6
dir_$(CONFIG_USER_BUSYBOX_BUSYBOX1221)      += busybox-1.22.1
dir_$(CONFIG_USER_CAL_CAL)                  += cal
dir_$(CONFIG_USER_CAL_DATE)                 += cal
dir_$(CONFIG_USER_CKSUM_CKSUM)              += cksum
dir_$(CONFIG_USER_CLOCK_CLOCK)              += clock
dir_$(CONFIG_USER_CPU_CPU)                  += cpu
dir_$(CONFIG_USER_CPU_LIMIT)			+= cpulimit
dir_$(CONFIG_USER_CRAMFS_MKCRAMFS)          += cramfs
dir_$(CONFIG_USER_CRAMFS_CRAMFSCK)          += cramfs
ifeq ($(CONFIG_USER_KMOD_NTFS3),y)
dir_$(CONFIG_USER_KMOD_NTFS3)              += kmod_ntfs3
else
ifeq ($(CONFIG_LUNA),y)
dir_$(CONFIG_USER_NTFS_NTFS3G)              += ntfs3g
else
dir_$(CONFIG_USER_NTFS_NTFS3G)              += ntfs-3g
endif
endif
dir_$(CONFIG_USER_RTL_HIGH_PERFORMANCE_FILESYSTEM)		+= rtl_fs
dir_$(CONFIG_USER_EXFAT)		    += exfat
dir_$(CONFIG_USER_CRON_CRON)                += cron
dir_$(CONFIG_USER_CUPS)			    += cups
dir_$(CONFIG_USER_CURL_7_36_0)		    += curl
dir_$(CONFIG_USER_CURL_7_75_0)              += curl
dir_$(CONFIG_USER_CURL_8_4_0)               += curl
dir_$(CONFIG_USER_FIRMWARE_ENHANCE)	    += firmware_enhance
dir_$(CONFIG_USER_DEMO_BUTTON)              += demo
dir_$(CONFIG_USER_DEMO_MORSE)               += demo
dir_$(CONFIG_USER_DHRYSTONE_DHRYSTONE)      += dhrystone
dir_$(CONFIG_USER_WIDE_DHCPV6)              += wide-dhcpv6
dir_$(CONFIG_USER_DHCP_ISC_SERVER_DHCPD)    += dhcp-isc
dir_$(CONFIG_USER_DHCP_ISC_CLIENT_DHCLIENT) += dhcp-isc
dir_$(CONFIG_USER_DHCP_ISC_RELAY_DHCRELAY)  += dhcp-isc
dir_$(CONFIG_USER_DHCPCD_DHCPCD)            += dhcpcd
dir_$(CONFIG_USER_DHCPCD_NEW_DHCPCD)        += dhcpcd-new
dir_$(CONFIG_USER_DHCPD_DHCPD)              += dhcpd
#For DHCPv6, we use sysconf command for it, so you need to set CONFIG_USER_RTK_SYSCONF in preconfig file. 2020/03/05
dir_$(CONFIG_USER_DHCPV6_ISC_DHCP411)	    += dhcp-4.1.1
dir_$(CONFIG_USER_DHCPV6_ISC_DHCP441)       += dhcp-4.4.1
dir_$(CONFIG_USER_DIALD_DIALD)              += diald
dir_$(CONFIG_USER_DISCARD_DISCARD)          += discard
dir_$(CONFIG_USER_ADAPTER_API_ISP)          += adapter-api
dir_$(CONFIG_USER_DMALLOC)		    += dmalloc-5.5.2
#dir_$(CONFIG_USER_SICHUAN_API)              +=sichuan_api
dir_$(CONFIG_USER_ADAPTER_API_ETHGETINFO)   += ethGetInfo
dir_$(CONFIG_USER_DNSMASQ_DNSMASQ245)       += dnsmasq-2.45
dir_$(CONFIG_USER_DNSMASQ_DNSMASQ278)       += dnsmasq-2.78
dir_$(CONFIG_USER_DNSMASQ_DNSMASQ280)       += dnsmasq-2.80
dir_$(CONFIG_USER_DNSMASQ_DNSMASQ285)       += dnsmasq-2.85
dir_$(CONFIG_USER_RTK_DSL_LOG)              += dslautolog
dir_$(CONFIG_USER_LLMNR)                    += llmnrd-master
dir_$(CONFIG_USER_PARTED)                   += parted
dir_$(CONFIG_USER_E2FSPROGS_E2FSCK_E2FSCK)  += e2fsprogs
dir_$(CONFIG_USER_E2FSPROGS_MISC_BADBLOCKS) += e2fsprogs
dir_$(CONFIG_USER_E2FSPROGS_MISC_CHATTR)    += e2fsprogs
dir_$(CONFIG_USER_E2FSPROGS_MISC_DUMPE2FS)  += e2fsprogs
dir_$(CONFIG_USER_E2FSPROGS_MISC_E2LABEL)   += e2fsprogs
dir_$(CONFIG_USER_E2FSPROGS_MISC_FSCK)      += e2fsprogs
dir_$(CONFIG_USER_E2FSPROGS_MISC_LSATTR)    += e2fsprogs
dir_$(CONFIG_USER_E2FSPROGS_MISC_MKE2FS)    += e2fsprogs
dir_$(CONFIG_USER_E2FSPROGS_MISC_MKLOST_FOUND) += e2fsprogs
dir_$(CONFIG_USER_E2FSPROGS_MISC_TUNE2FS)   += e2fsprogs
dir_$(CONFIG_USER_E2FSPROGS_MISC_UUIDGEN)   += e2fsprogs
dir_$(CONFIG_USER_SGDISK)                   += gptfdisk
dir_$(CONFIG_USER_ELVISTINY_VI)             += elvis-tiny
dir_$(CONFIG_USER_ETHATTACH_ETHATTACH)      += ethattach
dir_$(CONFIG_USER_EZIPUPDATE_EZIPUPDATE)    += ez-ipupdate
dir_$(CONFIG_USER_FDISK_FDISK)              += fdisk
dir_$(CONFIG_USER_FILEUTILS_CAT)            += fileutils
dir_$(CONFIG_USER_FILEUTILS_CHGRP)          += fileutils
dir_$(CONFIG_USER_FILEUTILS_CHMOD)          += fileutils
dir_$(CONFIG_USER_FILEUTILS_CHOWN)          += fileutils
dir_$(CONFIG_USER_FILEUTILS_CMP)            += fileutils
dir_$(CONFIG_USER_FILEUTILS_CP)             += fileutils
dir_$(CONFIG_USER_FILEUTILS_DD)             += fileutils
dir_$(CONFIG_USER_FILEUTILS_GREP)           += fileutils
dir_$(CONFIG_USER_FILEUTILS_L)              += fileutils
dir_$(CONFIG_USER_FILEUTILS_LN)             += fileutils
dir_$(CONFIG_USER_FILEUTILS_LS)             += fileutils
dir_$(CONFIG_USER_FILEUTILS_MKDIR)          += fileutils
dir_$(CONFIG_USER_FILEUTILS_MKFIFO)         += fileutils
dir_$(CONFIG_USER_FILEUTILS_MKNOD)          += fileutils
dir_$(CONFIG_USER_FILEUTILS_MORE)           += fileutils
dir_$(CONFIG_USER_FILEUTILS_MV)             += fileutils
dir_$(CONFIG_USER_FILEUTILS_RM)             += fileutils
dir_$(CONFIG_USER_FILEUTILS_RMDIR)          += fileutils
dir_$(CONFIG_USER_FILEUTILS_SYNC)           += fileutils
dir_$(CONFIG_USER_FILEUTILS_TOUCH)          += fileutils
dir_$(CONFIG_USER_FIO)                      += fio
dir_$(CONFIG_USER_FLASH_FLASH)              += flash
dir_$(CONFIG_USER_FLASHW_FLASHW)            += flashw
dir_$(CONFIG_USER_FLATFSD_XXX)              += flatfsd
ifneq ($(CONFIG_E8B),y)
dir_$(CONFIG_APACHE_FELIX_FRAMEWORK)        += osgi
endif
dir_$(CONFIG_USER_FON)			    += fongw
dir_$(CONFIG_USER_FREESWAN)                 += freeswan
dir_$(CONFIG_USER_FROB_LED_FROB_LED)        += frob-led
dir_$(CONFIG_USER_FSWCERT_FSWCERT)          += fswcert
dir_$(CONFIG_USER_FTP_FTP_FTP)              += ftp
dir_$(CONFIG_USER_FTPD_FTPD)                += ftpd
#Jia-chiam: gdbserver has been porting in user2@Luna trunk.
#           If someone want to recover it, plz let me know.
#dir_$(CONFIG_USER_GDB_GDB)                  += gdb-6.8
#dir_$(CONFIG_USER_GDB_GDBSERVER)            += gdb-6.8
#dir_$(CONFIG_USER_GDB_GDBREPLAY)            += gdb-6.8
dir_$(CONFIG_USER_GDB_7_12)                  += gdb-7.12
dir_$(CONFIG_USER_GETTYD_GETTYD)            += gettyd
dir_$(CONFIG_USER_HD_HD)                    += hd
dir_$(CONFIG_USER_HAPD)                     += hostapd sysconf
dir_$(CONFIG_USER_WPAS)                     += wpa_supplicant
dir_$(CONFIG_USER_HTTPD_HTTPD)              += httpd
dir_$(CONFIG_USER_HPING2_RC3)               += hping2-rc3
dir_$(CONFIG_USER_HWCLOCK_HWCLOCK)          += hwclock
dir_$(CONFIG_USER_IFATTACH_IFATTACH)        += ifattach
dir_$(CONFIG_USER_IFMETRIC)		    += ifmetric
dir_$(CONFIG_USER_IFMOND2_IFMOND)           += ifmond2
dir_$(CONFIG_USER_INETD_INETD)              += inetd
dir_$(CONFIG_USER_INIT.ORG_INIT)            += init.org
dir_$(CONFIG_USER_INIT_EXPAND)              += init
dir_$(CONFIG_USER_INIT_INIT)                += init
dir_$(CONFIG_USER_IPCHAINS_IPCHAINS)        += ipchains
dir_$(CONFIG_USER_IPERF)                    += iperf-2.0.4-src
dir_$(CONFIG_USER_IPERF_2_0_9)		    += iperf-2.0.9
dir_$(CONFIG_USER_IPERF_3_1_4)              += iperf-3.1.4
dir_$(CONFIG_USER_IPFWADM_IPFWADM)          += ipfwadm
dir_$(CONFIG_USER_IPMASQADM_IPMASQADM)      += ipmasqadm
dir_$(CONFIG_USER_IPPORTFW_IPPORTFW)        += ipportfw
dir_$(CONFIG_USER_IPREDIR_IPREDIR)          += ipredir
#dir_$(CONFIG_USER_IPROUTE2)                 += iproute2-2.6.33
dir_$(CONFIG_USER_IPROUTE2_3_4_0)			+= iproute2-3.4.0
dir_$(CONFIG_USER_IPROUTE2_4_4_0)			+= iproute2-4.4.0
dir_$(CONFIG_USER_IPROUTE2_3_18_0)			+= iproute2-3.18.0
dir_$(CONFIG_USER_IPROUTE2_5_15_0)		+= iproute2
# if define CONFIG_USER_MINIUPNP_V2_1,
# boa will include iptables lib to dynamic get iptables rules
# So, we pull high iptables compile priority
ifeq ($(CONFIG_USER_IPTABLES_IPTABLES_1_4_18),y)
dir_v	+= iptables-1.4.18
endif
ifeq ($(CONFIG_USER_IPTABLES_IPTABLES_1_4_21),y)
dir_v	+= iptables-1.4.21
endif
dir_$(CONFIG_USER_IPSET)					+= ipset
dir_$(CONFIG_USER_NFTABLES)		    += nftables
dir_$(CONFIG_BRIDGE_NF_EBTABLES)            += ebtables
dir_$(CONFIG_KMOD_NF_DSCPEXT)               += kmod_nf_dscpext
dir_$(CONFIG_USER_KLAXON_KLAXON)            += klaxon
dir_$(CONFIG_USER_LANG_A60)                 += a60
dir_$(CONFIG_USER_LCD_LCD)                  += lcd
#ql porting new l2tpd
#dir_$(CONFIG_USER_L2TPD_L2TPD)              += l2tpd
dir_$(CONFIG_USER_LEVEE_VI)                 += levee
dir_$(CONFIG_USER_LILO_LILO)                += lilo
dir_$(CONFIG_USER_LIRC)                     += lirc
dir_$(CONFIG_USER_LISSA_LISSA)              += lissa
dir_$(CONFIG_USER_LOATTACH_LOATTACH)        += loattach
dir_$(CONFIG_USER_LOGIN_LOGIN)              += login
dir_$(CONFIG_USER_LOGIN_PASSWD)             += login
dir_$(CONFIG_USER_RTK_LBD)					+= loopback
dir_$(CONFIG_USER_MAIL_MAIL_IP)             += mail
#MAP-E
dir_$(CONFIG_USER_MAP_E)                    += s46map
#MAP-T
dir_$(CONFIG_USER_MAP_T)                    += s46map
dir_$(CONFIG_USER_MAP_T)                    += nat46
dir_$(CONFIG_USER_XLAT464)                  += nat46
dir_$(CONFIG_USER_LW4O6)                    += s46map
dir_$(CONFIG_USER_DNS64)                    += bind
dir_$(CONFIG_USER_NAT64)                    += nat64
dir_$(CONFIG_USER_MATH_TEST)                += mathtest
dir_$(CONFIG_USER_MAWK_AWK)                 += mawk
dir_$(CONFIG_USER_MTDUTILS)                 += mtd-utils-1.5.2_luna
dir_$(CONFIG_USER_MICROWIN)                 += microwin
dir_$(CONFIG_USER_MINI_HTTPD_MINI_HTTPD)    += mini_httpd
dir_$(CONFIG_USER_MONITORD)                 += monitord
dir_$(CONFIG_USER_MOUNT_MOUNT)              += mount
dir_$(CONFIG_USER_MOUNT_UMOUNT)             += mount
dir_$(CONFIG_USER_MP3PLAY_MP3PLAY)          += mp3play
#dir_$(CONFIG_USER_MQTT_SUPPORT)             += mqtt
dir_$(CONFIG_USER_MSNTP_MSNTP)              += msntp
dir_$(CONFIG_USER_MSTPD)                    += mstpd
dir_$(CONFIG_USER_MUSICBOX_MUSICBOX)        += musicbox
dir_$(CONFIG_USER_NETFLASH_NETFLASH)        += netflash
dir_$(CONFIG_USER_NET_TOOLS_ARP)            += net-tools
dir_$(CONFIG_USER_NET_TOOLS_HOSTNAME)       += net-tools
dir_$(CONFIG_USER_NET_TOOLS_IFCONFIG)       += net-tools
dir_$(CONFIG_USER_NET_TOOLS_NAMEIF)         += net-tools
dir_$(CONFIG_USER_NET_TOOLS_NETSTAT)        += net-tools
dir_$(CONFIG_USER_NET_TOOLS_PLIPCONFIG)     += net-tools
dir_$(CONFIG_USER_NET_TOOLS_RARP)           += net-tools
dir_$(CONFIG_USER_NET_TOOLS_ROUTE)          += net-tools
dir_$(CONFIG_USER_NET_TOOLS_SLATTACH)       += net-tools
dir_$(CONFIG_USER_NET_TOOLS_IPMADDR)        += net-tools
dir_$(CONFIG_USER_NET_TOOLS_IPTUNNEL)       += net-tools
dir_$(CONFIG_USER_NET_TOOLS_MII_TOOL)       += net-tools
dir_$(CONFIG_USER_NETSTAT_NAT_NETSTAT_NAT)  += netstat-nat
dir_$(CONFIG_USER_NULL_NULL)                += null
dir_$(CONFIG_USER_NWSH_SH)                  += nwsh
dir_$(CONFIG_USER_OGGPLAY_EXAMPLE)          += oggplay
dir_$(CONFIG_USER_OGGPLAY_OGG123)           += oggplay
dir_$(CONFIG_USER_OPENSSL_APPS)             += openssl
dir_$(CONFIG_USER_IPSEC_TOOLS)              += ipsec-tools-0.8.2
dir_$(CONFIG_USER_P0F)			    += p0f
dir_$(CONFIG_USER_P7ZIP_7ZA)                += p7zip
dir_$(CONFIG_USER_PALMBOT_PALMBOT)          += palmbot
dir_$(CONFIG_USER_PCIUTILS_LSPCI)           += pciutils
dir_$(CONFIG_USER_PCIUTILS_SETPCI)          += pciutils
dir_$(CONFIG_USER_PCMCIA_CS)                += pcmcia-cs
dir_$(CONFIG_USER_PCSCLITE)                 += pcsc
dir_$(CONFIG_USER_PERL_PERL)                += perl
dir_$(CONFIG_USER_PING_PING)                += ping
dir_$(CONFIG_USER_PKTDELAY_PKTDELAY)        += pktdelay
dir_$(CONFIG_USER_PLAY_PLAY)                += play
dir_$(CONFIG_USER_PLAY_TONE)                += play
dir_$(CONFIG_USER_PLAYRT_PLAYRT)            += playrt
dir_$(CONFIG_USER_PLUG_PLUG)                += plug
dir_$(CONFIG_USER_PORTMAP_PORTMAP)          += portmap
#dir_$(CONFIG_USER_PPPOE_PROXY)              += pppd
#dir_$(CONFIG_USER_PPTP_CLIENT_PPTP)         += pptp-client
#dir_$(CONFIG_USER_PPTP_CLIENT_PPTP_CALLMGR) += pptp-client
#dir_$(CONFIG_USER_PPTPD_PPTPCTRL)           += pptpd
#dir_$(CONFIG_USER_PPTPD_PPTPD)              += pptpd
dir_$(CONFIG_USER_PPTP_SERVER)              += pptp-server
dir_$(CONFIG_USER_PROCPS_FREE)              += procps
dir_$(CONFIG_USER_PROCPS_KILL)              += procps
dir_$(CONFIG_USER_PROCPS_PGREP)             += procps
dir_$(CONFIG_USER_PROCPS_PKILL)             += procps
dir_$(CONFIG_USER_PROCPS_PMAP)              += procps
dir_$(CONFIG_USER_PROCPS_PS)                += procps
dir_$(CONFIG_USER_PROCPS_PWDX)              += procps
dir_$(CONFIG_USER_PROCPS_SKILL)             += procps
dir_$(CONFIG_USER_PROCPS_SLABTOP)           += procps
dir_$(CONFIG_USER_PROCPS_SNICE)             += procps
dir_$(CONFIG_USER_PROCPS_SYSCTL)            += procps
dir_$(CONFIG_USER_PROCPS_TLOAD)             += procps
dir_$(CONFIG_USER_PROCPS_TOP)               += procps
dir_$(CONFIG_USER_PROCPS_UPTIME)            += procps
dir_$(CONFIG_USER_PROCPS_VMSTAT)            += procps
dir_$(CONFIG_USER_PROCPS_W)                 += procps
dir_$(CONFIG_USER_PROCPS_WATCH)             += procps
dir_$(CONFIG_USER_HTOP)                     += htop
dir_$(CONFIG_USER_PYTHON_PYTHON)            += python
ifeq ($(CROSS_TARGET), aarch64-linux)
dir_$(CONFIG_USER_RADVD)                   += radvd-2.17
else
dir_$(CONFIG_USER_RADVD_1_8)               += radvd-1.8
dir_$(CONFIG_USER_RADVD_2_17)              += radvd-2.17
endif
dir_$(CONFIG_USER_RTK_RAMONITOR)		   += rtk_ramonitor
dir_$(CONFIG_USER_RARP)			    += rarp
dir_$(CONFIG_USER_NDPPD)		    += ndppd
dir_$(CONFIG_USER_RDATE_RDATE)              += rdate
dir_$(CONFIG_USER_RAMIMAGE_RAMFS64)         += ramimage
dir_$(CONFIG_USER_RAMIMAGE_RAMFS128)        += ramimage
dir_$(CONFIG_USER_RAMIMAGE_RAMFS256)        += ramimage
dir_$(CONFIG_USER_RAMIMAGE_RAMFS512)        += ramimage
dir_$(CONFIG_USER_READPROFILE_READPROFILE)  += readprofile
dir_$(CONFIG_USER_RECOVER_RECOVER)          += recover
dir_$(CONFIG_USER_REISERFSPROGS)            += reiserfsprogs
dir_$(CONFIG_USER_ROOTLOADER_ROOTLOADER)    += rootloader
dir_$(CONFIG_USER_ROUTE_ARP)                += route
dir_$(CONFIG_USER_ROUTE_IFCONFIG)           += route
dir_$(CONFIG_USER_ROUTE_MIITOOL)            += route
dir_$(CONFIG_USER_ROUTE_ROUTE)              += route
dir_$(CONFIG_USER_ROUTE_NETSTAT)            += route
dir_$(CONFIG_USER_ROUTED_ROUTED)            += routed
dir_$(CONFIG_USER_QUAGGA)                   += quagga
dir_$(CONFIG_USER_RP_PPPOE_PPPOE)           += rp-pppoe
#dir_$(CONFIG_USER_PPPOE_PROXY)              += rp-pppoe
dir_$(CONFIG_USER_RTC_M41T11)               += rtc-m41t11
dir_$(CONFIG_USER_RTC_DS1302)               += rtc-ds1302
ifeq ("$(CONFIG_USER_KSMBD)","y")
dir_$(CONFIG_USER_KSMBD)              		+= ksmbd
else
dir_$(CONFIG_USER_SAMBA_3_0_24)             += samba-3.0.24
dir_$(CONFIG_USER_SAMBA_3_6_24)              += samba
endif
#dir_$(CONFIG_USER_SAMBAAPI)					+= sambaapi
dir_$(CONFIG_USER_SASH_REBOOT)              += sash
dir_$(CONFIG_USER_SASH_SH)                  += sash
dir_$(CONFIG_USER_SASH_SHUTDOWN)            += sash
dir_$(CONFIG_USER_SASH_SHUTDOWN)            += sash
dir_$(CONFIG_USER_SETHDLC_SETHDLC)          += sethdlc
dir_$(CONFIG_USER_SETSERIAL_SETSERIAL)      += setserial
dir_$(CONFIG_USER_SH_SH)                    += sh
dir_$(CONFIG_USER_SHUTILS_BASENAME)         += shutils
dir_$(CONFIG_USER_SHUTILS_DATE)             += shutils
dir_$(CONFIG_USER_SHUTILS_DIRNAME)          += shutils
dir_$(CONFIG_USER_SHUTILS_ECHO)             += shutils
dir_$(CONFIG_USER_SHUTILS_FALSE)            += shutils
dir_$(CONFIG_USER_SHUTILS_LOGNAME)          += shutils
dir_$(CONFIG_USER_SHUTILS_PRINTENV)         += shutils
dir_$(CONFIG_USER_SHUTILS_PWD)              += shutils
dir_$(CONFIG_USER_SHUTILS_TRUE)             += shutils
dir_$(CONFIG_USER_SHUTILS_UNAME)            += shutils
dir_$(CONFIG_USER_SHUTILS_WHICH)            += shutils
dir_$(CONFIG_USER_SHUTILS_WHOAMI)           += shutils
dir_$(CONFIG_USER_SHUTILS_YES)              += shutils
dir_$(CONFIG_USER_SLATTACH_SLATTACH)        += slattach
dir_$(CONFIG_USER_SMBMOUNT_SMBMOUNT)        += smbmount
dir_$(CONFIG_USER_SMBMOUNT_SMBUMOUNT)       += smbmount
dir_$(CONFIG_USER_SMTP_SMTPCLIENT)          += smtpclient
dir_$(CONFIG_USER_SNMPD_SNMPD)              += snmpd
dir_$(CONFIG_USER_SNMPD_SNMPD_V2C)          += snmpdv2c
dir_$(CONFIG_USER_SNMPD_SNMPD_V2CTRAP)      += snmpdv2cTrap
dir_$(CONFIG_USER_SNMPD_5_7_3)		    += snmp-5.7.3
dir_$(CONFIG_USER_SNMPD_5_9_4)		    += net-snmp-5.9.4
dir_$(CONFIG_RTK_HOST_SPEEDUP)		    += speedtest
dir_$(CONFIG_USER_STUNNEL_STUNNEL)          += stunnel
dir_$(CONFIG_USER_SQUID_SQUID)              += squid
dir_$(CONFIG_USER_SSH_DROPBEAR)             += dropbear
dir_$(CONFIG_USER_SSH_SSHD)                 += ssh
dir_$(CONFIG_USER_SSH_SSH)                  += ssh
dir_$(CONFIG_USER_SSH_SSHKEYGEN)            += ssh
dir_$(CONFIG_USER_OPENSSH_SFTP)            	+= openssh
dir_$(CONFIG_USER_STP_STP)                  += stp
dir_$(CONFIG_USER_STUN_CLIENT) 		    += stun
dir_$(CONFIG_USER_STRACE_STRACE)            += strace
dir_$(CONFIG_USER_STRACE_STRACE_5_10)       += strace-5.10
dir_$(CONFIG_USER_STRACE_STRACE_5_18)       += strace-5.18
dir_$(CONFIG_USER_STTY_STTY)                += stty
dir_$(CONFIG_USER_SYSUTILS_DF)              += sysutils
dir_$(CONFIG_USER_SYSUTILS_FREE)            += sysutils
dir_$(CONFIG_USER_SYSUTILS_HOSTNAME)        += sysutils
dir_$(CONFIG_USER_SYSUTILS_KILL)            += sysutils
dir_$(CONFIG_USER_SYSUTILS_PS)              += sysutils
dir_$(CONFIG_USER_SYSUTILS_REBOOT)          += sysutils
dir_$(CONFIG_USER_SYSUTILS_SHUTDOWN)        += sysutils
dir_$(CONFIG_USER_SYSKLOD)					+= sysklogd-1.5.1
dir_$(CONFIG_USER_TCPBLAST_TCPBLAST)        += tcpblast
dir_$(CONFIG_USER_TCPWRAP_TCPD)             += tcpwrappers
dir_$(CONFIG_USER_TCPDUMP_TCPDUMP)          += tcpdump
dir_$(CONFIG_USER_TCPDUMP_WEB)              += libpcap
dir_$(CONFIG_USER_TCPDUMP_WEB)              += tcpdump_web
dir_$(CONFIG_USER_TCSH_TCSH)                += tcsh
dir_$(CONFIG_USER_TELNET_TELNET)            += telnet
dir_$(CONFIG_USER_TELNETD_TELNETD)          += telnetd
dir_$(CONFIG_USER_TFTP_TFTP)                += tftp
dir_$(CONFIG_USER_TFTPD_TFTPD)              += tftpd
dir_$(CONFIG_USER_THREADDEMOS_THREADDEMOS)  += threaddemos
dir_$(CONFIG_USER_THTTPD_THTTPD)            += thttpd
dir_$(CONFIG_USER_GLIBC_TIMEZONE)           += timezone
dir_$(CONFIG_USER_TINYLOGIN_CRYPT_CRYPT)    += tinylogin
dir_$(CONFIG_USER_TINYLOGIN_TINYLOGIN)      += tinylogin
dir_$(CONFIG_USER_TIP_TIP)                  += tip
dir_$(CONFIG_USER_TIMEPEG_TPT)              += tpt
dir_$(CONFIG_USER_TRACEROUTE_TRACEROUTE)    += traceroute
dir_$(CONFIG_USER_3G_USBTOOLS)              += usbtools
dir_$(CONFIG_USER_USBMOUNT_USBMOUNT)        += usbmount
dir_$(CONFIG_USER_USBMOUNT_USBMOUNT)        += util-linux
dir_$(CONFIG_USER_UCDSNMP_SNMPD)            += ucdsnmp
dir_$(CONFIG_USER_VERSION_VERSION)          += version
dir_$(CONFIG_USER_W3CAM_VIDCAT)             += w3cam
dir_$(CONFIG_USER_VALGRIND)		   			+= valgrind
dir_$(CONFIG_USER_VPLAY_VPLAY)              += vplay
dir_$(CONFIG_USER_VPLAY_VREC)               += vplay
dir_$(CONFIG_USER_VPLAY_MIXER)              += vplay
dir_$(CONFIG_USER_VPNLED_VPNLED)            += vpnled
dir_$(CONFIG_USER_LEDCON_LEDCON)            += ledcon
dir_$(CONFIG_USER_W3CAM_W3CAMD)             += w3cam
dir_$(CONFIG_USER_WAPI)                     += wapi-utils
dir_$(CONFIG_USER_WGET_1_20)                += wget-1.20
dir_$(CONFIG_USER_WGET_1_21_4)              += wget-1.21.4
dir_$(CONFIG_USER_WGET_MANAGE)              += wget_manage
dir_$(CONFIG_USER_WSDD2)		    += wsdd2
dir_$(CONFIG_USER_AXEL)                     += axel-2.4
dir_$(CONFIG_USER_WATCHDOG_WDG)             += watchdog
ifeq ($(CONFIG_USER_WIRELESS_TOOLS),y)
ifeq ($(CONFIG_USER_WIRELESS_TOOLS_29),y) # if select version 29, ignore origin wireless_tools
	dir_y			+= wireless_tools.29
else
	dir_y			+= wireless_tools
endif # Use CONFIG_USER_WIRELESS_TOOLS_29
endif
dir_$(CONFIG_USER_IW)                       += iw
dir_$(CONFIG_USER_WIRELESS_REGDB)           += wireless-regdb
dir_$(CONFIG_USER_XMLCONFIG)                += xmlconfig
dir_$(CONFIG_USER_WIRELESS_TOOLS_RTL8185_AUTH) += auth
#dir_$(CONFIG_USER_WIRELESS_TOOLS_RTL8185_AUTH)           += ../linux-2.4.x/drivers/net/wireless/RTL8185_AP/auth/src
#dir_$(CONFIG_USER_WIRELESS_TOOLS_RTL8185_AUTH)           += ../linux-2.4.x/drivers/net/wireless/RTL8185_AP/auth/src/dlisten
dir_$(CONFIG_USER_WIRELESS_TOOLS_RTL8185_IAPP)           += IAPP
dir_$(CONFIG_USER_Y1731)					+= y1731-0.0.1
dir_$(CONFIG_USER_8023AH)                   += 8023ah
dir_$(CONFIG_USER_XL2TPD)		    += xl2tpd
#dir_$(CONFIG_USER_PPTP_CLIENT)		    += pptp-1.10.0	# replace by pptp plugin of pppd
dir_$(CONFIG_USER_ZEBRA_BGPD_BGPD)          += zebra
dir_$(CONFIG_USER_ZEBRA_OSPFD_OSPFD)        += zebra
dir_$(CONFIG_USER_ZEBRA_RIPD_RIPD)          += zebra
dir_$(CONFIG_USER_ZEBRA_ZEBRA_ZEBRA)        += zebra
dir_$(CONFIG_USER_SFREE)                    += sfree
dir_$(CONFIG_USER_TINYMEMBENCH)             += tinymembench
dir_$(CONFIG_RTL_LINK_SUPPORT)              += rtl_link
dir_$(CONFIG_USER_MOSQUITTO)                += mosquitto
# CMCC app
ifeq ($(CONFIG_RTK_DEV_AP),y)
ifeq ($(CONFIG_CMCC_AP),y)
ifeq ($(CONFIG_USER_AP_CMCC),y)
dir_$(CONFIG_CMCC)                          += cmccap
else
dir_$(CONFIG_CMCC)                          += cmcc
endif #end CONFIG_USER_AP_CMCC
dir_$(CONFIG_CU)                            += cuap
endif
dir_$(CONFIG_CU_AP)                         += cuap
else
dir_$(CONFIG_CMCC)                          += cmcc
#end CMCC
# CU app
dir_$(CONFIG_CU_BASEON_CMCC)                += cmcc
dir_$(CONFIG_CU)                          	+= cu
endif
#end CMCC
dir_$(CONFIG_USER_CWMP_TR069)               += cwmp-tr069
dir_$(CONFIG_USER_CWMP_UPNP_DM)             += upnpmd-cp
#add by xl_yue for midware interface
dir_$(CONFIG_USER_DOT1AG_UTILS)             += dot1ag-utils libpcap

dir_$(CONFIG_TR142_MODULE)                  += rtk_tr142
dir_$(CONFIG_USER_RTK_VOIP)                 += rtk_voip/rtk_libomci_voip
dir_$(CONFIG_USER_RTK_FASTPASSNF_MODULE)    += rtk_fastPassNF
dir_$(CONFIG_USER_RTK_FAST_BR_PASSNF_MODULE) += rtk_fastBrPassNF

dir_$(CONFIG_USER_RTK_HWCRYPTO_MODULE)	    += rtk_HwCrypto

dir_$(CONFIG_USER_RTK_I2S_HOOK_MODULE) 	    += rtk_I2sHook
dir_$(CONFIG_USER_RTK_SPEEDTEST_APP) += rtk_speedtest
dir_$(CONFIG_USER_RTK_DIAGNOSTIC_APP) += rtk_diagnostic
dir_$(CONFIG_USER_ALSA_UTILS)               += alsa_utility
dir_$(CONFIG_QTNA_WIFI_AX)			+= qtna_wifi_ax2

ifeq ($(CONFIG_RTK_PE), y)
ifeq ($(CONFIG_RTK_SOC_TAURUS_ELNATH_COMPATIBLE), y)
dir_y += rboot
dir_y += rtk_vboot
else ifeq ($(CONFIG_ARCH_CORTINA_VENUS), y)
dir_y += rboot
else ifeq ($(CONFIG_ARCH_REALTEK_TAURUS), y)
dir_y += rboot
else
dir_y += rtk_vboot
endif
endif

ifeq ($(CONFIG_USER_RTL_WFO_MONITOR), y)
dir_y += rtl_wfo_mon
endif

#add by alex_huang netlogger
#dir_$(CONFIG_USER_NETLOGGER_SUPPORT)+= netlogger
#dir_y += games
dir_$(CONFIG_RTL8672_SAR) += sarctl
dir_$(CONFIG_DEV_xDSL) += adslctrl
dir_$(CONFIG_USER_ETHCTL) += ethctl
dir_$(CONFIG_RTL_SMUX_DEV) += smuxdev
dir_$(CONFIG_USER_WIRED_8021X) += hostapd
dir_$(CONFIG_RTL8685_PTM_MII) += ptmctl
dir_$(CONFIG_USER_WIRELESS_TOOLS) += wlan_daemon
dir_$(CONFIG_USB_RTL8672_USBCTL) += usbctl
#dir_y += goahead-2.1.4

dir_$(CONFIG_USER_DM) += richinfo-lwm2msdk-M
dir_$(CONFIG_USER_CONNTRACK_TOOLS) += conntrack-tools

#dir_y += wsc
#dir_y += mini_upnp
dir_$(CONFIG_USER_LANNETINFO) += lanNetInfo
dir_$(CONFIG_USER_OTHER_SH) += msh
dir_$(CONFIG_USER_UDHCP099PRE2) += udhcp-0.9.9-pre2
dir_$(CONFIG_AUTO_DHCP_CHECK)   += auto_dhcp
dir_$(CONFIG_USER_SPPPD)	+= spppd
dir_$(CONFIG_USER_PPP_MIXED)	+= pppd		# PPP Mixd Mode, spppd(master) + pppd
dir_$(CONFIG_USER_PPPD)		+= pppd
ifneq ($(CONFIG_USER_DATA_ELEMENTS_AGENT)$(CONFIG_USER_DATA_ELEMENTS_COLLECTOR),)
dir_y +=data_elements
endif
ifneq ($(CONFIG_USER_PHDDNS),y)
dir_$(CONFIG_USER_DDNS)			   +=updatedd-2.5
endif
dir_$(CONFIG_USER_DDNS)			   +=updateddctrl
dir_$(CONFIG_USER_PHDDNS)		   +=phddns
ifeq ($(CONFIG_IGMP_MLD_COMMON_SUPPORT),y)
dir_$(CONFIG_USER_IGMPPROXY)	+=mcast-tools
else
dir_$(CONFIG_USER_IGMPPROXY)	+=igmpproxy
endif
dir_$(CONFIG_USER_VSNTP)                   += vsntp
dir_$(CONFIG_USER_UPNPD)		   +=upnpd
dir_$(CONFIG_USER_UPNPD)		   +=upnpctrl
dir_$(CONFIG_USER_MINI_UPNPD)	+= mini_upnp
dir_$(CONFIG_USER_MINIUPNPD)		   +=miniupnpd
dir_$(CONFIG_USER_MINIUPNPD)		   +=upnpctrl
dir_$(CONFIG_WIFI_SIMPLE_CONFIG)   += mini_upnp
dir_$(CONFIG_WIFI_SIMPLE_CONFIG)   += wsc
ifndef CONFIG_ARCH_LUNA_SLAVE
ifeq ($(CONFIG_RTK_DEV_AP),y)
#dir_$(CONFIG_LIB_COAP)             += libcoap-master
dir_$(CONFIG_RTL_LINK_SUPPORT)     += rtl_link
#dir_$(CONFIG_ANDLINK_SUPPORT)      += andlink
#dir_$(CONFIG_ANDLINK_IF5_SUPPORT)  += andlink/andlink-if5
#dir_$(CONFIG_ELINKSDK_SUPPORT)	   += elink_cloud
#dir_$(CONFIG_ELINK_SUPPORT)     += elink
else
dir_$(CONFIG_WIFI_SIMPLE_CONFIG)	+= sysconf
endif
dir_$(CONFIG_RTL_11R_SUPPORT) += ft_daemon
dir_$(CONFIG_RTL_DOT11K_SUPPORT) += dot11k/daemon
endif
dir_$(CONFIG_DOT11K_TEST_TOOL) += dot11k/test_11k
dir_$(CONFIG_DOT11V_TEST_TOOL) += dot11v/test_11v
ifeq ($(CONFIG_USER_RTK_WLAN_MANAGER),y)
dir_y += wlan_manager
else
dir_$(CONFIG_WLAN_MANAGER) += wlan_manager
dir_$(CPTCFG_WLAN_MANAGER) += wlan_manager
endif
dir_$(CONFIG_RTL_MESH_SUPPORT) += 11s-PathSelection
ifndef CONFIG_STA_ROAMING_CHECK
#dir_$(CONFIG_RTK_SMART_ROAMING)+= opencapwap
endif
dir_$(CONFIG_RTL_CROSSBAND_REPEATER_SUPPORT) += crossband/daemon
dir_$(CONFIG_RTL_HS2_SUPPORT)  += hs2

MULTIAP_SRC_DIR = multi-ap
ifeq ($(CONFIG_CMCC),y)
ifneq ($(CONFIG_RTK_DEV_APROUTER_ONLY),y)
MULTIAP_RELEASE_DIR = multi-ap_release-v4_2_cmcc
else
MULTIAP_RELEASE_DIR = multi-ap_release
endif
else
ifneq (, $(filter y, $(CONFIG_RTL8934AE)))
ifneq ($(CONFIG_RTK_DEV_APROUTER_ONLY),y)
ifneq (, $(filter y, $(CONFIG_RTW_MULTI_AP_R6)))
MULTIAP_RELEASE_DIR = multi-ap_release
else
MULTIAP_RELEASE_DIR = multi-ap_release-r5
endif
else
MULTIAP_RELEASE_DIR = multi-ap_release
dir_$(CONFIG_USER_RTK_MULTI_AP) += map_launcher
endif
else
ifeq ($(CONFIG_USER_HAPD),y)
ifeq ($(CONFIG_E8B),y)
ifneq (, $(filter y, $(CONFIG_CTC_AP) $(CONFIG_CMCC_AP) $(CONFIG_CU_AP)))
MULTIAP_RELEASE_DIR = multi-ap_release
dir_$(CONFIG_USER_RTK_MULTI_AP) += map_launcher
else
MULTIAP_RELEASE_DIR = multi-ap_release-u2.2.0
endif
else
MULTIAP_RELEASE_DIR = multi-ap_release
dir_$(CONFIG_USER_RTK_MULTI_AP) += map_launcher
endif
else
MULTIAP_RELEASE_DIR = multi-ap_release-u2.2.0
endif
endif
endif
MULTIAP_BUILD_SRC = $(shell test -d $(MULTIAP_SRC_DIR) && echo -n y)
ifeq ($(MULTIAP_BUILD_SRC),y)
dir_$(CONFIG_USER_RTK_MULTI_AP)	+= $(MULTIAP_SRC_DIR)
endif
dir_$(CONFIG_USER_RTK_MULTI_AP)	+= $(MULTIAP_RELEASE_DIR)
dir_$(CONFIG_USER_RTK_MULTI_AP_INFO_HUB) += multi_ap_infohub
dir_$(CONFIG_USER_RTK_MULTI_AP)	+= sysconf
dir_$(CONFIG_USER_RTK_MULTI_AP_WFA) += control_agent

dir_$(CONFIG_USER_THERMAL_CONTROL) += thermal_control
dir_$(CONFIG_USER_RTIC_CONTROL) += rtic_ctl
dir_$(CONFIG_USER_MINIDLNA)                +=minidlna
ifeq ($(CONFIG_RTK_DEV_AP),y)
dir_$(CONFIG_WIFI_SIMPLE_CONFIG)   += auth/src/dlisten/
else
#dir_$(CONFIG_WIFI_SIMPLE_CONFIG)	+= auth
endif
dir_$(CONFIG_USB_RTL8192SU_SOFTAP) += rtl8192su

#august for ethtool
dir_$(CONFIG_ETHTOOL) += ethtool-2.6.36

ifeq ($(CONFIG_USER_WT_146),y)
dir_e +=bfd
else
dir_$(CONFIG_USER_WT_146) +=bfd
endif

dir_$(CONFIG_USER_BFDD) += bfdd

# Added by Mason Yu for p2r_test
dir_$(CONFIG_USER_MFCV6D) += mfcv6

ifeq ($(CONFIG_IGMP_MLD_COMMON_SUPPORT),y)
dir_$(CONFIG_USER_MLDPROXY)	+=mcast-tools
else
dir_$(CONFIG_USER_MLDPROXY)	+=mldproxy
endif

# debug scripts
dir_$(CONFIG_USER_DBG_SCRIPTS) += dbg_scripts

# added by piyajee_chen 2016-05-18
dir_$(CONFIG_USER_LINK_CHANGE) += linkchange

#nfbi
dir_$(CONFIG_USER_NFBI) += rtl867x_nfbi

dir_$(CONFIG_USER_RTL8686_IPC_UTIL) += aipc_util

dir_$(CONFIG_USER_LOG_WRITER) += log_writer

dir_$(CONFIG_USER_TCP2DEV)          += tcp2dev
dir_$(CONFIG_USER_RTK_SYSTEMD)		+= systemd
dir_$(CONFIG_USER_RTK_ONUINFO)		+= rtk_onuinfo
dir_$(CONFIG_USER_SYNC_SERVER)		+= syncserv
dir_$(CONFIG_USER_RTK_SYSCONF)		+= sysconf
dir_$(CONFIG_USER_RTK_ONUCOMM)		+= onucomm
dir_$(CONFIG_USER_RTK_LEDTEST)		+= ledtest
dir_$(CONFIG_USER_OLTCLI)			+= oltcli
dir_$(CONFIG_USER_RTKOLTBOSA)		+= rtkoltbosa

dir_$(CONFIG_YUEME)		+= ctcyueme
dir_$(CONFIG_CU_BASEON_YUEME)		+= ctcyueme
#end yueme
dir_$(CONFIG_USER_USBINOUT)                 += usbinout
dir_$(CONFIG_USER_STRONGSWAN)	+= strongswan
dir_$(CONFIG_CORTINA_CRYPTO)	+= spacc-epn1613

## SDN Project
dir_$(CONFIG_USER_OPENVSWITCH)    += openvswitch
dir_$(CONFIG_CTC_SDN)    += ctcsdn
## end SDN

ifeq ($(CROSS_TARGET),aarch64-linux)
#G3_eng A53(aarch64), user HotSpot (server mode)
dir_$(CONFIG_USER_OPENJDK8) += openjdk8-ARM64
else
ifeq ($(CROSS_TARGET),arm-linux)
dir_$(CONFIG_USER_OPENJDK8) += openjdk8-ARM64
else
dir_$(CONFIG_USER_OPENJDK8)             += openjdk8-jamvm-2016-12
endif
endif

dir_$(CONFIG_MODERN_TRACEROUTE)    += traceroute-2.0.21
ifeq ($(CONFIG_USER_BOA_SRC_BOA),y)
dir_$(CONFIG_USER_RTK_VOIP)                     += voipflashcmd
endif
dir_$(CONFIG_USER_SDCARD_DETECT)	+= sdcard
dir_$(CONFIG_USER_LED_ONOFF)	+= led_onoff
# awifi app
#dir_$(CONFIG_USER_AWIFI_SUPPORT)	+= wifidog
#end awifi

# TR-369
dir_$(CONFIG_USER_LIBWEBSOCKETS) += libwebsockets
dir_$(CONFIG_USER_MOSQUITTO) += mosquitto
dir_$(CONFIG_USER_SQLITE) += sqlite
dir_$(CONFIG_USER_USP_TR369) += usp-tr369
#end TR-369

dir_$(CONFIG_STATIC_CONFIG) += squashfs-tools
dir_$(CONFIG_USER_MEMTESTER)    += memtester-4.3.0
dir_$(CONFIG_USER_MEMTESTER_4_5)    += memtester-4.5.0

#bluetooth
dir_$(CONFIG_USER_RTK_HCIATTACH)		+= rtk_hciattach
dir_$(CONFIG_USER_RTK_BLUETOOTH_FIRMWARE)	+= rtk_bluetooth_firmware
dir_$(CONFIG_USER_RTK_BLUETOOTH_MP_TOOL)	+= rtk_bluetooth_mp_tool

#gphy
dir_${CONFIG_LIB_8226B}		+= lib8226b
ifeq ($(CONFIG_LIB_8226B),y)
dir_$(CONFIG_USER_RTK_RTL8226B)	+= rtl8226b
endif
dir_${CONFIG_LIB_8221B}		+= lib8221b
ifeq ($(CONFIG_LIB_8221B),y)
dir_$(CONFIG_USER_RTK_RTL8221B)	+= rtl8221b
endif

#ctcapd
#dir_$(CONFIG_USER_CTCAPD)	+= ctcapd
#dir_$(CONFIG_USER_CTCAPD_TEST)	+= ctcapd/test
#ifdef CONFIG_RTK_SOC_RTL8198D
#dir_$(CONFIG_USER_CTC_EOS_MIDDLEWARE)	+= e-OS_middleware/RTL8198D
#endif
dir_$(CONFIG_RTK_DEV_AP)                    += ctcap
dir_$(CONFIG_USER_DUMP_CLTS_INFO_ON_BR_MODE) += rtl_cltsInfoOnBRMode

dir_$(CONFIG_USER_RTK_NOSTB_IPTV)	+= udpxy xupnpd

dir_$(CONFIG_USER_WAN_PORT_AUTO_SELECTION)	+=rtl_wan_auto_selection

dir_$(CONFIG_RTK_WFO) += rtk_wfo

all: prepare voip_prepare
#ifeq ($(CONFIG_USER_RTK_VOIP),y)
ifneq ($(CONFIG_RTK_DEV_AP),y)
	[ ! -d rtk_voip ] || $(MAKE) -C rtk_voip prepare
endif
#endif
#like above busybox1124
ifeq ($(CONFIG_USER_BUSYBOX_BUSYBOX1221),y)
	$(MAKE) -C busybox-1.22.1 include/autoconf.h
endif

ifeq ($(CONFIG_USER_DMALLOC), y)
	cd dmalloc-5.5.2 && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --build=i686-pc-linux-gnu --disable-cxx --enable-threads && cd -
endif

ifeq ($(CONFIG_USER_SNMPD_SNMPD_V3),y)
ifeq ($(CONFIG_USER_SNMPD_5_7_3),y)
	# run ./rtk_configure.sh to set net-snmp customized configuration
	cd snmp-5.7.3 && chmod u+x ./rtk_configure.sh && chmod u+x ./configure && ./rtk_configure.sh
endif
ifeq ($(CONFIG_USER_SNMPD_5_9_4),y)
	tar -xvf net-snmp-5.9.4.tar.gz && cd net-snmp-5.9.4/ \
	&& ./configure --host=$(CROSS_HOST)  --target=$(CROSS_TARGET) --disable-perl-cc-checks --without-perl-modules --disable-applications --enable-static --disable-shared --with-default-snmp-version="3" --with-sys-contact="a@b.c"  --with-sys-location="test" --with-logfile="/var/log/snmpd.log" --with-persistent-directory="/var/net-snmp" \
	&& sed -i 's/-lperl//g' agent/Makefile \
	&& sed -i 's/-lperl//g' apps/Makefile \
	&& sed -i '$$a\\n' Makefile \
	&& sed -i '$$a\EXEC = snmpd' Makefile \
	&& sed -i '$$a\romfs:' Makefile \
	&& sed -i '$$a\	$$(ROMFSINST) agent/$$(EXEC) /bin/$$(EXEC)' Makefile
endif
endif
ifeq ($(CONFIG_USER_NFTABLES),y)
	cd nftables && chmod u+x ./configure 
endif
ifneq ($(CROSS_TARGET),)
	for i in $(dir_v) $(dir_y) $(dir_e); do \
		if [ ! -f $$i/Makefile ]; then \
			if [ "$$i" = "iptables-1.4.18" ]; then \
				cd $$i && ./configure --enable-static --enable-shared --host=$(CROSS_HOST) --target=$(CROSS_TARGET) $(IPTABLES_1_4_18_CONFIG_OPT) && cd -; \
			elif [ "$$i" = "iptables-1.4.21" ]; then \
				cd $$i && ./configure --enable-static --enable-shared --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && cd -; \
			elif [ "$(@:_only=)" = "gdb-7.12" ]; then \
				cd $(@:_only=) && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && cd -; \
			elif [ "$$i" = "ipsec-tools-0.7" ]; then \
				cd $$i && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --with-kernel-headers="$(ROOTDIR)/$(LINUXDIR)/user_headers/include" \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --enable-security-context=no && cd -; \
			elif [ "$$i" = "ipsec-tools-0.8.2" ]; then \
				cd $$i && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --with-kernel-headers="$(ROOTDIR)/$(LINUXDIR)/user_headers/include" \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --enable-ipv6=yes --enable-security-context=no && cd -; \
			elif [ "$$i" = "radvd-2.17" ]; then \
				cd $$i && touch aclocal.m4 && touch -c -d '-1 hour' configure.ac && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"  && cd -; \
			elif [ "$$i" = "dhcp-4.1.1" -o "$$i" = "dhcp-4.4.1" ]; then \
				cd $$i && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) LIBS="$(RECONF_LIBS)" CFLAGS="$(CFLAGS) -fcommon -include config/autoconf.h" && cd -; \
			elif [ "$$i" = "libcoap-master" ]; then \
				cd $$i && autoreconf -ivf && ./configure --disable-shared --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && touch Makefile.in && touch configure && $(MAKE) clean && echo "romfs:" >> ./Makefile.am && cd -; \
			elif [ "$$i" = "sntpv6" ]; then \
				cd $$i && ./autogen.sh && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" && cd -; \
			elif [ "$$i" = "wget-1.20" ]; then \
				cd $$i && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --with-ssl=openssl  \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --without-zlib --disable-pcre2 --disable-pcre --with-libssl-prefix="$(ROOTDIR)/lib/$(openssl_dir)" && cd -; \
			else \
				cd $$i && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && cd -; \
			fi \
		fi; \
		$(MAKE) -C $$i || exit $?; \
	done
else
	for i in $(dir_v) $(dir_y) $(dir_e); do \
		if [ ! -f $$i/Makefile ]; then \
			if [ "$$i" = "iptables-1.4.18" ]; then \
				cd $$i && ./configure --enable-static --enable-shared --host=mips-linux --target=mips-linux $(IPTABLES_1_4_18_CONFIG_OPT) && cd -; \
			elif [ "$$i" = "iptables-1.4.21" ]; then \
				cd $$i && ./configure --enable-static --enable-shared --host=mips-linux --target=mips-linux && cd -; \	
			elif [ "$(@:_only=)" = "gdb-7.12" ]; then \
				cd $(@:_only=) && ./configure --host=mips-linux --target=mips-linux && cd -; \
			elif [ "$$i" = "ipsec-tools-0.7" ]; then \
				cd $$i && ./configure --host=mips-linux --target=mips-linux --with-kernel-headers="$(ROOTDIR)/$(LINUXDIR)/user_headers/include" \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --enable-security-context=no && cd -; \
			elif [ "$$i" = "ipsec-tools-0.8.2" ]; then \
				cd $$i && ./configure --host=mips-linux --target=mips-linux --with-kernel-headers="$(ROOTDIR)/$(LINUXDIR)/user_headers/include" \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --enable-ipv6=yes --enable-security-context=no && cd -; \
			elif [ "$$i" = "dhcp-4.1.1" -o "$$i" = "dhcp-4.4.1" ]; then \
				cd $$i && ./configure --host=mips-linux --target=mips-linux LIBS="$(RECONF_LIBS)" CFLAGS="$(CFLAGS) -fcommon -include config/autoconf.h" && cd -; \
			elif [ "$$i" = "libcoap-master" ]; then \
				cd $$i && autoreconf -ivf && ./configure --disable-shared --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && touch Makefile.in && touch configure && $(MAKE) clean && echo "romfs:" >> ./Makefile.am && cd -; \
			elif [ "$$i" = "sntpv6" ]; then \
				cd $$i && ./autogen.sh && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"  && cd -; \
			elif [ "$$i" = "wget-1.20" ]; then \
				cd $$i && ./configure --host=mips-linux --target=mips-linux --with-ssl=openssl  \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --without-zlib --disable-pcre2 --disable-pcre --with-libssl-prefix="$(ROOTDIR)/lib/$(openssl_dir)" && cd -; \
			else \
				cd $$i && ./configure --host=mips-linux --target=mips-linux && cd -; \
			fi \
		fi; \
		$(MAKE) -C $$i || exit $?; \
	done
endif

romfs:
	for i in $(dir_v) $(sort $(dir_y)) $(dir_e) ; do  $(MAKE) -C $$i romfs || exit $? ; done
	find $(ROMFSDIR)/bin/ -type f -exec $(STRIP) '{}' \;

.PHONY: rcX prepare
rcX:
	for i in $(dir_v) $(sort $(dir_y)) $(dir_e) ; do \
		if [ "`grep "^rcX:" $$i/Makefile`" = "rcX:" ]; then \
			 $(MAKE) -C $$i rcX;\
		fi \
	done

dev:
	$(MAKE) -C $(dir_v) dev

prepare:
ifdef CONFIG_YUEME
	$(MAKE) -C ctcyueme prepare
endif
ifdef CONFIG_CU_BASEON_YUEME
	$(MAKE) -C ctcyueme prepare
endif
ifdef CONFIG_CMCC
	$(MAKE) -C cmcc prepare
endif
ifdef CONFIG_CU_BASEON_CMCC
	$(MAKE) -C cmcc prepare
endif
ifeq ($(CONFIG_RTK_DEV_AP),y)
	$(MAKE) -C ctcap prepare
ifeq ($(CONFIG_CMCC),y)
	$(MAKE) -C cmccap prepare
endif
endif
ifeq ($(CONFIG_USER_RTK_MULTI_AP),y)
	$(MAKE) -C $(MULTIAP_RELEASE_DIR) prepare
endif

voip_prepare:
ifdef CONFIG_USER_DBUS_PROXY
	$(MAKE) -C $(ROOTDIR)/user/ctcyueme/dbusproxy/reg libsmartproxy.so
endif
ifdef CONFIG_E8B
	cd $(ROOTDIR)/user ; ln -fsn ../rtk_voip/maserati_2.1 rtk_voip ; cd - ;
else
ifdef CONFIG_TELMEX
	cd $(ROOTDIR)/user ; ln -fsn ../rtk_voip/maserati_2.2_central_america rtk_voip ; cd - ;
else	
	cd $(ROOTDIR)/user ; ln -fsn ../rtk_voip/maserati_2.0 rtk_voip ; cd - ;
endif
endif


%_menuconfig:
	[ ! -d "$(@:_menuconfig=)" ] || $(MAKE) -C $(@:_menuconfig=) menuconfig

%_only:
ifneq ($(CROSS_TARGET),)
	if [ -n "$(findstring ctcyueme/,$@)" ]; then \
		$(MAKE) -C ctcyueme $(@:ctcyueme/%=%); \
	elif [ -n "$(findstring cmcc/,$@)" ]; then \
		$(MAKE) -C cmcc $(@:cmcc/%=%); \
	elif [ -n "$(findstring cu/,$@)" ]; then \
		$(MAKE) -C cu $(@:cu/%=%); \
	elif [ -n "$(findstring ctcap/,$@)" ]; then \
		$(MAKE) -C ctcap $(@:ctcap/%=%); \
	elif [ -n "$(findstring cmccap/,$@)" ]; then \
		$(MAKE) -C cmccap $(@:cmccap/%=%); \
	elif [ -d "$(@:_only=)" ]; then \
		if [ ! -f $(@:_only=)/Makefile ]; then \
			if [ "$(@:_only=)" = "iptables-1.4.18" ]; then \
				cd $(@:_only=) && ./configure --enable-static --enable-shared --host=$(CROSS_HOST) --target=$(CROSS_TARGET) $(IPTABLES_1_4_18_CONFIG_OPT) && cd -; \
			elif [ "$(@:_only=)" = "iptables-1.4.21" ]; then \
				cd $(@:_only=) && ./configure --enable-static --enable-shared --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && cd -; \
			elif [ "$(@:_only=)" = "gdb-7.12" ]; then \
				cd $(@:_only=) && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && cd -; \
			elif [ "$(@:_only=)" = "ipsec-tools-0.7" ]; then \
				cd $(@:_only=) && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --with-kernel-headers="$(ROOTDIR)/$(LINUXDIR)/user_headers/include" \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --enable-security-context=no && cd -; \
			elif [ "$(@:_only=)" = "ipsec-tools-0.8.2" ]; then \
				cd $(@:_only=) && ./configure --host=$(CROSS_HOST)  --target=$(CROSS_TARGET)  --with-kernel-headers="$(ROOTDIR)/$(LINUXDIR)/user_headers/include" \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --enable-ipv6=yes --enable-security-context=no && cd -; \
			elif [ "$(@:_only=)" = "radvd-2.17" ]; then \
				cd $(@:_only=) && touch aclocal.m4 && touch -c -d '-1 hour' configure.ac && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" && cd -; \
			elif [ "$(@:_only=)" = "dhcp-4.1.1" -o "$(@:_only=)" = "dhcp-4.4.1" ]; then \
				cd $(@:_only=) && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) LIBS="$(RECONF_LIBS)" CFLAGS="$(CFLAGS) -fcommon -include config/autoconf.h" && cd -; \
			elif [ "$(@:_only=)" = "libcoap-master" ]; then \
				cd $(@:_only=) && autoreconf -ivf && ./configure --disable-shared --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && touch Makefile.in && touch configure && $(MAKE) clean && echo "romfs:" >> ./Makefile.am && cd -; \
			elif [ "$$i" = "sntpv6" ]; then \
				cd $$i && ./autogen.sh && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" && cd -; \
			elif [ "$(@:_only=)" = "wget-1.20" ]; then \
				cd $(@:_only=) && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) --with-ssl=openssl  \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --without-zlib --disable-pcre2 --disable-pcre --with-libssl-prefix="$(ROOTDIR)/lib/$(openssl_dir)" && cd -; \
			else \
				cd $(@:_only=) && ./configure --host=$(CROSS_HOST)  --target=$(CROSS_TARGET) && cd -; \
			fi \
		fi; \
		$(MAKE) -C $(@:_only=); \
	fi
else
	if [ -n "$(findstring ctcyueme/,$@)" ]; then \
		$(MAKE) -C ctcyueme $(@:ctcyueme/%=%); \
	elif [ -n "$(findstring cmcc/,$@)" ]; then \
		$(MAKE) -C cmcc $(@:cmcc/%=%); \
	elif [ -n "$(findstring cu/,$@)" ]; then \
		$(MAKE) -C cu $(@:cu/%=%); \
	elif [ -n "$(findstring ctcap/,$@)" ]; then \
		$(MAKE) -C ctcap $(@:ctcap/%=%); \
	elif [ -n "$(findstring cmccap/,$@)" ]; then \
		$(MAKE) -C cmccap $(@:cmccap/%=%); \
	elif [ -d "$(@:_only=)" ]; then \
		if [ ! -f $(@:_only=)/Makefile ]; then \
			if [ "$(@:_only=)" = "iptables-1.4.18" ]; then \
				cd $(@:_only=) && ./configure --enable-static --enable-shared --host=mips-linux --target=mips-linux $(IPTABLES_1_4_18_CONFIG_OPT) && cd -; \
			elif [ "$(@:_only=)" = "iptables-1.4.21" ]; then \
				cd $(@:_only=) && ./configure --enable-static --enable-shared --host=mips-linux --target=mips-linux && cd -; \	
			elif [ "$(@:_only=)" = "gdb-7.12" ]; then \
				cd $(@:_only=) && ./configure --host=mips-linux --target=mips-linux && cd -; \				
			elif [ "$(@:_only=)" = "ipsec-tools-0.7" ]; then \
				cd $(@:_only=) && ./configure --host=mips-linux --target=mips-linux --with-kernel-headers="$(ROOTDIR)/$(LINUXDIR)/user_headers/include" \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --enable-security-context=no && cd -; \
			elif [ "$(@:_only=)" = "ipsec-tools-0.8.2" ]; then \
				cd $(@:_only=) && ./configure --host=mips-linux --target=mips-linux --with-kernel-headers="$(ROOTDIR)/$(LINUXDIR)/user_headers/include" \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --enable-ipv6=yes --enable-security-context=no && cd -; \
			elif [ "$(@:_only=)" = "dhcp-4.1.1" -o "$(@:_only=)" = "dhcp-4.4.1" ]; then \
				cd $(@:_only=) && ./configure --host=mips-linux --target=mips-linux LIBS="$(RECONF_LIBS)" CFLAGS="$(CFLAGS) -include config/autoconf.h" && cd -; \
			elif [ "$(@:_only=)" = "libcoap-master" ]; then \
				cd $(@:_only=) && autoreconf -ivf && ./configure --disable-shared --host=$(CROSS_HOST) --target=$(CROSS_TARGET) && touch Makefile.in && touch configure && $(MAKE) clean && echo "romfs:" >> ./Makefile.am && cd -; \
			elif [ "$$i" = "sntpv6" ]; then \
				cd $$i && ./autogen.sh && ./configure --host=$(CROSS_HOST) --target=$(CROSS_TARGET) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" && cd -; \
			elif [ "$(@:_only=)" = "wget-1.20" ]; then \
				cd $(@:_only=) && ./configure --host=mips-linux --target=mips-linux --with-ssl=openssl  \
				--with-openssl="$(ROOTDIR)/lib/$(openssl_dir)" --without-zlib --disable-pcre2 --disable-pcre --with-libssl-prefix="$(ROOTDIR)/lib/$(openssl_dir)" && cd -; \
			else \
				cd $(@:_only=) && ./configure --host=mips-linux --target=mips-linux && cd -; \
			fi \
		fi; \
		$(MAKE) -C $(@:_only=); \
	fi
endif

%_romfs:
	[ ! -d "$(@:_romfs=)" ] || $(MAKE) -C $(@:_romfs=) romfs

%_clean:
ifeq ($(CONFIG_USER_RTK_VOIP),y)
	echo ${DIR_LINUX}
	[ ! -d rtk_voip ] || $(MAKE) -C rtk_voip prepare
endif
	[ ! -d "$(@:_clean=)" ] || $(MAKE) -C $(@:_clean=) clean

clean:
ifeq ($(CONFIG_USER_RTK_VOIP),y)
	[ ! -d rtk_voip ] || $(MAKE) -C rtk_voip prepare
endif
	-for i in $(dir_v) $(sort $(dir_y) $(dir_n) $(dir_) $(dir_e)) ; do \
		if [ $$i = "radvd-1.8" -o $$i = "iptables-1.4.18" -o $$i = "iptables-1.4.21" -o $$i = "gdb-7.12" -o $$i = "dhcp-4.1.1" -o $$i = "dhcp-4.4.1" -o $$i = "ntfs-3g" \
			-o $$i = "iperf-2.0.4-src" -o $$i = "iperf-3.1.4" -o $$i = "libcoap-master" -o $$i = "radvd-2.17" -o $$i = "sntpv6" ]; then \
			[ ! -d $$i ] || $(MAKE) -C $$i distclean; \
			[ $$i = "dhcp-4.4.1" ] || rm -rf dhcp-4.4.1/bind/bind-9.11.2-P1; \
		else \
			[ ! -d $$i ] || $(MAKE) -C $$i clean; \
		fi; \
	done
ifeq ($(CONFIG_USER_TCPDUMP_TCPDUMP),y)
	[ ! -e tcpdump/Makefile ] || rm tcpdump/Makefile;
endif
prune:
	-for i in $(sort $(dir_n) $(dir_)) ; do \
		found=0; \
		for j in $(sort $(dir_y) $(dir_e)) ; do \
			if [ $$i == $$j ]; then \
				found=1; \
			fi; \
		done; \
		if [ $$found == 0 ]; then \
			rm -r $$i; \
		fi; \
	done
