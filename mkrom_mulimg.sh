#!/bin/sh

# This script is for generate image for multicast upgrade
# 1. Build SDK
# 2. Build loader, copy bootloader image to images
# 3. echo -n "[provice code]" > images/config_province
# 		ex) echo -n "shh_2" > images/config_province
# 4. if want to modify volume should create a file "images/config_volume"
# 		1) the value should in hex
#		2) should put decrease before increase to ensure that it have enough blocks to resize.
#		3) recommend resize partition alone with "mul_volume.img".
#		4) ex: 
#			ubi_k0=600000
#			ubi_r0=1900000
# 5. run mkrom_mulimg.sh
# 6. Script will generate image 
    # * framework: mul_framework.img
    # * loader(include env, bootloader, resize volume): bloader.img
    # * kernel+rootfs: mul_vm.img
    # * province: mul_province.img
	# * fpga: mul_fpgafs.img
    # * kernel+rootfs+framework+(fpgafs): vm_framework.img  
	# * boot+kernel+rootfs+(fpgafs): vm_boot.img  
    # * boot+kernel+rootfs+framework+(fpgafs): vm_boot_framework.img
    # * boot+kernel+rootfs+province+(fpgafs): vm_boot_province.img
    # * boot+kernel+rootfs+framework+province+(fpgafs): vm_boot_framework_province.img
    # * ubi: mul_ubi.img
    # * boot+ubi: boot_ubi.img
    # * boot+province+ubi: boot_province_ubi.img


PRECONFIG=`ls -a | grep preconfig | cut -c13-39`
BOOTLOADER_NAME=$PRECONFIG\_loader.img
DSTFOLDER=multiupgrade
FOLDERIMAGES=images
DSTPATH=$FOLDERIMAGES/$DSTFOLDER
VMIMG=$FOLDERIMAGES/vm.img
UBIIMG=$FOLDERIMAGES/G3_ubi_device.ubi
BOOTIMG=$FOLDERIMAGES/$BOOTLOADER_NAME
FRAMEWORKIMG=$FOLDERIMAGES/framework.img
CONFIG_PROVINCE=$FOLDERIMAGES/config_province
ENV_SETTING=$FOLDERIMAGES/uboot-env.bin
VOLUME_FILE=$FOLDERIMAGES/config_volume
DTB_FILE=$FOLDERIMAGES/dtb.img
FPGAFS_FILE=$FOLDERIMAGES/fpgafs

if [ ! -f "$BOOTIMG" ]; then
	BOOTLOADER_NAME=venus_spi_nand_loader.img
	BOOTIMG=$FOLDERIMAGES/$BOOTLOADER_NAME
fi 

if [ ! -f "$BOOTIMG" ]; then
	BOOTLOADER_NAME=taurus_spi_nand_loader.img
	BOOTIMG=$FOLDERIMAGES/$BOOTLOADER_NAME
fi

if [ -d "$DSTPATH" ]; then
	rm -rf $DSTPATH
fi

mkdir $DSTPATH

if [ ! -d "$DSTPATH" ]; then
	echo "No create folder ..."
	exit 1
fi

flash_base=0xbd000000
ImageOffsetHex=`echo "obase=16;ibase=10; $(($flash_base))" | bc`

if [ -f "$VMIMG" ]; then
	cp $VMIMG $DSTPATH/mul_vm.img
fi

if [ -f "$UBIIMG" ]; then
	key="0xa0000503"
	echo "key:"$key
	
	./tools/packimg/genhead -i $UBIIMG -o $DSTPATH/mul_ubi.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
	cat $DSTPATH/mul_ubi.hdr $UBIIMG > $DSTPATH/mul_ubi.img
	rm $DSTPATH/mul_ubi.hdr
fi

if [ -f "$ENV_SETTING" ]; then
	key="0xb0020001"
	echo "key:"$key
	
	./tools/packimg/genhead -i $ENV_SETTING -o $DSTPATH/mul_env.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
	cat $DSTPATH/mul_env.hdr $ENV_SETTING > $DSTPATH/mul_env.img
	rm $DSTPATH/mul_env.hdr
else
	echo "skip Environment file '${ENV_SETTING}'"
fi

if [ -f "$VOLUME_FILE" ]; then
	key="0xa0000603"
	echo "key:"$key
	
	cp $VOLUME_FILE $DSTPATH/config_volume_backup
	
	config_size=`stat --printf='%s' $DSTPATH/config_volume_backup`
	result=`expr $config_size % 4`
	
	while [ $result -ne 0 ]
	do
		echo -ne "\0" >> $VOLUME_FILE
		config_size=`stat --printf='%s' $DSTPATH/config_volume_backup`
		result=`expr $config_size % 4`
	done
	
	./tools/packimg/genhead -i $DSTPATH/config_volume_backup -o $DSTPATH/volume.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
	cat $DSTPATH/volume.hdr $DSTPATH/config_volume_backup > $DSTPATH/mul_volume.img
	rm $DSTPATH/volume.hdr $DSTPATH/config_volume_backup
else
	echo "skip Volume setting file '${VOLUME_FILE}'"
fi

if [ -f "$DTB_FILE" ]; then
	key="0xa0000443"
	echo "key:"$key
	
	./tools/packimg/genhead -i $BOOTIMG -o $DSTPATH/mul_dtb.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
	cat $DSTPATH/mul_dtb.hdr $BOOTIMG > $DSTPATH/mul_dtb.img
	rm $DSTPATH/mul_dtb.hdr
fi

if [ -f "$BOOTIMG" ]; then
	key="0xb0010001"
	echo "key:"$key
	
	./tools/packimg/genhead -i $BOOTIMG -o $DSTPATH/mul_loader.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
	cat $DSTPATH/mul_loader.hdr $BOOTIMG > $DSTPATH/mul_loader.img
	rm $DSTPATH/mul_loader.hdr
	
	if [ -f $DSTPATH/mul_env.img ]; then
		cat $DSTPATH/mul_loader.img $DSTPATH/mul_env.img > $DSTPATH/bloader.img
	else
		echo "no env setting."
		cp $DSTPATH/mul_loader.img $DSTPATH/bloader.img
	fi
	
	if [ -f $DSTPATH/mul_volume.img ]; then
		cat $DSTPATH/mul_loader.img $DSTPATH/mul_volume.img > $DSTPATH/mul_loader_tmp.img
		mv $DSTPATH/mul_loader_tmp.img $DSTPATH/bloader.img
	fi
fi

if [ -f "$FRAMEWORKIMG" ]; then
	key="0xa0000003"
	echo "key:"$key
	
	cp $FRAMEWORKIMG $DSTPATH/framework_backup

	config_size=`stat --printf='%s' $DSTPATH/framework_backup`
	result=`expr $config_size % 4`

	while [ $result -ne 0 ]
	do
		echo -ne "\\0" >> $DSTPATH/framework_backup
		config_size=`stat --printf='%s' $DSTPATH/framework_backup`
		result=`expr $config_size % 4`
	done
	
	./tools/packimg/genhead -i $DSTPATH/framework_backup -o $DSTPATH/mul_framework.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
	cat $DSTPATH/mul_framework.hdr $DSTPATH/framework_backup > $DSTPATH/mul_framework.img
	rm $DSTPATH/mul_framework.hdr $DSTPATH/framework_backup
fi

if [ -f "$FPGAFS_FILE" ]; then
	key="0xa0000423"
	echo "key:"$key
	
	./tools/packimg/genhead -i $FPGAFS_FILE -o $DSTPATH/mul_fpgafs.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
	cat $DSTPATH/mul_fpgafs.hdr $FPGAFS_FILE > $DSTPATH/mul_fpgafs.img
	rm $DSTPATH/mul_fpgafs.hdr
fi

if [ -f "$CONFIG_PROVINCE" ]; then
	key="0xCF010202"
	echo "key:"$key

	cp $CONFIG_PROVINCE $DSTPATH/config_province_backup

	config_size=`stat --printf='%s' $DSTPATH/config_province_backup`
	result=`expr $config_size % 4`

	while [ $result -ne 0 ]
	do
		echo -ne "\\0" >> $DSTPATH/config_province_backup
		config_size=`stat --printf='%s' $DSTPATH/config_province_backup`
		result=`expr $config_size % 4`
	done
	
	./tools/packimg/genhead -i $DSTPATH/config_province_backup -o $DSTPATH/mul_province.hdr -k $key -f $ImageOffsetHex -a 0x80000000 -e 0x80000000
	cat $DSTPATH/mul_province.hdr $DSTPATH/config_province_backup > $DSTPATH/mul_province.img
	rm $DSTPATH/mul_province.hdr $DSTPATH/config_province_backup
fi

MUL_BOOTIMG=
if [ -f "$DSTPATH/bloader.img" ]; then
	MUL_BOOTIMG=$DSTPATH/bloader.img
else
	MUL_BOOTIMG=$DSTPATH/mul_loader.img
fi

if [ -f "$DSTPATH/mul_framework.img" -a -f "$DSTPATH/mul_vm.img" ]; then
	cat $DSTPATH/mul_vm.img $DSTPATH/mul_framework.img > $DSTPATH/vm_framework.img
	if [ -f "$DSTPATH/mul_fpgafs.img" ]; then 
		cat $DSTPATH/mul_fpgafs.img >> $DSTPATH/vm_framework.img
	fi
fi

if [ -f "$MUL_BOOTIMG" -a -f "$DSTPATH/mul_vm.img" ]; then
	cat $MUL_BOOTIMG > $DSTPATH/vm_boot.img
	cat $DSTPATH/mul_vm.img >> $DSTPATH/vm_boot.img
	if [ -f "$DSTPATH/mul_fpgafs.img" ]; then 
		cat $DSTPATH/mul_fpgafs.img >> $DSTPATH/vm_boot.img
	fi

	if [ -f "$DSTPATH/mul_province.img" ] && [ -f "$DSTPATH/mul_framework.img" ]; then
		cat $DSTPATH/vm_boot.img $DSTPATH/mul_framework.img $DSTPATH/mul_province.img > $DSTPATH/vm_boot_framework_province.img
	elif [ -f "$DSTPATH/mul_framework.img" ]; then
		cat $DSTPATH/vm_boot.img $DSTPATH/mul_framework.img > $DSTPATH/vm_boot_framework.img
	fi
fi

# mul_ubi.img must be last one
if [ -f "$MUL_BOOTIMG" -a -f "$DSTPATH/mul_ubi.img" ]; then
	cat $MUL_BOOTIMG $DSTPATH/mul_ubi.img >> $DSTPATH/boot_ubi.img
	
	if [ -f "$DSTPATH/mul_province.img" ]; then
		cat $MUL_BOOTIMG $DSTPATH/mul_province.img $DSTPATH/mul_ubi.img > $DSTPATH/boot_province_ubi.img
	fi
fi

