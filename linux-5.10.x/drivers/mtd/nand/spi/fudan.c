// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022, Realtek Semiconductor Corp.
 * [RTK] 2022/06/08: Support FM25S02A.
 */


#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mtd/spinand.h>

#define SPINAND_MFR_FUDAN		0xA1

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

static int fm25s02a_ooblayout_ecc(struct mtd_info *mtd, int section, struct mtd_oob_region *region)
{
    return -ERANGE;
}

static int fm25s02a_ooblayout_free(struct mtd_info *mtd, int section, struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	region->offset = 2;
	region->length = mtd->oobsize - 2;

	return 0;
}

static const struct mtd_ooblayout_ops fm25s02a_ooblayout = {
	.ecc = fm25s02a_ooblayout_ecc,
	.free = fm25s02a_ooblayout_free,
};

static int fm25s02a_ecc_get_status(struct spinand_device *spinand, u8 status)
{
    u8 eccs = (status & STATUS_ECC_MASK) >> 4;

    if (eccs == 0) return 0;
    else if (eccs == 1) return 1;
    else return -EBADMSG;
}

static int fm25s0xbi3_ooblayout_ecc(struct mtd_info *mtd, int section,
					struct mtd_oob_region *region)
{
	if (section > 0)
		return -ERANGE;

	region->offset = mtd->oobsize / 2;
	region->length = mtd->oobsize / 2;

	return 0;
}

static int fm25s0xbi3_ooblayout_free(struct mtd_info *mtd, int section,
					 struct mtd_oob_region *region)
{
	if (section > 0)
		return -ERANGE;

	/* 2 bytes reserved for BBM */
	region->offset = 2;
	region->length = (mtd->oobsize / 2) - 2;

	return 0;
}

static const struct mtd_ooblayout_ops fm25s0xbi3_ooblayout = {
	.ecc = fm25s0xbi3_ooblayout_ecc,
	.free = fm25s0xbi3_ooblayout_free,
};

static int fm25s0xbi3_ecc_get_status(struct spinand_device *spinand,
					 u8 status)
{
	switch ((status & 0x70) >> 4) {
	case 0:
		return 0;

	case 2:
		return -EBADMSG;

	case 1:
		return 3;

	case 3:
		return 6;

	case 5:
		return 8;

	default:
		break;
	}

	return -EINVAL;
}

static const struct spinand_info fudan_spinand_table[] = {
	SPINAND_INFO("FM25S01A",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xE4),
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 40, 2, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&fm25s02a_ooblayout, fm25s02a_ecc_get_status),),

	SPINAND_INFO("FM25S02A",
				SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xE5),
				NAND_MEMORG(1, 2048, 64, 64, 2048, 40, 2, 1, 1),
				NAND_ECCREQ(1, 512),
				SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
							&write_cache_variants,
							&update_cache_variants),
				SPINAND_HAS_QE_BIT,
				SPINAND_ECCINFO(&fm25s02a_ooblayout, fm25s02a_ecc_get_status)),

	SPINAND_INFO("FM25S01BI3",
			SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xD4),
			NAND_MEMORG(1, 2048, 128, 64, 1024, 20, 1, 1, 1),
			NAND_ECCREQ(8, 512),
			SPINAND_INFO_OP_VARIANTS(&read_cache_variants, &write_cache_variants, &update_cache_variants),
			SPINAND_HAS_QE_BIT,
			SPINAND_ECCINFO(&fm25s0xbi3_ooblayout, fm25s0xbi3_ecc_get_status)),
};

static const struct spinand_manufacturer_ops fudan_spinand_manuf_ops = {
};

const struct spinand_manufacturer fudan_spinand_manufacturer = {
	.id = SPINAND_MFR_FUDAN,
	.name = "Fudan",
	.chips = fudan_spinand_table,
	.nchips = ARRAY_SIZE(fudan_spinand_table),
	.ops = &fudan_spinand_manuf_ops,
};
