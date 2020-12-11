/* Copyright 2014 Google Inc. All Rights Reserved. */

/*------------------------------------------------------------------------------
--
--  Abstract : PP Encoder API
--
------------------------------------------------------------------------------*/

#ifndef __PPENCAPI_H__
#define __PPENCAPI_H__

#include "basetype.h"
#include "encapicommon.h"
#include "scaler_common.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_LAG_IN_FRAMES     28

/*------------------------------------------------------------------------------
    1. Type definition for encoder instance
------------------------------------------------------------------------------*/

    typedef const void *PPEncInst;

/*------------------------------------------------------------------------------
    2. Enumerations for API parameters
------------------------------------------------------------------------------*/

/* Function return values */
    typedef enum
    {
        PPENC_OK = 0,
        PPENC_FRAME_READY = 1,
        PPENC_FRAME_READY_MORE_AVAILABLE = 2,
        PPENC_LAG_FIRST_PASS = 3,
        PPENC_LAG = 4,

        PPENC_ERROR = -1,
        PPENC_NULL_ARGUMENT = -2,
        PPENC_INVALID_ARGUMENT = -3,
        PPENC_MEMORY_ERROR = -4,
        PPENC_CWL_ERROR = -5,
        PPENC_CWL_MEMORY_ERROR = -6,
        PPENC_INVALID_STATUS = -7,
        PPENC_OUTPUT_BUFFER_OVERFLOW = -8,
        PPENC_HW_BUS_ERROR = -9,
        PPENC_HW_DATA_ERROR = -10,
        PPENC_HW_TIMEOUT = -11,
        PPENC_HW_RESERVED = -12,
        PPENC_SYSTEM_ERROR = -13,
        PPENC_INSTANCE_ERROR = -14,
        PPENC_HRD_ERROR = -15,
        PPENC_HW_RESET = -16,
        PPENC_FIRSTPASS_FAILED = -17
    } PPEncRet;

/* Picture bit-depth */
    typedef enum {
      PPENC_8BIT     = 0,
      PPENC_10BIT    = 1
    } PPEncBitDepth;

/* Picture type for encoding */
    typedef enum
    {
        PPENC_INTRA_FRAME = 0,
        PPENC_PREDICTED_FRAME = 1,
        PPENC_NOTCODED_FRAME         /* Used just as a return value  */
    } PPEncPictureCodingType;

/* Enumerations for adaptive parameters */
    enum {
        PPENC_FILTER_LEVEL_AUTO = 64,
        PPENC_FILTER_SHARPNESS_AUTO = 8,
        PPENC_QPDELTA_AUTO = 16
    };

/* Enumerations for quality metric optimization */
    typedef enum
    {
        PPENC_QM_PSNR  = 0,
        PPENC_QM_SSIM  = 1
    } PPEncQualityMetric;

/* Enumerations for bandwidth mode */
    typedef enum
    {
        PPENC_BWM_LO  = 0,
        PPENC_BWM_MID = 1,
        PPENC_BWM_HI  = 2
    } PPEncBandwidthMode;

/* Enumeration for trace levels. */
    typedef enum {
        PPENC_TRACE_ERROR = 0,
        PPENC_TRACE_WARNING = 1,
        PPENC_TRACE_INFO = 2,
        PPENC_TRACE_VERBOSE = 3
    } PPEncTraceLevel;

/*------------------------------------------------------------------------------
    3. User defined functions.
------------------------------------------------------------------------------*/

    typedef void (*PPEncTrace)(void *p, PPEncTraceLevel l, const char *fmt,
                                va_list args);

/*------------------------------------------------------------------------------
    4. Structures for API function parameters
------------------------------------------------------------------------------*/

/* Configuration info for initialization
 * Width and height are picture dimensions after rotation
 */
    typedef struct
    {
        u32 width;           /* Encoded picture width in pixels              */
        u32 height;          /* Encoded picture height in pixels             */
        PPEncBitDepth bitdepth;  /* Encoded picture bit-depth               */
        u32 frameRateNum;    /* The stream time scale, [1..1048575]          */
        u32 frameRateDenom;  /* Frame rate shall be frameRateNum/frameRateDenom
                              * in frames/second. [1..frameRateNum]          */
        u32 frameComp;       /* Enable reference frame compression           */
        u32 lookaheadCount;  /* Number of frames to look ahead. */
        u32 passes;          // Number of encoding passes (1/2)
        u32 enable_multicore; /* Set if each tile is encoded by one HW instance */
        void *trace_ctx;      /* User defined ptr for trace callbacks. */
        PPEncTrace trace;    /* User defined trace function. */
    } PPEncConfig;

/* Encoder input structure */
    typedef struct
    {
        size_t busLuma;      /* Bus address for input picture
                              * planar format: luminance component
                              * semiplanar format: luminance component
                              * interleaved format: whole picture
                              */
        size_t busChromaU;   /* Bus address for input chrominance
                              * planar format: cb component
                              * semiplanar format: both chrominance
                              * interleaved format: not used
                              */
        size_t busChromaV;   /* Bus address for input chrominance
                              * planar format: cr component
                              * semiplanar format: not used
                              * interleaved format: not used
                              */

        size_t busOutBuf;     /* Bus address of output stream buffer         */
        u32 outBufSize;      /* Size of output stream buffer in bytes       */

        PPEncPictureCodingType codingType; /* Proposed picture coding type,
                                             * INTRA/PREDICTED              */
        u32 timeIncrement;   /* The previous picture duration in units
                              * of 1/frameRateNum. 0 for the very first
                              * picture and typically equal to frameRateDenom
                              * for the rest.                               */
        u32 layerId;         /* When temporal layers are enabled by
                              * layerBitPerSecond, the layerId defines the
                              * layer this picture belongs to. [0..3]       */
        u32 refresh_frame_flags; /* 8-bit field to select which ones of the
                                  * reference frame buffers are refreshed.
                                  * Only applicable if goldenPictureRate==0 */
        u32 active_ref_idx[3];   /* Set which reference frame buffers are
                                  * used as reference. The motion estimation
                                  * is done for index 0 buffer and zero motion
                                  * is checked from index 1 buffer. Only
                                  * applicable if goldenPictureRate==0.     */
        u32 end_of_sequence;  /* Used when in latency-mode. */
        u32 write_stats;
        EncScaledPicture scaledPicture;
    } PPEncIn;

/* Encoder output structure */
    typedef struct
    {
        PPEncPictureCodingType codingType; /* Realized picture coding type   */
        u32 frameSize;       /* Size of output frame in bytes
                              * (==sum of partition sizes)                    */
        u32 refresh_frame_flags; /* 8-bit field to notify which of the
                                  * reference buffers were updated.           */
        u32 show_frame;  /* Recon debug, dunno if actually needed */
    } PPEncOut;

    typedef struct {
      u32 pass; /* Pass to execute (0 - both, 1 - first, 2 - second) */
      size_t num_stats; /* Number of stats entries in the stats buffer. */
      u32 width;        /* Encoded picture width in pixels, multiple of 8 */
      u32 height;       /* Encoded picture height in pixels, multiple of 8 */
      u32 origWidth;    /* Input camera picture width, multiple of 64 pixels  */
      u32 origHeight;   /* Input camera picture height, multiple of 64 pixels */
      u32 cropWidth;  /* Picture width after cropping. Will be scaled down to the
                         final encoded width. */
      u32 cropHeight; /* Picture height after cropping. Will be scaled down to
                         the final encoded height. */
      u32 xOffset;    /* Horizontal offset for cropping. */
      u32 yOffset;    /* Vertical offset for cropping. */
      u32 letterboxLeft;                      /* Letterbox area in the left. */
      u32 letterboxRight;                     /* Letterbox area in the right. */
      u32 letterboxTop;                       /* Letterbox area in the top. */
      u32 letterboxBottom;                    /* Letterbox area in the bottom. */
      u32 letterboxYColor;                    /* Letterbox Y area color. */
      u32 letterboxUColor;                    /* Letterbox U area color. */
      u32 letterboxVColor;                    /* Letterbox V area color. */
    } PPEncFirstPassCfg;

/* Version information */
    typedef struct
    {
        u32 major;           /* Encoder API major version */
        u32 minor;           /* Encoder API minor version */
    } PPEncApiVersion;

    typedef struct
    {
        u32 swBuild;         /* Software build ID */
        u32 hwBuild;         /* Hardware build ID */
    } PPEncBuild;

    typedef struct {
        u64 busLumaIn, busChromaUIn, busChromaVIn;
        u64 busLumaOut, busChromaOut;
        EncScaledPicture scaledPicture;
    } PPInput;

/*------------------------------------------------------------------------------
    4. Encoder API function prototypes
------------------------------------------------------------------------------*/

/* Static version information */
    PPEncApiVersion PPEncGetApiVersion(void);
    PPEncBuild PPEncGetBuild(void);

/* Helper for input format bit-depths */
    u32 PPEncGetBitsPerPixel(EncPictureType type, PPEncBitDepth bd);

/* Initialization & release */
    PPEncRet PPEncInit(const PPEncConfig *pEncConfig,
                         PPEncInst *instAddr);
    PPEncRet PPEncRelease(PPEncInst inst);

/* Picture pre-processing configuration which is applied before. */
    PPEncRet PPEncSetPreProcessing(PPEncInst inst,
                                     const EncPreProcessingCfg *pPreProcCfg);
    PPEncRet PPEncGetPreProcessing(PPEncInst inst,
                                     EncPreProcessingCfg *pPreProcCfg);

/* Read HW performance counter (clocks used since last call). If counters is
 * null, counter_count will be set to the number of counters. */
    PPEncRet PPEncGetHwPerfCounter(PPEncInst inst,
                                     HwPerfCounter *counters,
                                     u32 *counter_count);

    PPEncRet PPEncSetFirstPass(PPEncInst inst,
                                 const PPEncFirstPassCfg *pFirstPassCfg);

/* Generate taps for scaler. */
    PPEncRet PPComputeDefaultScalerTaps(PPEncInst inst, ScalerTaps *scalerTaps);

/* Select user defined taps or compute default */
    PPEncRet PPEncSetScalerTaps(PPEncInst inst, const ScalerTaps &scalerTaps);

    // Get the internal CWL instance
    void * PPEncGetCWL(PPEncInst inst);

    PPEncRet PreprocessorGetJob(PPEncInst inst, const PPEncIn *pEncIn,
                                SwRegisters *swregs);

/* Preprocess single frame. */
    PPEncRet PPImagePreProcess(PPEncInst inst, PPEncOut * pEncOut, SwRegisters* swregs);

#ifdef __cplusplus
}
#endif

#endif /*__PPENCAPI_H__*/
