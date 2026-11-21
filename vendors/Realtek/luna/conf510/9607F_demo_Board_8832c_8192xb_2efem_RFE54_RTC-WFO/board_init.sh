#!/bin/sh

#USB2.0/3.0 LED Trigger
#If PORT1 exist , you need to add other trigger.
# echo "1-1,2-1" > /sys/class/leds/LED_USB0/device_name
# echo "1-2" > /sys/class/leds/LED_USB1/device_name

#NETDEV LED Trigger
#netdev

pon_mode=`mib get PON_MODE`

echo "eth0.2"  > /sys/class/leds/LED_EPHY0/device_name
echo "eth0.3"  > /sys/class/leds/LED_EPHY1/device_name
echo "eth0.4"  > /sys/class/leds/LED_EPHY2/device_name

if [ "$pon_mode" == "PON_MODE=0" ] ; then
	echo "nas0"  > /sys/class/leds/LED_EPHY3/device_name
else
	echo "eth0.5"  > /sys/class/leds/LED_EPHY3/device_name
fi



