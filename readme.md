# 1. VPE description

VeriSilicon Platform Engine(VPE for short) consists of VeriSilicon Platform Interfaces (VPI for short) and SDK. It is the control software for VeriSilicon SoC platforms and separate IPs. Here VPI provides the APIs for VeriSilicon Video encoding and decoding functions, the details are below:

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


# 2. VPE Plugin description

VPE Plugin is for the multimedia frameworks to enable video transcoding, encoding,
decoding and processing with VeriSilicon Platform Engine.
Here VPE Plugin for FFmpeg is supported and to be extended to GStreamer and others.

# 3. The overall architecture:
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

# 4. VPI directory
```
├── build                         Configure files for building
├── doc                           Documentations
│   ├── enc_params_h26x.md        The enc_params parameter description for h26x encoder
│   └── enc_params_vp9.md         The enc_params parameter description for vp9 encoder
├── drivers                       The linux driver
│   ├── build_drivers.sh          The script to build the driver
│   ├── load_drivers.sh           The script to load the driver
│   ├── README.md                 Readme
│   └── transcoder-pcie           Driver source code
├── firmware                      The firmware for VeriSilicon Platform
│   └── ZSP_FW_RP_Vxxx.bin
├── Makefile                      Makefile
├── readme.md                     Readme
├── sdk_inc                       VeriSilicon Platform Codec SDK header files
├── sdk_libs                      VeriSilicon Platform Codec SDK libraries
└── vpi                           VeriSilicon Platform Interfaces source code
    ├── inc                       VPI header files
    ├── Makefile                  VPI Makefile
    ├── src                       VPI source code
    └── utils                     VPI log code

```

# 5. Build and install VPE

1. Build
```bash
$make
VPE build step - prepare
sudo cp /home/gyzhang/work/facebook/spsd/vpe/sdk_libs/*.so "/usr/lib/vpe"
[sudo] password for gyzhang:
sudo cp /home/gyzhang/work/facebook/spsd/vpe/vpi/inc/*.h "/usr/local/include/vpe"
sudo cp /home/gyzhang/work/facebook/spsd/vpe/build/libvpi.pc "/usr/share/pkgconfig"
VPE build step - build VPI
make -C vpi CHECK_MEM_LEAK=y DEBUG=
make[1]: Entering directory `/home/gyzhang/work/facebook/spsd/vpe/vpi'
...
```
2. Install
```bash
$sudo make install
VPE build step - install
cp /home/gyzhang/work/facebook/spsd/vpe/firmware/ZSP_FW_RP_V*.bin /lib/firmware/transcoder_zsp_fw.bin
cp /home/gyzhang/work/facebook/spsd/vpe/sdk_libs/*.so "/usr/lib/vpe"
cp /home/gyzhang/work/facebook/spsd/vpe/vpi/libvpi.so "/usr/lib/vpe"
echo "/usr/lib/vpe" >  /etc/ld.so.conf.d/vpe-x86_64.conf
/sbin/ldconfig
cp /home/gyzhang/work/facebook/spsd/vpe/vpi/inc/*.h "/usr/local/include/vpe"
insmod drivers/transcoder-pcie/transcoder_pcie.ko
rm "/lib/modules/4.19.106/kernel/drivers/pci/pcie/solios-x" -rf
mkdir -p "/lib/modules/4.19.106/kernel/drivers/pci/pcie/solios-x"
cp drivers/transcoder-pcie/transcoder_pcie.ko "/lib/modules/4.19.106/kernel/drivers/pci/pcie/solios-x"
depmod
```
