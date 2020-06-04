#!/bin/sh

. ./drv_env_set.sh

cd ${COMMON_BASE}
	make clean
cd -

cd VC8000D
	make clean
cd -

cd VC8000E
	make -C software/linux_reference clean
cd -

cd Bigsea/software
	make clean
cd -
	
cd ffmpeg
	make clean
cd -



