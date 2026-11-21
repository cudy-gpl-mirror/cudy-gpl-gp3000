#!/bin/sh

# framework update script

fw_img="framework.img"
md5_cmp="md5.txt"
md5_cmd="/bin/md5sum"
#md5 run-time result
tar_file=$1

tar -xf $tar_file $md5_cmp 
if [ $? != 0 ]; then 
    echo "Failed to extract $tar_file, aborted image updating !"
    exit 1
fi

# Stop this script upon any error
# set -e
echo "Pausing framework......"
dbus-send --system --print-reply --dest=com.ctc.saf1 /com/ctc/saf1 com.ctc.saf1.framework.Pause uint32:"10"

echo "Updating framework with file $tar_file"

# Find out kernel/rootfs mtd partition according to image destination
fw_mtd="mtd"`dbus-send --system --print-reply --dest=com.ctc.saf1 /com/ctc/saf1 com.ctc.igd1.Properties.Get string:"com.ctc.saf1.framework" string:"BackupMTD" | grep "variant" | tr -d -c 0-9`
fw_mtd_size=`cat /proc/mtd | grep $fw_mtd | sed 's/^.*: //g' | sed 's/ .*$//g'`
echo "framework image is located at $fw_mtd size is $fw_mtd_size"

# Extract framework image
img_sum=$(tar -xf "${tar_file}" "${fw_img}" -O | md5sum)
img_sum=${img_sum// */}
img_chk=$(grep ${fw_img} ${md5_cmp})
img_chk=${img_chk// */}

# Check integrity
if [ ! "${img_chk}" = "${img_sum}" ]; then
    echo "$k_img""md5_sum inconsistent, aborted image updating !"
    exit 1
fi

echo "Integrity of $fw_img is okay."

size=$(tar -tvf "${tar_file}" "${fw_img}")
oldIFS="${IFS}"
IFS=" "
set -- ${size}
IFS="${oldIFS}"
img_size_dec=$3
mtd_size_dec="`printf %d 0x$fw_mtd_size`"
expr "$img_size_dec" \< "$mtd_size_dec" > /dev/null
if [ $? != 0 ]; then
	echo "$fw_img size too big($img_size_dec) !"
	echo "3" > /var/firmware_upgrade_status
	exit 1
fi

echo "framework size check okay, start updating ..."

# Erase kernel partition 
flash_erase /dev/$fw_mtd 0 0
# Write kernel partition
echo "Writing $fw_img to /dev/$fw_mtd"
tar -tvf "${tar_file}" "${fw_img}" -O > /dev/$fw_mtd

cur_ver=`saf getfwkver`
up_ver=`saf getbakfwkver`

echo "Current framework version($cur_ver)"
echo "Update framework version($up_ver)"

genFwVersion()
{
	local s=${1#V}
	local n=0
	local i=0
	while [ ! $i = 5 ]
	do
		v=${s##*.}
		n=$(($n+($v<<(($i)*8))))
		if [ "$s" = "${s%.*}" ]; then
			break
		fi
		s=${s%.*}
		i=$(($i+1))
	done
	echo $n
}
cur_ver_num=0
up_ver_num=1
if [ ! "${cur_ver//*V/}" = "${cur_ver}" ] && [ ! "${up_ver//*V/}" = "${up_ver}" ]; then
	cur_ver_num=$(genFwVersion ${cur_ver})
	up_ver_num=$(genFwVersion ${up_ver})
elif [ ! "${cur_ver//*_R/}" = "${cur_ver}" ] && [ ! "${up_ver//*_R/}" = "${up_ver}" ]; then
	cur_ver_num=${cur_ver//*_R/}
	up_ver_num=${up_ver//*_R/}
fi

if [ ! $up_ver_num -gt $cur_ver_num ]; then
	echo "current Framework version($cur_ver) is newer, ignore Framwork update !!"
else
	echo "Do Framwork update !!"
	#Update
	dbus-send --system --print-reply --dest=com.ctc.saf1 /com/ctc/saf1 com.ctc.saf1.framework.Switch
fi

#Restart framework
echo "Restarting framework......"
dbus-send --system --print-reply --dest=com.ctc.saf1 /com/ctc/saf1 com.ctc.saf1.framework.Pause uint32:"0"

# Post processing (for future extension consideration)
echo "Successfully updated framework!!"

