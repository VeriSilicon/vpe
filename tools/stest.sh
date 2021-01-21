#!/bin/bash

# VPE stability test script
# ./ptest <task numbers> <test stream path>
#    <task numbers> can be 0-128
# Example:
# #> ./stest 16 ~/work/stream

#set -e
#set -x

# global parameters
fileoutput=disable ##enable
bitrate1=10000000
bitrate2=5000000
bitrate3=1000000
bitrate4=500000
vpelogvel=0
ffmpeglogvel=35
lowres_minw=300
lowres_minh=300
highres_maxw=4096
highres_maxh=2160
esformats="hevc|h264|avi|mp4|mkv|webm|mp4|flv"
yuvformats="nv12|yuv420p|uyvy422|abgr|rgba|agbr|rgb24|p010le"
srm_power_mode="performance"
encoder_params=""
decoder_params=""

#script input parameters
task_num=1
in_stream_path=`pwd`
out_stream_path=`pwd`

#dynamic parameters used by this script
input_codec=
input_file=
output_codec=
transcoder_option=
output_file1=
output_file2=
output_file3=
output_file4=
preset=
cmd_transcoder=
cmd_encoder_pp=
cmd_encoder_hw_uploader=
cmd_decoder=
cmd_common=
device=
file_list_h264=
file_list_hevc=
file_list_vp9=
file_list_yuv=
nums_h264=
nums_hevc=
nums_vp9=
nums_yuv=
low_res=
srm_onepath_loading="1080p"

## check $1
expr $1 "+" 10 &> /dev/null
if [[ $1 != "" && $? -eq 0 ]];then
    task_num=$1
fi

## check $2
if [ ! -d "$2" ]; then
    echo "Folder $2 is not exist"
    exit
else
    in_stream_path=$2
fi

## check $3
if [ ! -d "$3" ]; then
    out_stream_path=$3
fi

trap sumuary SIGHUP SIGINT SIGQUIT SIGKILL SIGTERM SIGSTOP SIGTSTP

#input: input_codec type, output_codec type
#output: input_codec, output_codec, preset, transcoder_option, input_file, output_file, cmd_common
function gen_parameters(){

  #input_codec, input_file, output_codec, preset will be generated randomly
  transcoder_option="-transcode 1"
  input_codec=`get_random_name $1`
  input_file=`get_random_file $input_codec`
  res=${input_file##*_}
  output_codec=`get_random_name $2`
  preset=`get_random_name preset`

  if [ "$fileoutput" == "enable" ]; then
    output_file1=${out_stream_path}/out1.${output_codec%enc_vpe*}
    output_file2=${out_stream_path}/out2.${output_codec%enc_vpe*}
    output_file3=${out_stream_path}/out3.${output_codec%enc_vpe*}
    output_file4=${out_stream_path}/out4.${output_codec%enc_vpe*}
  else
    output_file1="-f null /dev/null"
    output_file2="-f null /dev/null"
    output_file3="-f null /dev/null"
    output_file4="-f null /dev/null"
  fi
}

function get_random_file(){
    codec=$1
    id=0
    if [ "$codec" == "h264_vpe" ]; then
        list=(${file_list_h264[*]})
    elif [ "$codec" == "hevc_vpe" ]; then
        list=(${file_list_hevc[*]})
    elif [ "$codec" == "vp9_vpe" ]; then
        list=(${file_list_vp9[*]})
    elif [ "$codec" == "raw" ]; then
        list=(${file_list_yuv[*]})
    fi

    len=${#list[*]}
    if (($len > 1)); then
        len=$((len-1))
        id=$(rand 0 $len)
    fi
    file=${list[$id]}
    echo "$file"
}

#get a random file with specified key word like h264/hevc/vp9/yuv420p/nv12
function gen_file_list(){
    tmp=".tmp"
    files=`find $1 \( -iname '*.mp4'     \
                  -o -iname '*.mkv'  \
                  -o -iname '*.avi'  \
                  -o -iname '*.h264' \
                  -o -iname '*.hevc' \
                  -o -iname '*.webm' \
                  -o -iname '*.ivf'  \
                  -o -iname '*.yuv'  \) -type f`

    ## search hevc/h264/vp9 files
    for file in ${files[@]}
    do
        name=${file%.*}
        suffix=${file##*.}

        if [ "$suffix" == "yuv" ]; then
          codec=$(echo $name | grep -E -o $yuvformats)
          res=$(echo $name | grep -E -o "[0-9]{3,4}+-?x?h?H?X?-?_?[0-9]{3,4}+")
        else
          ffprobe $file &> $tmp
          codec=$(grep -E -o "Video: h264|Video: hevc|Video: vp9" $tmp)
          codec=${codec##*: }
          res=$(grep -E -o "[0-9]{3,4}+x[0-9]{3,4}+" $tmp)
        fi

        arr=($(echo $res | sed -r "s/([[:digit:]]+)[^[:digit:]]*([[:digit:]]+)/\1 \2/g"))
        w=${arr[0]}
        h=${arr[1]}
        res=${w}x${h}

        if [[ $w -lt $lowres_minw ]] || [[ $h -lt $lowres_minh ]]; then
            continue
        fi

        if [[ $w -gt $highres_maxw ]] || [[ $h -gt $highres_maxh ]]; then
            continue
        fi

        if [ "$codec" == "" ] || [ "$res" == "" ]; then
            continue
        fi

        file=${file}_${res}
        case $codec in
        "h264")
        file_list_h264[$nums_h264]=$file
        nums_h264=$((nums_h264+1))
            ;;
        "hevc")
        file_list_hevc[$nums_hevc]=$file
        nums_hevc=$((nums_hevc+1))
            ;;
        "vp9")
        file_list_vp9[$nums_vp9]=$file
        nums_vp9=$((nums_vp9+1))
            ;;
        *)
            ;;
        esac

        if [[ $yuvformats =~ $codec ]]; then
            file_list_yuv[$nums_yuv]=$file
            nums_yuv=$((nums_yuv+1))
        fi
    done

    total_files=$[ nums_h264 + nums_hevc + nums_vp9 + nums_yuv ]
    return $total_files
}

function get_raw_codec(){
    file=$1
    codec=$(echo $file | grep -E -o $yuvformats)
    echo $codec
}

function print_files_list(){
   echo -e "\nh264 files: $nums_h264"
    for file in ${file_list_h264[@]}
    do
      echo "${file%_*} -> [${file##*_}]"
    done

    echo -e "\nhevc files: $nums_hevc"
    for file in ${file_list_hevc[@]}
    do
      echo "${file%_*} -> [${file##*_}]"
    done

    echo -e "\nvp9 files: $nums_vp9"
    for file in ${file_list_vp9[@]}
    do
      echo "${file%_*} -> [${file##*_}]"
    done

    echo -e "\nyuv files: $nums_yuv"
    for file in ${file_list_yuv[@]}
    do
      echo "${file%_*} -> [${file##*_}]"
    done
    echo -e "\n"
}

# function to get random name
function get_random_name(){
    if [ "$1" == "decoder" ]; then
        item=("h264_vpe" "hevc_vpe" "vp9_vpe")
    elif [ "$1" == "encoder" ]; then
        item=("h264enc_vpe" "hevcenc_vpe" "vp9enc_vpe")
    elif [ "$1" == "preset" ]; then
        item=("superfast" "fast" "medium" "slow")
    else
        echo $1
        return
    fi

    len=${#item[*]}
    len=$((len-1))
    id=$(rand 0 ${len})
    echo ${item[$id]}
}

#get srm workload
#input: filename
#output: 480p/720p/1080p/2160p
function get_srm_load_from_filename(){
  file=$1
  name=${file%.*}
  suffix=${file##*.}
  sw=0
  sh=0
  tmp=".tmp"
  fps=30
  res=1080p
  fps=30

  ## get resolution
  if [[ $esformats =~ $suffix ]]; then
    ffprobe $file &> $tmp
    res=$(grep -E -o "[0-9]{3,4}+x[0-9]{3,4}+" $tmp)
    fps=$(grep -E -o "[0-9.]{2,5} fps" $tmp)
    fps=$(echo $fps | cut -b 1-2)
  else
    res=$(echo $name | grep -E -o "[0-9]{3,4}+-?x?h?H?X?-?_?[0-9]{3,4}+")
  fi
  arr=($(echo ${res} | sed -r "s/([[:digit:]]+)[^[:digit:]]*([[:digit:]]+)/\1 \2/g"))
  w=${arr[0]}
  h=${arr[1]}

  srmres=$((h*$fps))
  if [[ $srmres -lt 480*30 ]]; then
    res="480p"
  elif [[ $srmres -gt 480*30 ]] && [[ $srmres -lt 721*30 ]]; then
    res="720p"
  elif [[ $srmres -gt 720*30 ]] && [[ $srmres -lt 1081*30 ]]; then
    res="1080p"
  elif [[ $srmres -gt 1081*30 ]] ; then
    res="2160p"
  else
    res="1080p"
  fi
  echo "$res"
}

function align2()
{
	result=`expr $1 / 2`
	result=`expr $result \* 2`
	echo $result
}

# get lower resolution string
function get_lowres(){
    org=$1
    sw=${org%x*}
    sh=${org##*x}
    w=$((10#${sw}))
    h=$((10#${sh}))

    if [[ $w -lt $lowres_minw ]] || [[ $h -lt $lowres_minh ]] ; then
        return
    fi

    w1=$((w/3))
    h1=$((h/3))
    w2=$((w/4))
    h2=$((h/4))
    w3=$((w/5))
    h3=$((h/5))

    w1=$(align2 $w1)
    h1=$(align2 $h1)
    w2=$(align2 $w2)
    h2=$(align2 $h2)
    w3=$(align2 $w3)
    h3=$(align2 $h3)

    echo "($w1"x"$h1)($w2"x"$h2)($w3"x"$h3)"
}
get lower resolution string

#1. transcoder command:
function gen_transcoder_cmd(){

  gen_parameters "decoder" "encoder"

  if [ "$input_file" == "" ]; then
    return;
  fi

  res=${input_file##*_}
  file=${input_file%_*}
  low_res=`get_lowres $res`

  cmd="-c:v ${input_codec} ${decoder_params} ${transcoder_option} -low_res \"4:${low_res}\" -i ${file} "\
"-filter_complex 'spliter_vpe=outputs=4[out0][out1][out2][out3]' "\
"-map '[out0]' -c:v ${output_codec} -preset ${preset} ${encoder_params} -b:v ${bitrate1} ${output_file1} "\
"-map '[out1]' -c:v ${output_codec} -preset ${preset} ${encoder_params} -b:v ${bitrate2} ${output_file2} "\
"-map '[out2]' -c:v ${output_codec} -preset ${preset} ${encoder_params} -b:v ${bitrate3} ${output_file3} "\
"-map '[out3]' -c:v ${output_codec} -preset ${preset} ${encoder_params} -b:v ${bitrate4} ${output_file4} "

  echo $cmd
}

#2. encoder - through pp command:
function gen_encoder_pp_cmd(){

  gen_parameters "raw" "encoder"

  if [ "$input_file" == "" ]; then
    return;
  fi

  res=${input_file##*_}
  file=${input_file%_*}
  low_res=`get_lowres $res`
  input_codec=`get_raw_codec $file`

  #PP don't support UYVY422
  if [ "$input_codec" == "uyvy422" ]; then
    return;
  fi

  cmd="-s $res -pix_fmt ${input_codec} -i ${file} "\
"-filter_complex 'vpe_pp=outputs=4:low_res=$low_res,"\
"spliter_vpe=outputs=4[out0][out1][out2][out3]' "\
"-map '[out0]' -c:v ${output_codec} -preset ${preset} ${encoder_params} -b:v ${bitrate1} ${output_file1} "\
"-map '[out1]' -c:v ${output_codec} -preset ${preset} ${encoder_params} -b:v ${bitrate2} ${output_file2} "\
"-map '[out2]' -c:v ${output_codec} -preset ${preset} ${encoder_params} -b:v ${bitrate3} ${output_file3} "\
"-map '[out3]' -c:v ${output_codec} -preset ${preset} ${encoder_params} -b:v ${bitrate4} ${output_file4} "

  echo $cmd
}

#3. encoder - through hwuploader command:
function gen_encoder_hwuploader_cmd(){

  gen_parameters "raw" "encoder"

  if [ "$input_file" == "" ]; then
    return;
  fi

  res=${input_file##*_}
  file=${input_file%_*}
  low_res=`get_lowres $res`
  input_codec=`get_raw_codec $file`

  cmd="-s $res -pix_fmt ${input_codec} -i ${file} "\
"-filter_complex 'hwupload' "\
"-c:v ${output_codec} -preset ${preset} ${encoder_params} -b:v ${bitrate1} ${output_file1} "

  echo $cmd
}

#4. decoder command:
function gen_decoder_cmd(){

  gen_parameters "decoder" "nv12"

  if [ "$input_file" == "" ]; then
    return;
  fi

  res=${input_file##*_}
  file=${input_file%_*}
  low_res=`get_lowres $res`

  cmd="-c:v ${input_codec} ${decoder_params} -low_res \"4:$low_res\" -i ${file} "\
"-filter_complex 'spliter_vpe=outputs=4[1][2][3][4],"\
"[1]hwdownload,format=${output_codec}[a],"\
"[2]hwdownload,format=${output_codec}[b],"\
"[3]hwdownload,format=${output_codec}[c],"\
"[4]hwdownload,format=${output_codec}[d]' "\
"-map '[a]' ${output_file1} "\
"-map '[b]' ${output_file2} "\
"-map '[c]' ${output_file3} "\
"-map '[d]' ${output_file4} "

  echo $cmd
}

function gen_device_cmd(){
  device=$1
  cmd_common="ffmpeg -y -init_hw_device vpe=dev0:/dev/${device},priority=vod,vpeloglevel=${vpelogvel} "
  echo $cmd_common
}

function rand(){
    min=$1
    max=$(($2-$min+1))
    num=$(date +%s%N)
    echo $(($num % $max + $min))
}

function main(){
  while(true)
  do
      joblist=($(jobs -p))
      while (( ${#joblist[*]} > $task_num ))
      do
          echo "task is full, wait..."
          joblist=($(jobs -p))
          sleep 1
      done

      ##1. generate codec command list
      cmd=`gen_decoder_cmd`
      nums_cmds=0
      if [ "${cmd}" != "" ]; then
          cmds[$nums_cmds]=$cmd
          nums_cmds=$((nums_cmds+1))
      fi

      cmd=`gen_transcoder_cmd`
      if [ "${cmd}" != "" ]; then
          cmds[$nums_cmds]=$cmd
          nums_cmds=$((nums_cmds+1))
      fi

      cmd=`gen_encoder_pp_cmd`
      if [ "${cmd}" != "" ]; then
          cmds[$nums_cmds]=$cmd
          nums_cmds=$((nums_cmds+1))
      fi

      cmd=`gen_encoder_hwuploader_cmd`
      if [ "${cmd}" != "" ]; then
          cmds[$nums_cmds]=$cmd
          nums_cmds=$((nums_cmds+1))
      fi

      ##2. get one from command list
      id=0
      if (( $nums_cmds == 0 )); then
          echo "no commands can be generated, please check the configurations, maybe no right stream files?"
          continue
      elif (( $nums_cmds > 1 )); then
          nums_cmds=$((nums_cmds-1))
          id=$(rand 0 ${nums_cmds})
      fi
      cmd=${cmds[$id]}

      ##3. calculate srm required resource
      file=$(echo $cmd | grep -E -o "\-i\s+\S+\s+")
      file=$(echo $file | cut -b 4-)
      srm_onepath_loading=`get_srm_load_from_filename $file`

      ##4. allocate hw resource
      device=`./srmtool allocate ${srm_onepath_loading} 1 $srm_power_mode`
      pre="$(echo $device | cut -b 1-10)"
      if [ "$pre" == "" ]; then
          echo -ne "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t [Running tasks:$[${#joblist[*]}+1], no resource, waitting...]\r"
          continue
      elif [ "$pre" != "transcoder" ]; then
          echo "srm allocation error"
          return
      fi
      echo "SRM $srm_onepath_loading -> $device"

      ##5. generate command header
      cmd_header=`gen_device_cmd $device`

      ##6. generate final command
      cmd="$cmd_header ""${cmd}"
      script_name=".vpetest${totalcase}.sh"
      echo "#!/bin/bash" > $script_name
      echo "trap \"exit 1\" 1 2 3 15" >> $script_name
      echo "id=\$$" >> $script_name
      echo "time=$(date "+%Y%m%d%H%M%S")" >> $script_name
      echo "fflog=\"fflog${totalcase}_\${time}_pid\${id}.log\"" >> $script_name
      echo "vpelog=\"vpe${totalcase}_\${time}_pid\${id}.log\"" >> $script_name
      cmd="FFREPORT=\"level=${ffmpeglogvel}:file=\${fflog}\" VPEREPORT=\"level=${vpelogvel}:file=\${vpelog}\" ${cmd}"
      echo $cmd >> $script_name
      echo "ret=\$?" >> $script_name
      echo -e "\nif (( \$ret != 0 && \$ret != 255)); then" >> $script_name
      echo "     mv \$fflog e\${ret}_\$fflog" >> $script_name
      echo "     echo \"task [\$id] -> error(\$ret)\"" >> $script_name
      echo "     ./stop.sh" >> $script_name
      echo "else" >> $script_name
      echo "     echo \"task [\$id] -> finished\"" >> $script_name
      echo "     if [ -f \$\"fflog\" ]; then rm \$fflog; fi" >> $script_name
      echo "     if [ -f \$\"vpelog\" ]; then rm \$vpelog; fi" >> $script_name
      echo "fi" >> $script_name
      chmod 777 $script_name
      echo "command=$cmd"

      ##5. execute command
      ./$script_name &
      echo "New task -> $!"
      echo -ne "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t [Running tasks: $[${#joblist[*]}]]\r"
      sumuary "${cmd}"
  done
}

function sumuary(){
  if [ "$1" == "" ] || [ $((totalcase%20)) -eq 1 ]; then
      end=$(date +%s)
      s=$((end - $start ))
      h=$((s/3600))
      s=$((s-$h*3600))
      m=$((s/60))
      printf "\n%-20s %d hours %d minutes" "Total time:" $h $m
      printf "\n%-20s %d times" "Total cases:" $totalcase
      printf "\n%-20s" "Decoders:"
      printf "\n%-20s %d times" "  h264_vpe:" $h264tc
      printf "\n%-20s %d times" "  hevc_vpe:" $hevctc
      printf "\n%-20s %d times" "  vp9_vpe:" $vp9tc
      printf "\n%-20s" "Encoders:"
      printf "\n%-20s %d times" "  h264vpe_enc:" $h264enctc
      printf "\n%-20s %d times" "  hevcvpe_enc:" $hevcenctc
      printf "\n%-20s %d times" "  vp9enc_vpe:" $vp9enctc
      printf "\n%-20s" "Filters:"
      printf "\n%-20s %d times" "  vpe_pp:" $vpepptc
      printf "\n%-20s %d times" "  vpe_spliter:" $vpesplitertc
      printf "\n%-20s" "Encoder modes:"
      printf "\n%-20s %d times" "   superfast:" $superfasttc
      printf "\n%-20s %d times" "   fast:" $fasttc
      printf "\n%-20s %d times" "   medium:" $mediumtc
      printf "\n%-20s %d times" "   slow:" $slowtc
      printf "\n"
      if [ "$1" == "" ]; then
          ./stop.sh
          exit 0
      fi
    else
      h264tc=$((h264tc + $(echo $1 | grep -c "h264_vpe") ))
      hevctc=$((hevctc + $(echo $1 | grep -c "hevc_vpe") ))
      vp9tc=$((vp9tc + $(echo $1 | grep -c "vp9_vpe") ))
      h264enctc=$((h264enctc + $(echo $1 | grep -c "h264enc_vpe") ))
      hevcenctc=$((hevcenctc + $(echo $1 | grep -c "hevcenc_vpe") ))
      vp9enctc=$((vp9enctc + $(echo $1 | grep -c "vp9enc_vpe") ))
      vpepptc=$((vpepptc + $(echo $1 | grep -c "vpe_pp") ))
      vpesplitertc=$((vpesplitertc + $(echo $1 | grep -c "spliter_vpe") ))
      superfasttc=$((superfasttc + $(echo $1 | grep -c "superfast") ))
      fasttc=$((fasttc + $(echo $1 | grep -c "fast") ))
      mediumtc=$((mediumtc + $(echo $1 | grep -c "medium") ))
      slowtc=$((slowtc + $(echo $1 | grep -c "slow") ))
    fi
    totalcase=$((totalcase+1))
}

##start script
echo "enable core dump"
ulimit -c unlimited

start=$(date +%s)
./srmtool &
echo "searching video files from $in_stream_path"
gen_file_list $in_stream_path
total=$?
if [[ $total -gt 0 ]]; then
    echo "Total $total files was added!"
    print_files_list
    main
else
    echo "No files was found, exit"
    exit
fi
