/*------------------------------------------------------------------------------
          if (!ppu_cfg->monochrome && !mono_chrome)
--Copyright (c) 2015-2017, VeriSilicon Holdings Co., Ltd. All rights reserved --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#ifndef DECAPICOMMON_H
#define DECAPICOMMON_H

#include "basetype.h"

/** Maximum number of cores supported in multi-core configuration */
/** For G2, multi-core currently not supported. */
#define MAX_ASIC_CORES 4

#define HEVC_NOT_SUPPORTED (u32)(0x00)
#define HEVC_MAIN_PROFILE (u32)(0x01)
#define HEVC_MAIN10_PROFILE (u32)(0x02)
#define HEVC_SUPPORTED (u32)(0x01)
#define VP9_NOT_SUPPORTED (u32)(0x00)
#define VP9_PROFILE0 (u32)(0x01)
#define VP9_PROFILE2_10BITS (u32)(0x02)
#define MPEG4_NOT_SUPPORTED (u32)(0x00)
#define MPEG4_SIMPLE_PROFILE (u32)(0x01)
#define MPEG4_ADVANCED_SIMPLE_PROFILE (u32)(0x02)
#define MPEG4_CUSTOM_NOT_SUPPORTED (u32)(0x00)
#define MPEG4_CUSTOM_FEATURE_1 (u32)(0x01)
#define H264_NOT_SUPPORTED (u32)(0x00)
#define H264_BASELINE_PROFILE (u32)(0x01)
#define H264_MAIN_PROFILE (u32)(0x02)
#define H264_HIGH_PROFILE (u32)(0x03)
#define VC1_NOT_SUPPORTED (u32)(0x00)
#define VC1_SIMPLE_PROFILE (u32)(0x01)
#define VC1_MAIN_PROFILE (u32)(0x02)
#define VC1_ADVANCED_PROFILE (u32)(0x03)
#define MPEG2_NOT_SUPPORTED (u32)(0x00)
#define MPEG2_MAIN_PROFILE (u32)(0x01)
#define JPEG_NOT_SUPPORTED (u32)(0x00)
#define JPEG_BASELINE (u32)(0x01)
#define JPEG_PROGRESSIVE (u32)(0x02)
#define PP_NOT_SUPPORTED (u32)(0x00)
#define PP_SUPPORTED (u32)(0x01)
#define PP_TILED_4X4 (u32)(0x20000000)
#define PP_DITHERING (u32)(0x10000000)
#define PP_SCALING (u32)(0x0C000000)
#define PP_DEINTERLACING (u32)(0x02000000)
#define PP_ALPHA_BLENDING (u32)(0x01000000)
#define PP_OUTP_ENDIAN (u32)(0x00040000)
#define PP_TILED_INPUT (u32)(0x0000C000)
#define PP_PIX_ACC_OUTPUT (u32)(0x40000000)
#define PP_ABLEND_CROP (u32)(0x80000000)
#define SORENSON_SPARK_NOT_SUPPORTED (u32)(0x00)
#define SORENSON_SPARK_SUPPORTED (u32)(0x01)
#define VP6_NOT_SUPPORTED (u32)(0x00)
#define VP6_SUPPORTED (u32)(0x01)
#define VP7_NOT_SUPPORTED (u32)(0x00)
#define VP7_SUPPORTED (u32)(0x01)
#define VP8_NOT_SUPPORTED (u32)(0x00)
#define VP8_SUPPORTED (u32)(0x01)
#define REF_BUF_NOT_SUPPORTED (u32)(0x00)
#define REF_BUF_SUPPORTED (u32)(0x01)
#define REF_BUF_INTERLACED (u32)(0x02)
#define REF_BUF_DOUBLE (u32)(0x04)
#define TILED_NOT_SUPPORTED (u32)(0x00)
#define TILED_8x4_SUPPORTED (u32)(0x01)
#define AVS_NOT_SUPPORTED (u32)(0x00)
#define AVS_SUPPORTED (u32)(0x01)
#define JPEG_EXT_NOT_SUPPORTED (u32)(0x00)
#define JPEG_EXT_SUPPORTED (u32)(0x01)
#define RV_NOT_SUPPORTED (u32)(0x00)
#define RV_SUPPORTED (u32)(0x01)
#define MVC_NOT_SUPPORTED (u32)(0x00)
#define MVC_SUPPORTED (u32)(0x01)
#define WEBP_NOT_SUPPORTED (u32)(0x00)
#define WEBP_SUPPORTED (u32)(0x01)
#define EC_NOT_SUPPORTED (u32)(0x00)
#define EC_SUPPORTED (u32)(0x01)
#define STRIDE_NOT_SUPPORTED (u32)(0x00)
#define STRIDE_SUPPORTED (u32)(0x01)
#define DOUBLE_BUFFER_NOT_SUPPORTED (u32)(0x00)
#define DOUBLE_BUFFER_SUPPORTED (u32)(0x01)
#define FIELD_DPB_NOT_SUPPORTED (u32)(0x00)
#define FIELD_DPB_SUPPORTED (u32)(0x01)
#define AVS_PLUS_NOT_SUPPORTED (u32)(0x00)
#define AVS_PLUS_SUPPORTED  (u32)(0x01)
#define ADDR64_ENV_NOT_SUPPORTED (u32)(0x00)
#define ADDR64_ENV_SUPPORTED (u32)(0x01)

#define H264_NOT_SUPPORTED_FUSE (u32)(0x00)
#define H264_FUSE_ENABLED (u32)(0x01)
#define MPEG4_NOT_SUPPORTED_FUSE (u32)(0x00)
#define MPEG4_FUSE_ENABLED (u32)(0x01)
#define MPEG2_NOT_SUPPORTED_FUSE (u32)(0x00)
#define MPEG2_FUSE_ENABLED (u32)(0x01)
#define SORENSON_SPARK_NOT_SUPPORTED_FUSE (u32)(0x00)
#define SORENSON_SPARK_ENABLED (u32)(0x01)
#define JPEG_NOT_SUPPORTED_FUSE (u32)(0x00)
#define JPEG_FUSE_ENABLED (u32)(0x01)
#define VP6_NOT_SUPPORTED_FUSE (u32)(0x00)
#define VP6_FUSE_ENABLED (u32)(0x01)
#define VP7_NOT_SUPPORTED_FUSE (u32)(0x00)
#define VP7_FUSE_ENABLED (u32)(0x01)
#define VP8_NOT_SUPPORTED_FUSE (u32)(0x00)
#define VP8_FUSE_ENABLED (u32)(0x01)
#define VC1_NOT_SUPPORTED_FUSE (u32)(0x00)
#define VC1_FUSE_ENABLED (u32)(0x01)
#define JPEG_PROGRESSIVE_NOT_SUPPORTED_FUSE (u32)(0x00)
#define JPEG_PROGRESSIVE_FUSE_ENABLED (u32)(0x01)
#define REF_BUF_NOT_SUPPORTED_FUSE (u32)(0x00)
#define REF_BUF_FUSE_ENABLED (u32)(0x01)
#define AVS_NOT_SUPPORTED_FUSE (u32)(0x00)
#define AVS_FUSE_ENABLED (u32)(0x01)
#define RV_NOT_SUPPORTED_FUSE (u32)(0x00)
#define RV_FUSE_ENABLED (u32)(0x01)
#define MVC_NOT_SUPPORTED_FUSE (u32)(0x00)
#define MVC_FUSE_ENABLED (u32)(0x01)

#define PP_NOT_SUPPORTED_FUSE (u32)(0x00)
#define PP_FUSE_ENABLED (u32)(0x01)
#define PP_FUSE_DEINTERLACING_ENABLED (u32)(0x40000000)
#define PP_FUSE_ALPHA_BLENDING_ENABLED (u32)(0x20000000)
#define MAX_PP_OUT_WIDHT_1920_FUSE_ENABLED (u32)(0x00008000)
#define MAX_PP_OUT_WIDHT_1280_FUSE_ENABLED (u32)(0x00004000)
#define MAX_PP_OUT_WIDHT_720_FUSE_ENABLED (u32)(0x00002000)
#define MAX_PP_OUT_WIDHT_352_FUSE_ENABLED (u32)(0x00001000)

/* Maximum output channels from xxxDecNexPicture() */
#define DEC_MAX_OUT_COUNT 5

/* Picture dimensions are checked
   currently in vp9 and hecv api code */
#if !defined(MODEL_SIMULATION) || defined(HW_PIC_DIMENSIONS)
#define MIN_PIC_WIDTH 144
#define MIN_PIC_HEIGHT 144

#define MIN_PIC_WIDTH_VP9 48
#define MIN_PIC_HEIGHT_VP9 48

/* DTRC minimum size = 96x8, So the minimum size for vp9 decoding
   should be redefined if DTRC is enabled */
#define MIN_PIC_WIDTH_VP9_EN_DTRC 96
#define MIN_PIC_HEIGHT_VP9_EN_DTRC 72

#else /* MODEL_SIMULATION */
#define MIN_PIC_WIDTH 8
#define MIN_PIC_HEIGHT 8

#define MIN_PIC_WIDTH_VP9 8
#define MIN_PIC_HEIGHT_VP9 8

#define MIN_PIC_WIDTH_VP9_EN_DTRC 8
#define MIN_PIC_HEIGHT_VP9_EN_DTRC 8

#endif /* MODEL_SIMULATION */

struct DecHwConfig {
  u32 mpeg4_support;        /* one of the MPEG4 values defined above */
  u32 custom_mpeg4_support; /* one of the MPEG4 custom values defined above */
  u32 h264_support;         /* one of the H264 values defined above */
  u32 vc1_support;          /* one of the VC1 values defined above */
  u32 mpeg2_support;        /* one of the MPEG2 values defined above */
  u32 jpeg_support;         /* one of the JPEG values defined above */
  u32 jpeg_prog_support;  /* one of the Progressive JPEG values defined above */
  u32 max_dec_pic_width;  /* maximum picture width in decoder */
  u32 max_dec_pic_height; /* maximum picture height in decoder */
  u32 pp_support;         /* PP_SUPPORTED or PP_NOT_SUPPORTED */
  u32 pp_config;          /* Bitwise list of PP function */
  u32 max_pp_out_pic_width;   /* maximum post-processor output picture width */
  u32 sorenson_spark_support; /* one of the SORENSON_SPARK values defined above
                                 */
  u32 ref_buf_support;       /* one of the REF_BUF values defined above */
  u32 tiled_mode_support;    /* one of the TILED values defined above */
  u32 vp6_support;           /* one of the VP6 values defined above */
  u32 vp7_support;           /* one of the VP7 values defined above */
  u32 vp8_support;           /* one of the VP8 values defined above */
  u32 vp9_support;           /* HW supports VP9 */
  u32 avs_support;           /* one of the AVS values defined above */
  u32 jpeg_esupport;         /* one of the JPEG EXT values defined above */
  u32 rv_support;            /* one of the HUKKA values defined above */
  u32 mvc_support;           /* one of the MVC values defined above */
  u32 webp_support;          /* one of the WEBP values defined above */
  u32 ec_support;            /* one of the EC values defined above */
  u32 stride_support;        /* HW supports separate Y and C strides */
  u32 field_dpb_support;     /* HW supports field-mode DPB */
  u32 avs_plus_support;      /* one of the AVS PLUS values defined above */
  u32 addr64_support;         /* HW supports 64bit addressing */
  u32 hevc_support;          /* HW supports HEVC */
  u32 double_buffer_support; /* Decoder internal reference double buffering */

  u32 hevc_main10_support;  /* HW supports HEVC Main10 profile*/
  u32 vp9_10bit_support;     /* HW supports VP9 10 bits profile */
  u32 ds_support;            /* HW supports down scaling. */
  u32 rfc_support;           /* HW supports reference frame compression. */
  u32 ring_buffer_support;   /* HW supports ring buffer. */

  u32 fmt_p010_support;      /* HW supports P010 format. */
  u32 fmt_customer1_support; /* HW supports special customized format */
  u32 mrb_prefetch;          /* HW supports Multi-Reference Blocks Prefetch */
};

struct DecSwHwBuild {
  u32 sw_build;                 /* Software build ID */
  u32 hw_build;                 /* Hardware build ID */
  struct DecHwConfig hw_config[MAX_ASIC_CORES]; /* Hardware configuration */
};

/* DPB flags to control reference picture format etc. */
enum DecDpbFlags {
  /* Reference frame formats */
  DEC_REF_FRM_RASTER_SCAN = 0x0,
  DEC_REF_FRM_TILED_DEFAULT = 0x1,

  /* Flag to allow SW to use DPB field ordering on interlaced content */
  DEC_DPB_ALLOW_FIELD_ORDERING = 0x40000000
};

#define DEC_REF_FRM_FMT_MASK 0x01
#define LOW_LATENCY_PACKET_SIZE 256
/* Modes for storing content into DPB */
enum DecDpbMode {
  DEC_DPB_FRAME = 0,
  DEC_DPB_INTERLACED_FIELD = 1
};

/* Decoder working mode */
enum DecDecoderMode {
  DEC_NORMAL = 0,
  DEC_LOW_LATENCY = 1,
  DEC_LOW_LATENCY_RTL = 2,
  DEC_SECURITY = 3
};

/* DEPRECATED!!! do not use in new applications! */
#define DEC_DPB_DEFAULT DEC_DPB_FRAME

/* Output picture format types */
enum DecPictureFormat {
  DEC_OUT_FRM_TILED_4X4 = 0,
  DEC_OUT_FRM_TILED_8X4 = 1,
  DEC_OUT_FRM_RASTER_SCAN = 2, /* a.k.a. SEMIPLANAR_420 */
  DEC_OUT_FRM_PLANAR_420 = 3,
  DEC_OUT_FRM_MONOCHROME,       /* a.k.a. YUV400 */
  /* YUV420 */
  DEC_OUT_FRM_YUV420TILE,        /* YUV420, 8-bit, Tile4x4 */
  DEC_OUT_FRM_YUV420TILE_PACKED, /* Reference frame format */
  DEC_OUT_FRM_YUV420TILE_P010,
  DEC_OUT_FRM_YUV420SP,          /* YUV420, 8-bit, semi-planar */
  DEC_OUT_FRM_YUV420SP_PACKED,
  DEC_OUT_FRM_YUV420SP_P010,
  DEC_OUT_FRM_YUV420P,            /* YUV420, 8-bit, planar */
  DEC_OUT_FRM_YUV420P_PACKED,
  DEC_OUT_FRM_YUV420P_P010,
   /* YUV400 */
  DEC_OUT_FRM_YUV400TILE,        /* YUV420, 8-bit, Tile4x4 */
  DEC_OUT_FRM_YUV400TILE_P010,
  DEC_OUT_FRM_YUV400,            /* YUV420, 8-bit, planar */
  DEC_OUT_FRM_YUV400_P010,
  /* NV12 */
  DEC_OUT_FRM_NV21TILE,        /* YUV420, 8-bit, Tile4x4 */
  DEC_OUT_FRM_NV21TILE_PACKED, /* Reference frame format */
  DEC_OUT_FRM_NV21TILE_P010,
  DEC_OUT_FRM_NV21SP,          /* YUV420, 8-bit, semi-planar */
  DEC_OUT_FRM_NV21SP_PACKED,
  DEC_OUT_FRM_NV21SP_P010,
  DEC_OUT_FRM_NV21P,            /* YUV420, 8-bit, planar */
  DEC_OUT_FRM_NV21P_PACKED,
  DEC_OUT_FRM_NV21P_P010,
};


/* Output picture pixel format types for raster scan or down scale output */
enum DecPicturePixelFormat {
  DEC_OUT_PIXEL_DEFAULT = 0,    /* packed pixel: each pixel in at most 10 bits as reference buffer */
  DEC_OUT_PIXEL_P010 = 1,       /* a.k.a. MS P010 format */
  DEC_OUT_PIXEL_CUSTOMER1 = 2,  /* customer format: a 128-bit burst output in packed little endian format */
  DEC_OUT_PIXEL_CUT_8BIT = 3,   /* cut 10 bit to 8 bit per pixel */
  DEC_OUT_PIXEL_RFC = 4         /* compressed tiled output */
};

/* error handling */
enum DecErrorHandling {
  DEC_EC_PIC_COPY_REF = 0x1,
  DEC_EC_PIC_PARTIAL = 0x2,
  DEC_EC_PIC_PARTIAL_IGNORE = 0x4,
  DEC_EC_REF_REPLACE = 0x100,
  DEC_EC_REF_NEXT_IDR = 0x200,
  DEC_EC_REF_NEXT_I = 0x400,
  DEC_EC_OUT_ALL = 0x10000,
  DEC_EC_OUT_NO_ERROR = 0x20000,
  DEC_EC_OUT_FIRST_FIELD_OK = 0x40000,
  DEC_EC_PICTURE_FREEZE = DEC_EC_PIC_COPY_REF,
  DEC_EC_VIDEO_FREEZE = (DEC_EC_PIC_COPY_REF | DEC_EC_REF_NEXT_IDR),
  DEC_EC_PARTIAL_FREEZE = DEC_EC_PIC_PARTIAL,
  DEC_EC_PARTIAL_IGNORE = DEC_EC_PIC_PARTIAL_IGNORE,
  DEC_EC_FAST_FREEZE = (DEC_EC_PIC_PARTIAL_IGNORE | DEC_EC_REF_NEXT_I | DEC_EC_OUT_NO_ERROR)
};

struct DecCropCfg {
  u32 crop_x;
  u32 crop_y;
  u32 crop_w;
  u32 crop_h;
  u32 crop_enabled;
};

enum SCALE_MODE {
  NON_SCALE,
  FIXED_DOWNSCALE,
  FLEXIBLE_SCALE
} ;

struct DecFixedScaleCfg {
  u32 down_scale_x;
  u32 down_scale_y;
  u32 fixed_scale_enabled;
};

/* Stride alignment: aligned to 8/16/.../512 bytes */
typedef enum {
  DEC_ALIGN_1B = 0,
  DEC_ALIGN_8B = 3,
  DEC_ALIGN_16B,
  DEC_ALIGN_32B,
  DEC_ALIGN_64B,
  DEC_ALIGN_128B,
  DEC_ALIGN_256B,
  DEC_ALIGN_512B,
  DEC_ALIGN_1024B,
  DEC_ALIGN_2048B,
} DecPicAlignment;

typedef struct _PpUnitConfig {
  u32 enabled;    /* PP unit enabled */
  u32 tiled_e;    /* PP unit tiled4x4 output enabled */
  u32 cr_first;   /* CrCb instead of CbCr */
  u32 shaper_enabled;
  u32 planar;     /* Planar output */
  DecPicAlignment align;  /* pp output alignment */
  /* Stride for Y/C plane. SW should use the stride calculated from SW if it's
     set to 0. When not 0, SW should check the validation of the value. */
  u32 ystride;
  u32 cstride;
  struct {
    u32 enabled;  /* whether cropping is enabled */
    u32 x;        /* cropping start x */
    u32 y;        /* cropping start y */
    u32 width;    /* cropping width */
    u32 height;   /* cropping height */
  } crop;
  struct {
    u32 enabled;  /* whether scaling is enabled */
    u32 width;    /* scaled output width */
    u32 height;   /* scaled output height */
  } scale;
  u32 monochrome; /* PP output monochrome (luma only) */
  u32 out_p010;
  u32 out_cut_8bits;
  u32 out_be;     /* Output in big-endian */
  u32 out_format;
} PpUnitConfig;
#endif /* DECAPICOMMON_H */
