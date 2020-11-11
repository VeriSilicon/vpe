#!/bin/sh

clean_option=n
debug=n
mini_ffmpeg=n

if [ "$1" == "mini_ffmpeg" ] || [ "$2" == "mini_ffmpeg" ] || [ "$3" == "mini_ffmpeg" ]
	then
	mini_ffmpeg=y
fi

if [ "$1" == "debug" ] || [ "$2" == "debug" ] || [ "$3" == "debug" ]
	then
	debug=y
fi

if [ "$1" == "clean" ]
	then
	clean_option=y
fi

echo "Compiling opition: debug=$debug, mini_ffmpeg=$mini_ffmpeg, clean_option=$clean_option"

if [ "${clean_option}" == "y" ]
then
	cd vpe
	make clean
	cd ../ffmpeg
	make clean
	exit 0
fi

cd vpe
	make DEBUG=${debug}
	sudo make install
cd -

cd ffmpeg
	if [ "${debug}" == "y" ] && [ "${mini_ffmpeg}" == "n" ]
	then
		./configure --enable-vpe --extra-ldflags="-L/lib/vpe" --extra-libs="-lvpi" \
		--disable-optimizations \
		--disable-asm  \
		--disable-stripping \
		--disable-vaapi
	elif [ "${dbg}" == "y" ] && [ "${mini_ffmpeg}" == "y" ]
	then
		./configure --disable-sdl2 --enable-vpe --extra-ldflags="-L/lib/vpe" --extra-libs="-lvpi" \
		--disable-optimizations \
		--disable-asm  \
		--disable-stripping \
		--disable-sdl2 \
		--disable-libxcb \
		--disable-libxcb-shm \
		--disable-libxcb-xfixes \
		--disable-libxcb-shape \
		--disable-xlib \
		--disable-libmfx \
		--disable-vaapi
	elif [ "${debug}" == "n" ] && [ "${mini_ffmpeg}" == "y" ]
	then
		./configure --enable-vpe  --extra-ldflags="-L/lib/vpe" --extra-libs="-lvpi" \
		--disable-sdl2 \
		--disable-libxcb \
		--disable-libxcb-shm \
		--disable-libxcb-xfixes \
		--disable-libxcb-shape \
		--disable-xlib \
		--disable-libmfx \
		--disable-vaapi
	else
		./configure --enable-vpe  --extra-ldflags="-L/lib/vpe" --extra-libs="-lvpi" \
		--disable-vaapi

	fi
	make -j8
cd -

