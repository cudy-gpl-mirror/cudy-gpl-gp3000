#!/bin/sh
# From Cortina G3 SDK R0.5.7.0 
#PROJECT_NAME=g3_eng_RG
#BOARD_NAME=g3_eng
if [ ! -f bootloader.conf ];then
echo "Not find bootloader.conf!!"
exit 1
fi
BOARD_NAME=`grep BOARD_NAME bootloader.conf | awk -F= '{ print $2}' | tr -d ' '`
echo "BOARD_NAME   : $BOARD_NAME"
PROJECT_NAME=`grep PROJECT_NAME bootloader.conf | awk -F= '{ print $2}' | tr -d ' '`
echo "PROJECT_NAME : $PROJECT_NAME"

PWD=`pwd`
DATETIME=`date '+%Y%m%d%H%M'`
IMAGE_BASENAME=${PROJECT_NAME}
MACHINE=${BOARD_NAME}

echo "NAND_INFO_PARAM: "
if [ -f flash_support.csv ];then
echo "Use flash_support.csv: "
while read xxx; do  echo $xxx; done < ./flash_support.csv
NAND_INFO_PARAM="flash_support.csv"
else
#Default, MT29F8G08ABACAWP "3 0xe0 0x1000 0x80000"
echo "MT29F8G08ABACAWP \"3 0xe0 0x1000 0x80000\""
NAND_INFO_PARAM="3 0xe0 0x1000 0x80000"
fi

IMAGE_NAME="${IMAGE_BASENAME}-${MACHINE}-${DATETIME}"

IMAGE_LINK_NAME="${IMAGE_BASENAME}-${MACHINE}"

DEPLOY_DIR_IMAGE=${PWD}/image/

export PATH=${PWD}/host_build_tools:${PATH}

echo "DEPLOY_DIR_IMAGE:	${DEPLOY_DIR_IMAGE}"
echo "IMAGE_LINK_NAME:	${IMAGE_LINK_NAME}"
echo "IMAGE_NAME:	${IMAGE_NAME}"

gend_multi_nand_info(){

     grep -v ONFi ${PWD}/../flash_support.csv > .flash_support.lst
     while read xxx; do 
         MODEL=`echo $xxx | awk -F, '{print $1}'`
         NAND_INFO_PARAM=`echo $xxx | awk -F, '{print $2,$3,$4,$5}'`
         IMAGE_MODLE_NAME=${IMAGE_NAME}_${MODEL}
	echo "-----------------"
	echo ${IMAGE_MODLE_NAME}
	echo ${NAND_INFO_PARAM}
	echo "-----------------"
         gen_nand_info ${NAND_INFO_PARAM} ${DEPLOY_DIR_IMAGE}/${IMAGE_MODLE_NAME}-nandinfo.bin
         cp ${IMAGE_MODLE_NAME}-nandinfo.bin ${IMAGE_MODLE_NAME}-gpt-nandinfo.img
         dd if=${IMAGE_NAME}-gpt.img of=${IMAGE_MODLE_NAME}-gpt-nandinfo.img skip=1 oflag=append conv=notrunc
    done < ./.flash_support.lst
}



do_generate_fip_image_key() {
        cert_create  \
        -n  \
        --tfw-nvctr 0  \
        --ntfw-nvctr 0  \
        --trusted-key-cert ${DEPLOY_DIR_IMAGE}/fip/trusted_key.crt  \
        --key-alg rsa  \
        --rot-key ${DEPLOY_DIR_IMAGE}/fip/rotprivk_rsa.pem  \
        --tb-fw     ${DEPLOY_DIR_IMAGE}/fip/bl2.bin  \
        --tb-fw-cert ${DEPLOY_DIR_IMAGE}/fip/tb_fw.crt  \
        --soc-fw     ${DEPLOY_DIR_IMAGE}/fip/bl31.bin \
        --soc-fw-cert ${DEPLOY_DIR_IMAGE}/fip/soc_fw_content.crt  \
        --soc-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/soc_fw_key.crt \
        --nt-fw ${DEPLOY_DIR_IMAGE}/u-boot.bin  \
        --nt-fw-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_content.crt  \
        --nt-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_key.crt
}

do_generate_fip_image() {
        fip_create \
        --dump \
        --trusted-key-cert ${DEPLOY_DIR_IMAGE}/fip/trusted_key.crt \
        --tb-fw-cert ${DEPLOY_DIR_IMAGE}/fip/tb_fw.crt \
        --soc-fw-cert ${DEPLOY_DIR_IMAGE}/fip/soc_fw_content.crt \
        --soc-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/soc_fw_key.crt \
        --nt-fw-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_content.crt \
        --nt-fw-key-cert ${DEPLOY_DIR_IMAGE}/fip/nt_fw_key.crt \
        --tb-fw ${DEPLOY_DIR_IMAGE}/fip/bl2.bin \
        --soc-fw ${DEPLOY_DIR_IMAGE}/fip/bl31.bin \
        --nt-fw ${DEPLOY_DIR_IMAGE}/u-boot.bin \
        ${DEPLOY_DIR_IMAGE}/fip/fip.bin
}

do_image_fip() {
        do_generate_fip_image_key
        do_generate_fip_image

        cd ${DEPLOY_DIR_IMAGE}
        ln -sf ${DEPLOY_DIR_IMAGE}/fip/fip.bin  ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}.fip.bin
        ln -sf ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}.fip.bin  ${IMAGE_LINK_NAME}-fip.bin
}


do_image_gpt() {

    TEMP_IMAGE_SIZE=3
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
    if [ "x${EXTRA_PARTITIONS}" != 'x' ]; then
        for kv in `echo "${EXTRA_PARTITIONS}" | sed -e 's/.*;//' -e 's/,/ /g'`; do
            eval part_${kv%M}
        done
        TEMP_IMAGE_SIZE=`expr ${part_start} + ${part_size} + 1`
    fi

    dd if=/dev/zero of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img bs=1M count=${TEMP_IMAGE_SIZE}
    sgdisk -n 1:1M:+1000K -t 1:ca777000-f190-b69b-8136-dea2835d4178 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img
    sgdisk -c 1:fip0  ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img
    sgdisk -n 2:2M:+1000K -t 2:ca777000-f190-b69b-8136-dea2835d4178 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img
    sgdisk -c 2:fip1  ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img
    sgdisk -A 1:set:48 ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img

    if [ "x${EXTRA_PARTITIONS}" != 'x' ]; then
        part_idx=3
        for partition in `echo "${EXTRA_PARTITIONS}" | sed -e 's/;/ /g'`  ; do
            for kv in `echo ${partition} | sed -e 's/,/ /g'`; do
                eval part_$kv
            done

            sgdisk -n ${part_idx}:${part_start}:+${part_size} ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img
            sgdisk -c ${part_idx}:${part_name} ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img
            part_idx=`expr ${part_idx} + 1`
        done
    fi

    dd if=${DEPLOY_DIR_IMAGE}/${IMAGE_LINK_NAME}-fip.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img bs=1K seek=1024 conv=notrunc
    dd if=${DEPLOY_DIR_IMAGE}/${IMAGE_LINK_NAME}-fip.bin of=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img bs=1K seek=2048 conv=notrunc
    truncate -s3M ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-gpt.img

    cd ${DEPLOY_DIR_IMAGE}
    ln -sf ${IMAGE_NAME}-gpt.img ${IMAGE_LINK_NAME}-gpt.img

    if [ "x${NAND_INFO_PARAM}" != 'x' ];then
	if [ "x${NAND_INFO_PARAM}" = "xflash_support.csv" ];then
	  gend_multi_nand_info; 
	else
         gen_nand_info ${NAND_INFO_PARAM} ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}-nandinfo.bin
         cp ${IMAGE_NAME}-nandinfo.bin ${IMAGE_NAME}-gpt-nandinfo.img
         dd if=${IMAGE_NAME}-gpt.img of=${IMAGE_NAME}-gpt-nandinfo.img skip=1 oflag=append conv=notrunc
         ln -sf ${IMAGE_NAME}-gpt-nandinfo.img ${IMAGE_LINK_NAME}-gpt-nandinfo.img
	fi
    fi

}

#check base files
if [ ! -d ${DEPLOY_DIR_IMAGE} ]; then
echo "Miss  ${DEPLOY_DIR_IMAGE}!!"
exit
fi

if [ ! -f ${DEPLOY_DIR_IMAGE}/fip/bl2.bin ]; then
echo "Miss ${DEPLOY_DIR_IMAGE}/fip/bl2.bin!"
exit 
fi

if [ ! -f ${DEPLOY_DIR_IMAGE}/fip/bl31.bin ]; then
echo "Miss ${DEPLOY_DIR_IMAGE}/fip/bl31.bin!"
exit 
fi
if [ ! -f ${DEPLOY_DIR_IMAGE}/u-boot.bin ]; then
echo "Miss ${DEPLOY_DIR_IMAGE}/u-boot.bin!"
exit 
fi

#actions
do_image_fip



cd ${DEPLOY_DIR_IMAGE}
do_image_gpt
