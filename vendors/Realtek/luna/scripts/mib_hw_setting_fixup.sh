#!/bin/sh

mib_sn=$(mib get HW_SERIAL_NO)
sn=$(bdinfo sn)
pon_dect_ctrl_flag=$(mib get PON_DETECT_CONTROL_FLAG)
pon_dect_gpon_flag=$(mib get PON_DETECT_AVAILABLE_GPON_FLAG)
pon_dect_epon_flag=$(mib get PON_DETECT_AVAILABLE_EPON_FLAG)
reboot_flag=0
change_flag=0

if [ "$mib_sn" == "HW_SERIAL_NO=000000000002" ]; then
  echo "sn is 000000000002, set sn=$sn to mib"
  mib set HW_SERIAL_NO "$sn"
  change_flag=1
fi

if [ "$pon_dect_ctrl_flag" == "PON_DETECT_CONTROL_FLAG=0" ]; then
  echo "PON_DETECT_CONTROL_FLAG is 0, set to 1"
  mib set PON_DETECT_CONTROL_FLAG 1
  reboot_flag=1
  change_flag=1
fi

if [ "$pon_dect_gpon_flag" == "PON_DETECT_AVAILABLE_GPON_FLAG=0" ]; then
  echo "PON_DETECT_AVAILABLE_GPON_FLAG is 0, set to 1"
  mib set PON_DETECT_AVAILABLE_GPON_FLAG 1
  reboot_flag=1
  change_flag=1
fi

if [ "$pon_dect_epon_flag" == "PON_DETECT_AVAILABLE_EPON_FLAG=0" ]; then
  echo "PON_DETECT_AVAILABLE_EPON_FLAG is 0, set to 1"
  mib set PON_DETECT_AVAILABLE_EPON_FLAG 1
  reboot_flag=1
  change_flag=1
fi


if [ $change_flag == 1 ]; then
  mib commit hs now
  echo "...........mib HW_SETTING fix done." > /dev/console
fi

if [ $reboot_flag == 1 ]; then
  echo "PON DETECT config changed, reboot"
  reboot
fi

