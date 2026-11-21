#include <common.h>
#include "serdes_common.h"
#if defined(CONFIG_TARGET_RTK_TAURUS)
#include "../smc_call/cortina_smc.h"
#endif

#if defined(CONFIG_TARGET_RTK_TAURUS)
unsigned int __rtk_sds_get_chip_revision(void)
{
	char cut[2] = {SMC_CALL_GET_SOC_VERSION(), 0};
	if (!strcmp(cut, "A"))
		return RTK_SDS_CHIP_REVISION_A;
	else if (!strcmp(cut, "B"))
		return RTK_SDS_CHIP_REVISION_B;
	else if (!strcmp(cut, "C"))
		return RTK_SDS_CHIP_REVISION_C;
	else
		return RTK_SDS_CHIP_REVISION_A;
}
#else
unsigned int __rtk_sds_get_chip_revision(void)
{
	return RTK_SDS_CHIP_REVISION_A;
}
#endif

