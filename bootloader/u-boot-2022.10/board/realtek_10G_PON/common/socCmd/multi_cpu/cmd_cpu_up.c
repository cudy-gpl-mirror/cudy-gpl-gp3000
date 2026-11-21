/*
* Copyright (c) 2015-2019, Cortina Access. All rights reserved.
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
#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/psci.h>
#define mmio_write_32(a, d)	writel(d, a)
#define mmio_read_32(a)		readl(a)

#define CPU0_tmp_buf 0x00500000
/*
 * Use 0x510000,0x520000,0x530000 as CPU1~3 stack 
 */
#if defined(CONFIG_TARGET_RTK_ELNATH)
#define CPU_NUM 2
#elif defined(CONFIG_TARGET_RTK_TAURUS)
#define CPU_NUM 4
#elif defined(CONFIG_TARGET_RTK_MERCURY)
#define CPU_NUM 4
#else
#define CPU_NUM 2
#endif
#define wfe() __asm__ __volatile__ ("wfe\n\t" : : );
#define sev() __asm__ __volatile__ ("sev\n\t" : : );


/**********************/
#define TICKET_SHIFT	16

typedef struct {
#ifdef __AARCH64EB__
	u16 next;
	u16 owner;
#else
	u16 owner;
	u16 next;
#endif
} __aligned(4) arch_spinlock_t;

#define __ARCH_SPIN_LOCK_UNLOCKED	{ 0 , 0 }
static inline void arch_spin_lock(arch_spinlock_t *lock)
{
	unsigned int tmp;
	arch_spinlock_t lockval, newval;

	asm volatile(
	/* Atomically increment the next ticket. */
"	prfm	pstl1strm, %3\n"
"1:	ldaxr	%w0, %3\n"
"	add	%w1, %w0, %w5\n"
"	stxr	%w2, %w1, %3\n"
"	cbnz	%w2, 1b\n"
	/* Did we get the lock? */
"	eor	%w1, %w0, %w0, ror #16\n"
"	cbz	%w1, 3f\n"
	/*
	 * No: spin on the owner. Send a local event to avoid missing an
	 * unlock before the exclusive load.
	 */
"	sevl\n"
"2:	wfe\n"
"	ldaxrh	%w2, %4\n"
"	eor	%w1, %w2, %w0, lsr #16\n"
"	cbnz	%w1, 2b\n"
	/* We got the lock. Critical section starts here. */
"3:"
	: "=&r" (lockval), "=&r" (newval), "=&r" (tmp), "+Q" (*lock)
	: "Q" (lock->owner), "I" (1 << TICKET_SHIFT)
	: "memory");
}
static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
	asm volatile(
"	stlrh	%w1, %0\n"
	: "=Q" (lock->owner)
	: "r" (lock->owner + 1)
	: "memory");
}


arch_spinlock_t cpu_up_lock = __ARCH_SPIN_LOCK_UNLOCKED;

/* SMP printf*/

int smp_printf(const char *fmt, ...)
{
	va_list args;
	uint i;

	arch_spin_lock(&cpu_up_lock);
	va_start(args, fmt);
	i = vprintf(fmt, args);
	va_end(args);
	arch_spin_unlock(&cpu_up_lock);

	return i;
}

/*****************************/
extern void dcache_enable(void);
extern void dcache_disable(void);
typedef struct cmd_tbl        cmd_tbl_t;

typedef void (*SlaveFuncPtr)( void );
#define MAX_FUNC_NUM	10
static SlaveFuncPtr slave_func[4][MAX_FUNC_NUM] = { NULL }; //MAX CPU 4

static u8 cpu_on[CPU_NUM] = {0};
static ulong oncpu_mask=0xF;
	
static unsigned long rtk_invoke_psci_fn
                (unsigned long function_id, unsigned long arg0,
                 unsigned long arg1, unsigned long arg2)
{
        struct arm_smccc_res res;
        arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);
        return res.a0;
}

static inline unsigned long read_mpidr(void)
{
        unsigned long val;

        asm volatile("mrs %0, mpidr_el1" : "=r" (val));

        return val;
}

int get_cpu_id(void){
	#ifdef CONFIG_TARGET_RTK_MERCURY
	return read_mpidr() & 0xff;
	#else
	return (read_mpidr() >> 8) & 0xff;
	#endif
}
static int cpu_2_mpidr(int cpuid){
	#ifdef CONFIG_TARGET_RTK_MERCURY
	return cpuid & 0xff;
	#else
	return (cpuid & 0xff) << 8;
	#endif
}
static int wakeup_cpu(uint64_t sec_entrypoint, uint32_t cpu)
{

	//PSCI CPU idx is decied by mpidr//
	int ret;
	u32 cpu_idx = cpu_2_mpidr(cpu); //cpuid of mpidr 
	if (cpu_on[cpu] == 0x3) {
		cpu_on[cpu] = 0x2;
		dsb();
		sev();
	} else {
	//	smp_printf("	try invoke_psci_fn cpu(%d)\n", cpu_idx);
		ret = rtk_invoke_psci_fn(PSCI_0_2_FN64_CPU_ON, cpu_idx, sec_entrypoint, 0);
		if (ret){
			smp_printf("	invoke_psci_fn error(%d)\n", ret);
			return -1;
		}
	}
	return 0;
}


#include <asm/global_data.h>

static inline gd_t *get_gd(void)
{
	gd_t *gd_ptr;
	__asm__ volatile("mov %0, x18\n" : "=r" (gd_ptr));
	return gd_ptr;
}

static __maybe_unused ulong get_sp(void)
{
	ulong ret;

	asm("mov %0, sp" : "=r"(ret) : );
	return ret;
}



int slave_cpu_func_register(int func_id, void *func_p, int cpuid){
	if (func_id >= (sizeof(slave_func[0])/sizeof(void(*))))
		return -1;

	if(slave_func[cpuid][func_id] == NULL){
		slave_func[cpuid][func_id] = (SlaveFuncPtr) func_p;
	}else 
		return -2;
	
	return 0;
}
int slave_cpu_func_register_clean(int num, int cpuid){
	int i;
	if (num >= (sizeof(slave_func[0])/sizeof(void(*)))){
		for(i=0;i<(sizeof(slave_func[0])/sizeof(void(*)));i++){
			slave_func[cpuid][i] = NULL;
		}
		return 0;
	}

	slave_func[cpuid][num] = NULL;
	
	return 0;
}

void slave_cpu(ulong dummy){
	int i;
	gd_t *gd;
	int cpuid = get_cpu_id();
	/* get and setup gd */
	gd = (gd_t *)readq((void *)CPU0_tmp_buf);
	set_gd(gd);
	dsb();

	while (1) {
		invalidate_icache_all();
		dcache_enable();
		cpu_on[cpuid] = 0x3;


		for(i=0; i<(sizeof(slave_func[0])/sizeof(void(*))); i++){
			if(slave_func[cpuid][i])
				slave_func[cpuid][i]();
		}
	//	dcache_disable();
		while (cpu_on[cpuid] == 0x3) {
			wfe();
			rmb();
		}

	}
	smp_printf("\n%s %d: dead code\n", __func__,__LINE__);
	while (1)
		wfe();
}


void primary_cpu(ulong dummy){
	int i;
	for(i=0; i<(sizeof(slave_func[0])/sizeof(void(*))); i++){
		if(slave_func[0][i])
			slave_func[0][i]();
	}
}

static int do_cpu_on(void)
{
	gd_t *gd;
	unsigned int cpu_idx;
    
	gd = get_gd();
	
	smp_printf("%s : gd(%p), gd->relocaddr=0x%lx\n", __func__, gd, gd->relocaddr);
	
	writeq((u64)gd, CPU0_tmp_buf);

	flush_dcache_all();

	udelay(1000);
	for(cpu_idx=1 ; cpu_idx < CPU_NUM;cpu_idx++){
		if(oncpu_mask & (1<<cpu_idx)){
			smp_printf("Wake up CPU%d\n",cpu_idx);
			wakeup_cpu(gd->relocaddr, cpu_idx);
		}
		mdelay(5);
	}

	primary_cpu(0);
	mdelay(10);

	return 0;
}

/* Slave TEST  func */
static void smp_func0_dump_info(void);
static void smp_func1_memcpy_compare(void);
static void smp_func2_dram_test(void);
static void smp_func3_dram_test(void);
static void smp_func4_memcpy_compare(void);
static unsigned long smp_func_loop = 10;

static int do_cpu_run(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	int func_id = 0;
	char *cmd;

	if (argc > 3)
		return CMD_RET_USAGE;

	cmd = argv[1];

	if (strcmp(cmd, "on") == 0) {
		if (argc ==3)
		 func_id = simple_strtoul(argv[2], NULL, 10);

		switch (func_id) {
			case 4: 
				//use slot 1
				slave_cpu_func_register(1, &smp_func2_dram_test, 0);
				slave_cpu_func_register(1,  &smp_func4_memcpy_compare, 1);
				break;
			case 3: 
				//use slot 0
				//cpu 0 run &smp_func2_dram_test
				slave_cpu_func_register(0, &smp_func2_dram_test, 0);
				//cpu 1 run &smp_func3_dram_test
				slave_cpu_func_register(0, &smp_func3_dram_test, 1);
				break;
			case 2:
				slave_cpu_func_register(2, &smp_func2_dram_test,0);
				slave_cpu_func_register(2, &smp_func2_dram_test,1);
				slave_cpu_func_register(2, &smp_func2_dram_test,2);
				slave_cpu_func_register(2, &smp_func2_dram_test,3);
				break;		
			case 1:
				slave_cpu_func_register(1, &smp_func1_memcpy_compare, 1);
				break;		
			case 0:
			default :
				slave_cpu_func_register(0, &smp_func0_dump_info, 0);
				break;		
		}
		do_cpu_on();
	}else
	if (strcmp(cmd, "mask") == 0) {
		if (strict_strtoul(argv[2], 0, &oncpu_mask) < 0)
			return CMD_RET_USAGE;
	}else
	if (strcmp(cmd, "clean") == 0) {
		//Clare cpu0~3
		slave_cpu_func_register_clean(MAX_FUNC_NUM, 0); 
		slave_cpu_func_register_clean(MAX_FUNC_NUM, 1); 
		slave_cpu_func_register_clean(MAX_FUNC_NUM, 2); 
		slave_cpu_func_register_clean(MAX_FUNC_NUM, 3); 
	}else
	if (strcmp(cmd, "func_loop") == 0) {
		if (strict_strtoul(argv[2], 0, &smp_func_loop) < 0)
			return CMD_RET_USAGE;
		printf("smp_func_loop=%lu\n", smp_func_loop);
	}


	return 0;
}
U_BOOT_CMD(cpu_run, 3, 0, do_cpu_run,
           "CPU stress test", "oncpu_mask");



/*
 * CPU TESt Func
 *
 */
#include <common.h>
#include <rand.h>
static unsigned int y = 55476129U;
static unsigned int rtk_rand_r(unsigned int *seedp)
{
        *seedp ^= (*seedp << 13);
        *seedp ^= (*seedp >> 17);
        *seedp ^= (*seedp << 5);

        return *seedp;
}

static unsigned int rtk_rand(void)
{
        return rtk_rand_r(&y);
}


static void smp_func0_dump_info(void){
	int cpuid = get_cpu_id();
	smp_printf("\n%s %d: cpu(%d) gd(%p)\n", __func__,__LINE__, cpuid, get_gd());
	smp_printf("\n%s %d: cpu(%d) sp(0x%lx)\n", __func__,__LINE__, cpuid, get_sp());

	return;
}

extern void * memcpy_f(void *, const void *, __kernel_size_t);
extern int memcmp_aarch64(const void *,const void *,__kernel_size_t);

static void smp_func1_memcpy_compare(void){
	int cpuid = get_cpu_id();
	int i;
	void *src;
	void *dst;
	int *data_p;
	unsigned long src_addr = 0x1000000;  //16MB
	unsigned long dst_addr = 0x8000000; //128MB
	unsigned long test_size = 4*0x100000;

	src = (void *)(src_addr + (cpuid * test_size));
	dst = (void *)(dst_addr + (cpuid * test_size));

	data_p = (int *)src;
	for(i=0;i<test_size/sizeof(int);i=i+2) {
		*(data_p) = rtk_rand();
		*(data_p+1) = *(data_p) ^ 0xffffffff;
		data_p =  data_p +2;
	}

	//Use arch optimized mem cpy/compare
	memcpy_f(dst, src, test_size);
	if (memcmp_aarch64(dst, src, test_size) == 0)
		smp_printf("\n%s: cpu(%d) arch memcpy pass!\n", __func__, cpuid);
	else
		smp_printf("\n%s: cpu(%d) memcmp failed. size(%lu) done!\n", __func__, cpuid, test_size );
}

/*
 * Taget : DDR addr 0x6400000
 *
 */
#define MP_TEST_DDR_BASE 0x06400000
#define MP_TEST_DDR_SIZE 0x01000000
extern int _smp_dram_test(u32 dram_start, u32 area_size, u32 test_case);
static void smp_func2_dram_test(void){
	int cpuid = get_cpu_id();
	u32 src;
	u32 src_addr = 0x06400000; 
	int loop=10;

	u32 test_size = MP_TEST_DDR_SIZE;
	src = src_addr + (cpuid *test_size);
	while (loop) {
		_smp_dram_test((u32)src, (u32)test_size, 0);
		loop--;
	}

	return;
}

/*
 * Taget : DDR addr 0x050000000
 *
 */
#define MP_TEST_DDR_BASE1 0x05000000
#define MP_TEST_DDR_SIZE1 0x01000000
static void smp_func3_dram_test(void){
	int cpuid = get_cpu_id();
	u32 src;
	u32 src_addr = MP_TEST_DDR_BASE1; 
	int loop=10;

	u32 test_size = MP_TEST_DDR_SIZE1;
	src = src_addr + (cpuid *test_size);
	while (loop) {
		_smp_dram_test((u32)src, (u32)test_size, 0);
		loop--;
	}

	return;
}

static void smp_func4_memcpy_compare(void){
	int cpuid = get_cpu_id();
	int i,x;
	void *src;
	void *dst;
	int *data_p;
	unsigned long src_addr = 0x1000000;  //16MB
	unsigned long dst_addr = 0x8000000; //128MB
	unsigned long test_size = 4*0x100000;

	src = (void *)(src_addr + (cpuid * test_size));
	dst = (void *)(dst_addr + (cpuid * test_size));

	for (x=0; x<smp_func_loop; x++) {
	data_p = (int *)src;
	for(i=0;i<test_size/sizeof(int);i=i+2) {
		*(data_p) = rtk_rand();
		*(data_p+1) = *(data_p) ^ 0xffffffff;
		data_p =  data_p +2;
	}

	//Use arch optimized mem cpy/compare
	memcpy_f(dst, src, test_size);
	if (memcmp_aarch64(dst, src, test_size) == 0)
		smp_printf("\n%s: cpu(%d) loop(%d) arch memcpy pass!\n", __func__, cpuid, x);
	else
		smp_printf("\n%s: cpu(%d) memcmp failed. size(%lu). loop(%d) done!\n", __func__, cpuid, test_size, x );
	}
}
