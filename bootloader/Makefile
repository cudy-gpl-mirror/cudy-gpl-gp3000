unexport 
ifneq ($(BOOTLOADER_2_CONF),)
$(info BOOTLOADER_2_CONF=$(BOOTLOADER_2_CONF))
	include ./$(BOOTLOADER_2_CONF)
else
ifneq ($(wildcard ./bootloader.conf), ./bootloader.conf)
    $(warning EE: Missing './bootloader.conf'. Refer './bootloader.conf.sample' to create your own './bootloader.conf')
else
    include ./bootloader.conf
endif
endif

BL_DIR  = $(shell pwd)
PROJECT_D = $(BL_DIR)/project
PROJECT_NAME?=luna_g3
ifeq ($(CC),cc)
$(info cc: $(CC), USE_BOOTLOADER_CONF_CROSS_COMPILE=1)
USE_BOOTLOADER_CONF_CROSS_COMPILE=1
endif
ifeq ($(CC),)
$(info cc: $(CC), USE_BOOTLOADER_CONF_CROSS_COMPILE=1)
USE_BOOTLOADER_CONF_CROSS_COMPILE=1
endif
ifeq ($(USE_BOOTLOADER_CONF_CROSS_COMPILE),1)
CC := $(CROSS_COMPILE)gcc
AS := $(CROSS_COMPILE)as
LD := $(CROSS_COMPILE)ld
OC := $(CROSS_COMPILE)objcopy
OD := $(CROSS_COMPILE)objdump
RE := $(CROSS_COMPILE)readelf
NM := $(CROSS_COMPILE)nm
endif

ifeq ($(CONFIG_64BIT_BOOTLOADER),y)
$(info --- CONFIG_64BIT_BOOTLOADER=y, generate AARCH64 Bootloader! ---)
CROSS_COMPILE=$(RSDK)/bin/aarch64-linux-
CC := $(CROSS_COMPILE)gcc
AS := $(CROSS_COMPILE)as
LD := $(CROSS_COMPILE)ld
OC := $(CROSS_COMPILE)objcopy
OD := $(CROSS_COMPILE)objdump
RE := $(CROSS_COMPILE)readelf
NM := $(CROSS_COMPILE)nm
$(info --- $(CROSS_COMPILE))
endif

CFLAGS =
LDFLAGS =

UBOOT_MAYBE=u-boot-2015.04 u-boot-2018.11 u-boot-2020.01
UBOOT=u-boot-2015.04
ifeq ($(CONFIG_UBOOT_VER),u-boot-2018.11)
UBOOT=u-boot-2018.11
export UBOOT
endif
ifeq ($(CONFIG_UBOOT_VER),u-boot-2020.01)
UBOOT=u-boot-2020.01
export UBOOT
endif
ifeq ($(CONFIG_UBOOT_VER),u-boot-2022.10)
UBOOT=u-boot-2022.10
export UBOOT
endif
$(info "======== $(UBOOT) for bootloader")

UB_ENV_TOOL=./$(UBOOT)/tools/mkenvimage
UB_ENV_D=./u-boot-env

MACHINE =
ARCH =

UBOOT_ENV_FILE=$(subst .txt,,$(U-BOOT-ENV-FILE))

ifeq ($(UBOOT_ENV_FILE),)
$(warning "Miss u-boot-env file")
endif

ifeq ($(U-BOOT-CONF),)
$(warning "Not found u-boot conf!")
endif


ATF_IMAGE=./arm-trusted-firmware/$(ARM_TRUSTED_FIRMWARE)

export BL_DIR ATF_IMAGE

######## BL SIZE 
TARGET_BL_SIZE=
ifneq ($(SDK_BL_SIZE),)
ifneq ($(SDK_BL_SIZE),default)
TARGET_BL_SIZE=$(SDK_BL_SIZE)
endif
endif
################################
host-tool:
	make -C host_build_tools all

atf-key:
	-rm -rf $(BL_DIR)/image/fip/*
	make -C arm-trusted-firmware-key all

atf-fw:
	[ -d $(BL_DIR)/image/fip ] || mkdir -p $(BL_DIR)/image/fip;
	@if [ ! -f $(ATF_IMAGE)/build/*/release/bl2.bin ] ; then \
				echo "\033[31m Not found bl2.bin in $(ATF_IMAGE)!!\033[0m" ; \
				exit 1; \
        fi
	@if [ ! -f $(ATF_IMAGE)/build/*/release/bl31.bin ] ; then \
				echo "\033[31m Not found bl31.bin in $(ATF_IMAGE)!!\033[0m" ; \
				exit 1; \
        fi
	cp -f $(ATF_IMAGE)/build/*/release/bl2.bin ./image/fip
	cp -f $(ATF_IMAGE)/build/*/release/bl31.bin ./image/fip
######uboot env
uboot2018-extra-header:
	@dos2unix -F $(UB_ENV_D)/$(U-BOOT-ENV-FILE);
	@cd  $(UB_ENV_D) && rm -f UBOOT-2018-DEFAULT_ENV_FILE && \
	ln -s $(U-BOOT-ENV-FILE) UBOOT-2018-DEFAULT_ENV_FILE;

uboot-extra-header:
	@dos2unix -F $(UB_ENV_D)/$(U-BOOT-ENV-FILE);
	@rm -f $(UBOOT)/include/generated/rtk_uboot_extra_env_settings.h && \
	perl $(UB_ENV_D)/parser_uboot_env.perl $(UB_ENV_D)/$(U-BOOT-ENV-FILE)  $(UBOOT)/include/generated/rtk_uboot_extra_env_settings.h

uboot-env:
	@if [ -f $(UB_ENV_D)/$(U-BOOT-ENV-FILE) ]; then \
                echo "Build $(UBOOT_ENV_FILE) Env Binary!"; \
                dos2unix -F $(UB_ENV_D)/$(U-BOOT-ENV-FILE); \
                make -f Makefile.mkenv $(UBOOT_ENV_FILE); \
        else \
                echo "Cannot find $(UB_ENV_D)/$(UBOOT_ENV_FILE)!"; \
        fi

################
	
preconfig_uboot:
	ln -sfn $(UBOOT) u-boot
ifeq ($(UBOOT),u-boot-2015.04)
	make uboot-extra-header
endif
ifeq ($(UBOOT),u-boot-2018.11)
	make uboot2018-extra-header
endif
ifeq ($(UBOOT),u-boot-2020.01)
	make uboot2018-extra-header
endif
ifeq ($(UBOOT),u-boot-2022.10)
	make uboot2018-extra-header
endif
	rm -rf image
	if [ ! -e ${UBOOT}/.config ] ; \
	then \
	make -j 8 -C $(UBOOT) CROSS_COMPILE=$(CROSS_COMPILE) CC='$(CC)' CFLAGS='$(CFLAGS)'  $(U-BOOT-CONF) ; \
	fi	
ifeq ($(CONFIG_UBOOT_SIGNATURE_FIT),y)
	touch $(UBOOT)/Makefile
endif

menuconfig:
	make -C $(UBOOT) CROSS_COMPILE=$(CROSS_COMPILE) CC='$(CC)' menuconfig
##### BUILD ######
uboot: 
	make -j 8 -C $(UBOOT)  CROSS_COMPILE=$(CROSS_COMPILE) CC='$(CC)' CFLAGS='$(CFLAGS)' LDFLAGS='$(LDFLAGS)' TARGET_BL_SIZE='$(TARGET_BL_SIZE)' all
ifeq ($(CONFIG_UBOOT_SIGNATURE_FIT),y)
	@sh Verified_Boot/uboot_dtb_pubkey_write.sh UBOOT_FIP_PRI_KEY $(CONFIG_UBOOT_CIPHER)
	make -j 8 -C $(UBOOT)  CROSS_COMPILE=$(CROSS_COMPILE) CC='$(CC)' CFLAGS='$(CFLAGS)' LDFLAGS='$(LDFLAGS)' EXT_DTB=$(BL_DIR)/Verified_Boot/UBOOT_FIP_PRI_KEY/uboot_fip_pubkey.dtb TARGET_BL_SIZE='$(TARGET_BL_SIZE)' all
	@sh Verified_Boot/u-boot-bin_check.sh UBOOT_FIP_PRI_KEY
endif
	[ -d image/ ] || mkdir -p image
	[ -f $(UBOOT)/u-boot.bin ] && cp $(UBOOT)/u-boot.bin image/
	[ -d image/ ] || mkdir -p image
	[ -f $(UBOOT)/u-boot.bin ] && cp $(UBOOT)/u-boot.bin image/

clean:
	rm -rf image
	@if [ -d $(UBOOT) ]; then \
		make -C $(UBOOT) clean; \
	fi
	rm -f $(UBOOT)/.config
	make -C host_build_tools   clean
	for u in $(UBOOT_MAYBE) ; do \
		test -d $$u && make -C $$u clean; \
		rm -f $$u/.config; \
	done
	rm -f flash_support.csv

fip_gpt_image:
	cd $(UBOOT) && ./create_fip_gpt_image.sh

bismarck_build:
	@echo "Build Bismarck"
ifneq ("$(wildcard FLASH_TYPE_NOR)","")
	cd $(UBOOT) && ./build_bismarck_nor.sh
else
ifneq ("$(wildcard FLASH_TYPE_SPI_NAND)","")
	cd $(UBOOT) && ./build_bismarck_nand.sh
else
	@echo "No FLASH_TYPE file, so we didn't build boodloader"
endif
endif

bismarck_image:
	@echo "Copy Bootloader image"
	@[ -d image/ ] || mkdir -p image
	@if [ -f $(UBOOT)/release/plr.img ]; then \
		cp $(UBOOT)/release/plr.img image/; \
	fi;
	@if [ -f $(UBOOT)/release/encode_uboot.img ]; then \
		cp $(UBOOT)/release/encode_uboot.img image/; \
	fi;


all:
	@if [ -d $(UBOOT) ]; then \
		make preconfig_uboot uboot host-tool atf-key atf-fw all_phase2; \
	fi

all_phase2:
	make -f Makefile.img img
	@if [ -f $(UB_ENV_D)/$(U-BOOT-ENV-FILE) ]; then \
		echo "Build $(UBOOT_ENV_FILE) Env Binary!"; \
		dos2unix -F $(UB_ENV_D)/$(U-BOOT-ENV-FILE); \
		make -f Makefile.mkenv $(UBOOT_ENV_FILE); \
	else \
		echo "Cannot find $(UB_ENV_D)/$(UBOOT_ENV_FILE)!"; \
	fi
	@if [ -f $(BOOTLOADER_PATH)/$(UBOOT)/tools/mkimage ]; then \
		cp $(BOOTLOADER_PATH)/$(UBOOT)/tools/mkimage $(BOOTLOADER_PATH)/image/; \
	fi
