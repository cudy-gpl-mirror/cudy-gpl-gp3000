#include <common.h>
#include <linux/libfdt.h>
#include <fdt_support.h>
#include <env.h>
#include <linux/string.h>
#include <malloc.h>

void board_env_fix(void) {
        ulong __maybe_unused val;
	int error=0;
#ifdef TARGET_BL_SIZE
        ulong fip_size;
        val = env_get_hex("pfl_boot_sz",0);
        if ( val != TARGET_BL_SIZE){
		error=1;
                printf("warning: pfl_boot_sz is not consistent!\n");
                printf("         u-boot header uses (0x%08x), but env file uses (0x%08lx).\n", TARGET_BL_SIZE, val);
                env_set_hex("pfl_boot_sz", TARGET_BL_SIZE);
                printf("         force pfl_boot_sz to 0x%08lx\n",  env_get_hex("pfl_boot_sz",0));
        }
        val = env_get_hex("pfl_fip_sz",0);
        /* fip_size must 256KB alignment */
        fip_size = (TARGET_BL_SIZE - 0x40000)/2/0x40000;
        fip_size = fip_size * 0x40000;
        if ( val != fip_size){
		error=1;
                printf("warning: fip_size is not consistent!\n");
                printf("         u-boot header uses (0x%08lx), but env file uses (0x%08lx).\n", fip_size, val);
                env_set_hex("pfl_fip_size", fip_size);
                printf("         force pfl_fip_size to 0x%08lx\n",  env_get_hex("pfl_fip_size",0));
        }
        //pfl_env1_r
        val = env_get_hex("pfl_env1_r",0);
        if (val != CONFIG_ENV_OFFSET) {
		error=1;
                printf("warning: CONFIG_ENV_OFFSET is not consistent\n");
                printf("         u-boot header uses (0x%08x), env file uses (0x%08lx).\n", CONFIG_ENV_OFFSET, val);
                env_set_hex("pfl_env1_r", CONFIG_ENV_OFFSET);
                printf("         force pfl_env1_r value to 0x%08lx\n",  env_get_hex("pfl_env1_r",0));

        }
        val = env_get_hex("pfl_env2_r",0);
        if (val != CONFIG_ENV_OFFSET_REDUND) {
		error=1;
                printf("warning: CONFIG_ENV_OFFSET_REDUND is not consistent\n");
                printf("         u-boot header uses (0x%08x), but env file uses (0x%08lx).\n", CONFIG_ENV_OFFSET_REDUND, val);
                env_set_hex("pfl_env2_r", CONFIG_ENV_OFFSET_REDUND);
                printf("         force pfl_env2_r value to 0x%08lx\n",  env_get_hex("pfl_env2_r",0));
	}
#else
#ifdef CONFIG_ENV_OFFSET
        val = env_get_hex("pfl_env1_r",0);
        if (val != CONFIG_ENV_OFFSET) {
		error=1;
                printf("warning: CONFIG_ENV_OFFSET is not consistent\n");
                printf("         u-boot header uses (0x%08x), env file uses (0x%08lx).\n", CONFIG_ENV_OFFSET, val);
        }
        val = env_get_hex("pfl_env2_r",0);
        if (val != CONFIG_ENV_OFFSET_REDUND) {
		error=1;
                printf("warning: CONFIG_ENV_OFFSET_REDUND is not consistent\n");
                printf("         u-boot header uses (0x%08x), but env file uses (0x%08lx).\n", CONFIG_ENV_OFFSET_REDUND, val);
	}
#endif
#endif
	if(error)
		printf("Please fix default environment file. Refer to CONFIG_DEFAULT_ENV_FILE\n");
}

