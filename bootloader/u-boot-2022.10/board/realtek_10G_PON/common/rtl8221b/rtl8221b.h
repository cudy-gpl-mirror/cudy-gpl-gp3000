
#define PHY_MMD_PMAPMD              1
#define PHY_MMD_PCS                 3
#define PHY_MMD_AN                  7
#define PHY_MMD_VEND1               30   /* Vendor specific 1 */
#define PHY_MMD_VEND2               31   /* Vendor specific 2 */

#define BIT_0 (1<<0)
#define BIT_1 (1<<1)
#define BIT_2 (1<<2)
#define BIT_3 (1<<3)
#define BIT_4 (1<<4)
#define BIT_5 (1<<5)
#define BIT_6 (1<<6)
#define BIT_7 (1<<7)
#define BIT_8 (1<<8)
#define BIT_9 (1<<9)
#define BIT_10 (1<<10)
#define BIT_11 (1<<11)
#define BIT_12 (1<<12)
#define BIT_13 (1<<13)
#define BIT_14 (1<<14)
#define BIT_15 (1<<15)

#define DEFAULT_SERDES_MODE	0x1

typedef struct
{
    u16 dev;
    u16 addr;
    u16 value;
} RTL8221B_MMD_REG;

#define NO_LINK 0
#define LINK_SPEED_10M 10
#define LINK_SPEED_100M 100
#define LINK_SPEED_500M 500
#define LINK_SPEED_1G 1000
#define LINK_SPEED_2P5G 2500

typedef enum
{
    PHY_SERDES_MODE_OTHER = 0,
    PHY_SERDES_MODE_SGMII,
    PHY_SERDES_MODE_HiSGMII,
    PHY_SERDES_MODE_2500BASEX,
    PHY_SERDES_MODE_USXGMII,
    PHY_SERDES_MODE_NO_SDS,
    PHY_SERDES_MODE_END
} PHY_SERDES_MODE;

void ca_ni_rtl8221_phy_init(u32 phy_addr);

