#!/bin/sh

# System Settings - START
SYSTEMD_USOCK_SERVER="/var/run/systemd.usock"
# System Settings - END

STA_CONNECTED="CONNECTED"
STA_DISCONNECTED="DISCONNECTED"
STA_AUTHENTICATION_FAILURE="AUTHENTICATION-FAILURE"
STA_CONNECT_TIMEOUT="TIMEOUT"

# DPP Events
DPP_EVENT_CONNECTOR_STORE="DPP-CONNECTOR-STORE"
DPP_EVENT_CONNECTOR_1905_STORE="DPP-CONNECTOR-1905-STORE"
DPP_EVENT_C_SIGN_KEY_STORE="DPP-C-SIGN-KEY-STORE"
DPP_EVENT_PP_KEY_STORE="DPP-PP-KEY-STORE"
DPP_EVENT_NET_ACCESS_KEY_STORE="DPP-NET-ACCESS-KEY-STORE"
DPP_EVENT_CONFOBJ_SSID_STORE="DPP-CONFOBJ-SSID-STORE"
DPP_EVENT_CONFOBJ_PASS_STORE="DPP-CONFOBJ-PASS-STORE"
DPP_EVENT_CONFOBJ_AKM_STORE="DPP-CONFOBJ-AKM-STORE"
DPP_EVENT_CONFOBJ_PARSE_SUCCESS="DPP-CONFOBJ-PARSE_SUCCESS"

MAP_FILE=/var/multiap_mib.conf
DPP_CONN_FILE=/tmp/dpp_map_conn
DPP_RECONFIG_SCRIPT_PATH="/tmp/dpp_reconfig.sh"
DPP_RECONFIG_FLAG="/tmp/dpp_reconfig_flag"

#echo $@

case "$2" in
$STA_CONNECTED)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_sta_connect_event $1
        if [ -f "$MAP_FILE" ]; then
                sysconf send_map_unix_sock_message $1 $2
        fi
        if [ -f "$DPP_CONN_FILE" ]; then
                rm $DPP_RECONFIG_FLAG
        fi
        exit 0
        ;;
$STA_DISCONNECTED)
        if [ -f "$DPP_CONN_FILE" ]; then
                echo '1' > $DPP_RECONFIG_FLAG
                echo 'sleep 30; if [ -e '"$DPP_RECONFIG_FLAG"' ]; then for i in `seq 1 4`; do wpa_cli dpp_reconfig 0 multi_ap=1; sleep 15; done; fi' > $DPP_RECONFIG_SCRIPT_PATH
                /bin/sh $DPP_RECONFIG_SCRIPT_PATH &
        fi
        exit 0
        ;;
$STA_AUTHENTICATION_FAILURE)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_sta_authentication_failure_event
        exit 0
        ;;
$STA_CONNECT_TIMEOUT)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_wlan_sta_connect_timeout_event
        exit 0
        ;;
$DPP_EVENT_CONNECTOR_STORE)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_dpp_store_connector $1 $3
        exit 0
        ;;
$DPP_EVENT_CONNECTOR_1905_STORE)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_dpp_store_connector_1905 $1 $3
        exit 0
        ;;
$DPP_EVENT_C_SIGN_KEY_STORE)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_dpp_store_csign_key $1 $3
        exit 0
        ;;
$DPP_EVENT_PP_KEY_STORE)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_dpp_store_pp_key $1 $3
        exit 0
        ;;
$DPP_EVENT_NET_ACCESS_KEY_STORE)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_dpp_store_net_access_key $1 $3
        exit 0
        ;;
$DPP_EVENT_CONFOBJ_SSID_STORE)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_dpp_store_ssid $1 $3
        exit 0
        ;;
$DPP_EVENT_CONFOBJ_PASS_STORE)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_dpp_store_psk $1 $3
        exit 0
        ;;
$DPP_EVENT_CONFOBJ_AKM_STORE)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_dpp_store_akm $1 $3
        exit 0
        ;;
$DPP_EVENT_CONFOBJ_PARSE_SUCCESS)
        sysconf send_unix_sock_message $SYSTEMD_USOCK_SERVER do_dpp_store_success $1
        exit 0
        ;;
*)
	exit 1
	;;
esac
