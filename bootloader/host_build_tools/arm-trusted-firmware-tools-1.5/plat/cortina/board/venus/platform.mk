#
# Copyright (c) 2015-2018, Cortina Access., All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# Use generic OID definition (tbbr_oid.h)
USE_TBBR_DEFS                   :=      1

CORTINA_VENUS_PLATFORM	:= 1
# Process as define
ifneq (${CORTINA_VENUS_PLATFORM},0)
$(eval $(call add_define,CORTINA_VENUS_PLATFORM))
CORTINA_SOC_FAMILY	:= ca8277
# define it in environment
$(eval $(call add_define,CORTINA_SOC_FAMILY))
CORTINA_SOC_CPU_ARCH	:= aarch64
# define it in environment
$(eval $(call add_define,CORTINA_SOC_CPU_ARCH))
INC_CORTINA_SOC_REGS_HEADER	:= \<${CORTINA_SOC_FAMILY}_registers.h\>
# define it in environment
$(eval $(call add_define,INC_CORTINA_SOC_REGS_HEADER))
INC_PLAT_DEF_HEADER	:= \<${PLAT}_def.h\>
# define it in environment
$(eval $(call add_define,INC_PLAT_DEF_HEADER))
INC_PLAT_PRIVATE_HEADER	:= \<${PLAT}_private.h\>
# define it in environment
$(eval $(call add_define,INC_PLAT_PRIVATE_HEADER))
endif # CORTINA_VENUS_PLATFORM

CORTINA_VENUS_TESTCODE	?= 0
# Process as define
ifneq (${CORTINA_VENUS_TESTCODE},0)
$(eval $(call add_define,CORTINA_VENUS_TESTCODE))
endif # CORTINA_VENUS_TESTCODE

TEST_ON_VENUS_ASIC_SIM	?= 0
# Process as define
ifneq (${TEST_ON_VENUS_ASIC_SIM},0)
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM))
    BL2SMALL_SOURCES += plat/cortina/board/${PLAT}/${CORTINA_SOC_CPU_ARCH}/bl2small_entrypoint.S
$(eval $(call add_define,BL2SMALL_SOURCES))
endif # TEST_ON_VENUS_ASIC_SIM

CORTINA_PER_UART	:= 1
# Process as define
ifneq (${CORTINA_PER_UART},0)
$(eval $(call add_define,CORTINA_PER_UART))
CORTINA_PER_UART_CHECK_GLOBAL_STRAP	:= 1
ifneq (${CORTINA_PER_UART_CHECK_GLOBAL_STRAP},0)
# define it in environment
$(eval $(call add_define,CORTINA_PER_UART_CHECK_GLOBAL_STRAP))
endif #CORTINA_PER_UART_CHECK_GLOBAL_STRAP
endif # CORTINA_PER_UART

CORTINA_VENUS_TIMER	:= 1
# Process as define
ifneq (${CORTINA_VENUS_TIMER},0)
$(eval $(call add_define,CORTINA_VENUS_TIMER))
endif # CORTINA_VENUS_TIMER


# Compile flag for VENUS ASIC, default build for FPGA
CORTINA_VENUS_ASIC	?= 0
# Process as define
ifneq (${CORTINA_VENUS_ASIC},0)

#ifeq (${CORTINA_BOOT_EMMC},0)
#  $(error "CORTINA_BOOT_EMMC must not be turned off for VENUS ASIC")
#endif # CORTINA_BOOT_EMMC

ifeq (${CORTINA_VENUS_ASIC_OFFICIAL},0)
ifeq (${TEST_ON_VENUS_ASIC_SIM},0)
  $(error "Use CORTINA_VENUS_ASIC_OFFICIAL or TEST_ON_VENUS_ASIC_SIM to enable CORTINA_VENUS_ASIC")
endif #TEST_ON_VENUS_ASIC_SIM
endif #CORTINA_VENUS_ASIC_OFFICIAL
endif #CORTINA_VENUS_ASIC

# Compile flag to build BL1 to test on ASIC simulator
TEST_ON_VENUS_ASIC_SIM_HOTFIX ?= 0
TEST_ON_VENUS_ASIC_SIM_OTP_SHADOW_COPY ?= 0
TEST_ON_VENUS_ASIC_SIM_OTP_WRITE ?= 0
TEST_ON_VENUS_ASIC_SIM_MULTIBOOT ?= 0
TEST_ON_VENUS_ASIC_SIM ?= 0
ifeq (${TEST_ON_VENUS_ASIC_SIM},1)
ifeq (${CORTINA_BOOT_EMMC},0)
  $(error "CORTINA_BOOT_EMMC must not be turned off for VENUS ASIC")
endif
$(eval $(call add_define,CORTINA_VENUS_ASIC))
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM))
endif

ifeq (${TEST_ON_VENUS_ASIC_SIM_HOTFIX},1)
TEST_ON_VENUS_ASIC_SIM = 1
$(eval $(call add_define,CORTINA_VENUS_ASIC))
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM))
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM_HOTFIX))
endif

ifeq (${TEST_ON_VENUS_ASIC_SIM_OTP_SHADOW_COPY},1)
TEST_ON_VENUS_ASIC_SIM = 1
$(eval $(call add_define,CORTINA_VENUS_ASIC))
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM))
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM_OTP_SHADOW_COPY))
endif

ifeq (${TEST_ON_VENUS_ASIC_SIM_OTP_WRITE},1)
TEST_ON_VENUS_ASIC_SIM = 1
$(eval $(call add_define,CORTINA_VENUS_ASIC))
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM))
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM_OTP_WRITE))
endif

ifeq (${TEST_ON_VENUS_ASIC_SIM_MULTIBOOT},1)
TEST_ON_VENUS_ASIC_SIM = 1
$(eval $(call add_define,CORTINA_VENUS_ASIC))
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM))
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM_MULTIBOOT))
endif

# these flags take effect only if TEST_ON_VENUS_ASIC_SIM is set
# when all of the following are disabled, is it equivalent to build CORTINA_VENUS_ASIC_OFFICIAL
TEST_ON_VENUS_ASIC_SIM_DISABLE_MMIO_TRACING ?= 0
ifeq (${TEST_ON_VENUS_ASIC_SIM_DISABLE_MMIO_TRACING},1)
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM_DISABLE_MMIO_TRACING))
endif
TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_MARK ?= 0
ifeq (${TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_MARK},1)
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_MARK))
endif
TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_FLAG ?= 0
ifeq (${TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_FLAG},1)
$(eval $(call add_define,TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_FLAG))
endif


CORTINA_VENUS_ASIC_OFFICIAL ?= 0
ifeq (${CORTINA_VENUS_ASIC_OFFICIAL},1)
$(eval $(call add_define,CORTINA_VENUS_ASIC))
ifeq (${TEST_ON_VENUS_ASIC_SIM},1)
  $(error "TEST_ON_VENUS_ASIC_SIM should not be used when CORTINA_VENUS_ASIC_OFFICIAL is ued")
endif
endif

# check if we are missing to define either CORTINA_VENUS_ASIC_OFFICIAL or TEST_ON_VENUS_ASIC_SIM
ifneq (${CORTINA_VENUS_ASIC},0)
ifeq (${CORTINA_VENUS_ASIC_OFFICIAL},0)
ifeq (${TEST_ON_VENUS_ASIC_SIM},0)
  $(error "Use CORTINA_VENUS_ASIC_OFFICIAL or TEST_ON_VENUS_ASIC_SIM to enable CORTINA_VENUS_ASIC")
endif
endif
endif #CORTINA_VENUS_ASIC


# Latest CORTINA ASIC supports OTP_MODE_KP
CORTINA_OTP_SUPPORT	:= OTP_MODE_KP
# Process as define
ifneq (${CORTINA_OTP_SUPPORT},0)
$(eval $(call add_define,CORTINA_OTP_SUPPORT))
INC_CORTINA_PLATFORM_OTP_DATA_HEADER := \<${CORTINA_SOC_FAMILY}_otp_data.h\>
# define it in environment
$(eval $(call add_define,INC_CORTINA_PLATFORM_OTP_DATA_HEADER))
endif #CORTINA_OTP_SUPPORT

CORTINA_SECURE_JTAG_SUPPORT	:= 1
# Process as define
ifneq (${CORTINA_SECURE_JTAG_SUPPORT},0)
$(eval $(call add_define,CORTINA_SECURE_JTAG_SUPPORT))
endif # CORTINA_SECURE_JTAG_SUPPORT

CORTINA_BOOT_MEMMAP    ?= 0
# Process as define
ifneq (${CORTINA_BOOT_MEMMAP},0)
$(eval $(call add_define,CORTINA_BOOT_MEMMAP))
endif #CORTINA_BOOT_MEMMAP

CORTINA_BOOT_SERIAL_NOR    := 1
# Process as define
ifneq (${CORTINA_BOOT_SERIAL_NOR},0)
CORTINA_BOOT_SERIAL_NOR_LEGACY_API	:= 1
ifneq (${CORTINA_BOOT_SERIAL_NOR_LEGACY_API},0)
$(eval $(call add_define,CORTINA_BOOT_SERIAL_NOR_LEGACY_API))
endif #CORTINA_BOOT_SERIAL_NOR_LEGACY_API
$(eval $(call add_define,CORTINA_BOOT_SERIAL_NOR))
endif #CORTINA_BOOT_SERIAL_NOR

CORTINA_BOOT_SERIAL_NAND    := 1
# Process as define
ifneq (${CORTINA_BOOT_SERIAL_NAND},0)
$(eval $(call add_define,CORTINA_BOOT_SERIAL_NAND))
endif #CORTINA_BOOT_SERIAL_NAND

CORTINA_BOOT_PARALLEL_NAND    := 1
# Process as define
ifneq (${CORTINA_BOOT_PARALLEL_NAND},0)
CORTINA_BOOT_PARALLEL_NAND_LEGACY_API	:= 1
ifneq (${CORTINA_BOOT_PARALLEL_NAND_LEGACY_API},0)
$(eval $(call add_define,CORTINA_BOOT_PARALLEL_NAND_LEGACY_API))
endif #CORTINA_BOOT_PARALLEL_NAND_LEGACY_API
$(eval $(call add_define,CORTINA_BOOT_PARALLEL_NAND))
endif # CORTINA_BOOT_SERIAL_NAND

CORTINA_BOOT_EMMC    ?= 1
ifneq (${CORTINA_BOOT_EMMC},0)
$(eval $(call add_define,CORTINA_BOOT_EMMC))
endif

#Enable BL1 HF Checkpoint support
CORTINA_HOTFIX_CHECKPOINT_SUPPORT   := 1
# Process as define
ifneq (${CORTINA_HOTFIX_CHECKPOINT_SUPPORT},0)
$(eval $(call add_define,CORTINA_HOTFIX_CHECKPOINT_SUPPORT))
endif # CORTINA_HOTFIX_CHECKPOINT_SUPPORT

TEST_OTP  ?= 0
ifeq (${TEST_OTP},1)
OTP_SIM = 1
LOG_LEVEL = 40
$(eval $(call add_define,OTP_SIM))
$(eval $(call add_define,TEST_OTP))
endif

CORTINA_PER_WATCHDOG    := 1
# Process as define
ifneq (${CORTINA_PER_WATCHDOG},0)
$(eval $(call add_define,CORTINA_PER_WATCHDOG))
endif # CORTINA_PER_WATCHDOG

CORTINA_SECURE_SRAM    ?= 1
# Process as define
ifneq (${CORTINA_SECURE_SRAM},0)
$(eval $(call add_define,CORTINA_SECURE_SRAM))
endif # CORTINA_SECURE_SRAM

LIB_GF_SCRAMBLE	:= 1
ifneq (${LIB_GF_SCRAMBLE},0)
# define it in environment
$(eval $(call add_define,LIB_GF_SCRAMBLE))
endif #LIB_GF_SCRAMBLE

CONSOLE_BASE            :=      CORTINA_PER_UART0_BASE
# Process as define
$(eval $(call add_define,CONSOLE_BASE))

CRASH_CONSOLE_BASE      :=      CORTINA_PER_UART0_BASE
# Process as define
$(eval $(call add_define,CRASH_CONSOLE_BASE))

# On VENUS-FPGA, the TSP can execute either from Trusted SRAM or Trusted DRAM.
# Trusted DRAM is the default.
#
PLAT_VENUS_TSP_LOCATION       :=      tdram


ifeq (${PLAT_VENUS_TSP_LOCATION}, tsram)
  PLAT_VENUS_TSP_RAM_LOCATION_ID := PLAT_VENUS_TRUSTED_SRAM_ID
else ifeq (${PLAT_VENUS_TSP_LOCATION}, tdram)
  PLAT_VENUS_TSP_RAM_LOCATION_ID := PLAT_VENUS_TRUSTED_DRAM_ID
else
  $(error "Unsupported PLAT_VENUS_TSP_RAM_LOCATION value")
endif
# Process as define
$(eval $(call add_define,PLAT_VENUS_TSP_RAM_LOCATION_ID))



# On VENUS-FPGA, BL3-1 EL3 Trusted Runtime can execute either from Trusted SRAM or Trusted DRAM.
# Trusted SRAM is the default.
#
PLAT_VENUS_BL3_1_EL3_LOCATION       :=      tdram

ifeq (${PLAT_VENUS_BL3_1_EL3_LOCATION}, tsram)
  PLAT_VENUS_BL3_1_EL3_RAM_LOCATION_ID := PLAT_VENUS_TRUSTED_SRAM_ID
else ifeq (${PLAT_VENUS_BL3_1_EL3_LOCATION}, tdram)
  PLAT_VENUS_BL3_1_EL3_RAM_LOCATION_ID := PLAT_VENUS_TRUSTED_DRAM_ID
else
  $(error "Unsupported PLAT_VENUS_BL3_1_EL3_RAM_LOCATION_ID value")
endif
# Process as define
$(eval $(call add_define,PLAT_VENUS_BL3_1_EL3_RAM_LOCATION_ID))




PLAT_INCLUDES		:=	-Iinclude \
				-Iinclude/drivers/cortina/common \
				-Iinclude/drivers/cortina/${CORTINA_SOC_FAMILY} \
				-Iplat/cortina/board/${PLAT} \
				-Iplat/cortina/board/${PLAT}/include \
				


PLAT_BL_COMMON_SOURCES  :=      drivers/io/io_block.c                   \
                                drivers/io/io_fip.c                     \
                                drivers/io/io_memmap.c                  \
                                drivers/io/io_storage.c                 \
                                lib/${CORTINA_SOC_CPU_ARCH}/xlat_tables.c               \
                                plat/common/${CORTINA_SOC_CPU_ARCH}/plat_common.c       \
                                plat/common/plat_gic.c                  \
                                plat/cortina/board/${PLAT}/${CORTINA_SOC_CPU_ARCH}/${PLAT}_common.c       \
                                plat/cortina/board/${PLAT}/${CORTINA_SOC_CPU_ARCH}/plat_helpers.S       

BL1_SOURCES             +=      drivers/arm/cci/cci.c             \
                                lib/cpus/aarch64/cortex_a55.S           \
                                plat/common/${CORTINA_SOC_CPU_ARCH}/platform_up_stack.S \
                                plat/cortina/board/${PLAT}/${CORTINA_SOC_CPU_ARCH}/bl1_plat_helpers.S   \
                                plat/cortina/board/${PLAT}/bl1_plat_setup.c            \
                                plat/cortina/board/${PLAT}/plat_io_storage.c \
                                plat/cortina/board/${PLAT}/bl1_cli.c				\
                                plat/cortina/board/${PLAT}/partitions.c

BL2_SOURCES             +=      plat/common/${CORTINA_SOC_CPU_ARCH}/platform_up_stack.S \
                                plat/cortina/board/${PLAT}/bl2_plat_setup.c             \
                                plat/cortina/board/${PLAT}/plat_io_storage.c \
                                plat/cortina/board/${PLAT}/plat_security.c            \
                                plat/cortina/board/${PLAT}/partitions.c

BL31_SOURCES            +=      drivers/arm/cci/cci.c             \
                                drivers/arm/gic/gic_v3.c                \
                                drivers/arm/gic/v3/gicv3_main.c     \
				                drivers/arm/gic/v3/gicv3_helpers.c  \
				                plat/common/plat_gicv3.c        \
				                plat/cortina/board/${PLAT}/venus_gicv3.c		\
				                drivers/arm/gic/common/gic_common.c		\
				                drivers/arm/gic/v3/gic600.c		\
                                lib/cpus/aarch64/cortex_a55.S           \
                                plat/cortina/board/${PLAT}/bl31_plat_setup.c            \
                                plat/cortina/board/${PLAT}/plat_pm.c                    \
                                plat/cortina/board/${PLAT}/plat_topology.c				\
								plat/cortina/board/${PLAT}/ca_sip_main.c   	        \
								plat/cortina/board/${PLAT}/ca_svc_wdt.c


#Trusted Board Boot Support
ifneq (${TRUSTED_BOARD_BOOT},0)

    # By default, ARM platforms use RSA keys
    KEY_ALG             :=      rsa

    # Include common TBB sources
    AUTH_SOURCES        :=      drivers/auth/auth_mod.c                         \
                                drivers/auth/crypto_mod.c                       \
                                drivers/auth/img_parser_mod.c                   \
                                drivers/auth/tbbr/tbbr_cot.c                    \

    BL1_SOURCES         +=      ${AUTH_SOURCES}
    BL2_SOURCES         +=      ${AUTH_SOURCES}

    TF_MBEDTLS_KEY_ALG     :=      ${KEY_ALG}
    PLAT_INCLUDES	+=	-I./include/common/tbbr

    # We expect to locate the *.mk files under the directories specified below
    CRYPTO_LIB_MK := drivers/auth/mbedtls/mbedtls_crypto.mk
    IMG_PARSER_LIB_MK := drivers/auth/mbedtls/mbedtls_x509.mk

    $(info Including ${CRYPTO_LIB_MK})
    include ${CRYPTO_LIB_MK}

    $(info Including ${IMG_PARSER_LIB_MK})
    include ${IMG_PARSER_LIB_MK}

endif

ifneq (${TRUSTED_BOARD_BOOT},0)
    # ROTPK hash location
    ifeq (${ARM_ROTPK_LOCATION}, regs)
        ARM_ROTPK_LOCATION_ID = ARM_ROTPK_REGS_ID
    else ifeq (${ARM_ROTPK_LOCATION}, devel_rsa)
        ARM_ROTPK_LOCATION_ID = ARM_ROTPK_DEVEL_RSA_ID
    else
        $(error "Unsupported ARM_ROTPK_LOCATION value")
    endif
    $(eval $(call add_define,ARM_ROTPK_LOCATION_ID))

    BL1_SOURCES         +=     plat/cortina/board/${PLAT}/board_arm_trusted_boot.c 
    BL2_SOURCES         +=     plat/cortina/board/${PLAT}/board_arm_trusted_boot.c 
endif


# Cortina OTP boot support
ifneq (${CORTINA_OTP_SUPPORT},0)
    PLAT_BL_COMMON_SOURCES	+= drivers/cortina/common/otp/cortina_otp.c
    PLAT_INCLUDES	+=	-Iinclude/drivers/cortina/${CORTINA_SOC_FAMILY}/otp
    PLAT_INCLUDES	+=	-Iinclude/drivers/cortina/common/otp

CORTINA_MULTIBOOT_SUPPORT    := 1
# Process as define
ifneq (${CORTINA_MULTIBOOT_SUPPORT},0)
$(eval $(call add_define,CORTINA_MULTIBOOT_SUPPORT))
CORTINA_MULTIBOOT_ONE_SHOT_SUPPORT    := 1 
# Process as define
ifneq (${CORTINA_MULTIBOOT_ONE_SHOT_SUPPORT},0)
$(eval $(call add_define,CORTINA_MULTIBOOT_ONE_SHOT_SUPPORT))
endif # CORTINA_MULTIBOOT_ONE_SHOT_SUPPORT
endif # CORTINA_MULTIBOOT_SUPPORT

endif #CORTINA_OTP_SUPPORT

# Cortina PER Watchdog support
ifneq (${CORTINA_PER_WATCHDOG},0)
    PLAT_BL_COMMON_SOURCES	+= drivers/cortina/common/per/watchdog/cortina_per_watchdog.c
    PLAT_INCLUDES	+=	-Iinclude/drivers/cortina/common/per/watchdog
endif # PER Watchdog

# Synopsys Designware MMC/SD Support
ifneq (${CORTINA_BOOT_EMMC},0)
    BL1_SOURCES         +=     drivers/emmc/emmc.c                     \
                                drivers/synopsys/emmc/dw_mmc.c          
    BL2_SOURCES         +=     drivers/emmc/emmc.c                     \
                                drivers/synopsys/emmc/dw_mmc.c          
    PLAT_INCLUDES	+= 	-Iinclude/drivers/synopsys
endif # Synopsys Designware MMC/SD Support

# Cortina SECURE_SRAM support
ifneq (${CORTINA_SECURE_SRAM},0)
    PLAT_BL_COMMON_SOURCES	+= drivers/cortina/common/secure_sram/cortina_secure_sram.c
    PLAT_INCLUDES	+=	-Iinclude/drivers/cortina/common/secure_sram
endif # SECURE_SRAM

# Cortina Secure JTAG support in BL1
ifneq (${CORTINA_SECURE_JTAG_SUPPORT},0)
    BL1_SOURCES         +=      drivers/cortina/common/secure_jtag/cortina_secure_jtag.c
    PLAT_INCLUDES	+=	-Iinclude/drivers/cortina/common/secure_jtag
endif # Cortina Secure JTAG support in BL1

# Cortina Serial NOR Flash
ifneq (${CORTINA_BOOT_SERIAL_NOR},0)
    BL1_SOURCES         +=      drivers/cortina/common/serial_nor/cortina_serial_nor.c
    BL2_SOURCES         +=      drivers/cortina/common/serial_nor/cortina_serial_nor.c
    PLAT_INCLUDES	+=	-Iinclude/drivers/cortina/common/serial_nor
endif # CORTINA_BOOT_SERIAL_NOR

# Cortina Serial NAND Flash
ifneq (${CORTINA_BOOT_SERIAL_NAND},0)
    BL1_SOURCES         +=      drivers/cortina/common/serial_nand/cortina_serial_nand.c
    BL2_SOURCES         +=      drivers/cortina/common/serial_nand/cortina_serial_nand.c
    PLAT_INCLUDES       +=      -Iinclude/drivers/cortina/common/serial_nand
endif # CORTINA_BOOT_SERIAL_NAND

# Cortina RAW PARALLEL NAND Flash
ifneq (${CORTINA_BOOT_PARALLEL_NAND},0)
    BL1_SOURCES         +=      drivers/cortina/common/parallel_nand/cortina_parallel_nand.c
    BL2_SOURCES         +=      drivers/cortina/common/parallel_nand/cortina_parallel_nand.c
    PLAT_INCLUDES	+=	-Iinclude/drivers/cortina/common/parallel_nand
endif # CORTINA_BOOT_PARALLEL_NAND

# OTP Data Scramble Support
ifneq (${LIB_GF_SCRAMBLE},0)
    BL1_SOURCES         +=     drivers/oss/gf_scramble/gf_lfsr_32_26_23_22_16_13_11_10_8_7_5_4_2.c 
    BL1_SOURCES         +=     drivers/oss/gf_scramble/lib_gf_scramble.c
    PLAT_INCLUDES	+=	-Iinclude/drivers/oss/gf_scramble
endif # OTP Data Scramble Support

# CORTINA_PER_UART
ifneq (${CORTINA_PER_UART},0)
    PLAT_BL_COMMON_SOURCES	+= drivers/cortina/common/per/uart/asm/${CORTINA_SOC_CPU_ARCH}/cortina-per-uart-console.S
    PLAT_INCLUDES	+=	-Iinclude/drivers/cortina/common/per/uart
endif # CORTINA_PER_UART

# Check Global strap to determine if UART should output messages
ifneq (${CORTINA_PER_UART_CHECK_GLOBAL_STRAP},0)
    PLAT_BL_COMMON_SOURCES     += plat/cortina/board/${PLAT}/plat_cortina_uart_message.S
endif #CORTINA_PER_UART_CHECK_GLOBAL_STRAP

# CORTINA_VENUS_TIMER
ifneq (${CORTINA_VENUS_TIMER},0)
    PLAT_BL_COMMON_SOURCES	+= drivers/delay_timer/delay_timer.c
    PLAT_BL_COMMON_SOURCES	+= drivers/delay_timer/generic_delay_timer.c
endif # CORTINA_VENUS_TIMER


# Process Cortina specific JTAG Debug flag
$(eval $(call add_define,JTAG_DEBUG))
ifneq (${JTAG_DEBUG},)
# compile in symbols for JTAG debug
       CFLAGS         +=      -g
       ASFLAGS        +=      -g -Wa,--gdwarf-2
        # Use LOG_LEVEL_INFO by default for debug builds
       LOG_LEVEL      :=      30
endif #JTAG_DEBUG

# Process Debug flag, increase from default of 20 to 30
$(eval $(call add_define,DEBUG))
ifneq (${DEBUG}, 1)
        # Use LOG_LEVEL_NOTICE by default for release builds
        LOG_LEVEL	:=	30
endif #DEBUG

