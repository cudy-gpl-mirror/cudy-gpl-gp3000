#define REG32_FIELD_SET(_data, _val, _fOffset, _fMask)      ((_data & ~(_fMask)) | ((_val << (_fOffset)) & (_fMask)))

#define RTL8224_SWREG_ADDR(_addr)            ((_addr) & (0xffff))
#define PHY_MMD_VEND1               30   /* Vendor specific 1 */
#define PHY_MMD_VEND2               31   /* Vendor specific 2 */

#define RTK_HWPATCH_OP_UNKNOWN 0
#define RTK_HWPATCH_OP_SDS     1
#define RTK_HWPATCH_OP_PHY     2
#define RTK_HWPATCH_OP_PHYW    3
#define RTK_HWPATCH_OP_ALGO    4
#define RTK_HWPATCH_OP_TOP     5
#define RTK_HWPATCH_OP_MMD     6
#define RTK_HWPATCH_OP_DATARAM 7

#define PHY_RTL8224_VER_A                           (0)
#define PHY_RTL8224_VER_B                           (1)
#define PHY_RTL8224_VER_C                           (2)

#define PHY_PATCH_OP_NORMAL             0
#define PHY_PATCH_OP_BCAST              1
#define PHY_PATCH_OP_BCAST_SAME_CHIP    2
#define PHY_PATCH_WAIT_TIMEOUT          10000000

#define RTL8224_AB_FW_VER           0x006
#define RTL8224_FW_VER              0x0021
#define RTL8224_MAIN_VER            0xFF
#define RTL8224_SW_VER              0xFF
#define RTL8224_TOP_VER             0xFF
#define RTL8224_AFEFW_VER           0xFF
#define RTL8224_PATCH_FLOW_VER      "BV0002"

#define BIT_0       0x0001
#define BIT_1       0x0002
#define BIT_2       0x0004
#define BIT_3       0x0008
#define BIT_4       0x0010
#define BIT_5       0x0020
#define BIT_6       0x0040
#define BIT_7       0x0080
#define BIT_8       0x0100
#define BIT_9       0x0200
#define BIT_10      0x0400
#define BIT_11      0x0800
#define BIT_12      0x1000
#define BIT_13      0x2000
#define BIT_14      0x4000
#define BIT_15      0x8000

#define RTL8224_PORT_NUM                            (4)
#define RTL8224_PER_PORT_MAX_LED                    (4)
#define RTL8224_LED_PAD_MAX_INDEX                   (29)


#define RTL8224_INTER_REG_LSB                       (0)
#define RTL8224_INTER_REG_MSB                       (31)

#define RTL8224_PHY_MDI_REVERSE                     0xbb000a90
#define RTL8224_MDI_REVERSE_OFFSET                  (0)

#define RTL8224_PHY_TX_POLARITY_SWAP                0xbb000a94
#define RTL8224_PHY_TX_POLARITY_SWAP_OFFSET         (4)

#define RTL8224_CFG_PHY_POLL_CMD1_REG               (0xbb000ad8)
#define RTL8224_CMD_RD_EN_3_0_MSB                   (3)
#define RTL8224_CMD_RD_EN_3_0_LSB                   (0)


/* Internal Register */
/* Global interrupt control */
#define PHY_8224_INT_MODE_HIGH_LEVEL                (0)
#define PHY_8224_INT_MODE_LOW_LEVEL                 (1)
#define PHY_8224_INT_MODE_POS_EDGE                  (2)
#define PHY_8224_INT_MODE_NEG_EDGE                  (3)

#define RTL8224_ISR_SW_INT_MODE                     0xbb005f84
#define RTL8224_SWITCH_IE_OFFSET                    (6)
#define RTL8224_SWITCH_INT_MODE_OFFSET              (1)
#define RTL8224_SWITCH_INT_MODE_MSB                 (2)

/* GPHY interrupt control */
#define RTL8224_IMR_EXT_GPHY                        0xbb005f5c
#define RTL8224_IMR_EXT_GPHY_3_0_OFFSET             (0)
#define RTL8224_IMR_EXT_GPHY_3_0_MSB                (3)

#define RTL8224_ISR_EXT_GPHY                        0xbb005fb0
#define RTL8224_ISR_EXT_GPHY_OFFSET                 (0)
#define RTL8224_ISR_EXT_GPHY_MSB                    (3)

#define RTL8224_IMR_EXT_WOL                         0xbb005f68
#define RTL8224_IMR_EXT_PHYWOL_PORT_3_0_OFFSET      (9)
#define RTL8224_IMR_EXT_PHYWOL_PORT_3_0_MSB         (12)

#define RTL8224_IMR_INT_RLFD                        0xbb005f40
#define RTL8224_IMR_INT_RLFD_PORT_8_0_OFFSET        (0)

#define RTL8224_IMR_EXT_RLFD                        0xbb005f64
#define RTL8224_IMR_EXT_RLFD_PORT_8_0_OFFSET        (0)
#define RTL8224_IMR_EXT_RLFD_PORT_SHIFT             (4)


#define RTL8224_IMR_EXT_MISC                        0xbb005f78
#define RTL8224_IMR_EXT_TM_LOW_OFFSET               (1)
#define RTL8224_IMR_EXT_TM_HIGH_OFFSET              (0)
#define RTL8224_IMR_EXT_PTP1588_OFFSET              (18)
#define RTL8224_IMR_EXT_MACSECWRP_3_0_OFFSET        (19)
#define RTL8224_IMR_EXT_MACSECWRP_3_0_MSB           (22)

#define RTL8224_ISR_EXT_MISC                        0xbb005fcc
#define RTL8224_ISR_EXT_TM_LOW_OFFSET               (1)
#define RTL8224_ISR_EXT_TM_HIGH_OFFSET              (0)
#define RTL8224_ISR_EXT_PTP1588_OFFSET              (18)
#define RTL8224_ISR_EXT_MACSECWRP_3_0_OFFSET        (19)
#define RTL8224_ISR_EXT_MACSECWRP_3_0_MSB           (22)

#define RTL8224_ISR_EXT_WOL                         0xbb005fbc
#define RTL8224_ISR_EXT_PHYWOL_PORT_3_0_OFFSET      (9)

#define RTL8224_GPHY_INTR_STATUS_REG                (0xbb00a43a)
#define GPHY_AN_ERROR_BIT                           (0)
#define GPHY_AN_NEXT_PAGE_RECEIVE_BIT               (2)
#define GPHY_LINK_CHG_BIT                           (4)
#define GPHY_ALDPS_STS_CHG_BIT                      (9)
#define GPHY_FATEL_ERROR_BIT                        (11)

#define RTL8224_ISR_INT_TM_RLFD_REG                 (0xbb005f94)
#define RTL8224_ISR_INT_RLFD_PORT_8_0_OFFSET        (0)
#define RTL8224_ISR_INT_RLFD_PORT_8_0_MSB           (8)

#define RTL8224_ISR_EXT_TM_RLFD_REG                 (0xbb005fb8)
#define RTL8224_ISR_EXT_RLFD_PORT_8_0_OFFSET        (0)
#define RTL8224_ISR_EXT_RLFD_PORT_8_0_MSB           (8)
#define RTL8224_ISR_EXT_RLFD_PORT_SHIFT             (4)


#define RTL8224_LED_SET_CTRL_OFFSET                 (8)

#define RTL8224_RLFD_CTRL_REG                       (0xbb006458)
#define RTL8224_RLFD_SEL_OFFSET                     (10)
#define RTL8224_RLFD_EN_OFFSET                      (9)

#define RTL8224_LED3_2_SET0_CTRL0_REG               (0xbb006544)
#define RTL8224_SET_LED2_SEL0_OFFSET                (0)
#define RTL8224_SET_LED2_SEL0_MSB                   (15)
#define RTL8224_SET_LED3_SEL0_OFFSET                (16)
#define RTL8224_SET_LED3_SEL0_MSB                   (31)

#define RTL8224_LED1_0_SET0_CTRL0_REG               (0xbb006548)
#define RTL8224_SET_LED0_SEL0_OFFSET                (0)
#define RTL8224_SET_LED0_SEL0_MSB                   (15)
#define RTL8224_SET_LED1_SEL0_OFFSET                (16)
#define RTL8224_SET_LED1_SEL0_MSB                   (31)

#define RTL8224_LED_PORT_SET_SEL_CTRL               (0xbb00654c)
#define RTL8224_LED0_SET_PSEL_OFFSET                (0)
#define RTL8224_LED0_SET_PSEL_FILED_LEN             (2)

/* LED Mode*/
#define PHY_8224_LED_MODE_2P5G                      (1 << 0)
#define PHY_8224_LED_MODE_2P5G_LITE                 (1 << 1)
#define PHY_8224_LED_MODE_1G                        (1 << 2)
#define PHY_8224_LED_MODE_500M                      (1 << 3)
#define PHY_8224_LED_MODE_100M                      (1 << 4)
#define PHY_8224_LED_MODE_10M                       (1 << 5)
#define PHY_8224_LED_MODE_LINK                      (1 << 6)
#define PHY_8224_LED_MODE_LINK_FLASH                (1 << 7)
#define PHY_8224_LED_MODE_ACT_FLASH                 (1 << 8)
#define PHY_8224_LED_MODE_RX_FLASH                  (1 << 9)
#define PHY_8224_LED_MODE_TX_FLASH                  (1 << 10)
#define PHY_8224_LED_MODE_COL_FLASH                 (1 << 11)
#define PHY_8224_LED_MODE_DUPLEX                    (1 << 12)
#define PHY_8224_LED_MODE_TRAINING                  (1 << 13)
#define PHY_8224_LED_MODE_MASTER                    (1 << 14)

#define RTL8224_LED_GLB_CTRL_REG                    (0xbb006520)
#define RTL8224_BLINK_TIME_SEL_OFFSET               (21)
#define RTL8224_BLINK_TIME_SEL_MSB                  (23)

#define RTL8224_LED_GLB_ACTIVE_REG                  (0xbb0065d8)
#define RTL8224_PAD_LED_ACTTIVE_OFFSET              (0)

#define RTL8224_SW_LED_LOAD                         (0xbb006550)
#define RTL8224_SW_LED_LOAD_MSB                     (0)
#define RTL8224_SW_LED_LOAD_LSB                     (0)

#define RTL8224_LED_PORT_SW_EN_CTRL                 (0xbb006554)
#define RTL8224_SW_CTRL_LED_EN_OFFSET               (0)
#define RTL8224_SW_CTRL_LED_EN_FIELD_LEN            (4)
#define RTL8224_SW_CTRL_LED_EN_MSB                  (3)

#define RTL8224_LED_PORT_SW_CTRL_PORT_OFFSET        (4)

#define RTL8224_LED_PORT_SW_CTRL                    (0xbb00655c)
#define RTL8224_SW_LED_MODE_OFFSET                  (0)
#define RTL8224_SW_LED_MODE_FIELD_LEN               (3)
#define RTL8224_SW_LED_MODE_MSB                     (2)

#define RTL8224_SW_CTRL_MODE_OFF                    (0)
#define RTL8224_SW_CTRL_MODE_LIGHT                  (7)
#define RTL8224_SW_CTRL_BLINK_256MS                 (4)

#define RTL8224_LED_GLB_IO_EN_R                     (0xbb0065dc)
#define RTL8224_LED_PAD_EN_OFFSET                   (30)

#define RTL8224_LED_GLB_MUX_1                       (0xbb0065e0)
#define RTL8224_PAD_LED2_MUX_MSB                    (17)
#define RTL8224_PAD_LED2_MUX_LSB                    (12)
#define RTL8224_PAD_LED1_MUX_MSB                    (11)
#define RTL8224_PAD_LED1_MUX_LSB                    (6)
#define RTL8224_LED_GLB_MUX_2                       (0xbb0065e4)
#define RTL8224_PAD_LED8_MUX_MSB                    (23)
#define RTL8224_PAD_LED8_MUX_LSB                    (18)
#define RTL8224_PAD_LED7_MUX_MSB                    (17)
#define RTL8224_PAD_LED7_MUX_LSB                    (12)
#define RTL8224_LED_GLB_MUX_3                       (0xbb0065e8)
#define RTL8224_LED_GLB_MUX_4                       (0xbb0065ec)
#define RTL8224_LED_GLB_MUX_5                       (0xbb0065f0)
#define RTL8224_LED_GLB_MUX_6                       (0xbb0065f4)
#define RTL8224_PAD_ALL_RESERVED_MSB                (31)
#define RTL8224_PAD_ALL_RESERVED_LSB                (0)

/* PTP*/
#define RTL8224_PTP_TIME_TOD_DELAY_R                (0xbb007c20)
#define RTL8224_TOD_DELAY_MSB                       (15)
#define RTL8224_TOD_DELAY_LSB                       (0)

#define RTL8224_PTP_TIME_OP_DURATION_R              (0xbb007c24)
#define RTL8224_TIME_OP_DURATION_MSB                (9)
#define RTL8224_TIME_OP_DURATION_LSB                (0)

#define RTL8224_PTP_OTAG_CONFIG0_R                  (0xbb007c30)
#define RTL8224_OTAG_TPID_0_MSB                     (15)
#define RTL8224_OTAG_TPID_0_LSB                     (0)

#define RTL8224_PTP_OTAG_CONFIG1_R                  (0xbb007c34)
#define RTL8224_OTAG_TPID_1_MSB                     (15)
#define RTL8224_OTAG_TPID_1_LSB                     (0)

#define RTL8224_PTP_OTAG_CONFIG2_R                  (0xbb007c38)
#define RTL8224_OTAG_TPID_2_MSB                     (15)
#define RTL8224_OTAG_TPID_2_LSB                     (0)

#define RTL8224_PTP_OTAG_CONFIG3_R                  (0xbb007c3c)
#define RTL8224_OTAG_TPID_3_MSB                     (15)
#define RTL8224_OTAG_TPID_3_LSB                     (0)

#define RTL8224_PTP_ITAG_CONFIG0_R                  (0xbb007c40)
#define RTL8224_ITAG_TPID_0_MSB                     (15)
#define RTL8224_ITAG_TPID_0_LSB                     (0)

#define RTL8224_PTP_TIME_FREQ0_R                    (0xbb007c50)
#define RTL8224_CFG_PTP_TIME_FREQ0_MSB              (15)
#define RTL8224_CFG_PTP_TIME_FREQ0_LSB              (0)

#define RTL8224_PTP_TIME_FREQ1_R                    (0xbb007c54)
#define RTL8224_CFG_PTP_TIME_FREQ1_EXEC_OFFSET      (14)
#define RTL8224_CFG_PTP_TIME_FREQ1_MSB              (12)
#define RTL8224_CFG_PTP_TIME_FREQ1_LSB              (0)

#define RTL8224_PTP_CUR_TIME_FREQ0_R                (0xbb007c58)
#define RTL8224_CUR_PTP_TIME_FREQ0_MSB              (15)
#define RTL8224_CUR_PTP_TIME_FREQ0_LSB              (0)

#define RTL8224_PTP_CUR_TIME_FREQ1_R                (0xbb007c5c)
#define RTL8224_CUR_PTP_TIME_FREQ1_MSB              (12)
#define RTL8224_CUR_PTP_TIME_FREQ1_LSB              (0)

#define RTL8224_PTP_TIME_NSEC0_R                    (0xbb007c60)
#define RTL8224_CFG_PTP_TIME_NSEC_L_MSB             (15)
#define RTL8224_CFG_PTP_TIME_NSEC_L_LSB             (0)

#define RTL8224_PTP_TIME_NSEC1_R                    (0xbb007c64)
#define RTL8224_CFG_TOD_VALID_OFFSET                (15)
#define RTL8224_CFG_PTP_TIME_NSEC_H_MSB             (13)
#define RTL8224_CFG_PTP_TIME_NSEC_H_LSB             (0)

#define RTL8224_PTP_TIME_SEC0_R                     (0xbb007c68)
#define RTL8224_CFG_PTP_TIME_SEC_L_MSB              (15)
#define RTL8224_CFG_PTP_TIME_SEC_L_LSB              (0)

#define RTL8224_PTP_TIME_SEC1_R                     (0xbb007c6c)
#define RTL8224_CFG_PTP_TIME_SEC_M_MSB              (15)
#define RTL8224_CFG_PTP_TIME_SEC_M_LSB              (0)

#define RTL8224_PTP_TIME_SEC2_R                     (0xbb007c70)
#define RTL8224_CFG_PTP_TIME_SEC_H_MSB              (15)
#define RTL8224_CFG_PTP_TIME_SEC_H_LSB              (0)

#define RTL8224_PTP_TIME_CTRL_R                     (0xbb007c74)
#define RTL8224_PTP_TIME_EXEC_OFFSET                (2)
#define RTL8224_PTP_TIME_CMD_OFFSET                 (0)
#define RTL8224_PTP_TIME_CMD_LSB                    (0)
#define RTL8224_PTP_TIME_CMD_MSB                    (1)
#define RTL8224_PTP_TIME_CMD_ADJ                    (2)
#define RTL8224_PTP_TIME_CMD_WRITE                  (1)
#define RTL8224_PTP_TIME_CMD_READ                   (0)

#define RTL8224_PTP_TIME_NSEC_RD0_R                 (0xbb007c78)
#define RTL8224_RD_PTP_TIME_NSEC_L_MSB              (15)
#define RTL8224_RD_PTP_TIME_NSEC_L_LSB              (0)

#define RTL8224_PTP_TIME_NSEC_RD1_R                 (0xbb007c7c)
#define RTL8224_RD_PTP_TIME_NSEC_H_MSB              (13)
#define RTL8224_RD_PTP_TIME_NSEC_H_LSB              (0)

#define RTL8224_PTP_TIME_SEC_RD0_R                  (0xbb007c80)
#define RTL8224_RD_PTP_TIME_SEC_L_MSB               (15)
#define RTL8224_RD_PTP_TIME_SEC_L_LSB               (0)

#define RTL8224_PTP_TIME_SEC_RD1_R                  (0xbb007c84)
#define RTL8224_RD_PTP_TIME_SEC_M_MSB               (15)
#define RTL8224_RD_PTP_TIME_SEC_M_LSB               (0)

#define RTL8224_PTP_TIME_SEC_RD2_R                  (0xbb007c88)
#define RTL8224_RD_PTP_TIME_SEC_H_MSB               (15)
#define RTL8224_RD_PTP_TIME_SEC_H_LSB               (0)

#define RTL8224_PTP_CLKOUT_NSEC0_R                  (0xbb007c8c)
#define RTL8224_CLKOUT_PTP_TIME_NSEC_L_MSB          (15)
#define RTL8224_CLKOUT_PTP_TIME_NSEC_L_LSB          (0)

#define RTL8224_PTP_CLKOUT_NSEC1_R                  (0xbb007c90)
#define RTL8224_CLKOUT_PTP_TIME_NSEC_H_MSB          (13)
#define RTL8224_CLKOUT_PTP_TIME_NSEC_H_LSB          (0)

#define RTL8224_PTP_CLKOUT_SEC0_R                   (0xbb007c94)
#define RTL8224_CLKOUT_PTP_TIME_SEC_L_MSB           (15)
#define RTL8224_CLKOUT_PTP_TIME_SEC_L_LSB           (0)

#define RTL8224_PTP_CLKOUT_SEC1_R                   (0xbb007c98)
#define RTL8224_CLKOUT_PTP_TIME_SEC_M_MSB           (15)
#define RTL8224_CLKOUT_PTP_TIME_SEC_M_LSB           (0)

#define RTL8224_PTP_CLKOUT_SEC2_R                   (0xbb007c9c)
#define RTL8224_CLKOUT_PTP_TIME_SEC_H_MSB           (15)
#define RTL8224_CLKOUT_PTP_TIME_SEC_H_LSB           (0)

#define RTL8224_PTP_CLKOUT_CTRL_R                   (0xbb007ca0)
#define RTL8224_CFG_PULSE_MODE_OFFSET               (2)
#define RTL8224_CFG_CLKOUT_EN_OFFSET                (1)
#define RTL8224_RD_CLKOUT_RUN_OFFSET                (0)

#define RTL8224_PTP_CLKOUT_HALF_PERD_NS_L_R         (0xbb007ca4)
#define RTL8224_CFG_CLKOUT_HALF_PERIOD_NS_L_MSB     (15)
#define RTL8224_CFG_CLKOUT_HALF_PERIOD_NS_L_LSB     (0)

#define RTL8224_PTP_CLKOUT_HALF_PERD_NS_H_R         (0xbb007ca8)
#define RTL8224_CFG_CLKOUT_HALF_PERIOD_NS_H_MSB     (13)
#define RTL8224_CFG_CLKOUT_HALF_PERIOD_NS_H_LSB     (0)

#define RTL8224_PTP_TIME_OP_CTRL_R                  (0xbb007cac)
#define RTL8224_CFG_GPI_OP_MSB                      (6)
#define RTL8224_CFG_GPI_OP_LSB                      (4)
#define RTL8224_CFG_GPI_RISE_TRIG_OFFSET            (3)
#define RTL8224_CFG_GPI_FALL_TRIG_OFFSET            (2)

#define RTL8224_PTP_PPS_CTRL_R                      (0xbb007cb0)
#define RTL8224_CFG_PPS_EN_OFFSET                   (6)
#define RTL8224_CFG_PPS_WIDTH_MSB                   (5)
#define RTL8224_CFG_PPS_WIDTH_LSB                   (0)

#define RTL8224_PTP_TX_TIMESTAMP_RD0_R              (0xbb007cb4)
#define RTL8224_RD_TX_TIMESTAMP_VALID_OFFSET        (15)
#define RTL8224_RD_PORT_ID_MSB                      (13)
#define RTL8224_RD_PORT_ID_LSB                      (8)
#define RTL8224_RD_MSG_TYPE_MSB                     (7)
#define RTL8224_RD_MSG_TYPE_LSB                     (6)
#define RTL8224_RD_SEQ_ID_H_MSB                     (5)
#define RTL8224_RD_SEQ_ID_H_LSB                     (0)

#define RTL8224_PTP_TX_TIMESTAMP_RD1_R              (0xbb007cb8)
#define RTL8224_RD_SEQ_ID_L_MSB                     (15)
#define RTL8224_RD_SEQ_ID_L_LSB                     (6)
#define RTL8224_RD_TX_TIMESTAMP_SEC_H_MSB           (5)
#define RTL8224_RD_TX_TIMESTAMP_SEC_H_LSB           (0)

#define RTL8224_PTP_TX_TIMESTAMP_RD2_R              (0xbb007cbc)
#define RTL8224_RD_TX_TIMESTAMP_SEC_L_MSB           (15)
#define RTL8224_RD_TX_TIMESTAMP_SEC_L_LSB           (14)
#define RTL8224_RD_TX_TIMESTAMP_NSEC_H_MSB          (13)
#define RTL8224_RD_TX_TIMESTAMP_NSEC_H_LSB          (0)

#define RTL8224_PTP_TX_TIMESTAMP_RD3_R              (0xbb007cc0)
#define RTL8224_RD_TX_TIMESTAMP_NSEC_L_MSB          (15)
#define RTL8224_RD_TX_TIMESTAMP_NSEC_L_LSB          (0)

#define RTL8224_PTP_MIB_INTR_R                      (0xbb007cc4)
#define RTL8224_RD_ISR_PTP_OFFSET                   (1)
#define RTL8224_CFG_IMR_PTP_OFFSET                  (0)

#define RTL8224_PTP_CLK_CTRL_R                      (0xbb007ccc)
#define RTL8224_CFG_CLK_SRC_OFFSET                  (0)

#define RTL8224_P0_REG_PORT_OFFSET                  (0x20)

#define RTL8224_P0_PORT_CTRL_R                      (0xbb007d20)
#define RTL8224_P0_LINK_DELAY_L_MSB                 (15)
#define RTL8224_P0_LINK_DELAY_L_LSB                 (6)
#define RTL8224_P0_CFG_ALWAYS_TS_OFFSET             (5)
#define RTL8224_P0_PTP_ROLE_MSB                     (3)
#define RTL8224_P0_PTP_ROLE_LSB                     (2)
#define RTL8224_P0_CFG_UDP_EN_OFFSET                (1)
#define RTL8224_P0_CFG_ETH_EN_OFFSET                (0)

#define RTL8224_P0_LINK_DELAY_H_R                   (0xbb007d24)
#define RTL8224_P0_LINK_DELAY_H_MSB                 (15)
#define RTL8224_P0_LINK_DELAY_H_LSB                 (0)

#define RTL8224_P0_MISC_CTRL_R                      (0xbb007d28)
#define RTL8224_P0_CFG_BYPASS_OFFSET                (0)

#define RTL8224_P0_TX_IMBAL_R                       (0xbb007d2c)
#define RTL8224_P0_TX_IMBAL_MSB                     (11)
#define RTL8224_P0_TX_IMBAL_LSB                     (0)

#define RTL8224_P0_RX_IMBAL_R                       (0xbb007d30)
#define RTL8224_P0_RX_IMBAL_MSB                     (11)
#define RTL8224_P0_RX_IMBAL_LSB                     (0)

#define RTL8224_TOD_INTR_R                          (0xbb007eb8)
#define RTL8224_ISR_TOD_OFFSET                      (1)
#define RTL8224_IMR_TOD_OFFSET                      (0)

#define RTL8224_IO_MUX_SEL_0_R                      (0xbb007f8c)
#define RTL8224_PTP_PPS_OUT_SEL_OFFSET              (22)
#define RTL8224_PTP_CLK_OUT_SEL_OFFSET              (20)


#define RTL8224_IO_MUX_SEL_1_R                      (0xbb007f90)
#define RTL8224_GLB_RLDP_LED_EN_OFFSET              (29)
#define RTL8224_GLB_SYS_LED_EN_OFFSET               (28)

/* Thermal */
#define RTL8224_TM0_CTRL2                           (0xbb0002d8)
#define RTL8224_REG_EN_LATCH_OFFSET                 (2)

#define RTL8224_TM0_CTRL3                           (0xbb0002dc)
#define RTL8224_TM_HIGHCMP_EN_OFFSET                (19)
#define RTL8224_TM_HIGH_THR_SIGN_OFFSET             (18)
#define RTL8224_TM_HIGH_THR_MSB                     (17)
#define RTL8224_TM_HIGH_THR_LSB                     (10)
#define RTL8224_TM_LOWCMP_EN_OFFSET                 (9)
#define RTL8224_TM_LOW_THR_SIGN_OFFSET              (8)
#define RTL8224_TM_LOW_THR_MSB                      (7)
#define RTL8224_TM_LOW_THR_LSB                      (0)

/* MIB */
#define RTL8224_PHY_MIB_GLOBAL_CONFIG               (0xbb0006F0)
#define RTL8224_PHY0_RX_MIB_CNTR0                   (0xbb000710)
#define RTL8224_PHY0_RX_MIB_CNTR1                   (0xbb000714)
#define RTL8224_PHY0_RX_MIB_CNTR2                   (0xbb000718)
#define RTL8224_PHY0_RX_MIB_CNTR3                   (0xbb00071c)
#define RTL8224_PHY0_TX_MIB_CNTR0                   (0xbb000730)
#define RTL8224_PHY0_TX_MIB_CNTR1                   (0xbb000734)
#define RTL8224_PHY0_TX_MIB_CNTR2                   (0xbb000738)
#define RTL8224_PHY0_TX_MIB_CNTR3                   (0xbb00073c)
#define RTL8224_PHY1_RX_MIB_CNTR0                   (0xbb000750)
#define RTL8224_PHY1_RX_MIB_CNTR1                   (0xbb000754)
#define RTL8224_PHY1_RX_MIB_CNTR2                   (0xbb000758)
#define RTL8224_PHY1_RX_MIB_CNTR3                   (0xbb00075c)
#define RTL8224_PHY1_TX_MIB_CNTR0                   (0xbb000770)
#define RTL8224_PHY1_TX_MIB_CNTR1                   (0xbb000774)
#define RTL8224_PHY1_TX_MIB_CNTR2                   (0xbb000778)
#define RTL8224_PHY1_TX_MIB_CNTR3                   (0xbb00077c)
#define RTL8224_PHY2_RX_MIB_CNTR0                   (0xbb000790)
#define RTL8224_PHY2_RX_MIB_CNTR1                   (0xbb000794)
#define RTL8224_PHY2_RX_MIB_CNTR2                   (0xbb000798)
#define RTL8224_PHY2_RX_MIB_CNTR3                   (0xbb00079c)
#define RTL8224_PHY2_TX_MIB_CNTR0                   (0xbb0007b0)
#define RTL8224_PHY2_TX_MIB_CNTR1                   (0xbb0007b4)
#define RTL8224_PHY2_TX_MIB_CNTR2                   (0xbb0007b8)
#define RTL8224_PHY2_TX_MIB_CNTR3                   (0xbb0007bc)
#define RTL8224_PHY3_RX_MIB_CNTR0                   (0xbb0007d0)
#define RTL8224_PHY3_RX_MIB_CNTR1                   (0xbb0007d4)
#define RTL8224_PHY3_RX_MIB_CNTR2                   (0xbb0007d8)
#define RTL8224_PHY3_RX_MIB_CNTR3                   (0xbb0007dc)
#define RTL8224_PHY3_TX_MIB_CNTR0                   (0xbb0007f0)
#define RTL8224_PHY3_TX_MIB_CNTR1                   (0xbb0007f4)
#define RTL8224_PHY3_TX_MIB_CNTR2                   (0xbb0007f8)
#define RTL8224_PHY3_TX_MIB_CNTR3                   (0xbb0007fc)

#define PHY_RTL8224_VER_A                           (0)
#define PHY_RTL8224_VER_B                           (1)
#define PHY_RTL8224_VER_C                           (2)

/* Top Register*/
#define PHY_8224_DBG_CTRL_VAL_REG                   (0xc0d0)
#define PHY_8224_DBG_CTRL_ADR0_REG                  (0xc0b0)
#define PHY_8224_DBG_CTRL_ADR1_REG                  (0xc0b4)
#define PHY_8224_DBG_CTRL_ADR2_REG                  (0xc0b8)
#define PHY_8224_DBG_CTRL_ADR3_REG                  (0xc0bc)

#define PHY_8224_DBG_CTRL_SEL0_REG                  (0xc0c0)
#define PHY_8224_DBG_CTRL_SEL1_REG                  (0xc0c4)
#define PHY_8224_DBG_CTRL_SEL2_REG                  (0xc0c8)
#define PHY_8224_DBG_CTRL_SEL3_REG                  (0xc0cc)
#define PHY_8224_PAD_CTRL_REG                       (0x34)


/*  SerDes Register*/
#define PHY_8224_EYE_SCAN_EN_PAGE                   (0x21)
#define PHY_8224_EYE_SCAN_EN_REG                    (0x11)
#define PHY_8224_EYE_SCAN_EN_HIGH_BIT               (0)
#define PHY_8224_EYE_SCAN_EN_LOW_BIT                (0)

#define PHY_8224_EYE_BIAS_ADJ_PAGE                  (0x2E)
#define PHY_8224_EYE_BIAS_ADJ_REG                   (0x0c)
#define PHY_8224_EYE_BIAS_ADJ_HIGH_BIT              (7)
#define PHY_8224_EYE_BIAS_ADJ_LOW_BIT               (4)

#define PHY_8224_EYE_PI_ADJ_PAGE                    (0x2E)
#define PHY_8224_EYE_PI_ADJ_REG                     (0x0D)
#define PHY_8224_EYE_PI_ADJ_HIGH_BIT                (5)
#define PHY_8224_EYE_PI_ADJ_LOW_BIT                 (2)

#define PHY_8224_EYE_PI_PHASE_PAGE                  (0x2F)
#define PHY_8224_EYE_PI_PHASE_REG                   (0x16)
#define PHY_8224_EYE_PI_PHASE_HIGH_BIT              (10)
#define PHY_8224_EYE_PI_PHASE_LOW_BIT               (5)

#define PHY_8224_EYE_REF_CTRL_PAGE                  (0x36)
#define PHY_8224_EYE_REF_CTRL_REG                   (0x10)
#define PHY_8224_EYE_REF_CTRL_HIGH_BIT              (5)
#define PHY_8224_EYE_REF_CTRL_LOW_BIT               (0)

#define PHY_8224_EYE_READ_PAGE                      (0x21)
#define PHY_8224_EYE_READ_REG                       (0x1b)
#define PHY_8224_EYE_READ_HIGH_BIT                  (7)
#define PHY_8224_EYE_READ_LOW_BIT                   (7)

#define PHY_8224_REG0_TX_PREAMP_PAGE                (0x2E)
#define PHY_8224_REG0_TX_PREAMP_REG                 (0x7)
#define PHY_8224_REG0_TX_PREAMP_HIGH_BIT            (8)
#define PHY_8224_REG0_TX_PREAMP_LOW_BIT             (3)

#define PHY_8224_REG0_TX_PREEN_PAGE                 (0x2E)
#define PHY_8224_REG0_TX_PREEN_REG                  (0x7)
#define PHY_8224_REG0_TX_PREEN_HIGH_BIT             (2)
#define PHY_8224_REG0_TX_PREEN_LOW_BIT              (2)

#define PHY_8224_REG0_TX_MAINAMP_PAGE               (0x2E)
#define PHY_8224_REG0_TX_MAINAMP_REG                (0x7)
#define PHY_8224_REG0_TX_MAINAMP_HIGH_BIT           (15)
#define PHY_8224_REG0_TX_MAINAMP_LOW_BIT            (10)

#define PHY_8224_REG0_TX_MAINEN_PAGE                (0x2E)
#define PHY_8224_REG0_TX_MAINEN_REG                 (0x7)
#define PHY_8224_REG0_TX_MAINEN_HIGH_BIT            (9)
#define PHY_8224_REG0_TX_MAINEN_LOW_BIT             (9)

#define PHY_8224_REG0_TX_POST1AMP_PAGE              (0x2E)
#define PHY_8224_REG0_TX_POST1AMP_REG               (0x6)
#define PHY_8224_REG0_TX_POST1AMP_HIGH_BIT          (15)
#define PHY_8224_REG0_TX_POST1AMP_LOW_BIT           (10)

#define PHY_8224_REG0_TX_POST1EN_PAGE               (0x2E)
#define PHY_8224_REG0_TX_POST1EN_REG                (0x6)
#define PHY_8224_REG0_TX_POST1EN_HIGH_BIT           (3)
#define PHY_8224_REG0_TX_POST1EN_LOW_BIT            (3)

#define PHY_8224_REG0_TX_Z0_PAGE                    (0x2E)
#define PHY_8224_REG0_TX_Z0_REG                     (0xb)
#define PHY_8224_REG0_TX_Z0_HIGH_BIT                (10)
#define PHY_8224_REG0_TX_Z0_LOW_BIT                 (7)

#define PHY_8224_NWAY_OPCODE_PAGE                   (0x7)
#define PHY_8224_NWAY_OPCODE_REG                    (16)
#define PHY_8224_NWAY_OPCODE_HIGH_BIT               (7)
#define PHY_8224_NWAY_OPCODE_LOW_BIT                (0)

#define PHY_8224_NWAY_OPCODE_PAGE                   (0x7)
#define PHY_8224_TGR_PRO_1_REG19                    (19)
#define PHY_8224_USXG_RXDV_OFF_HIGH_BIT             (12)
#define PHY_8224_USXG_RXDV_OFF_LOW_BIT              (12)

#define PHY_8224_AM_PERIOD_PAGE                     (0x6)
#define PHY_8224_TGR_PRO_0_REG17                    (17)
#define PHY_8224_USXG_V2P1P2_RXOFF_HIGH_BIT         (13)
#define PHY_8224_USXG_V2P1P2_RXOFF_LOW_BIT          (13)

#define PHY_8224_AM_PERIOD_PAGE                     (0x6)
#define PHY_8224_AM_PERIOD_REG                      (18)
#define PHY_8224_AM_PERIOD_HIGH_BIT                 (15)
#define PHY_8224_AM_PERIOD_LOW_BIT                  (0)

#define PHY_8224_AM_PAGE                            (0x6)
#define PHY_8224_AM_CFG0_REG                        (19)
#define PHY_8224_AM0_M0_HIGH_BIT                    (7)
#define PHY_8224_AM0_M0_LOW_BIT                     (0)
#define PHY_8224_AM0_M1_HIGH_BIT                    (15)
#define PHY_8224_AM0_M1_LOW_BIT                     (8)

#define PHY_8224_AM_PAGE                            (0x6)
#define PHY_8224_AM_CFG1_REG                        (20)
#define PHY_8224_AM0_M2_HIGH_BIT                    (7)
#define PHY_8224_AM0_M2_LOW_BIT                     (0)
#define PHY_8224_AM1_M0_HIGH_BIT                    (15)
#define PHY_8224_AM1_M0_LOW_BIT                     (8)

#define PHY_8224_AM_PAGE                            (0x6)
#define PHY_8224_AM_CFG2_REG                        (21)
#define PHY_8224_AM1_M1_HIGH_BIT                    (7)
#define PHY_8224_AM1_M1_LOW_BIT                     (0)
#define PHY_8224_AM1_M2_HIGH_BIT                    (15)
#define PHY_8224_AM1_M2_LOW_BIT                     (8)

#define PHY_8224_AM_PAGE                            (0x6)
#define PHY_8224_AM_CFG3_REG                        (22)
#define PHY_8224_AM2_M0_HIGH_BIT                    (7)
#define PHY_8224_AM2_M0_LOW_BIT                     (0)
#define PHY_8224_AM2_M1_HIGH_BIT                    (15)
#define PHY_8224_AM2_M1_LOW_BIT                     (8)

#define PHY_8224_AM_PAGE                            (0x6)
#define PHY_8224_AM_CFG4_REG                        (23)
#define PHY_8224_AM2_M2_HIGH_BIT                    (7)
#define PHY_8224_AM2_M2_LOW_BIT                     (0)
#define PHY_8224_AM3_M0_HIGH_BIT                    (15)
#define PHY_8224_AM3_M0_LOW_BIT                     (8)

#define PHY_8224_AM_PAGE                            (0x6)
#define PHY_8224_AM_CFG5_REG                        (24)
#define PHY_8224_AM3_M1_HIGH_BIT                    (7)
#define PHY_8224_AM3_M1_LOW_BIT                     (0)
#define PHY_8224_AM3_M2_HIGH_BIT                    (15)
#define PHY_8224_AM3_M2_LOW_BIT                     (8)


#define PHY_8224_NWAY_AN_PAGE                       (0x7)
#define PHY_8224_NWAY_AN_REG                        (17)
#define PHY_8224_QHSG_AN_CH3_EN_BIT                 (3)
#define PHY_8224_QHSG_AN_CH2_EN_BIT                 (2)
#define PHY_8224_QHSG_AN_CH1_EN_BIT                 (1)
#define PHY_8224_QHSG_AN_CH0_EN_BIT                 (0)

#define RTL8224_RTCT_LEN_OFFSET                     (620)
#define RTL8224_RTCT_LEN_CABLE_FACTOR               (780)

/* tapbin def bitmask */
#define RTL8224_TAPBIN_DEF_OFFSET                   (8)
#define RTL8224_TAPBIN_DEF_BITMASK                  (0x00000F00)

#define PHY_8224_RLFD_ENABLE_OFFSET                 (15)

/* value for RTK_PHY_CTRL_PTP_PORT_ROLE */
#define RTK_PHY_CTRL_PTP_PORT_ROLE_NONE      0x0 /* Non PTP port */
#define RTK_PHY_CTRL_PTP_PORT_ROLE_BC_OC     0x1 /* Boundary Clock or Ordinary Clock*/
#define RTK_PHY_CTRL_PTP_PORT_ROLE_E2E_TC    0x2 /* E2E Transparent Clock */
#define RTK_PHY_CTRL_PTP_PORT_ROLE_P2P_TC    0x3 /* P2P Transparent Clock */

/* value for RTK_PHY_CTRL_MDI_POLARITY_SWAP */
#define RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_A    0b0001
#define RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_B    0b0010
#define RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_C    0b0100
#define RTK_PHY_CTRL_MDI_POLARITY_SWAP_CH_D    0b1000

typedef enum rtk_phy_ctrl_e {
    RTK_PHY_CTRL_AN_COMPLETE,                    /* status of auto-negotiation complete, R */
    RTK_PHY_CTRL_LED_1_MODE,                     /* LED mode for LED1, RW */
    RTK_PHY_CTRL_LED_2_MODE,                     /* LED mode for LED2, RW */
    RTK_PHY_CTRL_LED_3_MODE,                     /* LED mode for LED3, RW */
    RTK_PHY_CTRL_LED_4_MODE,                     /* LED mode for LED4, RW */
    RTK_PHY_CTRL_LED_5_MODE,                     /* LED mode for LED5, RW */
    RTK_PHY_CTRL_LED_6_MODE,                     /* LED mode for LED6, RW */
    RTK_PHY_CTRL_LED_CFG_FLASH_RATE,             /* LED flash rate, RW */
    RTK_PHY_CTRL_LED_CFG_BLINK_RATE,             /* LED blink rate, RW */
    RTK_PHY_CTRL_LED_1_CFG_ACTIVE_LOW,           /* active low/high for LED1, RW */
    RTK_PHY_CTRL_LED_2_CFG_ACTIVE_LOW,           /* active low/high for LED2, RW */
    RTK_PHY_CTRL_LED_3_CFG_ACTIVE_LOW,           /* active low/high for LED3, RW */
    RTK_PHY_CTRL_LED_4_CFG_ACTIVE_LOW,           /* active low/high for LED4, RW */
    RTK_PHY_CTRL_LED_5_CFG_ACTIVE_LOW,           /* active low/high for LED5, RW */
    RTK_PHY_CTRL_LED_6_CFG_ACTIVE_LOW,           /* active low/high for LED6, RW */
    RTK_PHY_CTRL_LED_1_CFG_FORCE,                /* LED force mode for LED1, RW */
    RTK_PHY_CTRL_LED_2_CFG_FORCE,                /* LED force mode for LED2, RW */
    RTK_PHY_CTRL_LED_3_CFG_FORCE,                /* LED force mode for LED3, RW */
    RTK_PHY_CTRL_LED_4_CFG_FORCE,                /* LED force mode for LED4, RW */
    RTK_PHY_CTRL_LED_5_CFG_FORCE,                /* LED force mode for LED5, RW */
    RTK_PHY_CTRL_LED_6_CFG_FORCE,                /* LED force mode for LED6, RW */
    RTK_PHY_CTRL_NBASET_802P3BZ_MASK,            /* mask 802.3bz and advert NBASE-T only, RW*/
    RTK_PHY_CTRL_NBASET,                         /* NBASET-T enable/disable, RW */
    RTK_PHY_CTRL_NBASET_STATUS,                  /* link is operated at NBASET-T, R */
    RTK_PHY_CTRL_LOOPBACK_INTERNAL_PMA,          /* internal loopback at PMA, RW */
    RTK_PHY_CTRL_LOOPBACK_REMOTE,                /* remote loopback, RW */
    RTK_PHY_CTRL_SERDES_RX_POLARITY,             /* SerDes RX polarity, RW */
    RTK_PHY_CTRL_SERDES_TX_POLARITY,             /* SerDes TX polarity, RW */
    RTK_PHY_CTRL_MDI_POLARITY_SWAP,              /* MDI polarity swap, RW  */
    RTK_PHY_CTRL_MDI_INVERSE,                    /* MDI pin sequence inverse, RW */
    RTK_PHY_CTRL_RAPID_LINK_FAULT_DETECT,        /* rapid link fault detection enable/disable, RW */
    RTK_PHY_CTRL_PREAMBLE_RECOVERY,              /* preamble-recovery enable, RW */
    RTK_PHY_CTRL_SYNCE,                          /* SyncE enable, RW */
    RTK_PHY_CTRL_SYNCE_0_CLOCK_FREQ,             /* clock frequency for SyncE0, RW */
    RTK_PHY_CTRL_SYNCE_1_CLOCK_FREQ,             /* clock frequency for SyncE1, RW */
    RTK_PHY_CTRL_SYNCE_0_RECOVERY_PHY,           /* recovery PHY port for SyncE0, RW */
    RTK_PHY_CTRL_SYNCE_1_RECOVERY_PHY,           /* recovery PHY port for SyncE1, RW */
    RTK_PHY_CTRL_SYNCE_0_IDLE_MODE,              /* idle mode for SyncE0, RW */
    RTK_PHY_CTRL_SYNCE_1_IDLE_MODE,              /* idle mode for SyncE1, RW */
    RTK_PHY_CTRL_TEMP,                           /* read temperature from thermal meter(bit[18]:sign; [17:10]:integer; [9:0]:decimal), R*/
    RTK_PHY_CTRL_TEMP_THRESHOLD_HIGH_LOWER,      /* thermal high threshold, RW([18]:sign; [17:10]:integer(degrees Celsius); [9:0]:decimal), for trigger interrupt when temperature lower then this threshold */
    RTK_PHY_CTRL_TEMP_THRESHOLD_HIGH_HIGHER,     /* thermal high threshold, RW([18]:sign; [17:10]:integer(degrees Celsius); [9:0]:decimal), for trigger interrupt when temperature higher then this threshold */
    RTK_PHY_CTRL_TEMP_THRESHOLD_LOW_LOWER,       /* thermal  low threshold, RW([18]:sign; [17:10]:integer(degrees Celsius); [9:0]:decimal), for trigger interrupt when temperature lower then this threshold */
    RTK_PHY_CTRL_TEMP_THRESHOLD_LOW_HIGHER,      /* thermal  low threshold, RW([18]:sign; [17:10]:integer(degrees Celsius); [9:0]:decimal), for trigger interrupt when temperature higher then this threshold */
    RTK_PHY_CTRL_FAST_RETRAIN,                   /* enable fast retrain ability, RW */
    RTK_PHY_CTRL_FAST_RETRAIN_LPCOUNT,           /* Counts the number of fast retrains requested by the link partner, R */
    RTK_PHY_CTRL_FAST_RETRAIN_LDCOUNT,           /* Counts the number of fast retrains requested by the local device, R */
    RTK_PHY_CTRL_FAST_RETRAIN_STATUS,            /* fast retrain capable at current link(1 for Both local and link-partner have fast retrain capability of current speed), R */
    RTK_PHY_CTRL_FAST_RETRAIN_NFR,               /* enable NFR ability, RW */
    RTK_PHY_CTRL_FAST_RETRAIN_NFR_STATUS,        /* NFR capable at current link, R */
    RTK_PHY_CTRL_COUNTER_CLEAR,                  /* clear counter, W */
    RTK_PHY_CTRL_SERDES_MODE,                    /* config SerDes mode, RW */
    RTK_PHY_CTRL_SERDES_UPDTAE,                  /* update SerDes setting for those modes can't auto change speed when link up(do this after MAC side serdes mode change), W */
    RTK_PHY_CTRL_SERDES_USXGMII_AN,              /* SerDes auto-negotiation enable(1)/disable(0) for USXGMII mode, RW */
    RTK_PHY_CTRL_SERDES_SGMII_AN,                /* SerDes auto-negotiation enable(1)/disable(0) for SGMII mode, RW */
    RTK_PHY_CTRL_SERDES_BASEX_AN,                /* SerDes auto-negotiation enable(1)/disable(0) for 1000BASE-X/2.5GBASE-X/5GBASE-X mode, RW */
    RTK_PHY_CTRL_SERDES_XSGMII_AN,               /* SerDes auto-negotiation enable(1)/disable(0) for XSGMII mode, RW */
    RTK_PHY_CTRL_SNR_CH0,                        /* Get current SNR value(MSE) of Channel 0, R */
    RTK_PHY_CTRL_SNR_CH1,                        /* Get current SNR value(MSE) of Channel 1, R */
    RTK_PHY_CTRL_SNR_CH2,                        /* Get current SNR value(MSE) of Channel 2, R */
    RTK_PHY_CTRL_SNR_CH3,                        /* Get current SNR value(MSE) of Channel 3, R */
    RTK_PHY_CTRL_SNR_THRESHOLD_10G_MASTER,       /* SNR threshold(MSE) for 10G Master, RW */
    RTK_PHY_CTRL_SNR_THRESHOLD_10G_SLAVE,        /* SNR threshold(MSE) for 10G Slave, RW */
    RTK_PHY_CTRL_SNR_THRESHOLD_5G_MASTER,        /* SNR threshold(MSE) for 5G Master, RW */
    RTK_PHY_CTRL_SNR_THRESHOLD_5G_SLAVE,         /* SNR threshold(MSE) for 5G Slave, RW */
    RTK_PHY_CTRL_SNR_THRESHOLD_2P5G_MASTER,      /* SNR threshold(MSE) for 2.5G Master, RW */
    RTK_PHY_CTRL_SNR_THRESHOLD_2P5G_SLAVE,       /* SNR threshold(MSE) for 2.5G Slave, RW */
    RTK_PHY_CTRL_SNR_THRESHOLD_1G,               /* SNR threshold(MSE) for 1G/100M, RW */
    RTK_PHY_CTRL_REINIT,                         /* Set 1 to reset and reinit port, W */
    RTK_PHY_CTRL_SKEW_PAIR_B,                    /* Get current skew delay on pair B with respect to physical pair A*/
    RTK_PHY_CTRL_SKEW_PAIR_C,                    /* Get current skew delay on pair B with respect to physical pair A*/
    RTK_PHY_CTRL_SKEW_PAIR_D,                    /* Get current skew delay on pair B with respect to physical pair A*/
    RTK_PHY_CTRL_SERDES_LOOPBACK_REMOTE,         /* remote loopback on SerDes, RW */
    RTK_PHY_CTRL_RTCT_CABLE_TYPE,                /* RTCT cable type, W */
    RTK_PHY_CTRL_RTCT_CABLE_FACTOR,              /* RTCT cable factor value to caculate cable length, set 0 to use driver default value, RW */
    RTK_PHY_CTRL_SERDES_EYE_PARAM_TYPE,          /* serdes mode target for rtk_port_phySdsEyeParam_set/get API, RW */
    RTK_PHY_CTRL_PTP,                            /* PTP enable, RW */
    RTK_PHY_CTRL_PTP_DURATION_THRESHOLD,         /* PTP duration threahold(ns) for trigger time operation (falling edge and rising edge), RW */
    RTK_PHY_CTRL_PTP_REFTIME_TOD_DELAY,          /* PTP ToD delay(ns) for reference time in 16-bit 2's complement format, RW */
    RTK_PHY_CTRL_PTP_PORT_ROLE,                  /* PTP port role, RW */
    RTK_PHY_CTRL_PTP_TX_IMBAL,                   /* PTP TX IMBAL(ns), egressTimestamp = egressMeasuredTimestamp + TX_IMBAL, RW */
    RTK_PHY_CTRL_PTP_RX_IMBAL,                   /* PTP RX IMBAL(ns), ingressTimestamp = ingressMeasuredTimestamp - RX_IMBAL, RW */
    RTK_PHY_CTRL_PTP_CLOCK_SRC,                  /* PTP clock source select external/internal clock, RW */
    RTK_PHY_CTRL_LINKDOWN_CNT,                   /* get link down counter (read clear), R */
    RTK_PHY_CTRL_FATAL_STATUS,                   /* get fatal error status bitmap, R */
    RTK_PHY_CTRL_FATAL_STATUS_READ_CLEAR,        /* get fatal error status bitmap(read clear), RC */
    RTK_PHY_CTRL_PTP_PLL_POW_SRC,                /* PTP PLL power source select, RW*/
    RTK_PHY_CTRL_PTP_PLL_CLK,                    /* PTP PLL clock frequence select, RW*/
    RTK_PHY_CTRL_PTP_PLL,                        /* PTP PLL enable, RW*/
    RTK_PHY_CTRL_EEE_PHYMODE,                    /* EEE PHY-mode enable, RW */
    RTK_PHY_CTRL_MACSEC_BYPASS,                  /* Set 1 to bypass MACsec, RW */
    RTK_PHY_CTRL_SERDES_RX_RESET,                /* Reset SerDes RX, W */
    RTK_PHY_CTRL_SERDES_TX_RESET,                /* Reset SerDes TX, W */
    /* Private/Debug options */
    RTK_PHY_CTRL_DEBUG_DUMP_DSP,                 /* used by rtk_phy_debug_get */
    RTK_PHY_CTRL_DEBUG_DUMP_DSP_INIT,            /* used by rtk_phy_debug_get */
    RTK_PHY_CTRL_DEBUG_DUMP_COUPLING,            /* used by rtk_phy_debug_get */
    RTK_PHY_CTRL_DEBUG_MDI_PLUG,                 /* get MDI link status, R */
    RTK_PHY_CTRL_DEBUG_MDIO_PARITY_CHK,          /* get debug conuter for MDIO parity check, R */
    RTK_PHY_CTRL_MIIM_BCAST,                     /* enable/disable miim broadcast, W*/
    RTK_PHY_CTRL_MIIM_BCAST_PHYAD,               /* set miim broadcast PHYAD, W*/
    RTK_PHY_CTRL_SYNCE_PLL,                      /* SyncE output clock source from PLL, W */
    RTK_PHY_CTRL_IPG_SHRINK,                     /* Enable PHY's IPG shrink when switch enables stacking on the port */
    RTK_PHY_CTRL_SERDES_USXGMII_AM_PERIOD,       /* get/set SerDes AM period RW */
    RTK_PHY_CTRL_END
} rtk_phy_ctrl_t;

typedef enum rt_serdesMode_e
{
    RTK_MII_NONE = 0,
    RTK_MII_DISABLE,
    RTK_MII_10GR,
    RTK_MII_RXAUI,
    RTK_MII_RXAUI_LITE,
    RTK_MII_RXAUISGMII_AUTO,
    RTK_MII_RXAUI1000BX_AUTO,
    RTK_MII_RSGMII_PLUS,
    RTK_MII_SGMII,
    RTK_MII_QSGMII,
    RTK_MII_1000BX_FIBER,
    RTK_MII_100BX_FIBER,
    RTK_MII_1000BX100BX_AUTO,
    RTK_MII_10GR1000BX_AUTO,
    RTK_MII_10GRSGMII_AUTO,
    RTK_MII_XAUI,
    RTK_MII_RMII,
    RTK_MII_SMII,
    RTK_MII_SSSMII,
    RTK_MII_RSGMII,
    RTK_MII_XSMII,
    RTK_MII_RS8MII = RTK_MII_XSMII,     /* RTK_MII_RS8MII is obsoleted */
    RTK_MII_XSGMII,
    RTK_MII_QHSGMII,
    RTK_MII_HISGMII,
    RTK_MII_HISGMII_5G,
    RTK_MII_DUAL_HISGMII,
    RTK_MII_2500Base_X,
    RTK_MII_RXAUI_PLUS,
    RTK_MII_USXGMII_10GSXGMII,
    RTK_MII_USXGMII_10GDXGMII,
    RTK_MII_USXGMII_10GQXGMII,
    RTK_MII_USXGMII_5GSXGMII,
    RTK_MII_USXGMII_5GDXGMII,
    RTK_MII_USXGMII_2_5GSXGMII,
    RTK_MII_USXGMII_1G,
    RTK_MII_USXGMII_100M,
    RTK_MII_USXGMII_10M,
    RTK_MII_5GBASEX,
    RTK_MII_5GR,
    RTK_MII_XFI_5G_ADAPT,
    RTK_MII_XFI_5G_CPRI,
    RTK_MII_XFI_2P5G_ADAPT,
    RTK_MII_QUSGMII,
    RTK_MII_OUSGMII,
    RTK_MII_END,
}rt_serdesMode_t;
#define rtk_serdesMode_t            rt_serdesMode_t

typedef enum rtk_port_linkStatus_e
{
    PORT_LINKDOWN = 0,
    PORT_LINKUP,
    PORT_LINKSTATUS_END
} rtk_port_linkStatus_t;

typedef struct rtk_phy_macIntfSdsLinkStatus_s
{
    rtk_port_linkStatus_t   link_status[2];      /* Link status, index is serdes id */
    int                   sds_num;                                    /* number of SerDes link status is availible in link_status */
} rtk_phy_macIntfSdsLinkStatus_t;

void ca_ni_rtl8224_phy_init(u32 qxgmii_port_base_phy_addr);

