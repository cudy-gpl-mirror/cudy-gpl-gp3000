#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <asm/io.h>
#include <fdt_support.h>
#include <linux/arm-smccc.h>
#include <rand.h>
#include <time.h>
#include "cortina_smc.h"

#define ARM_SMCCC_TRNG_RND_64           0xC4000053
#define TRNG_MAX_RND_64         (192 / 8)
#define ARM_SMCCC_RET_TRNG_SUCCESS              ((ulong)0)
#define ARM_SMCCC_RET_TRNG_NOT_SUPPORTED        ((ulong)-1)
#define ARM_SMCCC_RET_TRNG_INVALID_PARAMETER    ((ulong)-2)
#define ARM_SMCCC_RET_TRNG_NO_ENTROPY           ((ulong)-3)

static size_t smc64_copy_sample(u8 **ptr, size_t size, ulong *rnd)
{
        size_t len = min(size, sizeof(u64));
        u64 sample = *rnd;

        memcpy(*ptr, &sample, len);
        *ptr += len;

        return size - len;
}

static int rtk_smccc_trng_read(void *data, size_t len)
{
        struct arm_smccc_res res;
        u32 func_id;
        u8 *ptr = data;
        size_t rem = len;
        size_t max_sz;
        size_t (*copy_sample)(u8 **ptr, size_t size, ulong *rnd);

        copy_sample = smc64_copy_sample;
        func_id = ARM_SMCCC_TRNG_RND_64;
        max_sz = TRNG_MAX_RND_64;

        while (rem) {
                size_t sz = min(rem, max_sz);
                arm_smccc_smc(func_id, sz * 8, 0, 0, 0, 0, 0, 0, &res);
                switch (res.a0) {
                case ARM_SMCCC_RET_TRNG_SUCCESS:
                        break;
                case ARM_SMCCC_RET_TRNG_NO_ENTROPY:
                        return -EIO;
                default:
                        return -EIO;
                }

                rem -= sz;

                sz = copy_sample(&ptr, sz, &res.a3);
                if (sz)
                        sz = copy_sample(&ptr, sz, &res.a2);
                if (sz)
                        sz = copy_sample(&ptr, sz, &res.a1);
        }
        return 0;
}

void rtk_board_prng_read64(uint64_t *data){
	int err;
	u64 val0 = 0;
	err = rtk_smccc_trng_read((void *)data, 8);
	if (err) {
		printf("WARNING: Not support smccc-trng. Use software-trng\n");
		srand(get_ticks());
		rand();
		val0 = rand();
		val0 = val0 << 32;
		*data = val0  | rand();
	}
}

/*
 * Bootm use it to random linux kaslr seed.
 */
int fdt_fixup_kaslr(void *fdt)
{
		int nodeoffset;
		int err, ret = 0;
		u8 rand[8];

		rtk_board_prng_read64((uint64_t *)rand);
		err = fdt_check_header(fdt);
		if (err < 0) {
				printf("fdt_chosen: %s\n", fdt_strerror(err));
				return 0;
		}

		/* find or create "/chosen" node. */
		nodeoffset = fdt_find_or_add_subnode(fdt, 0, "chosen");
		if (nodeoffset < 0)
				return 0;

		err = fdt_setprop(fdt, nodeoffset, "kaslr-seed", rand,
								  sizeof(rand));
		if (err < 0) {
				printf("WARNING: can't set kaslr-seed %s.\n",
					   fdt_strerror(err));
				return 0;
		}
		ret = 1;

		return ret;
}
