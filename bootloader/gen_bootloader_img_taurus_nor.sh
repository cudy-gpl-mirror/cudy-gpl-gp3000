#!/bin/sh
########################################################
# LOADER_IMG_SIZE = GPT_HEADER + FIP_SIZE_LIMIT + FIP_SIZE_LIMIT + ReservedSpace
#  Offset					:	Size    : OPTION
#   0						:	64KB    : GPT HEADER
#   fip0_start(64KB + FIP_SIZE_LIMIT)   :	FIP_SIZE_LIMIT	:	FIP0
#   fip1_start(64KB + FIP_SIZE_LIMIT*2)	:	FIP_SIZE_LIMIT	:	FIP1
#   Reserved Space:
#######Don't Change#################################################
OPTEE_SUPPORT=0
#FIP_BS must be 64KB for NOR sector erase
FIP_BS=65536
############## Loader Image Layout Setting##################################
# Note
# 1. LOADER_IMG_SIZE, fip0_start and fip1_start must be  a multiple of 256KB(FIP_BS)!
# 2. FIP_SIZE_LIMIT: GPT will occupy reserved space B
#########################################################################
if [ "$2" = "optee" ];then
	#To include optee BL32
	OPTEE_SUPPORT=1
	#Loader setting
	LOADER_IMG_SIZE=`expr 37 \* ${FIP_BS}`
	fip0_start=`expr 1 \* ${FIP_BS}`
	fip1_start=`expr 19 \* ${FIP_BS}`
	FIP_SIZE_LIMIT=`expr ${fip1_start} - ${fip0_start}`
elif [ "$2" = "exp" ];then
	LOADER_IMG_SIZE=`expr 21 \* ${FIP_BS}`
	fip0_start=`expr 1 \* ${FIP_BS}`
	fip1_start=`expr 11 \* ${FIP_BS}`
	FIP_SIZE_LIMIT=`expr ${fip1_start} - ${fip0_start}`
else
#Default(No optee) Loader Layout: 
	#Loader setting
	LOADER_IMG_SIZE=`expr 21 \* ${FIP_BS}`
	fip0_start=`expr 1 \* ${FIP_BS}`
	fip1_start=`expr 11 \* ${FIP_BS}`
	FIP_SIZE_LIMIT=`expr ${fip1_start} - ${fip0_start}`
fi

echo "LOADER_IMG_SIZE: ${LOADER_IMG_SIZE}"
echo "fip0_start: ${fip0_start}"
echo "fip1_start: ${fip1_start}"
################################################################################
current_dir=$(pwd)
PLATFORM_CONFIG=${current_dir}/Flash_and_DDR_Support.txt
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

if [[ "mercury" = *"$1"* ]]; then
ENCRYPT_BL2=`grep CONFIG_BL2_ENCRYPT bootloader.conf | awk -F= '{ print $2}' | tr -d ' '`
ENCRYPT_BL31=`grep CONFIG_BL31_ENCRYPT bootloader.conf | awk -F= '{ print $2}' | tr -d ' '`
ENCRYPT_BL32=`grep CONFIG_BL32_ENCRYPT bootloader.conf | awk -F= '{ print $2}' | tr -d ' '`
fi
ENCRYPT_BL33=`grep CONFIG_UBOOT_ENCRYPT bootloader.conf | awk -F= '{ print $2}' | tr -d ' '`

if [ "${ENCRYPT_BL2}" = "" ];then
    ENCRYPT_BL2=n;
else
    ENC_NAME_SUFFIX_BL2=-bl2
fi
if [ "${ENCRYPT_BL31}" = "" ];then
    ENCRYPT_BL31=n;
else
    ENC_NAME_SUFFIX_BL31=-bl31
fi
if [ "${ENCRYPT_BL32}" = "" ];then
    ENCRYPT_BL32=n;
else
    ENC_NAME_SUFFIX_BL32=-bl32
fi
if [ "${ENCRYPT_BL33}" = "" ];then
    ENCRYPT_BL33=n;
else
    ENC_NAME_SUFFIX_BL33=-uboot
fi

PWD=`pwd`
DATETIME=`date '+%Y%m%d%H%M'`
IMAGE_NAME="${PROJECT_NAME}-${DATETIME}"
DEPLOY_DIR_IMAGE=${PWD}/image/
BL2_ENC=${DEPLOY_DIR_IMAGE}/fip/bl2_enc.bin
BL2_BIN=${DEPLOY_DIR_IMAGE}/fip/bl2.bin
BL31_ENC=${DEPLOY_DIR_IMAGE}/fip/bl31_enc.bin
BL31_BIN=${DEPLOY_DIR_IMAGE}/fip/bl31.bin
BL32_ENC=${DEPLOY_DIR_IMAGE}/fip/bl32_enc.bin
BL32_BIN=${DEPLOY_DIR_IMAGE}/fip/bl32.bin
UBOOT_BIN=${DEPLOY_DIR_IMAGE}/u-boot.bin
BL33_ENC=${DEPLOY_DIR_IMAGE}/bl33_enc.bin
BL33_BIN=${DEPLOY_DIR_IMAGE}/bl33.bin

echo "DEPLOY_DIR_IMAGE: ${DEPLOY_DIR_IMAGE}"
echo "IMAGE_NAME:	${IMAGE_NAME}"

if [ "$1" = "" ];then
	BOOT_IMAGE_NAME=taurus_spi_nor_loader
	FIP_IMAGE_NAME=taurus_spi_nor_fip
else
    IMAGE_NAME_PREFIX=$1
    IMAGE_NAME_SUFFIX="${ENC_NAME_SUFFIX_BL2}${ENC_NAME_SUFFIX_BL31}${ENC_NAME_SUFFIX_BL32}${ENC_NAME_SUFFIX_BL33}"
    BOOT_IMAGE_NAME="${IMAGE_NAME_PREFIX}_loader"
    FIP_IMAGE_NAME="${IMAGE_NAME_PREFIX}-fip"
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

##### Script Functions for DDR clock override
GPTINFO_DDR_CLK="DDR_CLK_OVERRIDE=0;"
GPTINFO_DDR_FLYBY="DDR_FLY_BY_TOPOLOGY=0;"
GPTINFO_DDR_16BIT="DDR_FORCE_16BIT=0;"
GPTINFO_DDR_SPECIAL_PARA="DDR_SPECIAL_PARAMETER=0;"


retrieve_ddr_info() {
    if [  -f ${PLATFORM_CONFIG} ];then
        grep "DDR_CLK="  ${PLATFORM_CONFIG} |  sed '/^\s*$/d' > .ddr_target_clock.lst
        while read read_line; do
            TEMP=`echo $read_line |tr -d '[:space:]'`
            if [ -z "${TEMP}" ] ; then
                break;
            fi
            GPTINFO_DDR_CLK="DDR_CLK_OVERRIDE=1;"${TEMP}
            echo "${GPTINFO_DDR_CLK}"
            break;
        done < ./.ddr_target_clock.lst
    fi

    if [  -f ${PLATFORM_CONFIG} ];then
        grep "DDR_FLY_BY_TOPOLOGY=" ${PLATFORM_CONFIG} |  sed '/^\s*$/d' > .ddr_fly_by_topology.lst
        while read read_line; do
            TEMP=`echo $read_line |tr -d '[:space:]'`
            if [ -z "${TEMP}" ] ; then
                break;
            fi
            GPTINFO_DDR_FLYBY=${TEMP}
            echo "${GPTINFO_DDR_FLYBY}"
            break;
        done < ./.ddr_fly_by_topology.lst
    fi

    if [  -f ${PLATFORM_CONFIG} ];then
        grep "DDR_FORCE_16BIT=" ${PLATFORM_CONFIG} |  sed '/^\s*$/d' > .ddr_force_16bit.lst
        while read read_line; do
            TEMP=`echo $read_line |tr -d '[:space:]'`
            if [ -z "${TEMP}" ] ; then
                break;
            fi
            GPTINFO_DDR_16BIT=${TEMP}
            echo "${GPTINFO_DDR_16BIT}"
            break;
        done < ./.ddr_force_16bit.lst
    fi

    if [  -f ${PLATFORM_CONFIG} ];then
        grep "DDR_SPECIAL_PARAMETER=" ${PLATFORM_CONFIG} |  sed '/^\s*$/d' > .ddr_special_parameter.lst
        while read read_line; do
            TEMP=`echo $read_line |tr -d '[:space:]'`
            if [ -z "${TEMP}" ] ; then
                break;
            fi
            GPTINFO_DDR_SPECIAL_PARA=${TEMP}
            echo "${GPTINFO_DDR_SPECIAL_PARA}"
            break;
        done < ./.ddr_special_parameter.lst
    fi
}

set_DDR_info_in_gpt() {
## DDR_CLK:
## DDR_CLK_OVERRIDE=0: Bit[52:50]=3b'000 => Use BL2 default clock setting
## DDR_CLK=0: [52:50]=3b'001  => 1200 (N.A. in Taurus)
## DDR_CLK=1: [52:50]=3b'010  => 1066 (N.A. in Taurus)
## DDR_CLK=2: [52:50]=3b'011  => 933
## DDR_CLK=3: [52:50]=3b'100  => 800
## DDR_CLK=4: [52:50]=3b'101  => 667  (N.A. in Taurus)
## 
## DDR_FLY_BY_TOPOLOGY:
## Bit[53]=1'b0                   => Use BL2 default DDR training flow
## DDR_FLY_BY_TOPOLOGY=1: Bit55=1 => DDR layout is fly-by topology
## 
## DDR_FORCE_16BIT:
## Bit[53]=1'b0                   => Use BL2 default DDR training flow
## DDR_FORCE_16BIT=1: Bit56=1     => Force DDR use 16-bit mode
## 
## DDR_SPECIAL_PARAMETER:
## Bit[55]                        => Reserved for DDR training code use

    echo "set_DDR_info_in_gpt()"

    local params1=${GPTINFO_DDR_CLK}
    local params2=${GPTINFO_DDR_FLYBY}
    local params3=${GPTINFO_DDR_16BIT}
    local params4=${GPTINFO_DDR_SPECIAL_PARA}
    local gpt_file=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img

    eval $params1
    eval $params2
    eval $params3
    eval $params4

    if [ ${DDR_CLK_OVERRIDE} = "0" ]; then
        set_gpt_attribute clear 50 $gpt_file
        set_gpt_attribute clear 51 $gpt_file
        set_gpt_attribute clear 52 $gpt_file
        if [  -f ${current_dir}/.ddr_target_clock.lst ];then
            rm ${current_dir}/.ddr_target_clock.lst
        fi
    else
        ###if [ ${DDR_CLK} = "0" ]; then
        ###    set_gpt_attribute set 50 $gpt_file
        ###    set_gpt_attribute clear 51 $gpt_file
        ###    set_gpt_attribute clear 52 $gpt_file
        ###    echo "DRAM clock override to 1200"
        ###fi

        ###if [ ${DDR_CLK} = "1" ]; then
        ###    set_gpt_attribute clear 50 $gpt_file
        ###    set_gpt_attribute set 51 $gpt_file
        ###    set_gpt_attribute clear 52 $gpt_file
        ###    echo "DRAM clock override to 1066"
        ###fi

        if [ ${DDR_CLK} = "2" ]; then
            set_gpt_attribute set 50 $gpt_file
            set_gpt_attribute set 51 $gpt_file
            set_gpt_attribute clear 52 $gpt_file
            echo "DRAM clock override to 933"
        fi

        if [ ${DDR_CLK} = "3" ]; then
            set_gpt_attribute clear 50 $gpt_file
            set_gpt_attribute clear 51 $gpt_file
            set_gpt_attribute set 52 $gpt_file
            echo "DRAM clock override to 800"
        fi

        ###if [ ${DDR_CLK} = "4" ]; then
        ###    set_gpt_attribute set 50 $gpt_file
        ###    set_gpt_attribute clear 51 $gpt_file
        ###    set_gpt_attribute set 52 $gpt_file
        ###    echo "DRAM clock override to 667"
        ###fi
    fi

    if [ ${DDR_FLY_BY_TOPOLOGY} = "1" ]; then
        set_gpt_attribute set 53 $gpt_file
        echo "DDR_FLY_BY_TOPOLOGY is 1"
    else
        set_gpt_attribute clear 53 $gpt_file
        if [  -f ${current_dir}/.ddr_fly_by_topology.lst ];then
            rm ${current_dir}/.ddr_fly_by_topology.lst
        fi
    fi

    if [ ${DDR_FORCE_16BIT} = "1" ]; then
        set_gpt_attribute set 54 $gpt_file
        echo "DDR_FORCE_16BIT"
    else
        set_gpt_attribute clear 54 $gpt_file
        if [  -f ${current_dir}/.ddr_force_16bit.lst ];then
            rm ${current_dir}/.ddr_force_16bit.lst
        fi
    fi

    if [ ${DDR_SPECIAL_PARAMETER} = "1" ]; then
        set_gpt_attribute set 55 $gpt_file
        echo "DDR_SPECIAL_PARAMETER is 1"
    else
        set_gpt_attribute clear 55 $gpt_file
        if [  -f ${current_dir}/.ddr_force_16bit.lst ];then
            rm ${current_dir}/.ddr_special_parameter.lst
        fi
    fi
}

##################

do_generate_fip_image() {
	BL31_PARAM=""
	if [ -f ${DEPLOY_DIR_IMAGE}/fip/bl31.bin ]; then
		BL31_PARAM="--soc-fw ${DEPLOY_DIR_IMAGE}/fip/bl31.bin"
	fi
	
	if [ $OPTEE_SUPPORT -eq 1 ];then
		OPTEE_ARGS="--tos-fw ${PWD}/optee_os/optee_bin/tee-header_v2.bin  \
					--tos-fw-extra1 ${PWD}/optee_os/optee_bin/tee-pager_v2.bin  \
					--tos-fw-extra2 ${PWD}/optee_os/optee_bin/tee-pageable_v2.bin  \
					--tos-fw-cert ${DEPLOY_DIR_IMAGE}/fip/tos_fw_content.crt  \
					--tos-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/tos_fw_key.crt "
	else
		OPTEE_ARGS=""
	fi
	echo "---------------------"
	fiptool create \
	--trusted-key-cert ${DEPLOY_DIR_IMAGE}/fip/trusted_key.crt \
	--tb-fw-cert ${DEPLOY_DIR_IMAGE}/fip/tb_fw.crt \
	--soc-fw-cert ${DEPLOY_DIR_IMAGE}/fip/soc_fw_content.crt \
	--soc-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/soc_fw_key.crt \
	--nt-fw-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_content.crt \
	--nt-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_key.crt \
	${OPTEE_ARGS} \
	--tb-fw ${DEPLOY_DIR_IMAGE}/fip/bl2.bin \
	${BL31_PARAM} \
	--nt-fw ${BL33_BIN} \
	${DEPLOY_DIR_IMAGE}/fip/fip.bin
    
    fiptool info ${DEPLOY_DIR_IMAGE}/fip/fip.bin
}

do_generate_fip_image_key() {
	BL31_PARAM=""
	if [ -f ${DEPLOY_DIR_IMAGE}/fip/bl31.bin ]; then
		BL31_PARAM="--soc-fw ${DEPLOY_DIR_IMAGE}/fip/bl31.bin"
	fi

	if [ $OPTEE_SUPPORT -eq 1 ];then
		OPTEE_ARGS="	--nt-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_key.crt \
		--tos-fw ${PWD}/optee_os/optee_bin/tee-header_v2.bin  \
		--tos-fw-extra1 ${PWD}/optee_os/optee_bin/tee-pager_v2.bin  \
		--tos-fw-extra2 ${PWD}/optee_os/optee_bin/tee-pageable_v2.bin  \
		--tos-fw-cert ${DEPLOY_DIR_IMAGE}/fip/tos_fw_content.crt  \
		--tos-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/tos_fw_key.crt "
	else
		OPTEE_ARGS=""
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
	--nt-fw ${UBOOT_BIN}  \
	--nt-fw-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_content.crt  \
	--nt-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_key.crt  \
	${OPTEE_ARGS}
}

do_generate_encrypted_image() {
    echo "do_generate_encrypted_image()"

    ENC_KEY=`grep ENC_KEY ${DEPLOY_DIR_IMAGE}/fip/aes_keys | awk -F= '{ print $2}' | tr -d '\n\r'`
    ENC_NONCE=`grep ENC_NONCE ${DEPLOY_DIR_IMAGE}/fip/aes_keys | awk -F= '{ print $2}' | tr -d \n\r`
#    echo "ENC_KEY is ${ENC_KEY}"
#    echo "ENC_NONCE = ${ENC_NONCE}"

    if [ ${ENCRYPT_BL2} = "y" ]; then
        encrypt_fw \
        -f 0 \
        -k ${ENC_KEY} \
        -n ${ENC_NONCE} \
        -i ${BL2_BIN} \
        -o ${BL2_ENC}

        mv ${BL2_ENC} ${BL2_BIN} 
    else
        echo "No BL2 encryption"
    fi

    if [ ${ENCRYPT_BL31} = "y" ]; then
        encrypt_fw \
        -f 0 \
        -k ${ENC_KEY} \
        -n ${ENC_NONCE} \
        -i ${BL31_BIN} \
        -o ${BL31_ENC}

        mv ${BL31_ENC} ${BL31_BIN} 
    else
        echo "No BL31 encryption"
    fi

    if [ ${ENCRYPT_BL32} = "y" ]; then
        encrypt_fw \
        -f 0 \
        -k ${ENC_KEY} \
        -n ${ENC_NONCE} \
        -i ${BL32_BIN} \
        -o ${BL32_ENC}

        mv ${BL32_ENC} ${BL32_BIN} 
    else
        echo "No BL32 encryption"
    fi

    if [ ${ENCRYPT_BL33} = "y" ]; then
        encrypt_fw \
        -f 0 \
        -k ${ENC_KEY} \
        -n ${ENC_NONCE} \
        -i ${UBOOT_BIN} \
        -o ${BL33_ENC}

        mv ${BL33_ENC} ${BL33_BIN} 
    else
        echo "No BL33 encryption"
        ln -s ${UBOOT_BIN} ${BL33_BIN} 
    fi
}


do_image_fip() {
    do_generate_fip_image_key

    do_generate_encrypted_image

    do_generate_fip_image

	cd ${DEPLOY_DIR_IMAGE}
	cp ${DEPLOY_DIR_IMAGE}/fip/fip.bin  ${FIP_IMAGE_NAME}.bin
    ln -s ${FIP_IMAGE_NAME}.bin ${DATETIME}-${IMAGE_NAME_PREFIX}${IMAGE_NAME_SUFFIX}-fip.bin
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
	else
		echo " Not found ${chk_img}!!!!"
	fi
}


do_image_gpt_preheader() {
	TEMP_IMAGE_SIZE=`expr ${LOADER_IMG_SIZE} / ${FIP_BS}`
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
	#GPT: the first usable block is at LBA 34 or higher
	GPT_RESERVED=`expr 34 \* 512`
	fip_filesize=$(stat -L --printf="%s" ${FIP_IMAGE_NAME}.bin )



	fip0_last=`expr ${fip0_start} + ${fip_filesize}`
	fip1_last=`expr ${fip1_start} + ${fip_filesize}`

	fip1_max_last=`expr ${fip1_start} + ${FIP_SIZE_LIMIT}`

	if [ ${fip1_max_last} -gt ${LOADER_IMG_SIZE} ];then
		echo "Error: ${fip1_start} + ${fip_filesize} over ${LOADER_IMG_SIZE}"
	fi

	loader_img_remain=`expr ${LOADER_IMG_SIZE} - ${fip1_max_last}`
	if [ ${loader_img_remain} -ge ${GPT_RESERVED} ];then
		FIP_RSERVED=`expr 1 \* 512`
	else
		FIP_RSERVED=${GPT_RESERVED}
	fi
	allowd_max_size=`expr ${FIP_SIZE_LIMIT} - ${FIP_RSERVED}`

	echo "--------------------------------------------------------------------------"
	echo "LOADER_IMG_SIZE = ${TEMP_IMAGE_SIZE} * ${FIP_BS} = ${LOADER_IMG_SIZE}"
	echo "Generated fip image file size = ${fip_filesize}"
	echo "Allowd max size: FIP_SIZE_LIMIT(${FIP_SIZE_LIMIT}) - RESERVED(${FIP_RSERVED}) = ${allowd_max_size}"
	echo "*fip_filesize usage= ${fip_filesize}/${allowd_max_size}"
	echo "FIP0: ${fip0_start}B ~ ${fip0_last}B"
	echo "FIP1: ${fip1_start}B ~ ${fip1_last}B"
	echo "--------------------------------------------------------------------------"

	[ ${fip_filesize} -gt ${allowd_max_size} ] && {
		echo "${FIP_IMAGE_NAME}.bin size is too large"
		echo "${FIP_IMAGE_NAME}.bin size is ${fip_filesize}"
		echo "fip0/1 partition size limitation is ${FIP_SIZE_LIMIT} "
		echo "Please check it"
		exit 1
	}	


	dd if=/dev/zero of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img bs=64K count=${TEMP_IMAGE_SIZE}

	echo " parted ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mklabel gpt"
	parted ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mklabel gpt
	retcode=$? ; if [ $retcode -ne 0 ]; then echo "parted ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mklabel gpt. retcode = " $retcode; exit $retcode;fi

	#echo "parted --align minimal ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mkpart fip0 ${fip0_start}B ${fip0_last}B"
	parted --align minimal ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mkpart fip0 ${fip0_start}B ${fip0_last}B
	retcode=$? ; if [ $retcode -ne 0 ]; then echo "parted fail. retcode = " $retcode; exit $retcode;fi
	
	#echo "parted --align minimal ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mkpart fip1 ${fip1_start}B ${fip1_last}B"
	parted --align minimal ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img mkpart fip1 ${fip1_start}B ${fip1_last}B
	

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
}

set_atf_encryption_in_gpt() {
    if [ ${ENCRYPT_BL2} = "y" ]; then
        set_gpt_attribute set 60 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
    else
        set_gpt_attribute clear 60 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
    fi

    if [ ${ENCRYPT_BL31} = "y" ]; then
        set_gpt_attribute set 61 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
    else
        set_gpt_attribute clear 61 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
    fi

    if [ ${ENCRYPT_BL32} = "y" ]; then
        set_gpt_attribute set 62 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
    else
        set_gpt_attribute clear 62 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
    fi

    if [ ${ENCRYPT_BL33} = "y" ]; then
        set_gpt_attribute set 63 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
    else
        set_gpt_attribute clear 63 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img
    fi
}

do_final_image(){
	dd if=./${FIP_IMAGE_NAME}.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img  bs=64K seek=`expr ${fip0_start} / ${FIP_BS}`  conv=notrunc
	dd if=./${FIP_IMAGE_NAME}.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img  bs=64K seek=`expr ${fip1_start} / ${FIP_BS}`  conv=notrunc

	check_gpt_img  ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img

	if [ "${AARCH}" = "Aarch32" ]; then
		echo "mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img  ${DEPLOY_DIR_IMAGE}/${BOOT_IMAGE_NAME}-${AARCH}.img"
		mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img  ${DEPLOY_DIR_IMAGE}/${BOOT_IMAGE_NAME}_${AARCH}.img
##		cd  ${DEPLOY_DIR_IMAGE}
##		ln -s ${BOOT_IMAGE_NAME}-${AARCH}.img ${BOOT_IMAGE_NAME}.img
	else
		echo "mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img  ${DEPLOY_DIR_IMAGE}/${BOOT_IMAGE_NAME}.img"
		mv ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt_preheader.img  ${DEPLOY_DIR_IMAGE}/${BOOT_IMAGE_NAME}.img
        cd  ${DEPLOY_DIR_IMAGE}
        ln -s ${BOOT_IMAGE_NAME}.img ${DATETIME}-${IMAGE_NAME_PREFIX}${IMAGE_NAME_SUFFIX}_loader.img
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
do_image_gpt_preheader
retrieve_ddr_info
set_DDR_info_in_gpt
do_image_gpt_add_Aarch
set_atf_encryption_in_gpt
do_final_image
