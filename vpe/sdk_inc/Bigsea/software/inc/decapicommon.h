/* Copyright 2012 Google Inc. All Rights Reserved. */

#ifndef DECAPICOMMON_H
#define DECAPICOMMON_H

#include "basetype.h"
#include "cwl.h"


#define EC_NOT_SUPPORTED (u32)(0x00)
#define EC_SUPPORTED (u32)(0x01)
#define STRIDE_NOT_SUPPORTED (u32)(0x00)
#define STRIDE_SUPPORTED (u32)(0x01)
#define DOUBLE_BUFFER_NOT_SUPPORTED (u32)(0x00)
#define DOUBLE_BUFFER_SUPPORTED (u32)(0x01)

#define PP_NOT_SUPPORTED_FUSE (u32)(0x00)
#define PP_FUSE_ENABLED (u32)(0x01)
#define PP_FUSE_DEINTERLACING_ENABLED (u32)(0x40000000)
#define PP_FUSE_ALPHA_BLENDING_ENABLED (u32)(0x20000000)
#define MAX_PP_OUT_WIDHT_1920_FUSE_ENABLED (u32)(0x00008000)
#define MAX_PP_OUT_WIDHT_1280_FUSE_ENABLED (u32)(0x00004000)
#define MAX_PP_OUT_WIDHT_720_FUSE_ENABLED (u32)(0x00002000)
#define MAX_PP_OUT_WIDHT_352_FUSE_ENABLED (u32)(0x00001000)

#define MIN_PIC_WIDTH 1
#define MIN_PIC_HEIGHT 1

struct DecHwConfig {
  u32 ec_support;            /* one of the EC values defined above */
  u32 stride_support;        /* HW supports separate Y and C strides */
  u32 double_buffer_support; /* Decoder internal reference double buffering */
};

struct DecSwHwBuild {
  u32 sw_build;                 /* Software build ID */
  u32 hw_build;                 /* Hardware build ID */
  struct CWLHwConfig hw_config; /* Hardware configuration */
};

/* DPB flags to control reference picture format etc. */
enum DecDpbFlags {
  /* Reference frame formats */
  DEC_REF_FRM_RASTER_SCAN = 0x0,
  DEC_REF_FRM_TILED_DEFAULT = 0x1
};

#define DEC_REF_FRM_FMT_MASK 0x01

/* Output picture format types */
enum DecPictureFormat {
  DEC_OUT_FRM_TILED_4X4 = 0,
  DEC_OUT_FRM_RASTER_SCAN = 1, /* a.k.a. SEMIPLANAR_420 */
  DEC_OUT_FRM_PLANAR_420 = 2
};

/* error handling */
enum DecErrorHandling {
  DEC_EC_PICTURE_FREEZE = 0,
  DEC_EC_VIDEO_FREEZE = 1,
  DEC_EC_PARTIAL_FREEZE = 2,
  DEC_EC_PARTIAL_IGNORE = 3
};

#endif /* DECAPICOMMON_H */
