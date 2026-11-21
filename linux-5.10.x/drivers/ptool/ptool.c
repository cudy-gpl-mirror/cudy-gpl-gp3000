#include <linux/ptool.h>
#include <linux/smp.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <asm/barrier.h>
#include <asm/errno.h>
#include <linux/dma-mapping.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/version.h>

#if defined(CONFIG_RTK_PTOOL_CPU_PERF) && (defined(CONFIG_RTK_L34_G3_PLATFORM) || defined(CONFIG_CA8277B_SERIES) || defined(CONFIG_RTL8277C_SERIES))
#include <asm/sysreg.h>

char *event_name[]={
	"SW_INCR",					//0
	"L1I_CACHE_REFILL",
	"L1I_TLB_REFILL",
	"L1D_CACHE_REFILL",
	"L1D_CACHE",
	"L1D_TLB_REFILL",
	"LD_RETIRED",
	"ST_RETIRED",
	"INST_RETIRED",
	"EXC_TAKEN",
	"EXC_RETURN",				//10
	"CID_WRITE_RETIRED",
	"PC_WRITE_RETIRED",
	"BR_IMMED_RETIRED",
	"BR_RETURN_RETIRED",
	"UNALIGNED_LDST_RETIRED",
	"BR_MIS_PRED",
	"CPU_CYCLES",
	"BR_PRED",
	"MEM_ACCESS",
	"L1I_CACHE",				//20
	"L1D_CACHE_WB",
	"L2D_CACHE",
	"L2D_CACHE_REFILL",
	"L2D_CACHE_WB",
	"BUS_ACCESS",
	"MEMORY_ERROR",
	"INST_SPEC",
	"TTBR_WRITE_RETIRED",
	"BUS_CYCLES",
	"CHAIN",					//30
	"",
	"L2D_CACHE_ALLOCATE",
	"BR_RETIRED",
	"BR_MIS_PRED_RETIRED",
	"STALL_FRONTEND",
	"STALL_BACKEND",
	"L1D_TLB",
	"L1I_TLB",
	"",
	"",							//40
	"L3D_CACHE_ALLOCATE",
	"L3D_CACHE_REFILL",
	"L3D_CACHE",
	"",
	"L2D_TLB_REFILL",
	"",
	"L2D_TLB",
	"",
	"",
	"",							//50
	"",
	"DTLB_WALK",
	"ITLB_WALK",
	"LL_CACHE_RD",
	"LL_CACHE_MISS_RD",
	"REMOTE_ACCESS_RD",

	"",
	"",
	"",
	"",							//60
	"",
	"",
	"",
	"L1D_CACHE_RD",
	"L1D_CACHE_WR",
	"L1D_CACHE_REFILL_RD",
	"L1D_CACHE_REFILL_WR",
	"L1D_CACHE_REFILL_INNER",
	"L1D_CACHE_REFILL_OUTER",
	"",							//70
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"L2D_CACHE_RD",				//80
	"L2D_CACHE_WR",
	"L2D_CACHE_REFILL_RD",
	"L2D_CACHE_REFILL_WR",
	"",
	"",
	"",
	"",
	"",
	"",
	"",							//90
	"",
	"",
	"",
	"",
	"",
	"BUS_ACCESS_RD",
	"BUS_ACCESS_WR",
	"",
	"",
	"",							//100
	"",
	"MEM_ACCESS_RD",
	"MEM_ACCESS_WR",
	"",
	"",
	"",
	"",
	"",
	"",
	"",							//110
	"",
	"LD_SPEC",
	"ST_SPEC",
	"LDST_SPEC",
	"DP_SPEC",
	"ASE_SPEC",
	"VFP_SPEC",
	"PC_WRITE_SPEC",
	"CRYPTO_SPEC",
	"BR_IMMED_SPEC",			//120
	"BR_RETURN_SPEC",
	"BR_INDIRECT_SPEC",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",							//130
	"",
	"",
	"",
	"EXC_IRQ",
	"EXC_FIQ",
	"",
	"",
	"",
	"",
	"",							//140
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",							//150
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"L3D_CACHE_RD",				//160
	"",
	"L3D_CACHE_REFILL_RD",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",							//170
	"",
	"",
	"",
	"",
	"",
	"",
	"",	
	"",
	"",
	"",							//180
	"",
	"",
	"",
	"",	
	"",
	"",
	"",
	"",
	"",
	"",							//190
	"",	
	"L3D_CACHE_REFILL_PREFETCH",
	"L2D_CACHE_REFILL_PREFETCH",
	"L1D_CACHE_REFILL_PREFETCH",
	"L2D_WS_MODE",
	"L1D_WS_MODE_ENTRY",
	"L1D_WS_MODE",
	"PREDECODE_ERROR",
	"L3D_WS_MODE",
	"",							//200
	"BR_COND_PRED",
	"BR_INDIRECT_MIS_PRED",
	"BR_INDIRECT_ADDR_MIS_PRED",
	"BR_COND_MIS_PRED",
	"BR_INDIRECT_ADDR_PRED",
	"BR_RETURN_ADDR_PRED",
	"BR_RETURN_ADDR_MIS_PRED",
	"L2D_LLWALK_TLB",
	"L2D_LLWALK_TLB_REFILL",
	"L2D_L2WALK_TLB",			//210
	"L2D_L2WALK_TLB_REFILL",
	"L2D_S2_TLB",
	"L2D_S2_TLB_REFILL",
	"L2D_CACHE_STASH_DROPPED",
	"",
	"",
	"",	
	"",
	"",
	"",							//220
	"",
	"",
	"",
	"",	
	"STALL_FRONTEND_CACHE",
	"STALL_FRONTEND_TLB",
	"STALL_FRONTEND_PDERR",
	"STALL_BACKEND_ILOCK",
	"STALL_BACKEND_ILOCK_AGU",
	"STALL_BACKEND_ILOCK_FPU",	//230
	"STALL_BACKEND_LD",
	"STALL_BACKEND_ST",
	"STALL_BACKEND_LD_CACHE",
	"STALL_BACKEND_LD_TLB",
	"STALL_BACKEND_ST_STB",
	"STALL_BACKEND_ST_TLB",
	"",
	"",
	"",
	"",
	"",
	"",
	""	
};

typedef enum cci_intf_e
{
	CCI_INTF_SLAVE_FROM_CPU=64,
	CCI_INTF_SLAVE_FROM_HSIO=32,
	CCI_INTF_SLAVE_FROM_MEM_SWITCH=0,
	CCI_INTF_MASTER_TO_CPE=256,
	CCI_INTF_MASTER_TO_MEM=288,
} cci_intf_t;

char *cci_intf_name[]={	
	"CCI_INTF_SLAVE_FROM_MEM_SWITCH",
	"CCI_INTF_SLAVE_FROM_HSIO",
	"CCI_INTF_SLAVE_FROM_CPU",
	"S3","S4","S5","S6","RSV",
	"CCI_INTF_MASTER_TO_CPE",
	"CCI_INTF_MASTER_TO_MEM",
	"M2","M3","M4","M5","M6","GLOBAL"
};


typedef enum cci_slave_event_e
{
	CCI_SLAVE_READ_REQ_VALID_READY=0,
	CCI_SLAVE_READ_REQ_DEVIE=1,
	CCI_SLAVE_READ_REQ_NORMAL_NONSHARE=2,
	CCI_SLAVE_READ_REQ_READONCE=3, //only read once, no need to change from unique to shared after read once.
	CCI_SLAVE_READ_REQ_READCLEAN_SHREAD_NOTSHAREDDIRTY_UNIQUE=4, //
	CCI_SLAVE_READ_REQ_MAKEUNIQUE_CLEANUNIQUE=5, //STORE,
	CCI_SLAVE_READ_REQ_CLEANINVALID_MAKEINVALID_CLEANSHARED=6, //STORE
	CCI_SLAVE_READ_REQ_DVM=7,
	CCI_SLAVE_READ_DATA_VALID_READY=8,
	CCI_SLAVE_READ_RLAST=9,
	CCI_SLAVE_WRITE_REQ_VALID_READY=0xa,
	CCI_SLAVE_WRITE_REQ_DEVIE=0xb,
	CCI_SLAVE_WRITE_REQ_NONSHARE=0xc,
	CCI_SLAVE_WRITE_REQ_WRITEBACK_WRITECLEAN=0xd,
	CCI_SLAVE_WRITE_REQ_WRITELINEUNIQUE=0xe,
	CCI_SLAVE_WRITE_REQ_WRITEUNIQUE=0xf,	
	CCI_SLAVE_WRITE_REQ_EVICT=0x10,
	CCI_SLAVE_WRITE_DATA_VALID_READY=0x12,
	CCI_SLAVE_SNOOP_REQ_VALID_READY=0x13,
	CCI_SLAVE_SNOOP_REQ_READONCE_CLEAN_NOTSHAREDDIRTY_SHARED_UNIQUE=0x14,	
	CCI_SLAVE_SNOOP_REQ_MAKEINVALID_CLEANINVALID_CLEANSHARED=0x15,
	CCI_SLAVE_SNOOP_RESP=0x16,
	CCI_SLAVE_READ_REQ_STALL=0x17, // ARVALID = HIGH, ARREADY = LOW  ARVALID = Read address is valid, ARREADY = Read address is ready (slave is ready to accept an address and related control signal)
	CCI_SLAVE_READ_DATA_STALL=0x18,  // RVALID = HIGH, RREADY = LOW  RVALID = Read data is valid, RREADY = read data is ready
	CCI_SLAVE_WRITE_REQ_STALL=0x19,
	CCI_SLAVE_WRITE_DATA_STALL=0x1a,
	CCI_SLAVE_WRITE_RESP_STALL=0x1b,
	CCI_SLAVE_SNOOP_REQ_STALL=0x1c,
	CCI_SLAVE_SNOOP_DATA_STALL=0x1d,
	CCI_SLAVE_REQ_STALL_BECAUSE_OF_OT_TRANS_LIMIT=0x1e,
	CCI_SLAVE_READ_STALL_BECAUSE_OF_ARBITRAION=0x1f,	
} cci_slave_event_t;

char *cci_slave_event_name[]={
	"READ_REQ_VALID_READY",
	"READ_REQ_DEVIE",
	"READ_REQ_NORMAL_NONSHARE",
	"READ_REQ_READONCE",
	"READ_REQ_READCLEAN_SHREAD_NOTSHAREDDIRTY_UNIQUE",
	"READ_REQ_MAKEUNIQUE_CLEANUNIQUE",
	"READ_REQ_CLEANINVALID_MAKEINVALID_CLEANSHARED",
	"READ_REQ_DVM",
	"READ_DATA_VALID_READY",
	"READ_RLAST",
	"WRITE_REQ_VALID_READY",
	"WRITE_REQ_DEVIE",
	"WRITE_REQ_NONSHARE",
	"WRITE_REQ_WRITEBACK_WRITECLEAN",
	"WRITE_REQ_WRITELINEUNIQUE",
	"WRITE_REQ_WRITEUNIQUE",	
	"WRITE_REQ_EVICT",
	"",
	"WRITE_DATA_VALID_READY",
	"SNOOP_REQ_VALID_READY",
	"SNOOP_REQ_READONCE_CLEAN_NOTSHAREDDIRTY_SHARED_UNIQUE",	
	"SNOOP_REQ_MAKEINVALID_CLEANINVALID_CLEANSHARED",
	"SNOOP_RESP",
	"READ_REQ_STALL",
	"READ_DATA_STALL",
	"WRITE_REQ_STALL",
	"WRITE_DATA_STALL",
	"WRITE_RESP_STALL",
	"SNOOP_REQ_STALL",
	"SNOOP_DATA_STALL",
	"REQ_STALL_BECAUSE_OF_OT_TRANS_LIMIT",
	"READ_STALL_BECAUSE_OF_ARBITRAION",
};

typedef enum cci_master_event_e
{
	CCI_MASTER_READ_DATA=0,
	CCI_MASTER_WRITE_DATA=1,
	CCI_MASTER_READ_REQ_STALL=2, //RVALID is HIGH, RREADY is LOW
	CCI_MASTER_READ_DATA_STALL=3,
	CCI_MASTER_WRITE_REQ_STALL=4,
	CCI_MASTER_WRITE_DATA_STALL=5,	
	CCI_MASTER_WRITE_RESP_STALL=6,
} cci_master_event_t;


char *cci_master_event_name[]={
	"CCI_MASTER_READ_DATA",
	"CCI_MASTER_WRITE_DATA",
	"CCI_MASTER_READ_REQ_STALL",
	"CCI_MASTER_READ_DATA_STALL",
	"CCI_MASTER_WRITE_REQ_STALL",
	"CCI_MASTER_WRITE_DATA_STALL",	
	"CCI_MASTER_WRITE_RESP_STALL",
};

unsigned long long read_pa(unsigned long long *addr)
{
	int ret;
	asm __volatile__(
	"	MRS x0, sctlr_el1	\n"
	"	mov x2,x0	\n"
	"	LSR x0,x0,#3	\n"
	"	LSL x0,x0,#3	\n" 
	"	MSR sctlr_el1,x0	\n"
	"	LDR x1,%[addr] \n"
	"	MSR sctlr_el1,x2	\n"
	"	STR x1,%[ret]	\n" 	
	:[ret] "=m" (ret) : [addr] "m" (*addr):"x0","x1","x2","memory"
	);	
	return ret;
}

void write_pa(unsigned long long *addr,unsigned long long value)
{
	asm __volatile__(
	"	LDR x1,%[value]	\n"
	"	MRS x0, sctlr_el1	\n"
	"	mov x2,x0	\n"
	"	LSR x0,x0,#3	\n"
	"	LSL x0,x0,#3	\n" 
	"	MSR sctlr_el1,x0	\n"
	"	STR x1,%[addr] \n"
	"	MSR sctlr_el1,x2	\n"
	:[addr] "=m" (*addr) : [value] "m" (value):"x0","x1","x2","memory"
	);	
}

#if defined(CONFIG_NR_CPUS)
#define CPU_NUMBER CONFIG_NR_CPUS
#else
#ERROR
#endif


u64 perf_count[CPU_NUMBER]={0};
u64 cp3_count[CPU_NUMBER][6][MAX_HASH_IDX]={0};
u64 cp3_count_min[CPU_NUMBER][6][MAX_HASH_IDX]={0};
u64 cp3_count_max[CPU_NUMBER][6][MAX_HASH_IDX]={0};
u64 cp3_cnt_temp[CPU_NUMBER][6][MAX_HASH_IDX]={0};
u64 funcs_pc[CPU_NUMBER][MAX_HASH_IDX]={0};
char funcs[CPU_NUMBER][MAX_HASH_IDX][FUNC_LEN]={0};
int func_in[CPU_NUMBER][MAX_HASH_IDX]={0};
int func_exit[CPU_NUMBER][MAX_HASH_IDX]={0};
int profile_stopped[CPU_NUMBER]={1};
int event_type[6]={0};
int lock_mode=0;
u32 cci_event_sel[6]={0};

struct work_struct ptool_wq[CPU_NUMBER];
struct workqueue_struct *ptool_pworkq[CPU_NUMBER];
int wq_action[CPU_NUMBER]={0}; //0: set event / reset / start, 1:stop
int pmcr_el0_start=7;
int pmcr_el0_stop=0;
void *cci_pmu_base=NULL;
int profile_cci_stopped=1;



void wq_func(struct work_struct *pwq)
{	
	int cpu_id=smp_processor_id();
	printk("[WQ] cpu_id=%d wq_act=%d\n",cpu_id,wq_action[cpu_id]);
	switch(wq_action[cpu_id])
	{
		case 0: //set event reset and start
			{
				u32 pmcntenset_el0=0;
				u32 pmselr_el0=0;
				u32 pmxevtyper_el0=0;
				int i;
			
				asm __volatile__(
						"	MRS %0, pmcntenset_el0		\n"
						:"=r"(pmcntenset_el0)::
				);
				
				pmcntenset_el0|=0x8000003f;
				
				asm __volatile__(
						"	MSR pmcntenset_el0,%0	\n"
						::"r"(pmcntenset_el0):
				);
			
				for(i=0;i<6;i++)
				{
					pmselr_el0=i;
					asm __volatile__(
							"	MSR pmselr_el0,%0	\n"
							::"r"(pmselr_el0):
					);
			
					pmxevtyper_el0=event_type[i]|(1<<27);
					asm __volatile__(
							"	MSR pmxevtyper_el0,%0	\n"
							::"r"(pmxevtyper_el0):
					);
				}

				memset(&funcs[cpu_id][0][0],0,MAX_HASH_IDX*FUNC_LEN);
				memset(&funcs_pc[cpu_id][0],0,MAX_HASH_IDX*sizeof(u64));
				memset(&func_in[cpu_id][0],0,sizeof(int)*MAX_HASH_IDX);
				memset(&func_exit[cpu_id][0],0,sizeof(int)*MAX_HASH_IDX);
				memset(&cp3_count[cpu_id][0][0],0,sizeof(u64)*MAX_HASH_IDX*6);
				memset(&cp3_count_max[cpu_id][0][0],0,sizeof(u64)*MAX_HASH_IDX*6);
				memset(&cp3_count_min[cpu_id][0][0],0xff,sizeof(u64)*MAX_HASH_IDX*6);
				memset(&cp3_cnt_temp[cpu_id][0][0],0,sizeof(u64)*MAX_HASH_IDX*6);

				profile_stopped[cpu_id]=0;

				
				
				asm __volatile__(
						"	MSR pmcr_el0,%0	\n"
						::"r"(pmcr_el0_start):
				);

			}
			break;
			
		case 1: //stop		
			asm __volatile__(
					"	MSR pmcr_el0,%0	\n"
					::"r"(pmcr_el0_stop):
			);
			
			profile_stopped[cpu_id]=1;
			break;

	}
	
	printk("[WQ] cpu_id=%d profile_stopped[0]=%d [1]=%d\n",cpu_id, profile_stopped[0], profile_stopped[1]);
}



int profile_stop(void)
{
	int i;
	int cpu_id=smp_processor_id();
	lock_mode=0;

	for(i=0;i<CPU_NUMBER;i++)
	{
		wq_action[i]=1;
		if(cpu_id!=i)
			queue_work_on(i,ptool_pworkq[i], &ptool_wq[i]);
		else
			wq_func(NULL);
	}
	return 0;
}

int profile_start(int event_start)
{	
	int cpu_id=smp_processor_id();
	int i;

	if((event_start==1)||(event_start==-1))
	{

		event_type[0]=17; //cpu cycles
		event_type[1]=8; // inst cnt	
		event_type[2]=6; //load cnt
		event_type[3]=7; //STORE cnt	
		event_type[4]=20; //L1I cache	
		event_type[5]=36; //STALL_BACKEND

		if(event_start==-1)
			lock_mode=1;
		else
			lock_mode=0;
	}
	else
	{
		int i;
		for(i=0;i<6;i++)
			event_type[i]=i+event_start;
	}

	printk("this proc in CPU[%d]\n",smp_processor_id());

	for(i=0;i<CPU_NUMBER;i++)
	{
		wq_action[i]=0;
		if(cpu_id!=i)
			queue_work_on(i,ptool_pworkq[i], &ptool_wq[i]);
		else
			wq_func(NULL);
	}
	

#if 1
#else


{
	u32 pmcr_el0_stop=0;
	u32 pmxevcntr_el0=0;
	u32 pmcr_el0;
	
	//void *base = ioremap(0x4f4320000, 0xffff);
	//u64 *addr=(u64 *)((u64)base+0xa008);
	mdelay(200);


	local_irq_disable();

	//reset and start
	pmcr_el0=7;
	asm __volatile__(
			"	MSR pmcr_el0,%0	\n"
			::"r"(pmcr_el0):
	);


	//do something here for profiling
	

	//stop
	asm __volatile__(
			"	MSR pmcr_el0,%0	\n"
			::"r"(pmcr_el0_stop):
	);

	

	local_irq_enable();

	
	//dump counter
	{
		u32 pmselr_el0=0;
		u32 pmxevtyper_el0=0;
		int i;
		for(i=0;i<6;i++)
		{

			pmselr_el0=i;

			asm __volatile__(
				"	MSR pmselr_el0,%0	\n"
				::"r"(pmselr_el0):
			);

			pmxevtyper_el0=event_type[i];
			asm __volatile__(
					"	MSR pmxevtyper_el0,%0	\n"
					::"r"(pmxevtyper_el0):
			);		

			asm __volatile__(
				"	MRS	%0, pmxevcntr_el0		\n"
				:"=r"(pmxevcntr_el0)::
			);

			if(event_name[pmxevtyper_el0][0]!=0)
			{
				if(pmxevcntr_el0!=0)
					printk("[%03d]%12d\t%s\n",pmxevtyper_el0,pmxevcntr_el0,event_name[pmxevtyper_el0]);
			}
		}
	}
}


#endif


	return 0;

}


inline int inline_strlen(char *buf)
{
	int i=0;
	while(1)
	{
		if(buf[i]==0) return i;
		i++;
	}
}

inline void inline_memcpy(u8 *dest, u8 *src, int len)
{
	int i;
	for(i=0;i<len;i++)
		dest[i]=src[i];
}


inline int inline_memcmp(u8 *str1, u8 *str2, int len)
{
	int i;
	int not_eq=0;
	for(i=0;i<len;i++)
		if(str1[i]!=str2[i])
		{
			if(str1[i]>str2[i]) not_eq=1;
			else	not_eq=-1;
			break;
		}
	return not_eq;
}


int profile(const char *func,u64 pc)
{
	int idx, j,ret;
	int cpu_id=smp_processor_id();

	if(lock_mode==1)
		local_irq_disable();
		
	if(likely(profile_stopped[cpu_id]==1)) {ret=MAX_HASH_IDX-1; goto return_val; }
	idx=((pc&((1<<MAX_HASH_IDX_BIT)-1))>>MAX_HASH_WAY_BIT)<<MAX_HASH_WAY_BIT;

	for(j=0;j<(1<<MAX_HASH_WAY_BIT);j++)
	{
		if(func_in[cpu_id][idx+j]==0)
		{
			++func_in[cpu_id][idx+j];
			inline_memcpy(funcs[cpu_id][idx+j],(char *)func,FUNC_LEN-1);
			funcs_pc[cpu_id][idx+j]=pc;
			goto out;
		}
		else
		{
			if(funcs_pc[cpu_id][idx+j]==pc)
			{
				++func_in[cpu_id][idx+j];
				goto out;
			}			
		}
	}


	//full
	ret=MAX_HASH_IDX-1;
	goto return_val;

out:
	idx+=j;
	ret=idx;

	for(j=0;j<6;j++)
	{
		asm __volatile__(
			"	MSR pmselr_el0,%0	\n"
			::"r"(j):
		);

		asm __volatile__(
			"	MRS %0, pmxevcntr_el0		\n"
			:"=r"(cp3_cnt_temp[cpu_id][j][idx])::
		);

		//cp3_cnt_temp[j][idx]=pmxevcntr_el0;
	}

return_val:



	return ret;

}
	


void profile_end(int idx)
{
	int j;
	u64 cnt;
	int cpu_id=smp_processor_id();	

	if(lock_mode==1)
		local_irq_enable();
	
	if(likely(profile_stopped[cpu_id]==1)) return;

	// fetch counter
	for(j=0;j<6;j++)
	{

		asm __volatile__(
			"	MSR pmselr_el0,%0	\n"
			::"r"(j):
		);
		asm __volatile__(
			"	MRS %0, pmxevcntr_el0		\n"
			:"=r"(perf_count[cpu_id])::
		);
		//printk("perf_count[%d] = %llu\n",cpu_id,perf_count[cpu_id]);
		if(likely(cp3_cnt_temp[cpu_id][j][idx]<=perf_count[cpu_id]))
		{
			cnt=perf_count[cpu_id]-cp3_cnt_temp[cpu_id][j][idx];

		}
		else
		{
			cnt=perf_count[cpu_id]+0x100000000LL-cp3_cnt_temp[cpu_id][j][idx];
		}
		//printk("cnt = %llu\n",cnt);
		if(cnt<cp3_count_min[cpu_id][j][idx]) cp3_count_min[cpu_id][j][idx]=cnt;
		if(cnt>cp3_count_max[cpu_id][j][idx]) cp3_count_max[cpu_id][j][idx]=cnt;
		cp3_count[cpu_id][j][idx]+=cnt;		
	}
	++func_exit[cpu_id][idx];
	
}



int profile_dump(void)
{

	int i,j=0,k[CPU_NUMBER]={0},c=0;	


	asm __volatile__(
	"	MSR pmcr_el0,xzr \n"
	:::
	);


	printk("Disable local interrupt mode: %s\n\n",lock_mode?"on":"off");


	printk("[HASH_],FUNC_IN_COUNT,FUNC_EXIT_COUNT, FUNCTION\n");
	printk("%23s[EVT],%20s,%15s,%15s,%15s\n\n","EVENT_NAME","EVENT_TOTAL_CNT","EVENT_CNT(AVG)","EVENT_CNT(MIN)","EVENT_CNT(MAX)");

	for(c=0;c<CPU_NUMBER;c++)
	{

		printk("CPU[%d]:\n",c);
		for(i=0;i<MAX_HASH_IDX;i++)
		{
			if(func_in[c][i]!=0)
			{
				++k[c];
				printk(KERN_CONT "[%5d],%13d,%15d",i,func_in[c][i],func_exit[c][i]);
				printk(KERN_CONT ", [%08llx] %s\n",funcs_pc[c][i]&0xffffffff,&funcs[c][i][0]);
				for(j=0;j<6;j++)
				{					
					if(event_name[event_type[j]][0]!=0)
					{
						u64 tmp=cp3_count[c][j][i];
						do_div(tmp,func_in[c][i]);
						printk(KERN_CONT "%23s[%3d]",event_name[event_type[j]],event_type[j]);
						printk(KERN_CONT ",%20llu",cp3_count[c][j][i]);
						printk(KERN_CONT ",%15llu",tmp); //avg
						printk(KERN_CONT ",%15llu",cp3_count_min[c][j][i]);
						printk(KERN_CONT ",%15llu\n",cp3_count_max[c][j][i]);
					}				
					
				}
				printk("\n");

			}
		}

		printk("Total %d functions in CPU%d\n\n",k[c],c);
	}




	return 0;

}




// Proc files

#define MAX_PROC_SIZE 40
char proc_data[MAX_PROC_SIZE];
struct proc_dir_entry *ptool_dir=NULL;
struct proc_dir_entry *proc_entry=NULL;


static int ptool_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, inode->i_private);
}


ssize_t ptool_proc_profile_event_type_dump(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	int i;
	for(i=0;i<240;i++)
	{
		if(event_name[i][0]!=0)
		{
			int f=0,j;
			for(j=0;j<6;j++)
				if(event_type[j]==i) {f=1; break; }
			
			printk("%s[%3d] %s\n",(f==1)?"*":" ",i,event_name[i]);
		}			
	}
	printk("\n");
	return 0;
}

ssize_t ptool_proc_profile_cci_event_dump(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	int i;


	printk("Interface List:\n");
	for(i=0;i<=15;i++)
	{
		if(strlen(cci_intf_name[i])>6)
			printk("%d: %s\n",i,cci_intf_name[i]);
	}

	printk("\nSlave Interface Events:\n");
	for(i=0;i<0x20;i++)
	{
		if(strlen(cci_slave_event_name[i])>1)
			printk("%d: %s\n",i,cci_slave_event_name[i]);
	}

	printk("\nMaster Interface Events:\n");
	for(i=0;i<7;i++)
	{
		printk("%d: %s\n",i,cci_master_event_name[i]);
	}

	printk("\nUse \"echo idx intf event > /proc/ptool/profile_cci_event\" to set CCI event.\n");

	printk("\nCurrent setting:\n");
	for(i=0;i<6;i++)
	{
		int intf=cci_event_sel[i]>>5;
		int event=cci_event_sel[i]&0x1f;		
		if(intf<8)
		{
			printk("%d: %s\t[%s]\n",i,cci_intf_name[intf],cci_slave_event_name[event]);
		}
		else if(intf<0xf)
		{
			printk("%d: %s\t[%s]\n",i,cci_intf_name[intf],cci_master_event_name[event]);
		}
		else if(intf==0xf)
		{
			printk("%d: %s\t[%d]\n",i,cci_intf_name[intf],event);
		}
		
	}
	printk("\n");
	return 0;
}


ssize_t ptool_proc_profile_enable_status(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	if(profile_stopped[0]==0)
	{
		printk("CPU Profile Enabled\n");
	}
	else
	{		
		printk("CPU Profile Disabled\n");
	}
	return 0;
	
}


ssize_t ptool_proc_profile_cci_enable_status(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	if(profile_cci_stopped==0)
	{
		printk("CCI Profile Enabled\n");
	}
	else
	{		
		printk("CCI Profile Disabled\n");
	}
	return 0;	
}


ssize_t ptool_proc_profile_event_type(struct file *file,const char *buf,size_t count,loff_t *data )
{
	int i;
	
	if(count > MAX_PROC_SIZE)
	    count = MAX_PROC_SIZE;
	if(copy_from_user(proc_data, buf, count))
	    return -EFAULT;

	sscanf(proc_data,"%d %d %d %d %d %d",&event_type[0],&event_type[1],&event_type[2],&event_type[3],&event_type[4],&event_type[5]);

	for(i=0;i<6;i++)
	{
		asm __volatile__(
				"	MSR pmselr_el0,%0	\n"
				::"r"(i):
		);

		asm __volatile__(
				"	MSR pmxevtyper_el0,%0	\n"
				::"r"(event_type[i]):
		);
	
		printk("[%3d] %s\n",event_type[i],event_name[event_type[i]]);
	}
	return count;
}


ssize_t ptool_proc_profile_cci_event(struct file *file,const char *buf,size_t count,loff_t *data )
{
	int index=0;
	int intf=0;
	int event=0;
	
	if(count > MAX_PROC_SIZE)
	    count = MAX_PROC_SIZE;
	if(copy_from_user(proc_data, buf, count))
	    return -EFAULT;

	sscanf(proc_data,"%d %d %d",&index,&intf,&event);

	if(index<6)
	{
		cci_event_sel[index]=(intf<<5)|event;
		*(volatile u32*)((u64)cci_pmu_base+0x10000*(index+1))=cci_event_sel[index];
	}
	
	if(intf<8)
	{
		printk("%d: %s\t[%s]\n",index,cci_intf_name[intf],cci_slave_event_name[event]);
	}
	else if(intf<0xf)
	{
		printk("%d: %s\t[%s]\n",index,cci_intf_name[intf],cci_master_event_name[event]);
	}
	else if(intf==0xf)
	{
		printk("%d: %s\t[%d]\n",index,cci_intf_name[intf],event);
	}

	
	return count;
}



ssize_t ptool_proc_profile_enable(struct file *file,const char *buf,size_t count,loff_t *data )
{
	int value=0;
	if(count > MAX_PROC_SIZE)
	    count = MAX_PROC_SIZE;
	if(copy_from_user(proc_data, buf, count))
	    return -EFAULT;

	sscanf(proc_data,"%d",&value);

	if(value)
	{
		profile_start(value);
	}
	else if(value==0) 
	{
		profile_stop();
	}

	return count;
}


ssize_t ptool_proc_profile_cci_enable(struct file *file,const char *buf,size_t count,loff_t *data )
{
	int value=0;
	if(count > MAX_PROC_SIZE)
	    count = MAX_PROC_SIZE;
	if(copy_from_user(proc_data, buf, count))
	    return -EFAULT;

	sscanf(proc_data,"%d",&value);

	if(value)
	{
		//start and reset CCI PMU counter
		*(volatile u32*)((u64)cci_pmu_base+0x100)|=3;
		profile_cci_stopped=0;
		

	}
	else if(value==0) 
	{
		//stop CCI PMU counter
		*(volatile u32*)((u64)cci_pmu_base+0x100)&=0xfffffffe;
		profile_cci_stopped=1;		
	}

	return count;
}


ssize_t ptool_proc_profile_dump(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	profile_dump();
	return 0;
}

ssize_t ptool_proc_profile_cci_dump(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	int i;

	//stop CCI event counter
	*(volatile u32*)((u64)cci_pmu_base+0x100)&=0xfffffffe;

	for(i=0;i<6;i++)
	{
		int intf=cci_event_sel[i]>>5;
		int event=cci_event_sel[i]&0x1f;
		u32 cnt=*(volatile u32*)((u64)cci_pmu_base+(0x10000*(i+1))+4);
		if(intf<8)
		{
			printk("%d: %s\t[%s]\t%d\n",i,cci_intf_name[intf],cci_slave_event_name[event],cnt);
		}
		else if(intf<0xf)
		{
			printk("%d: %s\t[%s]\t%d\n",i,cci_intf_name[intf],cci_master_event_name[event],cnt);
		}
		
	}

	{
		u32 m0=*(volatile u32*)((u64)cci_pmu_base+0x90100);
		u32 m1=*(volatile u32*)((u64)cci_pmu_base+0x90104);
		u32 s0=*(volatile u32*)((u64)cci_pmu_base+0x90000);
		u32 s1=*(volatile u32*)((u64)cci_pmu_base+0x90004);
		u32 s2=*(volatile u32*)((u64)cci_pmu_base+0x90008);
	}
	

	//start CCI event counter
	if(profile_cci_stopped==0)	*(volatile u32*)((u64)cci_pmu_base+0x100)|=1;

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
struct file_operations proc_file_fops[] = {

#ifdef CONFIG_RTK_PTOOL_CPU_PERF
	{
		read: ptool_proc_profile_event_type_dump,
		write: ptool_proc_profile_event_type
	},
	{
		read: ptool_proc_profile_enable_status,
		write: ptool_proc_profile_enable
	},
	{
		read: ptool_proc_profile_dump,
		write: NULL
	},
	{
		read: ptool_proc_profile_cci_event_dump,
		write: ptool_proc_profile_cci_event
	},
	{
		read: ptool_proc_profile_cci_enable_status,
		write: ptool_proc_profile_cci_enable
	},
	{
		read: ptool_proc_profile_cci_dump,
		write: NULL
	},	
#endif	
};
#else
static const struct proc_ops proc_file_fops[]  = {
#ifdef CONFIG_RTK_PTOOL_CPU_PERF
	{
		.proc_open	= ptool_proc_open,
		.proc_read	= ptool_proc_profile_event_type_dump,
		.proc_write	= ptool_proc_profile_event_type,
		.proc_lseek	= seq_lseek,
		.proc_release	= single_release,
	},
	{
		.proc_open	= ptool_proc_open,
		.proc_read	= ptool_proc_profile_enable_status,
		.proc_write	= ptool_proc_profile_enable,
		.proc_lseek	= seq_lseek,
		.proc_release	= single_release,
	},
	{
		.proc_open	= ptool_proc_open,
		.proc_read	= ptool_proc_profile_dump,
		.proc_write	= NULL,
		.proc_lseek	= seq_lseek,
		.proc_release	= single_release,
	},
	{
		.proc_open	= ptool_proc_open,
		.proc_read	= ptool_proc_profile_cci_event_dump,
		.proc_write	= ptool_proc_profile_cci_event,
		.proc_lseek	= seq_lseek,
		.proc_release	= single_release,
	},
	{
		.proc_open	= ptool_proc_open,
		.proc_read	= ptool_proc_profile_cci_enable_status,
		.proc_write	= ptool_proc_profile_cci_enable,
		.proc_lseek	= seq_lseek,
		.proc_release	= single_release,
	},
	{
		.proc_open	= ptool_proc_open,
		.proc_read	= ptool_proc_profile_cci_dump,
		.proc_write	= NULL,
		.proc_lseek	= seq_lseek,
		.proc_release	= single_release,
	},	
#endif	
};
#endif


char proc_name[][40]={
#ifdef CONFIG_RTK_PTOOL_CPU_PERF
	"profile_event",
	"profile_enable",
	"profile_dump",
	"profile_cci_event",
	"profile_cci_enable",
	"profile_cci_dump",
#endif
};


//dma_addr_t dma_handle=NULL;
//struct device dmadev={0};
static inline u32 armv8pmu_pmccfiltr_read(void)
{
	u32 val;
	asm volatile("mrs %0, pmccfiltr_el0" : "=r" (val));
	return val;
}

static inline void armv8pmu_pmccfiltr_write(u32 val)
{
	asm volatile("msr pmccfiltr_el0, %0" :: "r" (val));
}

__init int ptool_init(void)
{
#define STR_BUF_LEN	64
	int i;
	char name_str[STR_BUF_LEN]={0};
	cci_pmu_base = ioremap(0x4f6000000, 0xfffff);	
	/*
	{
		u32 *s2_share_over=(u32*)((u64)cci_pmu_base+0x3004);
		*s2_share_over=3;
	}
	*/

	for(i = 0; i < CPU_NUMBER; i++) {
		snprintf(name_str, STR_BUF_LEN, "PTOOL_WQ_CPU%d", i);
		ptool_pworkq[i]=alloc_workqueue(name_str,WQ_HIGHPRI,0);	
		INIT_WORK(&ptool_wq[i],wq_func);
	}

	cci_event_sel[0]=CCI_INTF_MASTER_TO_MEM|CCI_MASTER_READ_REQ_STALL;
	cci_event_sel[1]=CCI_INTF_MASTER_TO_MEM|CCI_MASTER_READ_DATA_STALL;
	cci_event_sel[2]=CCI_INTF_MASTER_TO_CPE|CCI_MASTER_READ_REQ_STALL;
	cci_event_sel[3]=CCI_INTF_MASTER_TO_CPE|CCI_MASTER_READ_DATA_STALL;
	cci_event_sel[4]=CCI_INTF_SLAVE_FROM_CPU|CCI_SLAVE_READ_REQ_STALL;
	cci_event_sel[5]=CCI_INTF_SLAVE_FROM_CPU|CCI_SLAVE_READ_DATA_STALL;

	*(volatile u32*)((u64)cci_pmu_base+0x10000)=cci_event_sel[0];
	*(volatile u32*)((u64)cci_pmu_base+0x20000)=cci_event_sel[1];
	*(volatile u32*)((u64)cci_pmu_base+0x30000)=cci_event_sel[2];
	*(volatile u32*)((u64)cci_pmu_base+0x40000)=cci_event_sel[3];
	*(volatile u32*)((u64)cci_pmu_base+0x50000)=cci_event_sel[4];
	*(volatile u32*)((u64)cci_pmu_base+0x60000)=cci_event_sel[5];	
	*(volatile u32*)((u64)cci_pmu_base+0x10008)=1;
	*(volatile u32*)((u64)cci_pmu_base+0x20008)=1;
	*(volatile u32*)((u64)cci_pmu_base+0x30008)=1;
	*(volatile u32*)((u64)cci_pmu_base+0x40008)=1;
	*(volatile u32*)((u64)cci_pmu_base+0x50008)=1;
	*(volatile u32*)((u64)cci_pmu_base+0x60008)=1;

	
	if(ptool_dir==NULL){
			ptool_dir = proc_mkdir("ptool", NULL);
	}

	for(i=0;i<sizeof(proc_file_fops)/sizeof(proc_file_fops[0]);i++)
	{
		proc_entry = proc_create(proc_name[i], 0, ptool_dir, &proc_file_fops[i]);
		if(!proc_entry){
			printk("create ptool proc[%d] failed!\n",i);
			return -ENOMEM;
		}
	}
	armv8pmu_pmccfiltr_write(armv8pmu_pmccfiltr_read() | 1 << 27);
	
	printk("PTOOL init done!!\n");		
	return 0;
}

module_init(ptool_init);
EXPORT_SYMBOL(profile);
EXPORT_SYMBOL(profile_end);
MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Realtek tysu@realtek.com");
MODULE_DESCRIPTION("Realtek performance monitor tool");

#endif

#if defined(CONFIG_RTL9607C_SERIES)	
#ifdef CONFIG_RTK_PTOOL_LA_BY_GPIO
#define SIGNAL_NAME_MAX_LEN 20
int la_gpio_signal_cpu_mask[LA_GPIO_MAX]={0};
int la_gpio_signal_gpio_id[LA_GPIO_MAX]={0};
char la_gpio_signal_name[LA_GPIO_MAX][SIGNAL_NAME_MAX_LEN]={0};

#define REG32(reg) 	(*((volatile unsigned int *)(reg)))
#define SET_GPIO_NAME(gpio_name)  do { set_gpio_name(gpio_name,#gpio_name); } while(0)


void set_gpio_name(int idx, char *gpio_name)
{
	memcpy(&la_gpio_signal_name[idx],gpio_name,SIGNAL_NAME_MAX_LEN);
}

int la_gpio_init(void)
{
	int i;
	for(i=0;i<LA_GPIO_MAX;i++)
	{
		la_gpio_signal_cpu_mask[i]=0xf;
		la_gpio_signal_gpio_id[i]=0;		
	}

	SET_GPIO_NAME(LA_GPIO_SOFTIRQ);
	SET_GPIO_NAME(LA_GPIO_HW_ISR);
	SET_GPIO_NAME(LA_GPIO_SPINLOCK);
	SET_GPIO_NAME(LA_GPIO_NIC_RX);
	SET_GPIO_NAME(LA_GPIO_NIC_TX);
	SET_GPIO_NAME(LA_GPIO_WIFI_RX_DSR);
	SET_GPIO_NAME(LA_GPIO_WIFI_TX);
	SET_GPIO_NAME(LA_GPIO_AUX0);
	SET_GPIO_NAME(LA_GPIO_AUX1);
	SET_GPIO_NAME(LA_GPIO_AUX2);
	SET_GPIO_NAME(LA_GPIO_AUX3);
	SET_GPIO_NAME(LA_GPIO_AUX4);
	SET_GPIO_NAME(LA_GPIO_AUX5);
	SET_GPIO_NAME(LA_GPIO_AUX6);
	SET_GPIO_NAME(LA_GPIO_AUX7);	

	return 0;
}

void la_gpio_high(int cpu_mask, int gpio_id)
{
	int cpu_id=smp_processor_id();
	if(cpu_mask&(1<<cpu_id))
	{		
		if(gpio_id<32)
		{
			REG32(0xb800330c)|=(1<<gpio_id);
		}
		else
		{
			REG32(0xb8003328)|=(1<<(gpio_id-32));
		}
	}
}

void la_gpio_low(int cpu_mask, int gpio_id)
{
	int cpu_id=smp_processor_id();
	if(cpu_mask&(1<<cpu_id))
	{		
		if(gpio_id<32)
		{
			REG32(0xb800330c)&=~(1<<gpio_id);
		}
		else
		{
			REG32(0xb8003328)&=~(1<<(gpio_id-32));
		}
	}
}
#endif

#ifdef CONFIG_RTK_PTOOL_CPU_PERF

unsigned int perf_count[NR_CPUS]={0};
unsigned int perf_count1[NR_CPUS]={0};


char funcs[NR_CPUS][MAX_HASH_IDX][FUNC_LEN]={{{0}}};
int lines[NR_CPUS][MAX_HASH_IDX]={0};
int lines_exit[NR_CPUS][MAX_HASH_IDX]={0};


unsigned long long cp3_count0[NR_CPUS][MAX_HASH_IDX]={0};
unsigned long long cp3_count1[NR_CPUS][MAX_HASH_IDX]={0};

unsigned int cp3_cnt0_temp[NR_CPUS][MAX_HASH_IDX]={0};
unsigned int cp3_cnt1_temp[NR_CPUS][MAX_HASH_IDX]={0};
unsigned int perf_cp0event=0;
unsigned int perf_cp1event=0;
unsigned int perf_cp0ctrl=0;
unsigned int perf_cp1ctrl=0;
int perf_vpe_id=-1;


int profile_stopped=1;

int profile_stop(void)
{
	perf_cp0ctrl=0;
	perf_cp1ctrl=0;
	perf_vpe_id=-1;

	__asm__ __volatile__ \
	("	;\
		la		$8, perf_cp0ctrl ;\
		lw		$8, 0($8)		;\
		mtc0	$8, $25,0;\
	");

	//reset counter
	__asm__ __volatile__ \
	("	;\
		mtc0	$0, $25,1;\
	");

	__asm__ __volatile__ \
	("	;\
		la		$8, perf_cp1ctrl ;\
		lw		$8, 0($8)		;\
		mtc0	$8, $25,2;\
	");
	
	__asm__ __volatile__ \
	("	;\
		mtc0	$0, $25,3;\
	");

	profile_stopped=1;	
	return 0;
}
	

int profile_start(void)
{
	unsigned int perf_tcid=0;
	unsigned int perf_mt_en=1;
	unsigned int perf_vpeid=0;
	unsigned int perf_pctd=0;
	unsigned int perf_ie=0,perf_u=0,perf_s=0,perf_k=1,perf_exl=0;
	
	memset(funcs,0,MAX_HASH_IDX*FUNC_LEN*NR_CPUS);
	memset(lines,0,sizeof(int)*MAX_HASH_IDX*NR_CPUS);
	memset(lines_exit,0,sizeof(int)*MAX_HASH_IDX*NR_CPUS);
	memset(cp3_count0,0,sizeof(unsigned long long)*MAX_HASH_IDX*NR_CPUS);
	memset(cp3_count1,0,sizeof(unsigned long long)*MAX_HASH_IDX*NR_CPUS);

	//write cp0ctrl command
	if(perf_vpe_id>=0){
		perf_mt_en=1;
		perf_vpeid=perf_vpe_id;
	}else{
		perf_mt_en=0;
		perf_vpeid=0;
	}
	perf_cp0ctrl=(perf_tcid<<22)|(perf_mt_en<<20)|(perf_vpeid<<16)|(perf_pctd<<15)|(perf_cp0event<<5)|(perf_ie<<4)|(perf_u<<3)|(perf_s<<2)|(perf_k<<1)|perf_exl;
	perf_cp1ctrl=(perf_tcid<<22)|(perf_mt_en<<20)|(perf_vpeid<<16)|(perf_pctd<<15)|(perf_cp1event<<5)|(perf_ie<<4)|(perf_u<<3)|(perf_s<<2)|(perf_k<<1)|perf_exl;

	printk("cpu_id=%d",perf_vpe_id);
	printk("CP0 $25(0)=0x%x perf_cp0event=%d\n",perf_cp0ctrl,perf_cp0event);
	printk("CP0 $25(2)=0x%x perf_cp1event=%d\n",perf_cp1ctrl,perf_cp1event);
	
	__asm__ __volatile__ \
	("	;\
		la		$8, perf_cp0ctrl ;\
		lw		$8, 0($8)		;\
		mtc0	$8, $25,0;\
	");

	//reset counter
	__asm__ __volatile__ \
	("	;\
		mtc0	$0, $25,1;\
	");
	
	__asm__ __volatile__ \
	("	;\
		la		$8, perf_cp1ctrl ;\
		lw		$8, 0($8)		;\
		mtc0	$8, $25,2;\
	");

	//reset counter
	__asm__ __volatile__ \
	("	;\
		mtc0	$0, $25,3;\
	");

	profile_stopped=0;

	return 0;

}


inline int inline_strlen(char *buf)
{
	int i=0;
	while(1)
	{
		if(buf[i]==0) return i;
		i++;
	}
}

inline void inline_memcpy(u8 *dest, u8 *src, int len)
{
	int i;
	for(i=0;i<len;i++)
		dest[i]=src[i];
}


inline int inline_memcmp(u8 *str1, u8 *str2, int len)
{
	int i;
	int not_eq=0;
	for(i=0;i<len;i++)
		if(str1[i]!=str2[i])
		{
			if(str1[i]>str2[i]) not_eq=1;
			else	not_eq=-1;
			break;
		}
	return not_eq;
}

int profile(const char *func)
{
	int idx, func_len, j;
	int cpu_id=smp_processor_id();
	
	if(profile_stopped) return MAX_HASH_IDX-1;
	
	func_len=inline_strlen((char *)func);
	idx=((
			 ((unsigned int)(func[func_len-1]-'_'+1)|
				 (unsigned int)((func[func_len-2]-'_'+1)<<8)|
				 (unsigned int)((func[func_len-3]-'_'+1)<<16) )
					 *
					 0x9e370001
					 *
			  ((unsigned int)(func[0]-'_'+1)|
				 (unsigned int)((func[1]-'_'+1)<<8)|
				 (unsigned int)((func[2]-'_'+1)<<16) )					 
				  )>>(32-MAX_HASH_IDX_BIT+MAX_HASH_WAY_BIT))<<MAX_HASH_WAY_BIT;

	for(j=0;j<(1<<MAX_HASH_WAY_BIT);j++)
	{
		if(lines[cpu_id][idx+j]==0)
		{
			++lines[cpu_id][idx+j];
			inline_memcpy(funcs[cpu_id][idx+j],(char *)func,FUNC_LEN-1);
			goto out;
		}
		else
		{			
			if(inline_memcmp((u8*)func,funcs[cpu_id][idx+j],(func_len<(FUNC_LEN-1))?func_len:(FUNC_LEN-1))==0)
			{
				++lines[cpu_id][idx+j];
				goto out;
			}			
		}
	}

	//full
	return MAX_HASH_IDX-1;

out:
	idx+=j;

	__asm__ __volatile__ ("mfc0 %0, $25,1" : "=r" (perf_count[cpu_id]));

	//printk("idx2=%d\n",idx);

	cp3_cnt0_temp[cpu_id][idx]=perf_count[cpu_id];

	//printk("idx3=%d\n",idx);	

	__asm__ __volatile__ ("mfc0 %0, $25,3" : "=r" (perf_count1[cpu_id]));

	cp3_cnt1_temp[cpu_id][idx]=perf_count1[cpu_id];
	
	return idx;
}
	

void profile_end(int idx)
{
	int cpu_id=smp_processor_id();

	if(profile_stopped) return;

	// fetch counter
	__asm__ __volatile__ ("mfc0 %0, $25,1" : "=r" (perf_count[cpu_id]));
	__asm__ __volatile__ ("mfc0 %0, $25,3" : "=r" (perf_count1[cpu_id]));

	++lines_exit[cpu_id][idx];

	if(likely(cp3_cnt0_temp[cpu_id][idx]<=perf_count[cpu_id]))
		cp3_count0[cpu_id][idx]+=(unsigned long long)(perf_count[cpu_id]-cp3_cnt0_temp[cpu_id][idx]);
	else
		cp3_count0[cpu_id][idx]+=(unsigned long long)(perf_count[cpu_id]+0x100000000LL-cp3_cnt0_temp[cpu_id][idx]);

	if(likely(cp3_cnt1_temp[cpu_id][idx]<=perf_count1[cpu_id]))
		cp3_count1[cpu_id][idx]+=(unsigned long long)(perf_count1[cpu_id]-cp3_cnt1_temp[cpu_id][idx]);
	else
		cp3_count1[cpu_id][idx]+=(unsigned long long)(perf_count1[cpu_id]+0x100000000LL-cp3_cnt1_temp[cpu_id][idx]);

}


int profile_dump(void)
{
	int i,j,c=0,display=0;
	unsigned long long tmp,tmp1;

	for(c=0;c<NR_CPUS;c++){
		j=0;
		display=0;
		for(i=0;i<MAX_HASH_IDX;i++)
		{
			if(lines[c][i]!=0)
			{
				tmp=cp3_count0[c][i];
				tmp1=cp3_count1[c][i];
				++j;

				if(display++==0)
					printk(KERN_CONT "CPU %d:\n",c);
				
				do_div(tmp ,lines_exit[c][i]);
				do_div(tmp1,lines_exit[c][i]);
				printk(KERN_CONT "[%5d],%10d",i,lines[c][i]);
				printk(KERN_CONT ",%10d",lines_exit[c][i]);
				printk(KERN_CONT ",%15llu",cp3_count0[c][i]);
				printk(KERN_CONT ",%10llu",tmp);
				printk(KERN_CONT ",%15llu",cp3_count1[c][i]);
				printk(KERN_CONT ",%10llu",tmp1);
				printk(KERN_CONT ", %s",&funcs[c][i][0]);
				printk(KERN_CONT "\n");
			}
		}
		if(display==1)
			printk(KERN_CONT "Total %d functions\n",j);
	}
	return 0;

}

#endif


// Proc files

struct proc_dir_entry *ptool_dir=NULL;
struct proc_dir_entry *proc_entry=NULL;

#ifdef CONFIG_RTK_PTOOL_LA_BY_GPIO
int ptool_proc_la_gpio_dump(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	int i;
	printk("%5s %20s %8s %5s\n","idx","name","cpu_mask","gpio");
	for(i=0;i<LA_GPIO_MAX;i++)
	{
		printk("%5d %20s %8x %5d\n",i,la_gpio_signal_name[i],la_gpio_signal_cpu_mask[i],la_gpio_signal_gpio_id[i]);	
	}
	printk("\nUser guide:\n");
	printk("use \"echo $idx $cpu_mask $gpio  >/proc/ptool/la_gpio_set\" to config each gpio signal.\n");
	return 0;
}


int ptool_proc_la_gpio_set(struct file *file,const char *buf,int count,void *data )
{
	int idx,cpumask,gpio_pin;
	
	if(count > MAX_PROC_SIZE)
	    count = MAX_PROC_SIZE;
	if(copy_from_user(proc_data, buf, count))
	    return -EFAULT;

	sscanf(proc_data,"%d %x %d",&idx,&cpumask,&gpio_pin);	
	if(idx<LA_GPIO_MAX)
	{
		printk("idx=%d cpu_mask=0x%x gpio_pin=%d\n",idx,cpumask,gpio_pin);
		la_gpio_signal_cpu_mask[idx]=cpumask;
		la_gpio_signal_gpio_id[idx]=gpio_pin;

		if(la_gpio_signal_gpio_id[idx]!=0)
		{
			if(la_gpio_signal_gpio_id[idx]<32)
			{
				int bitValue=(1<<(la_gpio_signal_gpio_id[idx]));
				REG32(0xbb000038)|=(bitValue);
				REG32(0xb8003308)|=(bitValue);
				REG32(0xb800330c)&=~(bitValue);
			}
			else
			{
				int bitValue=(1<<(la_gpio_signal_gpio_id[idx]-32));
				REG32(0xbb00003c)|=(bitValue);
				REG32(0xb8003324)|=(bitValue);
				REG32(0xb8003328)&=~(bitValue);
			}
		}		
	}
	
	return count;
}


#endif


#ifdef CONFIG_RTK_PTOOL_CPU_PERF
#include <linux/seq_file.h>
#include <linux/version.h>

int ptool_proc_profile_enable_status(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	if(profile_stopped==0)
	{		
		printk("CPU Profile Enabled\n");
	}
	else
	{		
		printk("CPU Profile Disabled\n");
	}
	return 0;
}


int ptool_proc_profile_enable(struct file *file, const char __user *buf, size_t count, loff_t *data)
{
	long 		value;
	char 		tmpbuf[64];
	char		*strptr;
	char		*tokptr;

	if (buf && !copy_from_user(tmpbuf, buf, count)) {
		tmpbuf[count] = '\0';
		strptr=tmpbuf;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		value=simple_strtol(tokptr, NULL, 0);
		if(value<0){
			profile_stop();
			goto errout;
		}
		perf_cp0event=(unsigned int)value;
		tokptr = strsep(&strptr," ");
		value=simple_strtol(tokptr, NULL, 0);
		if(value<0){
			profile_stop();
			goto errout;
		}
		perf_cp1event=(unsigned int)value;
		tokptr = strsep(&strptr," ");
		value=simple_strtol(tokptr, NULL, 0);
		perf_vpe_id=(int)value;
		
		profile_start();
	}

errout:
	return count;
}

static int ptool_proc_profile_dump(struct file *filp,char *buf,size_t count,loff_t *offp )
{
	profile_dump();
	return 0;
}

static int ptool_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, NULL, inode->i_private);
}
#endif

static const struct proc_ops proc_file_fops[] = {
#ifdef CONFIG_RTK_PTOOL_LA_BY_GPIO
		{
			.proc_open           = ptool_proc_open,
	        .proc_read           = ptool_proc_la_gpio_dump,
	        .proc_write          = NULL,
	        .proc_lseek          = seq_lseek,
	        .proc_release        = single_release,
		},
		{
			.proc_open			 = ptool_proc_open,
			.proc_read			 = ptool_proc_la_gpio_dump,
			.proc_write 		 = ptool_proc_la_gpio_set,
			.proc_lseek 		 = seq_lseek,
			.proc_release		 = single_release,
		},
#endif
#ifdef CONFIG_RTK_PTOOL_CPU_PERF
		{
			.proc_open			 = ptool_proc_open,
			.proc_read			 = ptool_proc_profile_enable_status,
			.proc_write 		 = ptool_proc_profile_enable,
			.proc_lseek 		 = seq_lseek,
			.proc_release		 = single_release,
		},
		{			
			.proc_open			 = ptool_proc_open,
			.proc_read			 = ptool_proc_profile_dump,
			.proc_write 		 = NULL,
			.proc_lseek 		 = seq_lseek,
			.proc_release		 = single_release,
		},
#endif	
};

char proc_name[][40]={
#ifdef CONFIG_RTK_PTOOL_LA_BY_GPIO
	"la_gpio_dump",
	"la_gpio_set",
#endif
#ifdef CONFIG_RTK_PTOOL_CPU_PERF
	"profile_enable",
	"profile_dump",
#endif
};


__init int ptool_init(void)
{
	int i;

#ifdef CONFIG_RTK_PTOOL_LA_BY_GPIO
	la_gpio_init();
#endif


	if(ptool_dir==NULL){
		ptool_dir = proc_mkdir("ptool", NULL);
	}

	for(i=0;i<sizeof(proc_file_fops)/sizeof(struct proc_ops);i++)
	{
		proc_entry = proc_create(proc_name[i], 0, ptool_dir, &proc_file_fops[i]);
		if(!proc_entry){
			printk("create ptool proc[%d] failed!\n",i);
			return -ENOMEM;
		}
	}		

	return 0;
}

module_init(ptool_init);

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Realtek Semiconductor Corp.");
MODULE_DESCRIPTION("Realtek performance monitor tool");

#endif
