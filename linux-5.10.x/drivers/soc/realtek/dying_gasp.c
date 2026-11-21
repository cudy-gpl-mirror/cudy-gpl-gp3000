// SPDX-License-Identifier: GPL-2.0-only

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>

extern void rtk_soc_reboot_reason_set(int reason);
static int reboot_reason_dyg(struct notifier_block *nb, unsigned long event, void *ptr)
{
	rtk_soc_reboot_reason_set(4);

	return NOTIFY_DONE;
}

static struct notifier_block reboot_reason_block = {
	reboot_reason_dyg,
	NULL,
	INT_MAX
};

static ATOMIC_NOTIFIER_HEAD(dyinggasp_notifier_list);

int register_dyinggasp_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&dyinggasp_notifier_list, nb);
}
EXPORT_SYMBOL(register_dyinggasp_notifier);

int unregister_dyinggasp_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&dyinggasp_notifier_list, nb);
}
EXPORT_SYMBOL(unregister_dyinggasp_notifier);

int call_dyinggasp_notifiers(void)
{
	return atomic_notifier_call_chain(&dyinggasp_notifier_list, 0, NULL);
}
EXPORT_SYMBOL(call_dyinggasp_notifiers);

static int __init dying_gasp_init(void)
{
	register_dyinggasp_notifier(&reboot_reason_block);
	return 0;
}

module_init(dying_gasp_init);
