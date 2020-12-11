/* Copyright 2014 Google Inc. All Rights Reserved. */

#ifndef __ENCAPICOMMON_H__
#define __ENCAPICOMMON_H__

#include "basetype.h"
#include "cwl.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Picture YUV type for pre-processing */
    typedef enum {
      ENC_YUV420_PLANAR,              /* YYYY... UUUU... VVVV */
      ENC_YUV420_SEMIPLANAR,          /* YYYY... UVUVUV...    */
      ENC_YUV420_SEMIPLANAR_VU,       /* YYYY... UVUVUV...    */
      ENC_YUV420_SEMIPLANAR_TILED4x4, /* 4x4 tile-ordered
                                         ENC_YUV420_SEMIPLANAR */
      ENC_YUV420_SEMIPLANAR_TILED8x4, /* 8x4 tile-ordered
                                         ENC_YUV420_SEMIPLANAR */
      ENC_YUV422_INTERLEAVED_YUYV,    /* YUYVYUYV...          */
      ENC_YUV422_INTERLEAVED_UYVY,    /* UYVYUYVY...          */
      ENC_RGBA,                       /* 32-bit RGBA 32bpp    */
      ENC_BGRA,                       /* 32-bit RGBA 32bpp    */
      ENC_YUV420_PLANAR_10BE,
      ENC_YUV420_PLANAR_4x4,          // 4x4 tile-ordered PLANAR
      ENC_YUV420_PLANAR_4x4_8b = ENC_YUV420_PLANAR_4x4, // alias
      ENC_YUV420_PLANAR_4x4_10b,      // BigSea transcode format
      ENC_YUV420_PLANAR_4x4_16b,      // BigSea transcode format
      ENC_RGB10,                      // A2B10G10R10
      ENC_BGR10,                      // A2R10G10B10
      ENC_YUV420_SEMIPLANAR_P010,     /* YYYY... UVUVUV...    */ 
      ENC_YUV420_SEMIPLANAR_P010_TILED4x4,     /* YYYY... UVUVUV...    */
      ENC_PICTURETYPE_COUNT
    } EncPictureType;

/* Picture FLIP for pre-processing */
    typedef enum {
      ENC_FLIP_NONE = 0,
      ENC_FLIP_VERT = 1
    } EncFlipType;

/* Picture rotation for pre-processing */
    typedef enum
    {
        ENC_ROTATE_0 = 0,
        ENC_ROTATE_90R = 1, /* Rotate 90 degrees clockwise           */
        ENC_ROTATE_90L = 2, /* Rotate 90 degrees counter-clockwise   */
        ENC_ROTATE_180 = 3  /* Rotate 180 degrees                    */
    } EncPictureRotation;

/* Picture color space conversion (RGB input) for pre-processing */
    typedef enum
    {
        ENC_RGBTOYUV_BT601_SS = 0, /* Color conversion according to BT.601, studio swing  */
        ENC_RGBTOYUV_BT709_SS = 1, /* Color conversion according to BT.709, studio swing  */
        ENC_RGBTOYUV_BT601_FS = 2, /* Color conversion according to BT.601, full swing    */
        ENC_RGBTOYUV_BT709_FS = 3, /* Color conversion according to BT.709, full swing    */
    } EncColorConversionType;

    typedef enum {
      ENC_COLORRANGE_UNCHANGED = 0,
      ENC_COLORRANGE_STUDIO_TO_FULL = 1,
      ENC_COLORRANGE_FULL_TO_STUDIO = 2
    } EncColorRangeConversion;

    typedef struct
    {
        u32 hw_run_type;     /* Type of the hardware operation
                              * 0 = I/P/PN frame encode
                              * 1 = first pass encode
                              * 2 = temporal filter
                              * 3 = re-code for finding optimal loop filter
                              * value                                         */
        u32 used_hw_cycles;  /* Number of clock cycles used by the hardware.  */
        u32 dram_reads;      // Number of words read from DRAM
        u32 dram_writes;     // Number of words written to DRAM
    } HwPerfCounter;

    typedef struct {
        u64 busLuma;     // Bus address for writing scaled picture luminance
        u64 busChroma;   // Bus address for writing scaled picture chrominance
        int packed_8bpp; // HW drops 2 LSB from the internal 10bpp accuracy
    } EncScaledPicture;

    typedef struct
    {
        u32 origWidth;                              /* Input camera picture width, multiple of 32 bytes:
                                                       32 pixels for YUV420 planar and semi-planar
                                                       16 pixels for YUV422 interleaved
                                                       8  pixels for RGB, tiled 8x4 and tiled 4x4 */
        u32 origHeight;                             /* Input camera picture height. Tiled 8x4 and 4x4 multiple 8. */
        u32 xOffset;                                /* Horizontal offset          */
        u32 yOffset;                                /* Vertical offset            */
        u32 cropWidth;                              /* Picture width after cropping. Will be scaled down to the final encoded width. */
        u32 cropHeight;                             /* Picture height after cropping. Will be scaled down to the final encoded height. */
        EncPictureType inputType;                   /* Input picture color format */
        EncPictureRotation rotation;                /* Input picture rotation     */
        EncFlipType flipType;                       /* Input picture flipping     */
        EncColorConversionType colorConversion;     /* Define color conversion parameters for RGB input   */
        EncColorRangeConversion colorRangeConversion; /* Enable studio<->full color range conversion */
        u32 letterboxLeft;                          /* Letterbox area in the left. */
        u32 letterboxRight;                         /* Letterbox area in the right. */
        u32 letterboxTop;                           /* Letterbox area in the top. */
        u32 letterboxBottom;                        /* Letterbox area in the bottom. */
        u32 letterboxYColor;                        /* Letterbox Y area color. */
        u32 letterboxUColor;                        /* Letterbox U area color. */
        u32 letterboxVColor;                        /* Letterbox V area color. */
        u32 pp_only;                                /* Preprocessor only mode. */
    } EncPreProcessingCfg;

#ifdef __cplusplus
}
#endif

#endif /*__ENCAPICOMMON_H__*/
