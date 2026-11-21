/*
 * (C) Copyright 2024
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * Misc functions
 */
#include <common.h>
#include <command.h>
#include <cpu_func.h>
#include <asm/io.h>
#include <linux/delay.h>

#define SIZE             (32 * 1024 * 1024)
#define BLOCKSIZE        2048
#ifndef MAXREPEATS
# define MAXREPEATS      10
#endif
#ifndef LATBENCH_COUNT
# define LATBENCH_COUNT  10000000
#endif

int size_bit = 10;


static u64 gettime(void)
{
	return timer_get_us();
}


static void noinline random_read_test(char *zerobuffer,
                                                       int count, int nbits)
{
    uint32_t seed = 0;
    uintptr_t addrmask = (1 << nbits) - 1;
    uint32_t v;
    static volatile uint32_t dummy;

#ifdef __arm__
    uint32_t tmp;
    __asm__ volatile (
        "subs %[count], %[count],       #16\n"
        "blt  1f\n"
    "0:\n"
        "subs %[count], %[count],       #16\n"
    ".rept 16\n"
        "mla  %[seed],  %[c1103515245], %[seed],        %[c12345]\n"
        "and  %[v],     %[xFF],         %[seed],        lsr #16\n"
        "mla  %[seed],  %[c1103515245], %[seed],        %[c12345]\n"
        "and  %[tmp],   %[xFF00],       %[seed],        lsr #8\n"
        "mla  %[seed],  %[c1103515245], %[seed],        %[c12345]\n"
        "orr  %[v],     %[v],           %[tmp]\n"
        "and  %[tmp],   %[x7FFF0000],   %[seed]\n"
        "orr  %[v],     %[v],           %[tmp]\n"
        "and  %[v],     %[v],           %[addrmask]\n"
        "ldrb %[v],     [%[zerobuffer], %[v]]\n"
        "orr  %[seed],  %[seed],        %[v]\n"
    ".endr\n"
        "bge  0b\n"
    "1:\n"
        "add  %[count], %[count],       #16\n"
        : [count] "+&r" (count),
          [seed] "+&r" (seed), [v] "=&r" (v),
          [tmp] "=&r" (tmp)
        : [c1103515245] "r" (1103515245), [c12345] "r" (12345),
          [xFF00] "r" (0xFF00), [xFF] "r" (0xFF),
          [x7FFF0000] "r" (0x7FFF0000),
          [zerobuffer] "r" (zerobuffer),
          [addrmask] "r" (addrmask)
        : "cc");
#else
    #define RANDOM_MEM_ACCESS()                 \
        seed = seed * 1103515245 + 12345;       \
        v = (seed >> 16) & 0xFF;                \
        seed = seed * 1103515245 + 12345;       \
        v |= (seed >> 8) & 0xFF00;              \
        seed = seed * 1103515245 + 12345;       \
        v |= seed & 0x7FFF0000;                 \
        seed |= zerobuffer[v & addrmask];

    while (count >= 16) {
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        count -= 16;
    }
#endif
    dummy = seed;
    #undef RANDOM_MEM_ACCESS
}

static void noinline random_dual_read_test(char *zerobuffer,
                                                            int count, int nbits)
{
    uint32_t seed = 0;
    uintptr_t addrmask = (1 << nbits) - 1;
    uint32_t v1, v2;
    static volatile uint32_t dummy;

#ifdef __arm__
    uint32_t tmp;
    __asm__ volatile (
        "subs %[count], %[count],       #16\n"
        "blt  1f\n"
    "0:\n"
        "subs %[count], %[count],       #16\n"
    ".rept 16\n"
        "mla  %[seed],  %[c1103515245], %[seed],        %[c12345]\n"
        "and  %[v1],    %[xFF00],       %[seed],        lsr #8\n"
        "mla  %[seed],  %[c1103515245], %[seed],        %[c12345]\n"
        "and  %[v2],    %[xFF00],       %[seed],        lsr #8\n"
        "mla  %[seed],  %[c1103515245], %[seed],        %[c12345]\n"
        "and  %[tmp],   %[x7FFF0000],   %[seed]\n"
        "mla  %[seed],  %[c1103515245], %[seed],        %[c12345]\n"
        "orr  %[v1],    %[v1],          %[tmp]\n"
        "and  %[tmp],   %[x7FFF0000],   %[seed]\n"
        "mla  %[seed],  %[c1103515245], %[seed],        %[c12345]\n"
        "orr  %[v2],    %[v2],          %[tmp]\n"
        "and  %[tmp],   %[xFF],         %[seed],        lsr #16\n"
        "orr  %[v2],    %[v2],          %[seed],        lsr #24\n"
        "orr  %[v1],    %[v1],          %[tmp]\n"
        "and  %[v2],    %[v2],          %[addrmask]\n"
        "eor  %[v1],    %[v1],          %[v2]\n"
        "and  %[v1],    %[v1],          %[addrmask]\n"
        "ldrb %[v2],    [%[zerobuffer], %[v2]]\n"
        "ldrb %[v1],    [%[zerobuffer], %[v1]]\n"
        "orr  %[seed],  %[seed],        %[v2]\n"
        "add  %[seed],  %[seed],        %[v1]\n"
    ".endr\n"
        "bge  0b\n"
    "1:\n"
        "add  %[count], %[count],       #16\n"
        : [count] "+&r" (count),
          [seed] "+&r" (seed), [v1] "=&r" (v1), [v2] "=&r" (v2),
          [tmp] "=&r" (tmp)
        : [c1103515245] "r" (1103515245), [c12345] "r" (12345),
          [xFF00] "r" (0xFF00), [xFF] "r" (0xFF),
          [x7FFF0000] "r" (0x7FFF0000),
          [zerobuffer] "r" (zerobuffer),
          [addrmask] "r" (addrmask)
        : "cc");
#else
    #define RANDOM_MEM_ACCESS()                 \
        seed = seed * 1103515245 + 12345;       \
        v1 = (seed >> 8) & 0xFF00;              \
        seed = seed * 1103515245 + 12345;       \
        v2 = (seed >> 8) & 0xFF00;              \
        seed = seed * 1103515245 + 12345;       \
        v1 |= seed & 0x7FFF0000;                \
        seed = seed * 1103515245 + 12345;       \
        v2 |= seed & 0x7FFF0000;                \
        seed = seed * 1103515245 + 12345;       \
        v1 |= (seed >> 16) & 0xFF;              \
        v2 |= (seed >> 24);                     \
        v2 &= addrmask;                         \
        v1 ^= v2;                               \
        seed |= zerobuffer[v2];                 \
        seed += zerobuffer[v1 & addrmask];

    while (count >= 16) {
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        RANDOM_MEM_ACCESS();
        count -= 16;
    }
#endif
    dummy = seed;
    #undef RANDOM_MEM_ACCESS
}


// Function to find the square-root of N
static int findSQRT(int number)
{
    int start = 0, end = number;
    int mid;
 
    // To store the answer
    int ans;
 
    // To find integral part of square
    // root of number
    while (start <= end) {
 
        // Find mid
        mid = (start + end) / 2;
 
        // If number is perfect square
        // then break
        if (mid * mid == number) {
            ans = mid;
            break;
        }
 
        // Increment start if integral
        // part lies on right side
        // of the mid
        if (mid * mid < number) {
          //first start value should be added to answer
            ans=start;
          //then start should be changed
            start = mid + 1;
        }
 
        // Decrement end if integral part
        // lies on the left side of the mid
        else {
            end = mid - 1;
        }
    }
     return ans;
}
static uint32_t rand32(void)
{
    static int seed = 0;
    uint32_t hi, lo;
    hi = (seed = seed * 1103515245 + 12345) >> 16;
    lo = (seed = seed * 1103515245 + 12345) >> 16;
    return (hi << 16) + lo;
}

int latency_bench(int size, int count, int use_hugepage)
{
    u64 t, t2, t_before, t_after, t_noaccess, t_noaccess2;
    u64 xs, xs0, xs1, xs2;
    u64 ys, ys0, ys1, ys2;
    u64 min_t, min_t2;
    int nbits, n;
    char *buffer, *buffer_alloc;
	/* default, use Size 32M*2 */

	/* default, use 128~192 */
	buffer = (char *)0x8000000;
	buffer_alloc = buffer;

    memset(buffer, 0, size);

    for (n = 1; n <= MAXREPEATS; n++)
    {
        t_before = gettime();
        random_read_test(buffer, count, 1);
        t_after = gettime();
        if (n == 1 || t_after - t_before < t_noaccess)
            t_noaccess = t_after - t_before;

        t_before = gettime();
        random_dual_read_test(buffer, count, 1);
        t_after = gettime();
        if (n == 1 || t_after - t_before < t_noaccess2)
            t_noaccess2 = t_after - t_before;
    }

    printf("\nblock size : single random read / dual random read");
    printf("\n");
#if 0
    for (nbits = 10; (1 << nbits) <= size; nbits++)
#else
    for (nbits = size_bit; (1 << nbits) <= size; nbits++)
#endif
    {
        int testsize = 1 << nbits;
        xs1 = xs2 = ys = ys1 = ys2 = 0;
        for (n = 1; n <= MAXREPEATS; n++)
        {
            /*
             * Select a random offset in order to mitigate the unpredictability
             * of cache associativity effects when dealing with different
             * physical memory fragmentation (for PIPT caches). We are reporting
             * the "best" measured latency, some offsets may be better than
             * the others.
             */
            int testoffs = (rand32() % (size / testsize)) * testsize;

            t_before = gettime();
            random_read_test(buffer + testoffs, count, nbits);
            t_after = gettime();
            t = t_after - t_before - t_noaccess;
            if (t < 0) t = 0;

            xs1 += t;
            xs2 += t * t;

            if (n == 1 || t < min_t)
                min_t = t;

            t_before = gettime();
            random_dual_read_test(buffer + testoffs, count, nbits);
            t_after = gettime();
            t2 = t_after - t_before - t_noaccess2;
            if (t2 < 0) t2 = 0;

            ys1 += t2;
            ys2 += t2 * t2;

            if (n == 1 || t2 < min_t2)
                min_t2 = t2;
#if 1
            if (n > 3)
            {
                xs = findSQRT((xs2 * n - xs1 * xs1) / (n * (n - 1)));
                ys = findSQRT((ys2 * n - ys1 * ys1) / (n * (n - 1)));
                if (xs < min_t / 1000 && ys < min_t2 / 1000)
                    break;
            }
#endif

        }
        printf("%10d :  %llu.%02llu ns          /   %llu.%02llu ns \n", (1 << nbits),
			min_t*1000 / count, (min_t*100000 / count)%100,  min_t2*1000/ count, (min_t2*100000/ count)%100);
			//printf("%10d : %lu ns          /  %lu ns \n", (1 << nbits),
           //min_t * 1000000000 / count,  min_t2 * 1000000000 / count);
    }

    return 1;
}
static int do_bench(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	int latbench_size = SIZE * 2, latbench_count = LATBENCH_COUNT;
	latency_bench(latbench_size, latbench_count, -1);
	return 0;
}

U_BOOT_CMD(
	tinymembench ,    2,    0,     do_bench,
	"tinymembench",
	"\n\t\t\n"
	"\t\t \n"
	"."
);

