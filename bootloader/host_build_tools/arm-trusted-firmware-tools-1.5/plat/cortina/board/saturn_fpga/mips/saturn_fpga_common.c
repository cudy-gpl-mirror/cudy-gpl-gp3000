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

#include <assert.h>
#include <bl_common.h>
#include <debug.h>
#include <mmio.h>
#include <platform.h>
#include <platform_def.h>
#include INC_PLAT_DEF_HEADER
#include INC_PLAT_PRIVATE_HEADER
#include INC_CORTINA_SOC_REGS_HEADER


unsigned long long plat_get_syscnt_freq(void)
{
    return plat_get_cpu_clock_mhz() * 1000000;
}

uintptr_t plat_get_ns_image_entrypoint(void)
{
    return (uintptr_t) NS_IMAGE_OFFSET;
}

/* ------------------------------------------
 * Clean+Invalidate the entire d-cache
 * no arguments are passed
 * ------------------------------------------
 */
void flush_dcache_all(void)
{
  uint32_t cctl0_tmp;

  /* wait for data pipeline to settle */
  __sync();

  cctl0_tmp = read_c0_cctl0(); 

  /* operation is rising edge triggered so we first need
     to clear the bit and then set it to trigger operation */
 if ((cctl0_tmp & CCTL0_DWBInv) != 0)
  {
  cctl0_tmp &= ~CCTL0_DWBInv;
  write_c0_cctl0(cctl0_tmp);
  }

  /* now set the bit to trigger rising edge */
  cctl0_tmp |= CCTL0_DWBInv;
  write_c0_cctl0(cctl0_tmp);
  
}

/* ------------------------------------------
 * Clean entire d-cache
 * no arguments are passed
 * ------------------------------------------
 */
void clean_dcache_all(void)
{
  uint32_t cctl0_tmp;

  /* wait for data pipeline to settle */
  __sync();

  cctl0_tmp = read_c0_cctl0();

  /* operation is rising edge triggered so we first need
     to clear the bit and then set it to trigger operation */
 if ((cctl0_tmp & CCTL0_DWB) != 0)
  {
  cctl0_tmp &= ~CCTL0_DWB;
  write_c0_cctl0(cctl0_tmp);
  }

  /* now set the bit to trigger rising edge */
  cctl0_tmp |= CCTL0_DWB;
  write_c0_cctl0(cctl0_tmp);

}

/* ------------------------------------------
 * Invalidate entire data cache
 * no arguments are passed
 * ------------------------------------------
 */
void inv_dcache_all(void)
{
  uint32_t cctl0_tmp;

  cctl0_tmp = read_c0_cctl0();
 
  /* operation is rising edge triggered so we first need
     to clear the bit and then set it to trigger operation */
 if ((cctl0_tmp & CCTL0_DInv) != 0)
  {
  cctl0_tmp &= ~CCTL0_DInv;
  write_c0_cctl0(cctl0_tmp);
  }

  /* now set the bit to trigger rising edge */
  cctl0_tmp |= CCTL0_DInv;
  write_c0_cctl0(cctl0_tmp);
}

/* ------------------------------------------
 * Invalidate entire instruction cache 
 * no arguments are passed
 * ------------------------------------------
 */
void inv_icache_all(void)
{
  uint32_t cctl0_tmp;

  cctl0_tmp = read_c0_cctl0();

  /* operation is rising edge triggered so we first need
     to clear the bit and then set it to trigger operation */
 if ((cctl0_tmp & CCTL0_IInv) != 0)
  {
  cctl0_tmp &= ~CCTL0_IInv;
  write_c0_cctl0(cctl0_tmp);
  }

  /* now set the bit to trigger rising edge */
  cctl0_tmp |= CCTL0_IInv;
  write_c0_cctl0(cctl0_tmp);
}

