#/bin/bash
DLL_PATH=/usr/lib/vpe/
INC_PATH=/usr/local/include/vpe/
PKG_PATH=/usr/share/pkgconfig/
DRV_PATH=/lib/modules/`uname -r`/kernel/drivers/pci/pcie/solios-x/
FRM_PATH=/lib/firmware/
CFG_PATH=/etc/ld.so.conf.d/

if [ ! -d $DLL_PATH ]; then
    mkdir $DLL_PATH -p
fi;

if [ ! -d ${INC_PATH} ]; then mkdir ${INC_PATH} -p; fi;
if [ ! -d ${PKG_PATH} ]; then mkdir ${PKG_PATH} -p; fi;
if [ ! -d ${DRV_PATH} ]; then mkdir ${DRV_PATH} -p; fi;
if [ ! -d ${FRM_PATH} ]; then mkdir ${FRM_PATH} -p; fi;
if [ ! -d ${CFG_PATH} ]; then mkdir ${CFG_PATH} -p; fi;

tar -xf drivers.tgz
make -C drivers

cp *.so ${DLL_PATH}
cp vpe/*.h ${INC_PATH}
cp libvpi.pc ${PKG_PATH}
cp drivers/transcoder_pcie.ko ${DRV_PATH}
cp ZSP_FW_RP_V*.bin ${FRM_PATH}/transcoder_zsp_fw.bin
echo "/usr/lib/vpe" > ${CFG_PATH}/vpe-${arch}.conf
/sbin/ldconfig
depmod
## VPE libs, fw, .h were installed to path:${installpath}

echo "Installing driver..."
rmmod transcoder_pcie
insmod drivers/transcoder_pcie.ko
if [ "`lsmod | grep transcoder_pcie`" != "" ]; then
    echo "driver was installed successfully!"
else
    echo "driver was installed failed!"
fi