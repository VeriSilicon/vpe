#!/bin/sh

. ./drv_env_set.sh

export dbg=n
clean_option=n

for option in $@
	do
		if [ "${option/clean/}" != "${option}" ]
		then
			clean_option=${option/clean=/}
		fi
done

echo clean_option is $clean_option

cd drivers
	./build_drivers.sh
cd -

cd common
	if [ "${clean_option}" == "y" ]
	then
		make clean
	fi
	make all HUGEPAGE=y CHECK_MEM_LEAK=y DEBUG=${dbg}
cd -

cd VC8000D
	if [ "${clean_option}" == "y" ]
	then
		make clean
	fi
	if [ "${dbg}" == "y" ]
		make hevcdeclibs_so ENV=x86_linux_pci USE_DEC_IRQ=y USE_TB_PP=n  USE_64BIT_ENV=n RELEASE=n SUPPORT_CACHE=y SUPPORT_DEC400=y SUPPORT_TCACHE=y SUPPORT_NEW_MEM_ALLOC=y USE_MULTI_CORE=y HUGEPAGE=y CHECK_MEM_LEAK=y USE_RANDOM_TEST=n
		make h264declibs_so ENV=x86_linux_pci USE_DEC_IRQ=y USE_TB_PP=n  USE_64BIT_ENV=n RELEASE=n SUPPORT_CACHE=y SUPPORT_DEC400=y SUPPORT_TCACHE=y SUPPORT_NEW_MEM_ALLOC=y USE_MULTI_CORE=y HUGEPAGE=y CHECK_MEM_LEAK=y USE_RANDOM_TEST=n
		make vp9declibs_so ENV=x86_linux_pci USE_DEC_IRQ=y USE_TB_PP=n  USE_64BIT_ENV=n RELEASE=n SUPPORT_CACHE=y SUPPORT_DEC400=y SUPPORT_TCACHE=y SUPPORT_NEW_MEM_ALLOC=y USE_MULTI_CORE=y HUGEPAGE=y CHECK_MEM_LEAK=y USE_RANDOM_TEST=n
		make ppdeclibs_so ENV=x86_linux_pci USE_DEC_IRQ=y USE_TB_PP=n  USE_64BIT_ENV=n RELEASE=n SUPPORT_CACHE=y SUPPORT_DEC400=y SUPPORT_TCACHE=y SUPPORT_NEW_MEM_ALLOC=y USE_MULTI_CORE=y HUGEPAGE=y CHECK_MEM_LEAK=y USE_RANDOM_TEST=n
	then
		make hevcdeclibs_so ENV=x86_linux_pci USE_DEC_IRQ=y USE_TB_PP=n  USE_64BIT_ENV=n RELEASE=y SUPPORT_CACHE=y SUPPORT_DEC400=y SUPPORT_TCACHE=y SUPPORT_NEW_MEM_ALLOC=y USE_MULTI_CORE=y HUGEPAGE=y CHECK_MEM_LEAK=y USE_RANDOM_TEST=n
		make h264declibs_so ENV=x86_linux_pci USE_DEC_IRQ=y USE_TB_PP=n  USE_64BIT_ENV=n RELEASE=y SUPPORT_CACHE=y SUPPORT_DEC400=y SUPPORT_TCACHE=y SUPPORT_NEW_MEM_ALLOC=y USE_MULTI_CORE=y HUGEPAGE=y CHECK_MEM_LEAK=y USE_RANDOM_TEST=n
		make vp9declibs_so ENV=x86_linux_pci USE_DEC_IRQ=y USE_TB_PP=n  USE_64BIT_ENV=n RELEASE=y SUPPORT_CACHE=y SUPPORT_DEC400=y SUPPORT_TCACHE=y SUPPORT_NEW_MEM_ALLOC=y USE_MULTI_CORE=y HUGEPAGE=y CHECK_MEM_LEAK=y USE_RANDOM_TEST=n
		make ppdeclibs_so ENV=x86_linux_pci USE_DEC_IRQ=y USE_TB_PP=n  USE_64BIT_ENV=n RELEASE=y SUPPORT_CACHE=y SUPPORT_DEC400=y SUPPORT_TCACHE=y SUPPORT_NEW_MEM_ALLOC=y USE_MULTI_CORE=y HUGEPAGE=y CHECK_MEM_LEAK=y USE_RANDOM_TEST=n
fi
cd -

cd VC8000E
	if [ "${clean_option}" == "y" ]
	then
		make -C software/linux_reference clean
	fi
	make -C software/linux_reference pci_shared pci ENCMODE="hevcenclibs" SUPPORT_L2CACHE=y SUPPORT_DEC400=y SUPPORT_TCACHE=y DRV_NEW_ARCH=y HUGEPAGE=y PERFORMANCE=y CHECK_MEM_LEAK=y DEBUG=${dbg}
cd -

cd Bigsea/software
	if [ "${clean_option}" == "y" ]
	then
		make clean
	fi
	make fpga SUPPORT_CACHE=y SUPPORT_DEC400=y SUPPORT_TCACHE=y DRV_NEW_ARCH=y HUGEPAGE=y CHECK_MEM_LEAK=y DEBUG=${dbg}
cd -
