#!/bin/sh

. ./drv_env_set.sh

if [ "${dbg}" == "y" ]
then
	echo "Current is debug build, not able to release, please swicth to release version"
	exit
fi

cp Bigsea/software/*.so vpe/sdk_libs/
cp VC8000D/out/x86_linux_pci/release/*.so vpe/sdk_libs/
cp VC8000E/software/linux_reference/libh2enc.so vpe/sdk_libs/
cp common/lib/*.so vpe/sdk_libs/

cp Bigsea/software/inc/*.h vpe/sdk_inc/Bigsea/software/inc/
cp common/inc/*.h vpe/sdk_inc/common/inc/
cp VC8000E/software/source/vp9/*.h vpe/sdk_inc/VC8000E/software/source/vp9/
cp VC8000E/software/source/jpeg/*.h vpe/sdk_inc/VC8000E/software/source/jpeg/
cp VC8000E/software/source/hevc/*.h vpe/sdk_inc/VC8000E/software/source/hevc/
cp VC8000E/software/source/common/*.h vpe/sdk_inc/VC8000E/software/source/common/
cp VC8000E/software/linux_reference/ewl/*.h vpe/sdk_inc/VC8000E/software/linux_reference/ewl/
cp VC8000E/software/inc/*.h vpe/sdk_inc/VC8000E/software/inc/

cp VC8000D/software/linux/h264high/*.h vpe/sdk_inc/VC8000D/software/linux/h264high/
cp VC8000D/software/source/common/*.h vpe/sdk_inc/VC8000D/software/source/common/
cp VC8000D/software/source/config/*.h vpe/sdk_inc/VC8000D/software/source/config/
cp VC8000D/software/source/h264high/*.h vpe/sdk_inc/VC8000D/software/source/h264high/
cp VC8000D/software/source/h264high/legacy/*.h vpe/sdk_inc/VC8000D/software/source/h264high/legacy/
cp VC8000D/software/source/inc/*.h vpe/sdk_inc/VC8000D/software/source/inc/
cp VC8000D/software/source/pp/*.h vpe/sdk_inc/VC8000D/software/source/pp/

sudo rm vpe/drivers/ -rf
cd drivers
find . -name *.o | xargs rm
cd -
cp drivers/ vpe/ -rf

