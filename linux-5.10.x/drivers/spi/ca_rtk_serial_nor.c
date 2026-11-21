/*
 * Copyright (c) 2022 Realtek Semiconductor. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __UBOOT__
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/math64.h>

#include <linux/init.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <linux/spi/flash.h>
#include <soc/cortina/cortina-soc.h>
#include <linux/mtd/cfi.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <soc/cortina/registers.h>
#include <soc/cortina/rtk_serial_nor.h>
#include <asm/current.h>
#include <linux/sched.h>
#else
#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <spi_flash.h>
#include <linux/compat.h>
#include "serial_nor.h"
#define SPI_NOR_CLOCK (100)
#endif

#if defined(SPI_NOR_USE_MMIO)
#undef SPI_NOR_USE_PIO
#elif defined(SPI_NOR_USE_PIO)
#undef SPI_NOR_USE_MMIO
#endif

#if !defined(SPI_NOR_USE_MMIO) && !defined(SPI_NOR_USE_PIO)
#error ERROR: One of SPI_NOR_USE_MMIO or SPI_NOR_USE_PIO should be defined
#endif

#define SF_MIN(x, y)    (y ^ ((x ^ y) & -(x < y)))

/* FLASH_SF_EXT_ACCESS bit-field definitions */
#define SF_START_BIT_ENABLE     0x0002
#ifndef __UBOOT__
#if defined(CONFIG_ARCH_CORTINA_VENUS)
#define SF_WAIT_CTRL_RDY		(SF_START_BIT_ENABLE)
#elif defined(CONFIG_ARCH_REALTEK_TAURUS) || defined(CONFIG_ARCH_REALTEK_9607F) || defined(CONFIG_ARCH_REALTEK_MERCURY)
#define SF_WAIT_CTRL_RDY		((1<<16)|SF_START_BIT_ENABLE)
#endif


/* Global variables for NOR Flash driver use  */
static norsf_cmd_info_t s_spi_nor_cmd;
struct mutex sflash_lock;
static u32 sflash_sector_size = 0x00040000;	//0x00010000
static u32 sflash_page_size = 0x00000100;
static u32 sflash_size = 0x01000000;

static void __iomem *flash_ctl_base;
static struct flash_info *info;
struct mtd_info *spi_nor_chipdrv_map_probe(void);
static struct mtd_info *s_spi_nor_mtd_info;

#define read_flash_ctrl_reg(offset)			readl(flash_ctl_base + offset - FLASH_ID)
#define write_flash_ctrl_reg(offset, val)		writel(val, flash_ctl_base + offset - FLASH_ID)

struct ca_sflash {
	struct device *dev;
	struct spi_master *master;
	void __iomem *base;
	u8 tx_bus_width;
	u8 rx_bus_width;
};
#else /* __UBOOT__ */
#if defined(CONFIG_TARGET_VENUS)
#define SF_WAIT_CTRL_RDY		(SF_START_BIT_ENABLE)
#elif defined(CONFIG_TARGET_RTK_TAURUS) || defined(CONFIG_TARGET_RTK_ELNATH) || defined(CONFIG_TARGET_RTK_MERCURY)
#define SF_WAIT_CTRL_RDY		((1<<16)|SF_START_BIT_ENABLE)
#endif

#define FAST_IO_BASE	0x400000000
#define read_flash_ctrl_reg(reg)			readl(reg)
#define write_flash_ctrl_reg(reg, data)		writel(data, reg)

/* Global variables for NOR Flash driver use  */
static norsf_cmd_info_t s_spi_nor_cmd __attribute__ ((section(".data")));
static uint32_t s_serial_nor_is_initted = 0;
static struct spi_flash s_spi_flash;

#define SPI_NOR_DRV_UNUSED (0)
#endif

static int wait_controller_ready(int timeout)
{
#ifndef __UBOOT__
	#define MIN_TRY_COUNT 1000
    int tmp;
    unsigned long timeout_jiffies;
    int count = 0;

    timeout_jiffies = jiffies + msecs_to_jiffies(timeout);

    do {
        tmp = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
        if ((tmp & SF_WAIT_CTRL_RDY) == 0) {
            return 0;
        }

        udelay(1);
        if(current->state == TASK_RUNNING) {
            schedule();
        }
        count++;
    } while (time_before(jiffies, timeout_jiffies) || count < MIN_TRY_COUNT);

    return -ETIME;
#else
	int tmp;
	unsigned long timebase;

	timebase = get_timer(0);
	do {
		tmp = read_flash_ctrl_reg(FLASH_FLASH_ACCESS_START);
		if ((tmp & SF_WAIT_CTRL_RDY) == 0) {
			return 0;
		}
	} while (get_timer(timebase) < timeout);
	return -1;
#endif
}

int serial_nor_compound_cmd(norsf_cmd_attr_t *cmd_attr)
{
    FLASH_SF_ACCESS_t sf_access;
    FLASH_SF_EXT_ACCESS_t sf_ext_access;
    FLASH_FLASH_ACCESS_START_t  sf_access_start;
    uint32_t addr=0;
    uint32_t data_value=0;

    /* Set feature use 1 byte address as sub-command.
         * We must use extend access code to meet this protocol.
         */
    sf_access.wrd = 0;
    sf_access.bf.sflashAcCode = SF_ACCODE_EXTEND_MODE;

    /* Use command in extend_acc register */
    sf_ext_access.wrd = 0;
    sf_ext_access.bf.sflashDirRdCmdEn = 1;
    sf_ext_access.bf.sflashOpCode = cmd_attr->cmd;
    sf_ext_access.bf.sflashDummyCount = cmd_attr->n_dummy - 1;
    sf_ext_access.bf.sflashAddrCount = cmd_attr->n_addr - 1;
    sf_ext_access.bf.sflashDataCount = cmd_attr->n_data - 1;

    if(cmd_attr->n_addr > 0) addr = cmd_attr->addr[0];
    if(cmd_attr->n_addr > 1) addr |= cmd_attr->addr[1]<<8;
    if(cmd_attr->n_addr > 2) addr |= cmd_attr->addr[2]<<16;
    if(cmd_attr->n_addr > 3) addr |= cmd_attr->addr[3]<<24;

    /* Issue command */
    sf_access_start.wrd = 0;
    sf_access_start.bf.sflashRegReq = 1;

    if (cmd_attr->op_dir == attr_tx) {
        sf_access_start.bf.sflashRegCmd = 1;

        if(cmd_attr->n_data > 0) data_value = cmd_attr->data_buf[0];
        if(cmd_attr->n_data > 1) data_value |= cmd_attr->data_buf[1]<<8;
        if(cmd_attr->n_data > 2) data_value |= cmd_attr->data_buf[2]<<16;
        if(cmd_attr->n_data > 3) data_value |= cmd_attr->data_buf[3]<<24;

        write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);
        write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);
        write_flash_ctrl_reg(FLASH_SF_ADDRESS, addr);
        write_flash_ctrl_reg(FLASH_SF_DATA, data_value);
        write_flash_ctrl_reg(FLASH_SF_PREDICTOR, 0);
        write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, sf_access_start.wrd);

        return wait_controller_ready(SFLASH_CMD_TIMEOUT);
    } else if (cmd_attr->op_dir == attr_rx){
        write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);
        write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sf_ext_access.wrd);
        write_flash_ctrl_reg(FLASH_SF_ADDRESS, addr);
        write_flash_ctrl_reg(FLASH_SF_PREDICTOR, 0);
        write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, sf_access_start.wrd);

        if (wait_controller_ready(SFLASH_CMD_TIMEOUT))
            return -1;

        data_value = read_flash_ctrl_reg(FLASH_SF_DATA);

        if(cmd_attr->n_data > 0) cmd_attr->data_buf[0] = data_value&0xFF;
        if(cmd_attr->n_data > 1) cmd_attr->data_buf[1] = (data_value>>8)&0xFF;
        if(cmd_attr->n_data > 2) cmd_attr->data_buf[2] = (data_value>>16)&0xFF;
        if(cmd_attr->n_data > 3) cmd_attr->data_buf[3] = (data_value>>24)&0xFF;

        return 0;
    }
    sfprint("EE: Unsupport command type\n");
    return -1;
}

static int serial_nor_write_enable(void)
{
    int ret;
    FLASH_SF_ACCESS_t sf_access;
    sf_access.wrd = 0;
    sf_access.bf.sflashOpCode = 0x06;
    sf_access.bf.sflashAcCode = SF_ACCODE;
	write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);
	write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

    ret = wait_controller_ready(SFLASH_CMD_TIMEOUT);
    if(ret == -1) return ret;

    return s_spi_nor_cmd.wait_wel();
}

int serial_nor_write_status_reg(uint8_t wdsr_cmd, u16 status, uint8_t data_cnt)
{
    norsf_cmd_attr_t cmd_attr;
    memset(&cmd_attr, 0, sizeof(norsf_cmd_attr_t));
    cmd_attr.cmd = wdsr_cmd;
    cmd_attr.op_dir = attr_tx;
    cmd_attr.n_data= data_cnt;
    cmd_attr.data_buf = (uint8_t *)&status;

    serial_nor_write_enable();

    serial_nor_compound_cmd(&cmd_attr);

    return 0;
}

int serial_nor_read_status_reg(uint8_t rdsr_cmd)
{
    norsf_cmd_attr_t cmd_attr;
    uint8_t readsts;
    memset(&cmd_attr, 0, sizeof(norsf_cmd_attr_t));
    cmd_attr.cmd = rdsr_cmd;
    cmd_attr.op_dir = attr_rx;
    cmd_attr.n_data= 1;
    cmd_attr.data_buf = &readsts;

    serial_nor_compound_cmd(&cmd_attr);

    return readsts;
}

#if defined(SPI_NOR_USE_MMIO)
static void serial_nor_configure_mmio_read(void)
{
    FLASH_SF_ACCESS_t sflash_access;
    FLASH_SF_EXT_ACCESS_t sflash_ext_access;

    sflash_access.wrd = 0;
    sflash_access.bf.sflashAcCode = SF_ACCODE_EXTEND_MODE;
    sflash_access.bf.sflashForceBurst = 1;
    sflash_access.bf.sflashMData = s_spi_nor_cmd.cread->data_io ? 1 : 0;
    sflash_access.bf.sflashMAddr = s_spi_nor_cmd.cread->addr_io ? 1 : 0;
    sflash_access.bf.sflashMIO = s_spi_nor_cmd.cread->data_io;

    sflash_ext_access.wrd = 0;
    sflash_ext_access.bf.sflashOpCode = s_spi_nor_cmd.cread->cmd;
    sflash_ext_access.bf.sflashAddrCount = s_spi_nor_cmd.cread->addr_byte - 1;
    sflash_ext_access.bf.sflashDummyCount = s_spi_nor_cmd.cread->dummy_ck - 1;
    sflash_ext_access.bf.sflashDirRdCmdEn = 1;

    write_flash_ctrl_reg(FLASH_SF_ACCESS, sflash_access.wrd);
    write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sflash_ext_access.wrd);
    write_flash_ctrl_reg(FLASH_SF_PREDICTOR, s_spi_nor_cmd.cread->predictor);
}

static int serial_nor_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	uint8_t *src;

	mutex_lock(&sflash_lock);

	*retlen = len;

	serial_nor_configure_mmio_read();

	src = (uint8_t *)((uintptr_t)(CONFIG_SYS_FLASH_BASE + from));
	memcpy(buf, src, len);

    /* MXIC: Command for exit High Performance Mode */
    write_flash_ctrl_reg(FLASH_SF_ACCESS, 0x1FF);
    write_flash_ctrl_reg(FLASH_SF_DATA, 0x3F);
    write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, 0x202);
    write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, 0);

    mutex_unlock(&sflash_lock);
    return 0;
}

static int serial_nor_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
	uint32_t size;
	uint8_t *src, *dst;
    FLASH_SF_ACCESS_t sflash_access;
    FLASH_SF_EXT_ACCESS_t sflash_ext_access;

	mutex_lock(&sflash_lock);

    *retlen = len;

	src = (uint8_t *)buf;
	dst = (uint8_t *)((uintptr_t)(CONFIG_SYS_FLASH_BASE + to));
	while (len > 0) {
		size = s_spi_nor_cmd.cprog->cprog_lim_b - ((uintptr_t)dst % s_spi_nor_cmd.cprog->cprog_lim_b);
		if (size > len)
			size = len;

		if (serial_nor_write_enable()) {
			break;
		}

        sflash_access.wrd = 0;
        sflash_access.bf.sflashAcCode = SF_ACCODE_EXTEND_MODE;
        sflash_access.bf.sflashForceBurst = 1;
        sflash_access.bf.sflashMData = s_spi_nor_cmd.cprog->data_io ? 1 : 0;
        sflash_access.bf.sflashMAddr = s_spi_nor_cmd.cprog->addr_io ? 1 : 0;
        sflash_access.bf.sflashMIO = s_spi_nor_cmd.cprog->data_io;

        sflash_ext_access.wrd = 0;
        sflash_ext_access.bf.sflashOpCode = s_spi_nor_cmd.cprog->cmd;
        sflash_ext_access.bf.sflashAddrCount = s_spi_nor_cmd.cprog->addr_byte - 1;
        sflash_ext_access.bf.sflashDummyCount = 0x3F;
        sflash_ext_access.bf.sflashDataCount = 3;
        sflash_ext_access.bf.sflashDirRdCmdEn = 1;

        write_flash_ctrl_reg(FLASH_SF_ACCESS, sflash_access.wrd);
        write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sflash_ext_access.wrd);
        write_flash_ctrl_reg(FLASH_SF_PREDICTOR, 0);
		read_flash_ctrl_reg(FLASH_SF_ACCESS);	/* dummy read */
		read_flash_ctrl_reg(FLASH_SF_EXT_ACCESS);	/* dummy read */

//		wmb();
		memcpy( dst, src, size);

        sflash_access.wrd = 0;
        sflash_access.bf.sflashForceTerm = 1;
		write_flash_ctrl_reg(FLASH_SF_ACCESS, sflash_access.wrd);

        if(s_spi_nor_cmd.wait_wip(s_spi_nor_cmd.cprog->cprog_to_us)) {
            printf("EE: write fail, remain 0x%x\n",(uint32_t)len);
        };

		len -= size;
		dst += size;
		src += size;
	}

	mutex_unlock(&sflash_lock);
	return len > 0 ? -1 : 0;
}

#elif defined(SPI_NOR_USE_PIO)
static int serial_nor_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
#define MAX_BYTE_PER_PIO_READ 0x1F00

    uint32_t next_offset = from % mtd->size;
	uint32_t remain = len;
	uint32_t cnt;
	uint32_t reg_data = 0;
    FLASH_SF_ACCESS_t sflash_access;
    FLASH_SF_EXT_ACCESS_t sflash_ext_access;

	mutex_lock(&sflash_lock);

	*retlen = 0;

    sflash_access.wrd = 0;
    sflash_access.bf.sflashAcCode = SF_ACCODE_EXTEND_MODE;
    sflash_access.bf.sflashForceBurst = 1;
    sflash_access.bf.sflashMData = s_spi_nor_cmd.cread->data_io ? 1 : 0;
    sflash_access.bf.sflashMAddr = s_spi_nor_cmd.cread->addr_io ? 1 : 0;
    sflash_access.bf.sflashMIO = s_spi_nor_cmd.cread->data_io;

    sflash_ext_access.wrd = 0;
    sflash_ext_access.bf.sflashOpCode = s_spi_nor_cmd.cread->cmd;
    sflash_ext_access.bf.sflashAddrCount = s_spi_nor_cmd.cread->addr_byte - 1;
    sflash_ext_access.bf.sflashDummyCount = s_spi_nor_cmd.cread->dummy_ck - 1;
    sflash_ext_access.bf.sflashDirRdCmdEn = 1;

    while (remain > 0) {
        sflash_ext_access.bf.sflashDataCount = (remain>MAX_BYTE_PER_PIO_READ)?(MAX_BYTE_PER_PIO_READ-1): (remain-1);  /* len bytes in one command request */

        write_flash_ctrl_reg(FLASH_SF_ACCESS, sflash_access.wrd);
        write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sflash_ext_access.wrd);
        write_flash_ctrl_reg(FLASH_SF_PREDICTOR, s_spi_nor_cmd.cread->predictor);

        /* Fill address */
        write_flash_ctrl_reg(FLASH_SF_ADDRESS, next_offset);

        cnt = sflash_ext_access.bf.sflashDataCount + 1;

        /* Read out data */
        while (cnt >= 4) {
            /* Start access */
            write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

            /* Wait command ready */
            if (wait_controller_ready(SFLASH_CMD_TIMEOUT) != 0) {
                sfprint("%s fail!\n", __func__);
                mutex_unlock(&sflash_lock);
                return -1;
            }
            reg_data = read_flash_ctrl_reg(FLASH_SF_DATA);
            *buf++ = reg_data & 0xFF;
            *buf++ = (reg_data >> 8) & 0xFF;
            *buf++ = (reg_data >> 16) & 0xFF;
            *buf++ = (reg_data >> 24) & 0xFF;

            remain -= 4;
            next_offset += 4;
            *retlen += 4;
            cnt -= 4;
        }

        if (cnt > 0) {
            /* Start access */
            write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

            /* Wait command ready */
            if (wait_controller_ready(SFLASH_CMD_TIMEOUT) != 0) {
                sfprint("%s fail!\n", __func__);
                mutex_unlock(&sflash_lock);
                return -1;
            }
            reg_data = read_flash_ctrl_reg(FLASH_SF_DATA);
            switch (cnt) {
                case 1:
                    *buf++ = reg_data & 0xFF;
                    break;
                case 2:
                    *buf++ = reg_data & 0xFF;
                    *buf++ = (reg_data >> 8) & 0xFF;
                    break;
                case 3:
                    *buf++ = reg_data & 0xFF;
                    *buf++ = (reg_data >> 8) & 0xFF;
                    *buf++ = (reg_data >> 16) & 0xFF;
                    break;
            }

            remain -= cnt;
            next_offset += cnt;
            *retlen += cnt;
            cnt = 0;
        }

        /* MXIC: Command for exit High Performance Mode */
        write_flash_ctrl_reg(FLASH_SF_ACCESS, 0x1FF);
        write_flash_ctrl_reg(FLASH_SF_DATA, 0x3F);
        write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, 0x202);
        write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, 0);
    }

	mutex_unlock(&sflash_lock);
	return 0;
}

static int serial_nor_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
    uint32_t next_offset = to % mtd->size;
    uint32_t cnt;
	uint32_t reg_data;
	uint32_t remain = len;
    FLASH_SF_ACCESS_t sflash_access;
    FLASH_SF_EXT_ACCESS_t sflash_ext_access;
    FLASH_SF_ADDRESS_t sflash_address;
	FLASH_FLASH_ACCESS_START_t access_start;

	mutex_lock(&sflash_lock);

	/* sanity checks */
	if (len == 0 || (to + len > mtd->size)) {
		*retlen = 0;
		mutex_unlock(&sflash_lock);
		return 0;
	}

    *retlen = 0;

    sflash_access.wrd = 0;
    sflash_access.bf.sflashAcCode = SF_ACCODE_EXTEND_MODE;
    sflash_access.bf.sflashForceBurst = 1;
    sflash_access.bf.sflashMData = s_spi_nor_cmd.cprog->data_io ? 1 : 0;
    sflash_access.bf.sflashMAddr = s_spi_nor_cmd.cprog->addr_io ? 1 : 0;
    sflash_access.bf.sflashMIO = s_spi_nor_cmd.cprog->data_io;

    sflash_ext_access.wrd = 0;
    sflash_ext_access.bf.sflashOpCode = s_spi_nor_cmd.cprog->cmd;
    sflash_ext_access.bf.sflashAddrCount = s_spi_nor_cmd.cprog->addr_byte - 1;
    sflash_ext_access.bf.sflashDummyCount = 0x3F;
    sflash_ext_access.bf.sflashDirRdCmdEn = 1;

	access_start.wrd = 0;
	access_start.bf.sflashRegReq = 1;
	access_start.bf.sflashRegCmd = 1;

    while (remain > 0) {
        if (serial_nor_write_enable()) {
            mutex_unlock(&sflash_lock);
            return -EACCES;
        }

        cnt = SF_MIN(remain, (256 - (next_offset % 256)));

        sflash_ext_access.bf.sflashDataCount = cnt - 1;

        write_flash_ctrl_reg(FLASH_SF_ACCESS, sflash_access.wrd);
        write_flash_ctrl_reg(FLASH_SF_EXT_ACCESS, sflash_ext_access.wrd);
        write_flash_ctrl_reg(FLASH_SF_PREDICTOR, 0);

        /* Fill address */
        sflash_address.wrd = next_offset;
        write_flash_ctrl_reg(FLASH_SF_ADDRESS, sflash_address.wrd);

        while (cnt >= 4) {
            reg_data = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
            write_flash_ctrl_reg(FLASH_SF_DATA, reg_data);
            /* Start access */
            write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, access_start.wrd);

            /* Wait command ready */
            if (wait_controller_ready(SFLASH_CMD_TIMEOUT) != 0) {
                sfprint("%s fail!\n", __func__);
                mutex_unlock(&sflash_lock);
                return -1;
            }

            *retlen += 4;
            next_offset += 4;
            remain -= 4;
            buf += 4;
            cnt -= 4;
        }

        if (cnt > 0) {
            reg_data = 0xFFFFFFFF;
            switch (cnt) {
                case 1:
                    reg_data = buf[0];
                    break;
                case 2:
                    reg_data = buf[0] | (buf[1] << 8);
                    break;
                case 3:
                    reg_data = buf[0] | (buf[1] << 8) | (buf[2] << 16);
                    break;
            }
            write_flash_ctrl_reg(FLASH_SF_DATA, reg_data);
            /* Start access */
            write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, access_start.wrd);

            /* Wait command ready */
            if (wait_controller_ready(SFLASH_CMD_TIMEOUT) != 0) {
                sfprint("%s fail!\n", __func__);
                mutex_unlock(&sflash_lock);
                return -ETIME;
            }

            *retlen += cnt;
            next_offset += cnt;
            remain -= cnt;
            buf += cnt;
            cnt = 0;
        }

        /* Terminate burst mode */
        write_flash_ctrl_reg(FLASH_SF_ACCESS, 0x1000);

        if(s_spi_nor_cmd.wait_wip(s_spi_nor_cmd.cprog->cprog_to_us)) {
            sfprint("EE: write fail\n");
            mutex_unlock(&sflash_lock);
            return -EACCES;
        }
    }

	mutex_unlock(&sflash_lock);
	return remain > 0 ? -1 : 0;
}
#endif


static int serial_nor_erase_cmd_sel(struct mtd_info *mtd, const uint32_t offset, const uint32_t len) {
    uint32_t i = 0;
    int res = 0, offset_lmt;
    const norsf_erase_cmd_t *cmds = s_spi_nor_cmd.cerase;

    for (i=0; i<3; i++) {
        offset_lmt = (cmds[i].offset_lmt == 0 ? mtd->size : cmds[i].offset_lmt);
        if ((((int)offset) - offset_lmt) & mtd->size) {
            if ((offset % cmds[i].sz_b) == 0) {
                res = i;
                if (len >= cmds[i].sz_b) {
                    break;
                }
            } else {
                res = -1;
            }
        } else {
            /* res = NORSFG2_E4_BAD_CMD_AND_OFFSET; */
        }
    }

    return res;
}

static int serial_nor_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	u32 len = instr->len;
    int done_b = 0, res = 0;
    uint32_t next_offset = instr->addr % mtd->size;
    FLASH_SF_ACCESS_t sf_access;
	unsigned int ret = 0;

	mutex_lock(&sflash_lock);

	/* Sanity checks */
	if (instr->addr + instr->len > mtd->size) {
		mutex_unlock(&sflash_lock);
		return -EINVAL;
	}

	while (done_b < len) {
        res = serial_nor_erase_cmd_sel(mtd, next_offset, len - done_b);
        if (res < 0) {
            break;
        } else {
    		if (serial_nor_write_enable()) {
                mutex_unlock(&sflash_lock);
                return -EACCES;
    		}

            /* Send Erase command */
             sf_access.wrd = 0;
             sf_access.bf.sflashOpCode = s_spi_nor_cmd.cerase[res].cmd;
             sf_access.bf.sflashAcCode = SF_ACCODE_4_ADDR;
             write_flash_ctrl_reg(FLASH_SF_ACCESS, sf_access.wrd);
             write_flash_ctrl_reg(FLASH_SF_ADDRESS, next_offset);
             write_flash_ctrl_reg(FLASH_FLASH_ACCESS_START, SF_START_BIT_ENABLE);

    		if (wait_controller_ready(SFLASH_CMD_TIMEOUT) || \
                    s_spi_nor_cmd.wait_wip(s_spi_nor_cmd.cerase[res].to_us)) {
                //instr->state = MTD_ERASE_FAILED;
                instr->fail_addr = ((loff_t) next_offset);
                goto erase_exit;
    		}

            done_b += s_spi_nor_cmd.cerase[res].sz_b;
            next_offset += s_spi_nor_cmd.cerase[res].sz_b;

            #ifndef __UBOOT__
            if(current->state == TASK_RUNNING)
            schedule();
            #endif
        }
	}
	//instr->state = MTD_ERASE_DONE;

 erase_exit:
	mutex_unlock(&sflash_lock);
	//ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;
        ret |= (done_b==len) ? 0 : -1;

	//if (!ret)
	//	mtd_erase_callback(instr);

	return (ret);
}

int serial_nor_read_id(void)
{
    norsf_cmd_attr_t cmd_attr;
    uint32_t readid;
    memset(&cmd_attr, 0, sizeof(norsf_cmd_attr_t));

    cmd_attr.cmd = 0x9F;
    cmd_attr.op_dir = attr_rx;
    cmd_attr.n_data= 3;
    cmd_attr.data_buf = (uint8_t *)&readid;

    serial_nor_compound_cmd(&cmd_attr);
	return ((readid&0xff)<<16) | (readid&0xff00) | ((readid&0xff0000)>>16);
}

#ifndef __UBOOT__
static const char * const probe_types[] = {
    "cmdlinepart", "ofpart"
};

static void serial_nor_nop(struct mtd_info *mtd)
{
	/* Nothing to see here */
}

struct mtd_info *spi_nor_chipdrv_map_probe(void)
{
	struct mtd_info *mtd;

	mtd = kmalloc(sizeof(*mtd), GFP_KERNEL);
	if (!mtd) {
		printk("Serial flash allocation failed\n");
		return NULL;
	}

	mutex_init(&sflash_lock);

	memset(mtd, 0, sizeof(*mtd));

	mtd->name = "spi-nor";
	mtd->type = MTD_NORFLASH;
	mtd->_erase = serial_nor_erase;
	mtd->size = sflash_size;
	mtd->_read = serial_nor_read;
	mtd->_write = serial_nor_write;
	mtd->_sync = serial_nor_nop;
	mtd->flags = MTD_CAP_NORFLASH;

	mtd->erasesize = 0x1000;
	//mtd->writesize = sflash_sector_size;
	mtd->writesize = 1;

	__module_get(THIS_MODULE);

	return mtd;
}

static int ca_sflash_exec_op(struct spi_mem *mem, const struct spi_mem_op *op)
{
    //FIXME
	return 0;
}
int serial_nor_initialization(struct platform_device *pdev)
{
	const struct spi_device_id *jid;
	struct mtd_part_parser_data ppdata = {
		.origin = pdev->resource->start,
	};
	int err;
	FLASH_TYPE_t flash_type;
    memset(&s_spi_nor_cmd, 0, sizeof(norsf_cmd_info_t));

	jid = serial_nor_scan_id_table(serial_nor_read_id());

	if( jid == NULL )
			return -EIO;

	info = (void *)jid->driver_data;

	sflash_sector_size = info->sector_size;
	sflash_page_size = info->page_size;
	sflash_size = info->sector_size * info->n_sectors; //this driver use BLOCK erase actually

	/* enable 4-byte addressing if the device exceeds 16MiB */
	if (sflash_size > 0x1000000) {
        printk("SPI NOR: %dMB, 4-Byte addr mode, ",sflash_size/(1024*1024));
        flash_type.wrd = read_flash_ctrl_reg(FLASH_TYPE);
        flash_type.bf.flashSize = 2;
        write_flash_ctrl_reg(FLASH_TYPE, flash_type.wrd);
	} else {
        /* use strapping setting */
        printk("SPI NOR: %dMB, 3-Byte addr mode, ",sflash_size/(1024*1024));
    }

    /* Configurate global command info structure for later driver use */
    serial_nor_configure_cmd_info(&s_spi_nor_cmd, info);


#ifndef __UBOOT__
	sfprint("SPI NOR: 0x%02x(1%d%d) / 0x%02x(1%d%d)", s_spi_nor_cmd.cread->cmd, (1<<s_spi_nor_cmd.cread->addr_io), (1<<s_spi_nor_cmd.cread->data_io),
    s_spi_nor_cmd.cprog->cmd, (1<<s_spi_nor_cmd.cprog->addr_io), (1<<s_spi_nor_cmd.cprog->data_io));
#endif

#ifdef SFLASH_USE_QIO
    /* Enable QUAD mode if support */
    if(info->flags & NEED_QE){
        printk("SPI NOR: Enable QUAD mode\n");
        s_spi_nor_cmd.cread->xread_ex();
        s_spi_nor_cmd.cread->xread_en();
    }
#endif

	s_spi_nor_mtd_info = spi_nor_chipdrv_map_probe();

    if (s_spi_nor_mtd_info == NULL) {
        printk("Serial flash probe failed\n");
        return -EIO;
    }

	s_spi_nor_mtd_info->owner = THIS_MODULE;

	err = mtd_device_parse_register(s_spi_nor_mtd_info, probe_types, &ppdata, NULL, 0);
	if (err) {
		dev_err(&pdev->dev, "failed to add partitions\n");
		return -EIO;
	}

	return 0;
}

static const struct spi_controller_mem_ops ca_sflash_mem_ops = {
      .exec_op = ca_sflash_exec_op,
};

static const struct of_device_id ca_sflash_dt_ids[] = {
	{.compatible = "cortina,ca-sflash"},
	{},
};

MODULE_DEVICE_TABLE(of, ca_sflash_dt_ids);

static int ca_sflash_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct ca_sflash *spi;
	struct spi_master *master;
	struct resource *res;
	u32 tx_bus_width = 0, rx_bus_width = 0;
	int ret = 0;

	/* Only support device-tree instantiation */
	if (!np) {
		return -ENODEV;
	}
	if (!of_match_node(ca_sflash_dt_ids, np)) {
		return -ENODEV;
	}
	master = spi_alloc_master(dev, sizeof(struct ca_sflash));
	if (!master) {
		dev_err(dev, "!error allocating spi_master\n");
		return -ENOMEM;
	}
	master->bus_num = -1;
	if (!of_property_read_u32(np, "spi-tx-bus-width", &tx_bus_width)) {
		switch (tx_bus_width) {
		case 1:
			break;
		case 2:
			master->mode_bits |= SPI_TX_DUAL;
			break;
		case 4:
			master->mode_bits |= SPI_TX_QUAD;
			break;
		default:
			dev_warn(dev,
				"spi-tx-bus-width %d not supported\n",
				tx_bus_width);
			break;
		}
	}
	if (!of_property_read_u32(np, "spi-rx-bus-width", &rx_bus_width)) {
		switch (rx_bus_width) {
		case 1:
			break;
		case 2:
			master->mode_bits |= SPI_RX_DUAL;
			break;
		case 4:
			master->mode_bits |= SPI_RX_QUAD;
			break;
		default:
			dev_warn(dev,
				"spi-rx-bus-width %d not supported\n",
				rx_bus_width);
			break;
		}
	}

	master->mem_ops = &ca_sflash_mem_ops;
	master->dev.of_node = dev->of_node;
	spi = spi_master_get_devdata(master);
	spi->master = master;
	spi->dev = &pdev->dev;
	spi->tx_bus_width = (u8)tx_bus_width;
	spi->rx_bus_width = (u8)rx_bus_width;
	platform_set_drvdata(pdev, spi);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "spi-base");
	if (res) {
		spi->base = devm_ioremap_resource(dev, res);
		if (IS_ERR(spi->base)) {
			ret = PTR_ERR(spi->base);
			goto err;
		}
		dev_info(dev, "ca-spi resource - %pr mapped at 0x%pK\n",
			res, spi->base);
	} else {
		dev_err(dev,
			"!can't get regs base addresses(ret = %d)!\n", ret);
		goto err;

	}

	flash_ctl_base = spi->base;
    ret = serial_nor_initialization(pdev);
	if (ret < 0) {
		goto err;
	}
	ret = devm_spi_register_master(dev, master);
	if (ret < 0) {
		dev_err(dev, "!can't register master\n");
		goto err;
	}
	dev_info(dev,
		"spi-flash controller probed, while mode_bits=%#06x\n",
		master->mode_bits);
	return 0;
err:
	spi_master_put(master);
	return ret;
}

static int ca_sflash_remove(struct platform_device *pdev)
{
	struct ca_sflash *spi = platform_get_drvdata(pdev);

	spi_unregister_master(spi->master);

	return 0;
}

static int __maybe_unused ca_sflash_suspend(struct device *dev)
{
    return 0;
}

static int __maybe_unused ca_sflash_resume(struct device *dev)
{
    return 0;
}

static SIMPLE_DEV_PM_OPS(ca_sflash_pm_ops, ca_sflash_suspend, ca_sflash_resume);

static struct platform_driver ca_sflash_driver = {
	.probe	= ca_sflash_probe,
	.remove	= ca_sflash_remove,
	.driver	= {
		.name	= "ca-sflash",
		.pm =   &ca_sflash_pm_ops,
		.of_match_table	= ca_sflash_dt_ids,
	}
};

module_platform_driver(ca_sflash_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("CORTINA ACCESS SPI NOR Controller Driver");
MODULE_ALIAS("platform:ca-sflash");
#else
static int serial_nor_lock(struct spi_flash *flash, loff_t ofs, uint64_t len) {
	return 0;
}

static int serial_nor_unlock(struct spi_flash *flash, loff_t ofs, uint64_t len) {
	return 0;
}

static int serial_nor_is_locked(struct spi_flash *flash, loff_t ofs, uint64_t len) {
	return 0;
}

void spi_flash_free(struct spi_flash *flash)
{
    return;
}

void spi_nor_set_clock(uint32_t sclk)
{
    uint32_t lsaxi_MHz;
    uint32_t sflashClkWidth, final_clock;
    GLOBAL_EPLLDIV_t global_eplldiv;
    global_eplldiv.wrd = readl(GLOBAL_EPLLDIV);

    /* Set LS_AXI to 200MHz */
    if(global_eplldiv.bf.lsaxi_divsel != 20) {
        global_eplldiv.bf.lsaxi_divsel = 20;
        writel(global_eplldiv.wrd, GLOBAL_EPLLDIV);
    }
    lsaxi_MHz = 2000/(global_eplldiv.bf.lsaxi_divsel/2);

    /* Minimize flash timing */
#if defined(CONFIG_TARGET_VENUS)
    write_flash_ctrl_reg(FLASH_SF_TIMING, 0x07010101);
#elif defined(CONFIG_TARGET_RTK_TAURUS) || defined(CONFIG_TARGET_RTK_ELNATH) || defined(CONFIG_TARGET_RTK_MERCURY)
    if(sclk >= 100) {
        write_flash_ctrl_reg(FLASH_SF_TIMING, 0x28010101);
#if defined(CONFIG_RTK_V7_FPGA_PLATFORM) || defined(CONFIG_RTK_V8_FPGA_PLATFORM)
        write_flash_ctrl_reg(FLASH_SF_TIMING, 0x18010101);
#endif
    } else if (sclk >= 50) {
        write_flash_ctrl_reg(FLASH_SF_TIMING, 0x19010101);
    } else if (sclk >= 40) {
        write_flash_ctrl_reg(FLASH_SF_TIMING, 0x17010101);
    }
#endif

    sflashClkWidth = read_flash_ctrl_reg(FLASH_SF_TIMING)>>24&0xF;

    if (sflashClkWidth == 8) final_clock = (lsaxi_MHz/2);
    else if (sflashClkWidth == 9) final_clock = (lsaxi_MHz/4);
    else if (sflashClkWidth == 7) final_clock = (lsaxi_MHz/5);
    else if (sflashClkWidth == 6) final_clock = (lsaxi_MHz/6);
    else if (sflashClkWidth == 5) final_clock = (lsaxi_MHz/8);
    else if (sflashClkWidth == 4) final_clock = (lsaxi_MHz/10);
    else if (sflashClkWidth == 3) final_clock = (lsaxi_MHz/12);
    else if (sflashClkWidth == 2) final_clock = (lsaxi_MHz/14);
    else if (sflashClkWidth == 1) final_clock = (lsaxi_MHz/16);
    else final_clock = (lsaxi_MHz/18);


    printf("SPI NOR: %d MHz\n",final_clock);
}

struct spi_flash *spi_flash_probe(uint32_t bus, uint32_t cs,
				  uint32_t max_hz, uint32_t spi_mode)
{
	const struct spi_device_id *jid;
	struct flash_info *info;
	FLASH_TYPE_t flash_type;

    if(s_serial_nor_is_initted){
        return &s_spi_flash;
    }

    memset(&s_spi_nor_cmd, 0, sizeof(norsf_cmd_info_t));
    memset(&s_spi_flash, 0, sizeof(struct spi_flash));


    /* Set FLASH_TYPE as serial flash */
    write_flash_ctrl_reg(FLASH_TYPE, 0);

	jid = serial_nor_scan_id_table(serial_nor_read_id());
	if (!jid) {
		return NULL;
    }

    info = (void *)jid->driver_data;

	s_spi_flash.spi = NULL;
	s_spi_flash.name = jid->name;
	s_spi_flash.size = info->sector_size * info->n_sectors;
	s_spi_flash.page_size = SPI_NOR_DRV_UNUSED;
	s_spi_flash.sector_size = info->sector_size;
	s_spi_flash.erase_size = info->sector_size;
	s_spi_flash.erase_opcode = SPI_NOR_DRV_UNUSED;
	s_spi_flash.read_opcode = SPI_NOR_DRV_UNUSED;
	s_spi_flash.program_opcode = SPI_NOR_DRV_UNUSED;
	s_spi_flash.read_dummy = SPI_NOR_DRV_UNUSED;
	s_spi_flash.flash_lock = serial_nor_lock;
	s_spi_flash.flash_unlock = serial_nor_unlock;
	s_spi_flash.flash_is_locked = serial_nor_is_locked;


    struct mtd_info *mtd = &s_spi_flash.mtd;
    mtd->name = "SPI-NOR";
    mtd->priv = &s_spi_flash;
    mtd->type = MTD_NORFLASH;
    mtd->writesize = 1;
    mtd->flags = MTD_CAP_NORFLASH;
    mtd->size = s_spi_flash.size;
    mtd->_erase = serial_nor_erase;
    mtd->_read = serial_nor_read;
    mtd->_write = serial_nor_write;
    mtd->erasesize = 1;

	/* enable 4-byte addressing if the device exceeds 16MiB */
    if (s_spi_flash.size > 0x1000000) {
        printf("SPI NOR: %dMB, 4-Byte addr mode, ",s_spi_flash.size/(1024*1024));
        flash_type.wrd = read_flash_ctrl_reg(FLASH_TYPE);
        flash_type.bf.flashSize = 2;
        write_flash_ctrl_reg(FLASH_TYPE, flash_type.wrd);
    } else {
        /* use strapping setting */
        printf("SPI NOR: %dMB, 3-Byte addr mode, ",s_spi_flash.size/(1024*1024));
    }

    /* Configurate global command info structure for later driver use */
    serial_nor_configure_cmd_info(&s_spi_nor_cmd, info);

    /* Configurate SPI NOR Clock */
    spi_nor_set_clock(SPI_NOR_CLOCK);

    if(info->flags & NEED_QE){
        s_spi_nor_cmd.cread->xread_ex();
        s_spi_nor_cmd.cread->xread_en();
    }

    s_serial_nor_is_initted = 1;
	return &s_spi_flash;
}
#endif

