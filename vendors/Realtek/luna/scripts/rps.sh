#!/bin/sh
cpu=`cat /proc/cpuinfo | grep processor | wc -l`
case "$1" in
	on)
		case "$cpu" in
			4)
				echo "4 cpu"
					RPS_cpus=f
				;;
			2)
				echo "2 cpu"
                                        RPS_cpus=3
                ;;
			*)
				echo "OTHERS"
					RPS_cpus=0
				exit 1
				;;
		esac
		RPS_flow_cnt=4096
		;;
	off)
		RPS_cpus=0
		RPS_flow_cnt=0
		;;
	*)
		echo "$0 <on|off>"
		exit 1
		;;
esac

for f in /sys/class/net/*; do
	SET=1
	case "$f" in
	/sys/class/net/eth0.2)
		;;
	/sys/class/net/eth0.3)
		;;
	/sys/class/net/eth0.4)
		;;
	/sys/class/net/eth0.5)
		;;
	/sys/class/net/nas0)
		;;
	*)
		SET=0
		;;
	esac

	if [ "$SET" -eq 1 ]; then
		echo $RPS_cpus > $f/queues/rx-0/rps_cpus
		echo $RPS_flow_cnt > $f/queues/rx-0/rps_flow_cnt
	fi
	if [ "$SET" -eq 2 ]; then
                echo 4 > $f/queues/rx-0/rps_cpus
                echo $RPS_flow_cnt > $f/queues/rx-0/rps_flow_cnt
        fi

done
