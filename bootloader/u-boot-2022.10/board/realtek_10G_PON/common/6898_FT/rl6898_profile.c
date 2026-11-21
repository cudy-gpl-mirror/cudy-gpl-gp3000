#define CHIP_NAME    "RL6898"
#define TEST_PROG_VER 0x0001

#define DEFAULT_TESTS \
    ( \
    (0x1 << FT2_PARAM_PRBS_PON) | \
    (0x1 << FT2_PARAM_GPIO) | \
    (0x1 << FT2_PARAM_PRBS_PON_RESULT) \
    )

#define PRBS_PON_MASK (1 << 0)
#define PRBS_WTG_MASK (1 << 0)

#define GPIO_PON_BEN 123
#define PON_BEN_MASK (1 << 0)

#define PCIE_MASK (1 << 0)
#define GPIO_PCIE0 4
#define GPIO_PCIE1 5
#define GPIO_PCIE2 6
#define GPIO_PCIE7 79
#define GPIO_PCIE8 8

#define USB_MASK (1 << 0)

#define CPU_CORE_MASK ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3))

#define RISCV_MASK (1 << 0)

#define GPIO_MASK (1 << 0)

#define S0_HISGMII_MASK (1 << 0)
#define S2_HISGMII_MASK (1 << 0)
#define S3_HISGMII_MASK (1 << 0)
#define GPIO_RELAY_S0 112
#define GPIO_RELAY_S2 113
#define GPIO_RELAY_S3 140

#define GPIO_MASK  (1 << 0)
