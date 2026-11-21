#!/bin/sh

# System Settings - START
SYSTEMD_USOCK_SERVER="/var/run/systemd.usock"
# System Settings - END

AP_EVENT_ENABLED="AP-ENABLED"
AP_EVENT_DISABLED="AP-DISABLED"
WPA_EVENT_CHANNEL_SWITCH="CTRL-EVENT-CHANNEL-SWITCH"
AP_STA_CONNECTED="AP-STA-CONNECTED"
AP_STA_DISCONNECTED="AP-STA-DISCONNECTED"
AP_STA_POSSIBLE_PSK_MISMATCH="AP-STA-POSSIBLE-PSK-MISMATCH"
WPA_EVENT_AUTH_REJECT="CTRL-EVENT-AUTH-REJECT"

# DPP Events
DPP_CHIRP_RX="DPP-CHIRP-RX"

map_file=/var/multiap_mib.conf
tmpvar="global"

#echo $@

if [ "$1" = "$tmpvar" ]; then
case "$3" in
$AP_EVENT_ENABLED)
#       sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_channel_change_event $2
        exit 0
        ;;
$AP_EVENT_DISABLED)
        exit 0
        ;;
$WPA_EVENT_CHANNEL_SWITCH)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_channel_change_event $2
        exit 0
        ;;
$AP_STA_CONNECTED)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_ap_sta_connect_event $2 $4
        if [ -f "$map_file" ]; then
                sysconf send_map_unix_sock_message $2 $3 $4
        fi
        exit 0
        ;;
$AP_STA_DISCONNECTED)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_ap_sta_disconnect_event $2 $4
        if [ -f "$map_file" ]; then
                sysconf send_map_unix_sock_message $2 $3 $4
        fi
        exit 0
        ;;
$AP_STA_POSSIBLE_PSK_MISMATCH)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_ap_sta_psk_mismatch_event $2 $4
		if [ -f "$map_file" ]; then
			sysconf send_map_unix_sock_message $2 $3 $4
		fi
        exit 0
        ;;
$WPA_EVENT_AUTH_REJECT)
	sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_ap_sta_auth_reject_event $2 $4
                if [ -f "$map_file" ]; then
                        sysconf send_map_unix_sock_message $2 $3 $4
                fi
        exit 0
        ;;
$DPP_CHIRP_RX)
		if [ -f "$map_file" ]; then
			sysconf send_map_unix_sock_message $2 $3 $4 $5 $6 $7
		fi
        exit 0
        ;;
*)
        exit 1
        ;;
esac

else

case "$2" in
$AP_EVENT_ENABLED)
#	sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_channel_change_event $1
	exit 0
	;;
$AP_EVENT_DISABLED)
	exit 0
	;;
$WPA_EVENT_CHANNEL_SWITCH)
	sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_channel_change_event $1
	exit 0
	;;
$AP_STA_CONNECTED)
	sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_ap_sta_connect_event $1 $3
	if [ -f "$map_file" ]; then
		sysconf send_map_unix_sock_message $1 $2 $3
	fi
	exit 0
	;;
$AP_STA_DISCONNECTED)
	sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_ap_sta_disconnect_event $1 $3
	if [ -f "$map_file" ]; then
		sysconf send_map_unix_sock_message $1 $2 $3
	fi
	exit 0
	;;
$AP_STA_POSSIBLE_PSK_MISMATCH)
	sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_ap_sta_psk_mismatch_event $1 $3
	if [ -f "$map_file" ]; then
		sysconf send_map_unix_sock_message $1 $2 $3
	fi
	exit 0
	;;
$WPA_EVENT_AUTH_REJECT)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_ap_sta_auth_reject_event $1 $3
                if [ -f "$map_file" ]; then
                        sysconf send_map_unix_sock_message $1 $2 $3
                fi
        exit 0
        ;;
$DPP_CHIRP_RX)
	if [ -f "$map_file" ]; then
		sysconf send_map_unix_sock_message $1 $2 $3 $4 $5 $6 $7
	fi
	exit 0
	;;
*)
	exit 1
	;;
esac
fi
