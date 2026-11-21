/*
 * Copyright (c) Cortina-Access Limited 2015.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * aal_common.h:
 *
 */
#ifndef __AAL_COMMON_H__
#define __AAL_COMMON_H__

/************************************************************************
**                   AAL table access opeartion
*************************************************************************/
//reg operation, please modify it to uboot
#define REG_READ(r)                                   *(volatile unsigned int *) (r)
#define REG_WRITE(r, v)                               *(volatile unsigned int*) (r) = (v)

///-- For resolve conflict definetion with taurus.h
#if 0
#define RTK_NE_REG_READ(REG)         REG_READ((int)REG)
#define RTK_NE_REG_WRITE(REG, REG_DATA) REG_WRITE((int)REG, REG_DATA)
#else
#define RTK_NE_REG_READ(REG)         CA_REG_READ(REG)
#define RTK_NE_REG_WRITE(REG, REG_DATA) CA_REG_WRITE(REG_DATA, REG)
#endif

#define CA_NE_REG_READ(REG)   RTK_NE_REG_READ(REG)
#define CA_NE_REG_WRITE(REG_DATA, REG) RTK_NE_REG_WRITE(REG, REG_DATA)

#define FIND_INDIRCT_ADDRESS(rbw, index)    (((ca_uint32_t)1<<31)|((ca_uint32_t)(rbw)<<30)|(ca_uint32_t)(index))
#define CHECK_INDIRCT_OPERATE_STATE(dev,addr)    do{\
                                                    ca_uint32_t i=0;\
                                                    ca_uint32_t val32;\
                                                    for(i = 0; i<10000; i++){\
                                                       val32 = CA_NE_REG_READ((ca_uint32_t)addr);\
                                                       if((val32 & ((ca_uint32)1 <<31)) == 0){\
                                                        break;\
                                                        }\
                                                    }\
                                                    if (i>= 10000) \
                                                        printk("%s(): access %s timeout.\n", __func__, #addr);\
                                                }while(0)


#define DO_INDIRCT_OP(dev,rbw, indx, raddr) do { CA_NE_REG_WRITE(FIND_INDIRCT_ADDRESS(rbw, indx), raddr); \
                                  CHECK_INDIRCT_OPERATE_STATE(dev,raddr); } while(0)

#define WRITE_INDIRCT_REG(dev,index,addr) DO_INDIRCT_OP(dev,1,index,addr)
#define READ_INDIRCT_REG(dev,index,addr) DO_INDIRCT_OP(dev,0,index,addr)

#define REG_L2FE_FIELD_READ(reg, field, val)             do{\
                                                         reg##_t reg_data;\
                                                         reg_data.wrd = CA_NE_REG_READ(reg);\
                                                         (val) = reg_data.bf.field;\
                                                         }while(0)

#define REG_L2FE_FIELD_WRITE(reg, field, val)            do{\
                                                         reg##_t reg_data;\
                                                         reg_data.wrd = CA_NE_REG_READ(reg);\
                                                         reg_data.bf.field = (val);\
                                                         CA_NE_REG_WRITE(reg_data.wrd, reg);\
                                                         }while(0)

#define CA_DEV_WRITE_INDIRCT_REG(dev,index,addr) DO_INDIRCT_OP(dev,1,index,addr)
#define CA_DEV_READ_INDIRCT_REG(dev,index,addr)  DO_INDIRCT_OP(dev,0,index,addr)

#define CA_L2FE_REG_READ(addr)            CA_NE_REG_READ(addr)
#define CA_L2FE_REG_WRITE(data, addr)     CA_NE_REG_WRITE(data, addr)

#define CA_DEV_NI_REG_READ(dev,addr)             CA_NE_REG_READ(addr)
#define CA_DEV_NI_REG_WRITE(dev,data,addr)       CA_NE_REG_WRITE(data, addr)
#define CA_DEV_NI_REG_WRITE2(dev,addr, data)     CA_NE_REG_WRITE(data, addr)
#define CA_DEV_NE_REG_READ(dev,addr)             CA_NE_REG_READ(addr)
#define CA_DEV_NE_REG_WRITE(dev,data,addr)       CA_NE_REG_WRITE(data, addr)
#define CA_DEV_NE_REG_WRITE(dev,data,addr)       CA_NE_REG_WRITE(data, addr)
#define CA_DEV_NE_REG_WRITE2(dev,addr,data)      CA_NE_REG_WRITE(data, addr)

#define CA_NI_REG_READ(addr)            RTK_NE_REG_READ(addr)
#define CA_NI_REG_WRITE(data, addr)        RTK_NE_REG_WRITE(addr, data)


//g_dma_reg_iobase 0xf7001000
#define AAL_DMA_REG_ADDR(addr)      (0xf7001000 + addr)
#define AAL_GLB_REG_ADDR(addr)      ((addr & 0xfff) + 0xf4320000)
#define AAL_PER_MDIO_REG_ADDR(addr) (addr - PER_MDIO_CFG + 0xf4329118)

//reorder
//extern void *g_ne_axi_reo_reg_iobase;
//void aal_ne_axi_reo_reg_base_init(void *iobase);
#define AAL_NE_AXI_REO_REG_ADDR(addr)     ((((ca_uint32_t)(addr)) - QM_AXI_REO_AXI_REO_RD_ORIG_ID) + 0xf432d000)
#define CA_NE_AXI_REO_REG_READ(addr)      RTK_NE_REG_READ((void *)AAL_NE_AXI_REO_REG_ADDR(addr))
#define CA_NE_AXI_REO_REG_WRITE(data, addr)     RTK_NE_REG_WRITE((void *)AAL_NE_AXI_REO_REG_ADDR(addr), data)


//phy
//extern void *g_ne_phy_reg_iobase; //0xf4600000
//void aal_ne_phy_reg_base_init(void *iobase);
#define AAL_NE_PHY_REG_ADDR(addr)      ((((ca_uint32_t)(addr)) & 0x00000FFF) + 0xf4600000)
#define CA_NE_PHY_REG_READ(addr)    RTK_NE_REG_READ((void *)AAL_NE_PHY_REG_ADDR(addr))
#define CA_NE_PHY_REG_WRITE(data, addr)      RTK_NE_REG_WRITE((void *)AAL_NE_PHY_REG_ADDR(addr), data)
#define AAL_NE_OCP_REG_ADDR(addr)      ((((ca_uint32_t)(addr)) & 0x000FFFFC) + 0xf4600000)
#define CA_NE_OCP_REG_READ(addr)    RTK_NE_REG_READ((void *)AAL_NE_OCP_REG_ADDR(addr))
#define CA_NE_OCP_REG_WRITE(data, addr)      RTK_NE_REG_WRITE((void *)AAL_NE_OCP_REG_ADDR(addr), data)

//extern void *g_ne_gphy_wrap_reg_iobase; //0xf4700000
//void aal_ne_gphy_wrap_reg_base_init(void *iobase);
#define AAL_NE_GPHY_WRAP_REG_ADDR(addr)                 ((((ca_uint32_t)(addr)) - WRAP_GPHY_CTRL0) + 0xf4700000)
#define CA_NE_GPHY_WRAP_REG_READ(addr)                  RTK_NE_REG_READ((void *)AAL_NE_GPHY_WRAP_REG_ADDR(addr))
#define CA_NE_GPHY_WRAP_REG_WRITE(data, addr)           RTK_NE_REG_WRITE((void *)AAL_NE_GPHY_WRAP_REG_ADDR(addr), data)

#define CA_PER_MDIO_REG_READ(addr)        RTK_NE_REG_READ((void *)AAL_PER_MDIO_REG_ADDR(addr))
#define CA_PER_MDIO_REG_WRITE(data, addr)    RTK_NE_REG_WRITE((void *)AAL_PER_MDIO_REG_ADDR(addr), data)

//sds
//extern void *g_ne_sds_reg_iobase;
#define AAL_NE_SDS_REG_ADDR(addr)      ((((ca_uint32_t)(addr)) & 0x0000FFFF) + 0xf4330000)
#define CA_NE_SDS_REG_READ(addr)    RTK_NE_REG_READ((void *)AAL_NE_SDS_REG_ADDR(addr))
#define CA_NE_SDS_REG_WRITE(data, addr)      RTK_NE_REG_WRITE(data, (void *)AAL_NE_SDS_REG_ADDR(addr))

//pon glb
//extern void *ca_venus_pon_glb_reg_iobase;
#define AAL_PON_GLB_REG_ADDR(reg)    (volatile void *)((((ca_uint32_t)(reg)) & 0x0000FFFF) + 0xf5500000)
#define AAL_GLB_REG_READ(reg)            RTK_NE_REG_READ(AAL_PON_GLB_REG_ADDR(reg))
#define AAL_GLB_REG_WRITE(data, reg)     RTK_NE_REG_WRITE(data, AAL_PON_GLB_REG_ADDR(reg))                                                    

//others
#define AAL_CHIP_REVISION_A   'A'
#define AAL_CHIP_REVISION_B   'B'
#define AAL_CHIP_REVISION_C     'C'


#ifndef AAL_ASSERT_RET
#define AAL_ASSERT_RET(x, ret) \
   do { \
      if (!(x)) { \
         PRINTF("%s() _%d_: assert fail\n", __FUNCTION__, __LINE__); \
         return ret; \
      } \
   } while (0)
#endif

typedef enum {
   AAL_TBL_OP_FLAG_RD = 0,
   AAL_TBL_OP_FLAG_WR,
} aal_tbl_op_flag_t;

#define AAL_TBL_OP_TIMEOUT 0xfff


#endif /* __AAL_COMMON_H__ */