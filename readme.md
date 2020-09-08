# Table of Contents

# Table of Contents

* [Introduction](#Introduction)
* [VPE Plugin Description](#VPE-Plugin-Description)
* [VPE Overall Architecture](#VPE-Overall-Architecture)
* [VPE Folder Struct Description](#VPE-Folder-Struct-Description)
* [Building and Installation](#Building-and-Installation)
* [Codec Parameters](#codec)
   * [Device](#Device)
   * [Decoder](#Decoder)
   * [Spliter](#Spliter)
   * [PP](#PP)
   * [HWdownloader](#HWdownloader)
   * [VP9 Encoder](#VP9-Encoder)
   * [H264 H265 Encoder](#H264-H265-Encoder)
   * [Preset Detail](#Preset-Detail)

# Introduction

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


# VPE Plugin Description

VPE Plugin is for the multimedia frameworks to enable video transcoding, encoding,
decoding and processing with VeriSilicon Platform Engine.
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

# Building and Installation

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
# Device

It's followed by FFMpeg parameters -init_hw_device bparameter:

| Option     | Sub Option | Type   | Description                                | Range of Value Valid | Default Value |
|------------|------------|--------|--------------------------------------------|----------------------|---------------|
| priority   |            | string | Priority of codec, live is higher than vod | live/vod             | vod           |
| vpeloglevel |            | int    | set the VPE log level                      | 0\-9                 | 0             |


# Decoder
| Option        | Sub Option | Type   | Description                                                                 | Range of Value Valid | Default Value    |
|---------------|------------|--------|-----------------------------------------------------------------------------|----------------------|------------------|
| \-low\_res    |            | string | Set output number and resize config for each channel                        |                      | null             |
| \-dev         |            | string | Set device name\.                                                           |                      | /dev/transcoder0 |
| \-transcode |            | int | Whether need doing transcoding.| 0,1        |0            |

# Spliter
| Option  | Sub Option | Type | Description             | Range of Value Valid | Default Value |
|---------|------------|------|-------------------------|----------------------|---------------|
| outputs |            | int  | Set number of outputs\. | \[1\.\.4\]           | 1             |

# PP
| Option     | Sub Option | Type   | Description                                          | Range of Value Valid | Default Value |
|------------|------------|--------|------------------------------------------------------|----------------------|---------------|
| outputs    |            | int    | Set number of outputs\.                              | \[1\.\.4\]           | 1             |
| force10bit |            | int    | force output 10bit format                            | \[0\.\.1\]           | 0             |
| low\_res   |            | string | Set output number and resize config for each channel |                      | null          |

# HWdownloader
| Option | Sub Option | Type | Description | Range of Value Valid | Default Value |
|--------|------------|------|-------------|----------------------|---------------|
| /      |            | /    | /           | /                    | /             |

# VP9 Encoder

| Option             | Sub Option      | Type   | Description                                                                                              | Range of Value Valid                                                                                                                                                                 | Default Value              |
|--------------------|-----------------|--------|----------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------|
| \-b:v                                     |                 | int    | Target bitrate for rate control\.     | \[10000\.\.60000000\]                | 1000000        |
| \-r (for input\)                         |                 | int    | Input picture rate numerator\.        | \[1\.\.1048575\]                     | 30             |
|                                           |                 | int    | Input picture rate denominator\.      | \[1\.\.1048575\]                     | 1              |
| \-r (for output\)                        |                 | int    | Output picture rate numerator\.       | \[1\.\.1048575\]                     | inputRateNumer |
|                                           |                 | int    | Output picture rate denominator\.     | \[1\.\.1048575\]                     | inputRateDenom |
| \-enc\-params                             | intraPicRate    | int    | Intra picture rate in frames\.        | \[0\.\.INT\_MAX\]                    | 0              |
|                                           | bitrateWindow   | int    | Bitrate window length in frames\.     | \[1\.\.300\]                         | 150            |
|                                           | qpHdr           | int    | Initial QP used for the first frame\. | \[\-1\.\.255\]                       | \-1            |
|                                           | qpMin           | int    | Minimum frame header QP\.             | \[0\.\.255\]                         | 10             |
|                                           | qpMax           | int    | Maximum frame header QP\.             | \[0\.\.255\]                         | 255            |
|                                           | fixedIntraQp    | int    | Fixed Intra QP, 0 = disabled\.        | \[0\.\.255\]                         | 0              |
|                                           | picRc           | int    | Picture rate control enable\.         | 0=OFF, 1=ON                          | 1              |
|                                           | mcompFilterType | int    | Interpolation filter mode\.           | \[0\.\.4\]                           | 4              |
|                                           | force8bit       | int    | Force to output 8bit stream           | \[0\.\.1\]                           | 0              |
|                                           | refFrameScheme  | int    | Reference frame update scheme\.       | \[0\.\.4\]                           | 4              |
|                                           | filterLevel     | int    | Filter strength level for deblocking  | \[0\.\.64\]                          | 64             |
|                                           | filterSharpness | int    | Filter sharpness for deblocking\.     | \[0\.\.8\], 8=auto                   | 8              |
| \-effort or inside \-enc\-params          | effort          | int    | Encoder effort level\.                | \[0\.\.5\]0=fastest,5=best quality   | 0              |
| \-lag_in_frames or inside \-enc\-params | lag_in_frames | int    | Number of frames to lag\. Up to 25\.  | \[0\.\.25\]                          | 7              |
| \-passes or inside \-enc\-params          | passes          | int    | Number of passes\.                    | \[1\.\.2\]                           | 1              |
| \-profile:v                               |                 | int    | Encoder profile                       | \[0\.\.3\]                           | 0              |
| \-preset           |                 | string | Encoding preset\.                                                                                        | superfast/fast/medium/slow/superslow                                                                                                                                                 | none                       |

# H264 H265 Encoder

| Option             | Sub Option      | Type   | Description                                                                                              | Range of Value Valid                                                                                                                                                                 | Default Value              |
|--------------------|-----------------|--------|----------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------|
| \-b:v              |                 | int    | Target bitrate for rate control\.                                                                        | \[10000\.\.levelMax\]                                                                                                                                                                | 1000000                    |
| \-r (for input\)  |                 | int    | Input picture rate numerator\.                                                                           | \[1\.\.1048575\]                                                                                                                                                                     | 30                         |
|                    |                 | int    | Input picture rate denominator\.                                                                         | \[1\.\.1048575\]                                                                                                                                                                     | 1                          |
| \-r (for output\) |                 | int    | Output picture rate numerator\.                                                                          | \[1\.\.1048575\]                                                                                                                                                                     | inputRateNumer             |
|                    |                 | int    | Output picture rate denominator\.                                                                        | \[1\.\.1048575\]                                                                                                                                                                     | inputRateDenom             |
| \-profile:v        |                 | int    | Encode profile:HEVC:0\-2; H264:9\-12                                                                     | HEVC: main main10 H264:baseline/main/high/high10                                                                                                                                     | main                       |
| \-level            |                 | int    | Encode level                                                                                             | HEVC support level\(1\.0; 2\.0; 2\.1; 3\.0; 3\.1; 4\.0; 4\.1; 5\.0; 5\.1\) H264 support level\(1; 1b; 1\.1; 1\.2; 1\.3; 2; 2\.1; 2\.2; 3; 3\.1; 3\.2; 4; 4\.1; 4\.2; 5; 5\.1; 5\.2\) | HEVC: 5\.1 H264: 5\.2      |
| \-enc-params   | intraPicRate    | int    | Intra picture rate in frames\.                                                                           | \[0\.\.INT\_MAX\]                                                                                                                                                                    | 0                          |
|                    | bitrateWindow   | int    | Bitrate window length in frames\.                                                                        | \[1\.\.300\]                                                                                                                                                                         | intraPicRate               |
|                    | intraQpDelta    | int    | Intra QP delta, QP difference between target QP and intra frame QP\.                                     | \[\-51\.\.51\]                                                                                                                                                                       | \-5                        |
|                    | qpHdr           | int    | Initial target QP\.                                                                                      | \[\-1\.\.255\]                                                                                                                                                                       | 26                         |
|                    | qpMin           | int    | Minimum frame header QP for any slices\.                                                                 | \[0\.\.51\]                                                                                                                                                                          | 0                          |
|                    | qpMax           | int    | Maximum frame header QP for any slices\.                                                                 | \[0\.\.51\]                                                                                                                                                                          | 51                         |
|                    | fixedIntraQp    | int    | Fixed Intra QP\. Use fixed QP value for every intra frame in stream\.                                    | \[0\.\.51\]                                                                                                                                                                          | 0                          |
|                    | tier            | int    | Encoder tier                                                                                             | 0  \-main tier ; 1  \-high tier                                                                                                                                                      | 0                          |
|                    | byteStream      | int    | Stream type\.                                                                                            | 0 \- NAL units; 1 \- byte stream according to Hevc Standard Annex B                                                                                                                  | 1                          |
|                    | videoRange      | int    | Video signal sample range value in Hevc stream\.                                                         | 0 \- Y range in \[16\.\.235\] Cb,Cr in \[16\.\.240\] ; 1 \- Y,Cb,Cr range in \[0\.\.255\]                                                                                            | 1                          |
|                    | sei             | int    | Enable SEI messages \(buffering period \+ picture timing\)\.                                             | \[0\.\.INT\_MAX\]                                                                                                                                                                    | 0                          |
|                    | cabac           | int    | Select cavlc or cabac, only for H264 encoding                                                            | 0 for cavlc, 1 for cabac\.                                                                                                                                                           | 1                          |
|                    | sliceSize       | int    | Slice size in number of CTU rows\. 0 to encode each picture in one slice\.                               | \[0\.\.height/ctu\_size\]                                                                                                                                                            | 0                          |
|                    | bitVarRangeI    | int    | Percent variations over average bits per frame for I frame\.                                             | \[10\.\.10000\]                                                                                                                                                                      | 10000                      |
|                    | bitVarRangeP    | int    | Percent variations over average bits per frame for P frame\.                                             | \[10\.\.10000\]                                                                                                                                                                      | 10000                      |
|                    | bitVarRangeB    | int    | Percent variations over average bits per frame for B frame\.                                             | \[10\.\.10000\]                                                                                                                                                                      | 10000                      |
|                    | picRc           | int    | Picture rate control enable\.                                                                            | 0=OFF, 1=ON                                                                                                                                                                          | 0                          |
|                    | ctbRc           | int    | CTB QP adjustment mode for Rate Control and Subjective Quality\.                                         | \[0\.\.1\]                                                                                                                                                                           | 0                          |
|                    | tolCtbRcInter   | float  | Tolerance of Ctb Rate Control for INTER frames\.                                                         | float point number, \[targetPicSize/\(1\+tolCtbRcInter\), argetPicSize\*\(1\+tolCtbRcInter\)\]; A negative number means no bit rate limit in Ctb Rc                                  | 0\.0                       |
|                    | tolCtbRcIntra   | float  | Tolerance of Ctb Rate Control for INTRA frames\.                                                         | float point number                                                                                                                                                                   | \-1\.0                     |
|                    | ctbRowQpStep    | int    | The maximum accumulated QP adjustment step per CTB Row allowed by Ctb Rate Control\.                     | QP\_step\_per\_CTB = \(ctbRowQpStep / Ctb\_per\_Row\) and limited by maximum = 4                                                                                                     | H264: 4; HEVC: 16          |
|                    | picQpDeltaRange | int    | Qp\_Delta Range in Picture RC                                                                            | \[Min:Max\] Min: \[\-1\.\.\-10\], Max: \[1\.\.10\]                                                                                                                                   | Min:\-2; Max:3             |
|                    | hrdConformance  | int    | Enable HRD conformance\. Uses standard defined model to limit bitrate variance\.                         | \[0\.\.1\]                                                                                                                                                                           | 0                          |
|                    | cpbSize         | int    | HRD Coded Picture Buffer size in bits\.                                                                  | > 0                                                                                                                                                                                  | 1000000                    |
|                    | gopSize         | int    | GOP Size\.                                                                                               | \[0\.\.8\] 0 adaptive size; 1\-7 fixed size                                                                                                                                          | 0                          |
|                    | gopLowdelay     | int    | Enable default lowDelay GOP configuration\. If \-\-gopConfig not specified, only valid for GOP size <= 4 | \[0\.\.1\]                                                                                                                                                                           | 0                          |
|                    | qpMinI          | int    | qpMin for I Slice\.                                                                                      | \[0\.\.51\]                                                                                                                                                                          | 0                          |
|                    | qpMaxI          | int    | qpMax for I Slice\.                                                                                      | \[0\.\.51\]                                                                                                                                                                          | 51                         |
|                    | bFrameQpDelta   | int    | QP difference between BFrame QP and target QP\.                                                          | \[\-1\.\.51\]                                                                                                                                                                        | \-1                        |
|                    | chromaQpOffset  | int    | Chroma QP offset\.                                                                                       | \[\-12\.\.12\]                                                                                                                                                                       | 0                          |
|                    | vbr             | int    | Enable variable Bit Rate Control by qpMin\.                                                              | 0=OFF, 1=ON                                                                                                                                                                          | 0                          |
|                    | userData        | string | SEI User data file name\. File is read and inserted as SEI message before first frame\.                  |                                                                                                                                                                                      |                            |
|                    | intraArea       | int    | CTB coordinates specifying rectangular area of CTBs to force encoding in intra mode\.                    | left : top : right : bottom                                                                                                                                                                | \-1                        |
|                    | ipcm1Area       | int    | CTB coordinates specifying rectangular area of CTBs to force encoding in IPCM mode\.                     | left : top : right : bottom                                                                                                                                                                | \-1                        |
|                    | ipcm2Area       | int    | CTB coordinates specifying rectangular area of CTBs to force encoding in IPCM mode\.                     | left : top : right : bottom                                                                                                                                                                | \-1                        |
|                    | constChroma     | int    | Enable setting chroma a constant pixel value\.                                                           | \[0\.\.1\]                                                                                                                                                                           | 0                          |
|                    | constCb         | int    | The constant pixel value for Cb\.                                                                        | 8bit \[0\.\.255\], 10bit \[0\.\.1023\]                                                                                                                                               | 8\-bit:  128, 10\-bit: 512 |
|                    | constCr         | int    | The constant pixel value for Cr\.                                                                        | 8bit \[0\.\.255\], 10bit \[0\.\.1023\]                                                                                                                                               | 8\-bit:  128, 10\-bit: 512 |
|                    | rdoLevel        | int    | Programable HW RDO Level\.                                                                               | \[1\.\.3\]                                                                                                                                                                           | 1                          |
|                    | ssim            | int    | Enable SSIM Calculation\.                                                                                | 0 \- Disable, 1 \- Enable                                                                                                                                                            | 1                          |
|                    | vuiTimingInfo   | int    | Write VUI timing info in SPS\.                                                                           | 0 \- Disable, 1 \- Enable                                                                                                                                                            | 1                          |
|                    | lookaheadDepth  | int    | Number of lookahead frames\.                                                                             | \[0\.\.40\]                                                                                                                                                                          | 0                          |
|                    | force8bit       | int    | Force to output 8bit stream\.                                                                            | \[0\.\.1\]                                                                                                                                                                           | 0                          |
| \-crf              |                 | int    | VCE Constant rate factor mode\. Works with lookahead turned on\.                                         | \[\-1\.\.51\]                                                                                                                                                                        | \-1                        |
| \-preset           |                 | string | Encoding preset\.                                                                                        | superfast/fast/medium/slow/superslow                                                                                                                                                 | none                       |

# Preset Detail
| Level     | format | Parameters setting                                                                                                                                          | comment                                                         |
|-----------|--------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------|
| superfast | h264   |  \-\-intraQpDelta=\-2 \-\-qpHdr=\-1 \-\-picRc=1 \-\-ctbRc=0 \-\-gopSize=1                                                                                   |                                                                 |
|           | hevc   |  \-\-intraQpDelta=\-2 \-\-qpHdr=\-1 \-\-picRc=1 \-\-ctbRc=0 \-\-gopSize=1 \-\-rdoLevel=1                                                                    |                                                                 |
|           | vp9    |  \-\-qpHdr=\-1 \-\-picRc=1 \-\-effort=0 \-\-mcompFilterType=4 \-\-refFrameScheme=0 \-\-lag\-in\-frames=0                                                    |                                                                 |
| fast      | h264   |  \-\-intraQpDelta=\-2 \-\-qpHdr=\-1 \-\-picRc=1 \-\-ctbRc=0 \-\-gopSize=4                                                                                   | intraQpDelta=\-2, intra frame better quality than P frame;      |
|           |        |                                                                                                                                                             | ctbRc: 1 better subjective quality; 0 better objective quality; |
|           | hevc   |  \-\-intraQpDelta=\-2 \-\-qpHdr=\-1 \-\-picRc=1 \-\-ctbRc=0 \-\-gopSize=4 \-\-rdoLevel=1                                                                    |                                                                 |
|           | vp9    |  \-\-qpHdr=\-1 \-\-picRc=1 \-\-effort=0 \-\-mcompFilterType=4 \-\-refFrameScheme=4 \-\-lag\-in\-frames=7                                                    |                                                                 |
| medium    | h264   |  \-\-intraQpDelta=\-2 \-\-qpHdr=\-1 \-\-picRc=1 \-\-ctbRc=0 \-\-gopSize=4 \-\-lookaheadDepth=20                                                             |                                                                 |
|           | hevc   |  \-\-intraQpDelta=\-2 \-\-qpHdr=\-1 \-\-picRc=1 \-\-ctbRc=0 \-\-gopSize=4 \-\-rdoLevel=1 \-\-lookaheadDepth=20                                              |                                                                 |
|           | vp9    |  \-\-qpHdr=\-1 \-\-picRc=1 \-\-effort=0 \-\-mcompFilterType=4 \-\-refFrameScheme=4 \-\-arf\-temporal\-filter\-enabled=0 \-\-passes=2 \-\-lag\-in\-frames=12 |                                                                 |
| slow      | h264   |  \-\-intraQpDelta=\-2 \-\-qpHdr=\-1 \-\-picRc=1 \-\-ctbRc=0 \-\-gopSize=0 \-\-lookaheadDepth=30                                                             | adaptive GOP size                                               |
|           | hevc   |  \-\-intraQpDelta=\-2 \-\-qpHdr=\-1 \-\-picRc=1 \-\-ctbRc=0 \-\-gopSize=0 \-\-rdoLevel=2 \-\-lookaheadDepth=30                                              | adaptive GOP size                                               |
|           | vp9    |  \-\-qpHdr=\-1 \-\-picRc=1 \-\-effort=1 \-\-mcompFilterType=4 \-\-refFrameScheme=4 \-\-arf\-temporal\-filter\-enabled=0 \-\-passes=2 \-\-lag\-in\-frames=25 | 2\-pass and max lag\-in\-frame setting                          |
| superslow | h264   |  \-\-intraQpDelta=\-2 \-\-qpHdr=\-1 \-\-picRc=1 \-\-ctbRc=0 \-\-gopSize=0 \-\-lookaheadDepth=40                                                             | adaptive GOP size                                               |
|           | hevc   |  \-\-intraQpDelta=\-2 \-\-qpHdr=\-1 \-\-picRc=1 \-\-ctbRc=0 \-\-gopSize=0 \-\-rdoLevel=3 \-\-lookaheadDepth=40                                              | adptive gop encoding                                            |
|           | vp9    |  \-\-qpHdr=\-1 \-\-picRc=1 \-\-effort=2 \-\-mcompFilterType=4 \-\-refFrameScheme=4 \-\-arf\-temporal\-filter\-enabled=0 \-\-passes=2 \-\-lag\-in\-frames=25 | 2\-pass and max lag\-in\-frame setting, effort to 2             |
