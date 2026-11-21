// SPDX-License-Identifier: GPL-2.0
//
// LED Kernel Netdev Trigger
//
// Toggles the LED to reflect the link and traffic state of named net devices
//

#include <linux/atomic.h>
#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include "../leds.h"

#define DEV_NUM	4
#define DEV_NAME_BUFFER_SIZE 12

/*
 * Configurable sysfs attributes:
 *
 * device_name - network device name to monitor
 * interval - duration of LED blink, in milliseconds
 * link -  LED's normal state reflects whether the link is up
 *         (has carrier) or not
 * tx -  LED blinks on transmitted data
 * rx -  LED blinks on receive data
 *
 */
struct ndevice {
	char device_name[DEV_NAME_BUFFER_SIZE];
	struct net_device *net_dev;
};


struct led_netdev_data {
	spinlock_t lock;

	struct delayed_work work;
	struct notifier_block notifier;

	struct led_classdev *led_cdev;
	struct ndevice devs[DEV_NUM];
	int ndevs;

	atomic_t interval;
	unsigned int last_activity;

	unsigned long mode;
#define NETDEV_LED_LINK	0
#define NETDEV_LED_TX	1
#define NETDEV_LED_RX	2
#define NETDEV_LED_MODE_LINKUP	3
};

enum netdev_led_attr {
	NETDEV_ATTR_LINK,
	NETDEV_ATTR_TX,
	NETDEV_ATTR_RX
};

static void set_baseline_state(struct led_netdev_data *trigger_data)
{
	int current_brightness;
	struct led_classdev *led_cdev = trigger_data->led_cdev;

	current_brightness = led_cdev->brightness;
	if (current_brightness)
		led_cdev->blink_brightness = current_brightness;
	if (!led_cdev->blink_brightness)
		led_cdev->blink_brightness = led_cdev->max_brightness;

	if (!test_bit(NETDEV_LED_MODE_LINKUP, &trigger_data->mode))
		led_set_brightness(led_cdev, LED_OFF);
	else {
		if (test_bit(NETDEV_LED_LINK, &trigger_data->mode))
			led_set_brightness(led_cdev,
					   led_cdev->blink_brightness);
		else
			led_set_brightness(led_cdev, LED_OFF);

		/* If we are looking for RX/TX start periodically
		 * checking stats
		 */
		if (test_bit(NETDEV_LED_TX, &trigger_data->mode) ||
		    test_bit(NETDEV_LED_RX, &trigger_data->mode))
			schedule_delayed_work(&trigger_data->work, 0);
	}
}

static ssize_t device_name_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct led_netdev_data *trigger_data = led_trigger_get_drvdata(dev);
	ssize_t len = 0;
	int n;

	spin_lock_bh(&trigger_data->lock);
	for (n = 0; n < trigger_data->ndevs; n++) {
		if (n != (trigger_data->ndevs-1))
			len += sprintf(&buf[len], "%s,", trigger_data->devs[n].device_name);
		else
			len += sprintf(&buf[len], "%s\n", trigger_data->devs[n].device_name);
	}
	spin_unlock_bh(&trigger_data->lock);

	return len;
}

static ssize_t device_name_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	struct led_netdev_data *trigger_data = led_trigger_get_drvdata(dev);
	const char *devnames[DEV_NUM], *tmp0, *tmp1;
	int devnamelens[DEV_NUM], n, ndevs = 0, stop = 0;

	tmp0 = buf;
	while (!stop) {
		tmp1 = strchr(tmp0, ',');
		if (!tmp1) {
			tmp1 = strchr(tmp0, '\n');
			if (tmp1 == NULL)
				tmp1 = strchr(tmp0, '\0');
			stop = 1;
		}
		devnames[ndevs] = tmp0;
		devnamelens[ndevs] = tmp1-tmp0;
		if ((devnamelens[ndevs] >= DEV_NAME_BUFFER_SIZE) || (devnamelens[ndevs] == 0))
			return -EINVAL;
		tmp0 = tmp1+1;
		ndevs++;
		if (ndevs >= DEV_NUM)
			stop = 1;
	}

	if (size >= IFNAMSIZ * DEV_NUM)
		return -EINVAL;

	cancel_delayed_work_sync(&trigger_data->work);
	spin_lock_bh(&trigger_data->lock);
	trigger_data->ndevs = ndevs;

	for (n = 0; n < DEV_NUM; n++) {
		if (trigger_data->devs[n].net_dev) {
			dev_put(trigger_data->devs[n].net_dev);
			trigger_data->devs[n].net_dev = NULL;
		}
	}
	clear_bit(NETDEV_LED_MODE_LINKUP, &trigger_data->mode);

	if (ndevs > 0) {
		for (n = 0; n < trigger_data->ndevs; n++) {
			strncpy(trigger_data->devs[n].device_name, devnames[n], devnamelens[n]);
			trigger_data->devs[n].device_name[devnamelens[n]] = 0;
			if (devnamelens[n] > 0 && trigger_data->devs[n].device_name[devnamelens[n] - 1] == '\n')
				trigger_data->devs[n].device_name[devnamelens[n] - 1] = 0;

			trigger_data->devs[n].net_dev =
				dev_get_by_name(&init_net, trigger_data->devs[n].device_name);

			if (trigger_data->devs[n].net_dev != NULL)
				if (netif_carrier_ok(trigger_data->devs[n].net_dev))
					set_bit(NETDEV_LED_MODE_LINKUP, &trigger_data->mode);
		}
	}

	trigger_data->last_activity = 0;

	set_baseline_state(trigger_data);
	spin_unlock_bh(&trigger_data->lock);

	return size;
}

static DEVICE_ATTR_RW(device_name);

static ssize_t netdev_led_attr_show(struct device *dev, char *buf,
	enum netdev_led_attr attr)
{
	struct led_netdev_data *trigger_data = led_trigger_get_drvdata(dev);
	int bit;

	switch (attr) {
	case NETDEV_ATTR_LINK:
		bit = NETDEV_LED_LINK;
		break;
	case NETDEV_ATTR_TX:
		bit = NETDEV_LED_TX;
		break;
	case NETDEV_ATTR_RX:
		bit = NETDEV_LED_RX;
		break;
	default:
		return -EINVAL;
	}

	return sprintf(buf, "%u\n", test_bit(bit, &trigger_data->mode));
}

static ssize_t netdev_led_attr_store(struct device *dev, const char *buf,
	size_t size, enum netdev_led_attr attr)
{
	struct led_netdev_data *trigger_data = led_trigger_get_drvdata(dev);
	unsigned long state;
	int ret;
	int bit;

	ret = kstrtoul(buf, 0, &state);
	if (ret)
		return ret;

	switch (attr) {
	case NETDEV_ATTR_LINK:
		bit = NETDEV_LED_LINK;
		break;
	case NETDEV_ATTR_TX:
		bit = NETDEV_LED_TX;
		break;
	case NETDEV_ATTR_RX:
		bit = NETDEV_LED_RX;
		break;
	default:
		return -EINVAL;
	}

	cancel_delayed_work_sync(&trigger_data->work);

	if (state)
		set_bit(bit, &trigger_data->mode);
	else
		clear_bit(bit, &trigger_data->mode);

	set_baseline_state(trigger_data);

	return size;
}

static ssize_t link_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return netdev_led_attr_show(dev, buf, NETDEV_ATTR_LINK);
}

static ssize_t link_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	return netdev_led_attr_store(dev, buf, size, NETDEV_ATTR_LINK);
}

static DEVICE_ATTR_RW(link);

static ssize_t tx_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return netdev_led_attr_show(dev, buf, NETDEV_ATTR_TX);
}

static ssize_t tx_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	return netdev_led_attr_store(dev, buf, size, NETDEV_ATTR_TX);
}

static DEVICE_ATTR_RW(tx);

static ssize_t rx_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return netdev_led_attr_show(dev, buf, NETDEV_ATTR_RX);
}

static ssize_t rx_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	return netdev_led_attr_store(dev, buf, size, NETDEV_ATTR_RX);
}

static DEVICE_ATTR_RW(rx);

static ssize_t interval_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct led_netdev_data *trigger_data = led_trigger_get_drvdata(dev);

	return sprintf(buf, "%u\n",
		       jiffies_to_msecs(atomic_read(&trigger_data->interval)));
}

static ssize_t interval_store(struct device *dev,
			      struct device_attribute *attr, const char *buf,
			      size_t size)
{
	struct led_netdev_data *trigger_data = led_trigger_get_drvdata(dev);
	unsigned long value;
	int ret;

	ret = kstrtoul(buf, 0, &value);
	if (ret)
		return ret;

	/* impose some basic bounds on the timer interval */
	if (value >= 5 && value <= 10000) {
		cancel_delayed_work_sync(&trigger_data->work);

		atomic_set(&trigger_data->interval, msecs_to_jiffies(value));
		set_baseline_state(trigger_data);	/* resets timer */
	}

	return size;
}

static DEVICE_ATTR_RW(interval);

static struct attribute *netdev_trig_attrs[] = {
	&dev_attr_device_name.attr,
	&dev_attr_link.attr,
	&dev_attr_rx.attr,
	&dev_attr_tx.attr,
	&dev_attr_interval.attr,
	NULL
};
ATTRIBUTE_GROUPS(netdev_trig);

static int netdev_trig_notify(struct notifier_block *nb,
			      unsigned long evt, void *dv)
{
	struct net_device *dev =
		netdev_notifier_info_to_dev((struct netdev_notifier_info *)dv);
	struct led_netdev_data *trigger_data =
		container_of(nb, struct led_netdev_data, notifier);
	int n;

	if (evt != NETDEV_UP && evt != NETDEV_DOWN && evt != NETDEV_CHANGE
	    && evt != NETDEV_REGISTER && evt != NETDEV_UNREGISTER
	    && evt != NETDEV_CHANGENAME)
		return NOTIFY_DONE;

	for (n = 0; n < trigger_data->ndevs; n++) {
		if (!(dev == trigger_data->devs[n].net_dev ||
			  (evt == NETDEV_CHANGENAME && !strcmp(dev->name, trigger_data->devs[n].device_name)) ||
			  (evt == NETDEV_REGISTER && !strcmp(dev->name, trigger_data->devs[n].device_name)))) {
			if (n + 1 >= trigger_data->ndevs)
				return NOTIFY_DONE;
			continue;
		} else
			break;
	}

	cancel_delayed_work_sync(&trigger_data->work);

	spin_lock_bh(&trigger_data->lock);

	clear_bit(NETDEV_LED_MODE_LINKUP, &trigger_data->mode);
	switch (evt) {
	case NETDEV_CHANGENAME:
	case NETDEV_REGISTER:
		if (trigger_data->devs[n].net_dev)
			dev_put(trigger_data->devs[n].net_dev);
		dev_hold(dev);
		trigger_data->devs[n].net_dev = dev;
		break;
	case NETDEV_UNREGISTER:
		if (trigger_data->devs[n].net_dev)
			dev_put(trigger_data->devs[n].net_dev);
		trigger_data->devs[n].net_dev = NULL;
		break;
	case NETDEV_UP:
	case NETDEV_CHANGE:
		for (n = 0; n < trigger_data->ndevs; n++) {
			if (trigger_data->devs[n].net_dev != NULL)
				if (netif_carrier_ok(trigger_data->devs[n].net_dev))
					set_bit(NETDEV_LED_MODE_LINKUP, &trigger_data->mode);
		}
		break;
	}

	set_baseline_state(trigger_data);

	spin_unlock_bh(&trigger_data->lock);

	return NOTIFY_DONE;
}

/* here's the real work! */
static void netdev_trig_work(struct work_struct *work)
{
	struct led_netdev_data *trigger_data =
		container_of(work, struct led_netdev_data, work.work);
	struct rtnl_link_stats64 *dev_stats;
	unsigned int new_activity = 0;
	struct rtnl_link_stats64 temp;
	unsigned long interval;
	int invert;
	int n;

	/* If we dont have a device, insure we are off */
	for (n = 0; n < trigger_data->ndevs; n++) {
		if (!trigger_data->devs[n].net_dev) {
			if (n + 1 >= trigger_data->ndevs)
				led_set_brightness(trigger_data->led_cdev, LED_OFF);
		} else
			break;
	}

	/* If we are not looking for RX/TX then return  */
	if (!test_bit(NETDEV_LED_TX, &trigger_data->mode) &&
	    !test_bit(NETDEV_LED_RX, &trigger_data->mode))
		return;

	for (n = 0; n < trigger_data->ndevs; n++) {
		if (!trigger_data->devs[n].net_dev)
			continue;
		dev_stats = dev_get_stats(trigger_data->devs[n].net_dev, &temp);
		new_activity +=
			(test_bit(NETDEV_LED_TX, &trigger_data->mode) ?
			dev_stats->tx_packets : 0) +
			(test_bit(NETDEV_LED_RX, &trigger_data->mode) ?
			dev_stats->rx_packets : 0);
	}

	if (trigger_data->last_activity != new_activity) {
		led_stop_software_blink(trigger_data->led_cdev);

		invert = test_bit(NETDEV_LED_LINK, &trigger_data->mode);
		interval = jiffies_to_msecs(
				atomic_read(&trigger_data->interval));
		/* base state is ON (link present) */
		led_blink_set_oneshot(trigger_data->led_cdev,
				      &interval,
				      &interval,
				      invert);
		trigger_data->last_activity = new_activity;
	}

	schedule_delayed_work(&trigger_data->work,
			(atomic_read(&trigger_data->interval)*2));
}

static int netdev_trig_activate(struct led_classdev *led_cdev)
{
	struct led_netdev_data *trigger_data;
	int rc;
	int n;

	trigger_data = kzalloc(sizeof(struct led_netdev_data), GFP_KERNEL);
	if (!trigger_data)
		return -ENOMEM;

	spin_lock_init(&trigger_data->lock);

	trigger_data->notifier.notifier_call = netdev_trig_notify;
	trigger_data->notifier.priority = 10;

	INIT_DELAYED_WORK(&trigger_data->work, netdev_trig_work);

	trigger_data->led_cdev = led_cdev;
	for (n = 0; n < DEV_NUM; n++) {
		trigger_data->devs[n].net_dev = NULL;
		trigger_data->devs[n].device_name[0] = 0;
	}

	trigger_data->mode = BIT_MASK(NETDEV_LED_LINK)|BIT_MASK(NETDEV_ATTR_RX)|BIT_MASK(NETDEV_ATTR_TX);
	atomic_set(&trigger_data->interval, msecs_to_jiffies(50));
	trigger_data->last_activity = 0;

	led_set_trigger_data(led_cdev, trigger_data);

	rc = register_netdevice_notifier(&trigger_data->notifier);
	if (rc)
		kfree(trigger_data);

	return rc;
}

static void netdev_trig_deactivate(struct led_classdev *led_cdev)
{
	struct led_netdev_data *trigger_data = led_get_trigger_data(led_cdev);
	int n;

	unregister_netdevice_notifier(&trigger_data->notifier);

	cancel_delayed_work_sync(&trigger_data->work);

	for (n = 0; n < DEV_NUM; n++) {
		if (trigger_data->devs[n].net_dev)
			dev_put(trigger_data->devs[n].net_dev);
	}

	kfree(trigger_data);
}

static struct led_trigger rtk_multi_netdev_led_trigger = {
	.name = "rtk_multi-netdev",
	.activate = netdev_trig_activate,
	.deactivate = netdev_trig_deactivate,
	.groups = netdev_trig_groups,
};

static int __init rtk_multi_netdev_trig_init(void)
{
	return led_trigger_register(&rtk_multi_netdev_led_trigger);
}

static void __exit rtk_multi_netdev_trig_exit(void)
{
	led_trigger_unregister(&rtk_multi_netdev_led_trigger);
}

module_init(rtk_multi_netdev_trig_init);
module_exit(rtk_multi_netdev_trig_exit);

MODULE_DESCRIPTION("Multiple Netdev LED trigger");
MODULE_LICENSE("GPL v2");
