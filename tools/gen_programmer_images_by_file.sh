#!/bin/bash
#set -x
#################################################################
#################################################################
#################################################################
ROOTDIR=`pwd`

IMAGES_DIR=${ROOTDIR}/images
PROG_IMG_DIR=${ROOTDIR}/images/programmer_images

COMBINED_ONEBIN_IMAGE="combined_onebin_image.bin"

echo "$1"
if [ ! -f "$1" ]; then
echo "No table file!"
exit -1
else
echo "Use programmer_image_table.csv to generate proggramer image!"
part_table=$1
fi

if [ "$2" != "" ]; then
COMBINED_ONEBIN_IMAGE=$2
fi


### func ##################################
alignmet_unit=131072  #128KB
check_alignment(){

x=$(($1%alignmet_unit))
if [ $x -gt 0 ];then
	printf "Failed. Addr(%u) isn't %u alignmet\n" $1 $alignmet_unit
	exit -1
fi
}

##############READ input table ####################

count=0
while IFS= read -r line
    do
        echo "Line: $line"
	if echo $line | egrep "^#" > /dev/null; then
		continue;
	fi	
	file_addr[$count]=$(echo $line | cut -d',' -f1)
	file_addr[$count]=$(eval echo ${file_addr[$count]})
	check_alignment ${file_addr[$count]}
	file_name[$count]=$(echo -n $line | cut -d',' -f2 | tr -d '\r')
	file_name[$count]=$(eval echo ${file_name[$count]} )
	count=$(($count+1))
done < $part_table

########################################################
if [ ! -d ${PROG_IMG_DIR} ];then
mkdir ${PROG_IMG_DIR}
fi
rm -f ${PROG_IMG_DIR}/${COMBINED_ONEBIN_IMAGE}
cd ${PROG_IMG_DIR}
rm -f ./.prog-imgs-combine.log
######### main ###################################
dd_unit=512
len=${#file_addr[@]}
for ((i=0; i < len; i++)); do

size_r=0
#### part  size
offset=$(printf "%u" ${file_addr[$i]})
sec=$(($i + 1))
offset1=$(printf "%u" ${file_addr[$sec]})

size_r=$(( ${offset1} - ${offset}))


echo "File: ${file_name[$i]} : $offset -->$offset1 : $size_r" | tee -a ./.prog-imgs-combine.log

seek_off=$(printf "%u" ${file_addr[$i]})


dd_unit_off=$(($seek_off/${dd_unit}))

#### file size

#target_file=${IMAGES_DIR}/${file_name[$i]}
target_file=${file_name[$i]}
if [ ! -f ${target_file} ];then
echo "Missing ${target_file}"
exit -1
fi

file_real_size=$(du -bL ${target_file} | awk -F' ' '{print $1}')
echo "=====file_real_size ${file_real_size} ===" | tee -a ./.prog-imgs-combine.log

if [ $size_r -gt 0 ];then
if [ ${file_real_size} -gt ${size_r} ];then
printf "Error: Real File Size(%u) > Space(%u)\n" $file_real_size $size_r 
exit -1
fi
fi

#### calu part and file szie
if [ ${size_r} -gt 0 ];then
seek_bl_cnt=$(($size_r/${dd_unit}))
else
seek_bl_cnt=$((${file_real_size}/${dd_unit}))
tmp_res=$((${file_real_size}%${dd_unit}))
if [ $tmp_res -gt 0 ];then
seek_bl_cnt=$((${seek_bl_cnt} + 1))
fi
fi

### fill with 0xFF by DD
if [ ${seek_bl_cnt} -gt 0 ];then
tr '\000' '\377' < /dev/zero| dd of=${COMBINED_ONEBIN_IMAGE} bs=${dd_unit} count=${seek_bl_cnt} seek=${dd_unit_off}  conv=notrunc
echo "tr '\000' '\377' < /dev/zero| dd of=${COMBINED_ONEBIN_IMAGE} bs=${dd_unit} count=${seek_bl_cnt} seek=${dd_unit_off}  conv=notrunc" | tee -a ./.prog-imgs-combine.log
fi



### fill with input file by DD

echo "file_real_size = ${file_real_size}"
echo "dd if=${target_file} of=${COMBINED_ONEBIN_IMAGE} bs=${dd_unit} seek=${dd_unit_off}   conv=notrunc" | tee -a ./.prog-imgs-combine.log
dd if=${target_file} of=${COMBINED_ONEBIN_IMAGE} bs=${dd_unit} seek=${dd_unit_off}   conv=notrunc


done
