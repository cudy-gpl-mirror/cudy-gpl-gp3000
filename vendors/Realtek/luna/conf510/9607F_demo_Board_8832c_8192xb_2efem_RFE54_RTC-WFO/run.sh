#!/bin/sh
ARCH=arm64
DTS_PATH=${ROOTDIR}/${LINUXDIR}/arch/${ARCH}/boot/dts/realtek/
if [ ! -d "$1" ]; then 
	exit; 
fi

if [ -e $1/dts ] ;then
	find ${1}/dts/ -name '*.dts'  -exec cp {} ${DTS_PATH} \; 
	find ${1}/dts/ -name '*.dtsi' -exec cp {} ${DTS_PATH} \; 
fi
if [ -e $1/board_init.sh ] ;then
        rm -f ${ROOTDIR}/user2/proc_var/board_init.sh
        ln -s $1/board_init.sh ${ROOTDIR}/user2/proc_var/board_init.sh
fi

if [ -e $1/scfg.txt ] ;then
	cp $1/scfg.txt ${ROOTDIR}/ca_packages/ca-scfg/1.0-r0/ca-scfg-1.0/customize_scfg.txt
fi
