#!/bin/sh

clean_option=n
dbg=n

cd vpe
	if [ "${clean_option}" == "y" ]
	then
		make clean
	fi
	make DEBUG=${dbg}
	sudo make install
cd -

cd ffmpeg
	if [ "${dbg}" == "y" ]
	then
		./configure --enable-vpe --extra-ldflags="-L/lib/vpe" --extra-libs="-lvpi" \
			--disable-optimizations \
			--disable-asm  \
			--disable-stripping
	else
		./configure --enable-vpe  --extra-ldflags="-L/lib/vpe" --extra-libs="-lvpi"
	fi
	if [ "${clean_option}" == "y" ]
        then
			make clean
        fi
	make -j 32

