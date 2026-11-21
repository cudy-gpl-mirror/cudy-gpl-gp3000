/*
 * Include Files
 */
#include <common.h>
#include <linux/ctype.h>
#include <asm/io.h>


/* Definitions for debug & error handing manner */
#define MT_SUCCESS (0)
#define MT_FAIL    (-1)
#define MT_ILL_CMD (-2)
#define HANDLE_FAIL ({ return MT_FAIL; })

extern void flush_dcache_all(void);
extern int smp_printf(const char *fmt, ...);
extern int get_cpu_id(void);

#define _cache_flush() flush_dcache_all()


#define CORENUM get_cpu_id()
#define PRINTF_CT(fmt, ...)   smp_printf("[CORE%d]" fmt, CORENUM, ##__VA_ARGS__)

#define printf PRINTF_CT



static const unsigned int dram_patterns[] = {
  0x00000000, 0xffffffff, 0x55555555, 0xaaaaaaaa, 0x01234567,
  0x76543210, 0x89abcdef, 0xfedcba98, 0xA5A5A5A5, 0x5A5A5A5A,
  0xF0F0F0F0, 0x0F0F0F0F, 0xFF00FF00, 0x00FF00FF, 0x0000FFFF,
  0xFFFF0000, 0x00FFFF00, 0xFF0000FF, 0x5A5AA5A5, 0xA5A55A5A,
};

/*
 * Function Declaration
 */
static int dram_normal_patterns(u32 dram_start, u32 size_per_pat, u32 area_size) {
	u32 i, j;
	u32 read_value;
	volatile u32 *r_addr;
	volatile u32 *w_addr;

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
		printf("pattern[%d](0x%08x) 0x%08x pass\n", i, dram_patterns[i], (u32)(uintptr_t)w_addr);
	}
	printf("%s completed.                   \n", __FUNCTION__);
	return MT_SUCCESS;
}

/*
 * write two half-words and read word.
 */
static int dram_half_word_access(u32 dram_start, u32 size_per_pat, u32 area_size) {
	u32 i, j;
	u16 h_word;
	u32 read_value, test_start;
	volatile u16 *waddr_h;
	volatile u32 *raddr_w;

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
		printf("pattern[%d](0x%08x) 0x%08x pass\n", i, dram_patterns[i], (u32)(uintptr_t)waddr_h);
	}

	printf("%s completed.                   \n", __FUNCTION__);
	return MT_SUCCESS;
}

static int dram_byte_access(u32 dram_start, u32 size_per_pat, u32 area_size) {
	int i, j;
	u8 byte;
	u32 read_value, test_start;
	volatile u8 *waddr_b;
	volatile u32 *raddr_w;

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
		printf("pattern[%d](0x%08x) 0x%08x pass\n", i, dram_patterns[i], (u32)(uintptr_t)waddr_b);
	}

	printf("%s completed.                   \n", __FUNCTION__);
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
static int unaligned_test(u32 dram_start, u32 size_per_pat, u32 area_size) {
	unsigned int i, j;
	volatile t_off_1 *off1;
	volatile t_off_2 *off2;
	volatile t_off_3 *off3;

	off1 = (t_off_1 *)(uintptr_t)dram_start;
	off2 = (t_off_2 *)(uintptr_t)dram_start;
	off3 = (t_off_3 *)(uintptr_t)dram_start;

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
				printf("&dram_pattern[%d](0x%p) = 0x%08x\n", j, &dram_patterns[j], dram_patterns[j]);
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
		printf("pattern[%d](0x%08x) 0x%08x pass\n", j, dram_patterns[j], (u32)(uintptr_t)off3);
	}

	printf("%s test passed.                   \n", __FUNCTION__);
	return MT_SUCCESS;
}

static int dram_busrt_read(u64 dram_start, u64 size_per_pat, u64 area_size) {
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

int _smp_dram_test(u32 dram_start, u32 area_size, u32 test_case)
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

