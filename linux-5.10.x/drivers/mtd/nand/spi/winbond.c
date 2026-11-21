// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017 exceet electronics GmbH
 *
 * Authors:
 *	Frieder Schrempf <frieder.schrempf@exceet.de>
 *	Boris Brezillon <boris.brezillon@bootlin.com>
 *
 *
 * Copyright (c) 2022, Realtek Semiconductor Corp.
 * [RTK] 2022/08/30: Support W25N02KVZEIE-H, W25N04KVZEIE-H
*/

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/mtd/spinand.h>

#define SPINAND_MFR_WINBOND		0xEF

#define WINBOND_CFG_BUF_READ		BIT(3)

#define W25N0XKV_MBF_TABLE_ADDR 0x30

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

static int w25m02gv_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 8;
	region->length = 8;

	return 0;
}

static int w25m02gv_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 2;
	region->length = 6;

	return 0;
}

static const struct mtd_ooblayout_ops w25m02gv_ooblayout = {
	.ecc = w25m02gv_ooblayout_ecc,
	.free = w25m02gv_ooblayout_free,
};


static int w25n0xkv_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = 64 + (16 * section);
	region->length = 16;

	return 0;
}

static int w25n0xkv_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 2;
	region->length = 14;

	return 0;
}

static const struct mtd_ooblayout_ops w25n0xkv_ooblayout = {
	.ecc = w25n0xkv_ooblayout_ecc,
	.free = w25n0xkv_ooblayout_free,
};

static int w25n0xkv_ecc_get_status(struct spinand_device *spinand,
					u8 status)
{
	u8 mbf;
	struct spi_mem_op op = SPINAND_GET_FEATURE_OP(W25N0XKV_MBF_TABLE_ADDR,
						      &mbf);
	int ret;

	switch (status & STATUS_ECC_MASK) {
	case STATUS_ECC_NO_BITFLIPS:
		return 0;

	case STATUS_ECC_HAS_BITFLIPS:
		/*
		 * Read status2 register to determine a more fine grained
		 * bit error status
		 */
		ret = spi_mem_exec_op(spinand->spimem, &op);
		if (ret)
			return ret;

		/*
		 * 1 ... 8 bits are flipped (and corrected)
		 */
		/* bits sorted this way (1...0): MBF3, MBF2, MBF1, MBF0 */
		return (mbf >> 4)&0xF;

	case STATUS_ECC_UNCOR_ERROR:
		return -EBADMSG;

	default:
		break;
	}

	return -EINVAL;
}

static int w25m02gv_select_target(struct spinand_device *spinand,
				  unsigned int target)
{
	struct spi_mem_op op = SPI_MEM_OP(SPI_MEM_OP_CMD(0xc2, 1),
					  SPI_MEM_OP_NO_ADDR,
					  SPI_MEM_OP_NO_DUMMY,
					  SPI_MEM_OP_DATA_OUT(1,
							spinand->scratchbuf,
							1));

	*spinand->scratchbuf = target;
	return spi_mem_exec_op(spinand->spimem, &op);
}


//u16 tmp_id=0xab21;
const u8 tmp_id[2]={0xab,0x21};

static const struct spinand_info winbond_spinand_table[] = {
	SPINAND_INFO("W25M02GVZEIG",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xab, 0x21),
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 20, 1, 1, 2),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL),
		     SPINAND_SELECT_TARGET(w25m02gv_select_target)),
	SPINAND_INFO("W25N01GVZEIG",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xaa, 0x21),
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 20, 1, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL)),
	SPINAND_INFO("W25N01KVZEIR",
         SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xae, 0x21),  
         NAND_MEMORG(1, 2048, 64, 64, 1024, 20, 1, 1, 1),
         NAND_ECCREQ(1, 512),
         SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
                                  &write_cache_variants,
                                  &update_cache_variants), 
         0,
         SPINAND_ECCINFO(&w25n0xkv_ooblayout, w25n0xkv_ecc_get_status)),
    SPINAND_INFO("W25N02KVZEIE",
             SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xaa, 0x22),
             NAND_MEMORG(1, 2048, 128, 64, 2048, 40, 1, 1, 1),
             NAND_ECCREQ(8, 512),
             SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
                          &write_cache_variants,
                          &update_cache_variants),
             0,
             SPINAND_ECCINFO(&w25n0xkv_ooblayout, w25n0xkv_ecc_get_status)),
    SPINAND_INFO("W25N04KVZEIE",
             SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xaa, 0x23),
             NAND_MEMORG(1, 2048, 128, 64, 4096, 80, 1, 1, 1),
             NAND_ECCREQ(8, 512),
             SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
                          &write_cache_variants,
                          &update_cache_variants),
             SPINAND_HAS_QE_BIT,
             SPINAND_ECCINFO(&w25n0xkv_ooblayout, w25n0xkv_ecc_get_status)),
};

static int winbond_spinand_init(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int i;

	/*
	 * Make sure all dies are in buffer read mode and not continuous read
	 * mode.
	 */
	for (i = 0; i < nand->memorg.ntargets; i++) {
		spinand_select_target(spinand, i);
		spinand_upd_cfg(spinand, WINBOND_CFG_BUF_READ,
				WINBOND_CFG_BUF_READ);
	}

	return 0;
}

static const struct spinand_manufacturer_ops winbond_spinand_manuf_ops = {
	.init = winbond_spinand_init,
};

const struct spinand_manufacturer winbond_spinand_manufacturer = {
	.id = SPINAND_MFR_WINBOND,
	.name = "Winbond",
	.chips = winbond_spinand_table,
	.nchips = ARRAY_SIZE(winbond_spinand_table),
	.ops = &winbond_spinand_manuf_ops,
};
