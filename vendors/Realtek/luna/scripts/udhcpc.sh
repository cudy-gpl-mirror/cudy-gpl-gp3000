#!/bin/sh

PIDFILE=/var/run/dhcrelay.pid
RUN_SCRIPT=/var/udhcpc/udhcpc.$interface

UPNP=`flash get UPNP`
UPNP=${UPNP/UPNP=/}

if [ $1 = 'bound' ] || [ $1 = 'renew' ]; then
	retry=0
	while [ $retry -lt 3 ]
	do
		if [ -e "$RUN_SCRIPT" ]; then
			/bin/sh -c $RUN_SCRIPT
			if [ $? -eq 0 ]; then
				break
			fi
		fi
		let retry++
		echo "Fail to run $RUN_SCRIPT !!, retry $retry..."
		sleep 3
	done

	if [ "$ip" != '' ]; then
		if [ "$UPNP" = "1" ]; then
			/bin/upnpctrl down $interface br0
			/bin/upnpctrl up $interface br0
		fi
		/bin/updateddctrl $interface 1
		/bin/ethctl route add		
		if [ -f $PIDFILE ]; then
			PID=`cat $PIDFILE`
			echo "Kick dhcrelay to sync interface-1"
			kill -17 $PID		
		fi
	fi
elif [ $1 = 'deconfig' ]; then
	/etc/scripts/udhcpc.deconfig
	
elif [ $1 = 'nak' ]; then
	echo "dhcp nak event occurs"
fi
