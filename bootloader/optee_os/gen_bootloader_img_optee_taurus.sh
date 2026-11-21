#!/bin/sh
########################################################
# 256KB                     : GPT HEAD
# 256KB + FIP_SIZE_LIMIT    : FIP0
# 256KB + FIP_SIZE_LIMIT*2  : FIP1
########################################################
FIP_SIZE_LIMIT=2097152   ###Bytes

current_dir=$(pwd)
if [ ! -f bootloader.conf ];then
	echo "Not find bootloader.conf!!"
	exit 1
fi

U_BOOT_CONF="${current_dir}/${CONFIG_UBOOT_VER}/include/generated/autoconf.h"
if [ ! -f ${U_BOOT_CONF} ];then
	echo "Not find the autoconf.h of u-boot!!"
	echo " - File path: \"${CONFIG_UBOOT_VER}/include/generated/autoconf.h}\""
	exit 1
fi
echo "***************************************************************"
PROJECT_NAME=`grep PROJECT_NAME bootloader.conf | awk -F= '{ print $2}' | tr -d ' '`
echo "PROJECT_NAME : $PROJECT_NAME"

PWD=`pwd`
DATETIME=`date '+%Y%m%d'`
IMAGE_NAME="${PROJECT_NAME}-${DATETIME}"
DEPLOY_DIR_IMAGE=${PWD}/image/

echo "DEPLOY_DIR_IMAGE: ${DEPLOY_DIR_IMAGE}"
echo "IMAGE_NAME:	${IMAGE_NAME}"

if [ "$1" = "" ];then
    BOOT_IMAGE_NAME=taurus_spi_nand_loader
    FIP_IMAGE_NAME=taurus_spi_nand_fip
else
    BOOT_IMAGE_NAME=$1"_loader"
    FIP_IMAGE_NAME=$1"-fip"
fi
echo "BOOT_IMAGE_NAME: ${BOOT_IMAGE_NAME}"
echo "FIP_IMAGE_NAME: ${FIP_IMAGE_NAME}"

echo "***************************************************************"

export PATH=${PWD}/host_build_tools:${PATH}


set_gpt_attribute() {
	local action="$1"
	local bit="$2"
	local gpt_file="$3"
	echo "set_gpt_attribute $action $bit $gpt_file"
	sgdisk -A 1:${action}:${bit} $gpt_file
	sgdisk -A 2:${action}:${bit} $gpt_file
}

##################

do_generate_fip_image() {
	BL31_PARAM=""
	if [ -f ${DEPLOY_DIR_IMAGE}/fip/bl31.bin ]; then
		BL31_PARAM="--soc-fw ${DEPLOY_DIR_IMAGE}/fip/bl31.bin"
	fi
	echo "---------------------"
	fiptool create \
	--trusted-key-cert ${DEPLOY_DIR_IMAGE}/fip/trusted_key.crt \
	--tb-fw-cert ${DEPLOY_DIR_IMAGE}/fip/tb_fw.crt \
	--soc-fw-cert ${DEPLOY_DIR_IMAGE}/fip/soc_fw_content.crt \
	--soc-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/soc_fw_key.crt \
	--nt-fw-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_content.crt \
	--nt-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_key.crt \
	--tos-fw ${PWD}/optee_os/optee_bin/tee-header_v2.bin  \
	--tos-fw-extra1 ${PWD}/optee_os/optee_bin/tee-pager_v2.bin  \
	--tos-fw-extra2 ${PWD}/optee_os/optee_bin/tee-pageable_v2.bin  \
	--tos-fw-cert ${DEPLOY_DIR_IMAGE}/fip/tos_fw_content.crt  \
	--tos-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/tos_fw_key.crt  \
	--tb-fw ${DEPLOY_DIR_IMAGE}/fip/bl2.bin \
	${BL31_PARAM} \
	--nt-fw ${DEPLOY_DIR_IMAGE}/u-boot.bin \
	${DEPLOY_DIR_IMAGE}/fip/fip.bin
}

do_generate_fip_image_key() {
	BL31_PARAM=""
	if [ -f ${DEPLOY_DIR_IMAGE}/fip/bl31.bin ]; then
		BL31_PARAM="--soc-fw ${DEPLOY_DIR_IMAGE}/fip/bl31.bin"
	fi
	echo "---------------------"
	ls -l  ${DEPLOY_DIR_IMAGE}/fip/
	echo "---------------------"

	cert_create  \
	-n  \
	--tfw-nvctr 0  \
	--ntfw-nvctr 0  \
	--trusted-key-cert ${DEPLOY_DIR_IMAGE}/fip/trusted_key.crt  \
	--key-alg rsa  \
	--rot-key ${DEPLOY_DIR_IMAGE}/fip/rotprivk_rsa.pem  \
	--tb-fw     ${DEPLOY_DIR_IMAGE}/fip/bl2.bin  \
	--tb-fw-cert ${DEPLOY_DIR_IMAGE}/fip/tb_fw.crt  \
	${BL31_PARAM} \
	--soc-fw-cert ${DEPLOY_DIR_IMAGE}/fip/soc_fw_content.crt  \
	--soc-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/soc_fw_key.crt \
	--nt-fw ${DEPLOY_DIR_IMAGE}/u-boot.bin  \
	--nt-fw-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_content.crt  \
	--nt-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_key.crt \
	--tos-fw ${PWD}/optee_os/optee_bin/tee-header_v2.bin  \
	--tos-fw-extra1 ${PWD}/optee_os/optee_bin/tee-pager_v2.bin  \
	--tos-fw-extra2 ${PWD}/optee_os/optee_bin/tee-pageable_v2.bin  \
	--tos-fw-cert ${DEPLOY_DIR_IMAGE}/fip/tos_fw_content.crt  \
	--tos-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/tos_fw_key.crt 
}


do_image_fip() {
	do_generate_fip_image_key
	do_generate_fip_image

	cd ${DEPLOY_DIR_IMAGE}
	ln -sf ${DEPLOY_DIR_IMAGE}/fip/fip.bin  ${FIP_IMAGE_NAME}.bin
}

check_gpt_img(){
	chk_img=$1
	if [ -f ${chk_img} ]; then
		echo "---------- ${chk_img} ----------"
		sgdisk -p ${chk_img} | tee  ${chk_img}.log
		retcode=$? ; if [ $retcode -ne 0 ]; then echo "sgdisk fail. retcode = " $retcode; exit $retcode;fi
		partition_cnt=`grep -c fip ${chk_img}.log`     
		if [ $partition_cnt -lt 2 ] ;then
			echo "Bootlader GPT_IMG Check Fail. GPT NUM Error!!"; 
			exit 1;
		fi
		echo "---------------------------------------------------------"
	fi
}


do_image_gpt_preheader() {
	#
	# We create 2 partitions only by default. Both are for fip. One is master,
	# and the other is slave. However, we may want some additional partitions
	# in some cases. Use EXTRA_PARTITIONS to create more partitions. The
	# EXTRA_PARTITIONS syntax likes following. This is an exmple with 2
	# additional partitions.
	#
	# EXTRA_PARTITIONS="name=uboot-env,size=1M,start=4M;name=rootfs,size=180M,start=6M"
	#
	# To Simplify problems,
	# 1. the unit of size/start is M (=1024*1024 bytes) only.
	# 2. The partitions'order must be from the beginning of device to the end.
	# 3. Don't put semicolon(;) at the end of varibale.
	# 4. This is not inconsistent with final image size since we don't know
	# which partition is used to rootfs. The partition's name may be different.
	# It may called root, rootfs_0, root_standby, or anything else. You must
	# check it by yourself.
	#
	fip_filesize=$(stat -L --printf="%s" ${FIP_IMAGE_NAME}.bin )


	echo "Generated fip image file size=${fip_filesize}"
	echo "Allowd max size: ${FIP_SIZE_LIMIT}"
	[ ${fip_filesize} -ge ${FIP_SIZE_LIMIT} ] && {
		echo "${FIP_IMAGE_NAME}.bin size is too large"
		echo "${FIP_IMAGE_NAME}.bin size is ${fip_filesize}"
		echo "fip0 partition size  size is ${FIP_SIZE_LIMIT}"
		echo "Please check it"
		exit 1
	}	
	
	fip0_start=512      #From 2th 4KPG block, OFFSET=256*1024=262144, The default block size for dd is 512 bytes. 262144/512=512
	fip1_start=`expr ${FIP_SIZE_LIMIT} / 512  + ${fip0_start}`

	fip0_end=`expr ${FIP_SIZE_LIMIT} / 512  + ${fip0_start}`
	fip1_end=`expr ${FIP_SIZE_LIMIT} / 512  + ${fip1_start}`

	fip0_last=`expr ${fip0_start} + ${fip_filesize} / 512 + 1`
	fip1_last=`expr ${fip1_start} + ${fip_filesize} / 512 + 1`

	echo "Fip_0 Offset: ${fip0_start} ~ ${fip0_end}"
	echo "Fip_1 Offset: ${fip1_start} ~ ${fip1_end}" 

	TEMP_IMAGE_SIZE=17 ##8+8+1=17
	dd if=/dev/zero of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img bs=256K count=${TEMP_IMAGE_SIZE}
	echo " parted ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mklabel gpt"
	parted ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mklabel gpt
	retcode=$? ; if [ $retcode -ne 0 ]; then echo "parted ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mklabel gpt. retcode = " $retcode; exit $retcode;fi
 
	parted --align minimal ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mkpart fip0 `expr $fip0_start \* 512`B `expr $fip0_last \* 512`B
	retcode=$? ; if [ $retcode -ne 0 ]; then echo "parted fail. retcode = " $retcode; exit $retcode;fi
	
	parted --align minimal ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mkpart fip1 `expr $fip1_start \* 512`B `expr $fip1_last \* 512`B
	
	sgdisk -t 1:52544b00-f190-b69b-8136-dea2835d4178 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
	retcode=$? ; if [ $retcode -ne 0 ]; then echo "sgdisk fail. retcode = " $retcode; exit $retcode;fi
	
	sgdisk -t 2:52544b00-f190-b69b-8136-dea2835d4178 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
	sgdisk -A 1:set:48 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
}	

Aarch32_Set=0;
do_image_gpt_add_Aarch() {
	echo "do_image_gpt_add_Aarch"
	if [ -f ${U_BOOT_CONF} ];then
		ARMV7=$(grep -c "#define[[:blank:]]*CONFIG_SYS_CPU[[:blank:]]*\"armv7\"" ${U_BOOT_CONF});

			if [ "${ARMV7}" = "1" ]; then
				echo "Aarch32: set_gpt_attribute set 49"
				set_gpt_attribute set 49 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
				Aarch32_Set=1;
				AARCH="Aarch32";
			else
				echo "Aarch64: set_gpt_attribute clear 49"
				set_gpt_attribute clear 49 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
			fi
		if [ "$Aarch32_Set" = "0" ] ;then
				echo "Aarch64: set_gpt_attribute clear 49"
				set_gpt_attribute clear 49 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
		fi
	else
		echo "do_image_gpt_add_Aarch: Not found u-boot autoconf.h at ${U_BOOT_CONF}."
	fi
	mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img
}

do_final_image(){
	dd if=./${FIP_IMAGE_NAME}.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  seek=${fip0_start}  conv=notrunc
	dd if=./${FIP_IMAGE_NAME}.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  seek=${fip1_start}  conv=notrunc

	check_gpt_img  ${DEPLOY_DIR_IMAGE}/${BOOT_IMAGE_NAME}.img

	if [ "${AARCH}" = "Aarch32" ]; then
		echo "mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  ${DEPLOY_DIR_IMAGE}/${BOOT_IMAGE_NAME}-${AARCH}.img"
		mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  ${DEPLOY_DIR_IMAGE}/${BOOT_IMAGE_NAME}_${AARCH}.img
##		cd  ${DEPLOY_DIR_IMAGE}
##		ln -s ${BOOT_IMAGE_NAME}-${AARCH}.img ${BOOT_IMAGE_NAME}.img
	else
		echo "mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  ${DEPLOY_DIR_IMAGE}/${BOOT_IMAGE_NAME}.img"
		mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  ${DEPLOY_DIR_IMAGE}/${BOOT_IMAGE_NAME}.img
	fi
}

#check base files
if [ ! -d ${DEPLOY_DIR_IMAGE} ]; then
echo "Miss  ${DEPLOY_DIR_IMAGE}!!"
exit 1
fi

if [ ! -f ${DEPLOY_DIR_IMAGE}/fip/bl2.bin ]; then
echo "Miss ${DEPLOY_DIR_IMAGE}/fip/bl2.bin!"
exit 1
fi

if [ ! -f ${DEPLOY_DIR_IMAGE}/fip/bl31.bin ]; then
echo "Miss ${DEPLOY_DIR_IMAGE}/fip/bl31.bin!"
exit 1
fi
if [ ! -f ${DEPLOY_DIR_IMAGE}/u-boot.bin ]; then
echo "Miss ${DEPLOY_DIR_IMAGE}/u-boot.bin!"
exit 1
fi


#actions
do_image_fip
#cd ${DEPLOY_DIR_IMAGE}
do_image_gpt_preheader
echo "---------------------"
do_image_gpt_add_Aarch
echo "---------------------"
do_final_image


