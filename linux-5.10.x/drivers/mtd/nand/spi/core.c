// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016-2017 Micron Technology, Inc.
 *
 * Authors:
 *	Peter Pan <peterpandong@micron.com>
 *	Boris Brezillon <boris.brezillon@bootlin.com>
 */

#define pr_fmt(fmt)	"spi-nand: " fmt

#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/spinand.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <soc/cortina/cortina-soc.h>
#include "block_state_proc.h"
#include "nand_info_proc.h"

#ifdef CONFIG_NAND_HIDE_BAD
/*
 * Standard SPI-NAND flash commands
 */
#define SPINAND_CMD_READ_FROM_CACHE					0x03
#define SPINAND_CMD_READ_FROM_CACHE_FAST			0x0b /* CMD_READ_RDM */
#define SPINAND_CMD_READ_FROM_CACHE_X2				0x3b /* CMD_READ_RDM_X2 */
#define SPINAND_CMD_READ_FROM_CACHE_DUAL_IO			0xbb
#define SPINAND_CMD_READ_FROM_CACHE_X4				0x6b /* CMD_READ_RDM_X4 */
#define SPINAND_CMD_READ_FROM_CACHE_QUAD_IO			0xeb
#define SPINAND_CMD_PROG_LOAD						0x02 /* CMD_PROG_PAGE_CLRCACHE */
#define SPINAND_CMD_PROG_LOAD_RDM_DATA				0x84 /* CMD_PROG_PAGE */
#define SPINAND_CMD_PROG_LOAD_X4					0x32 /* CMD_PROG_PAGE_CLRCACHE_X4 */
#define SPINAND_CMD_PROG_LOAD_RDM_DATA_X4			0x34

/*
 * Factory Bad Block mech
 */
#define BAD_BLOCK_LIST_ELEMENT	128
#define BBL_INDICATORS          4
#define VENUS_CONFIG_BBL_OFFSET         (0x180000)  /* 1.5MB */
//#define VENUS_CONFIG_BBL_OFFSET_REDUND  (0x1C0000)

#define TAURUS_CONFIG_BBL_OFFSET        (0x200000)  /* 2.0MB */
//#define TAURUS_CONFIG_BBL_OFFSET_REDUND (0x240000)

#define BBL0_NAME 0x42626c30 //'Bbl0'
#define BBL1_NAME 0x316C6242 //'1lbB'

enum {
	FACTORY_1ST_SCAN = 1,
	FACTORY_FROM_BBT0= 2,
	FACTORY_FROM_BBT1= 3,
};
static u8 bbl_source=0;
static u8 acc_phy_offset=0;
static u8 bad_block_cnt=0;
static uint32_t bad_block_list[BAD_BLOCK_LIST_ELEMENT];

//static int spi_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip);
static bool spinand_isbad(struct nand_device *nand, const struct nand_pos *pos);
extern int flash_uuid_init(u8 *uuid, int len);
static int F50L1G41LB_nandflash_uid_detect(struct spinand_device *spinand, u8* uuid);
static int F50L1G41LC_nandflash_uid_detect(struct spinand_device *spinand, u8* uuid);
static int F50L2G41XA_nandflash_uid_detect(struct spinand_device *spinand, u8* uuid);

struct nand_flash_dev_uid {
	char *name;
	union {
		struct {
			uint8_t mfr_id;
			uint8_t dev_id;
		};
		uint8_t id[SPINAND_MAX_ID_LEN];
	};
	uint16_t id_len;
	int (*flash_uid_detect)(struct spinand_device *spinand, u8* uuid);
};

struct nand_flash_dev_uid nand_flash_uids[] = {
	{"F50L1G41LB(2M) SPI NAND 1G", { .id = {0xc8, 0x01} }, 2, F50L1G41LB_nandflash_uid_detect},
	{"F50L1G41LC(2P) SPI NAND 1G", { .id = {0x8c, 0x2c} }, 2, F50L1G41LC_nandflash_uid_detect},
	{"F50L2G41XA(2B) SPI NAND 2G", { .id = {0x2c, 0x24} }, 2, F50L2G41XA_nandflash_uid_detect},
	{NULL}
};

/* "crc16_tab & cyg_crc16" are Inherited from u-boot-2015.04 */
/* Table of CRC constants - implements x^16+x^12+x^5+1 */
static const uint16_t crc16_tab[] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};
static uint16_t cyg_crc16(unsigned char *buf, int len)
{
	int i;
	uint16_t cksum;

	cksum = 0;
	for (i = 0;  i < len;  i++) {
		cksum = crc16_tab[((cksum>>8) ^ *buf++) & 0xFF] ^ (cksum << 8);
	}
	return cksum;
}
static int spinand_mtd_read(struct mtd_info *mtd, loff_t from,  struct mtd_oob_ops *ops);
static int spinand_mtd_write(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops);

static int hide_bad_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
		     size_t *retlen, uint8_t *buf)
{
	int ret = 0;
	struct mtd_oob_ops ops = {
		.len = len,
		.datbuf = buf,
	};

	ret = spinand_mtd_read(mtd, from, &ops);
	if (ret < 0)
		pr_info("%s: error! ret=%d\n", __func__, ret);
	*retlen = ops.retlen;

	return ret;
}

static void scan_all_bad_blocks(struct mtd_info *mtd, uint32_t start, uint32_t size)
{
	struct nand_device *nand = mtd_to_nanddev(mtd);
	struct nand_pos pos;
	loff_t offset;
	uint32_t ret=0;
	nand_block_state.bad_block_num = 0;
	offset = start;
	while((offset-start) < size){
		 nanddev_offs_to_pos(nand, offset, &pos);
		 ret = spinand_isbad(nand, &pos);
		 if(ret == 1){
			 nand_block_state.bad_block_num += 1;
		 }
		 offset += nanddev_eraseblock_size(nand);
	}
	nand_block_state.block_num = nanddev_eraseblocks_per_target(nand);

}

static void get_spi_nand_factory_bad_blocks(struct mtd_info *mtd, uint32_t start, uint32_t size)
{
	//struct spi_nand_chip *chip = mtd->priv;
	struct nand_device *nand = mtd_to_nanddev(mtd);
	struct nand_pos pos;
	/* Check factory bad stored in flash chip:
		 * 1) Read Bbl0 if the factory bad had already been scanned
		 * 2) If Bbl0 is bad. Read Bbl1 for the factory bad
		 */
	loff_t offset;
	uint32_t ret=0;
	size_t retlen;
	uint32_t indicator[BBL_INDICATORS];
	uint32_t tempread[BAD_BLOCK_LIST_ELEMENT];
	loff_t bbl_off = 0;
	memset(indicator, 0xF, sizeof(indicator));
	memset(tempread, 0xF, sizeof(tempread));
	if (RTK_BBL_OFFSET)
		bbl_off = RTK_BBL_OFFSET;
	else if (RTK_SOC_TAURUS_GEN)
		bbl_off = TAURUS_CONFIG_BBL_OFFSET;
	else
		bbl_off = VENUS_CONFIG_BBL_OFFSET;

	acc_phy_offset = 1;
	for(offset=bbl_off ; offset < nanddev_size(nand); offset += nanddev_eraseblock_size(nand) ){
		nanddev_offs_to_pos(nand, offset, &pos);
		ret = spinand_isbad(nand, &pos);
		if(ret == 1) continue;
		hide_bad_mtd_read(mtd, offset, sizeof(indicator), &retlen, (u8 *)indicator);

		if(((indicator[0] == BBL0_NAME) || (indicator[0] == BBL1_NAME)) && (indicator[2] == indicator[3])){
			ret = hide_bad_mtd_read(mtd, offset + nanddev_page_size(nand), sizeof(tempread), &retlen, (u8 *)tempread);
			if(indicator[2] == cyg_crc16((u8 *)tempread, sizeof(tempread))){
				bad_block_cnt = indicator[1];
				bbl_source = (indicator[0]==BBL0_NAME)?FACTORY_FROM_BBT0:FACTORY_FROM_BBT1;
				memcpy(bad_block_list, tempread, sizeof(bad_block_list));
				acc_phy_offset = 0;
				pr_info("spi-nand: BBL OFFSET found at (0x%08llx)\n", offset);
				return;
			}
		}
	}

	/* If the BBLn is not read from flash, scan the BBI in OOB */
	offset = start;
	bbl_source = FACTORY_1ST_SCAN;
	bad_block_cnt = 0;
	memset(bad_block_list, 0xF, sizeof(bad_block_list));
	while(((offset - start) < size) && (bad_block_cnt<BAD_BLOCK_LIST_ELEMENT)) {
		 nanddev_offs_to_pos(nand, offset, &pos);
		 ret = spinand_isbad(nand, &pos);
		 if(ret == 1){
			bad_block_list[bad_block_cnt++] = offset;
		}
		offset += nanddev_eraseblock_size(nand) ;
	}
	mtd->size -= bad_block_cnt*nanddev_eraseblock_size(nand) ;
	acc_phy_offset = 0;
	return;
}

 int xlat_offset_to_skip_factory_bad(	struct nand_device *nand,  struct nand_pos *pos)
{
	uint32_t i;
	loff_t offset;

	/* Just return original offset:
	     1. if no bad block found
	     2. Physical flash offset access is assigned */
	if((bad_block_cnt==0) || (acc_phy_offset==1)){
		return 0;
	}

	offset = nanddev_pos_to_offs(nand, pos);
	for(i=0; i<bad_block_cnt; i++){
		if(bad_block_list[i] <= offset){
			offset += nanddev_eraseblock_size(nand);
		}else{
			break;
		}
	}
	if(offset >= nanddev_size(nand)){
		return -EINVAL;
	}
	nanddev_offs_to_pos(nand, offset, pos);
	return 0;
}

#endif /* #ifdef CONFIG_NAND_HIDE_BAD */
static int spinand_read_reg_op(struct spinand_device *spinand, u8 reg, u8 *val)
{
	struct spi_mem_op op = SPINAND_GET_FEATURE_OP(reg,
						      spinand->scratchbuf);
	int ret;

	ret = spi_mem_exec_op(spinand->spimem, &op);
	if (ret)
		return ret;

	*val = *spinand->scratchbuf;
	return 0;
}

static int spinand_write_reg_op(struct spinand_device *spinand, u8 reg, u8 val)
{
	struct spi_mem_op op = SPINAND_SET_FEATURE_OP(reg,
						      spinand->scratchbuf);

	*spinand->scratchbuf = val;
	return spi_mem_exec_op(spinand->spimem, &op);
}

static int spinand_read_status(struct spinand_device *spinand, u8 *status)
{
	return spinand_read_reg_op(spinand, REG_STATUS, status);
}

static int spinand_get_cfg(struct spinand_device *spinand, u8 *cfg)
{
	struct nand_device *nand = spinand_to_nand(spinand);

	if (WARN_ON(spinand->cur_target < 0 ||
		    spinand->cur_target >= nand->memorg.ntargets))
		return -EINVAL;

	*cfg = spinand->cfg_cache[spinand->cur_target];
	return 0;
}

static int spinand_set_cfg(struct spinand_device *spinand, u8 cfg)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	int ret;

	if (WARN_ON(spinand->cur_target < 0 ||
		    spinand->cur_target >= nand->memorg.ntargets))
		return -EINVAL;

	if (spinand->cfg_cache[spinand->cur_target] == cfg)
		return 0;

	ret = spinand_write_reg_op(spinand, REG_CFG, cfg);
	if (ret)
		return ret;

	spinand->cfg_cache[spinand->cur_target] = cfg;
	return 0;
}

/**
 * spinand_upd_cfg() - Update the configuration register
 * @spinand: the spinand device
 * @mask: the mask encoding the bits to update in the config reg
 * @val: the new value to apply
 *
 * Update the configuration register.
 *
 * Return: 0 on success, a negative error code otherwise.
 */
int spinand_upd_cfg(struct spinand_device *spinand, u8 mask, u8 val)
{
	int ret;
	u8 cfg;

	ret = spinand_get_cfg(spinand, &cfg);
	if (ret)
		return ret;

	cfg &= ~mask;
	cfg |= val;

	return spinand_set_cfg(spinand, cfg);
}

/**
 * spinand_select_target() - Select a specific NAND target/die
 * @spinand: the spinand device
 * @target: the target/die to select
 *
 * Select a new target/die. If chip only has one die, this function is a NOOP.
 *
 * Return: 0 on success, a negative error code otherwise.
 */
int spinand_select_target(struct spinand_device *spinand, unsigned int target)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	int ret;

	if (WARN_ON(target >= nand->memorg.ntargets))
		return -EINVAL;

	if (spinand->cur_target == target)
		return 0;

	if (nand->memorg.ntargets == 1) {
		spinand->cur_target = target;
		return 0;
	}

	ret = spinand->select_target(spinand, target);
	if (ret)
		return ret;

	spinand->cur_target = target;
	return 0;
}

static int spinand_init_cfg_cache(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	struct device *dev = &spinand->spimem->spi->dev;
	unsigned int target;
	int ret;

	spinand->cfg_cache = devm_kcalloc(dev,
					  nand->memorg.ntargets,
					  sizeof(*spinand->cfg_cache),
					  GFP_KERNEL);
	if (!spinand->cfg_cache)
		return -ENOMEM;

	for (target = 0; target < nand->memorg.ntargets; target++) {
		ret = spinand_select_target(spinand, target);
		if (ret)
			return ret;

		/*
		 * We use spinand_read_reg_op() instead of spinand_get_cfg()
		 * here to bypass the config cache.
		 */
		ret = spinand_read_reg_op(spinand, REG_CFG,
					  &spinand->cfg_cache[target]);
		if (ret)
			return ret;
	}

	return 0;
}

static int spinand_init_quad_enable(struct spinand_device *spinand)
{
	bool enable = false;

	if (!(spinand->flags & SPINAND_HAS_QE_BIT))
		return 0;

	if (spinand->op_templates.read_cache->data.buswidth == 4 ||
	    spinand->op_templates.write_cache->data.buswidth == 4 ||
	    spinand->op_templates.update_cache->data.buswidth == 4)
		enable = true;

	return spinand_upd_cfg(spinand, CFG_QUAD_ENABLE,
			       enable ? CFG_QUAD_ENABLE : 0);
}

static int spinand_ecc_enable(struct spinand_device *spinand,
			      bool enable)
{
	return spinand_upd_cfg(spinand, CFG_ECC_ENABLE,
			       enable ? CFG_ECC_ENABLE : 0);
}

static int spinand_write_enable_op(struct spinand_device *spinand)
{
	struct spi_mem_op op = SPINAND_WR_EN_DIS_OP(true);

	return spi_mem_exec_op(spinand->spimem, &op);
}

static int spinand_load_page_op(struct spinand_device *spinand,
				const struct nand_page_io_req *req)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int row = nanddev_pos_to_row(nand, &req->pos);
	struct spi_mem_op op = SPINAND_PAGE_READ_OP(row);

	return spi_mem_exec_op(spinand->spimem, &op);
}

static int spinand_read_from_cache_op(struct spinand_device *spinand,
				      const struct nand_page_io_req *req)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	struct mtd_info *mtd = nanddev_to_mtd(nand);
	struct spi_mem_dirmap_desc *rdesc;
	unsigned int nbytes = 0;
	void *buf = NULL;
	u16 column = 0;
	ssize_t ret;

	if (req->datalen) {
		buf = spinand->databuf;
		nbytes = nanddev_page_size(nand);
		column = 0;
	}

	if (req->ooblen) {
		nbytes += nanddev_per_page_oobsize(nand);
		if (!buf) {
			buf = spinand->oobbuf;
			column = nanddev_page_size(nand);
		}
	}

	rdesc = spinand->dirmaps[req->pos.plane].rdesc;

	while (nbytes) {
		ret = spi_mem_dirmap_read(rdesc, column, nbytes, buf);
		if (ret < 0)
			return ret;

		if (!ret || ret > nbytes)
			return -EIO;

		nbytes -= ret;
		column += ret;
		buf += ret;
	}

	if (req->datalen)
		memcpy(req->databuf.in, spinand->databuf + req->dataoffs,
		       req->datalen);

	if (req->ooblen) {
		if (req->mode == MTD_OPS_AUTO_OOB)
			mtd_ooblayout_get_databytes(mtd, req->oobbuf.in,
						    spinand->oobbuf,
						    req->ooboffs,
						    req->ooblen);
		else
			memcpy(req->oobbuf.in, spinand->oobbuf + req->ooboffs,
			       req->ooblen);
	}

	return 0;
}

static int spinand_write_to_cache_op(struct spinand_device *spinand,
				     const struct nand_page_io_req *req)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	struct mtd_info *mtd = nanddev_to_mtd(nand);
	struct spi_mem_dirmap_desc *wdesc;
	unsigned int nbytes, column = 0;
	void *buf = spinand->databuf;
	ssize_t ret;

	/*
	 * Looks like PROGRAM LOAD (AKA write cache) does not necessarily reset
	 * the cache content to 0xFF (depends on vendor implementation), so we
	 * must fill the page cache entirely even if we only want to program
	 * the data portion of the page, otherwise we might corrupt the BBM or
	 * user data previously programmed in OOB area.
	 */
	nbytes = nanddev_page_size(nand) + nanddev_per_page_oobsize(nand);
	memset(spinand->databuf, 0xff, nbytes);

	if (req->datalen)
		memcpy(spinand->databuf + req->dataoffs, req->databuf.out,
		       req->datalen);

	if (req->ooblen) {
		if (req->mode == MTD_OPS_AUTO_OOB)
			mtd_ooblayout_set_databytes(mtd, req->oobbuf.out,
						    spinand->oobbuf,
						    req->ooboffs,
						    req->ooblen);
		else
			memcpy(spinand->oobbuf + req->ooboffs, req->oobbuf.out,
			       req->ooblen);
	}

	wdesc = spinand->dirmaps[req->pos.plane].wdesc;

	while (nbytes) {
		ret = spi_mem_dirmap_write(wdesc, column, nbytes, buf);
		if (ret < 0)
			return ret;

		if (!ret || ret > nbytes)
			return -EIO;

		nbytes -= ret;
		column += ret;
		buf += ret;
	}

	return 0;
}

static int spinand_program_op(struct spinand_device *spinand,
			      const struct nand_page_io_req *req)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int row = nanddev_pos_to_row(nand, &req->pos);
	struct spi_mem_op op = SPINAND_PROG_EXEC_OP(row);

	return spi_mem_exec_op(spinand->spimem, &op);
}

static int spinand_erase_op(struct spinand_device *spinand,
			    const struct nand_pos *pos)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int row = nanddev_pos_to_row(nand, pos);
	struct spi_mem_op op = SPINAND_BLK_ERASE_OP(row);

	return spi_mem_exec_op(spinand->spimem, &op);
}

static int spinand_wait(struct spinand_device *spinand, u8 *s)
{
	unsigned long timeo =  jiffies + msecs_to_jiffies(400);
	u8 status;
	int ret;

	do {
		ret = spinand_read_status(spinand, &status);
		if (ret)
			return ret;

		if (!(status & STATUS_BUSY))
			goto out;
	} while (time_before(jiffies, timeo));

	/*
	 * Extra read, just in case the STATUS_READY bit has changed
	 * since our last check
	 */
	ret = spinand_read_status(spinand, &status);
	if (ret)
		return ret;

out:
	if (s)
		*s = status;

	return status & STATUS_BUSY ? -ETIMEDOUT : 0;
}

static int spinand_read_id_op(struct spinand_device *spinand, u8 naddr,
			      u8 ndummy, u8 *buf)
{
	struct spi_mem_op op = SPINAND_READID_OP(
		naddr, ndummy, spinand->scratchbuf, SPINAND_MAX_ID_LEN);
	int ret;

	ret = spi_mem_exec_op(spinand->spimem, &op);
	if (!ret)
		memcpy(buf, spinand->scratchbuf, SPINAND_MAX_ID_LEN);

	return ret;
}

static int spinand_reset_op(struct spinand_device *spinand)
{
	struct spi_mem_op op = SPINAND_RESET_OP;
	int ret;

	ret = spi_mem_exec_op(spinand->spimem, &op);
	if (ret)
		return ret;

	return spinand_wait(spinand, NULL);
}

static int spinand_lock_block(struct spinand_device *spinand, u8 lock)
{
	return spinand_write_reg_op(spinand, REG_BLOCK_LOCK, lock);
}

static int spinand_check_ecc_status(struct spinand_device *spinand, u8 status)
{
	struct nand_device *nand = spinand_to_nand(spinand);

	if (spinand->eccinfo.get_status)
		return spinand->eccinfo.get_status(spinand, status);

	switch (status & STATUS_ECC_MASK) {
	case STATUS_ECC_NO_BITFLIPS:
		return 0;

	case STATUS_ECC_HAS_BITFLIPS:
		/*
		 * We have no way to know exactly how many bitflips have been
		 * fixed, so let's return the maximum possible value so that
		 * wear-leveling layers move the data immediately.
		 */
		return nanddev_get_ecc_requirements(nand)->strength;

	case STATUS_ECC_UNCOR_ERROR:
		return -EBADMSG;

	default:
		break;
	}

	return -EINVAL;
}

static int spinand_read_page(struct spinand_device *spinand,
			     const struct nand_page_io_req *req,
			     bool ecc_enabled)
{
	u8 status;
	int ret;

	ret = spinand_load_page_op(spinand, req);
	if (ret)
		return ret;

	ret = spinand_wait(spinand, &status);
	if (ret < 0)
		return ret;

	ret = spinand_read_from_cache_op(spinand, req);
	if (ret)
		return ret;

	if (!ecc_enabled)
		return 0;

	return spinand_check_ecc_status(spinand, status);
}

static int spinand_write_page(struct spinand_device *spinand,
			      const struct nand_page_io_req *req)
{
	u8 status;
	int ret;

	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	ret = spinand_write_to_cache_op(spinand, req);
	if (ret)
		return ret;

	ret = spinand_program_op(spinand, req);
	if (ret)
		return ret;

	ret = spinand_wait(spinand, &status);
	if (!ret && (status & STATUS_PROG_FAILED))
		ret = -EIO;

	return ret;
}

static int spinand_mtd_read(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
	struct spinand_device *spinand = mtd_to_spinand(mtd);
	struct nand_device *nand = mtd_to_nanddev(mtd);
	unsigned int max_bitflips = 0;
	struct nand_io_iter iter;
	bool enable_ecc = false;
	bool ecc_failed = false;
	int ret = 0;
#ifdef CONFIG_SPI_NAND_PAGECACHE
    unsigned int row;
#endif
#ifdef CONFIG_NAND_HIDE_BAD
	struct nand_pos tmp_pos;
	nanddev_offs_to_pos(nand, from, &tmp_pos);
#endif

	if (ops->mode != MTD_OPS_RAW && spinand->eccinfo.ooblayout)
		enable_ecc = true;

	mutex_lock(&spinand->lock);

	nanddev_io_for_each_page(nand, NAND_PAGE_READ, from, ops, &iter) {
#ifdef CONFIG_NAND_HIDE_BAD
		iter.req.pos = tmp_pos;
		ret = xlat_offset_to_skip_factory_bad(nand, &iter.req.pos);
		if(ret == -EINVAL){
			pr_err("%s: offset xlat exceeds chip size\n",__FUNCTION__);
			return ret;
		}
		if(nanddev_pos_cmp(&tmp_pos, &iter.req.pos) !=0 ){
			pr_debug("Read xlat: 0x%llx => 0x%llx\n", nanddev_pos_to_offs(nand, &tmp_pos), nanddev_pos_to_offs(nand, &iter.req.pos));
			//printk("Read xlat: 0x%llx => 0x%llx\n", nanddev_pos_to_offs(nand, &tmp_pos), nanddev_pos_to_offs(nand, &iter.req.pos) );
		}
		nanddev_pos_next_page(nand, &tmp_pos);
#endif
		ret = spinand_select_target(spinand, iter.req.pos.target);
		if (ret)
			break;

		ret = spinand_ecc_enable(spinand, enable_ecc);
		if (ret)
			break;

#ifdef CONFIG_SPI_NAND_PAGECACHE
/*
  * Read performance enhancement
  * For the page that had been read last time,
  * just copy the flash data from [spinand->databuf] instead of sending out command again.
  */
        row = nanddev_pos_to_row(nand, &iter.req.pos);
		if ((row != spinand->pagecache.page) || (iter.req.pos.target != spinand->pagecache.die)) {
			ret = spinand_read_page(spinand, &iter.req, enable_ecc);
            if (ret < 0 && ret != -EBADMSG)
                break;

            if (ret == -EBADMSG) {
                ecc_failed = true;
                mtd->ecc_stats.failed++;
            } else {
                mtd->ecc_stats.corrected += ret;
                max_bitflips = max_t(unsigned int, max_bitflips, ret);
            }

			spinand->pagecache.bitflips = max_bitflips;
            spinand->pagecache.page = row;
			spinand->pagecache.die = iter.req.pos.target;
		} else {
            if (iter.req.datalen)
                memcpy(iter.req.databuf.in, spinand->databuf + iter.req.dataoffs,
                       iter.req.datalen);

            if (iter.req.ooblen) {
                if (iter.req.mode == MTD_OPS_AUTO_OOB)
                    mtd_ooblayout_get_databytes(mtd, iter.req.oobbuf.in,
                                    spinand->oobbuf,
                                    iter.req.ooboffs,
                                    iter.req.ooblen);
                else
                    memcpy(iter.req.oobbuf.in, spinand->oobbuf + iter.req.ooboffs,
                           iter.req.ooblen);
            }

            max_bitflips = max_t(unsigned int, max_bitflips, spinand->pagecache.bitflips);
		}
#else
		ret = spinand_read_page(spinand, &iter.req, enable_ecc);
		if (ret < 0 && ret != -EBADMSG)
			break;

		if (ret == -EBADMSG) {
			ecc_failed = true;
			mtd->ecc_stats.failed++;
		} else {
			mtd->ecc_stats.corrected += ret;
			max_bitflips = max_t(unsigned int, max_bitflips, ret);
		}
#endif
		ret = 0;
		ops->retlen += iter.req.datalen;
		ops->oobretlen += iter.req.ooblen;
	}

	mutex_unlock(&spinand->lock);

	if (ecc_failed && !ret)
		ret = -EBADMSG;

	return ret ? ret : max_bitflips;
}

static int spinand_mtd_write(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops)
{
	struct spinand_device *spinand = mtd_to_spinand(mtd);
	struct nand_device *nand = mtd_to_nanddev(mtd);
	struct nand_io_iter iter;
	bool enable_ecc = false;
	int ret = 0;
#ifdef CONFIG_NAND_HIDE_BAD
	struct nand_pos tmp_pos;
	nanddev_offs_to_pos(nand, to, &tmp_pos);
#endif

	if (ops->mode != MTD_OPS_RAW && mtd->ooblayout)
		enable_ecc = true;

	mutex_lock(&spinand->lock);

	nanddev_io_for_each_page(nand, NAND_PAGE_WRITE, to, ops, &iter) {

#ifdef CONFIG_SPI_NAND_PAGECACHE
    /* Invalidate the page cache reference */
    spinand->pagecache.page = -1;
#endif
#ifdef CONFIG_NAND_HIDE_BAD
		iter.req.pos = tmp_pos;

		ret = xlat_offset_to_skip_factory_bad(nand, &iter.req.pos);
		if(ret == -EINVAL){
			pr_err("%s: offset xlat exceeds chip size\n",__FUNCTION__);
			return ret;
		}
		if(nanddev_pos_cmp(&tmp_pos, &iter.req.pos) !=0 ){
			pr_debug("Write xlat: 0x%llx => 0x%llx\n", nanddev_pos_to_offs(nand, &tmp_pos), nanddev_pos_to_offs(nand, &iter.req.pos));
			//printk("Write xlat: 0x%llx => 0x%llx\n", nanddev_pos_to_offs(nand, &tmp_pos), nanddev_pos_to_offs(nand, &iter.req.pos) );
		}
		nanddev_pos_next_page(nand, &tmp_pos);
#endif
		ret = spinand_select_target(spinand, iter.req.pos.target);
		if (ret)
			break;

		ret = spinand_ecc_enable(spinand, enable_ecc);
		if (ret)
			break;

		ret = spinand_write_page(spinand, &iter.req);
		if (ret)
			break;

		ops->retlen += iter.req.datalen;
		ops->oobretlen += iter.req.ooblen;
	}

	mutex_unlock(&spinand->lock);

	return ret;
}

static bool spinand_isbad(struct nand_device *nand, const struct nand_pos *pos)
{
	struct spinand_device *spinand = nand_to_spinand(nand);
	u8 marker[2] = { };
	struct nand_page_io_req req = {
		.pos = *pos,
		.ooblen = sizeof(marker),
		.ooboffs = 0,
		.oobbuf.in = marker,
		.mode = MTD_OPS_RAW,
	};
#ifdef CONFIG_NAND_HIDE_BAD
	int ret = 0;
	struct nand_pos xlat_pos = *pos;
#endif
#ifdef CONFIG_NAND_HIDE_BAD
	ret = xlat_offset_to_skip_factory_bad(nand, &xlat_pos);
	if(ret == -EINVAL){
		pr_err("%s: offset xlat exceeds chip size\n",__FUNCTION__);
		return true;
	}
	if(nanddev_pos_cmp(&xlat_pos, pos) !=0 ){
		pr_debug("isbad xlat: 0x%llx => 0x%llx\n", nanddev_pos_to_offs(nand, pos), nanddev_pos_to_offs(nand, &xlat_pos));
	}
	req.pos = xlat_pos;
	spinand_select_target(spinand, xlat_pos.target);
#else
	spinand_select_target(spinand, pos->target);
#endif
	spinand_read_page(spinand, &req, false);
	if (marker[0] != 0xff || marker[1] != 0xff)
		return true;

	return false;
}

static int spinand_mtd_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	struct nand_device *nand = mtd_to_nanddev(mtd);
	struct spinand_device *spinand = nand_to_spinand(nand);
	struct nand_pos pos;
	int ret;

	nanddev_offs_to_pos(nand, offs, &pos);
	mutex_lock(&spinand->lock);
	ret = nanddev_isbad(nand, &pos);
	mutex_unlock(&spinand->lock);

	return ret;
}

static int spinand_markbad(struct nand_device *nand, const struct nand_pos *pos)
{
	struct spinand_device *spinand = nand_to_spinand(nand);
	u8 marker[2] = { };
	struct nand_page_io_req req = {
		.pos = *pos,
		.ooboffs = 0,
		.ooblen = sizeof(marker),
		.oobbuf.out = marker,
		.mode = MTD_OPS_RAW,
	};
	int ret;
#ifdef CONFIG_NAND_HIDE_BAD
		struct nand_pos xlat_pos;
		//memcpy(&xlat_pos, pos, sizeof(struct nand_pos));
		xlat_pos = *pos;
		ret = xlat_offset_to_skip_factory_bad(nand, &xlat_pos);
		if(ret == -EINVAL){
			pr_err("%s: offset xlat exceeds chip size\n",__FUNCTION__);
			return true;
		}
		if(nanddev_pos_cmp(pos, &xlat_pos) !=0 ){
			pr_debug("markbad xlat: 0x%llx => 0x%llx\n", nanddev_pos_to_offs(nand, pos), nanddev_pos_to_offs(nand, &xlat_pos));
			req.pos = xlat_pos;
		}

	ret = spinand_select_target(spinand, xlat_pos.target);
	if (ret)
		return ret;

#else
	ret = spinand_select_target(spinand, pos->target);
	if (ret)
		return ret;
#endif
	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	nand_block_state.bad_block_num += 1;
	return spinand_write_page(spinand, &req);
}

static int spinand_mtd_block_markbad(struct mtd_info *mtd, loff_t offs)
{
	struct nand_device *nand = mtd_to_nanddev(mtd);
	struct spinand_device *spinand = nand_to_spinand(nand);
	struct nand_pos pos;
	int ret;

	nanddev_offs_to_pos(nand, offs, &pos);
	mutex_lock(&spinand->lock);
	ret = nanddev_markbad(nand, &pos);
	mutex_unlock(&spinand->lock);

	return ret;
}

static int spinand_erase(struct nand_device *nand, const struct nand_pos *pos)
{
	struct spinand_device *spinand = nand_to_spinand(nand);
	u8 status;
	int ret;

	ret = spinand_select_target(spinand, pos->target);
	if (ret)
		return ret;

	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	ret = spinand_erase_op(spinand, pos);
	if (ret)
		return ret;

	ret = spinand_wait(spinand, &status);
	if (!ret && (status & STATUS_ERASE_FAILED))
		ret = -EIO;

	return ret;
}

static int spinand_mtd_erase(struct mtd_info *mtd,
			     struct erase_info *einfo)
{
	struct spinand_device *spinand = mtd_to_spinand(mtd);
	int ret;

#ifdef CONFIG_SPI_NAND_PAGECACHE
    /* Invalidate the page cache reference */
    spinand->pagecache.page = -1;
#endif

	mutex_lock(&spinand->lock);
	ret = nanddev_mtd_erase(mtd, einfo);
	mutex_unlock(&spinand->lock);

	return ret;
}

static int spinand_mtd_block_isreserved(struct mtd_info *mtd, loff_t offs)
{
	struct spinand_device *spinand = mtd_to_spinand(mtd);
	struct nand_device *nand = mtd_to_nanddev(mtd);
	struct nand_pos pos;
	int ret;

	nanddev_offs_to_pos(nand, offs, &pos);
	mutex_lock(&spinand->lock);
	ret = nanddev_isreserved(nand, &pos);
	mutex_unlock(&spinand->lock);

	return ret;
}

static int spinand_create_dirmap(struct spinand_device *spinand,
				 unsigned int plane)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	struct spi_mem_dirmap_info info = {
		.length = nanddev_page_size(nand) +
			  nanddev_per_page_oobsize(nand),
	};
	struct spi_mem_dirmap_desc *desc;

	/* The plane number is passed in MSB just above the column address */
	info.offset = plane << fls(nand->memorg.pagesize);

	info.op_tmpl = *spinand->op_templates.update_cache;
	desc = devm_spi_mem_dirmap_create(&spinand->spimem->spi->dev,
					  spinand->spimem, &info);
	if (IS_ERR(desc))
		return PTR_ERR(desc);

	spinand->dirmaps[plane].wdesc = desc;

	info.op_tmpl = *spinand->op_templates.read_cache;
	desc = devm_spi_mem_dirmap_create(&spinand->spimem->spi->dev,
					  spinand->spimem, &info);
	if (IS_ERR(desc))
		return PTR_ERR(desc);

	spinand->dirmaps[plane].rdesc = desc;

	return 0;
}

static int spinand_create_dirmaps(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	int i, ret;

	spinand->dirmaps = devm_kzalloc(&spinand->spimem->spi->dev,
					sizeof(*spinand->dirmaps) *
					nand->memorg.planes_per_lun,
					GFP_KERNEL);
	if (!spinand->dirmaps)
		return -ENOMEM;

	for (i = 0; i < nand->memorg.planes_per_lun; i++) {
		ret = spinand_create_dirmap(spinand, i);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct nand_ops spinand_ops = {
	.erase = spinand_erase,
	.markbad = spinand_markbad,
	.isbad = spinand_isbad,
};

static const struct spinand_manufacturer *spinand_manufacturers[] = {
	&gigadevice_spinand_manufacturer,
	&macronix_spinand_manufacturer,
	&micron_spinand_manufacturer,
	&paragon_spinand_manufacturer,
	&toshiba_spinand_manufacturer,
	&winbond_spinand_manufacturer,
	&dosilicon_spinand_manufacturer,
	&etron_spinand_manufacturer,
    &esmt_spinand_manufacturer,
    &esmt2_spinand_manufacturer,
    &fudan_spinand_manufacturer,
    &longsys_spinand_manufacturer,
    &xin_cun_spinand_manufacturer,
    &xtx_spinand_manufacturer,
    &heyangtek_spinand_manufacturer,
	&umtek_spinand_manufacturer,
	&hiksemi_spinand_manufacturer,
};

static int spinand_manufacturer_match(struct spinand_device *spinand,
				      enum spinand_readid_method rdid_method)
{
	u8 *id = spinand->id.data;
	unsigned int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(spinand_manufacturers); i++) {
		const struct spinand_manufacturer *manufacturer =
			spinand_manufacturers[i];

		if (id[0] != manufacturer->id)
			continue;

		ret = spinand_match_and_init(spinand,
					     manufacturer->chips,
					     manufacturer->nchips,
					     rdid_method);
		if (ret < 0)
			continue;

		spinand->manufacturer = manufacturer;
		return 0;
	}
	return -ENOTSUPP;
}

static int spinand_id_detect(struct spinand_device *spinand)
{
	u8 *id = spinand->id.data;
	int ret;

	ret = spinand_read_id_op(spinand, 0, 0, id);
	if (ret)
		return ret;
	ret = spinand_manufacturer_match(spinand, SPINAND_READID_METHOD_OPCODE);
	if (!ret)
		return 0;

	ret = spinand_read_id_op(spinand, 1, 0, id);
	if (ret)
		return ret;
	ret = spinand_manufacturer_match(spinand,
					 SPINAND_READID_METHOD_OPCODE_ADDR);
	if (!ret)
		return 0;

	ret = spinand_read_id_op(spinand, 0, 1, id);
	if (ret)
		return ret;
	ret = spinand_manufacturer_match(spinand,
					 SPINAND_READID_METHOD_OPCODE_DUMMY);

	return ret;
}

static int spinand_manufacturer_init(struct spinand_device *spinand)
{
	if (spinand->manufacturer->ops->init)
		return spinand->manufacturer->ops->init(spinand);

	return 0;
}

static void spinand_manufacturer_cleanup(struct spinand_device *spinand)
{
	/* Release manufacturer private data */
	if (spinand->manufacturer->ops->cleanup)
		return spinand->manufacturer->ops->cleanup(spinand);
}

static const struct spi_mem_op *
spinand_select_op_variant(struct spinand_device *spinand,
			  const struct spinand_op_variants *variants)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int i;

	for (i = 0; i < variants->nops; i++) {
		struct spi_mem_op op = variants->ops[i];
		unsigned int nbytes;
		int ret;

		nbytes = nanddev_per_page_oobsize(nand) +
			 nanddev_page_size(nand);

		while (nbytes) {
			op.data.nbytes = nbytes;
			ret = spi_mem_adjust_op_size(spinand->spimem, &op);
			if (ret)
				break;

			if (!spi_mem_supports_op(spinand->spimem, &op))
				break;

			nbytes -= op.data.nbytes;
		}

		if (!nbytes)
			return &variants->ops[i];
	}

	return NULL;
}

/**
 * spinand_match_and_init() - Try to find a match between a device ID and an
 *			      entry in a spinand_info table
 * @spinand: SPI NAND object
 * @table: SPI NAND device description table
 * @table_size: size of the device description table
 * @rdid_method: read id method to match
 *
 * Match between a device ID retrieved through the READ_ID command and an
 * entry in the SPI NAND description table. If a match is found, the spinand
 * object will be initialized with information provided by the matching
 * spinand_info entry.
 *
 * Return: 0 on success, a negative error code otherwise.
 */
int spinand_match_and_init(struct spinand_device *spinand,
			   const struct spinand_info *table,
			   unsigned int table_size,
			   enum spinand_readid_method rdid_method)
{
	u8 *id = spinand->id.data;
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int i;

	for (i = 0; i < table_size; i++) {
		const struct spinand_info *info = &table[i];
		const struct spi_mem_op *op;

		if (rdid_method != info->devid.method)
			continue;

		if (memcmp(id + 1, info->devid.id, info->devid.len))
			continue;

		nand->memorg = table[i].memorg;
		nanddev_set_ecc_requirements(nand, &table[i].eccreq);
		spinand->eccinfo = table[i].eccinfo;
		spinand->flags = table[i].flags;
		spinand->id.len = 1 + table[i].devid.len;
		spinand->select_target = table[i].select_target;

		op = spinand_select_op_variant(spinand,
					       info->op_variants.read_cache);
		if (!op)
			return -ENOTSUPP;

		spinand->op_templates.read_cache = op;

		op = spinand_select_op_variant(spinand,
					       info->op_variants.write_cache);
		if (!op)
			return -ENOTSUPP;

		spinand->op_templates.write_cache = op;

		op = spinand_select_op_variant(spinand,
					       info->op_variants.update_cache);
		spinand->op_templates.update_cache = op;

		dev_info(&spinand->spimem->spi->dev,
			"model_name: %s\n", info->model);

		spi_nand_info.model = info->model;
		return 0;
	}

	return -ENOTSUPP;
}

static u8 nand_flash_id[SPINAND_MAX_ID_LEN] = {0}; 

static void nand_set_flash_id(struct spinand_device *spinand)
{
	int i = 0;
	u8 devid_len = spinand->manufacturer->chips->devid.len;
	
	nand_flash_id[0] = spinand->manufacturer->id;
	for (i=0; (i<devid_len)&&(i<SPINAND_MAX_ID_LEN-1); i++)
	{
		nand_flash_id[i+1] = spinand->manufacturer->chips->devid.id[i];
	}
}

u8* nand_get_flash_id(void)
{
	return nand_flash_id;
}

static int spinand_detect(struct spinand_device *spinand)
{
	struct device *dev = &spinand->spimem->spi->dev;
	struct nand_device *nand = spinand_to_nand(spinand);
	int ret;

	ret = spinand_reset_op(spinand);
	if (ret)
		return ret;

	ret = spinand_id_detect(spinand);
	if (ret) {
		dev_err(dev, "unknown raw ID %*phN\n", SPINAND_MAX_ID_LEN,
			spinand->id.data);
		return ret;
	}

	if (nand->memorg.ntargets > 1 && !spinand->select_target) {
		dev_err(dev,
			"SPI NANDs with more than one die must implement ->select_target()\n");
		return -EINVAL;
	}

	dev_info(&spinand->spimem->spi->dev,
		 "%s SPI NAND was found. Flash Model: %s, ID: %02X%02X\n", spinand->manufacturer->name, 
		 spinand->manufacturer->chips->model,
		 spinand->manufacturer->id, 
		 spinand->manufacturer->chips->devid.id[0]);

	nand_set_flash_id(spinand);
	
	dev_info(&spinand->spimem->spi->dev,
		 "%llu MiB, block size: %zu KiB, page size: %zu, OOB size: %u\n",
		 nanddev_size(nand) >> 20, nanddev_eraseblock_size(nand) >> 10,
		 nanddev_page_size(nand), nanddev_per_page_oobsize(nand));

	return 0;
}

static int spinand_noecc_ooblayout_ecc(struct mtd_info *mtd, int section,
				       struct mtd_oob_region *region)
{
	return -ERANGE;
}

static int spinand_noecc_ooblayout_free(struct mtd_info *mtd, int section,
					struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	/* Reserve 2 bytes for the BBM. */
	region->offset = 2;
	region->length = 62;

	return 0;
}

static const struct mtd_ooblayout_ops spinand_noecc_ooblayout = {
	.ecc = spinand_noecc_ooblayout_ecc,
	.free = spinand_noecc_ooblayout_free,
};

void nandflash_uid_detect(struct spinand_device *spinand)
{
	u8* nandflash_id;
	u8 uuid[129] = {0};
	struct nand_flash_dev_uid *type = nand_flash_uids;

	nandflash_id = nand_get_flash_id();

	for (; type->name != NULL; type++)
	{
		if (type->id_len && !strncmp(type->id, nandflash_id, type->id_len))
		{
			memset(uuid, 0, sizeof(uuid));
			if (!type->flash_uid_detect(spinand, uuid))
			{
				flash_uuid_init(uuid, sizeof(uuid));
				break;
			}	
		}
	}
}

static int F50L1G41LB_nandflash_uid_detect(struct spinand_device *spinand, u8* uuid)
{
	struct device *dev = &spinand->spimem->spi->dev;	
	u8 val;
	int page_addr = 0x0;
	uint8_t page_buf[SZ_2K+64] = {0}; //pagesize + oobsize
	int step = 0, i = 0, ret=0, uid_error = 0;
	bool enable_ecc = false;
	u8* nandflash_id;
	u8 tmp[64] = {0};
	int j=0;

	nandflash_id = nand_get_flash_id();

	struct nand_pos pos = {
		.target = 0,
		.lun = 0,
		.plane = 0,
		.eraseblock = 0,
		.page = page_addr,		
	};
	
	struct nand_page_io_req req = {
		.pos = pos,
		.datalen = 512,
		.databuf.in = page_buf,
		.mode = MTD_OPS_RAW,

	};

	if (spinand->eccinfo.ooblayout)
		enable_ecc = true;

	//read uid step 1: Set Feature command to enable OTP_EN.
	ret = spinand_upd_cfg(spinand, CFG_OTP_ENABLE, CFG_OTP_ENABLE);
	if (ret)
	{
		dev_err(dev, "Error in setting feature enable OTP_EN. (err = %d)\n", ret);
		return ret;	
	}

	//read uid step 2: Get Feature command and check if the OTP_EN is enable
	ret = spinand_get_cfg(spinand, &val);
	if (ret)
	{
		dev_err(dev, "Error in getting feature OTP_EN. (err = %d)\n", ret);
		return ret;	
	}
	if ( !(val&CFG_OTP_ENABLE) )
	{
		dev_err(dev, "OTP_EN is still disabled, setting error. (err = %d)\n", ret);
		return ret; 
	}

	//read uid step 3: Page read to cache with page address 0x0, read data from page cache
	ret = spinand_read_page(spinand, &req, enable_ecc);
	if (ret)
	{
		dev_err(dev, "read page for uid error. (err = %d)\n", ret);
		return ret;	
	}

	//read uid step 4: Set Feature command to disable OTP_EN
	ret = spinand_upd_cfg(spinand, CFG_OTP_ENABLE, 0);
	if (ret)
	{
		dev_err(dev, "Error in setting feature disable OTP_EN. (err = %d)\n", ret);
		return ret;	
	}

	//read uid step 5: read data to set uuid
#if 0
	printk("nand flash unique id: \n");
	j = 0;
	for (i=0; i<16; i++)
		{
		printk("%d: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", 
		i, 
		page_buf[j],page_buf[j+1],page_buf[j+2],page_buf[j+3],page_buf[j+4],page_buf[j+5],page_buf[j+6],page_buf[j+7],
		page_buf[j+8],page_buf[j+9],page_buf[j+10],page_buf[j+11],page_buf[j+12],page_buf[j+13],page_buf[j+14],page_buf[j+15],
		page_buf[j+16],page_buf[j+17],page_buf[j+18],page_buf[j+19],page_buf[j+20],page_buf[j+21],page_buf[j+22],page_buf[j+23],
		page_buf[j+24],page_buf[j+25],page_buf[j+26],page_buf[j+27],page_buf[j+28],page_buf[j+29],page_buf[j+30],page_buf[j+31]
		);

		j+=32;
	}
#endif

	step = 0;
	uid_error = 1;
	for (i=0; i<16; i++)
	{
		for (j=0; j<16; j++) 
		{
			//If the XOR of the UID and its bit-wise complement is all ones, then the UID is valid
			if ((page_buf[step+j] ^ page_buf[step+j+16]) != 0xFF)
			{
				uid_error = 1;
				break;
			}
			uid_error = 0;
		}

		if (uid_error == 0)
			break;

		step += 32;
	}

	if (uid_error==1)
	{
		dev_err(dev, "ESMT Flash Device read UID error\n");
		return -1;
	}

	sprintf(uuid, "%02X%02X-", nandflash_id[0], nandflash_id[1]);

	for (i=step; i<step+32; i++)
	{
		sprintf(tmp, "%02X", page_buf[i]);
		strcat(uuid, tmp);
	}

	#if 0
	sprintf(tmp, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			page_buf[step+10],page_buf[step+11],page_buf[step+12],page_buf[step+13],page_buf[step+14],page_buf[step+15],page_buf[step+16],
			page_buf[step+26],page_buf[step+27],page_buf[step+28],page_buf[step+29],page_buf[step+30],page_buf[step+31]);
	strcat(uuid, tmp);
	#endif
	
	return 0;
}

static int F50L1G41LC_nandflash_uid_detect(struct spinand_device *spinand, u8* uuid)
{
	struct device *dev = &spinand->spimem->spi->dev;	
	u8 val;
	int page_addr = 0x0;
	uint8_t page_buf[SZ_2K+64] = {0}; //pagesize + oobsize
	int step = 0, i = 0, ret=0, uid_error = 0;
	bool enable_ecc = false;
	u8* nandflash_id;
	u8 tmp[64] = {0};
	int j=0;

	nandflash_id = nand_get_flash_id();

	struct nand_pos pos = {
		.target = 0,
		.lun = 0,
		.plane = 0,
		.eraseblock = 0,
		.page = page_addr,		
	};
	
	struct nand_page_io_req req = {
		.pos = pos,
		.datalen = 512,
		.databuf.in = page_buf,
		.mode = MTD_OPS_RAW,

	};

	if (spinand->eccinfo.ooblayout)
		enable_ecc = true;

	//read uid step 1: Set Feature command to enable OTP_EN.
	ret = spinand_upd_cfg(spinand, CFG_OTP_ENABLE, CFG_OTP_ENABLE);
	if (ret)
	{
		dev_err(dev, "Error in setting feature enable OTP_EN. (err = %d)\n", ret);
		return ret;	
	}

	//read uid step 2: Get Feature command and check if the OTP_EN is enable
	ret = spinand_get_cfg(spinand, &val);
	if (ret)
	{
		dev_err(dev, "Error in getting feature OTP_EN. (err = %d)\n", ret);
		return ret;	
	}
	if ( !(val&CFG_OTP_ENABLE) )
	{
		dev_err(dev, "OTP_EN is still disabled, setting error. (err = %d)\n", ret);
		return ret; 
	}

	//read uid step 3: Page read to cache with page address 0x0, read data from page cache
	ret = spinand_read_page(spinand, &req, enable_ecc);
	if (ret)
	{
		dev_err(dev, "read page for uid error. (err = %d)\n", ret);
		return ret;	
	}

	//read uid step 4: Set Feature command to disable OTP_EN
	ret = spinand_upd_cfg(spinand, CFG_OTP_ENABLE, 0);
	if (ret)
	{
		dev_err(dev, "Error in setting feature disable OTP_EN. (err = %d)\n", ret);
		return ret;	
	}

	//read uid step 5: read data to set uuid
#if 0
	printk("nand flash unique id: \n");
	j = 0;
	for (i=0; i<16; i++)
		{
		printk("%d: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", 
		i, 
		page_buf[j],page_buf[j+1],page_buf[j+2],page_buf[j+3],page_buf[j+4],page_buf[j+5],page_buf[j+6],page_buf[j+7],
		page_buf[j+8],page_buf[j+9],page_buf[j+10],page_buf[j+11],page_buf[j+12],page_buf[j+13],page_buf[j+14],page_buf[j+15],
		page_buf[j+16],page_buf[j+17],page_buf[j+18],page_buf[j+19],page_buf[j+20],page_buf[j+21],page_buf[j+22],page_buf[j+23],
		page_buf[j+24],page_buf[j+25],page_buf[j+26],page_buf[j+27],page_buf[j+28],page_buf[j+29],page_buf[j+30],page_buf[j+31]
		);

		j+=32;
	}
#endif

	step = 0;
	uid_error = 1;
	for (i=0; i<16; i++)
	{
		for (j=0; j<16; j++) 
		{
			//If the XOR of the UID and its bit-wise complement is all ones, then the UID is valid
			if ((page_buf[step+j] ^ page_buf[step+j+16]) != 0xFF)
			{
				uid_error = 1;
				break;
			}
			uid_error = 0;
		}

		if (uid_error == 0)
			break;

		step += 32;
	}

	if (uid_error==1)
	{
		dev_err(dev, "ESMT Flash Device read UID error\n");
		return -1;
	}

	sprintf(uuid, "%02X%02X-", nandflash_id[0], nandflash_id[1]);

	for (i=step; i<step+32; i++)
	{
		sprintf(tmp, "%02X", page_buf[i]);
		strcat(uuid, tmp);
	}

	#if 0
	sprintf(tmp, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			page_buf[step+10],page_buf[step+11],page_buf[step+12],page_buf[step+13],page_buf[step+14],page_buf[step+15],page_buf[step+16],
			page_buf[step+26],page_buf[step+27],page_buf[step+28],page_buf[step+29],page_buf[step+30],page_buf[step+31]);
	strcat(uuid, tmp);
	#endif
	
	return 0;
}

static int F50L2G41XA_nandflash_uid_detect(struct spinand_device *spinand, u8* uuid)
{
	struct device *dev = &spinand->spimem->spi->dev;	
	u8 val;
	int page_addr = 0x0;
	uint8_t page_buf[SZ_2K+128] = {0}; //pagesize + oobsize
	int step = 0, i = 0, ret=0, uid_error = 0;
	bool enable_ecc = false;
	u8* nandflash_id;
	u8 tmp[64] = {0};
	int j;

	nandflash_id = nand_get_flash_id();

	struct nand_pos pos = {
		.target = 0,
		.lun = 0,
		.plane = 0,
		.eraseblock = 0,
		.page = page_addr,		
	};
	
	struct nand_page_io_req req = {
		.pos = pos,
		.datalen = 512,
		.databuf.in = page_buf,
		.mode = MTD_OPS_RAW,

	};

	if (spinand->eccinfo.ooblayout)
		enable_ecc = true;

	//read uid step 1: Set Feature command to enable OTP_EN.
	ret = spinand_upd_cfg(spinand, CFG_OTP_ENABLE, CFG_OTP_ENABLE);
	if (ret)
	{
		dev_err(dev, "Error in setting feature enable OTP_EN. (err = %d)\n", ret);
		return ret;	
	}

	//read uid step 2: Get Feature command and check if the OTP_EN is enable
	ret = spinand_get_cfg(spinand, &val);
	if (ret)
	{
		dev_err(dev, "Error in getting feature OTP_EN. (err = %d)\n", ret);
		return ret;	
	}
	if ( !(val&CFG_OTP_ENABLE) )
	{
		dev_err(dev, "OTP_EN is still disabled, setting error. (err = %d)\n", ret);
		return ret; 
	}

	//read uid step 3: Page read to cache with page address 0x0, read data from page cache
	ret = spinand_read_page(spinand, &req, enable_ecc);
	if (ret)
	{
		dev_err(dev, "read page for uid error. (err = %d)\n", ret);
		return ret;	
	}

	//read uid step 4: Set Feature command to disable OTP_EN
	ret = spinand_upd_cfg(spinand, CFG_OTP_ENABLE, 0);
	if (ret)
	{
		dev_err(dev, "Error in setting feature disable OTP_EN. (err = %d)\n", ret);
		return ret;	
	}

#if 0
	for (i=0; i<64; i++)
		printk("%02X\n", page_buf[i]);
#endif 

	//read uid step 5: read data to set /proc/sys/dev/flash_uuid
	step = 0;
	uid_error = 1;
	for (i=0; i<16; i++)
	{
		for (j=0; j<16; j++) 
	{
		//If the XOR of the UID and its bit-wise complement is all ones, then the UID is valid
			if ((page_buf[step+j] ^ page_buf[step+j+16]) != 0xFF)
		{
			uid_error = 1;
			break;
		}
			uid_error = 0;
	}

		if (uid_error == 0)
				break;

		step += 32;
	}

	if (uid_error==1)
	{
		dev_err(dev, "ESMT Flash Device read UID error\n");
		return -1;
	}

	sprintf(uuid, "%02X%02X-", nandflash_id[0], nandflash_id[1]);
	for (i=step; i <step+16; i++)
	{
		sprintf(tmp, "%02X", page_buf[i]);
		strcat(uuid, tmp);
	}

	return 0;
}

static int spinand_init(struct spinand_device *spinand)
{
	struct device *dev = &spinand->spimem->spi->dev;
	struct mtd_info *mtd = spinand_to_mtd(spinand);
	struct nand_device *nand = mtd_to_nanddev(mtd);
	int ret, i;

#ifdef CONFIG_NAND_HIDE_BAD
		char op_mode[4]="?/?";
#endif

#ifdef CONFIG_SPI_NAND_PAGECACHE
	/* Invalidate the page cache reference */
	spinand->pagecache.page = -1;
#endif

	/*
	 * We need a scratch buffer because the spi_mem interface requires that
	 * buf passed in spi_mem_op->data.buf be DMA-able.
	 */
	spinand->scratchbuf = kzalloc(SPINAND_MAX_ID_LEN, GFP_KERNEL);
	if (!spinand->scratchbuf)
		return -ENOMEM;

	ret = spinand_detect(spinand);
	if (ret)
		goto err_free_bufs;

	/*
	 * Use kzalloc() instead of devm_kzalloc() here, because some drivers
	 * may use this buffer for DMA access.
	 * Memory allocated by devm_ does not guarantee DMA-safe alignment.
	 */
	spinand->databuf = kzalloc(nanddev_page_size(nand) +
			       nanddev_per_page_oobsize(nand),
			       GFP_KERNEL);
	if (!spinand->databuf) {
		ret = -ENOMEM;
		goto err_free_bufs;
	}

	spinand->oobbuf = spinand->databuf + nanddev_page_size(nand);

	ret = spinand_init_cfg_cache(spinand);
	if (ret)
		goto err_free_bufs;

	ret = spinand_init_quad_enable(spinand);
	if (ret)
		goto err_free_bufs;

	ret = spinand_upd_cfg(spinand, CFG_OTP_ENABLE, 0);
	if (ret)
		goto err_free_bufs;

	ret = spinand_manufacturer_init(spinand);
	if (ret) {
		dev_err(dev,
			"Failed to initialize the SPI NAND chip (err = %d)\n",
			ret);
		goto err_free_bufs;
	}

	ret = spinand_create_dirmaps(spinand);
	if (ret) {
		dev_err(dev,
			"Failed to create direct mappings for read/write operations (err = %d)\n",
			ret);
		goto err_manuf_cleanup;
	}

	/* After power up, all blocks are locked, so unlock them here. */
	for (i = 0; i < nand->memorg.ntargets; i++) {
		ret = spinand_select_target(spinand, i);
		if (ret)
			goto err_manuf_cleanup;

		ret = spinand_lock_block(spinand, BL_ALL_UNLOCKED);
		if (ret)
			goto err_manuf_cleanup;
	}

	ret = nanddev_init(nand, &spinand_ops, THIS_MODULE);
	if (ret)
		goto err_manuf_cleanup;

	/*
	 * Right now, we don't support ECC, so let the whole oob
	 * area is available for user.
	 */
#ifdef CONFIG_ARCH_CORTINA
	mtd->name = "ca_spinand_flash";
#endif
	mtd->_read_oob = spinand_mtd_read;
	mtd->_write_oob = spinand_mtd_write;
	mtd->_block_isbad = spinand_mtd_block_isbad;
	mtd->_block_markbad = spinand_mtd_block_markbad;
	mtd->_block_isreserved = spinand_mtd_block_isreserved;
	mtd->_erase = spinand_mtd_erase;
	mtd->_max_bad_blocks = nanddev_mtd_max_bad_blocks;

	if (spinand->eccinfo.ooblayout)
		mtd_set_ooblayout(mtd, spinand->eccinfo.ooblayout);
	else
		mtd_set_ooblayout(mtd, &spinand_noecc_ooblayout);

	ret = mtd_ooblayout_count_freebytes(mtd);
	if (ret < 0)
		goto err_cleanup_nanddev;

	mtd->oobavail = ret;

	/* Propagate ECC information to mtd_info */
	mtd->ecc_strength = nanddev_get_ecc_requirements(nand)->strength;
	mtd->ecc_step_size = nanddev_get_ecc_requirements(nand)->step_size;

	scan_all_bad_blocks(mtd, 0, mtd->size);

#ifdef CONFIG_NAND_HIDE_BAD

		get_spi_nand_factory_bad_blocks(mtd, 0, mtd->size);

		/* Update the the size information to the availabe capacity */
		mtd->size -= bad_block_cnt*mtd->erasesize;
		printk("spi nand total capacity is %lluMB(0x%llx)\n",(uint64_t)(nanddev_size(nand)/0x100000),   nanddev_size(nand));
		printk("spi nand available size is %lluMB(0x%llx)\n",(uint64_t)(mtd->size/0x100000), mtd->size);
		if(bbl_source == FACTORY_1ST_SCAN)
			printk("spi nand 1st scan factory bad to memory\n");
		else
			printk("spi nand %s from flash\n",(bbl_source==FACTORY_FROM_BBT0)?"Bbl0":"Bbl1");

		printk("spi nand hidden: ");
		for(i=0 ; i<bad_block_cnt ; i++){
			printk("0x%x ", bad_block_list[i]);
		}
		printk("\n");
		switch(spinand->op_templates.read_cache->cmd.opcode){
			case SPINAND_CMD_READ_FROM_CACHE_QUAD_IO:
				op_mode[0] = 'Q';
				break;
			case SPINAND_CMD_READ_FROM_CACHE_X4:
				op_mode[0] = '4';
				break;
			case SPINAND_CMD_READ_FROM_CACHE_DUAL_IO:
				op_mode[0] = 'D';
				break;
			case SPINAND_CMD_READ_FROM_CACHE_X2:
				op_mode[0] = '2';
				break;
			case SPINAND_CMD_READ_FROM_CACHE:
			case SPINAND_CMD_READ_FROM_CACHE_FAST:
				op_mode[0] = 'S';
				break;
		}
		switch(spinand->op_templates.write_cache->cmd.opcode){
			case SPINAND_CMD_PROG_LOAD_X4:
				op_mode[2] = '4';
				break;
			case SPINAND_CMD_PROG_LOAD:
				op_mode[2] = 'S';
				break;
		}
		printk("spi nand Mode: %s\n", op_mode);
#endif
		spi_nand_info.nand_size = nanddev_size(nand);
		spi_nand_info.chip_size = nanddev_size(nand);
		spi_nand_info.block_size = nanddev_eraseblock_size(nand);
		spi_nand_info.page_size = nanddev_page_size(nand);
		spi_nand_info.oob_size = nanddev_per_page_oobsize(nand);
		spi_nand_info.oob_avail = mtd->oobavail;
		spi_nand_info.PagePerBlock = (uint32_t)(nanddev_eraseblock_size(nand)/nanddev_page_size(nand));
	return 0;

err_cleanup_nanddev:
	nanddev_cleanup(nand);

err_manuf_cleanup:
	spinand_manufacturer_cleanup(spinand);

err_free_bufs:
	kfree(spinand->databuf);
	kfree(spinand->scratchbuf);
	return ret;
}

static void spinand_cleanup(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);

	nanddev_cleanup(nand);
	spinand_manufacturer_cleanup(spinand);
	kfree(spinand->databuf);
	kfree(spinand->scratchbuf);
}

static int spinand_probe(struct spi_mem *mem)
{
	struct spinand_device *spinand;
	struct mtd_info *mtd;
	int ret;

	spinand = devm_kzalloc(&mem->spi->dev, sizeof(*spinand),
			       GFP_KERNEL);
	if (!spinand)
		return -ENOMEM;

	spinand->spimem = mem;
	spi_mem_set_drvdata(mem, spinand);
	spinand_set_of_node(spinand, mem->spi->dev.of_node);
	mutex_init(&spinand->lock);
	mtd = spinand_to_mtd(spinand);
	mtd->dev.parent = &mem->spi->dev;

	ret = spinand_init(spinand);
	if (ret)
		return ret;

	ret = mtd_device_register(mtd, NULL, 0);
	if (ret)
		goto err_spinand_cleanup;

	nandflash_uid_detect(spinand);

	return 0;

err_spinand_cleanup:
	spinand_cleanup(spinand);

	return ret;
}

static int spinand_remove(struct spi_mem *mem)
{
	struct spinand_device *spinand;
	struct mtd_info *mtd;
	int ret;

	spinand = spi_mem_get_drvdata(mem);
	mtd = spinand_to_mtd(spinand);

	ret = mtd_device_unregister(mtd);
	if (ret)
		return ret;

	spinand_cleanup(spinand);

	return 0;
}

static const struct spi_device_id spinand_ids[] = {
	{ .name = "spi-nand" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(spi, spinand_ids);

#ifdef CONFIG_OF
static const struct of_device_id spinand_of_ids[] = {
	{ .compatible = "spi-nand" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, spinand_of_ids);
#endif

static struct spi_mem_driver spinand_drv = {
	.spidrv = {
		.id_table = spinand_ids,
		.driver = {
			.name = "spi-nand",
			.of_match_table = of_match_ptr(spinand_of_ids),
		},
	},
	.probe = spinand_probe,
	.remove = spinand_remove,
};
module_spi_mem_driver(spinand_drv);

MODULE_DESCRIPTION("SPI NAND framework");
MODULE_AUTHOR("Peter Pan<peterpandong@micron.com>");
MODULE_LICENSE("GPL v2");
