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

#include <platform_def.h>
#include <psci.h>

unsigned int plat_get_aff_count(unsigned int aff_lvl, unsigned long mpidr)
{
	/* Report 1 (absent) instance at levels higher that the cluster level */
	if (aff_lvl > MPIDR_AFFLVL1)
		return 1;

	if (aff_lvl == MPIDR_AFFLVL1)
		return 1; /* We have one clusters */

	return 4; /* 4 cpus in cluster 0 */
}

unsigned int plat_get_aff_state(unsigned int aff_lvl, unsigned long mpidr)
{
	return aff_lvl <= MPIDR_AFFLVL1 ? PSCI_AFF_PRESENT : PSCI_AFF_ABSENT;
}

int plat_get_max_afflvl()
{
	return MPIDR_AFFLVL1;
}

int plat_setup_topology()
{
	/* Juno todo: Make topology configurable via SCC */
	return 0;
}
