#!/bin/sh

retry_cnt=0

#If pon init fails, the following commands are required to generate pclk
#-------- 9607F --------
#devmem 0xf432009c 32 0x0000030f
#devmem 0xf550a02c 32 0x0000045A
#devmem 0xf432025c 32 0x00000020
#-------- Mercury --------
#devmem 0xf432006c 32 0x00000F0F
#devmem 0xf550a02c 32 0x0000040C

modprobe snd-soc-rtl8277x-pinctrl.ko

while [ 1 ]; do
	retry_cnt=$((retry_cnt+1))
	
	echo "test $retry_cnt..."
	
	modprobe snd-soc-rtl8277x.ko debug=8 pattern=0x59
	modprobe snd-soc-rtl8277x-diag.ko pclk=4096000 loopback=2 ifmode=0 test_item=0
	
	sleep 5

	if [ $retry_cnt -eq 20 ]; then
		cat /proc/interrupts
		sleep 1
	fi
	
	echo "remove snd-soc-rtl8277x-diag.ko..."
	echo "remove snd-soc-rtl8277x.ko..."
	rmmod snd-soc-rtl8277x-diag.ko
	rmmod snd-soc-rtl8277x.ko
	
	sleep 5

	if [ $retry_cnt -eq 20 ]; then
		echo "pcm loopback test done!!"
       exit 0
	fi
done