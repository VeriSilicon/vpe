#!/bin/bash

# 2. ./ptest <task numbers> <test type> <stream>
#    <task numbers> can be 0-128
#    <test type>:
#         decoder: decoder test
#         encoder: 720p nv12 rawdata encoder test,
#         transcoder: 264->264 transcoding test
#         <stream>: test stream file
#
# Example:
# #> ./ptest 16 transcoder test_1080p.h264
#

echo "VPE performance test start..."

## check $1
expr $1 "+" 10 &> /dev/null
if [[ $1 != "" && $? -eq 0 ]];then
  echo "will start $1 test instance"
else
  echo "Wrong instance numbers $1"
  exit 1
fi

## check $2
if [[ "$2" != "decoder" && "$2" != "encoder" && "$2" != "transcoder" ]]; then
  echo "Wrong test type $2"
  exit 1
fi

## check $3
if [ ! -f "$3" ]; then
  echo "File $3 is not exist"
  exit 1
fi

file=$3

device=transcoder0

while(true)
do
    joblist=($(jobs -p))
    while (( ${#joblist[*]} >= $1 ))
    do
        sleep 1
        joblist=($(jobs -p))
    done

    if [ -f "srmtool" ]; then
      device=`./srmtool allocate 720p 1 performance`
      if [ "$device" == "" ]; then
          continue
      else
          echo "allocated device ${device}"
      fi
    fi

    echo "start job $[${#joblist[*]}+1]..."

    if [ "$2" == "decoder" ]; then
        ffmpeg -y -filter_threads 1 -filter_complex_threads 1 -threads 1 -init_hw_device \
        vpe=dev0:/dev/$device -c:v h264_vpe -i ${file} -filter_complex "hwdownload,format=nv12" -f null /dev/null &
    elif [ "$2" == "encoder" ]; then
        ffmpeg -y -filter_threads 1 -filter_complex_threads 1 -threads 1 -init_hw_device \
        vpe=dev0:/dev/$device -s 1280x720 -pix_fmt nv12 \
        -i ${file} -filter_complex 'vpe_pp' -c:v h264enc_vpe -preset superfast -b:v 10000000 -f null /dev/null &
    elif [ "$2" == "transcoder" ]; then
        ffmpeg -y -filter_threads 1 -filter_complex_threads 1 -threads 1 -init_hw_device \
        vpe=dev0:/dev/"${device}" -c:v h264_vpe -transcode 1 \
        -i ${file} -c:v h264enc_vpe -preset superfast -b:v 10000000 -f null /dev/null &
    fi
    sleep 2
    if [ $? != 0 ]; then exit 1; fi
done
