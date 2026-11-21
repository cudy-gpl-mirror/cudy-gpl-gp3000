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

#ifndef __PARTITIONS_H__
#define __PARTITIONS_H__

#include "plat_io_storage.h"

#define MAX_PARTITION_NUM		128
#define EFI_NAMELEN			36

typedef struct 
{
	unsigned int	start;
	unsigned int	length;
	unsigned int	flags;
	unsigned int	loadaddr;
	unsigned int	loadsize;
	int		id;
	uint64_t	attr;
	uint8_t		type_uuid[16];
	char		name[EFI_NAMELEN];
} ptentry_t;

#define GPT_ATTR_SYS_PARTITION	0x0000000000000001ULL
#define GPT_ATTR_HIDE_EFI		0x0000000000000002ULL
#define GPT_ATTR_LEGACY_BOOT	0x0000000000000004ULL
#define GPT_ATTR_READONLY		0x1000000000000000ULL
#define GPT_ATTR_FIP_ACTIVE		0x0001000000000000ULL
#define GPT_ATTR_HIDDEN			0x4000000000000000ULL
#define GPT_ATTR_NOT_AUTOMNT	0x8000000000000000ULL

int get_partition(uint8_t boot_device, uint8_t boot_partition, plat_io_policy_t * policy);
ptentry_t * find_fip_partition(uint8_t boot_partition);

#endif /* __PARTITIONS_H__ */

