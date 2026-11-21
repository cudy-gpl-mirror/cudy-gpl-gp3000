#/bin/sh
ROOTDIR=$1
LINUX_DIR=$2
LZMA_TOOL=${ROOTDIR}/tools/usr/bin/lzma
if [ -f ${ROOTDIR}/bootloader/image/mkimage ]; then
MKIMG=${ROOTDIR}/bootloader/image/mkimage
else
MKIMG=${ROOTDIR}/tools/uboot_2018/mkimage
fi
DTB_A=rtl8277C-engboard.dtb
DTB_B=rtl9607F-engboard.dtb
ITS=uboot_fit_image_2DTBS_taurus_elnath.its
if [ ! -f ${MKIMG} ] ;then
	echo "Error: Not found mkimage: ${MKIMG}"
	exit 1
fi
cd ${ROOTDIR}/images


if [ -f ${ROOTDIR}/${LINUX_DIR}/arch/arm64/boot/dts/realtek/${DTB_A} ];then
	${LZMA_TOOL} -kzc   ${ROOTDIR}/${LINUX_DIR}/arch/arm64/boot/dts/realtek/${DTB_A} > ${DTB_A}.lzma;
else
	echo "Miss ${ROOTDIR}/${LINUX_DIR}/arch/arm64/boot/dts/realtek/${DTB_A}";
	exit 1
fi
if [ -f ${ROOTDIR}/${LINUX_DIR}/arch/arm64/boot/dts/realtek/${DTB_B} ];then
	${LZMA_TOOL} -kzc   ${ROOTDIR}/${LINUX_DIR}/arch/arm64/boot/dts/realtek/${DTB_B}  > ${DTB_B}.lzma;
else
	echo "Miss ${ROOTDIR}/${LINUX_DIR}/arch/arm64/boot/dts/realtek/${DTB_A}";
	exit 1;
fi

ln -sf ../tools/${ITS}  ${ITS}
echo "Doing: ${MKIMG} -f  ${ITS}  uImage"
${MKIMG} -f  ${ITS}  uImage
if [ ! -f uImage ];then
	echo "uImage generates failed!"
	exit 1
fi

echo "Use uImage instead of Image.lzma. Link Image.lzma to uImage"
rm -f Image.lzma && ln -s uImage Image.lzma

