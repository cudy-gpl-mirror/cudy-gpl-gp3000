#!/bin/bash
#set -x
#################################################################
ROOTDIR=`pwd`

argc=$#

############ Global val
# env_print will show real env_offset at run-time of u-boot 
tool=${ROOTDIR}/bootloader/u-boot/tools/env_print
BL_IMG_DIR=${ROOTDIR}/bootloader/image/
FW_IMG_DIR=${ROOTDIR}/images/
UBOOT_ENV_FILE=${ROOTDIR}/bootloader/u-boot-env/UBOOT-2018-DEFAULT_ENV_FILE
BOOT_IMAGE_NAME=_spi_nand_loader.img
PARTITION_SIZE_UNIT=262144
STATIC_CONFIG_SIZE=262144
UBI_FILE=G3_ubi_device.ubi
ENV_FILE=uboot-env.bin

TARGET_PATH=images
bl_conf=${TARGET_PATH}/default_Bloader_onebin_layout.txt
fw_conf=${TARGET_PATH}/default_Firmware_onebin_layout.txt
##########ARGS###########
nostatic=0
do_parse_arg() {
	for VAR in $@
	do
		if [ "$VAR" = "nostatic" ];then
			nostatic=1
			echo "Not include static partition"
		fi		
	done
}


do_onebin_bootloader_tbl_show(){
       if [ -f ${ROOTDIR}/bootloader/u-boot/tools/env_print ];then
               printf "0x0,%s\n" ${BL_IMG_DIR}/${BOOT_IMAGE_NAME} > ${bl_conf}

               env1=$(${tool} -a)
			   printf "0x%08x,%s\n" ${env1} ${BL_IMG_DIR}/${ENV_FILE} >>  ${bl_conf}

               env2=$(${tool} -b)
			   printf "0x%08x,%s\n" ${env2} ${BL_IMG_DIR}/${ENV_FILE} >>  ${bl_conf}		   
       fi
}

do_onebin_firmware_tbl_show(){
       if [ -f ${ROOTDIR}/bootloader/u-boot/tools/env_print ];then
               printf "0x0,%s\n" ${BL_IMG_DIR}/${BOOT_IMAGE_NAME} > ${fw_conf}

               env1=$(${tool} -a)
			   printf "0x%08x,%s\n" ${env1} ${BL_IMG_DIR}/${ENV_FILE} >>  ${fw_conf}

               env2=$(${tool} -b)
			   printf "0x%08x,%s\n" ${env2} ${BL_IMG_DIR}/${ENV_FILE} >>  ${fw_conf}

			   env2_start=$(printf "%u" ${env2})
			   env2_end=$(($env2_start + $PARTITION_SIZE_UNIT))
			   if [ $nostatic -eq 0 ];then
			   #STATIC, will fill with 0xff by default
				static_conf=$env2_end
				echo "static_conf offset $static_conf"
				ubi_start=$(($static_conf + $STATIC_CONFIG_SIZE))
			   else
				ubi_start=$env2_end
			   fi
			   echo "ubi_start $ubi_start"
			   printf "0x%08x,%s\n" $ubi_start ${FW_IMG_DIR}/$UBI_FILE >>  ${fw_conf}
			   
       fi
}

retval=-1
do_hex_compare(){
	if [ ! $# -eq 2 ];then 
		echo "Error: do_hex_compare x y"
	fi
	x=$(printf "0x%x" $1)
	y=$(printf "0x%x" $2)
	if [ "$x" != "$y" ];then
		echo " do_hex_compare failed! $x != $y"
		retval=1
	else
		retval=0
	fi
}

show_uboot_notes(){
	printf '\e[31m Please check %s\n\e[0m' $UBOOT_ENV_FILE
	printf '\e[31m  %s\n\e[0m' "pfl_boot_sz"
	printf '\e[31m  %s\n\e[0m' "pfl_fip_sz"
	printf '\e[31m  %s\n\e[0m' "pfl_env1_r"
	printf '\e[31m  %s\n\e[0m' "pfl_env2_r"
	printf '\e[31m  %s\n\e[0m' "pfl_env_size"
	printf '\e[31m  %s\n\e[0m' "mtdparts"

}

do_offset_check(){
	if [ ! -f $UBOOT_ENV_FILE ];then
		echo "Can't find $UBOOT_ENV_FILE "
	fi
	if [ ! -f $tool ];then
		echo "Can't find $tool "
	fi
	pfl_env1_r=$(grep pfl_env1_r $UBOOT_ENV_FILE | awk -F'=' '{print $2}')
	pfl_env1_r_in_uboot=$($tool -a)

	do_hex_compare $pfl_env1_r $pfl_env1_r_in_uboot

	if [ $retval -eq 1 ];then
		printf '\e[31m%s\n\e[0m' "pfl_env1_r is different between uboot_env_file and uboot_header"
		printf '\e[31m%s\n\e[0m' "$pfl_env1_r != $pfl_env1_r_in_uboot"
		show_uboot_notes
		exit -1
	fi
	
	pfl_env2_r=$(grep pfl_env2_r $UBOOT_ENV_FILE | awk -F'=' '{print $2}')
	pfl_env2_r_in_uboot=$($tool -b)

	do_hex_compare $pfl_env2_r $pfl_env2_r_in_uboot

	if [ $retval -eq 1 ];then
		printf '\e[31m%s\n\e[0m'  "pfl_env2_r is different between uboot_env_file and uboot_header"
		printf '\e[31m%s\n\e[0m'  "$pfl_env2_r != $pfl_env2_r_in_uboot"
		show_uboot_notes
		exit -1
	fi
	
}

##### main #############
if [ ! -d ${BL_IMG_DIR} ];then
	echo "No folder $BL_IMG_DIR"
fi
if [ ! -d ${TARGET_PATH} ];then
	mkdir ${TARGET_PATH}
fi

###### Find file
echo "--- Try to find loader image in ${BL_IMG_DIR}\n" 
#find out load image
loader_name=$(find ${BL_IMG_DIR} -regex ".*_loader.img" -type f)
if [ x"${loader_name}" = x ];then
	printf "Can't find loader.img in ${BL_IMG_DIR}\n"
	exit -1
fi
echo "    ${loader_name}"
BOOT_IMAGE_NAME=$(basename $loader_name)
if [ x"${BOOT_IMAGE_NAME}" = x ];then
	printf "Can't find ${BOOT_IMAGE_NAME} in ${BL_IMG_DIR}\n"
	exit -1
fi

echo "--- Try to find uboot-env in ${BL_IMG_DIR}\n"

uboot_env_name=$(find ${BL_IMG_DIR} -regex ".*uboot-env-.*\.bin" -type f)
if [ x"${uboot_env_name}" = x ];then
	printf "Can't find uboot-env*.bin in ${BL_IMG_DIR}\n"
	exit -1
fi
echo "    ${uboot_env_name}"
ENV_FILE=$(basename $uboot_env_name)

### check partiton 
do_offset_check

##########

do_parse_arg $@
do_onebin_bootloader_tbl_show
do_onebin_firmware_tbl_show

echo "--- list images layout\n"
echo "-------- cat ${bl_conf} -----------"
cat  ${bl_conf}

echo "-------- cat ${fw_conf} -----------"
cat  ${fw_conf}
echo "-----------------------------------"

printf "4. generate onebin image\n"
sh ./tools/gen_programmer_images_by_file.sh ${bl_conf} combined_loader_image.bin
sh ./tools/gen_programmer_images_by_file.sh ${fw_conf} combined_onebin_image.bin


