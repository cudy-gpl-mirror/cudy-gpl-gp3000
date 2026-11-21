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
#ifndef __CA_AAL_PROC_H__
#define __CA_AAL_PROC_H__

enum CA_AAL_DEBUG {
	AAL_DBG_DIASBLE		= 0,
	AAL_DBG_NI		= 0x00000001,
	AAL_DBG_L3QM		= 0x00000002,
	AAL_DBG_L3FE		= 0x00000004,
	AAL_DBG_L3CLS		= 0x00000008,
	AAL_DBG_HASHLITE	= 0x00000010,
	AAL_DBG_HASH		= 0x00000020,
	AAL_DBG_MCAST		= 0x00000040,
	AAL_DBG_DMA_LSO		= 0x00000080,
	AAL_DBG_XFI             = 0x00000100,
	AAL_DBG_PSDS            = 0x00000200,
	AAL_DBG_PHY             = 0x00000400,
	AAL_DBG_MDIO            = 0x00000800,
	AAL_DBG_SCFG            = 0x00001000,
	AAL_DBG_NI_L2           = 0x00002000,
	AAL_DBG_TABLE_RAW       = 0x00004000,
	AAL_DBG_DMA_LSO_INTERRUPT       = 0x00008000,
	AAL_DBG_NE_GLB_INTERRUPT       = 0x00010000,
};

extern uint32 ca_aal_debug;
#if 0
int ca_aal_proc_init(void);
void ca_aal_proc_exit(void);

int dump_headera_show(struct seq_file *s, void *v);
void aal_debug_set(uint32_t aal_debug);

int dump_dropcount_show(struct seq_file *s, void *v);
ssize_t dump_dropcount_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos);
#endif

#endif /* __CA_AAL_PROC_H__ */
