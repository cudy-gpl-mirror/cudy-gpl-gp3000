/*
 * Configuration for Realtek Semiconductor Corp. Mercury platform. Parts were derived from
 * other ARM configurations.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MERCURY_ASIC_H
#define __MERCURY_ASIC_H

#include <generated/autoconf.h>

#ifndef __ASSEMBLY__
#	include <asm/arch/registers.h>
#endif


#ifdef CONFIG_NET
/* XRAM BASE ADDRESS */
#	define NI_XRAM_BASE 0xF4500000
#	define GET_NI_XRAM_BASE() (NI_XRAM_BASE)
#endif /* CONFIG_NET */

/* The following CA_READ_READ and WRITE MACROS are used by some Cortina drivers
 * to read and write to SoC registers.  Since some SoC require address
 * translation, we define them here to allow the platform to specify the
 * required tranlation, if any for this platform.
 */
#ifdef CONFIG_ARM64
#define CA_REG_READ(addr) readl((u64)addr)
#define CA_REG_WRITE(value, addr) writel(value, (u64)addr)
#else
#define CA_REG_READ(addr) readl((u32)addr)
#define CA_REG_WRITE(value, addr) writel(value, (u32)addr)
#endif


#define SYS_NUM_IRQS 160

/* note: arch/arm/cpu/armv8/start.S which references GICD_BASE/GICC_BASE
 * does not yet support DT. Thus define it here.
 */
#ifdef CONFIG_CMD_SOC_TEST
#define CONFIG_GICV3
#endif
#define GICD_BASE                      0xF8000000
#define GICR_BASE                      0xF8040000

#define CONFIG_SYS_MEMTEST_SCRATCH	0x00100000
#define CONFIG_SYS_MEMTEST_START	0x05000000
#define CONFIG_SYS_MEMTEST_END		0x0D000000

/* Size of malloc() pool */

#define CONFIG_SYS_TIMER_BASE		0xf4321000

/* Use external clock source */
#if defined(CONFIG_RTK_V7_FPGA_PLATFORM) || defined(CONFIG_RTK_V8_FPGA_PLATFORM) && !defined(CONFIG_RTK_ZEBU_SIM)
#ifdef CONFIG_RTK_MERCURY_SINGLE_FPGA_PLATFORM
#define COUNTER_FREQUENCY		25000000
#define MERCURY_APB_CLK (25000000) /* Use external clock source */
#else
#define COUNTER_FREQUENCY		250000
#define MERCURY_APB_CLK (1250000)
#endif
#else //ASIC
#define COUNTER_FREQUENCY (25000000) /* 25MHz */
#define MERCURY_APB_CLK (125000000)
#endif

#define CORTINA_PER_IO_FREQ		MERCURY_APB_CLK

/* Cortina Serial Configuration */
#define CORTINA_UART_CLOCK		(MERCURY_APB_CLK)
#define CORTINA_SERIAL_PORTS		{(void *)CONFIG_SYS_SERIAL0, \
					 (void *)CONFIG_SYS_SERIAL1}

#define CONFIG_SYS_SERIAL0		PER_UART0_CFG
#define CONFIG_SYS_SERIAL1		PER_UART1_CFG

#define CONFIG_SYS_BAUDRATE_TABLE   { 19200, 38400, 57600, 115200, 921600 }


#ifdef CONFIG_LED_STATUS
#	ifdef CONFIG_RTK_LED_STYLE_GPIO
/* When use GPIO to control LED, must disabe CONFIG_CORTINA_LED */
#		undef CONFIG_CORTINA_LED
#	else
#		define CONFIG_CORTINA_LED
#	endif
#endif

#ifdef CONFIG_CORTINA_LED
#	define CORTINA_LED_NUM 16

#	define TRIGGER_HW_RX 0
#	define TRIGGER_HW_TX 1
#	define TRIGGER_SW 2
#	define TRIGGER_NONE 3

#	define BLINK_RATE1 0
#	define BLINK_RATE2 1

/* only used for CONFIG_CMD_LED */
#	define CONFIG_STATUS_LED
#	define CONFIG_BOARD_SPECIFIC_LED
#	define STATUS_LED_BIT 0x0
#	define STATUS_LED_STATE STATUS_LED_OFF
#	define STATUS_LED_PERIOD (CONFIG_SYS_HZ / 4)

#	define CONFIG_CMD_LED
#endif

/* BOOTP options */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_PXE
#define CONFIG_BOOTP_PXE_CLIENTARCH 0x100

/* SDRAM Bank #1 */
#define DDR_BASE			0x00000000
#define PHYS_SDRAM_1			DDR_BASE
#define PHYS_SDRAM_1_SIZE		0x80000000 /* 2GB */

#define PHYS_NS_DRAM_0			DDR_BASE
#define PHYS_NS_DRAM_1			0x10000000 //256 MB

#define PHYS_NS_DRAM_SIZE0		0x10000000 - 0x40000 //256MB - 256KB
#define PHYS_NS_DRAM_SIZE1		0x70000000 //1792 MB


/* Initial environment variables */

#define CONFIG_EXTRA_ENV_SETTINGS \
	"ethaddr=" __stringify(CONFIG_ETHADDR) "\0" \
	"kernel_addr_r=00080000\0" \
	"initrd_addr_r=0x02000000\0" \
	"fdt_addr_r=0x01800000\0" \
	"fdt_high=0x00000000\0" \
	"active_port=1\0" \
	"auto_scan_active_port=0\0" \
	"recv_multicast_udp_in_booting=0"

#define CONFIG_BOOTARGS \
	"earlycon=serial,0xf4329188 console=ttyS0,115200 " \
	"initrd=0x02000000,0x800000 swiotlb=16384 root=/dev/ram0 rw"
/*
#define CONFIG_BOOTCOMMAND		"bootm $kernel_addr_r " \
          "$initrd_addr_r:$initrd_size $fdt_addr_r"
*/
#define CONFIG_BOOTCOMMAND "booti $kernel_addr_r - $fdt_addr_r"
#define CONFIG_IPADDR 192.168.1.1
#define CONFIG_NETMASK 255.255.255.0
#define CONFIG_SERVERIP 192.168.1.7
#define CONFIG_GATEWAYIP 192.168.1.2



/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
/*
 * Note that CONFIG_SYS_MAX_FLASH_SECT allows for a parameter block
 * i.e. the bottom "sector" (bottom boot), or top "sector"
 *      (top boot), is a seperate erase region divided into
 *      4 (equal) smaller sectors. This, notionally, allows
 *      quicker erase/rewrire of the most frequently changed
 *      area......
 *      CONFIG_SYS_MAX_FLASH_SECT is padded up to a multiple of 4
 */
#define CONFIG_SYS_FLASH_BASE 0xe0000000
#define PHYS_FLASH_SIZE 0x08000000 /* 128MB */

#define CONFIG_SYS_MAX_FLASH_BANKS 1 /* max number of memory banks */
#define CONFIG_SYS_MAX_FLASH_SECT (512 * 2)

#ifdef CONFIG_NAND_HIDE_BAD
#define NAND_BBL_OFFSET         (0x340000)  /* 3.25MB */
#define NAND_BBL_OFFSET_REDUND  (0x380000)  /* 3.25MB +  256K */
#define NAND_BBL_SIZE           (0x40000)   /* For 4K Page, the block size is 256K*/

#ifdef CONFIG_BL_IMAGE_INC_OPTEED
#undef NAND_BBL_OFFSET
#define NAND_BBL_OFFSET         (0x480000)  /* 0x480000 */
#undef NAND_BBL_OFFSET_REDUND
#define NAND_BBL_OFFSET_REDUND  (0x4C0000)  /* 0x480000 +  256K */
#endif
#endif /* #ifdef CONFIG_NAND_HIDE_BAD */

/* don't change position, must afer default setting */
#include <configs/rtk_board_common.h>

#ifdef CONFIG_RTK_ENG_CMD
#undef CONFIG_SYS_PROMPT
#define CONFIG_SYS_PROMPT     "\n\n###### Mercury_uboot_for_inhouse_only### "
#endif

#ifdef CONFIG_CORTINA_SERIAL_NAND
    #define CONFIG_SYS_NAND_SELF_INIT 1
    #define CONFIG_SYS_NAND_BASE 0xE0000000
    #define CONFIG_SYS_MAX_NAND_CHIPS  1
    #define CONFIG_SYS_MAX_NAND_DEVICE 1

    #ifdef CONFIG_LUNA_MULTI_BOOT
        #ifdef CONFIG_ENV_IS_IN_NAND
            #ifndef CONFIG_SYS_REDUNDAND_ENVIRONMENT
                #define CONFIG_SYS_REDUNDAND_ENVIRONMENT
    	    #endif
		/* uboot config should define them */
            #ifndef CONFIG_ENV_OFFSET
                #define CONFIG_ENV_OFFSET         (0x3C0000)
                #define CONFIG_ENV_OFFSET_REDUND  (0x400000)
                #define CONFIG_ENV_RANGE          (0x40000)
                #define CONFIG_ENV_SIZE           (0x40000)
                #define CONFIG_ENV_SECT_SIZE      (CONFIG_ENV_SIZE)
            #endif
            #undef CONFIG_EXTRA_ENV_SETTINGS
            #define CONFIG_EXTRA_ENV_SETTINGS \
                        "pfl_env1_r=0x3c0000\0" \
                        "pfl_env2_r=0x400000\0" \
                        "pfl_env_size=0x40000\0" \
                        "unzip_src=0x5000000\0" \
                        "ftfp_base=0x5000000\0" \
                        "upenv=tftpboot ${ftfp_base} uboot-env.bin && nand erase " \
                        "${pfl_env1_r} ${pfl_env_size} && nand write ${ftfp_base} " \
                        "${pfl_env1_r} ${filesize} && nand erase ${pfl_env2_r} " \
                        "${pfl_env_size} && nand write ${ftfp_base} ${pfl_env2_r} " \
                        "${filesize}\0" \
                        "active_port=0\0" \
                        "auto_scan_active_port=0\0"
        #endif /* #ifdef CONFIG_ENV_IS_IN_NAND */
    #endif /* #ifdef CONFIG_LUNA_MULTI_BOOT */
#endif /* #ifdef CONFIG_CORTINA_SPI_NAND */

#if defined(CONFIG_MTD_CORTINA_SERIAL_NOR)
#	undef CONFIG_BOOTCOMMAND
#	define CONFIG_BOOTCOMMAND \
		"sf read $kernel_addr_r 0x200000 0x900000;sf read $fdt_addr_r 0x100000 " \
		"0x10000;booti $kernel_addr_r - $fdt_addr_r"

    #define CONFIG_SF_DEFAULT_CS    (0)
    #define CONFIG_SF_DEFAULT_BUS   (0)
    #define CONFIG_SF_DEFAULT_MODE  (0)
    #define CONFIG_SF_DEFAULT_SPEED (1000000)

#undef CONFIG_EXTRA_ENV_SETTINGS
#define CONFIG_EXTRA_ENV_SETTINGS \
            "pfl_env_r=0x180000\0" \
            "pfl_env_size=0x40000\0" \
            "unzip_src=0x5000000\0" \
            "ftfp_base=0x5000000\0" \
            "upenv=tftpboot ${ftfp_base} uboot-env.bin && nand erase " \
            "${pfl_env_r} ${pfl_env_size} && nand write ${ftfp_base} " \
            "${pfl_env_r} ${filesize}\0" \
            "active_port=0\0" \
            "auto_scan_active_port=0\0"
#endif /* CONFIG_MTD_CORTINA_SERIAL_NOR */


#endif /* __MERCURY_ASIC_H */
