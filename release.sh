#!/bin/sh

. ./drv_env_set.sh

if [ "${dbg}" == "y" ]
then
	echo "Current is debug build, not able to release, please swicth to release version"
	exit
fi

cp Bigsea/software/*.so vpi/sdk_libs/
cp VC8000D/out/x86_linux_pci/release/*.so vpi/sdk_libs/
cp VC8000E/software/linux_reference/libh2enc.so vpi/sdk_libs/
cp common/lib/*.so vpi/sdk_libs/

cp Bigsea/software/inc/*.h vpi/sdk_inc/Bigsea/software/inc/
cp common/inc/*.h vpi/sdk_inc/common/inc/
cp VC8000E/software/source/vp9/*.h vpi/sdk_inc/VC8000E/software/source/vp9/
cp VC8000E/software/source/jpeg/*.h vpi/sdk_inc/VC8000E/software/source/jpeg/
cp VC8000E/software/source/hevc/*.h vpi/sdk_inc/VC8000E/software/source/hevc/
cp VC8000E/software/source/common/*.h vpi/sdk_inc/VC8000E/software/source/common/
cp VC8000E/software/linux_reference/ewl/*.h vpi/sdk_inc/VC8000E/software/linux_reference/ewl/
cp VC8000E/software/inc/*.h vpi/sdk_inc/VC8000E/software/inc/

cp VC8000D/software/linux/h264high/*.h vpi/sdk_inc/VC8000D/software/linux/h264high/
cp VC8000D/software/source/common/*.h vpi/sdk_inc/VC8000D/software/source/common/
cp VC8000D/software/source/config/*.h vpi/sdk_inc/VC8000D/software/source/config/
cp VC8000D/software/source/h264high/*.h vpi/sdk_inc/VC8000D/software/source/h264high/
cp VC8000D/software/source/h264high/legacy/*.h vpi/sdk_inc/VC8000D/software/source/h264high/legacy/
cp VC8000D/software/source/inc/*.h vpi/sdk_inc/VC8000D/software/source/inc/
cp VC8000D/software/source/pp/*.h vpi/sdk_inc/VC8000D/software/source/pp/

rm vpi/drivers/ -rf
cd drivers
find . -name *.o | xargs rm
cd -
cp drivers/ vpi/ -rf

