/*
 * Include Files
 */
#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <spi_flash.h>
#include "serial_nor.h"

static struct spi_flash *g_mnor_test;

extern void flush_dcache_all(void);
#define _cache_flush() flush_dcache_all()
#define ARY_LEN(x)    (sizeof(x)/sizeof(x[0]))


#define MT_SUCCESS (0)
#define MT_FAIL    (-1)
#define MT_ILL_CMD (-2)

#define MT_NOR_FAIL \
({ \
    printf("%s (%d) test failed.\n", __FUNCTION__,__LINE__);\
    return MT_FAIL; \
})

#define ENG_CMD_ILL \
({ \
    printf("ERR: Illegal command (%d).\n",__LINE__);\
    return MT_ILL_CMD;\
})

/* Minimum DRAM_SIZE is 256MB
 * Useable test range[0]: 0x00000000 ~ 0x03D00000 // 0~61MB
 * Useable test range[1]: 0x04300000 ~ 0x0D200000 // 67MB~210MB
 */
#define SIZE_4KB  (0x1000)
#define SIZE_1MB  (0x100000)
#define SIZE_2MB  (0x200000)
#define TEST_SIZE_PER_PATTREN (0x10000)    //Test 64KB per pattern
#define RSV_SPACE             (0x400000)   //SIZE_4MB
#define SRC_PATT_ADDR         (0x00000000) //Offset = DRAM 0 MB
#define DST_DATA_ADDR         (0x01000000) //Offset = DRAM 16 MB
#define FLASH_BACKUP_ADDR     (0x04300000) //Offset = DRAM 67 MB
#define MAX_BACKUP_SIZE       (0x08000000) //128MB

/**************************
  * Command Parsing
  *************************/
typedef struct {
    uint8_t test_loops;
    uint8_t assign_test_range;
    uint32_t mt_offset; //Offset: 0 ~ 16MB ~ 32MB ~ 64MB
    uint32_t mt_size;   //Unit is byte
    uint32_t test_case; //0xCE: chip_erase
} spif_cmd_parsing_info_t;


extern unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base);

/*
 * Data Declaration
 */
DECLARE_GLOBAL_DATA_PTR;

static const uint32_t flash_patterns[] = {
    0x5a5aa5a5,
    0xa5a5a5a5,
    0x55555555,
    0xaaaaaaaa,
    0x00000000,
    0xff00ff00,
    0x00ff00ff,
    0x0000ffff,
    0xffff0000,
    0xffffffff,
};

/*
 * Function Declaration
 */
static int flash_sect_erase (ulong addr_first, ulong cnt)
{
    uint32_t offset = addr_first-CONFIG_SYS_FLASH_BASE;
    return spi_flash_erase(g_mnor_test, offset, cnt);
}

/*-----------------------------------------------------------------------
 * Copy memory to flash.
 * Make sure all target addresses are within Flash bounds,
 * and no protected sectors are hit.
 * Returns:
 * ERR_OK          0 - OK
 * ERR_TIMOUT      1 - write timeout
 * ERR_NOT_ERASED  2 - Flash not erased
 * ERR_PROTECTED   4 - target range includes protected sectors
 * ERR_INVAL       8 - target address not in Flash memory
 * ERR_ALIGN       16 - target address not aligned on boundary
 *            (only some targets require alignment)
 */
static int flash_write (char *src, ulong addr, ulong cnt)
{
    uint32_t offset = (addr-CONFIG_SYS_FLASH_BASE);
    return spi_flash_write(g_mnor_test, offset, cnt, src);
}

static int flash_read (char *src, ulong addr, ulong cnt)
{
    uint32_t offset = (addr-CONFIG_SYS_FLASH_BASE);
#if 1
    return spi_flash_read(g_mnor_test, offset, cnt, src);
#else /* For checking JIRA issue: https://jira.realtek.com/browse/VENUSII-1558 */
    uint32_t i;
    uint32_t bytes_per_read = 12;
    uint32_t remain = cnt;

    for(i=bytes_per_read ; i<cnt ; i+=bytes_per_read){
        spi_flash_read(g_mnor_test, offset, bytes_per_read, src);
        offset  += bytes_per_read;
        src  += bytes_per_read;
        remain = remain - bytes_per_read;
    }
    spi_flash_read(g_mnor_test, offset, remain, src);
    return 0;
#endif
}


static int flash_normal_patterns(uint32_t flash_start_addr, uint32_t test_size_per_pattern, uint32_t flash_test_size)
{
    int i, j;
    uint32_t read_value, flash_start;
    volatile uint32_t *dram_addr;
    uint32_t re_idx=0;
    uint32_t remain_size=flash_test_size;
    char _err_msg[] = {"\nEE: addr(0x%x): 0x%x != pattern(0x%x) %s, %d\n"};

    printf("=======start %s test / 0x%08x~0x%08x (%dMB/%dKB)=======\n", __FUNCTION__, flash_start_addr, flash_start_addr+flash_test_size-1, flash_test_size/SIZE_1MB, flash_test_size/1024);
    for (i=0; i < (sizeof(flash_patterns)/sizeof(uint32_t)); i++, remain_size=flash_test_size, re_idx=0){
        /* write pattern*/
        _cache_flush();
        dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
        for(j=0; j < test_size_per_pattern; j=j+4) {
            *dram_addr++ = flash_patterns[i];
        }

        /* check pattern */
        _cache_flush();
        dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
        for(j=0; j < test_size_per_pattern; j=j+4){
            read_value = (*dram_addr);
            if(read_value != flash_patterns[i]){
                printf(_err_msg,(uint32_t)(uintptr_t)dram_addr,read_value,flash_patterns[i],__FUNCTION__,__LINE__);
                MT_NOR_FAIL;
            }
            dram_addr++;
        }
        printf("\rSPIF: pattern[%d](0x%x) setting pass", i, flash_patterns[i]);


        while(remain_size >= test_size_per_pattern){
            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            flash_start = flash_start_addr + ((re_idx*test_size_per_pattern)%flash_test_size);
            flash_sect_erase(flash_start, test_size_per_pattern);
            flash_write((char *)dram_addr, flash_start, test_size_per_pattern);

            /* check flash data sequentially. */
            dram_addr = (uint32_t *)(uintptr_t)DST_DATA_ADDR;
            flash_read((char *)dram_addr, flash_start, test_size_per_pattern);
            for(j=0; j < test_size_per_pattern; j=j+4){
                read_value = (*dram_addr);
                if(read_value != flash_patterns[i]){
                    printf(_err_msg,(uint32_t)(uintptr_t)dram_addr,read_value,flash_patterns[i],__FUNCTION__,__LINE__);
                    MT_NOR_FAIL;
                }
                dram_addr++;
            }
            re_idx++;
            remain_size -= test_size_per_pattern;
            printf("\rSPIF: pat[%d](0x%x) 0x%x pass (remain 0x%x)", i, flash_patterns[i], flash_start, remain_size);
        }
    }
    printf("\n\r%s test succeed.                              \n", __FUNCTION__);
    return MT_SUCCESS;
}

static int flash_walking_of_1(uint32_t flash_start_addr, uint32_t test_size_per_pattern, uint32_t flash_test_size)
{
    int i;
    int j;
    uint32_t walk_pattern;
    uint32_t start_value;
    uint32_t flash_start;
    uint32_t re_idx=0;
    uint32_t remain_size = flash_test_size;
    volatile uint32_t *dram_addr;

    printf("\n=======start %s test / 0x%08x~0x%08x (%dMB/%dKB)=======\n", __FUNCTION__, flash_start_addr, flash_start_addr+flash_test_size-1, flash_test_size/SIZE_1MB, flash_test_size/1024);
    while(remain_size >= test_size_per_pattern){
        for (i=0; i < 32; i++){
            /* generate pattern */
            walk_pattern = (1 << i);

            /* write pattern*/
            _cache_flush();
            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            for(j=0; j < test_size_per_pattern; j=j+4){
                *dram_addr++ = walk_pattern;
            }

            /* check pattern */
            _cache_flush();
            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            for(j=0; j < test_size_per_pattern; j=j+4){
                start_value = (*dram_addr);
                if(start_value != walk_pattern){
                    printf("EE: addr(0x%x): 0x%x != pattern(0x%x) %s, %d\n",\
                    (uint32_t)(uintptr_t)dram_addr , start_value, walk_pattern,  __FUNCTION__, __LINE__);
                    MT_NOR_FAIL;
                }
                dram_addr++;
            }
            printf("\rFlash: pattern[%d](0x%x) setting passed", i, walk_pattern);

            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            flash_start = flash_start_addr + (((re_idx++)*test_size_per_pattern)%flash_test_size);
            flash_sect_erase(flash_start, test_size_per_pattern);
            flash_write((char *)dram_addr, flash_start, test_size_per_pattern);

            /* check data */
            dram_addr = (uint32_t *)(uintptr_t)DST_DATA_ADDR;
            flash_read((char *)dram_addr, flash_start, test_size_per_pattern);
            for(j=0; j < test_size_per_pattern; j=j+4){
                start_value = (*dram_addr);
                if(start_value != walk_pattern){
                    printf("\nEE: addr(0x%x): 0x%x != pattern(0x%x) %s, %d\n",\
                    (uint32_t)(uintptr_t)dram_addr , start_value, walk_pattern,  __FUNCTION__, __LINE__);
                    MT_NOR_FAIL;
                }
                dram_addr++;
            }
            remain_size -= test_size_per_pattern;
            printf("\rFlash: pattern[%d](0x%x) 0x%x passed (remain 0x%x)", i, walk_pattern, flash_start, remain_size);
            if(0 == remain_size) goto end;
        }
    }
end:
    printf("\r%s test succeed.            \n", __FUNCTION__);
    return MT_SUCCESS;
}

static int flash_walking_of_0(uint32_t flash_start_addr, uint32_t test_size_per_pattern, uint32_t flash_test_size)
{
    int i;
    int j;
    uint32_t read_value;
    uint32_t walk_pattern;
    uint32_t flash_start;
    uint32_t re_idx=0;
    uint32_t remain_size = flash_test_size;
    volatile uint32_t *dram_addr;

    printf("\n=======start %s test / 0x%08x~0x%08x (%dMB/%dKB)=======\n", __FUNCTION__, flash_start_addr, flash_start_addr+flash_test_size-1, flash_test_size/SIZE_1MB, flash_test_size/1024);
    while(remain_size >= test_size_per_pattern){
        for (i=0; i < 32; i++){
            /* generate pattern */
            walk_pattern = ~(1 << i);

            /* write pattern*/
            _cache_flush();
            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            for(j=0; j < test_size_per_pattern; j=j+4){
                *dram_addr++ = walk_pattern;
            }

            /* check data */
            _cache_flush();
            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            for(j=0; j < test_size_per_pattern; j=j+4){
                read_value = (*dram_addr);
                if(read_value != walk_pattern){
                    printf("EE: addr(0x%x): 0x%x != pattern(0x%x) %s, %d\n",\
                    (uint32_t)(uintptr_t)dram_addr , read_value, walk_pattern,  __FUNCTION__, __LINE__);
                    MT_NOR_FAIL;
                }
                dram_addr++;
            }
            printf("\rFlash:pattern[%d](0x%x) setting passed", i, walk_pattern);

            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            flash_start = flash_start_addr + (((re_idx++)*test_size_per_pattern)%flash_test_size);
            flash_sect_erase(flash_start, test_size_per_pattern);
            flash_write((char *)dram_addr, flash_start, test_size_per_pattern);

            /* check data */
            dram_addr = (uint32_t *)(uintptr_t)DST_DATA_ADDR;
            flash_read((char *)dram_addr, flash_start, test_size_per_pattern);
            for(j=0; j < test_size_per_pattern; j=j+4){
                read_value = (*dram_addr);
                if(read_value != walk_pattern){
                    printf("\nEE: addr(0x%x): 0x%x != pattern(0x%x) %s, %d\n",\
                    (uint32_t)(uintptr_t)dram_addr , read_value, walk_pattern,  __FUNCTION__, __LINE__);
                    MT_NOR_FAIL;
                }
                dram_addr++;
            }
            remain_size -= test_size_per_pattern;
            printf("\rFlash:pattern[%d](0x%x) 0x%x passed (remain 0x%x)", i, walk_pattern, flash_start, remain_size);
            if(0 == remain_size) goto end;
        }
    }
end:
    printf("\r%s test succeed.            \n", __FUNCTION__);
    return MT_SUCCESS;
}

static int flash_addr_rot(uint32_t flash_start_addr, uint32_t test_size_per_pattern, uint32_t flash_test_size)
{
    int i;
    int j;
    uint32_t read_value;
    uint32_t flash_start;
    uint32_t pattern;
    uint32_t re_idx=0;
    uint32_t remain_size = flash_test_size;
    volatile uint32_t *dram_addr;
    volatile uint32_t *check_addr;

    printf("\n=======start %s test / 0x%08x~0x%08x (%dMB/%dKB)=======\n", __FUNCTION__, flash_start_addr, flash_start_addr+flash_test_size-1, flash_test_size/SIZE_1MB, flash_test_size/1024);
    while(remain_size >= test_size_per_pattern){
        for (i=0; i < 32; i=i+4){
            /* write pattern*/
            _cache_flush();
            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            for(j=0; j < test_size_per_pattern; j=j+4){
                pattern = ((uint32_t)(uintptr_t)dram_addr << i);
                *dram_addr++ = pattern;
            }

            /* check data */
            _cache_flush();
            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            for(j=0; j < test_size_per_pattern; j=j+4){
                pattern = ((uint32_t)(uintptr_t)dram_addr << i);
                read_value = (*dram_addr);
                if(read_value != pattern){
                    printf("\nEE: addr(0x%x): 0x%x != pattern(0x%x) %s, %d\n",\
                          (uint32_t)(uintptr_t)dram_addr , read_value, pattern,  __FUNCTION__, __LINE__);
                    MT_NOR_FAIL;
                }
                dram_addr++;
            }
            printf("\rFlash: rotate %d setting passed", i);

            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            flash_start = flash_start_addr + (((re_idx++)*test_size_per_pattern)%flash_test_size);
            flash_sect_erase(flash_start, test_size_per_pattern);
            flash_write((char *)dram_addr, flash_start, test_size_per_pattern);

            /* check data */
            check_addr = (uint32_t *)(uintptr_t)DST_DATA_ADDR;
            flash_read((char *)check_addr, flash_start, test_size_per_pattern);
            for(j=0; j < test_size_per_pattern; j=j+4){
                pattern = ((uint32_t)(uintptr_t)dram_addr << i);
                read_value = (*check_addr);
                if(read_value != pattern){
                    printf("\nEE: addr(0x%x): 0x%x != pattern(0x%x) %s, %d\n",\
                    (uint32_t)(uintptr_t)check_addr , read_value, pattern,  __FUNCTION__, __LINE__);
                    MT_NOR_FAIL;
                }
                check_addr++;
                dram_addr++;
            }
            remain_size -= test_size_per_pattern;
            printf("\rFlash: rotate %d 0x%x passed (remain 0x%x)", i, flash_start, remain_size);
            if(0 == remain_size) goto end;
        }
    }
end:
    printf("\r%s test succeed.            \n", __FUNCTION__);
    return MT_SUCCESS;
}

static int flash_com_addr_rot(uint32_t flash_start_addr, uint32_t test_size_per_pattern, uint32_t flash_test_size)
{
    int i;
    int j;
    uint32_t start_value;
    uint32_t flash_start;
    uint32_t re_idx=0;
    uint32_t remain_size = flash_test_size;
    volatile uint32_t *dram_addr;
    volatile uint32_t *check_addr;

    printf("\n=======start %s test / 0x%08x~0x%08x(%dMB/%dKB)=======\n", __FUNCTION__, flash_start_addr, flash_start_addr+flash_test_size-1, flash_test_size/SIZE_1MB, flash_test_size/1024);
    while(remain_size >= test_size_per_pattern){
        for (i=0; i < 32; i=i+4){
            /* write pattern*/
            _cache_flush();
            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            for(j=0; j < test_size_per_pattern; j=j+4){
                *dram_addr = ~(((uint32_t)(uintptr_t)dram_addr) << i);
                dram_addr++;
            }

            /* check data */
            _cache_flush();
            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            for(j=0; j < test_size_per_pattern; j=j+4){
                start_value = (*dram_addr);
                if(start_value != (~((uint32_t)(uintptr_t)dram_addr << i))){
                     printf("EE: addr(0x%x): 0x%x != pattern(0x%x) %s, %d\n",\
                            (uint32_t)(uintptr_t)dram_addr , start_value, ~(((uint32_t)(uintptr_t)dram_addr) << i),  __FUNCTION__, __LINE__);
                    MT_NOR_FAIL;
                }
                dram_addr++;
            }
            printf("\rFlash: ~rotate %d setting passed", i);

               _cache_flush();
            dram_addr = (uint32_t *)(uintptr_t)SRC_PATT_ADDR;
            flash_start = flash_start_addr + (((re_idx++)*test_size_per_pattern)%flash_test_size);
            flash_sect_erase(flash_start, test_size_per_pattern);
            flash_write((char *)dram_addr, flash_start, test_size_per_pattern);

            /* check data */
            dram_addr = (uint32_t *)(uintptr_t) SRC_PATT_ADDR;
            check_addr = (uint32_t *)(uintptr_t)DST_DATA_ADDR;
            flash_read((char *)check_addr, flash_start, test_size_per_pattern);
            for(j=0; j < test_size_per_pattern; j=j+4){
                start_value = (*check_addr);
                if(start_value != (~((uint32_t)(uintptr_t)dram_addr << i))){
                    printf("\nEE: addr(0x%x): 0x%x != pattern(0x%x) %s, %d\n",\
                    (uint32_t)(uintptr_t)check_addr , start_value, ~(((uint32_t)(uintptr_t)dram_addr) << i),  __FUNCTION__, __LINE__);
                    MT_NOR_FAIL;
                }
                dram_addr++;
                check_addr++;
            }

            remain_size -= test_size_per_pattern;
            printf("\rFlash: ~rotate %d 0x%x passed (remain 0x%x)", i, flash_start,remain_size);
            if(0 == remain_size) goto end;
        }
    }
end:
    printf("\r%s test succeed.            \n", __FUNCTION__);
    return MT_SUCCESS;
}

static int flash_chip_erase_test(void)
{
    uint32_t i, offset;
    uint32_t *dram_addr = (uint32_t *)(uintptr_t)FLASH_BACKUP_ADDR;

    printf("\n=======start chip_erase test... erase %d MB ", (g_mnor_test->size/SIZE_1MB));
    flash_sect_erase(CONFIG_SYS_FLASH_BASE, g_mnor_test->size);

    printf(" ok... check erased data ");
    for(offset=0; offset < g_mnor_test->size; offset=offset+RSV_SPACE){
        printf("0x%08x\b\b\b\b\b\b\b\b\b\b",offset);

        flash_read((char *)dram_addr, CONFIG_SYS_FLASH_BASE, RSV_SPACE);
        for(i=0; i < RSV_SPACE; i=i+4){
            if(*dram_addr != 0xFFFFFFFF){
                printf("EE: Erase fail at offset(0x%x+0x%x) addr(0x%x):0x%x \n",offset,i,(uint32_t)(uintptr_t)dram_addr,*dram_addr);
                MT_NOR_FAIL;
            }
            dram_addr++;
        }
    }
    printf(" ok=======\n");
    return 0;

}

static int _flash_test(uint32_t flash_start_addr, uint32_t test_size_per_pattern, uint32_t flash_test_size) {
    uint32_t i;
    typedef int (norsf_test_t)(uint32_t, uint32_t, uint32_t);
    norsf_test_t *norsf_tf_list[] = {
        flash_normal_patterns,
        flash_walking_of_1,
        flash_walking_of_0,
        flash_addr_rot,
        flash_com_addr_rot,
    };

    for (i=0; i<ARY_LEN(norsf_tf_list); i++) {
        if (norsf_tf_list[i](flash_start_addr, test_size_per_pattern, flash_test_size)) {
            MT_NOR_FAIL;
        }
    }

    return MT_SUCCESS;
}

static int back_up_spif_data(uint32_t flash_addr_to_be_backup, uint32_t backup_size)
{
    volatile uint32_t *bootcode;
    volatile uint32_t *bk_buffer;

    bk_buffer = (uint32_t *)(uintptr_t)FLASH_BACKUP_ADDR;
    bootcode = (uint32_t *)(uintptr_t)flash_addr_to_be_backup;
    printf("Backing up %dMB/%dKB flash data: (0x%08x -> 0x%08x)... ",(backup_size/SIZE_1MB), backup_size/1024, (uint32_t)(uintptr_t)bootcode,(uint32_t)(uintptr_t)bk_buffer);
    flash_read((char *)bk_buffer, (ulong)bootcode, (ulong)backup_size);
    puts("done\n");
    return MT_SUCCESS;
}

static int restore_spif_data(uint32_t flash_addr_to_be_backup, uint32_t backup_size)
{
    volatile uint32_t *bootcode;
    volatile uint32_t *bk_buffer;
    int i;
    uint32_t addr = flash_addr_to_be_backup;
    uint32_t size = backup_size;

    printf("Recover %dMB/%dKB flash data (0x%08x -> 0x%08x) ... ", (backup_size/SIZE_1MB), backup_size/1024, FLASH_BACKUP_ADDR, addr);
    flash_sect_erase(addr, backup_size);
    flash_write((char *)FLASH_BACKUP_ADDR, addr, size);

    bk_buffer = (uint32_t *)FLASH_BACKUP_ADDR;

    bootcode = (uint32_t *)DST_DATA_ADDR;
    flash_read((char *)bootcode, addr, backup_size);
    for(i=0; i<backup_size; i=i+4){
        if(*bk_buffer != *bootcode){
        printf("#Recover flash data error: bk_addr(0x%08x):0x%08x != flash_addr(0x%08x):0x%08x\n",\
            (uint32_t)(uintptr_t)bk_buffer, *bk_buffer, (uint32_t)(uintptr_t)bootcode, *bootcode);
            return MT_FAIL;
        }
        bk_buffer++;
        bootcode++;
    }
    if(i == backup_size){
        puts("Verify OK.\n");
        return MT_SUCCESS;
    }
    return MT_FAIL;
}

static int serial_nor_memory_test(int argc, char * const argv[], spif_cmd_parsing_info_t *info)
{
    uint32_t i;
    uint32_t mt_size;
    uint32_t mt_start;
    uint32_t backup_size;

    for(i=2 ; i<argc ; i++)    {
        if((strcmp(argv[i],"-loops") == 0) || (strcmp(argv[i],"-l") == 0)){
            info->test_loops = simple_strtoul(argv[++i], NULL, 10);
        }else if((strcmp (argv[i], "-range") == 0) || (strcmp (argv[i], "-r") == 0)){
            info->mt_offset = simple_strtoul(argv[++i], NULL, 16);
            info->mt_size= simple_strtoul(argv[++i], NULL, 16);
            info->assign_test_range =1;
        }
    }

    info->test_loops = (info->test_loops==0)?1:(info->test_loops);
    info->mt_size    = (info->mt_size < SIZE_4KB)?SIZE_4KB:info->mt_size;
    if (info->mt_offset > g_mnor_test->size) ENG_CMD_ILL;
    if (info->mt_offset%SIZE_4KB) ENG_CMD_ILL;
    if ((info->mt_offset+info->mt_size) > g_mnor_test->size) ENG_CMD_ILL;
    printf("[mnor_test]: spi nor chip size is %2dMB/%dKB\n",(uint32_t)(uintptr_t)(g_mnor_test->size/SIZE_1MB),(uint32_t)(uintptr_t)(g_mnor_test->size/1024));

    if (info->assign_test_range) {
        mt_size = info->mt_size;
        mt_start= info->mt_offset + CONFIG_SYS_FLASH_BASE;
        backup_size = mt_size;
        printf("             backup_size is %2d MB / %d KB\n",backup_size/SIZE_1MB,backup_size/1024);
        printf("             test_size   is %2d MB / %d KB\n",mt_size/SIZE_1MB,mt_size/1024);

        if (mt_size >= MAX_BACKUP_SIZE) {
            backup_size = 16*1024*1024;
            printf("[Warning] Assigned test size is %d MB, DRAM_Size is %d MB => Only backup 16MB flash content\n", (mt_size/SIZE_1MB), (MAX_BACKUP_SIZE/SIZE_1MB));
        }

        /* Step1: Back up and verify the assigned spif area data. */
        back_up_spif_data(mt_start, backup_size);

        /* Step2: Do mflash_test */
        for (i=0 ; i<info->test_loops ; i++) {
            if(_flash_test(mt_start, TEST_SIZE_PER_PATTREN, mt_size)){
                printf("[ERROR] Just try to recover data ...\n");
                restore_spif_data(mt_start, backup_size);
                return MT_FAIL;
            }
            printf("\n[mnor_test] %d runs OK\n",i+1);
        }

        /* Step3: Copy back and verify data into the flash. */
        if (restore_spif_data(mt_start, backup_size))
            return MT_FAIL;
    }else {
        mt_size = SIZE_2MB;

        for (i=0; i<info->test_loops; i++) {
            uint32_t mt_start_mb_list[] = {64*SIZE_1MB, 48*SIZE_1MB, 32*SIZE_1MB,
                                           18*SIZE_1MB, 16*SIZE_1MB, 8*SIZE_1MB,
                                            4*SIZE_1MB};
            uint32_t msl_idx;
            uint32_t part_num = 0;

            for (msl_idx=0; msl_idx < ARY_LEN(mt_start_mb_list); msl_idx++) {
                if (mt_start_mb_list[msl_idx] > g_mnor_test->size) {
                    continue;
                }

                mt_start = CONFIG_SYS_FLASH_BASE + mt_start_mb_list[msl_idx] - mt_size;
                printf("\nRegion(%d): 0x%08x ~ 0x%08x\n", part_num++, mt_start, (mt_start+mt_size));

                /* Step1: Back up and verify the assigned spif area data. */
                back_up_spif_data(mt_start, backup_size);

                /* Step2: Do mflash_test */
                if(_flash_test(mt_start, TEST_SIZE_PER_PATTREN, mt_size)){
                    printf("[ERROR] Just try to recover data ...\n");
                    restore_spif_data(mt_start, backup_size);
                    return MT_FAIL;
                }

                /* Step3: Copy back and verify data into the flash. */
                if (restore_spif_data(mt_start, backup_size))
                    return MT_FAIL;
            }
            printf("\n[mnor_test] %d runs OK\n", i);
        }
    }

    return MT_SUCCESS;
}

static int spif_cmd_parsing(int argc, char * const argv[], spif_cmd_parsing_info_t *info)
{
    uint32_t i;

    /* Parse command flag for test range / test loops / mt error (non-)blocking mode */
    for(i=1 ; i<argc ;)    {
        if(strcmp(argv[i],"-mt") == 0) {
            return (serial_nor_memory_test(argc, argv, info));
        }else if((strcmp (argv[i], "-ce") == 0) || (strcmp (argv[i], "-chip_erase") == 0)){
            i++;
            return flash_chip_erase_test();
        }else if((strcmp (argv[i], "-sc") == 0) || (strcmp (argv[i], "-set_clk") == 0)){
            spi_nor_set_clock(simple_strtoul(argv[++i], NULL, 10));
            return 0;
        }else{
            ENG_CMD_ILL;
        }
    }
    return 0;
}

int do_serial_nor_eng_cmd(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
    spif_cmd_parsing_info_t cmd_info = {0};

    g_mnor_test = spi_flash_probe(0, 0, 0, 0);
    if (!g_mnor_test) {
        printf("spi flash 0 probe failed\n");
        return 0;
    }

    if(spif_cmd_parsing(argc, argv, &cmd_info)) {
        return MT_ILL_CMD;
    }

    return MT_SUCCESS;
}

U_BOOT_CMD(
    mnor_eng, 10, 1, do_serial_nor_eng_cmd,
    "mnor_eng  - do spi-nor flash engineer command. ",
    "-mt -r/-range <start_offset> <test_size (64KB alignment)> [-l/-loops <test loops>]\n"\
    "-ce/-chip_erase: doing chip erase and check erased data as ALL-ONE\n"\
);

