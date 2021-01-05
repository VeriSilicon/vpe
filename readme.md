# Introduction

VPE main functions:

    * Transcoding
          The input is HEVC,H264,VP9 bitstream up to 4K@60 FPS. The output is
          HEVC,H264,VP9 up to four channels with downscaling.
    * Encoding
          The input is raw frame.
          The output supports below:
              H264: Main Profile, levels 1 - 5.2
                    High Profile, levels 1 - 5.2
                    High 10 Profile, levels 1 - 5.2
              HEVC: Main Profile, Level 5.1, High Tier
                    Main10 profile, Level 5.1, High Tier
                    Main Still Profile
              VP9:  Profile 0 (8-bit)
                    Profile 2 (10-bit)
    * Decoding
          The input supports below:
              H264: Baseline Profile, Levels 1 – 5.1
                    Main Profile, Levels 1 - 5.1
                    High Profile, Levels 1 – 5.1
              HEVC: Main Profile, up to Level 5.2, High Tier
                    Main10 profile, up to Level 5.2, High Tier
                    Main Still Profile
              VP9:  Profile 0 (8-bit)
                    Profile 2 (10-bit)
          The output is YUV NV12.
    * Video Processing
          Download/Downscale/Format conversion


# VPE Plugin Description

VPE Plugin is for the multimedia frameworks to enable video transcoding, encoding,
decoding and processing.
Here VPE Plugin for FFmpeg is supported and to be extended to GStreamer and others.

# VPE Overall Architecture
     +----------------------------------------+
     |              |              |          |
     |  FFmpeg      |  GStreamer   |   APP    |
     |              |              |          |
     |  +-----------|  +-----------|          |
     |  | VPE Plugin|  | VPE Plugin|          |
     +----------------------------------------+
     +----------------------------------------+
     |  VPE                                   |
     |   +--------------------------------+   |
     |   |                                |   |
     |   |             VPI                |   |
     |   |                                |   |
     |   +--------------------------------+   |
     |   +--------------------------------+   |
     |   |                                |   |
     |   |            VPE Codecs          |   |
     |   |      (Dynamic Link Libraries)  |   |
     |   |                                |   |
     |   +--------------------------------+   |
     |   +--------------------------------+   |
     |   |                                |   |
     |   |         OS Wrapper Layer       |   |
     |   |                                |   |
     |   +--------------------------------+   |
     +----------------------------------------+
     +----------------------------------------+
     |                                        |
     |              Linux  kernel             |
     |                                        |
     +----------------------------------------+

# VPE Folder Struct Description
```
├── build                         Configure files for building
├── drivers                       The linux driver
├── firmware                      The firmware for VeriSilicon Platform
├── sdk_inc                       VeriSilicon Platform Codec SDK header files
├── sdk_libs                      VeriSilicon Platform Codec SDK libraries
└── vpi                           VeriSilicon Platform Interfaces source code

```

# Building and Installation

## 1. Config the toolchain
If you are doing the cross compiling, then you need to run configure to generate the config file first:

Example for non-cross compiling:

```bash
$./configure

arch=x86_64
cross=
sysroot=
kernel=
debug=n
outpath=
Create VPE build config file successfully!
```
Example for cross compiling:

```bash
./configure --arch=arm64 --cross=aarch64-linux-gnu- --sysroot=toolchain/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/aarch64-linux-gnu/libc --kernel=/work/imx8mmevk-poky-linux/linux-imx/4.19.35-r0/build

arch=arm64
cross=aarch64-linux-gnu-
sysroot=/toolchain/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/aarch64-linux-gnu/libc
kernel=/work/imx8mmevk-poky-linux/linux-imx/4.19.35-r0/build
debug=n
outpath=
Create VPE build config file successfully!
```

## 2. Build
```bash
$make
VPE build step - build VPI
make -C vpi CHECK_MEM_LEAK=y DEBUG=
make[1]: Entering directory `/home/gyzhang/work/spsd/vpe/vpi'
...
 Building modules, stage 2.
  MODPOST 1 modules
  CC      /home/gyzhang/work/spsd/vpe/drivers/transcoder-pcie/transcoder_pcie.mod.o
  LD [M]  /home/gyzhang/work/spsd/vpe/drivers/transcoder-pcie/transcoder_pcie.ko
make[2]: Leaving directory `/home/gyzhang/work/transcoder/linux-4.19.106'
make[1]: Leaving directory `/home/gyzhang/work/spsd/vpe/drivers/transcoder-pcie'
Build release VPE
```

## 3. Install
```bash
$sudo make install
VPE build step - install
cp /home/gyzhang/work/spsd/vpe/firmware/ZSP_FW_RP_V*.bin /lib/firmware/transcoder_zsp_fw.bin
cp /home/gyzhang/work/spsd/vpe/sdk_libs/*.so "/usr/lib/vpe"
cp /home/gyzhang/work/spsd/vpe/vpi/libvpi.so "/usr/lib/vpe"
echo "/usr/lib/vpe" >  /etc/ld.so.conf.d/vpe-x86_64.conf
/sbin/ldconfig
cp /home/gyzhang/work/spsd/vpe/vpi/inc/*.h "/usr/local/include/vpe"
insmod drivers/transcoder-pcie/transcoder_pcie.ko
rm "/lib/modules/4.19.106/kernel/drivers/pci/pcie/solios-x" -rf
mkdir -p "/lib/modules/4.19.106/kernel/drivers/pci/pcie/solios-x"
cp drivers/transcoder-pcie/transcoder_pcie.ko "/lib/modules/4.19.106/kernel/drivers/pci/pcie/solios-x"
depmod
VPE installation finished!
```

## 4. Uninstall
```bash
$sudo make uninstall
VPE build step - uninstall
/sbin/ldconfig
depmod
VPE uninstallation finished!
```
