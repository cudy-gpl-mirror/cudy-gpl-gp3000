/*
 * Include Files
 */
#include <common.h>
#include <linux/ctype.h>
#include <asm/io.h>

#if defined(CONFIG_TARGET_RTK_MERCURY)
#define DRAM_SIZE  ((readl(GLOBAL_DRAM_SIZE_RCOD)>2048?2048:readl(GLOBAL_DRAM_SIZE_RCOD)) * 0x100000)
#else
#define DRAM_SIZE  (readl(0x111100c) * 0x100000)
#endif
#define SIZE_128MB  (0x08000000)
#define SIZE_256MB  (0x10000000)
#define SIZE_512MB  (0x20000000)
#define SIZE_1024MB (0x40000000)
#define SIZE_2048MB (0x80000000)

static u32 g_bit_width;
static u32 g_dram_type;
static u32 g_ddr_size;
static u32 g_ddr_freq;

/**************************************************
  * Offset 64MB(0x0400_0000) is u-boot start
  * Offset 240MB(0x0F00_0000) ~ 256MB(0x1000_0000) is reserved for Secure DRAM region
  * Bottom 30MB is reserved for u-boot relocation use
  **************************************************/
typedef struct {
    u32 start_addr;
    u32 end_addr;
    u32 test_size;
} memory_test_area_t;

memory_test_area_t mt_area_128MiB[] = {
    {.start_addr = (u32)(0x00000000), .end_addr = (u32)(0x03D00000), .test_size=0x03D00000}, // 0~61MB
    {.start_addr = (u32)(0x04300000), .end_addr = (u32)(0x07F00000), .test_size=0x03C00000}, // 67MB~127MB
    {.start_addr = (u32)(0x08000000), .end_addr = (u32)(0x08000000), .test_size=0x00000000}, // Dummy area
};

memory_test_area_t mt_area_256MiB[] = {
    {.start_addr = (u32)(0x00000000), .end_addr = (u32)(0x03D00000), .test_size=0x03D00000}, // 0~61MB
    {.start_addr = (u32)(0x04300000), .end_addr = (u32)(0x0D200000), .test_size=0x08F00000}, // 67MB~210MB
    {.start_addr = (u32)(0x10000000), .end_addr = (u32)(0x10000000), .test_size=0x00000000}, // Dummy area
};

memory_test_area_t mt_area_512MiB[] = {
    {.start_addr = (u32)(0x00000000), .end_addr = (u32)(0x03D00000), .test_size=0x03D00000}, // 0~61MB
    {.start_addr = (u32)(0x04300000), .end_addr = (u32)(0x0F000000), .test_size=0x0AD00000}, // 67MB~240MB
    {.start_addr = (u32)(0x10000000), .end_addr = (u32)(0x1E200000), .test_size=0x0E200000}, // 256MB~482MB
    {.start_addr = (u32)(0x20000000), .end_addr = (u32)(0x20000000), .test_size=0x00000000}, // Dummy area
};

memory_test_area_t mt_area_1024MiB[] = {
    {.start_addr = (u32)(0x00000000), .end_addr = (u32)(0x03D00000), .test_size=0x03D00000}, // 0~61MB
    {.start_addr = (u32)(0x04300000), .end_addr = (u32)(0x0F000000), .test_size=0x0AD00000}, // 67MB~240MB
    {.start_addr = (u32)(0x10000000), .end_addr = (u32)(0x3E200000), .test_size=0x2E200000}, // 256MB~994MB
    {.start_addr = (u32)(0x40000000), .end_addr = (u32)(0x40000000), .test_size=0x00000000}, // Dummy area
};

memory_test_area_t mt_area_2048MiB[] = {
    {.start_addr = (u32)(0x00000000), .end_addr = (u32)(0x03D00000), .test_size=0x03D00000}, // 0~61MB
    {.start_addr = (u32)(0x04300000), .end_addr = (u32)(0x0F000000), .test_size=0x0AD00000}, // 67MB~240MB
    {.start_addr = (u32)(0x10000000), .end_addr = (u32)(0x7E200000), .test_size=0x6E200000}, // 256MB~2018MB
    {.start_addr = (u32)(0x80000000), .end_addr = (u32)(0x80000000), .test_size=0x00000000}, // Dummy area
};

/* Definitions for debug & error handing manner */
#define MT_SUCCESS (0)
#define MT_FAIL    (-1)
#define MT_ILL_CMD (-2)
#define HANDLE_FAIL ({ return MT_FAIL; })

extern void flush_dcache_all(void);
#define _cache_flush() flush_dcache_all()

const unsigned int dram_patterns[] = {
  0x00000000, 0xffffffff, 0x55555555, 0xaaaaaaaa, 0x01234567,
  0x76543210, 0x89abcdef, 0xfedcba98, 0xA5A5A5A5, 0x5A5A5A5A,
  0xF0F0F0F0, 0x0F0F0F0F, 0xFF00FF00, 0x00FF00FF, 0x0000FFFF,
  0xFFFF0000, 0x00FFFF00, 0xFF0000FF, 0x5A5AA5A5, 0xA5A55A5A,
};

/**************************
 * Command Parsing
 *************************/
typedef struct {
	u32 start_addr[3];
	u32 test_size[3];
	u8 area_num;
	u8 test_case;
	u16 test_loops;
} ddr_cmd_parsing_info_t;

/*
 * Function Declaration
 */
int dram_normal_patterns(u32 dram_start, u32 size_per_pat, u32 area_size) {
	u32 i, j;
	u32 read_value;
	volatile u32 *r_addr;
	volatile u32 *w_addr;

	printf("=======start %s test=======\r", __FUNCTION__);
	for (i = 0; i < (sizeof(dram_patterns) / sizeof(u32)); i++) {
		w_addr = (u32 *)(uintptr_t)(dram_start + (i * size_per_pat) % (area_size));
		r_addr = (u32 *)((uintptr_t)w_addr + size_per_pat - 4);

		/* Write pattern */
		_cache_flush();
		for (j = 0; j < size_per_pat; j = j + 4) {
			*w_addr = dram_patterns[i];
			w_addr++;
		}

		/* Check data */
		_cache_flush();
		for (j = 0; j < size_per_pat; j = j + 4) {
			read_value = (*r_addr);
			if (read_value != dram_patterns[i]) {
				printf("\naddr(0x%08x):0x%08x != pattern(0x%08x) %s, %d\n", (u32)(uintptr_t)r_addr, read_value, dram_patterns[i], __FUNCTION__, __LINE__);
				HANDLE_FAIL;
			}
			r_addr--;
		}
		printf("pattern[%d](0x%08x) 0x%08x pass\r", i, dram_patterns[i], (u32)(uintptr_t)w_addr);
	}
	printf("%s completed.                   \r", __FUNCTION__);
	return MT_SUCCESS;
}

/*
 * write two half-words and read word.
 */
int dram_half_word_access(u32 dram_start, u32 size_per_pat, u32 area_size) {
	u32 i, j;
	u16 h_word;
	u32 read_value, test_start;
	volatile u16 *waddr_h;
	volatile u32 *raddr_w;

	printf("=======start %s test=======\r", __FUNCTION__);
	for (i = 0; i < (sizeof(dram_patterns) / sizeof(u32)); i++) {

		waddr_h = (u16 *)(uintptr_t)(dram_start + (i * size_per_pat) % (area_size));
		raddr_w = (u32 *)(uintptr_t)(dram_start + (i * size_per_pat) % (area_size) + size_per_pat - 4);
		test_start = (u32)(uintptr_t)waddr_h;

		/* Write half-word pattern*/
		_cache_flush();
		for (j = 0; j < size_per_pat; j = j + 4) {

			h_word = (u16)(dram_patterns[i]);
			*waddr_h = h_word;
			h_word = (u16)(dram_patterns[i] >> 16);
			*(waddr_h + 1) = h_word;
			waddr_h += 2;
		}

		/* Read word and check data */
		_cache_flush();
		for (j = 0; j < size_per_pat; j = j + 4) {
			if ((u32)(uintptr_t)raddr_w < test_start) {
				break;
			}
			read_value = (*raddr_w);
			if (read_value != dram_patterns[i]) {
				printf("\naddr(0x%08x):0x%08x != pattern(0x%08x) %s, %d\n", (u32)(uintptr_t)raddr_w, read_value, dram_patterns[i], __FUNCTION__, __LINE__);
				HANDLE_FAIL;
			}
			raddr_w--;
		}
		printf("pattern[%d](0x%08x) 0x%08x pass\r", i, dram_patterns[i], (u32)(uintptr_t)waddr_h);
	}

	printf("%s completed.                   \r", __FUNCTION__);
	return MT_SUCCESS;
}

int dram_byte_access(u32 dram_start, u32 size_per_pat, u32 area_size) {
	int i, j;
	u8 byte;
	u32 read_value, test_start;
	volatile u8 *waddr_b;
	volatile u32 *raddr_w;

	printf("=======start %s test=======\r", __FUNCTION__);
	for (i = 0; i < (sizeof(dram_patterns) / sizeof(u32)); i++) {
		raddr_w = (u32 *)(uintptr_t)(dram_start + (i*size_per_pat)%(area_size) + (size_per_pat-4));
		waddr_b =  (u8 *)(uintptr_t)(dram_start + (i*size_per_pat)%(area_size));
		test_start = (u32)(uintptr_t)waddr_b;

		/* write byte pattern*/
		_cache_flush();
		for (j = 0; j < size_per_pat; j = j + 4) {
			byte = (u8)(dram_patterns[i]);
			*(waddr_b + 0) = byte;
			byte = (u8)(dram_patterns[i] >> 8);
			*(waddr_b + 1) = byte;
			byte = (u8)(dram_patterns[i] >> 16);
			*(waddr_b + 2) = byte;
			byte = (u8)(dram_patterns[i] >> 24);
			*(waddr_b + 3) = byte;
			waddr_b += 4;
		}

		/* read word and check data */
		_cache_flush();
		for (j = 0; j < size_per_pat; j = j + 4) {
			if ((u32)(uintptr_t)raddr_w < test_start) {
				break;
			}
			read_value = *raddr_w;
			if (read_value != dram_patterns[i]) {
				printf("\naddr(0x%08x):0x%08x != pattern(0x%08x) %s, %d\n", (u32)(uintptr_t)raddr_w, read_value, dram_patterns[i], __FUNCTION__, __LINE__);
				HANDLE_FAIL;
			}
			raddr_w--;
		}
		printf("pattern[%d](0x%08x) 0x%08x pass\r", i, dram_patterns[i], (u32)(uintptr_t)waddr_b);
	}

	printf("%s completed.                   \r", __FUNCTION__);
	return MT_SUCCESS;
}

#pragma pack(1)
/* Data structures used for testing unaligned load/store operations. */
typedef struct {
	unsigned char c1;
	unsigned int w;
} t_off_1;

typedef struct {
	unsigned char c1;
	unsigned char c2;
	unsigned int w;
} t_off_2;
typedef struct {
	unsigned char c1;
	unsigned char c2;
	unsigned char c3;
	unsigned int w;
} t_off_3;

#pragma pack(4)
int unaligned_test(u32 dram_start, u32 size_per_pat, u32 area_size) {
	unsigned int i, j;
	volatile t_off_1 *off1;
	volatile t_off_2 *off2;
	volatile t_off_3 *off3;

	off1 = (t_off_1 *)(uintptr_t)dram_start;
	off2 = (t_off_2 *)(uintptr_t)dram_start;
	off3 = (t_off_3 *)(uintptr_t)dram_start;

	printf("=======start %s test=======\r", __FUNCTION__);

	/* patterns loop */
	for (j = 0; j < sizeof(dram_patterns) / sizeof(unsigned int); j++) {
		/* offset 1 bytes */
		off1 = (t_off_1 *)(uintptr_t)(dram_start + (j * size_per_pat) % (area_size));

		/* Write value */
		for (i = 0; i < (size_per_pat / sizeof(t_off_1)); i++) {
			off1[i].c1 = 0xcc;
			off1[i].w = dram_patterns[j];
		}

		/* check data */
		_cache_flush();
		for (i = 0; i < (size_per_pat / sizeof(t_off_1)); i++) {
			if ((off1[i].w != dram_patterns[j]) || (off1[i].c1 != 0xcc)) {
				printf("\noffset 1 error:addr(0x%08x) write 0x%08x, read 0x%08x, c1(%02x)\n", (u32)(uintptr_t)&off1[i], dram_patterns[j], off1[i].w, off1[i].c1);
				HANDLE_FAIL;
			}
		}

		/* offset 2 bytes */
		off2 = (t_off_2 *)(uintptr_t)(dram_start + (j * size_per_pat) % (area_size));

		/* Write value */
		for (i = 0; i < (size_per_pat / sizeof(t_off_2)); i++) {
			off2[i].c1 = 0xcc;
			off2[i].c2 = 0xdd;
			off2[i].w = dram_patterns[j];
		}

		/* check data */
		_cache_flush();
		for (i = 0; i < (size_per_pat / sizeof(t_off_2)); i++) {
			if (off2[i].w != dram_patterns[j] || (off2[i].c1 != 0xcc) ||
			    (off2[i].c2 != 0xdd)) {
				printf(
				  "\noffset 2 error:addr(0x%08x) write 0x%08x, read 0x%08x, c1(0x%08x), "
				  "c2(0x%08x)\n",
				  (u32)(uintptr_t)&off2[i], dram_patterns[j], off2[i].w, off2[i].c1,
				  off2[i].c2);
				printf("&dram_pattern[%d](0x%p) = 0x%08x\r", j, &dram_patterns[j], dram_patterns[j]);
				HANDLE_FAIL;
			}
		}

		/* offset 3 bytes */
		off3 = (t_off_3 *)(uintptr_t)(dram_start + (j * size_per_pat) % (area_size));

		/* Write value */
		for (i = 0; i < (size_per_pat / sizeof(t_off_3)); i++) {
			off3[i].c1 = 0xcc;
			off3[i].c2 = 0xdd;
			off3[i].c3 = 0xee;
			off3[i].w = dram_patterns[j];
		}

		/* check data */
		_cache_flush();
		for (i = 0; i < (size_per_pat / sizeof(t_off_3)); i++) {
			if (off3[i].w != dram_patterns[j] || (off3[i].c1 != 0xcc) ||
			    (off3[i].c2 != 0xdd) || (off3[i].c3 != 0xee)) {
				printf(
				  "\noffset 3 error:addr(0x%08x) write 0x%08x, "
				  "read 0x%08x, c1(%02x), c2(%02x), c3(%02x)\n",
				  (u32)(uintptr_t)&off3[i], dram_patterns[j], off3[i].w, off3[i].c1, off3[i].c2, off3[i].c3);
				HANDLE_FAIL;
			}
		}
		printf("pattern[%d](0x%08x) 0x%08x pass\r", j, dram_patterns[j], (u32)(uintptr_t)off3);
	}

	printf("%s test passed.                   \r", __FUNCTION__);
	return MT_SUCCESS;
}

int dram_busrt_read(u64 dram_start, u64 size_per_pat, u64 area_size) {
    u32 j;
    u64 read_value __attribute__((__unused__));
    volatile u64 *r_addr = (u64 *)(dram_start);

    printf("dram_busrt_read ... ");

    for (j = 0; j < area_size/8; j+=4) {
        read_value = (*r_addr++);
        read_value = (*r_addr++);
        read_value = (*r_addr++);
        read_value = (*r_addr++);
    }
    printf("completed.\n");
    return MT_SUCCESS;
}

int _dram_test(u32 dram_start, u32 area_size, u32 test_case)
{
    u32 i;
    u32 size_per_pat[2];
    u32 test_size[2];
    u32 start_addr[2];
    u32 total_dram_patterns = sizeof(dram_patterns) / sizeof(unsigned int);

    size_per_pat[0] = (area_size/total_dram_patterns)&0xFFFFFFF0;
    test_size[0] =  size_per_pat[0] * total_dram_patterns;
    test_size[1] = area_size - test_size[0];
    size_per_pat[1] = test_size[1];
    start_addr[0] = dram_start;
    start_addr[1] = start_addr[0] + test_size[0];

    /*
     *  5: burst read
     *  4: word test
     *  3: half word test
     *  2: byte test
     *  1: unalign test
     *  0: The general case, including word/half word/byte/unalign test
     */

    switch (test_case) {
        case 5:
            for(i=0 ; i<2 ; i++) {
                if (MT_FAIL == dram_busrt_read((u64)start_addr[i], (u64)size_per_pat[i], (u64)test_size[i]))
                    return MT_FAIL;
            }
            break;

        case 4:
            for(i=0 ; i<2 ; i++) {
                if (MT_FAIL == dram_normal_patterns(start_addr[i], size_per_pat[i], test_size[i]))
                    return MT_FAIL;
            }
            break;

        case 3:
            for(i=0 ; i<2 ; i++) {
                if (MT_FAIL == dram_half_word_access(start_addr[i], size_per_pat[i], test_size[i]))
                    return MT_FAIL;
            }
            break;

        case 2:
            for(i=0 ; i<2 ; i++) {
                if (MT_FAIL == dram_byte_access(start_addr[i], size_per_pat[i], test_size[i]))
                    return MT_FAIL;
            }
            break;

        case 1:
            for(i=0 ; i<2 ; i++) {
                if (MT_FAIL == unaligned_test(start_addr[i], size_per_pat[i], test_size[i]))
                    return MT_FAIL;
            }
            break;

        default:
            for(i=0 ; i<2 ; i++) {
                if (MT_FAIL == dram_normal_patterns(start_addr[i], size_per_pat[i], test_size[i]))
                    return MT_FAIL;

                if (MT_FAIL == dram_half_word_access(start_addr[i], size_per_pat[i], test_size[i]))
                    return MT_FAIL;

                if (MT_FAIL == dram_byte_access(start_addr[i], size_per_pat[i], test_size[i]))
                    return MT_FAIL;

                if (MT_FAIL == unaligned_test(start_addr[i], size_per_pat[i], test_size[i]))
                    return MT_FAIL;
            }
            break;
    }

	return MT_SUCCESS;
}

int ddr_cmd_parsing(int argc, char *const argv[], ddr_cmd_parsing_info_t *info)
{
	u32 i, x, y, tmp_loop;
	memory_test_area_t test_area[3];
    u32 tmp_start = 0;
    u32 tmp_size = 0;
    u32 tmp_end = 0;

    memset(&test_area, 0, sizeof(test_area));

#define ILL_CMD \
	({ \
		printf("EE: Illegal command (%d).\n\n\n", __LINE__); \
		return MT_ILL_CMD; \
	})

#define WRONG_INPUT_RANGE \
    ({ \
        printf("EE: Allowable test ranges: (%d)\n", __LINE__); \
        switch(DRAM_SIZE) { \
            case SIZE_256MB: \
                for (x=0 ; x<2 ; x++) \
                    printf("    <Range %d>: 0x%08x ~ 0x%08x\n",x, test_area[x].start_addr, test_area[x].end_addr); \
                break; \
            case SIZE_512MB: \
            case SIZE_1024MB: \
            case SIZE_2048MB: \
                for (x=0 ; x<3 ; x++) \
                    printf("    <Range %d>: 0x%08x ~ 0x%08x\n",x, test_area[x].start_addr, test_area[x].end_addr); \
                break; \
        } \
        return MT_ILL_CMD; \
    })


	/* Initialize the memory test parameters..... */
	info->test_loops = 1;

	/* Parse command flag for test range / test loops / mt error (non-)blocking
	 * mode */
	for (i = 1; i < argc;) {
		if ('-' != *argv[i]) ILL_CMD;

		if ((strcmp(argv[i], "-loops") == 0) || (strcmp(argv[i], "-l") == 0)) {
			if (((i + 1) >= argc) || (isxdigit(*argv[i + 1]) == 0)) ILL_CMD;

			tmp_loop = simple_strtoul(argv[i + 1], NULL, 10);
			info->test_loops = -1; /* set to maximal number for unsigned. */
			if (tmp_loop > info->test_loops) {
				puts("WW: loop count capped.\n");
				tmp_loop = info->test_loops;
			}
			info->test_loops = (tmp_loop == 0) ? 1 : tmp_loop;
			i = i + 2;
		} else if ((strcmp(argv[i], "-range") == 0) || (strcmp(argv[i], "-r") == 0)) {
			if (((i + 2) >= argc) || (isxdigit(*argv[i + 1]) == 0) || (isxdigit(*argv[i + 2]) == 0))
				ILL_CMD;

			tmp_start = simple_strtoul(argv[i + 1], NULL, 16);
			tmp_size = simple_strtoul(argv[i + 2], NULL, 16);
			tmp_end = tmp_start + tmp_size;

			if (0 == tmp_size) {
				printf( "EE: Please assign the correct test range, <size> shouldn't be zero.\n");
				ILL_CMD;
			}
			if ((tmp_start > DRAM_SIZE) || (tmp_size > DRAM_SIZE) || (tmp_end > DRAM_SIZE)) {
				printf("EE: Incorrect memory test range.\n");
				ILL_CMD;
			}
			if (((tmp_start%4) != 0) || ((tmp_size%4) != 0)){
				printf("EE: Unalignment address range.\n");
				ILL_CMD;
			}

            switch(DRAM_SIZE) {
                case SIZE_256MB:
                    memcpy(&test_area, &mt_area_256MiB, sizeof(mt_area_256MiB));

                    if (tmp_end >= test_area[1].end_addr) {
                        tmp_end = test_area[1].end_addr;
                    }

                    // Check if both [tmp_start & tmp_end] locate in forbidden area
                    for (x=0 ; x<2 ; x++) {
                        if ((tmp_start >= test_area[x].end_addr) && (tmp_end <= test_area[x+1].start_addr)) {
                            WRONG_INPUT_RANGE;
                        }
                    }

                    // Relocate [tmp_start & tmp_end] if one of them in forbidden area
                    for (x=0 ; x<2 ; x++) {
                        if ((tmp_start >= test_area[x].end_addr) && (tmp_start <= test_area[x+1].start_addr)) {
                            if (tmp_end == test_area[x+1].start_addr) {
                                WRONG_INPUT_RANGE;
                            }
                            tmp_start = test_area[x+1].start_addr;
                        }
                        if ((tmp_end >= test_area[x].end_addr) && (tmp_end <= test_area[x+1].start_addr))
                            tmp_end = test_area[x].end_addr;
                    }
                    tmp_size = tmp_end - tmp_start;

                    for (x=0 ; x<2 ; x++) {
                        if (((tmp_start < test_area[x].end_addr) && (tmp_start >= test_area[x].start_addr)) &&
                            ((tmp_end <= test_area[x].end_addr) && (tmp_end > test_area[x].start_addr))) {
                            info->area_num = 1;
                            info->start_addr[0] = tmp_start;
                            info->test_size[0] = tmp_size;
                            break;
                        }
                    }

                    x = 0;
                    y = 1;
                    if ((info->area_num == 0) &&
                        ((tmp_start < test_area[x].end_addr) && (tmp_start >= test_area[x].start_addr)) &&
                        ((tmp_end <= test_area[y].end_addr) && (tmp_end > test_area[y].start_addr))) {
                        info->area_num = 2;
                        info->start_addr[0] = tmp_start;
                        info->test_size[0]  = test_area[0].test_size - tmp_start;
                        info->start_addr[1] = test_area[1].start_addr;
                        info->test_size[1]  = tmp_end - info->start_addr[1];
                    }


                    if (info->area_num == 0) {
                        WRONG_INPUT_RANGE;
                    }
                    break;

                case SIZE_512MB:
                    memcpy(&test_area, &mt_area_512MiB, sizeof(mt_area_512MiB));
                case SIZE_1024MB:
                    memcpy(&test_area, &mt_area_1024MiB, sizeof(mt_area_1024MiB));
                case SIZE_2048MB:
                    memcpy(&test_area, &mt_area_2048MiB, sizeof(mt_area_2048MiB));

                    if (tmp_start >= test_area[2].end_addr) {
                        WRONG_INPUT_RANGE;
                    }

                    if (tmp_end >= test_area[2].end_addr) {
                        tmp_end = test_area[2].end_addr;
                    }

                    // Check if both [tmp_start & tmp_end] locate in forbidden area
                    for (x=0 ; x<3 ; x++) {
                        if ((tmp_start >= test_area[x].end_addr) && (tmp_end <= test_area[x+1].start_addr)) {
                            WRONG_INPUT_RANGE;
                        }
                    }

                    // Relocate [tmp_start & tmp_end] if one of them in forbidden area
                    for (x=0 ; x<3 ; x++) {
                        if ((tmp_start >= test_area[x].end_addr) && (tmp_start <= test_area[x+1].start_addr)) {
                            if (tmp_end == test_area[x+1].start_addr) {
                                WRONG_INPUT_RANGE;
                            }
                            tmp_start = test_area[x+1].start_addr;
                        }
                        if ((tmp_end >= test_area[x].end_addr) && (tmp_end <= test_area[x+1].start_addr))
                            tmp_end = test_area[x].end_addr;
                    }
                    tmp_size = tmp_end - tmp_start;

                    for (x=0 ; x<3 ; x++) {
                        if (((tmp_start < test_area[x].end_addr) && (tmp_start >= test_area[x].start_addr)) &&
                            ((tmp_end <= test_area[x].end_addr) && (tmp_end > test_area[x].start_addr))) {
                            info->area_num = 1;
                            info->start_addr[0] = tmp_start;
                            info->test_size[0] = tmp_size;
                            break;
                        }
                    }

                    x = 0;
                    y = 2;
                    if ((info->area_num == 0) &&
                        ((tmp_start < test_area[x].end_addr) && (tmp_start >= test_area[x].start_addr)) &&
                        ((tmp_end <= test_area[y].end_addr) && (tmp_end > test_area[y].start_addr))) {
                        info->area_num = 3;
                        info->start_addr[0] = tmp_start;
                        info->test_size[0]  = test_area[x].test_size - tmp_start;
                        info->start_addr[1] = test_area[1].start_addr;
                        info->test_size[1]  = test_area[1].test_size;
                        info->start_addr[2] = test_area[y].start_addr;
                        info->test_size[2]  = tmp_end - info->start_addr[2];
                    }

                    x = 0;
                    y = 1;
                    if ((info->area_num == 0) &&
                        ((tmp_start < test_area[x].end_addr) && (tmp_start >= test_area[x].start_addr)) &&
                        ((tmp_end <= test_area[y].end_addr) && (tmp_end > test_area[y].start_addr))) {
                        info->area_num = 2;
                        info->start_addr[0] = tmp_start;
                        info->test_size[0]  = test_area[x].test_size - tmp_start;
                        info->start_addr[1] = test_area[y].start_addr;
                        info->test_size[1]  = tmp_end - info->start_addr[1];
                    }

                    x = 1;
                    y = 2;
                    if ((info->area_num == 0) &&
                        ((tmp_start < test_area[x].end_addr) && (tmp_start >= test_area[x].start_addr)) &&
                        ((tmp_end <= test_area[y].end_addr) && (tmp_end > test_area[y].start_addr))) {
                        info->area_num = 2;
                        info->start_addr[0] = tmp_start;
                        info->test_size[0]  = test_area[x].test_size - tmp_start;
                        info->start_addr[1] = test_area[y].start_addr;
                        info->test_size[1]  = tmp_end - info->start_addr[1];
                    }

                    if (info->area_num == 0) {
                        WRONG_INPUT_RANGE;
                    }
                    break;
            }
			    i = i + 3;
		} else if (strcmp(argv[i], "-c") == 0) {
			info->test_case = simple_strtoul(argv[i + 1], NULL, 10);
			i = i + 2;
		} else {
			ILL_CMD;
		}
	}

    if (info->area_num == 0) {
        switch(DRAM_SIZE) {
            case SIZE_128MB:
                memcpy(&test_area, &mt_area_128MiB, sizeof(mt_area_128MiB));
                info->area_num = 2;
                break;
            case SIZE_256MB:
                memcpy(&test_area, &mt_area_256MiB, sizeof(mt_area_256MiB));
                info->area_num = 2;
                break;
            case SIZE_512MB:
                memcpy(&test_area, &mt_area_512MiB, sizeof(mt_area_512MiB));
                info->area_num = 3;
                break;
            case SIZE_1024MB:
                memcpy(&test_area, &mt_area_1024MiB, sizeof(mt_area_1024MiB));
                info->area_num = 3;
                break;
            case SIZE_2048MB:
                memcpy(&test_area, &mt_area_2048MiB, sizeof(mt_area_2048MiB));
                info->area_num = 3;
                break;

            default:
                ILL_CMD;
                break;
        }

        /* Assign the default test range */
        for (x=0 ; x<info->area_num ; x++) {
            info->start_addr[x] = test_area[x].start_addr;
            info->test_size[x] = test_area[x].test_size;
        }
    }

	printf("II: # of iteration: %d\n", info->test_loops);
	return MT_SUCCESS;
}

int dram_memory_test(int argc, char *const argv[]) {
	u32 i, j;
	int retcode = MT_SUCCESS;
	ddr_cmd_parsing_info_t cmd_info;

    g_bit_width = readl(0x1111004);
    g_dram_type = readl(0x1111008);
    g_ddr_size  = readl(0x111100c);
    g_ddr_freq  = readl(0x1111010);

    memset(&cmd_info, 0, sizeof(ddr_cmd_parsing_info_t));
	retcode = ddr_cmd_parsing(argc, argv, &cmd_info);
	if (MT_ILL_CMD == retcode) return retcode;

	for (j = 0; j < cmd_info.test_loops; j++) {
		for (i = 0; i < cmd_info.area_num; i++) {
			printf("<Range %d: 0x%08x~0x%08x>\n", i + 1, cmd_info.start_addr[i], cmd_info.start_addr[i] + cmd_info.test_size[i] - 1);

            /* [test_case]
             *  4: word test
             *  3: half word test
             *  2: byte test
             *  1: unalign test
             *  0: The general case, including word/half word/byte/unalign test
             */
            retcode = _dram_test(cmd_info.start_addr[i], cmd_info.test_size[i], cmd_info.test_case);
			if (MT_FAIL == retcode){
                printf("\nEE: mdram_test failed.\n\n");
                goto end;
			}

#if 0
			switch (cmd_info.test_case) {
                case 2:
				case 1:
				case 0:
				default:
					retcode = _dram_test(cmd_info.start_addr[i], cmd_info.test_size[i]);
					if (MT_FAIL == retcode){
                        printf("\nEE: mdram_test failed.\n\n");
					    goto end;
					}
					break;
			}
#endif
		}

		printf("<mdram_test %d runs>                  \n\n", j + 1);
	}

end:
    writel(g_bit_width, 0x1111004);
    writel(g_dram_type, 0x1111008);
    writel(g_ddr_size,  0x111100c);
    writel(g_ddr_freq,  0x1111010);
	return MT_SUCCESS;
}

