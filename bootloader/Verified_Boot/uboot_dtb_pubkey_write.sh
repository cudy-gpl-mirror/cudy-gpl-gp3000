#!/bin/sh
KPATH=$1
CIPHERING=$2
echo "---------------------------- $UBOOT -------------------------------------"

UPATH=../u-boot-2020.01/
UBOOT_DTB=${UPATH}/arch/arm/dts/venus_fip_control.dtb

if [ x"$UBOOT" != "x" ]; then

UPATH=../$UBOOT/
UVer=`echo $UBOOT |  tr -d ' ' | cut -c 8-11`
echo " u-boot MainVer: ${UVer} "

 if [ "${UVer}" -ge 2018 ]; then
  DTS_F=`cat ${UBOOT}/.config | grep CONFIG_DEFAULT_DEVICE_TREE | awk -F"=" '{ print $2 }' | tr -d "\""`
  UBOOT_DTB=${UPATH}/arch/arm/dts/${DTS_F}.dtb
 fi
fi

echo " u-boot DTB: $UBOOT_DTB --------------"
MIMG_TOOL=${UPATH}/tools/mkimage

UBOOT_PUBKEY_DTB=uboot_fip_pubkey.dtb

path_check(){
	target=$1
	if [ "${target}" = "" ];then
		echo "Error:Path or File not exist! - Check \"${target}\""
		exit 1
	fi

	if [ ! -e ${target} ];then
		echo "Error:Path or File not exist! - Check \"${target}\""
		exit 1
	fi
}

WORK_DIR=`dirname $0`
cd $WORK_DIR

path_check ${KPATH}
path_check ${UPATH}
path_check ${MIMG_TOOL}
path_check ${UBOOT_DTB}

if [ ! -d tmp ];then
	mkdir tmp
else
	rm -f tmp/*
fi


dd if=/dev/urandom of=./tmp/dumy0 bs=4K count=1
dd if=/dev/urandom of=./tmp/dumy1 bs=4K count=1

if [ "$CIPHERING" = "y" ];then
	echo "Enable FIT_SIGNATURE and FIT_CIPHER, use boot_verified_cipher_dumy.its"
	${MIMG_TOOL}  -f  boot_verified_cipher_dumy.its  ./tmp/image.fit
else
	echo "Enable FIT_SIGNATURE, use boot_verified_dumy.its"
	${MIMG_TOOL}  -f  boot_verified_dumy.its  ./tmp/image.fit
fi

cp ${UBOOT_DTB} ./tmp/
cp ${UBOOT_DTB} ./tmp/${UBOOT_PUBKEY_DTB}


${MIMG_TOOL}   -k ${KPATH} -K ./tmp/${UBOOT_PUBKEY_DTB} -F -r ./tmp/image.fit


if [ -f ./tmp/${UBOOT_PUBKEY_DTB} ]; then
	echo "------------------------ U-boot public_key DTB -----------------------"
	dtc -s ./tmp/${UBOOT_PUBKEY_DTB}
	cp ./tmp/${UBOOT_PUBKEY_DTB}  ${KPATH}/
fi
