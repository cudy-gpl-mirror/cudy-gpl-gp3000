ifneq ($(UBOOT),)
$(info "======== $(UBOOT) for host_build_tools/Makefile")
sinclude ../$(UBOOT)/.config
else
$(info "-------- u-boot-2015.04 for host_build_tools/Makefile")
sinclude ../u-boot-2015.04/.config
endif

CC := gcc
AS := as
LD := ld
OC := objcopy
OD := objdump
RE := readelf
NM := nm
WORK_D = $(shell pwd)
dir_y = 
atf_tool =
target_ic =
### Default , G3, CA77xx
ifeq ($(CONFIG_TARGET_PRESIDIO_ASIC),y)
dir_y = nandinfo
atf_tool = arm-trusted-firmware-tools-1.0
target_ic = G3HGU
endif

ifeq ($(CONFIG_TARGET_VENUS),y) #venus
atf_tool = arm-trusted-firmware-tools-1.5
dir_y = nandinfo_venus
target_ic = VENUS
endif

ifeq ($(CONFIG_TARGET_RTK_TAURUS),y) #TAURUS
atf_tool = arm-trusted-firmware-tools-1.5
dir_y = nandinfo_venus
target_ic = TAURUS
endif

ifeq ($(CONFIG_CA8277B),y) #venus
atf_tool = arm-trusted-firmware-tools-1.5
dir_y = nandinfo_venus
target_ic = VENUS
endif

dir_clean = nandinfo arm-trusted-firmware-tools-1.0 arm-trusted-firmware-tools-1.5 nandinfo_venus

$(info "  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
$(info "  x  Target : $(target_ic)   xxxxxx")
$(info "  x  Use    : $(atf_tool)    xxxxxx")
$(info "  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
dir_y += $(atf_tool)

.DEFAULT_GOAL = all

nandinfo_install:
	if [ -f ./nandinfo/gen_nand_info ]; then \
	cp -f ./nandinfo/gen_nand_info $(WORK_D) ; \
	fi

nandinfo_venus_install:
	if [ -f ./nandinfo_venus/gen_nand_info ]; then \
        cp -f ./nandinfo_venus/gen_nand_info $(WORK_D) ; \
        fi

atftools_install:
	make -C $(atf_tool) install DEST=$(WORK_D)

check_target:
	$(info "  x  Target : -$(target_ic)-   xxxxxx")
ifeq ($(target_ic),) 
	$(error unknow arm-trusted-firmware-tools!!)
endif

all:	check_target
	for i in $(dir_y) ; do \
                make -C $$i all || exit $?; \
        done
	make nandinfo_install nandinfo_venus_install atftools_install

clean:
	for i in $(dir_clean) ; do \
                make -C $$i clean || exit $?; \
        done
	rm -f fip_create
	rm -f cert_create
	rm -f gen_nand_info
	rm -f encrypt_fw
