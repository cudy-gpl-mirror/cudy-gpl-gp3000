#!/bin/sh
########################################################
# 256KB                     : GPT HEAD
# 256KB + FIP_SIZE_LIMIT    : FIP0
# 256KB + FIP_SIZE_LIMIT*2  : FIP1
########################################################
FIP_SIZE_LIMIT=2097152   ###Bytes

PLATFORM_CONFIG="Flash_and_DDR_Support.txt"
if [ ! -f bootloader.conf ];then
	echo "Not find bootloader.conf!!"
	exit 1
fi

echo "***************************************************************"
PROJECT_NAME=`grep PROJECT_NAME bootloader.conf | awk -F= '{ print $2}' | tr -d ' '`
echo "PROJECT_NAME : $PROJECT_NAME"

PWD=`pwd`
DATETIME=`date '+%Y%m%d'`
IMAGE_BASENAME=${PROJECT_NAME}
IMAGE_NAME="${IMAGE_BASENAME}-${DATETIME}"
IMAGE_LINK_NAME="${IMAGE_BASENAME}"
DEPLOY_DIR_IMAGE=${PWD}/image/

echo "DEPLOY_DIR_IMAGE: ${DEPLOY_DIR_IMAGE}"
echo "IMAGE_LINK_NAME:	 ${IMAGE_LINK_NAME}"
echo "IMAGE_NAME:	${IMAGE_NAME}"

if [ "$2" = "" ];then
	IMAGE_PRJ_NAME=venus_sflash_loader
else
	IMAGE_PRJ_NAME=$2
fi
echo "IMAGE_PRJ_NAME: ${IMAGE_PRJ_NAME}"

echo "***************************************************************"
if [ -f ${PLATFORM_CONFIG} ];then
	cp ${PLATFORM_CONFIG} ${DEPLOY_DIR_IMAGE}
	echo "Use ${DEPLOY_DIR_IMAGE}/${PLATFORM_CONFIG}"
else
	echo "WARNING: No ${PLATFORM_CONFIG}"
fi

export PATH=${PWD}/host_build_tools:${PATH}


set_gpt_attribute() {
	local action="$1"
	local bit="$2"
	local gpt_file="$3"
	echo "set_gpt_attribute $action $bit $gpt_file"
	sgdisk -A 1:${action}:${bit} $gpt_file
	sgdisk -A 2:${action}:${bit} $gpt_file
}

##set_DDR_info_in_gpt() {
##	local params="$1"
##	local gpt_file="$2"
##	eval $params

##	if [ ${TYPE} = "DDR4" ]; then
##		set_gpt_attribute set 49 $gpt_file
##		IMG_NAME_DDRTYPE=_ddr4
##	else
##		set_gpt_attribute clear 49 $gpt_file
##		IMG_NAME_DDRTYPE=_ddr3
##	fi
##
##	if [ ${BITS} = "16" ]; then
##		set_gpt_attribute set 50 $gpt_file
##		IMG_NAME_DDRBITS=_16bit
##	else
##		set_gpt_attribute clear 50 $gpt_file
##		IMG_NAME_DDRBITS=_32bit
##	fi
##}

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
	ln -sf ${DEPLOY_DIR_IMAGE}/fip/fip.bin  ${IMAGE_NAME}-fip.bin
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
	fip_filesize=$(stat -L --printf="%s" ${IMAGE_NAME}-fip.bin )


	echo "Generated fip image file size=${fip_filesize}"
	echo "Allowd max size: ${FIP_SIZE_LIMIT}"
	[ ${fip_filesize} -ge ${FIP_SIZE_LIMIT} ] && {
		echo "${IMAGE_NAME}-fip.bin size is too large"
		echo "${IMAGE_NAME}-fip.bin size is ${fip_filesize}"
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
	
	sgdisk -t 1:ca777000-f190-b69b-8136-dea2835d4178 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
	retcode=$? ; if [ $retcode -ne 0 ]; then echo "sgdisk fail. retcode = " $retcode; exit $retcode;fi
	
	sgdisk -t 2:ca777000-f190-b69b-8136-dea2835d4178 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
	sgdisk -A 1:set:48 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
}	

##do_image_gpt_add_draminfo() {
##	if [ -f ${PLATFORM_CONFIG} ];then
##		grep -v DDR_INFO ${PLATFORM_CONFIG} | grep -v "^#"  | grep "TYPE=DDR"  |  sed '/^\s*$/d' > .ddr_support.lst
##		while read read_line; do 
##			DDR_INFO=`echo $read_line |tr -d '[:space:]'`
##
##			if [ -z "${DDR_INFO}" ] ; then
##				echo "No DDR info assigned."
##				break;
##			fi

##			DDR_INFO_PARAM=${DDR_INFO}		
##			if [ "x${DDR_INFO_PARAM}" != 'x' ];then
##				echo "set_DDR_info_in_gpt ${DDR_INFO_PARAM} ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img"
##				set_DDR_info_in_gpt ${DDR_INFO_PARAM} ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img
##			else
##				echo "Error! No DDR_INFO_PARAM"
##				exit 1;
##			fi

##			dd if=./${IMAGE_NAME}-fip.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  seek=${fip0_start}  conv=notrunc
##			dd if=./${IMAGE_NAME}-fip.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  seek=${fip1_start}  conv=notrunc

##			echo "mv ${IMAGE_NAME}-gpt.img ${IMAGE_PRJ_NAME}.img"
##			mv ${IMAGE_NAME}-gpt.img ${IMAGE_PRJ_NAME}.img
##			check_gpt_img  ${DEPLOY_DIR_IMAGE}/${IMAGE_PRJ_NAME}.img
##		done < ./.ddr_support.lst
##	fi

##	## Enter this case if
##	## 1. No ${PLATFORM_CONFIG} file
##	## 2. No DDR_INFO is assigned in ${PLATFORM_CONFIG}
##	if [ -z "${DDR_INFO}" ] ; then
##		cp ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img
##		dd if=./${IMAGE_NAME}-fip.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  seek=${fip0_start}  conv=notrunc
##		dd if=./${IMAGE_NAME}-fip.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  seek=${fip1_start}  conv=notrunc
##
##		echo "mv ${IMAGE_NAME}-gpt.img ${IMAGE_PRJ_NAME}.img"
##		mv ${IMAGE_NAME}-gpt.img ${IMAGE_PRJ_NAME}.img
##		check_gpt_img  ${DEPLOY_DIR_IMAGE}/${IMAGE_PRJ_NAME}.img
##	fi
##}
Aarch32_Set=0;
do_image_gpt_add_Aarch() {
	echo "do_image_gpt_add_Aarch"
	if [ -f ${PLATFORM_CONFIG} ];then
		grep -v ".*UBI.*_ARGS" ./${PLATFORM_CONFIG} | grep "AARCH="|  sed '/^\s*$/d' > .arm_aarch.lst
		while read read_line; do  
			echo $read_line; 
			ARM_ARCH_PARAM=`echo $read_line |tr -d '[:space:]'`
			eval $ARM_ARCH_PARAM

			if [ "${AARCH}" = "Aarch32" ]; then
				echo "Aarch32: set_gpt_attribute set 54"
				set_gpt_attribute set 54 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
				Aarch32_Set=1;
			else
				echo "Aarch64: set_gpt_attribute clear 54"
				set_gpt_attribute clear 54 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
			fi
		done < ./.arm_aarch.lst
		if [ "$Aarch32_Set" = "0" ] ;then
				echo "Aarch64: set_gpt_attribute clear 54"
				set_gpt_attribute clear 54 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
		fi
	else
		echo "do_image_gpt_add_Aarch: Not found ${PLATFORM_CONFIG}."
	fi
	mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img
}

do_final_image(){
	dd if=./${IMAGE_NAME}-fip.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  seek=${fip0_start}  conv=notrunc
	dd if=./${IMAGE_NAME}-fip.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  seek=${fip1_start}  conv=notrunc

	if [ "${AARCH}" = "Aarch32" ]; then
		echo "mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  ${DEPLOY_DIR_IMAGE}/${IMAGE_PRJ_NAME}-${AARCH}.img"
		mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  ${DEPLOY_DIR_IMAGE}/${IMAGE_PRJ_NAME}_${AARCH}.img
##		cd  ${DEPLOY_DIR_IMAGE}
##		ln -s ${IMAGE_PRJ_NAME}-${AARCH}.img ${IMAGE_PRJ_NAME}.img
	else
		echo "mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  ${DEPLOY_DIR_IMAGE}/${IMAGE_PRJ_NAME}.img"
		mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img  ${DEPLOY_DIR_IMAGE}/${IMAGE_PRJ_NAME}.img
	fi
	check_gpt_img  ${DEPLOY_DIR_IMAGE}/${IMAGE_PRJ_NAME}.img
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
#echo "---------------------"
##do_image_gpt_add_draminfo
echo "---------------------"
do_image_gpt_add_Aarch
echo "---------------------"
do_final_image


