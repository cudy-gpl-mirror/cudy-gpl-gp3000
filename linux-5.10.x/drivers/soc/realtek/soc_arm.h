/* SPDX-License-Identifier: GPL-2.0 */

#ifdef CONFIG_REALTEK_BOARD_FPGA_V8
#define IS_FPGA
#endif

extern u64 ca_smc_call(unsigned int smc_id, unsigned long x1, unsigned long x2,
		unsigned long x3, unsigned long x4);
