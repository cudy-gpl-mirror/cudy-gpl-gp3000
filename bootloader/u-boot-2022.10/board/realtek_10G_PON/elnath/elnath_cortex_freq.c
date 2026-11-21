#include <common.h>
#include <asm/io.h>

uint64_t rtk_cortex_freq_get(void)
{
    unsigned short cpu_speed_table[2][4] = {
        {500, 550, 1000, 1100},
        {500, 600, 1000, 1200}
    };
    uint32_t pad_rsvd0 = (readl(GLOBAL_GPI_STATUS1) >> 29) & 1;

    GLOBAL_CPLLDIV_t cplldiv;
    GLOBAL_CPLL0_t cpll0;
    GLOBAL_STRAP_t global_strap;
    uint64_t cur_rate_MHz;

    global_strap.wrd = readl(GLOBAL_STRAP);
    cplldiv.wrd = readl(GLOBAL_CPLLDIV);

    if ((cplldiv.bf.cpll_div_override == 1) && (cplldiv.bf.cpll_mode_override == 1)) {
        cpll0.wrd = readl(GLOBAL_CPLL0);

        if (cpll0.bf.DIV_PREDIV_BPS) {
            cur_rate_MHz = (cpll0.bf.SCPU_DIV_DIVN + 3) * 25;
        } else {
            cur_rate_MHz = (cpll0.bf.SCPU_DIV_DIVN + 3) * (40 / 2);
        }
    } else {
        cur_rate_MHz = cpu_speed_table[pad_rsvd0][global_strap.bf.speed];
    }

    return cur_rate_MHz;
}

