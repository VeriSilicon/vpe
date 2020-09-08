# Table of Contents

* [Introduction](#Introduction)
* [VPE Plugin Description](#VPE-Plugin-Description)
* [VPE Overall Architecture](#VPE-Overall-Architecture)
* [VPE Directory Description](#VPE-Directory-Description)
* [Building and Installation](#Building-and-Installation)
* [VPE H264/H265 Encoder Parameters](#VPE-H264/H265-Encoder-Parameters)
* [VPE VP9 Encoder Parameters](#VPE-VP9-Encoder-Parameters)
* [Enable Log](#Enable-Log)

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

# VPE Directory Description
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
# VPE H264/H265 Encoder Parameters
Verisilicon platform H26X encoder enc_params user manual

The enc_params parameter fully follows FFmpeg rule, for example:
> -enc_params "ref_frame_scheme=4:lag_in_frames=18:passes=2:bitrate_window=60:effort=0:intra_pic_rate=60"

Below table lists all of the supported parameters by VPE H264/H265 encoder:

|params name              |Type|  Min|    Max|  Description                     |
|:------------------------|:---|:----|:------|:---------------------------------|
|force8bit|                 INT|    0|      1|  force to output 8bit stream.    |
|intra_pic_rate|            INT|    0|      -|  Intra picture rate in frames. (default [0])Forces every Nth frame to be encoded as intra frame.|
|bitrate_window|            INT| -255|    300|  Bitrate window length in frames. (default [intra_pic_rate]).|
|intra_qp_delta|            INT| -255|    127|  Intra QP delta. (default [26], -1..51).[Bigsea] Intra QP delta. (default [0], -127..127).|
|qp_hdr|                    INT|   -1|    255|  Initial QP used for the first frame. (default [-1], -1..51).[Bigsea] Initial QP used for the first frame. (default [-1 or 50], -1..255).|
|qp_min|                    INT|    0|    255|  Minimum frame header QP for any slices. (default [0], 0..51).[Bigsea] Minimum frame header QP. (default [10], 0..255).|
|qp_max|                    INT|    0|    255|  Maximum frame header QP for any slices. (default [51], 0..51).[Bigsea] Maximum frame header QP. (default [255], 0..255)|
|fixed_intra_qp|            INT|    0|    255|  Fixed Intra QP,0|= disabled. (default [0], 0..51).[Bigsea] Fixed Intra QP,0|= disabled. (default [0], 0..255).|
|pic_skip|                  INT|    0|      1|  Enable picture skip rate control.|
|bitdepth|                  INT|    0|      1|  Bitdepth. 0=8-bit, 1=10-bit. [default: keep the same as input].|
|tier|                      INT|    0|      1|  [VC8000E/HEVC] encoder only (default [0], 0..1)0 -main tier1 -high tier|
|byte_stream|               INT|    0|      1|  stream type (default [1], 0..1)0 - NAL units. Nal sizes returned in <nal_sizes.txt>1 - byte stream according to Hevc Standard Annex B.|
|video_range|               INT|    0|      1|  [VC8000E/HEVC] Video signal sample range value in Hevc stream. (default [0], 0..1)0 - Y range in [16..235] Cb,Cr in [16..240]1 - Y,Cb,Cr range in [0..255]|
|sei|                       INT|    -|      -|  enable SEI messages.|
|disable_cabac|             INT|    0|      1|  disable cabac, use cavlc.|
|slice_size|                INT|    0|      -|  slice size in number of CTU rows. (default [0], 0..height/ctu_size)0 - to encode each picture in one slice1..height/ctu_size - to each slice with N CTU row|
|tol_moving_bitrate|        INT|    0|   2000|  percent tolerance over target bitrate of moving bit rate (default [2000], 0..2000%%).|
| bit_var_range_I|          INT|    10| 10000|  percent variations over average bits per frame for I frame. (default [10000], 10..10000%%).|
| bit_var_range_P|          INT|    10| 10000|  percent variations over average bits per frame for P frame. (default [10000], 10..10000%%).|
| bit_var_range_B|          INT|    10| 10000|  percent variations over average bits per frame for B frame. (default [10000], 10..10000%%).|
|enable_pic_rc|             INT|  -255|     1|  enable picture rate control. Calculates new target QP for every frame.|
|ctb_rc|                    INT|    0|      3|  CTB QP adjustment mode for Rate Control and Subjective Quality. (default [0], 0..3).0 = No CTB QP adjustment (best PSNR).1 = CTB QP adjustment for Subjective Quality only.2 = CTB QP adjustment for Rate Control only(suggest, best bitrate).3 = CTB QP adjustment for both Subjective Quality and Rate Control.|
|tol_ctb_rc_inter|          FLOAT|  -|      -|  Tolerance of Ctb Rate Control for INT|ER frames. (float point number). (default [0.0])Ctb Rc will try to limit INT|ER frame bits within the range of:\t[targetPicSize/(1+tol_ctb_rc_inter), targetPicSize*(1+tol_ctb_rc_inter)].|
|tol_ctb_rc_intra|          FLOAT|  -|      -|  Tolerance of Ctb Rate Control for INT|RA frames. (float point number). (default [-1.0])|
|ctb_row_qp_step|           INT|    -|  -|      The maximum accumulated QP adjustment step per CTB Row allowed by Ctb Rate Control.Default value is [4] for H264 and [16] for HEVC.QP_step_per_CTB = (ctbRowQpStep / Ctb_per_Row) and limited by maximum = 4.|
|pic_qp_deelta_range|       INT|    -|      -|  Min:Max. Qp_Delta Range in Picture RC.Min: -1..-10 Minimum Qp_Delta in Picture RC. [-2]Max: 1..10 Maximum Qp_Delta in Picture RC. [3]|
|hrd_conformance|           INT|    0|      1|  enable HRD conformance. Uses standard defined model to limit bitrate variance.|
|cpb_size|                  INT|    -|      -|  HRD Coded Picture Buffer size in bits. default [1000000], suggest 2xbitrate.|
|gop_size|                  INT|    0|      8|  GOP Size. (default [0], 0..8).0 for adaptive GOP size; 1~7 for fixed GOP size.|
|gop_lowdelay|              INT|    0|      1|  Enable default lowDelay GOP configuration if --gopConfig not specified, only valid for GOP size <= 4.|
|qp_min_I|                  INT|    0|     51|  minimum frame header QP overriding qp_min for I slices. (default [0], 0..51).|
|qp_max_I|                  INT|    0|     51|  maximum frame header QP overriding qp_max for I slices. (default [51], 0..51).|
|bframe_qp_delta|           INT|    -1|    51|  BFrame QP Delta. (default [-1], -1..51).|
|chroma_qp_offset|          INT|    -12|   12|  Chroma QP offset. (default [0], -12..12).|
|vbr|                       INT|    0|      1|  enable variable Bit Rate Control by qp_min.|
|user_data|                 STR|    -|       |  SEI User data file name. File is read and inserted as SEI message before first frame.|
|intra_area|                INT|    -|      -|  left : top : right : bottom. CTB coordinatesspecifying rectangular area of CTBs to force encoding in intra mode.|
|ipcm1_area|                INT|    -|      -|  left : top : right : bottom. CTB coordinatesspecifying rectangular area of CTBs to force encoding in IPCM mode.|
|ipcm2_area|                INT|    -|      -|  left : top : right : bottom. CTB coordinatesspecifying rectangular area of CTBs to force encoding in IPCM mode.|
|enable_const_chroma|       INT|    0|      1|  enable setting chroma a constant pixel value.|
|const_cb|                  INT|    0|   1023|  The constant pixel value for Cb.(for 8bit default [128], 0..255, for 10bit default [512], 0..1023).|
|const_cr|                  INT|    0|   1023|  The constant pixel value for Cr.(for 8bit default [128], 0..255, for 10bit default [512], 0..1023).|
|rdo_level|                 INT|    1|     3 |  [VC8000E/HEVC] programable HW RDO Level (default [1], 1..3).|
|disable_ssim|              INT|    0|  1    |  Disable SSIM Calculation.|
|disable_vui_timing_info|   INT|    0|  1    |  Disable Write VUI timing info in SPS.|
|lookahead_depth|           INT|    0|  40   |  Number of frames to lookahead. Up to 40. [0]|

# VPE VP9 Encoder Parameters

Verisilicon platform VP9 encoder enc_params user manual

The enc_params parameter fully follows FFmpeg rule, for example:
> -enc_params "ref_frame_scheme=4:lag_in_frames=18:passes=2:bitrate_window=60:effort=0:intra_pic_rate=60"

Below table lists all of the supported parameters by VPE VP9 encoder:

|params name        |Type        |Min     |Max      |Description      |
|:------------------|:-----------|:-------|:--------|:----------------|
|intra_pic_rate		|int		 |0	      |65535	|Intra picture rate in frames. |
|bitrate_window		|int		 |0	      |300		|Bitrate window length in frames.|
|qp_hdr				|int		 |0	      |255		|Initial QP used for the first frame. |
|qp_min				|int		 |0	      |255		|Minimum frame header QP.|
|qp_max				|int		 |0	      |255		|Maximum frame header QP.|
|fixed_intra_qp		|int		 |0	      |255		|Fixed Intra QP 0 = disabled.|
|picRc				|int		 |0	      |1		|0=OFF, 1=ON Picture rate control enable. |
|mcomp_filter_type	|int		 |0	      |4		|Interpolation filter mode. |
|effort				|int		 |0	      |5		|Encoder effort level 0=fastest 5=best quality|
|ref_frame_scheme	|int		 |0	      |5		|Reference frame update scheme. Values TBD.|
|filte_level		|int		 |0	      |64		|Filter strength level for deblocking|
|filter_sharpness	|int		 |0	      |8		|0..8 8=auto Filter sharpness for deblocking.|
|lag_in_frames		|int		 |0	      |25		|Number of frames to lag. Up to 25. |
|passes				|int		 |0	      |2		|Number of passes (1/2). |

# Enable Log

Log is controlled in FFMpeg parameters -init_hw_device by vpeloglevel parameter:

for example:
> -init_hw_device vpe=dev0:/dev/transcoder0,priority=vod,vpeloglevel=4

Below is lists all of the debug level:

|Level        |log level        |
|:------------------|:-----------|
|0|turn off log|
|3|error|
|4|warnning|
|5|information|
|6|debug|
|6|verbose|
