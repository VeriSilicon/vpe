#export dbg=y
#export ERROR_TEST=y
export HUGEPAGE=y
export PERFORMANCE=y


#export VP9_ERR_TEST=y
#export VP9_EDMA_ERR_TEST=y
#export PP_ERROR_TEST=y
#export VCE_ERROR_TEST=y

#export EMULATOR=y


export COMMON_BASE=`pwd`/common
export DRV_BASE=`pwd`/drivers
export HANTRO_PATH=`pwd`/VC8000D
export HANTRO_PATH_E=`pwd`/VC8000E
export BIGSEA_PATH=`pwd`/Bigsea
export VPI_PATH=`pwd`/vpi

# TEST_ENV select 1 in below 5
# for 2 slices
export TEST_ENV=
# for fpga
#export TEST_ENV="FPGA_S0_ONLY"
#export TEST_ENV="FPGA_S0VCE_ONLY"
#export TEST_ENV="FPGA_S0VCD_ONLY"
#export TEST_ENV="FPGA_S0BIGSEA_ONLY"

# DRV_SEL select 1 in below 2
# for adaptive 2 slices
export DRV_SEL=
# for independent 2 slices
#export DRV_SEL="INDEPENDENT_2S" #not support now

