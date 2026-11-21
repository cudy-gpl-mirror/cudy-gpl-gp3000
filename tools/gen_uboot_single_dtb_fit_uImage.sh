#/bin/sh
ROOTDIR=$1
LINUX_DIR=$2
LZMA_TOOL=${ROOTDIR}/tools/usr/bin/lzma
if [ -f ${ROOTDIR}/bootloader/image/mkimage ]; then
MKIMG=${ROOTDIR}/bootloader/image/mkimage
else
MKIMG=${ROOTDIR}/tools/uboot_2018/mkimage
fi
DTB_B=$3
ARCH=$4
FIT_SIGNATURE=$5

if [ "$ARCH" = "arm" ];then
kernel_img="zImage"
elif [ "$ARCH" = "arm64" ];then
kernel_img="Image"
else
	echo "arch error!"
	exit 1
fi

cd ${ROOTDIR}/images

if [ ! -f ${MKIMG} ] ;then
        echo "Error: Not found mkimage: ${MKIMG}"
        exit 1
fi


if [ ! -d ${ROOTDIR}/${LINUX_DIR}/arch/${ARCH}/boot/dts/realtek/ ];then
	echo "arch: \"${ARCH}\""
    echo "Miss ${ROOTDIR}/${LINUX_DIR}/arch/${ARCH}";
	exit 1
fi


if [ ! -f ${ROOTDIR}/${LINUX_DIR}/arch/${ARCH}/boot/dts/realtek/${DTB_B} ];then
        echo "Miss ${ROOTDIR}/${LINUX_DIR}/arch/${ARCH}/boot/dts/realtek/${DTB_B}";
        exit 1;
fi

if [ ! -f ${ROOTDIR}/${LINUX_DIR}/arch/${ARCH}/boot/${kernel_img} ];then
        echo "Miss ${ROOTDIR}/${LINUX_DIR}/arch/${ARCH}/boot/${kernel_img}";
        exit 1;
fi

if [ ! -d .fit_uImage ];then
	mkdir .fit_uImage
fi

cd .fit_uImage

rm -f board_raw.dtb ${kernel_img}

ln -sf ${ROOTDIR}/${LINUX_DIR}/arch/${ARCH}/boot/dts/realtek/${DTB_B}	board_raw.dtb

ln -sf ${ROOTDIR}/${LINUX_DIR}/arch/${ARCH}/boot/${kernel_img} ${kernel_img}

if [ "$FIT_SIGNATURE" = "y" ];then
rm -f uboot_verity_script UBOOT_FIP_PRI_KEY
ln -sf ${ROOTDIR}/images/uboot_verity_script uboot_verity_script
ln -sf ${ROOTDIR}/bootloader/Verified_Boot/UBOOT_FIP_PRI_KEY UBOOT_FIP_PRI_KEY
fi

if [ "$ARCH" = "arm64" ];then
	${LZMA_TOOL} -f -k  ${kernel_img}
	if [ "${LINUX_DIR}" = "linux-5.10.x" ];then
		if [ "$FIT_SIGNATURE" = "y" ];then
			ln -sf ${ROOTDIR}/tools/uboot_fit_signature_image_A64_1DTB_linux5.its uboot_fit_image_A64_1DTB_linux5.its
			echo "Doing: ${MKIMG} -f  uboot_fit_signature_image_A64_1DTB_linux5.its uImage"
		else
			ln -sf ${ROOTDIR}/tools/uboot_fit_image_A64_1DTB_linux5.its uboot_fit_image_A64_1DTB_linux5.its
			echo "Doing: ${MKIMG} -f  uboot_fit_image_A64_1DTB_linux5.its uImage"
		fi
		${MKIMG} -f  uboot_fit_image_A64_1DTB_linux5.its uImage
		ret=$?
		if [ $ret -ne 0 ];then
			printf "\033[1;31m   ${MKIMG} Error! \n \033[0m";
			exit -1
		fi
	else
		if [ "$FIT_SIGNATURE" = "y" ];then
			ln -sf ${ROOTDIR}/tools/uboot_fit_signature_image_A64_1DTB.its uboot_fit_image_A64_1DTB.its
			echo "Doing: ${MKIMG} -f  uboot_fit_signature_image_A64_1DTB.its uImage"
		else
			ln -sf ${ROOTDIR}/tools/uboot_fit_image_A64_1DTB.its uboot_fit_image_A64_1DTB.its
			echo "Doing: ${MKIMG} -f  uboot_fit_image_A64_1DTB.its uImage"
		fi
		${MKIMG} -f  uboot_fit_image_A64_1DTB.its uImage
	fi
else
	if [ "$FIT_SIGNATURE" = "y" ];then
		ln -sf ${ROOTDIR}/tools/uboot_fit_signature_image_A32_singleDTB.its uboot_fit_image_A32_singleDTB.its
		echo "Doing: ${MKIMG} -f  uboot_fit_signature_image_A32_singleDTB.its uImage"
	else
		ln -sf ${ROOTDIR}/tools/uboot_fit_image_A32_singleDTB.its uboot_fit_image_A32_singleDTB.its 
		echo "Doing: ${MKIMG} -f  uboot_fit_image_A32_singleDTB.its uImage"
	fi
	${MKIMG} -f  uboot_fit_image_A32_singleDTB.its uImage
		if [ $ret -ne 0 ];then
			printf "\033[1;31m   ${MKIMG} Error! \n \033[0m";
			exit -1
		fi
fi

if [ ! -f uImage ];then
        echo "uImage with ${DTB_B} generated failed!"
        exit 1
fi

if [ "$FIT_SIGNATURE" = "y" ];then
	echo "Doing: ${MKIMG} -F -k UBOOT_FIP_PRI_KEY  -r uImage"
	${MKIMG} -F -k UBOOT_FIP_PRI_KEY  -r uImage
		if [ $ret -ne 0 ];then
			printf "\033[1;31m   ${MKIMG} Error! \n \033[0m";
			exit -1
		fi
fi

cp uImage ${ROOTDIR}/images/
cd ${ROOTDIR}/images/
echo "Use uImage instead of Image.lzma. Link Image.lzma to uImage"
rm -f Image.lzma && ln -s uImage Image.lzma

