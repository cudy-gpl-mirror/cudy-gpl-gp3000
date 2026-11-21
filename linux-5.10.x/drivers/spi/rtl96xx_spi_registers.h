/*
 * REGISTERS.h
 *
 *
 */
#ifndef _RTL96XX_SPI_REGISTERS_H_
#define _RTL96XX_SPI_REGISTERS_H_

//#include "ca_types.h"

typedef unsigned int ca_uint32_t;

#ifndef __ASSEMBLER__
typedef volatile union {
	struct {
		ca_uint32_t command_cyc          :1; /* bits 0:0 */
		ca_uint32_t read_write           :1; /* bits 1:1 */
		ca_uint32_t micro_wire_cs_sel    :5; /* bits 6:2 */
		ca_uint32_t rsrvd1               :1;
		ca_uint32_t sel_ssp_cs           :5; /* bits 12:8 */
		ca_uint32_t rsrvd2               :3;
		ca_uint32_t ssp_cmd_cnt          :7; /* bits 22:16 */
		ca_uint32_t rsrvd3               :1;
		ca_uint32_t pre_ssp_dat_cnt      :6; /* bits 29:24 */
		ca_uint32_t sspcs_en3_gpio       :1; /* bits 30:30 */
		ca_uint32_t sspcs_en4_gpio       :1; /* bits 31:31 */
	} bf;
	ca_uint32_t     wrd;
} PER_SPI_CFG_t;
#endif /* !__ASSEMBLER__ */

#ifndef __ASSEMBLER__
typedef volatile union {
	struct {
		ca_uint32_t sspdone              :1; /* bits 0:0 */
		ca_uint32_t rsrvd1               :6;
		ca_uint32_t sspstart             :1; /* bits 7:7 */
		ca_uint32_t rsrvd2               :24;
	} bf;
	ca_uint32_t     wrd;
} PER_SPI_CTRL_t;
#endif /* !__ASSEMBLER__ */

#ifndef __ASSEMBLER__
typedef volatile union {
	struct {
		ca_uint32_t counter_load         :16; /* bits 15:0 */
		ca_uint32_t ssp_igap             :8; /* bits 23:16 */
		ca_uint32_t micro_wire_cs_sel_extend :3; /* bits 26:24 */
		ca_uint32_t sel_ssp_cs_extend    :3; /* bits 29:27 */
		ca_uint32_t rsrvd1               :2;
	} bf;
	ca_uint32_t     wrd;
} PER_SPI_CLK_t;
#endif /* !__ASSEMBLER__ */

#endif
