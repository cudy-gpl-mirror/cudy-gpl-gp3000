/*
 * Copyright (c) 2013-2016, ARM Limited and Contributors. All rights reserved.
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
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
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

#ifndef __ASIC_SIM_TEST__
#define __ASIC_SIM_TEST__
#ifndef __ASSEMBLY__
#include <stdio.h>

#ifdef TEST_OTP
#define TEST_INFO INFO
#else
#define TEST_INFO(...)
#endif


#ifdef TEST_ON_VENUS_ASIC_SIM
// VENUS ASIC simulation watches the changes in GLOBAL_SOFTWARE, GLOBAL_SOFTWARE2 and GLOBAL_SCRATCH.
// Software can use them as a way to display debugging information.
// We use GLOBAL_SOFTWARE to mark the progress of the execution on ASIC simulator
// Each bit in GLOBAL_SOFTWARE is used at different locatioin in code. 
// flags are from 0 to 31
// Register GLOBAL_SOFTWARE2 is to make a special mark, a signature of two bytes 0xABCD
// plus a 2 bytes marking id
// Register GLOBAL_SCRATCH is used to dump 32 bit values

#ifndef TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_FLAG
#define HWSIM_PROGRESS_FLAG_SET(flag) do {                              \
    volatile unsigned int data = *(volatile unsigned int *)GLOBAL_SOFTWARE; \
    data |= (1<<flag);                                       \
    *(volatile unsigned int *)GLOBAL_SOFTWARE = data;        \
} while (0);
#else /* TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_FLAG */
define HWSIM_PROGRESS_FLAG_SET(flag)
#endif /* TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_FLAG */

#ifndef TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_MARK
// mark is 0xABCDmark
#define HWSIM_PROGRESS_MARK(mark) do {                      \
    volatile unsigned int data = *(volatile unsigned int *)GLOBAL_SOFTWARE2;  \
    if (data == (0xABCD0000 | mark)) { data = (0xABCE0000 | mark); } else { data = (0xABCD0000 | mark);}; \
    *(volatile unsigned int *)GLOBAL_SOFTWARE2 = data;\
} while (0);
// if val is the same as before, val is val+1 to make sure it is always updated so HW ASIC SIM catches the update
#define HWSIM_PROGRESS_VAL(val) do {                      \
    volatile unsigned int data = *(volatile unsigned int *)GLOBAL_SCRATCH; \
    if (val == data) {data++;} else {data = val;}                              \
    *(volatile unsigned int *)GLOBAL_SCRATCH = data;\
} while (0);

void HWSIM_PROGRESS_VAL64(unsigned long long int val);

#else // TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_MARK

#define HWSIM_PROGRESS_MARK(mark) 
#define HWSIM_PROGRESS_VAL(val)
#define HWSIM_PROGRESS_VAL64(val)

#endif // TEST_ON_VENUS_ASIC_SIM_DISABLE_PROGRESS_MARK

#else /* !TEST_ON_VENUS_ASIC_SIM */
#define HWSIM_PROGRESS_FLAG_SET(flag)
#define HWSIM_PROGRESS_MARK(flag)
#define HWSIM_PROGRESS_VAL(val);
#define HWSIM_PROGRESS_VAL64(val);
#endif /* TEST_ON_VENUS_ASIC_SIM */


#endif /* __ASSEMBLY__ */
#endif /* __ASIC_SIM_TEST__ */
