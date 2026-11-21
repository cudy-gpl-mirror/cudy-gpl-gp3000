// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022, Realtek Semiconductor Corp.
 * [RTK] 2024/06/24: Support XCSP2AAPK-IT.
 */


#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mtd/spinand.h>

#define SPINAND_MFR_XIN_CUN		0x8C

static SPINAND_OP_VARIANTS(read_cache_variants,
		SPINAND_PAGE_READ_FROM_CACHE_QUADIO_OP(0, 2, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X4_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_DUALIO_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X2_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(true, 0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(false, 0, 1, NULL, 0));

static SPINAND_OP_VARIANTS(write_cache_variants,
		SPINAND_PROG_LOAD_X4(true, 0, NULL, 0),
		SPINAND_PROG_LOAD(true, 0, NULL, 0));

static SPINAND_OP_VARIANTS(update_cache_variants,
		SPINAND_PROG_LOAD_X4(false, 0, NULL, 0),
		SPINAND_PROG_LOAD(false, 0, NULL, 0));

static int xcsp2aapkit_ecc_get_status(struct spinand_device *spinand, u8 status)
{
    u8 eccs = (status & STATUS_ECC_MASK) >> 4;

    if (eccs == 0)
		return 0;
    else if (eccs == 1)
		return 4;
    else if (eccs == 3)
		return 8;
    else
		return -EBADMSG;
}

static int xcsp2aapkit_ooblayout_ecc(struct mtd_info *mtd, int section, struct mtd_oob_region *region)
{
    return -ERANGE;
}

static int xcsp2aapkit_ooblayout_free(struct mtd_info *mtd, int section, struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	region->offset = 2;
	region->length = (mtd->oobsize / 2) - 2;

	return 0;
}

static const struct mtd_ooblayout_ops xcsp2aapkit_ooblayout = {
	.ecc = xcsp2aapkit_ooblayout_ecc,
	.free = xcsp2aapkit_ooblayout_free,
};

static const struct spinand_info xin_cun_spinand_table[] = {
	SPINAND_INFO("XCSP2AAPK-IT",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_ADDR, 0xA1),
			NAND_MEMORG(1, 2048, 128, 64, 2048, 40, 1, 1, 1),
			NAND_ECCREQ(8, 512),
			SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
									&write_cache_variants,
									&update_cache_variants),
									0,
									SPINAND_ECCINFO(&xcsp2aapkit_ooblayout, xcsp2aapkit_ecc_get_status)),
};

static const struct spinand_manufacturer_ops xin_cun_spinand_manuf_ops = {
};

const struct spinand_manufacturer xin_cun_spinand_manufacturer = {
	.id = SPINAND_MFR_XIN_CUN,
	.name = "Xincun",
	.chips = xin_cun_spinand_table,
	.nchips = ARRAY_SIZE(xin_cun_spinand_table),
	.ops = &xin_cun_spinand_manuf_ops,
};
