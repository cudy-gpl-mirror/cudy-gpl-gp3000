
#include <linux/types.h>

#ifndef NAND_INFO_PROC_H_NMT5KNSU
#define NAND_INFO_PROC_H_NMT5KNSU

struct nand_info_s{
	uint64_t nand_size;
	uint64_t chip_size;
	uint32_t block_size;
	uint32_t page_size;
	uint32_t oob_size;
	uint32_t oob_avail;
	uint32_t PagePerBlock;
	const char *model;
};

extern struct nand_info_s spi_nand_info;
int nand_info_init(void);

#endif /* end of include guard: NAND_INFO_PROC_H_NMT5KNSU */
