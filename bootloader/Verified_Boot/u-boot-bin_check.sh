#!/bin/sh
echo "---------------------------- $UBOOT -------------------------------------"

UPATH=../${UBOOT}/
KPATH=$1
UBOOT_PUBKEY_DTB=uboot_fip_pubkey.dtb

DTS_F=`cat ${UBOOT}/.config | grep CONFIG_DEFAULT_DEVICE_TREE | awk -F"=" '{ print $2 }' | tr -d "\""`
UBOOT_1ST_DTB=${UPATH}/arch/arm/dts/${DTS_F}.dtb


WORK_DIR=`dirname $0`
cd $WORK_DIR

uboot_nodtb_size=`du -b ${UPATH}/u-boot-nodtb.bin | awk -F' ' '{ print $1 }'`


if [ ! -f ${UPATH}/u-boot.bin ];then
	 printf "\033[1;31m   Not found ${UPATH}/u-boot.bin  ! \n \033[0m";
	 exit -1;
fi
uboot_size=`du -b ${UPATH}/u-boot.bin | awk -F' ' '{ print $1 }'`

dtb_size=$(($uboot_size - $uboot_nodtb_size))


if [ $dtb_size -le 0 ];then
	 printf "\033[1;31m   Not found DTB in u-boot.bin  ! \n \033[0m";
	 exit -1;
fi


uboot_fip_pubkey_size=`du -b $KPATH/$UBOOT_PUBKEY_DTB | awk -F' ' '{ print $1 }'`

UBOOT_1ST_DTB_SIZE=`du -b $UBOOT_1ST_DTB | awk -F' ' '{ print $1 }'`

echo "UBOOT_1ST_DTB_SIZE=$UBOOT_1ST_DTB_SIZE"
echo "uboot_size=$uboot_size"
echo "used_dtb_size = $dtb_size"
echo "uboot_nodtb_size=$uboot_nodtb_size"
echo  "uboot_fip_pubkey_size=$uboot_fip_pubkey_size"

#uboot_size should >= (uboot_nodtb_size + UBOOT_1ST_DTB_SIZE)
#RSA2048_SIZE = 256
if [ $uboot_size -lt $(($uboot_nodtb_size + $UBOOT_1ST_DTB_SIZE + 256)) ] ;then
	printf "\033[1;31m  u-boot.bin  size too small ! uboot_size should >= (uboot_nodtb_size + UBOOT_1ST_DTB_SIZE + 256)\n \033[0m";
	printf "\033[1;31m     UBOOT_1ST_DTB_SIZE: board dtb without signature and cipher. \n \033[0m";
	exit -1;
fi


dd if=${UPATH}/u-boot.bin bs=1 count=$dtb_size skip=$uboot_nodtb_size of=./tmp/uboot_used.dtb

if [ -f ./tmp/uboot_used.dtb ] ;then
	 printf "\033[1;34m -----------------DTB contect of u-boot.bin  -----------------------------\n";
		dtc -s ./tmp/uboot_used.dtb 
	 printf "\033[1;34m -------------------------------------------- -----------------------------\n \033[0m";
fi