#!/bin/sh

PIDFILE=/var/run/udhcpd.pid
RUN_SCRIPT=/var/udhcpc/udhcpc.$interface
RETRY_COUNT=50

UPNP=`flash get UPNP`
UPNP=${UPNP/UPNP=/}

if [ $1 = 'bound' ] || [ $1 = 'renew' ]; then
	retry=0
	while [ $retry -lt $RETRY_COUNT ]
	do
		if [ -f "$RUN_SCRIPT" ]; then
			/bin/sh -c $RUN_SCRIPT
			if [ $? -eq 0 ]; then
				break
			fi
		fi
		let retry++
		echo "Fail to run $RUN_SCRIPT !!, retry $retry..."
		if [ -f /bin/lsof -a -f $RUN_SCRIPT ]; then
			/bin/lsof | grep $RUN_SCRIPT
		fi
		sleep 6
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
			echo "Kick dhcrelay to sync interface-2"
			kill -SIGUSR2 $PID
		fi
	fi
	echo "............send usr2 to boa" > /dev/console
	kill -SIGUSR2 $(cat /var/run/boa.pid)
elif [ $1 = 'deconfig' ]; then
	/etc/scripts/udhcpc.deconfig
	
elif [ $1 = 'nak' ]; then
	echo "dhcp nak event occurs"
fi
