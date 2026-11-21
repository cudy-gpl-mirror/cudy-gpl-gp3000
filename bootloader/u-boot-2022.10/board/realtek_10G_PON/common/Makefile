obj-y += aarch32_init.o
obj-y += rtk_board_fdt_fixup.o
obj-y += rtk_board_comm.o
obj-$(CONFIG_OF_SYSTEM_SETUP) += fdt_fixup.o
#obj-$(CONFIG_CORTINA_UART) += serial_cortina.o
obj-$(CONFIG_CORTINA_SPINAND_BBT) += spi_nand/spi-nand-bbt.o
obj-$(CONFIG_CORTINA_SERIAL_NAND) += spi_nand/cortina_spi_nand.o
obj-$(CONFIG_CORTINA_SERIAL_NAND) += spi_nand/cortina_spi_nand_base.o
obj-$(CONFIG_CORTINA_SERIAL_NAND) += spi_nand/spi-nand-core.o
obj-${CONFIG_CMD_SPI_NAND_TEST_FUNC} += spi_nand/cmd_snaf_test.o
obj-$(CONFIG_CORTINA_SERIAL_NAND)  += spi_nand/cmd_spi_nand_bbt.o
obj-$(CONFIG_CORTINA_PNAND) += pnand/ca_nand.o
obj-$(CONFIG_MTD_CORTINA_SERIAL_NOR) += nor/serial_nor.o
obj-$(CONFIG_MTD_CORTINA_SERIAL_NOR) += nor/serial_nor_base.o
obj-${CONFIG_CMD_SPI_NOR_TEST_FUNC} += nor/cmd_spi_nor_test.o
obj-${CONFIG_CMD_DRAM_TEST_FUNC} += cmd_ram_mt.o mdram_test.o cmd_rtk_mem_burst.o
obj-${CONFIG_MMC_DW_CORTINA} += ca_dw_mmc.o
obj-${CONFIG_MMC_RTK_EMMC} += mmc_driver_wrapper.o
obj-$(CONFIG_CMD_PART) += cmd_partinfo.o
CFLAGS_mmc_driver_wrapper.o += -I./drivers/mmc/ -I./drivers/mmc/mmc_cr_v1/sihnah_v1/
ifdef CONFIG_DM_ETH
obj-$(CONFIG_NET) += eth_ni.o
obj-$(CONFIG_TARGET_RTK_TAURUS) += eth_ni_taurus.o
obj-$(CONFIG_TARGET_RTK_ELNATH) += eth_ni_elnath.o
else
ifdef CONFIG_RTK_NI_SERDES
obj-y += serdes/serdes_common.o
obj-$(CONFIG_RTK_NI_SUPPORT_QXGMII_P6) += serdes/p6/serdes_p6.o
obj-$(CONFIG_RTK_NI_SUPPORT_USXGMII_P6) += serdes/p6/serdes_p6.o
obj-$(CONFIG_RTK_NI_SUPPORT_XFI_P6) += serdes/p6/serdes_p6.o
obj-$(CONFIG_RTK_NI_SUPPORT_HSGMII_P4) += serdes/p4/serdes_p4.o
obj-$(CONFIG_RTK_NI_SUPPORT_HSGMII_P5) += serdes/p5/serdes_p5.o
obj-$(CONFIG_RTK_NI_SUPPORT_RXAUI_P5) += serdes/p5/serdes_p5.o
obj-$(CONFIG_RTK_NI_SUPPORT_HSGMII_P6) += serdes/p6/serdes_p6.o
endif
obj-$(CONFIG_RTK_NI_SUPPORT_RTL8221B) += rtl8221b/rtl8221b.o
obj-$(CONFIG_RTK_NI_SUPPORT_RTL8224B) += rtl8224b/rtl8224b.o
obj-$(CONFIG_RTK_NI_SUPPORT_RTL8261I) += rtl8261i/rtl8261i.o
ifdef CONFIG_RTK_NE
# ne
obj-$(CONFIG_NET) += ne/
else
obj-$(CONFIG_NET) += cortina_ni.o
endif
endif
obj-y += cmd_upimgtar.o
obj-y += cmd_upvmimg.o
# obj-$(CONFIG_CORTINA_LED) += cortina_led.o
obj-$(CONFIG_RTK_WATCHDOG) += cmd_rtk_watchdog.o
obj-$(CONFIG_MULTICAST_UPGRADE) += multicast_upgrade.o
obj-$(CONFIG_MULITCASTUPGRADE_BIND_RESETBUTTON) += realtek_gpio_state.o
ifndef CONFIG_RTK_ENG_CMD
obj-${CONFIG_CMD_OTP} += smc_call/cmd_otp.o
endif
obj-${CONFIG_CMD_CRYPTO} += smc_call/cmd_cryptography.o
obj-y += smc_call/cmd_bootloader_ver.o smc_call/cortina_smc.o
obj-$(CONFIG_REALTEK_KASLR) += smc_call/realtek_kalsr.o smc_call/cmd_secure_rand.o
obj-y += cmd_uploadername.o
obj-${CONFIG_TARGET_RTK_VENUS_FT} += 8277B_FT/cmd_ft2.o
obj-${CONFIG_TARGET_RTK_VENUS_FT} += 8277B_FT/i2c-cortina.o
obj-${CONFIG_TARGET_RTK_VENUS_FT} += 8277B_FT/cmd_ne.o
obj-${CONFIG_TARGET_RTK_VENUS_FT} += 8277B_FT/xhci-ca7774.o
obj-$(CONFIG_RTK_GPIO) += realtek_gpio.o

# Mercury FT
obj-${CONFIG_TARGET_RTK_MERCURY_FT} += 6898_FT/cmd_ft.o
obj-${CONFIG_TARGET_RTK_MERCURY_FT} += 6898_FT/aal_phy.o

# socCmd
obj-$(CONFIG_CMD_SOC_TEST) += socCmd/
obj-$(CONFIG_RTK_MULTI_CPU_CMD) += socCmd/

# optee
obj-$(CONFIG_OPTEE) += optee_app/

obj-$(CONFIG_RTK_ENG_CMD) += smc_call/cmd_eng_otp.o smc_call/cmd_eng_cpu_clk.o
obj-$(CONFIG_RTK_PE_CLK_CMD) += 8277x_cortex_freq/elnath_pe_freq.o

