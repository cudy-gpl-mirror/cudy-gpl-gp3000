#!/bin/sh
#set -x
#################################################################
# Taurus Partitions 
#################################################################
# #: name                size            offset          mask_flags
# 0: boot                0x00180000      0x00000000      0
# 1: env                 0x00040000      0x00280000      0
# 2: env2                0x00040000      0x002C0000      0
# 3: static_conf         0x00040000      0x00300000      0
# 4: ubi_device          0x0f5c0000      0x00340000      0
#################################################################
ROOTDIR=`pwd`
BOOTLOADER_DIR=${ROOTDIR}/bootloader
PROG_IMG_DIR=${ROOTDIR}/images/programmer_images

PLATFORM_CONFIG="Flash_and_DDR_Support.txt"
pfl_env1_r=1376256
pfl_env2_r=1441792
pfl_cfg=1507328
pfl_fdt1=11900544
pfl_kernel1=2162688
pfl_rootfs1=5308416
STATIC_CONFIG_SIZE=393216

BOOTLOADER_IMG="bootloader.bin"
UBOOTENV_IMG="uboot-env.bin"
FDT_IMG="dtb.img"
KERNEL_IMG="Image.lzma"
ROOTFS_IMG="rootfs"

ALL_ONE_FILE="all_one_file.bin"

PROGRAMMER_LOADER_IMAGE="programmer_loader_image.bin"
PROGRAMMER_ONEBIN_IMAGE="programmer_onebin_image.bin"


CHUNK_SIZE=4096
#usage() {
#     echo "prog <in-filename> <out-filename> <thread_n>"
#}
# func <in-filename> <out-filename> <thread_n>
bin2nand_split() {
    if [ "$#" -lt 2 ]; then
        usage
        exit 1
    fi 

    if [ ! -f "$1" ]; then
        echo "$1 not exist"
        exit 1
    fi

    let cpu_nums=`cat /proc/cpuinfo | grep processor | wc -l`
    
    set -e
    let file_size=`stat -c '%s' $1`
    let split_size="($file_size/$cpu_nums+$CHUNK_SIZE-1)/$CHUNK_SIZE*$CHUNK_SIZE"

    split -b $split_size $1 -d tmp_

    for file in tmp_*; do 
    {
        ${BIN2NAND_TOOL} $file --ecc-layout=BCH16 --pagesize=$PAGE_SIZE > fl_$file
        rm -f $file
    } &
    done

    wait

    cat  fl_tmp_* > $2
    rm -f fl_tmp_*

    set +e
}


get_image_combine_addr(){
    echo "------------------------------------" 
    env1_addr=`printf "%d" ${pfl_env1_r}`
    env2_addr=`printf "%d" ${pfl_env2_r}`
    static_addr=`printf "%d" ${pfl_cfg}`
    fdt_addr=`printf "%d" ${pfl_fdt1}`
    kernel_addr=`printf "%d" ${pfl_kernel1}`
    rootfs_addr=`printf "%d" ${pfl_rootfs1}`


    #### Generate ALL ONE file for padding
    tr '\000' '\377' < /dev/zero| dd of=${ALL_ONE_FILE} bs=1 count=${STATIC_CONFIG_SIZE}    

    
    echo "get_image_combine_addr: env1_addr=${env1_addr}"
    echo "get_image_combine_addr: env2_addr=${env2_addr}"

    if [ "${ONLY_BOOTLOADER}" = 0 ];then
        echo "get_image_combine_addr: static_addr=${static_addr}"
        echo "get_image_combine_addr: fdt_addr=${fdt_addr}"
        echo "get_image_combine_addr: kernel_addr=${kernel_addr}"
        echo "get_image_combine_addr: rootfs_addr=${rootfs_addr}"
    fi
}

gen_sflash_image(){
    echo "------------------------------------" 
    echo "@Generate SPI flash combined image of bootloader, uboot-env, DTB, kernel and rootfs"

    ln -s ${ROOTDIR}/bootloader/image/${soc_name}_*loader*.img ${ROOTDIR}/bootloader/image/${BOOTLOADER_IMG}
    ln -s ${ROOTDIR}/bootloader/image/*-env*.bin ${ROOTDIR}/bootloader/image/${UBOOTENV_IMG}
    
    echo "dd if=${BOOTLOADER_DIR}/image/${BOOTLOADER_IMG} of=${PROGRAMMER_LOADER_IMAGE} seek=0  conv=notrunc" | tee -a ./.programmer-image-combine.log
    echo "dd if=${BOOTLOADER_DIR}/image/${UBOOTENV_IMG} of=${PROGRAMMER_LOADER_IMAGE} bs=1 seek=${env1_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
    echo "dd if=${BOOTLOADER_DIR}/image/${UBOOTENV_IMG} of=${PROGRAMMER_LOADER_IMAGE} bs=1 seek=${env2_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
    dd if=${BOOTLOADER_DIR}/image/${BOOTLOADER_IMG} of=${PROGRAMMER_LOADER_IMAGE} seek=0  conv=notrunc 
    dd if=${BOOTLOADER_DIR}/image/${UBOOTENV_IMG} of=${PROGRAMMER_LOADER_IMAGE} bs=1 seek=${env1_addr}  conv=notrunc
    dd if=${BOOTLOADER_DIR}/image/${UBOOTENV_IMG} of=${PROGRAMMER_LOADER_IMAGE} bs=1 seek=${env2_addr}  conv=notrunc
    if [ "${BOOTDEV}" = "SPI" ];then
        echo "cp ${PROGRAMMER_LOADER_IMAGE} ${SFLASH_PROGGRAMER_IMG_LOADER}" | tee -a ./.programmer-image-combine.log          
        cp ${PROGRAMMER_LOADER_IMAGE} ${SFLASH_PROGGRAMER_IMG_LOADER}
    fi

    if [ "${ONLY_BOOTLOADER}" = 0 ];then
        cp ${PROGRAMMER_LOADER_IMAGE} ${PROGRAMMER_ONEBIN_IMAGE}
        echo "dd if=${ROOTDIR}/images/${FDT_IMG} of=${PROGRAMMER_ONEBIN_IMAGE} bs=1 seek=${pfl_fdt1}  conv=notrunc" | tee -a ./.programmer-image-combine.log
        echo "dd if=${ROOTDIR}/images/${KERNEL_IMG} of=${PROGRAMMER_ONEBIN_IMAGE} bs=1 seek=${kernel_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
        echo "dd if=${ROOTDIR}/images/${ROOTFS_IMG} of=${PROGRAMMER_ONEBIN_IMAGE} bs=1 seek=${rootfs_addr}  conv=notrunc" | tee -a ./.programmer-image-combine.log
        dd if=${ROOTDIR}/images/${FDT_IMG} of=${PROGRAMMER_ONEBIN_IMAGE} bs=1 seek=${pfl_fdt1}  conv=notrunc
        dd if=${ROOTDIR}/images/${KERNEL_IMG} of=${PROGRAMMER_ONEBIN_IMAGE} bs=1 seek=${kernel_addr}  conv=notrunc
        dd if=${ROOTDIR}/images/${ROOTFS_IMG} of=${PROGRAMMER_ONEBIN_IMAGE} bs=1 seek=${rootfs_addr}  conv=notrunc
        
        if [ "${BOOTDEV}" = "SPI" ];then
            echo "cp ${PROGRAMMER_ONEBIN_IMAGE} ${SFLASH_PROGGRAMER_IMG_COMBINED}" | tee -a ./.programmer-image-combine.log          
            cp ${PROGRAMMER_ONEBIN_IMAGE} ${SFLASH_PROGGRAMER_IMG_COMBINED}
        fi
    fi
   
    rm ${ROOTDIR}/bootloader/image/${BOOTLOADER_IMG}
    rm ${ROOTDIR}/bootloader/image/${UBOOTENV_IMG} 
}


################################
# main
################################
if [ ! -d ${ROOTDIR}/images ];then
    echo "Can't find ${ROOTDIR}/images !"
    exit 1
fi

if [ ! -d ${PROG_IMG_DIR} ];then
    mkdir  ${PROG_IMG_DIR}
else
    rm -rf ${PROG_IMG_DIR}/*
fi

#Step1: Check only bootloader
ONLY_BOOTLOADER="$1"
if [ "ONLY_BOOTLOADER" = "$1" ];then
    ONLY_BOOTLOADER=1
else
    ONLY_BOOTLOADER=0
fi
echo "ONLY_BOOTLOADER=${ONLY_BOOTLOADER}"
soc_name="$2"

#Step2: cd ${ROOTDIR}/programmer_images
echo "ROOTDIR=${ROOTDIR}"
echo "BOOTLOADER_DIR=${BOOTLOADER_DIR}"
echo "PROG_IMG_DIR=${PROG_IMG_DIR}"
cd ${PROG_IMG_DIR}

#Step3: get address for combine image
get_image_combine_addr

#Step4: cd ${ROOTDIR}/programmer_images
gen_sflash_image

#Step5: Remove tmpp files
rm ${ALL_ONE_FILE}
