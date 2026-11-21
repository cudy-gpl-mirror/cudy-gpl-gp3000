#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <asm/io.h>
#include <fdt_support.h>

#ifdef CONFIG_NAND_HIDE_BAD
static int spi_nand_fdt_fixup_bbl_off(void *fdt)
{
                int nodeoffset;
                int err, ret = 0;
                u32 offset = NAND_BBL_OFFSET;
                err = fdt_check_header(fdt);
                if (err < 0) {
                                printf("fdt_chosen: %s\n", fdt_strerror(err));
                                return 0;
                }

                /* find or create "/chosen" node. */
                nodeoffset = fdt_find_or_add_subnode(fdt, 0, "sf-ctrl@f4324000");
                if (nodeoffset < 0)
                                return 0;
                err = fdt_setprop_u32(fdt, nodeoffset, "bbl_offset", offset);
                if (err < 0) {
                                printf("WARNING: can't set bbl_offset %s.\n",
                                           fdt_strerror(err));
                                return 0;
                }
                ret = 1;

                return ret;
}
#endif

/*
 * Bootm call it to fixup DTB for specified board
 */
int rtk_board_fdt_fixup(void *fdt)
{
	int ret=0;
#ifdef CONFIG_NAND_HIDE_BAD
	spi_nand_fdt_fixup_bbl_off(fdt);
#endif

	return ret;
}
