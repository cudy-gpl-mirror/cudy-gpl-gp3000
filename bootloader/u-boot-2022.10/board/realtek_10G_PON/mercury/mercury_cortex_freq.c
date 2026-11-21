#include <common.h>
#include <asm/io.h>

uint64_t rtk_cortex_freq_get(void)
{
#ifdef CONFIG_RTK_MERCURY_SINGLE_FPGA_PLATFORM
    return 50;
#elif defined(CONFIG_TARGET_RTK_MERCURY) && defined(CONFIG_RTK_V8_FPGA_PLATFORM)
    return 20;
#else /* ASIC */
    /* Lower CPU PLL to avoid rush current
     * Configure CPU to 800MHz according to HW suggestion.
     * CLK = 25M * [(ncode + 4) + (fcode/2048)]
     */
    GLOBAL_CPLL0_t cpll0;
    GLOBAL_CPLL_SSC1_t cpll_ssc1;

    cpll0.wrd = readl(GLOBAL_CPLL0);

    cpll_ssc1.wrd = readl(GLOBAL_CPLL_SSC1);

    return (25000000 * ((cpll0.bf.ncode_t + 4) + (cpll_ssc1.bf.fcode_t/2048))) / 1000000;
#endif

}

