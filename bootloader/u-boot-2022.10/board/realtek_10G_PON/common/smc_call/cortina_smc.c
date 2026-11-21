#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/arm-smccc.h>

u64 ca_smc_call(unsigned int smc_id, unsigned long x1, unsigned long x2, unsigned long x3, unsigned long x4)
{
    struct arm_smccc_res res;

    arm_smccc_smc(smc_id, x1, x2, x3, x4, 0, 0, 0, &res);
    return res.a0;

#if 0
#ifdef CONFIG_ARM64

    asm volatile(
          "mov x0, %0\n"
          "mov x1, %1\n"
          "mov x2, %2\n"
          "mov x3, %3\n"
          "smc  #0\n"
          : "+r"(smc_id)
          : "r"(x1), "r"(x2), "r"(x3));
#else
    asm volatile(
	".arch_extension sec\n"
          "mov r0, %0\n"
          "mov r1, %1\n"
          "mov r2, %2\n"
          "mov r3, %3\n"
          "smc  #0\n"
          : "+r"(smc_id)
          : "r"(x1), "r"(x2), "r"(x3));

#endif
        return smc_id;
#endif
}



