#!/bin/bash

if [ ! -f "./config.mk" ]; then
	echo "generate config.mk"
	./configure
fi

echo "lunch config.mk"
. ./config.mk

cross=n
arch=$ARCH

if [ -z "$ARCH" ]; then
arch=`uname -m`
fi

if [[ "$arch" != "`uname -m`" ]]; then
echo "Target arch is $arch, host arch is $host_arch, so enable cross compiling"
cross=y
fi

ifeq ($(ARCH),aarch64)
arch=arm64
endif

cmd="./configure --pkg-config=true --enable-vpe "

if [ "$1" == "clean" ]; then
        make clean
        cd ../ffmpeg
        make clean
        exit 0
fi

make DEBUG=${DEBUG} ARCH=$arch
if [ $? != 0 ]; then echo "build VPE error";exit 1; fi

if [ "${cross}" == "y" ]; then
	export PKG_CONFIG_PATH=`pwd`:$PKG_CONFIG_PATH
	echo $PKG_CONFIG_PATH
        echo "Name: libvpi" > "libvpi.pc"
	echo "Description: VSI platform interface lib" >> "libvpi.pc"
	echo "Version: 1.0" >> "libvpi.pc"
	echo "Cflags: -I"`pwd`"/vpi_inc/" >> "libvpi.pc"
	echo "Libs: -L"`pwd`"/sdk_libs/"${arch}"/" -L"`pwd`""/vpi -lvpi" >> "libvpi.pc"
	mkdir vpi_inc/vpe -p
	cp vpi/inc/*.h vpi_inc/vpe/
else
        sudo make install ARCH=$arch
fi

cd ../ffmpeg

if [ "${DEBUG}" == "y" ]; then
	cmd=$cmd"--disable-optimizations --disable-asm --disable-stripping "
fi

if [ "${cross}" == "n" ]; then
	cmd=$cmd"--extra-ldflags="-L/usr/lib/vpe" --extra-libs="-lvpi" "
else
	cmd=$cmd\
"--disable-vaapi "\
"--disable-shared "\
"--enable-static "\
"--cross-prefix=$CROSS_COMPILE "\
"--enable-cross-compile "\
"--arch=${arch} "\
"--extra-cflags=-I../vpe/vpi_inc "\
"--extra-ldflags=\"-L./../vpe/sdk_libs/${arch} -L./../vpe/vpi/\" "\
"--extra-libs=\"-lvpi -lhugetlbfs -lcwl -ldwlg2 -lenc -lg2common -lg2h264 -lg2hevc -lg2vp9 -lh2enc -lhal -lpp -lsyslog\" "\
"--target-os=linux "\
"--sysroot=$SDKTARGETSYSROOT "
fi
	echo "command is $cmd"
	echo $cmd > config.sh
	chmod 777 config.sh
	./config.sh
	if [ $? != 0 ]; then echo "FFmpeg configureation error";exit 1; fi

	rm config.sh
	make -j8
	if [ $? != 0 ]; then echo "build FFmpeg error";exit 1; fi

if [ "${cross}" == "y" ]; then
	rm ../vpe/vpi_inc -rf
	rm ../vpe/libvpi.pc
fi
