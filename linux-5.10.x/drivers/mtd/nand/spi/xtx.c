// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022, Realtek Semiconductor Corp.
 * [RTK] 2022/04/28: Support XT26G02C
 * [RTK] 2022/08/31: Support XT26G01C, XT26G04C
*/
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mtd/spinand.h>

#define SPINAND_MFR_XTX		0x0B

static SPINAND_OP_VARIANTS(read_cache_variants,
		SPINAND_PAGE_READ_FROM_CACHE_X4_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X2_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(true, 0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(false, 0, 1, NULL, 0));

static SPINAND_OP_VARIANTS(write_cache_x4_variants,
		SPINAND_PROG_LOAD_X4(true, 0, NULL, 0),
		SPINAND_PROG_LOAD(true, 0, NULL, 0));

static SPINAND_OP_VARIANTS(update_cache_x4_variants,
		SPINAND_PROG_LOAD_X4(false, 0, NULL, 0),
		SPINAND_PROG_LOAD(false, 0, NULL, 0));

/**
 * Backward compatibility for 1st generation Serial NAND devices
 * which don't support Quad Program Load operation.
 */
static SPINAND_OP_VARIANTS(write_cache_variants,
		SPINAND_PROG_LOAD(true, 0, NULL, 0));

static SPINAND_OP_VARIANTS(update_cache_variants,
		SPINAND_PROG_LOAD(false, 0, NULL, 0));

static int xt26g0xc_ooblayout_ecc(struct mtd_info *mtd, int section,
					struct mtd_oob_region *region)
{
	if (section > 0)
		return -ERANGE;

	region->offset = mtd->oobsize / 2;
	region->length = mtd->oobsize / 2;

	return 0;
}

static int xt26g0xc_ooblayout_free(struct mtd_info *mtd, int section,
					 struct mtd_oob_region *region)
{
	if (section > 0)
		return -ERANGE;

	/* 2 bytes reserved for BBM */
	region->offset = 2;
	region->length = (mtd->oobsize / 2) - 2;

	return 0;
}

static const struct mtd_ooblayout_ops xt26g0xc_ooblayout = {
	.ecc = xt26g0xc_ooblayout_ecc,
	.free = xt26g0xc_ooblayout_free,
};

static int xt26g0xc_ecc_get_status(struct spinand_device *spinand,
					 u8 status)
{
    uint32_t ecc_status = (status & 0xF0) >> 4;
    if (ecc_status == 0xF) {
        return -EBADMSG;
    } else {
        return ecc_status;
    }
}

static int xt26gxxd_ecc_get_status(struct spinand_device *spinand,
					 u8 status)
{
	uint32_t ecc_status = (status & 0xF0) >> 4;
	uint32_t eccs10 = (ecc_status & 0x3);
	uint32_t eccs32 = (ecc_status >> 2) & 0x3;

	if (eccs10 == 2) {
		return -EBADMSG;
	} else if (eccs10 == 3) {
		return 8;
	} else if (eccs10 == 0) {
		return 0;
	} else {
		return  eccs32 + 4;
	}
}

static const struct spinand_info xtx_spinand_table[] = {
	SPINAND_INFO("XT26G01C",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0x11),
		     NAND_MEMORG(1, 2048, 128, 64, 1024, 20, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
                                      &write_cache_variants,
                                      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
 		     SPINAND_ECCINFO(&xt26g0xc_ooblayout, xt26g0xc_ecc_get_status)),

	SPINAND_INFO("XT26G02C",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0x12),
		     NAND_MEMORG(1, 2048, 128, 64, 2048, 40, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
                                      &write_cache_variants,
                                      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
 		     SPINAND_ECCINFO(&xt26g0xc_ooblayout, xt26g0xc_ecc_get_status)),

	SPINAND_INFO("XT26G04C",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0x13),
		     NAND_MEMORG(1, 4096, 256, 64, 2048, 40, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
                                      &write_cache_variants,
                                      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
 		     SPINAND_ECCINFO(&xt26g0xc_ooblayout, xt26g0xc_ecc_get_status)),

	SPINAND_INFO("XT26G01DWSIGA",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0x31),
		     NAND_MEMORG(1, 2048, 128, 64, 1024, 20, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
                                      &write_cache_variants,
                                      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
 		     SPINAND_ECCINFO(&xt26g0xc_ooblayout, xt26gxxd_ecc_get_status)),

	SPINAND_INFO("XT26G02DWSIGA",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0x32),
		     NAND_MEMORG(1, 2048, 128, 64, 2048, 40, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
                                      &write_cache_variants,
                                      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
 		     SPINAND_ECCINFO(&xt26g0xc_ooblayout, xt26gxxd_ecc_get_status)),

	SPINAND_INFO("XT26G04DWSIGA",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0x33),
		     NAND_MEMORG(1, 4096, 256, 64, 2048, 40, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
                                      &write_cache_variants,
                                      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
 		     SPINAND_ECCINFO(&xt26g0xc_ooblayout, xt26gxxd_ecc_get_status)),

    SPINAND_INFO("XT26G11DWSIGA",
            SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0x34),
            NAND_MEMORG(1, 2048, 128, 64, 1024, 20, 1, 1, 1),
            NAND_ECCREQ(8, 512),
            SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
                                    &write_cache_variants,
                                    &update_cache_variants),
            SPINAND_HAS_QE_BIT,
            SPINAND_ECCINFO(&xt26g0xc_ooblayout, xt26gxxd_ecc_get_status)),

	SPINAND_INFO("XT26G12DWSIGA",
			SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0x35),
			NAND_MEMORG(1, 2048, 128, 64, 2048, 40, 1, 1, 1),
			NAND_ECCREQ(8, 512),
			SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
									&write_cache_variants,
									&update_cache_variants),
			SPINAND_HAS_QE_BIT,
			SPINAND_ECCINFO(&xt26g0xc_ooblayout, xt26gxxd_ecc_get_status)),
};

static const struct spinand_manufacturer_ops xtx_spinand_manuf_ops = {
};

const struct spinand_manufacturer xtx_spinand_manufacturer = {
	.id = SPINAND_MFR_XTX,
	.name = "XTX",
	.chips = xtx_spinand_table,
	.nchips = ARRAY_SIZE(xtx_spinand_table),
	.ops = &xtx_spinand_manuf_ops,
};
