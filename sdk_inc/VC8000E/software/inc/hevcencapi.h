/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------
--
--  Abstract : H2 VC Encoder API
--
------------------------------------------------------------------------------*/

#ifndef API_H
#define API_H

#include "base_type.h"
#include "enccommon.h"

#ifdef __cplusplus
extern "C"
{
#endif
#define VCENC_MAX_REF_FRAMES 8
#define MAX_GOP_PIC_CONFIG_NUM 48
#define MAX_GOP_SIZE 8
#define MAX_CORE_NUM 4
#define MAX_ADAPTIVE_GOP_SIZE 8
#define MAX_GOP_SPIC_CONFIG_NUM 16
#define VCENC_STREAM_MIN_BUF0_SIZE       1024*11
#define LONG_TERM_REF_ID2DELTAPOC(id) ((id)+10000)
#define LONG_TERM_REF_DELTAPOC2ID(poc) ((poc)-10000)
#define IS_LONG_TERM_REF_DELTAPOC(poc) ((poc) >= 10000)
#define VCENC_MAX_LT_REF_FRAMES VCENC_MAX_REF_FRAMES
#define MMO_STR2LTR(poc) ((poc) | 0x40000000)
#define X265_BFRAME_MAX         16
#define X265_LOOKAHEAD_MAX      64

    /* define for special configure*/
#define QPOFFSET_RESERVED   -255
#define QPFACTOR_RESERVED   -255
#define TEMPORALID_RESERVED -255
#define NUMREFPICS_RESERVED -255
#define FRAME_TYPE_RESERVED -255
#define INVALITED_POC       -1

  /*------------------------------------------------------------------------------
      1. Type definition for encoder instance
  ------------------------------------------------------------------------------*/
  typedef const void *VCEncInst;


  /*------------------------------------------------------------------------------
      2. Enumerations for API parameters
  ------------------------------------------------------------------------------*/

  /* Function return values */
  typedef enum
  {
    VCENC_OK = 0,
    VCENC_FRAME_READY = 1,
    VCENC_FRAME_ENQUEUE = 2,

    VCENC_ERROR = -1,
    VCENC_NULL_ARGUMENT = -2,
    VCENC_INVALID_ARGUMENT = -3,
    VCENC_MEMORY_ERROR = -4,
    VCENC_EWL_ERROR = -5,
    VCENC_EWL_MEMORY_ERROR = -6,
    VCENC_INVALID_STATUS = -7,
    VCENC_OUTPUT_BUFFER_OVERFLOW = -8,
    VCENC_HW_BUS_ERROR = -9,
    VCENC_HW_DATA_ERROR = -10,
    VCENC_HW_TIMEOUT = -11,
    VCENC_HW_RESERVED = -12,
    VCENC_SYSTEM_ERROR = -13,
    VCENC_INSTANCE_ERROR = -14,
    VCENC_HRD_ERROR = -15,
    VCENC_HW_RESET = -16
  } VCEncRet;

  /* Video Codec Format */
  typedef enum
  {
    VCENC_VIDEO_CODEC_HEVC=0,
    VCENC_VIDEO_CODEC_H264=1,
    VCENC_VIDEO_CODEC_AV1=2
  }VCEncVideoCodecFormat;

  /* Stream type for initialization */
  typedef enum
  {
    VCENC_BYTE_STREAM = 0,    /* NAL unit starts with hex bytes '00 00 00 01' */
    VCENC_NAL_UNIT_STREAM = 1 /* Plain NAL units without startcode */
  } VCEncStreamType;

  /* Level for initialization */
  typedef enum
  {
    VCENC_HEVC_LEVEL_1 = 30,
    VCENC_HEVC_LEVEL_2 = 60,
    VCENC_HEVC_LEVEL_2_1 = 63,
    VCENC_HEVC_LEVEL_3 = 90,
    VCENC_HEVC_LEVEL_3_1 = 93,
    VCENC_HEVC_LEVEL_4 = 120,
    VCENC_HEVC_LEVEL_4_1 = 123,
    VCENC_HEVC_LEVEL_5 = 150,
    VCENC_HEVC_LEVEL_5_1 = 153,
    VCENC_HEVC_LEVEL_5_2 = 156,
    VCENC_HEVC_LEVEL_6 = 180,
    VCENC_HEVC_LEVEL_6_1 = 183,
    VCENC_HEVC_LEVEL_6_2 = 186,

    /* H264 Defination*/
    VCENC_H264_LEVEL_1 = 10,
    VCENC_H264_LEVEL_1_b = 99,
    VCENC_H264_LEVEL_1_1 = 11,
    VCENC_H264_LEVEL_1_2 = 12,
    VCENC_H264_LEVEL_1_3 = 13,
    VCENC_H264_LEVEL_2 = 20,
    VCENC_H264_LEVEL_2_1 = 21,
    VCENC_H264_LEVEL_2_2 = 22,
    VCENC_H264_LEVEL_3 = 30,
    VCENC_H264_LEVEL_3_1 = 31,
    VCENC_H264_LEVEL_3_2 = 32,
    VCENC_H264_LEVEL_4 = 40,
    VCENC_H264_LEVEL_4_1 = 41,
    VCENC_H264_LEVEL_4_2 = 42,
    VCENC_H264_LEVEL_5 = 50,
    VCENC_H264_LEVEL_5_1 = 51,
    VCENC_H264_LEVEL_5_2 = 52,
    VCENC_H264_LEVEL_6 = 60,
    VCENC_H264_LEVEL_6_1 = 61,
    VCENC_H264_LEVEL_6_2 = 62
  } VCEncLevel;

  /* Profile for initialization */
  typedef enum
  {
    VCENC_HEVC_MAIN_PROFILE = 0,
    VCENC_HEVC_MAIN_STILL_PICTURE_PROFILE = 1,
    VCENC_HEVC_MAIN_10_PROFILE = 2,
    /* H264 Defination*/
    VCENC_H264_BASE_PROFILE = 9,
    VCENC_H264_MAIN_PROFILE = 10,
    VCENC_H264_HIGH_PROFILE = 11,
    VCENC_H264_HIGH_10_PROFILE = 12,
  } VCEncProfile;

  /* Tier for initialization */
  typedef enum
  {
    VCENC_HEVC_MAIN_TIER = 0,
    VCENC_HEVC_HIGH_TIER = 1,
  } VCEncTier;

  /* Picture YUV type for initialization */
  typedef enum
  {
    VCENC_YUV420_PLANAR = 0,                  /* YYYY... UUUU... VVVV...  */
    VCENC_YUV420_SEMIPLANAR = 1,              /* YYYY... UVUVUV...        */
    VCENC_YUV420_SEMIPLANAR_VU = 2,           /* YYYY... VUVUVU...        */
    VCENC_YUV422_INTERLEAVED_YUYV = 3,        /* YUYVYUYV...              */
    VCENC_YUV422_INTERLEAVED_UYVY = 4,        /* UYVYUYVY...              */
    VCENC_RGB565 = 5,                         /* 16-bit RGB 16bpp         */
    VCENC_BGR565 = 6,                         /* 16-bit RGB 16bpp         */
    VCENC_RGB555 = 7,                         /* 15-bit RGB 16bpp         */
    VCENC_BGR555 = 8,                         /* 15-bit RGB 16bpp         */
    VCENC_RGB444 = 9,                         /* 12-bit RGB 16bpp         */
    VCENC_BGR444 = 10,                         /* 12-bit RGB 16bpp         */
    VCENC_RGB888 = 11,                         /* 24-bit RGB 32bpp         */
    VCENC_BGR888 = 12,                         /* 24-bit RGB 32bpp         */
    VCENC_RGB101010 = 13,                      /* 30-bit RGB 32bpp         */
    VCENC_BGR101010 = 14,                       /* 30-bit RGB 32bpp         */
    VCENC_YUV420_PLANAR_10BIT_I010 = 15,         /* YYYY... UUUU... VVVV...  */
    VCENC_YUV420_PLANAR_10BIT_P010 = 16,         /* YYYY... UUUU... VVVV...  */
    VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR = 17,/* YYYY... UUUU... VVVV...  */
    VCENC_YUV420_10BIT_PACKED_Y0L2 = 18,         /* Y0U0Y1a0a1Y2V0Y3a2a3Y4U1Y5a4a5Y6V1Y7a6a7... */
    VCENC_YUV420_PLANAR_8BIT_DAHUA_HEVC = 19,
    VCENC_YUV420_PLANAR_8BIT_DAHUA_H264 = 20,
    VCENC_YUV420_SEMIPLANAR_8BIT_FB = 21,              /* YYYY... UVUVUV...        */
    VCENC_YUV420_SEMIPLANAR_VU_8BIT_FB = 22,           /* YYYY... VUVUVU...        */
    VCENC_YUV420_PLANAR_10BIT_P010_FB = 23,            /* YYYY... UVUV... */
    VCENC_YUV420_SEMIPLANAR_101010 = 24,               /* YYYY... UVUV... */
    VCENC_YUV420_8BIT_TILE_64_4 = 26,                  /* YYYY... VUVU... */
    VCENC_YUV420_UV_8BIT_TILE_64_4 = 27,               /* YYYY... UVUV... */
    VCENC_YUV420_10BIT_TILE_32_4 = 28,                  /* YYYY... UVUV... */
    VCENC_YUV420_10BIT_TILE_48_4 = 29,                  /* YYYY... UVUV... */
    VCENC_YUV420_VU_10BIT_TILE_48_4 = 30,               /* YYYY... VUVU... */
    VCENC_YUV420_8BIT_TILE_128_2 = 31,                  /* YYYY... VUVU... */
    VCENC_YUV420_UV_8BIT_TILE_128_2 = 32,               /* YYYY... UVUV... */
    VCENC_YUV420_10BIT_TILE_96_2 = 33,                  /* YYYY... UVUV... */
    VCENC_YUV420_VU_10BIT_TILE_96_2 = 34,               /* YYYY... VUVU... */
    VCENC_FORMAT_MAX,

#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
    INPUT_FORMAT_YUV420_SEMIPLANAR_8BIT_COMPRESSED_FB = VCENC_FORMAT_MAX+1, /*36*/
    INPUT_FORMAT_YUV420_SEMIPLANAR_VU_8BIT_COMPRESSED_FB, /*37*/
    INPUT_FORMAT_YUV420_PLANAR_10BIT_P010_COMPRESSED_FB, /*38*/
    INPUT_FORMAT_RFC_8BIT_COMPRESSED_FB, /*39*/
    INPUT_FORMAT_RFC_10BIT_COMPRESSED_FB, /*40*/
    INPUT_FORMAT_ARGB_FB, /*41*/
    INPUT_FORMAT_ABGR_FB, /*42*/
    INPUT_FORMAT_RGBA_FB, /*43*/
    INPUT_FORMAT_BGRA_FB, /*44*/
    INPUT_FORMAT_RGB24_FB, /*45*/
    INPUT_FORMAT_BGR24_FB, /*46*/
    INPUT_FORMAT_YUV420_PLANAR_10BIT_P010LE, /*47*/
    INPUT_FORMAT_YUV420_PLANAR_10BIT_P010BE, /*48*/
    INPUT_FORMAT_YUV420_SEMIPLANAR_10BIT_P010BE, /*49*/
    INPUT_FORMAT_YUV422P, /*50*/
    INPUT_FORMAT_YUV422P10LE, /*51*/
    INPUT_FORMAT_YUV422P10BE, /*52*/
    INPUT_FORMAT_YUV444P, /*53*/
#endif
    INPUT_FORMAT_PP_YUV420_SEMIPLANNAR = 54, /*54*/
    INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_VU, /*55*/
    INPUT_FORMAT_PP_YUV420_PLANAR_10BIT_P010, /*56*/
    INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_YUV420P, /*57*/
  } VCEncPictureType;

  /* Picture rotation for pre-processing */
  typedef enum
  {
    VCENC_ROTATE_0 = 0,
    VCENC_ROTATE_90R = 1, /* Rotate 90 degrees clockwise */
    VCENC_ROTATE_90L = 2,  /* Rotate 90 degrees counter-clockwise */
    VCENC_ROTATE_180R = 3  /* Rotate 180 degrees clockwise */
  } VCEncPictureRotation;
  /* Picture mirror for pre-processing */
  typedef enum
  {
    VCENC_MIRROR_NO = 0, /* no mirror */
    VCENC_MIRROR_YES = 1 /* mirror */
  } VCEncPictureMirror;

  /* Picture color space conversion (RGB input) for pre-processing */
  typedef enum
  {
    VCENC_RGBTOYUV_BT601 = 0, /* Color conversion of limited range[16,235] according to BT.601 */
    VCENC_RGBTOYUV_BT709 = 1, /* Color conversion of limited range[16,235] according to BT.709 */
    VCENC_RGBTOYUV_USER_DEFINED = 2,   /* User defined color conversion */
    VCENC_RGBTOYUV_BT2020 = 3, /* Color conversion according to BT.2020 */
    VCENC_RGBTOYUV_BT601_FULL_RANGE = 4, /* Color conversion of full range[0,255] according to BT.601*/
    VCENC_RGBTOYUV_BT601_LIMITED_RANGE = 5, /* Color conversion of limited range[0,219] according to BT.601*/
    VCENC_RGBTOYUV_BT709_FULL_RANGE = 6 /* Color conversion of full range[0,255] according to BT.709*/
  } VCEncColorConversionType;
  /* Picture type for encoding */
  typedef enum
  {
    VCENC_INTRA_FRAME = 0,
    VCENC_PREDICTED_FRAME = 1,
    VCENC_BIDIR_PREDICTED_FRAME = 2,
    VCENC_NOTCODED_FRAME  /* Used just as a return value */
  } VCEncPictureCodingType;

  /* Reference picture mode for reading and writing */
  typedef enum
  {
    VCENC_NO_REFERENCE_NO_REFRESH = 0,
    VCENC_REFERENCE = 1,
    VCENC_REFRESH = 2,
    VCENC_REFERENCE_AND_REFRESH = 3
  } VCEncRefPictureMode;

  /* HDR10, transfer funtion */
  typedef enum
  {
	  VCENC_HDR10_BT2020 = 14,
	  VCENC_HDR10_ST2084 = 16,
	  VCENC_HDR10_STDB67 = 18,
  } VCEncHDRTransferType;

  /*------------------------------------------------------------------------------
      3. Structures for API function parameters
  ------------------------------------------------------------------------------*/
  typedef struct
  {
    u32 *ctuOffset; /* point to a memory containing the total CU number by the end of each CTU */
    u8  *cuData; /* cu information of a picture output by HW */
  } VCEncCuOutData;

  typedef struct
  {
    u8   refIdx; /* reference idx in reference list */
    i16  mvX; /* horiazontal motion in 1/4 pixel */
    i16  mvY; /* vertical motion in 1/4 pixel */
  } VCEncMv;

  /* HDR10 */
  typedef struct
  {
	  u8 hdr10_display_enable;
	  u16 hdr10_dx0;
	  u16 hdr10_dy0;
	  u16 hdr10_dx1;
	  u16 hdr10_dy1;
	  u16 hdr10_dx2;
	  u16 hdr10_dy2;
	  u16 hdr10_wx;
	  u16 hdr10_wy;
	  u32 hdr10_maxluma;
	  u32 hdr10_minluma;
  }Hdr10DisplaySei;

  typedef struct
  {
	  u8  hdr10_lightlevel_enable;
	  u16 hdr10_maxlight;
	  u16 hdr10_avglight;
  }Hdr10LightLevelSei;

  typedef struct
  {
	  u8 vuiColorDescripPresentFlag;
	  u8 vuiColorPrimaries;
	  u8 vuiTransferCharacteristics;
	  u8 vuiMatrixCoefficients;
  }VuiColorDescription;

  typedef struct
  {
    u8  cuLocationX; /* cu x coordinate relative to CTU */
    u8  cuLocationY; /* cu y coordinate relative to CTU */
    u8  cuSize; /*cu size. 8/16/32/64 */
    u8  cuMode; /* cu mode. 0:INTER; 1:INTRA; 2:IPCM */
    u32 cost;  /* sse cost of cuMode*/
    u32 costOfOtherMode;  /* sse cost of the other cuMode */
    u32 costIntraSatd;  /* satd cost of intra mode */
    u32 costInterSatd;  /* satd cost of inter mode*/
    u8  interPredIdc; /* only for INTER cu. prediction direction. 0: by list0; 1: by list1; 2: bi-direction */
    VCEncMv mv[2];  /* only for INTER cu. motion information. mv[0] for list0 if it's valid; mv[1] for list1 if it's valid */
    u8  intraPartMode; /* only for INTRA cu. partition mode. 0:2Nx2N; 1: NxN */
    u8  intraPredMode[16];/* only for INTRA CU. prediction mode. 0: planar; 1: DC; 2-34: Angular in HEVC spec. intraPredMode[1~3] only valid for NxN */
    u8  qp;
    u32 mean;
    u32 variance;
  } VCEncCuInfo;

  /* Configuration info for initialization
   * Width and height are picture dimensions after rotation
   * Width and height are restricted by level limitations
   */
  typedef struct
  {
    VCEncStreamType streamType;   /* Byte stream / Plain NAL units */

    VCEncProfile profile; /* Main, Main Still or main10 */

    VCEncLevel level; /* Supported Level */

    VCEncTier tier; /* Main Tier or High Tier */

    u32 width;           /* Encoded picture width in pixels, multiple of 2 */
    u32 height;          /* Encoded picture height in pixels, multiple of 2 */
    u32 frameRateNum;    /* The stream time scale, [1..1048575] */
    u32 frameRateDenom;  /* Maximum frame rate is frameRateNum/frameRateDenom
                              * in frames/second. The actual frame rate will be
                              * defined by timeIncrement of encoded pictures,
                              * [1..frameRateNum] */

    u32 refFrameAmount; /* Amount of reference frame buffers, [0..8]
                              * 0 = only I frames are encoded.
                               * 1 = gop size is 1 and interlacedFrame =0,
                               * 2 = gop size is 1 and interlacedFrame =1,
                               * 2 = gop size is 2 or 3,
                               * 3 = gop size is 4,5,6, or 7,
                               * 4 = gop size is 8
                               * 8 = gop size is 8 svct hirach 7B+1p 4 layer, only libva support this config*/
    u32 strongIntraSmoothing;       /* 0 = Normal smoothing,
                                         * 1 = Strong smoothing. */


    u32 compressor;       /*Enable/Disable Embedded Compression
                                              0 = Disable Compression
                                              1 = Only Enable Luma Compression
                                              2 = Only Enable Chroma Compression
                                              3 = Enable Both Luma and Chroma Compression*/
    u32 interlacedFrame;   /*0 = progressive frame; 1 = interlace frame  */

    u32 bitDepthLuma;     /*luma sample bit depth of encoded bit stream, 8 = 8 bits, 9 = 9 bits, 10 = 10 bits  */
    u32 bitDepthChroma;  /*chroma sample bit depth of encoded bit stream,  8 = 8 bits, 9 = 9 bits, 10 = 10 bits */
    u32 enableOutputCuInfo;  /* 1 to enable CU information dumping */
    u32 enableSsim;                /* Enable/Disable SSIM calculation */
    u32 maxTLayers; /*max number Temporal layers*/
    u32 rdoLevel; /*control RDO hw runtime effort level, balence between quality and throughput performance, [0..2]
                         * 0 = RDO run 1x cadidates
                         * 1 = RDO run 2x cadidates
                         * 2 = RDO run 3x cadidates*/
    VCEncVideoCodecFormat codecFormat;     /* Video Codec Format: HEVC/H264/AV1 */
    u32 verbose;       /* log printing mode */
    u32 exp_of_input_alignment;
    u32 exp_of_ref_alignment;
    u32 exp_of_ref_ch_alignment;
    u32 exteralReconAlloc; /* 0 = recon frame is allocated by encoder itself
                              1 = recon frame is allocated by APP*/
    u32 P010RefEnable;  /* enable P010 tile-raster format for reference frame buffer */
    u32 ctbRcMode; /* CTB QP adjustment mode for Rate Control and Subjective Quality.
                                      0 = No CTB QP adjustment.\n"
                                      1 = CTB QP adjustment for Subjective Quality only."
                                      2 = CTB QP adjustment for Rate Control only."
                                      3 = CTB QP adjustment for both Subjective Quality and Rate Control. */

    u32 picOrderCntType;
    u32 log2MaxPicOrderCntLsb;
    u32 log2MaxFrameNum;

    u32 dumpRegister;
    u32 rasterscan;  /* enable raster recon yuv dump on FPGA & HW */

    u32 parallelCoreNum;  /*Num of Core used by one encoder instance */

    /* Multipass coding config, currently only for internal test purpose */
    u32 pass;
    bool bPass1AdaptiveGop;

    u8 lookaheadDepth;
    u32 extDSRatio; /* external downsample ratio for first pass. 0=1/1 (no downsample), 1=1/2 downsample */

    // add for ds first pass
    u32 width_ds;
    u32 height_ds;

#ifdef FB_SYSLOG_ENABLE
    int enc_index;
    int device_id;
#endif
    ENCPERF * perf;
   } VCEncConfig;

  typedef struct
  {
      i32 ref_pic;          /*  delta_poc of this short reference picture relative to the poc of current picture or index of LTR */
      u32 used_by_cur;      /*  whether this reference picture used by current picture  */
  } VCEncGopPicRps;

  typedef struct
  {
      u32 poc;                            /*  picture order count within a GOP  */
      i32 QpOffset;                       /*  QP offset  */
      double QpFactor;                    /*  QP Factor  */
      i32 temporalId;                     /*  temporal layer ID */
      VCEncPictureCodingType codingType;  /*  picture coding type  */
      u32 numRefPics;                     /*  the number of reference pictures kept for this picture, the value should be within [0, VCENC_MAX_REF_FRAMES] */
      VCEncGopPicRps refPics[VCENC_MAX_REF_FRAMES];  /*  short-term reference picture sets for this picture*/
  } VCEncGopPicConfig;

  typedef struct
  {
      u32 poc;                            /*  picture order count within a GOP  */
      i32 QpOffset;                       /*  QP offset  */
      double QpFactor;                    /*  QP Factor  */
      i32 temporalId;                     /*  temporal layer ID */
      VCEncPictureCodingType codingType;  /*  picture coding type  */
      u32 numRefPics;                     /*  the number of reference pictures kept for this picture, the value should be within [0, VCENC_MAX_REF_FRAMES] */
      VCEncGopPicRps refPics[VCENC_MAX_REF_FRAMES];  /*  short-term reference picture sets for this picture*/
      i32 i32Ltr;                         /* index of the long-term referencr frame,the value should be within [0, VCENC_MAX_LT_REF_FRAMES]. 0 -- not LTR; 1...VCENC_MAX_LT_REF_FRAMES--- LTR's index */
      i32 i32Offset;                      /* offset of the special pics, relative to start of ltrInterval*/
      i32 i32Interval;                    /* interval between two pictures using LTR as referencr picture or  interval between two pictures coded as special frame */
      i32 i32short_change;                /* only change short term coding parameter. 0 - not change, 1- change */
  } VCEncGopPicSpecialConfig;

  typedef struct
  {
      VCEncGopPicConfig *pGopPicCfg;      /* Pointer to an array containing all used VCEncGopPicConfig */
      u8 size;                            /* the number of VCEncGopPicConfig pointed by pGopPicCfg, the value should be within [0, MAX_GOP_PIC_CONFIG_NUM] */
      u8 id;                              /* the index of VCEncGopPicConfig in pGopPicCfg used by current picture, the value should be within [0, size-1] */
      u8 id_next;                         /* the index of VCEncGopPicConfig in pGopPicCfg used by next picture, the value should be within [0, size-1] */
      u8 special_size;                    /* number of special configuration, the value should be within [0, MAX_GOP_PIC_CONFIG_NUM] */
      i32 delta_poc_to_next;              /* the difference between poc of next picture and current picture */
      VCEncGopPicSpecialConfig *pGopPicSpecialCfg; /* Pointer to an array containing all used VCEncGopPicSpecialConfig */
      u8 ltrcnt;                               /* Number of long-term ref pics used,the value should be within [0, VCENC_MAX_LT_REF_FRAMES]  */
      u32 u32LTR_idx[VCENC_MAX_LT_REF_FRAMES]; /* LTR's INDEX */
      i32 idr_interval;
      i32 gdrDuration;
      i32 firstPic;
      i32 lastPic;
      i32 outputRateNumer;      /* Output frame rate numerator */
      i32 outputRateDenom;      /* Output frame rate denominator */
      i32 inputRateNumer;      /* Input frame rate numerator */
      i32 inputRateDenom;      /* Input frame rate denominator */
      i32 gopLowdelay;
      i32 interlacedFrame;
      u8 gopCfgOffset[MAX_GOP_SIZE + 1];
      VCEncGopPicConfig *pGopPicCfgPass1;      /* Pointer to an array containing all used VCEncGopPicConfig, used for pass1*/
      VCEncGopPicConfig *pGopPicCfgPass2;      /* Pointer to an array containing all used VCEncGopPicConfig, used for pass2*/
  } VCEncGopConfig;

  /* Defining rectangular macroblock area in encoder picture */
  typedef struct
  {
    u32 enable;         /* [0,1] Enables this area */
    u32 top;            /* Top macroblock row inside area [0..heightMbs-1] */
    u32 left;           /* Left macroblock row inside area [0..widthMbs-1] */
    u32 bottom;         /* Bottom macroblock row inside area [top..heightMbs-1] */
    u32 right;          /* Right macroblock row inside area [left..widthMbs-1] */
  } VCEncPictureArea;

  /* Coding control parameters */
  typedef struct
  {
    u32 sliceSize;       /* Slice size in macroblock rows,
                              * 0 to encode each picture in one slice,
                              * [0..height/ctu_size]
                              */
    u32 seiMessages;     /* Insert picture timing and buffering
                              * period SEI messages into the stream,
                              * [0,1]
                              */
    u32 vuiVideoFullRange;  /* Input video signal sample range, [0,1]
                              * 0 = Y range in [16..235],
                              * Cb&Cr range in [16..240]
                              * 1 = Y, Cb and Cr range in [0..255]
                              */
    u32 disableDeblockingFilter;    /* 0 = Filter enabled,
                                         * 1 = Filter disabled,
                                         * 2 = Filter disabled on slice edges */
    i32 tc_Offset;                  /* deblock parameter, tc_offset */

    i32 beta_Offset;                /* deblock parameter, beta_offset */


    u32 enableDeblockOverride;      /* enable deblock override between slice*/
    u32 deblockOverride;            /* flag to indicate whether deblock override between slice */

    u32 enableSao;                  /* Enable SAO */

    u32 enableScalingList;          /* Enabled ScalingList */

    u32 sampleAspectRatioWidth; /* Horizontal size of the sample aspect
                                     * ratio (in arbitrary units), 0 for
                                     * unspecified, [0..65535]
                                     */
    u32 sampleAspectRatioHeight;    /* Vertical size of the sample aspect ratio
                                         * (in same units as sampleAspectRatioWidth)
                                         * 0 for unspecified, [0..65535]
                                         */
    u32 enableCabac;      /* [0,1] H.264 entropy coding mode, 0 for CAVLC, 1 for CABAC */
    u32 cabacInitFlag;    /* [0,1] CABAC table initial flag */
    u32 cirStart;           /* [0..mbTotal] First macroblock for
                                   Cyclic Intra Refresh */
    u32 cirInterval;        /* [0..mbTotal] Macroblock interval for
                                   Cyclic Intra Refresh, 0=disabled */

    i32 pcm_enabled_flag;               /* enable PCM encoding */
    i32 pcm_loop_filter_disabled_flag;  /* disable deblock filter for IPCM, 0=enabled, 1=disabled */
    VCEncPictureArea intraArea;   /* Area for forcing intra macroblocks */
    VCEncPictureArea ipcm1Area;   /* 1st Area for forcing IPCM macroblocks */
    VCEncPictureArea ipcm2Area;   /* 2st Area for forcing IPCM macroblocks */
    VCEncPictureArea roi1Area;    /* Area for 1st Region-Of-Interest */
    VCEncPictureArea roi2Area;    /* Area for 2nd Region-Of-Interest */
    VCEncPictureArea roi3Area;    /* Area for 3rd Region-Of-Interest */
    VCEncPictureArea roi4Area;    /* Area for 4th Region-Of-Interest */
    VCEncPictureArea roi5Area;    /* Area for 5th Region-Of-Interest */
    VCEncPictureArea roi6Area;    /* Area for 6th Region-Of-Interest */
    VCEncPictureArea roi7Area;    /* Area for 7th Region-Of-Interest */
    VCEncPictureArea roi8Area;    /* Area for 8th Region-Of-Interest */
    i32 roi1DeltaQp;                /* [-30..0] QP delta value for 1st ROI */
    i32 roi2DeltaQp;                /* [-30..0] QP delta value for 2nd ROI */
    i32 roi3DeltaQp;                /* [-30..0] QP delta value for 3rd ROI */
    i32 roi4DeltaQp;                /* [-30..0] QP delta value for 4th ROI */
    i32 roi5DeltaQp;                /* [-30..0] QP delta value for 5th ROI */
    i32 roi6DeltaQp;                /* [-30..0] QP delta value for 6th ROI */
    i32 roi7DeltaQp;                /* [-30..0] QP delta value for 7th ROI */
    i32 roi8DeltaQp;                /* [-30..0] QP delta value for 8th ROI */
    i32 roi1Qp;                     /* [0..51] QP value for 1st ROI */
    i32 roi2Qp;                     /* [0..51] QP value for 2nd ROI */
    i32 roi3Qp;                     /* [0..51] QP value for 3rd ROI */
    i32 roi4Qp;                     /* [0..51] QP value for 4th ROI */
    i32 roi5Qp;                     /* [0..51] QP value for 5th ROI */
    i32 roi6Qp;                     /* [0..51] QP value for 6th ROI */
    i32 roi7Qp;                     /* [0..51] QP value for 7th ROI */
    i32 roi8Qp;                     /* [0..51] QP value for 8th ROI */

    u32 fieldOrder;         /* Field order for interlaced coding,
                                   0 = bottom field first, 1 = top field first */
    i32 chroma_qp_offset;    /* chroma qp offset */

    u32 roiMapDeltaQpEnable;          /*0 = ROI map disabled, 1 = ROI map enabled.*/
    u32 roiMapDeltaQpBlockUnit;       /* 0-64x64,1-32x32,2-16x16,3-8x8*/
    u32 ipcmMapEnable;                /* 0 = IPCM map disabled,  1 = IPCM map enabled */
    u32 RoimapCuCtrl_index_enable;    /* 0 = ROI map cu ctrl index disabled,  1 = ROI map cu ctrl index enabled */
    u32 roiMapDeltaQpBinEnable;       /*0 = ROI map disabled, 1 = ROI map enabled.*/
    u32 RoimapCuCtrl_enable;          /* 0 = ROI map cu ctrl disabled,  1 = ROI map cu ctrl enabled */
    u32 RoimapCuCtrl_ver;             /* ROI map cu ctrl version number */
    u32 RoiQpDelta_ver;               /* QpDelta map version number */

    u32 skipMapEnable;               /* 0 = SKIP map disabled, 1 = SKIP map enabled */

    //wiener denoise parameters

    u32 noiseReductionEnable; /*0 = disable noise reduction; 1 = enable noise reduction */
    u32 noiseLow; /* valid value range :[1,30] , default: 10 */
    u32 firstFrameSigma; /* valid value range :[1,30] , default :11*/

    u32 gdrDuration; /*how many pictures it will take to do GDR, if 0, not do GDR*/

    /* for low latency */
    u32 inputLineBufEn;            /* enable input image control signals */
    u32 inputLineBufLoopBackEn;    /* input buffer loopback mode enable */
    u32 inputLineBufDepth;         /* input buffer depth in mb lines */
    u32 inputLineBufHwModeEn;        /* hw handshake*/
    u32 amountPerLoopBack; /* Handshake sync amount for every loopback */
    EncInputLineBufCallBackFunc inputLineBufCbFunc;  /* callback function */
    void *inputLineBufCbData;     /* callback function data */

    /*stream multi-segment*/
    u32 streamMultiSegmentMode; /* 0:single segment 1:multi-segment with no sync 2:multi-segment with sw handshake mode*/
    u32 streamMultiSegmentAmount; /*must be >= 2*/
    EncStreamMultiSegCallBackFunc streamMultiSegCbFunc; /*callback function*/
    void *streamMultiSegCbData; /*callback data*/

    /* for smart */
    i32 smartModeEnable;
    i32 smartH264Qp;
    i32 smartHevcLumQp;
    i32 smartHevcChrQp;
    i32 smartH264LumDcTh;
    i32 smartH264CbDcTh;
    i32 smartH264CrDcTh;
    /* threshold for hevc cu8x8/16x16/32x32 */
    i32 smartHevcLumDcTh[3];
    i32 smartHevcChrDcTh[3];
    i32 smartHevcLumAcNumTh[3];
    i32 smartHevcChrAcNumTh[3];
    /* back ground */
    i32 smartMeanTh[4];
    /* foreground/background threashold: maximum foreground pixels in background block */
    i32 smartPixNumCntTh;

    /* for tile*/
    i32 tiles_enabled_flag;
    i32 num_tile_columns;
    i32 num_tile_rows;
    i32 loop_filter_across_tiles_enabled_flag;

	/* for HDR10 */
	Hdr10DisplaySei    Hdr10Display;
	Hdr10LightLevelSei Hdr10LightLevel;
	VuiColorDescription vuiColorDescription;
  u32                 vuiVideoSignalTypePresentFlag;
  u32                 vuiVideoFormat;

	u32 RpsInSliceHeader;
  } VCEncCodingCtrl;

  /* Rate control parameters */
  typedef struct
  {
    i32 crf;             /*CRF constant [0,51]*/
    u32 pictureRc;       /* Adjust QP between pictures, [0,1] */
    u32 ctbRc;           /* Adjust QP between Lcus, [0,1] */ //CTB_RC
    u32 blockRCSize;    /*size of block rate control : 2=16x16,1= 32x32, 0=64x64*/
    u32 pictureSkip;     /* Allow rate control to skip pictures, [0,1] */
    i32 qpHdr;           /* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
    u32 qpMinPB;           /* Minimum QP for any P/B picture, [0..51] */
    u32 qpMaxPB;           /* Maximum QP for any P/B picture, [0..51] */
    u32 qpMinI;          /* Minimum QP for any I picture, [0..51] */
    u32 qpMaxI;          /* Maximum QP for any I picture, [0..51] */
    u32 bitPerSecond;    /* Target bitrate in bits/second, this is
                              * needed if pictureRc, mbRc, pictureSkip or
                              * hrd is enabled [10000..60000000]
                              */

    u32 hrd;             /* Hypothetical Reference Decoder model, [0,1]
                              * restricts the instantaneous bitrate and
                              * total bit amount of every coded picture.
                              * Enabling HRD will cause tight constrains
                              * on the operation of the rate control
                              */
    u32 hrdCpbSize;      /* Size of Coded Picture Buffer in HRD (bits) */
    u32 bitrateWindow;          /* Length for Group of Pictures, indicates
                              * the distance of two intra pictures,
                              * including first intra [1..300]
                              */
    i32 intraQpDelta;    /* Intra QP delta. intraQP = QP + intraQpDelta
                              * This can be used to change the relative quality
                              * of the Intra pictures or to lower the size
                              * of Intra pictures. [-12..12]
                              */
    u32 fixedIntraQp;    /* Fixed QP value for all Intra pictures, [0..51]
                              * 0 = Rate control calculates intra QP.
                              */
    i32 bitVarRangeI;/*variations over average bits per frame for I frame*/

    i32 bitVarRangeP;/*variations over average bits per frame for P frame*/

    i32 bitVarRangeB;/*variations over average bits per frame for B frame*/
    i32 tolMovingBitRate;/*tolerance of max Moving bit rate */
    i32 monitorFrames;/*monitor frame length for moving bit rate*/
    i32 smoothPsnrInGOP;  /* dynamic bits allocation among GOP using SSE feed back for smooth psnr, 1 - enable, 0 - disable */
    u32 u32StaticSceneIbitPercent; /* I frame bits percent in static scene */
    u32 rcQpDeltaRange; /* ctb rc qp delta range */
    u32 rcBaseMBComplexity; /* ctb rc mb complexity base */
    i32 picQpDeltaMin;  /* minimum pic qp delta */
    i32 picQpDeltaMax;  /* maximum pic qp delta */
    i32 longTermQpDelta; /* QP delta of the frame using long term reference */
    i32 vbr; /* Variable Bit Rate Control by qpMin */
    float tolCtbRcInter; /*Tolerance of Ctb Rate Control for INTER frames*/
    float tolCtbRcIntra; /*Tolerance of Ctb Rate Control for INTRA frames*/
    i32 ctbRcRowQpStep;  /* max accumulated QP adjustment step per CTB Row by Ctb Rate Control.
                                            QP adjustment step per CTB is ctbRcRowQpStep/ctb_per_row. */
  } VCEncRateCtrl;


	/* add for two pass */
  typedef struct {
    ptr_t TSLumaMemBusAddress;
    ptr_t TSChromaMemBusAddress;
    ptr_t PicMemRcBusAddr;
    ptr_t busLuma;
    ptr_t busChromaU;
    ptr_t busChromaV;

    u32 lumaSize;
    u32 chromaSize;
    i32 inputFormat;
    i32 lumWidthSrc;
    i32 lumHeightSrc;
    u32 input_alignment;
    i32 bitDepthLuma;

    u32 crop_x;
    u32 crop_y;
    u32 crop_w;
    u32 crop_h;

#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
    void * dec400f2_handle;
#endif

#ifdef SUPPORT_L2CACHE
    void * vce_l2r_handle;
#endif
#ifdef SUPPORT_TCACHE
    void * tcache_handle;
    void * tcache_dtrc_handle;
    void * tcache_dtrc_l2_handle;
    EWLLinearMem_t edma_link_buf;
    void * edma_handle;
#endif

  }Pass2HWParam;


  /* Encoder input structure */
  typedef struct
  {
    ptr_t busLuma;         /* Bus address for input picture
                              * planar format: luminance component
                              * semiplanar format: luminance component
                              * interleaved format: whole picture
                              */
    ptr_t busChromaU;      /* Bus address for input chrominance
                              * planar format: cb component
                              * semiplanar format: both chrominance
                              * interleaved format: not used
                              */
    ptr_t busChromaV;      /* Bus address for input chrominance
                              * planar format: cr component
                              * semiplanar format: not used
                              * interleaved format: not used
                              */
    ptr_t busLumaOrig;     /* Bus address for original input picture when first-pass input is down-scaled
                              * planar format: luminance component
                              * semiplanar format: luminance component
                              * interleaved format: whole picture
                              */
    ptr_t busChromaUOrig;  /* Bus address for original input chrominance when first-pass input is down-scaled
                              * planar format: cb component
                              * semiplanar format: both chrominance
                              * interleaved format: not used
                              */
    ptr_t busChromaVOrig;  /* Bus address for original input chrominance when first-pass input is down-scaled
                              * planar format: cr component
                              * semiplanar format: not used
                              * interleaved format: not used
                              */
    u32 timeIncrement;   /* The previous picture duration in units
                              * of 1/frameRateNum. 0 for the very first picture
                              * and typically equal to frameRateDenom for the rest.
                              */
    u32 vui_timing_info_enable; /* Write VUI timing info in SPS
                                 * 1=enable, 0=disable.
                                 */
    u32 *pOutBuf[MAX_STRM_BUF_NUM];        /* Pointer to output stream buffer */
    ptr_t busOutBuf[MAX_STRM_BUF_NUM];     /* Bus address of output stream buffer */
    u32 outBufSize[MAX_STRM_BUF_NUM];      /* Size of output stream buffer in bytes */
    VCEncPictureCodingType codingType;    /* Proposed picture coding type,
                                                 * INTRA/PREDICTED
                                                 */
    i32 poc;      /* Picture display order count */
    VCEncGopConfig gopConfig; /* GOP configuration*/
    i32 gopSize;  /* current GOP size*/
    i32 gopPicIdx;   /* encoded order count of current picture within its GOP, shoule be in the range of [0, gopSize-1] */
    ptr_t roiMapDeltaQpAddr;   /* Bus address of QpDelta map  */
    ptr_t RoimapCuCtrlAddr;      /* Bus address of ROI map cu ctrl */
    ptr_t RoimapCuCtrlIndexAddr; /* Bus address of ROI map cu ctrl index  */
    i8 *pRoiMapDelta;  /* Pointer address of QpDelta map  */
    u32 roiMapDeltaSize; /* size of QpDelta map (per frame) */
    i32 picture_cnt; /* encoded picture count */
    i32 last_idr_picture_cnt;/* encoded picture count of last IDR */

    /* for low latency */
    u32 lineBufWrCnt;    /* The number of MB lines already in input MB line buffer */

    /* for crc32/checksum */
    u32 hashType;   /* 0=disable, 1=crc32, 2=checksum32 */

    u32 sceneChange;  /* a scene change frame */

    i16 gmv[2][2];       /* global mv */

    /* long-term reference relatived info */
    u32 bIsIDR;                              /* coded current picture as IDR frame ,: false - not coded as IDR, true-coded as IDR */
    u32 bIsPeriodUsingLTR;                   /* use LTR as referencr picture periodly: false - not periodly used, true - periodly used */
    u32 bIsPeriodUpdateLTR;                  /* update LTR periodly:false - not periodly update, true - periodly update */
    VCEncGopPicConfig  gopCurrPicConfig;                   /* current pic running configuration */
    i32   long_term_ref_pic[VCENC_MAX_LT_REF_FRAMES];      /* LTR's poc:-1 - not a valid poc, >= long-term referencr's poc */
    u32  bLTR_used_by_cur[VCENC_MAX_LT_REF_FRAMES];       /* LTR used by current picture: false- not used as reference, true - used as reference by current picture */
    u32  bLTR_need_update[VCENC_MAX_LT_REF_FRAMES];       /* LTR's poc should update: false - not need updated, true- need updated */
    i8  i8SpecialRpsIdx;                    /* special RPS index selected by current coded picture, -1 - NOT, >= 0 - special RPS */
    i8  i8SpecialRpsIdx_next;               /* special RPS index selected by nextbeing coded picture, -1 - NOT, >= 0 - special RPS,for h264 */
    u8  u8IdxEncodedAsLTR;                  /* current picture coded as LTR : 0 - not a LTR, 1... VCENC_MAX_LT_REF_FRAMES -- idx of LTR */
    u32 resendSPS;      /* 0=NOT send SPS, 1= send SPS */
    u32 resendPPS;      /* 0=NOT send PPS, 1= send PPS */
    u32 sendAUD;        /* 0=NOT send AUD, 1= send AUD */
    u32 resendVPS;      /* 0=NOT send VPS, 1= send VPS */
    u32 bSkipFrame;   /* code all MB/CTB in current frame as SKIP Mode. 0=No, 1=Yes */

    Pass2HWParam PrivData;
    Pass2HWParam PassTwoHWData;
    i32 indexTobeEncode;
    bool forceIDR;
    int64_t pts;
    //int64_t dts;
  } VCEncIn;


#define SHORT_TERM_REFERENCE	0x01
#define LONG_TERM_REFERENCE	0x02
  /*all physical addresses related to recon frame*/
  typedef struct
  {
    /*recon buffer*/

    i32 poc;                    /* pic order count*/

    i32 frame_num;

    i32 frame_idx;

    i32 flags;

    i32 temporalId;

    ptr_t busReconLuma;         /* Bus address for recon picture
                                                      * semiplanar format: luminance component
                                                    */
    ptr_t busReconChromaUV;      /* Bus address for recon chrominance
                                                    * semiplanar format: both chrominance
                                                    */


    /*private buffers with recon frame*/
    ptr_t reconLuma_4n;          /* Bus address for 4n
                                                     */
    ptr_t compressTblReconLuma;          /* Bus address for compress table*/

    ptr_t compressTblReconChroma;          /* Bus address for compress table
                                                    */
    ptr_t colBufferH264Recon;        /* Bus address for colomn buffer of H264
                                                    */
    ptr_t cuInfoMemRecon;            /* Bus address for cu info memory
                                                    */
  }VCEncReconPara;

  typedef struct
  {
    //recon :    short or long term ref
    //ref list0 :    short or long term ref
    //ref list1 :    short or long term ref

    /** \brief Is picture an IDR picture? */
    unsigned int idr_pic_flag;
    /** \brief Is picture a reference picture? */
    unsigned int reference_pic_flag;

    /** \brief The picture identifier.
     *   Range: 0 to \f$2^{log2\_max\_frame\_num\_minus4 + 4} - 1\f$, inclusive.
     */
   // unsigned short  frame_num;



    /** \brief Slice type.
     *  Range: 0..2, 5..7, i.e. no switching slices.
     */
    unsigned char   slice_type;
    /** \brief Same as the H.264 bitstream syntax element. */
    /*unsigned char   pic_parameter_set_id;*/
    /** \brief Same as the H.264 bitstream syntax element. */
    unsigned short  idr_pic_id;

    /** \brief Specifies if
     * \ref _VAEncPictureParameterBufferH264::num_ref_idx_l0_active_minus1 or
     * \ref _VAEncPictureParameterBufferH264::num_ref_idx_l1_active_minus1 are
     * overriden by the values for this slice.
     */
    //unsigned char   num_ref_idx_active_override_flag;

    /** \brief Maximum reference index for reference picture list 0.
     *  Range: 0 to 31, inclusive.
     */
    unsigned char   num_ref_idx_l0_active_minus1;
    /** \brief Maximum reference index for reference picture list 1.
     *  Range: 0 to 31, inclusive.
     */
    unsigned char   num_ref_idx_l1_active_minus1;


    /** @name If pic_order_cnt_type == 0 */
    /**@{*/
    /** \brief The picture order count modulo MaxPicOrderCntLsb. */
    unsigned short  pic_order_cnt_lsb;
    /**@}*/

  }VCEncH264Para;


  typedef struct
  {
    //recon :    short or long term ref
    //ref list0 :    short or long term ref
    //ref list1 :    short or long term ref

    /** \brief Is picture an IDR picture? */
    unsigned int idr_pic_flag;
    /** \brief Is picture a reference picture? */
    unsigned int reference_pic_flag;

     /** \brief The picture identifier.
     *   Range: 0 to \f$2^{log2\_max\_frame\_num\_minus4 + 4} - 1\f$, inclusive.
     */
    //unsigned short  frame_num;



    /** \brief Slice type.
     *  Range: 0..2, 5..7, i.e. no switching slices.
     */
    unsigned char   slice_type;
    /** \brief Same as the H.264 bitstream syntax element. */
    /*unsigned char   pic_parameter_set_id;*/
    /** \brief Same as the H.264 bitstream syntax element. */
    unsigned short  idr_pic_id;

    /** \brief Maximum reference index for reference picture list 0.
     *  Range: 0 to 31, inclusive.
     */
    unsigned char   num_ref_idx_l0_active_minus1;
    /** \brief Maximum reference index for reference picture list 1.
     *  Range: 0 to 31, inclusive.
     */
    unsigned char   num_ref_idx_l1_active_minus1;


    /** @name If pic_order_cnt_type == 0 */
    /**@{*/
    /** \brief The picture order count modulo MaxPicOrderCntLsb. */
    unsigned short  pic_order_cnt_lsb;
    /**@}*/
    /*slice data*/
    unsigned char rps_neg_pic_num;
	unsigned char rps_pos_pic_num;
	signed int rps_delta_poc[VCENC_MAX_REF_FRAMES];
    unsigned char rps_used_by_cur[VCENC_MAX_REF_FRAMES];




  }VCEncHevcPara;



  /* Encoder  libva private input structure */
  typedef struct
  {
    VCEncReconPara recon;
    VCEncReconPara reflist0[2];
    VCEncReconPara reflist1[2];
    union {
        VCEncH264Para h264Para;
        VCEncHevcPara hevcPara;
    } params;

  } VCEncExtParaIn;


  /* Encoder output structure */
  typedef struct
  {
    VCEncPictureCodingType codingType;    /* Realized picture coding type,
                                                 * INTRA/PREDICTED/NOTCODED
                                                 */
    u32 streamSize;      /* Size of output stream in bytes */

    u32 *pNaluSizeBuf;   /* Output buffer for NAL unit sizes
                              * pNaluSizeBuf[0] = NALU 0 size in bytes
                              * pNaluSizeBuf[1] = NALU 1 size in bytes
                              * etc
                              * Zero value is written after last NALU.
                              */
    u32 numNalus;        /* Amount of NAL units */
    u32 maxSliceStreamSize;    /* max size of ouput slice in bytes*/

    ptr_t busScaledLuma;   /* Bus address for scaled encoder picture luma   */
    u8 *scaledPicture;   /* Pointer for scaled encoder picture            */

    VCEncCuOutData cuOutData; /* Pointer for cu information output by HW */

    u8 boolCurIsLongTermRef; /*Flag for current frame will be used as LongTermReference for future frame*/

    double ssim[3]; /* Y/Cb/Cr SSIM values calculated by HW. Valid only if SSIM calculation feature is supported by HW. */

    double psnr_y; /* PSNR_Y value, HW calcuate SSE, SW calculate log10f*/

    double psnr_y_predeb; /* PSNR_Y value (before deblock), HW calcuate SSE, SW calculate log10f*/
    double psnr_cb_predeb; /* PSNR_CB value (before deblock), HW calcuate SSE, SW calculate log10f*/
    double psnr_cr_predeb; /* PSNR_CR value (before deblock), HW calcuate SSE, SW calculate log10f*/

    u32 sliceHeaderSize;  /* Size of output slice header in multi-tile case */

    i32 indexEncoded; /* add for look ahead*/
    int64_t pts;
    int64_t dts;
    bool resendSPS; /* add for idr resend SPS... */
    u8 *header_buffer; /* store SPS header*/
    u32 header_size;  /* SPS header stored size */
  } VCEncOut;

  /* Input pre-processing */
  typedef struct
  {
    VCEncColorConversionType type;
    u16 coeffA;          /* User defined color conversion coefficient */
    u16 coeffB;          /* User defined color conversion coefficient */
    u16 coeffC;          /* User defined color conversion coefficient */
    u16 coeffE;          /* User defined color conversion coefficient */
    u16 coeffF;          /* User defined color conversion coefficient */
    u16 coeffG;          /* User defined color conversion coefficient */
    u16 coeffH;          /* User defined color conversion coefficient */
    u16 LumaOffset;      /* User defined color conversion coefficient */
  } VCEncColorConversion;

  typedef struct
  {
    u32 origWidth;                          /* Input camera picture width */
    u32 origHeight;                         /* Input camera picture height*/

    // for 1/4 resolution first pass, for disable rfc datapath, xOffset and yOffset should be 0, so not need ds params
    u32 origWidth_ds;
    u32 origHeight_ds;

    u32 xOffset;                            /* Horizontal offset          */
    u32 yOffset;                            /* Vertical offset            */
    VCEncPictureType inputType;           /* Input picture color format */
    VCEncPictureRotation rotation;        /* Input picture rotation     */
    VCEncPictureMirror mirror;            /* Input picture mirror     */
    VCEncColorConversion colorConversion; /* Define color conversion
                                                   parameters for RGB input   */
    u32 scaledWidth;    /* Optional down-scaled output picture width,
                              multiple of 4. 0=disabled. [16..width] */
    u32 scaledHeight;   /* Optional down-scaled output picture height,
                              multiple of 2. [96..height]                    */

    u32 scaledOutput;                     /* Enable output of down-scaled encoder picture.  */
    u32 scaledOutputFormat; /*0:YUV422   1:YUV420SP*/
    u32 inLoopDSRatio;
    u32 *virtualAddressScaledBuff;  /*virtual address of  allocated buffer in aplication for scaled picture.*/
    ptr_t busAddressScaledBuff; /*phyical address of  allocated buffer in aplication for scaled picture.*/
    u32 sizeScaledBuff;         /*size of allocated buffer in aplication for scaled picture.
                                                          the size is not less than scaledWidth*scaledOutput*2 bytes */
    /* constant chroma control */
    i32 constChromaEn;
    u32 constCb;
    u32 constCr;

    u32 input_alignment;

    i32 b_close_dummy_regs;  //for edma trans raw from rc to ep, it need to close the dummy regisiter.
  } VCEncPreProcessingCfg;

  typedef struct
  {
    i32 chroma_qp_offset;           /* chroma qp offset */
    i32 tc_Offset;                  /* deblock parameter, tc_offset */
    i32 beta_Offset;                /* deblock parameter, beta_offset */
  } VCEncPPSCfg;

  /* output stream buffers */
  typedef struct {
      u8 *buf[MAX_STRM_BUF_NUM];   /* Pointers to beginning of output stream buffer. */
      u32 bufLen[MAX_STRM_BUF_NUM]; /* Length of output stream buffer */
  } VCEncStrmBufs;

  /* Callback struct and function type. The callback is made by the encoder
   * when a slice is completed and available in the encoder stream output buffer. */
  typedef struct
  {
    u32 slicesReadyPrev;/* Indicates how many slices were completed at
                               previous callback. This is given because
                               several slices can be completed between
                               the callbacks. */
    u32 slicesReady;    /* Indicates how many slices are completed. */
    u32 nalUnitInfoNum;  /* Indicates how many information nal units are completed, including all kinds of sei information.*/
    u32 nalUnitInfoNumPrev; /* Indicates how many information nal units are completed at previous callback, including all kinds of sei information.*/
    u32 *sliceSizes;    /* Holds the size (bytes) of every completed slice. */
    VCEncStrmBufs streamBufs; /* output stream buffers */
    void *pAppData;     /* Pointer to application data. */
  } VCEncSliceReady;

  typedef void (*VCEncSliceReadyCallBackFunc)(VCEncSliceReady *sliceReady);

  /* Version information */
  typedef struct
  {
    u32 major;           /* Encoder API major version */
    u32 minor;           /* Encoder API minor version */
  } VCEncApiVersion;

  typedef struct
  {
    u32 swBuild;         /* Software build ID */
    u32 hwBuild;         /* Hardware build ID */
  } VCEncBuild;

  struct vcenc_buffer
  {
    struct vcenc_buffer *next;
    u8 *buffer;     /* Data store */
    u32 cnt;      /* Data byte cnt */
    ptr_t busaddr;      /* Data store bus address */
  };

  enum VCEncStatus
  {
    VCENCSTAT_INIT = 0xA1,
    VCENCSTAT_START_STREAM,
    VCENCSTAT_START_FRAME,
    VCENCSTAT_ERROR
  };

  /* used for RPS in slice header , begin*/
  typedef struct {
	  u8   u1short_term_ref_pic_set_sps_flag;
	  u8   u3NegPicNum;                  //  0...7, num_negative_pics ,3bits
	  u8   u3PosPicNum;                  //  0...7, num_positive_pics ,3bits
	  u8   u1DeltaPocS0Used[VCENC_MAX_REF_FRAMES];
	  u32  u20DeltaPocS0[VCENC_MAX_REF_FRAMES];    //  {20'bDeltaPoc}, 20bits
	  u8   u1DeltaPocS1Used[VCENC_MAX_REF_FRAMES];
	  u32  u20DeltaPocS1[VCENC_MAX_REF_FRAMES];    //  {20'bDeltaPoc}, 20bits
  }HWRPS_CONFIG;
  /* used for RPS in slice header , end*/

  /*------------------------------------------------------------------------------
      4. Encoder API function prototypes
  ------------------------------------------------------------------------------*/
  /* Version information */
  VCEncApiVersion VCEncGetApiVersion(void);
  VCEncBuild VCEncGetBuild(u32 core_id);
  u32 VCEncGetCoreIdByFormat(VCEncInst inst,u32 client_type);
  u32 VCEncGetAsicHWid(VCEncInst inst,u32 core_id);
  EWLHwConfig_t VCEncGetAsicConfig(VCEncInst inst,u32 core_id);
  //u32 VCEncGetRoiMapVersion(u32 core_id);

  /* Helper for input format bit-depths */
  u32 VCEncGetBitsPerPixel(VCEncPictureType type);

  u32 VCEncGetAlignedStride(int width, i32 input_format, u32 *luma_stride, u32 *chroma_stride,u32 input_alignment);

  /* Initialization & release */
  VCEncRet VCEncInit(const VCEncConfig *config, VCEncInst *instAddr, const void *ewl, const void *twoPassEwl);
  VCEncRet VCEncRelease(VCEncInst inst);

  /*to get cycle number of finishing encoding current whole frame*/
  u32 VCEncGetPerformance(VCEncInst inst);

  /* Encoder configuration before stream generation */
  VCEncRet VCEncSetCodingCtrl(VCEncInst instAddr, const VCEncCodingCtrl *pCodeParams);
  VCEncRet VCEncGetCodingCtrl(VCEncInst inst, VCEncCodingCtrl *pCodeParams);

  /* Encoder configuration before and during stream generation */
  VCEncRet VCEncSetRateCtrl(VCEncInst inst, const VCEncRateCtrl *pRateCtrl);
  VCEncRet VCEncGetRateCtrl(VCEncInst inst, VCEncRateCtrl *pRateCtrl);

  VCEncRet VCEncSetPreProcessing(VCEncInst inst, const VCEncPreProcessingCfg *pPreProcCfg);
  VCEncRet VCEncGetPreProcessing(VCEncInst inst, VCEncPreProcessingCfg *pPreProcCfg);

  /* Encoder user data insertion during stream generation */
  VCEncRet VCEncSetSeiUserData(VCEncInst inst, const u8 *pUserData, u32 userDataSize);

  /* Stream generation */

  /* VCEncStrmStart generates the SPS and PPS. SPS is the first NAL unit and PPS
   * is the second NAL unit. NaluSizeBuf indicates the size of NAL units.
   */
  VCEncRet VCEncStrmStart(VCEncInst inst, const VCEncIn *pEncIn, VCEncOut *pEncOut);

  /* VCEncStrmEncode encodes one video frame. If SEI messages are enabled the
   * first NAL unit is a SEI message. When MVC mode is selected first encoded
   * frame belongs to view=0 and second encoded frame belongs to view=1 and so on.
   * When MVC mode is selected a prefix NAL unit is generated before view=0 frames.
   */
  VCEncRet VCEncStrmEncode(VCEncInst inst, const VCEncIn *pEncIn,
                               VCEncOut *pEncOut,
                               VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                               void *pAppData);
  /* VCEncStrmEncode encodes one video frame. If SEI messages are enabled the
   * first NAL unit is a SEI message. When MVC mode is selected first encoded
   * frame belongs to view=0 and second encoded frame belongs to view=1 and so on.
   * When MVC mode is selected a prefix NAL unit is generated before view=0 frames.
   */

#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
i32 SetupPass2InputHwTransformer(VCEncInst inst, Pass2HWParam * privPass2HwParam,VCEncIn * encIn,const void * ewl);
i32 ReleasePass2InputHwTransformer(VCEncInst inst, Pass2HWParam *privPass2HwParam);
#endif

  VCEncRet VCEncStrmEncodeExt(VCEncInst inst, const VCEncIn *pEncIn,
                                const VCEncExtParaIn *pEncExtParaIn,
                               VCEncOut *pEncOut,
                               VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                               void *pAppData,i32 useExtFlag);


  /* VCEncStrmEnd ends a stream with an EOS code. */

  VCEncRet VCEncStrmEnd(VCEncInst inst, const VCEncIn *pEncIn, VCEncOut *pEncOut);

  /* VCEncFlush flushes out remaining frames on multi-core/lookahead encoding.
   * returns VCENC_FRAME_READY if one decoded frame has flushed out;
   * returns VCENC_OK if no frames remain */
  VCEncRet VCEncFlush(VCEncInst inst, const VCEncIn *pEncIn,
                             VCEncOut *pEncOut,
                             VCEncSliceReadyCallBackFunc sliceReadyCbFunc);

  /*  internal encoder testing */
  VCEncRet VCEncSetTestId(VCEncInst inst, u32 testId);

  /*PPS config*/
  VCEncRet VCEncCreateNewPPS(VCEncInst inst, const VCEncPPSCfg *pPPSCfg, i32* newPPSId);
  VCEncRet VCEncModifyOldPPS(VCEncInst inst, const VCEncPPSCfg *pPPSCfg, i32 ppsId);
  VCEncRet VCEncGetPPSData(VCEncInst inst,  VCEncPPSCfg *pPPSCfg, i32 ppsId);
  VCEncRet VCEncActiveAnotherPPS(VCEncInst inst, i32 ppsId);
  VCEncRet VCEncGetActivePPSId(VCEncInst inst,  i32* ppsId);

  /* low latency */
  /* Set valid input MB lines for encoder to work */
  VCEncRet VCEncSetInputMBLines(VCEncInst inst, u32 lines);

  /* Get encoded lines information from encoder */
  u32 VCEncGetEncodedMbLines(VCEncInst inst);

  /* Set error status for multi-pass */
  void VCEncSetError(VCEncInst inst);

  /* Set output buffer bus addr */
  void VCEncSetOutBusAddr(VCEncInst inst, EWLLinearMem_t *outbuf);

  /*------------------------------------------------------------------------------
       API Functions only valid for HEVC.
   ------------------------------------------------------------------------------*/
   /* Get the encoding information of a specified CU in a specified CTU */
   VCEncRet VCEncGetCuInfo(VCEncInst inst, VCEncCuOutData *pEncCuOutData,
                                      VCEncCuInfo *pEncCuInfo, u32 ctuNum, u32 cuNum);

   VCEncPictureCodingType VCEncFindNextPic (VCEncInst inst, VCEncIn *encIn, i32 nextGopSize, const u8 *gopCfgOffset, bool forceIDR);
   /*------------------------------------------------------------------------------
      5. Encoder API tracing callback function
  ------------------------------------------------------------------------------*/

  void VCEncTrace(const char *msg);
  void VCEncTraceProfile(VCEncInst inst);

#ifdef __cplusplus
}
#endif

#endif
