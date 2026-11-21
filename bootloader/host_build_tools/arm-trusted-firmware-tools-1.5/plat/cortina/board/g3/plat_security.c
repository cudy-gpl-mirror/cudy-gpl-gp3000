/*
 * Copyright (c) 2015-2016, Cortina Access. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <debug.h>
#include <stdint.h>
#include <strings.h>
#include <platform_def.h>
#include INC_CORTINA_SOC_REGS_HEADER


/*
 * Configure secure memory region using G3 DRAM Trusted Access Controller
 * region_size must be at least 64KB and a multple of 64KB
 * region_base must be 64KB aligned
 */
static void g3_dram_tac_protect(int region_number, uint32_t region_base, uint32_t region_size)
{
	/* temp host based unioned structures to G3 registers of interest */
	DDR_CTRL_DDR_SCH_SECURED_DDR_REGION0_t g3_ddr_tac_region_reg;
	DDR_CTRL_DDR_SCH_SECURED_DDR_REGION0_EN_t g3_ddr_tac_region_enable_reg; 

	/* determine the address to the right region, g3 has eight configurable regions 0-7 */
	/* this is an address, NOT a pointer */
	uint64_t aG3_ddr_tac_region = 
		DDR_CTRL_DDR_SCH_SECURED_DDR_REGION0 + (region_number * 8);

	uint64_t aG3_ddr_tac_region_enable =
		DDR_CTRL_DDR_SCH_SECURED_DDR_REGION0_EN + (region_number * 8);

	if ((region_size & 0xFFFF) || region_size < 0x10000) {
		ERROR("Secure region size is not a multiple of 64KB or zero\n");
		return;
	}
	if (region_base & 0xFFFF) {
		ERROR("Secure region base address is not 64KB aligned\n");
		return;
	}

	INFO("BL2: TrustZone: protecting %u bytes of memory at 0x%x\n", region_size,
	     region_base);

	/* first disable the region */
	g3_ddr_tac_region_enable_reg.wrd = CA_REG_READ(aG3_ddr_tac_region_enable);
        g3_ddr_tac_region_enable_reg.bf.write = 0;
        g3_ddr_tac_region_enable_reg.bf.read = 0;
	CA_REG_WRITE(aG3_ddr_tac_region_enable, g3_ddr_tac_region_enable_reg.wrd);

	/* base_addr is only the top 16 bits of address, block is always 64KB aligned */ 
	g3_ddr_tac_region_reg.bf.base_addr = region_base >>16;

	/* limit_addr is only the top 16 bits of address, 
         * block always ends in 0xffff in lower 16 bits and
	 * therefore an entire 64KB region. For example,
         * base and limit can both the be the same value, this would represent a
         * total block region size of 64KB */ 
	g3_ddr_tac_region_reg.bf.limit_addr = (region_base + region_size - 1)>>16;

	CA_REG_WRITE(aG3_ddr_tac_region, g3_ddr_tac_region_reg.wrd);

	/* finally enable both read and write protection */
	g3_ddr_tac_region_enable_reg.bf.write = 1;
	g3_ddr_tac_region_enable_reg.bf.read = 1;
	CA_REG_WRITE(aG3_ddr_tac_region_enable, g3_ddr_tac_region_enable_reg.wrd);


}

/*******************************************************************************
 * Initialize the secure environment.
 ******************************************************************************/
void plat_security_setup(void)
{
	/* Map Secure DRAM via G3 Trusted Access Controller Region 0 */
	if(G3_USE_DRAM_TAC)
	g3_dram_tac_protect(0, DRAM_SEC_BASE, DRAM_SEC_SIZE);
}
