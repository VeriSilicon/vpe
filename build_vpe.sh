#!/bin/bash

vpe_out_path=`pwd`/vpe_package

if [ ! -f "./config.mk" ]; then
	echo "generate config.mk"
	./configure
fi

echo "lunch config.mk"
. ./config.mk

if [ "$1" == "clean" ]; then
	make clean
	cd ../ffmpeg
	make clean
	exit 0
fi

## 1. build VPE
make
if [ $? != 0 ]; then echo "build VPE error";exit 1; fi

## 2.install VPE
if [ "${cross}" == "n" ]; then
	echo "installing VPE to host"
	sudo make install
elif [[ "${cross}" == "y" && ! -z "$installpath" ]]; then
	echo "installing VPE to sysroot $installpath"
	sudo make install
else
	echo "Cross compiling, and 'installpath' is not specified, skip VPE installation"
fi

## 3. generate FFmpeg configration script
cd ../ffmpeg
cmd="./configure --pkg-config=true --enable-vpe "
if [ "${DEBUG}" == "y" ]; then
	cmd=$cmd"--disable-optimizations --disable-asm --disable-stripping "
fi

if [ "${cross}" == "n" ]; then
	##for libhugetlbfs.so on arm64 platform
	export LD_LIBRARY_PATH=/usr/lib64
	cmd=$cmd"--extra-ldflags="-L/usr/lib/vpe" --extra-libs="-lvpi" "
else
	cmd="$cmd"\
"--disable-vaapi "\
"--disable-shared "\
"--enable-static "\
"--cross-prefix=$CROSS_COMPILE "\
"--enable-cross-compile "\
"--arch=${ARCH} "\
"--extra-cflags=-I${vpe_out_path} "\
"--extra-ldflags=-L${vpe_out_path} "\
"--extra-libs=\"-lvpi -lhugetlbfs -lcwl -ldwlg2 -lenc -lg2common -lg2h264 -lg2hevc -lg2vp9 -lh2enc -lhal -lpp -lsyslog\" "\
"--target-os=linux "\
"--sysroot=$SDKTARGETSYSROOT "
fi

echo "FFmpeg config command is: "
echo "$cmd"
echo $cmd > config.sh
chmod 777 config.sh

## 4. FFmpeg config
export PKG_CONFIG_PATH="`pwd`/vpe/package":$PKG_CONFIG_PATH
./config.sh
if [ $? != 0 ]; then echo "FFmpeg configureation error";exit 1; fi
rm config.sh

## 5. Build FFmpeg
make -j8
if [ $? != 0 ]; then echo "build FFmpeg error";exit 1; fi

cd ../vpe
cp ../ffmpeg/ffmpeg ${vpe_out_path}/
cp ../ffmpeg/ffplay ${vpe_out_path}/
tar -czf vpe_package.tgz ${vpe_out_path}/

echo "VPE compiling was done! FFmpeg + VPE everyting had been put to $vpe_out_path"
