/* Copyright 2014 Google Inc. All Rights Reserved. */

/*------------------------------------------------------------------------------
--
--  Abstract : VP9 Encoder API
--
------------------------------------------------------------------------------*/

#ifndef __VP9ENCAPI_H__
#define __VP9ENCAPI_H__

#include "basetype.h"
#include "encapicommon.h"
//#include "scaler_common.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_LAG_IN_FRAMES     28
#define HW_MAX_LAG_IN_FRAMES  25

/*------------------------------------------------------------------------------
    1. Type definition for encoder instance
------------------------------------------------------------------------------*/

    typedef const void *VP9EncInst;

/*------------------------------------------------------------------------------
    2. Enumerations for API parameters
------------------------------------------------------------------------------*/

/* Function return values */
    typedef enum
    {
        VP9ENC_OK = 0,
        VP9ENC_FRAME_READY = 1,
        VP9ENC_FRAME_READY_MORE_AVAILABLE = 2,
        VP9ENC_LAG_FIRST_PASS = 3,
        VP9ENC_LAG = 4,

        VP9ENC_ERROR = -1,
        VP9ENC_NULL_ARGUMENT = -2,
        VP9ENC_INVALID_ARGUMENT = -3,
        VP9ENC_MEMORY_ERROR = -4,
        VP9ENC_CWL_ERROR = -5,
        VP9ENC_CWL_MEMORY_ERROR = -6,
        VP9ENC_INVALID_STATUS = -7,
        VP9ENC_OUTPUT_BUFFER_OVERFLOW = -8,
        VP9ENC_HW_BUS_ERROR = -9,
        VP9ENC_HW_DATA_ERROR = -10,
        VP9ENC_HW_TIMEOUT = -11,
        VP9ENC_HW_RESERVED = -12,
        VP9ENC_SYSTEM_ERROR = -13,
        VP9ENC_INSTANCE_ERROR = -14,
        VP9ENC_HRD_ERROR = -15,
        VP9ENC_HW_RESET = -16,
        VP9ENC_FIRSTPASS_FAILED = -17
    } VP9EncRet;

/* Picture bit-depth */
    typedef enum {
      VP9ENC_8BIT     = 0,
      VP9ENC_10BIT    = 1
    } VP9EncBitDepth;

/* Picture type for encoding */
    typedef enum
    {
        VP9ENC_INTRA_FRAME = 0,
        VP9ENC_PREDICTED_FRAME = 1,
        VP9ENC_NOTCODED_FRAME         /* Used just as a return value  */
    } VP9EncPictureCodingType;

/* Enumerations for adaptive parameters */
    enum {
        VP9ENC_FILTER_LEVEL_AUTO = 64,
        VP9ENC_FILTER_SHARPNESS_AUTO = 8,
        VP9ENC_QPDELTA_AUTO = 16
    };

/* Enumerations for quality metric optimization */
    typedef enum
    {
        VP9ENC_QM_PSNR  = 0,
        VP9ENC_QM_SSIM  = 1
    } VP9EncQualityMetric;

/* Enumerations for bandwidth mode */
    typedef enum
    {
        VP9ENC_BWM_LO  = 0,
        VP9ENC_BWM_MID = 1,
        VP9ENC_BWM_HI  = 2
    } VP9EncBandwidthMode;

/* Enumeration for trace levels. */
    typedef enum {
        VP9ENC_TRACE_ERROR = 0,
        VP9ENC_TRACE_WARNING = 1,
        VP9ENC_TRACE_INFO = 2,
        VP9ENC_TRACE_VERBOSE = 3
    } VP9EncTraceLevel;

/*------------------------------------------------------------------------------
    3. User defined functions.
------------------------------------------------------------------------------*/

    typedef void (*VP9EncTrace)(void *p, VP9EncTraceLevel l, const char *fmt,
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
        VP9EncBitDepth bitdepth;  /* Encoded picture bit-depth               */
        u32 frameRateNum;    /* The stream time scale, [1..1048575]          */
        u32 frameRateDenom;  /* Frame rate shall be frameRateNum/frameRateDenom
                              * in frames/second. [1..frameRateNum]          */
        u32 frameComp;       /* Enable reference frame compression           */
        u32 lookaheadCount;  /* Number of frames to look ahead. */
        u32 temporalFilter;  /* Enable temporal filtering. Only valid when
                              * lookaheadCount is nonzero. */
        u32 passes;          // Number of encoding passes (1/2)
        u32 enable_multicore; /* Set if each tile is encoded by one HW instance */
        u32 calculate_ssim;   /* Set if application wants to receive HW SSIM
                               * results. */
        void *trace_ctx;      /* User defined ptr for trace callbacks. */
        VP9EncTrace trace;    /* User defined trace function. */
		  //add for new driver
#ifdef DRV_NEW_ARCH
		int priority;
		char * device;
		int mem_id;
		int cwl_index;
#endif
#ifdef VP9_ERR_TEST
        int memory_shadow;
#endif
#ifdef VP9_EDMA_ERR_TEST
		int edma_err_shadow;
#endif

        VP9ENCPERF * perf;
    } VP9EncConfig;

/* Coding control parameters */
    typedef struct
    {
        u32 interpolationFilter;/* Defines the type of interpolation filter
                                 * 0 = smooth   1 = normal     2 = sharp
                                 * 3 = bilinear 4 = switchable              */
        u32 filterLevel;        /* Deblocking filter level [0..64]
                                 * 0 = No filtering,
                                 * higher level => more filtering,
                                 * VP9ENC_FILTER_LEVEL_AUTO = calculate
                                 *      filter level based on quantization  */
        u32 filterSharpness;    /* Deblocking filter sharpness [0..8],
                                 * VP9ENC_FILTER_SHARPNESS_AUTO = calculate
                                 *      filter sharpness automatically      */
        u32 errorResilient;     /* Enable error resilient stream mode. [0,1]
                                 * This prevents cumulative probability
                                 * updates.                                 */
        u32 frameParallelDecodingMode; /* Enable frame parallel decodability features.
                                 * This prevents backward adaptation of probabilities. */
        VP9EncQualityMetric qualityMetric; /* Setup encoding settings for
                                 * optimizing against defined quality metrics */
        i32 qpDelta[3];         /* [-15, 16] QP difference from picture QP.
                                 * qpDelta[0] = delta for luma DC,
                                 * qpDelta[1] = delta for chroma DC,
                                 * qpDelta[2] = delta for chroma AC,
                                 * 0 = disabled (default),
                                 * VP9ENC_QPDELTA_AUTO = calculate QP delta
                                 *    based on quantization. */
        u32 highPrecMvEnable;   /* 1/8 pixel motion vector enable.           */
        u32 meChromaWeight;     /* Motion estimation chroma weight
                                 * 0 = 1X, 1=2X, 3=4X, 4=8X                  */
        i32 effort;             /* Encoder effort level [0..3]
                                 * 0 = fastest - 3 = best quality           */
        VP9EncBandwidthMode bandwidthMode;  /* Instruct encoder to adjust
                                             * bandwidth usage. */
        u32 lossless;           /* Lossless encoding. Forces all filter and
                                 * quant parameters to 0.                   */
        u32 rdoLambda[4];       /* ME and RDO lambdas. Testing purposes
                                 * only, not recommended to be modified.     */
        u32 splitPenalty[4];    /* Penalty for partitioning. Testing purposes
                                 * only, not recommended to be modified.     */
        u32 wmeLambdaZero;
        u32 wmeLambdaMotion;
        /* TEMP HERE */
        u32 refFrameScheme;

        u32 arfTemporalFilterStrength;  /* 0=filter disabled. Otherwise 1...15 */
        u32 arfTemporalFilterLength;    /* 0=3 frames.
                                         * 1=6 frames. */
        u32 arfTemporalFilterThreshold; /* Filtering threshold. NOTE! for
                                         * sweeping purposes currently. */
        u32 cqLevel; /* constrained quality level, defines lowest qp for regular
                        frames */
        u32 minArfPeriod; /* min period between alt-ref frames */
        u32 maxArfPeriod; /* max period between alt-ref frames */
        u32 videoFullRange;  /* Input video signal sample range, [0,1]
                              * 0 = Y range in [16..235],
                              * Cb&Cr range in [16..240]
                              * 1 = Y, Cb and Cr range in [0..255]
                              */
    } VP9EncCodingCtrl;

    typedef struct {
        u32 enable;
        // Segmentation map. One bit for each 8x8 block, picture dimensions
        // rounded up to SB multiple.
        const u8* fgbg_map;
        // segment tree probabilities. NULL implies using internal values
        const u8 (*segment_tree_probs)[7];
        // segment prediction probabilities. NULL implies using internal values.
        const u8 (*segment_pred_probs)[3];
        // Quantizer level to use for each segment. Negative value implies using
        // current rate control determined level.
        i16 quantSegment[8];
        // Filter level to use for each segment. Negative value implies using
        // current filter level.
        i16 filterLevelSegment[8];
    } Vp9EncSegmentCtrl;

/* Rate control parameters */
    typedef struct
    {
        u32 pictureRc;       /* Adjust QP between pictures, [0,1]           */
        u32 pictureSkip;     /* Allow rate control to skip pictures, [0,1]  */
        i32 qpHdr;           /* QP for next encoded picture, [-1..255]
                              * -1 = Let rate control calculate initial QP.
                              * qpHdr is used for all pictures if
                              * pictureRc is disabled.                      */
        u32 currentActiveQp; /* Current encoding QP. Read-only. */
        u32 qpMin;           /* Minimum QP for any picture, [0..255]        */
        u32 qpMax;           /* Maximum QP for any picture, [0..255]        */
        u32 bitPerSecond;    /* Target bitrate in bits/second
                              * [10000..60000000]                           */
        u32 layerBitPerSecond[4]; /* Enables temporal layers and sets the
                              * target bitrate in bits/second for each
                              * temporal layer. 0 disables temporal layers.
                              * When enabled, overrides the bitPerSecond value.
                              * The resulting bitrate will be the sum of all
                              * layer bitrates.                             */
        u32 layerFrameRateDenom[4]; /* When temporal layers are enabled
                              * defines the average frame rate denominator for
                              * each layer. frameRateNum is used as time scale
                              * for every layer. [1..frameRateNum]          */
        u32 bitrateWindow;   /* Number of pictures over which the target
                              * bitrate should be achieved. Smaller window
                              * maintains constant bitrate but forces rapid
                              * quality changes whereas larger window
                              * allows smoother quality changes. [1..300]   */
        i32 intraQpDelta;    /* Intra QP delta. intraQP = QP + intraQpDelta
                              * This can be used to change the relative
                              * quality of the Intra pictures or to decrease
                              * the size of Intra pictures. [-50..50]       */
        u32 fixedIntraQp;    /* Fixed QP value for all Intra pictures, [0..127]
                              * 0 = Rate control calculates intra QP.       */
        u32 intraPictureRate;/* The distance of two intra pictures, [0..1000]
                              * This will force periodical intra pictures.
                              * 0=disabled.                                 */
        u32 goldenPictureRate;/* The distance of two golden pictures, [0..1000]
                              * This will force periodical golden pictures.
                              * 0=disabled.                                 */
        u32 altrefPictureRate;/* The distance of two altref pictures, [0..1000]
                              * This will force periodical altref pictures.
                              * 0=disabled.                                 */
        u32 goldenPictureBoost;/* Quality boost for golden pictures, [0..100]
                              * This will increase quality for golden pictures.
                              * 0=disabled.                                 */
        u32 adaptiveGoldenBoost; /* Quality for adaptive boost for golden
                              * pictures, [0..100]. Encoder will select boost
                              * level based on statistics gathered from encoded
                              * bitstream. 0=disabled.                      */
        u32 adaptiveGoldenUpdate; /* Enable adaptive update of golden picture.
                              * Picture not necessarily updated on given period
                              * if certain criterias met.                   */
    } VP9EncRateCtrl;

/* First pass configurations. */
    typedef struct Vp9EncFirstPassStats_s {
      u64 intra_error;      /* Intra coding cumulative error */
      u32 intra_skip_count; /* Intra blocks with very small error */
      u64 coded_error;      /* Frame cumulative error */
      u64 second_ref_error; /* Second reference cumulative error */
      u32 inter_count;      /* Number of inter blokcs */
      u32 ref0_count;       /* Number of blocks referencing last frame */
      u32 ref1_count;       /* Number of blocks referencing golden frame */
      u32 ref2_count;       /* Number of blocks referencing arf frame */
      u32 neutral_count; /* Number of blocks where intra and inter are close */
      u32 mv_count;      /* Number of motion vectors (MV) */
      i64 sum_mv_x;      /* Sum of row MV values */
      u64 sum_mv_x_abs;  /* Sum of abs row MV values */
      i64 sum_mv_y;      /* Sum of col MV values */
      u64 sum_mv_y_abs;  /* Sum of abs col MV values */
      u64 sum_mv_x_sqr;  /* Sum of row MV square (for variance calc) */
      u64 sum_mv_y_sqr;  /* Sum of col MV square (for variance calc) */
      i32 mv_in_count;   /* Number of MV pointing inwards */
      u32 new_mv_count;  /* Number of new MV */
      u32 skip_count;    /* Number of skipped blocks */
      i16 image_data_start_row; /* First non-blank row */
      size_t num_mbs;    /* Number of macroblocks in the picture */
      size_t byte_count; /* Number of bytes picture encoding produced */
    } Vp9EncFirstPassStats;

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

        void* pOutBuf;        /* Pointer to output stream buffer             */
        size_t busOutBuf;     /* Bus address of output stream buffer         */
        u32 outBufSize;      /* Size of output stream buffer in bytes       */

        VP9EncPictureCodingType codingType; /* Proposed picture coding type,
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

        i32 indexTobeEncode; /* add for lag_in */
        size_t busLumaTable;
        size_t busChromaTable;
        int compress;
	 	int width;
		int height;
        int encode_width;
        int encode_height;
        i32 nextIndex;
		u32 encindex;
		i32 inputdepth;
    } VP9EncIn;

/* Encoder output structure */
    typedef struct
    {
        VP9EncPictureCodingType codingType; /* Realized picture coding type   */
        void* pOutBuf[9];    /* Pointers to start of each partition in
                              * output stream buffer,
                              * pOutBuf[0] = Frame header + mb mode partition,
                              * pOutBuf[1] = DCT partition
                              *
                              * In first pass mode pOutBuf[0] contains first
                              * pass statistics (Vp9EncFirstPassStats)        */
        u32 streamSize[9];   /* Size of each partition of output stream
                              * in bytes.                                     */
        u32 frameSize;       /* Size of output frame in bytes
                              * (==sum of partition sizes)                    */
        u32 refresh_frame_flags; /* 8-bit field to notify which of the
                                  * reference buffers were updated.           */
        u32 show_frame;  /* Recon debug, dunno if actually needed */
        u8 *curr_frame;  /* Recon debug, def not needed in real world */
        Vp9EncFirstPassStats stats; /* stats from regular encoding pass, or next
                                       frame first pass stats for low latency
                                       2-pass mode */
        double  ssim_lu;  /* Luma SSIM, if enabled */
        double  ssim_cb;  /* Chroma Cb SSIM, if enabled */
        double  ssim_cr;  /* Chroma Cr SSIM, if enabled */

        i32 indexEncoded; /* add for lag_in */
        i32 nextIndex;		
    } VP9EncOut;

    typedef struct {
      u32 pass; /* Pass to execute (0 - both, 1 - first, 2 - second) */
      Vp9EncFirstPassStats *stats; /* Pointer to the first pass log data. */
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
      u32 pp_only;                            /* Preprocessor only mode */
    } VP9EncFirstPassCfg;

/* Version information */
    typedef struct
    {
        u32 major;           /* Encoder API major version */
        u32 minor;           /* Encoder API minor version */
    } VP9EncApiVersion;

    typedef struct
    {
        u32 swBuild;         /* Software build ID */
        u32 hwBuild;         /* Hardware build ID */
    } VP9EncBuild;

/*------------------------------------------------------------------------------
    4. Encoder API function prototypes
------------------------------------------------------------------------------*/

/* Static version information */
    VP9EncApiVersion VP9EncGetApiVersion(void);
    VP9EncBuild VP9EncGetBuild(void);

/* Helper for input format bit-depths */
    u32 VP9EncGetBitsPerPixel(EncPictureType type, VP9EncBitDepth bd);

/* Initialization & release */
    VP9EncRet VP9EncInit(const VP9EncConfig *pEncConfig,
                         VP9EncInst *instAddr);
    VP9EncRet VP9EncRelease(VP9EncInst inst);

/* Coding parameters which can not be adjusted during encoding. */
    VP9EncRet VP9EncSetCodingCtrl(VP9EncInst inst,
                                  const VP9EncCodingCtrl *pCodingParams);
    VP9EncRet VP9EncGetCodingCtrl(VP9EncInst inst,
                                  VP9EncCodingCtrl *pCodingParams);

/* Coding parameters which can be adjusted during encoding. */
    VP9EncRet VP9EncSetRateCtrl(VP9EncInst inst,
                                const VP9EncRateCtrl *pRateCtrl);
    VP9EncRet VP9EncGetRateCtrl(VP9EncInst inst,
                                VP9EncRateCtrl *pRateCtrl);

/* Picture pre-processing configuration which is applied before. */
    VP9EncRet VP9EncSetPreProcessing(VP9EncInst inst,
                                     const EncPreProcessingCfg *pPreProcCfg);
    VP9EncRet VP9EncGetPreProcessing(VP9EncInst inst,
                                     EncPreProcessingCfg *pPreProcCfg);
    VP9EncRet VP9EncSetSegmentation(VP9EncInst inst, Vp9EncSegmentCtrl *info, u32 sw_mixed_segment_penalty);

/* Stream generation. Encode one video frame. */
    VP9EncRet VP9EncStrmEncode(VP9EncInst inst, const VP9EncIn *pEncIn,
                               VP9EncOut *pEncOut);

/* Read HW performance counter (clocks used since last call). If counters is
 * null, counter_count will be set to the number of counters. */
    VP9EncRet VP9EncGetHwPerfCounter(VP9EncInst inst,
                                     HwPerfCounter *counters,
                                     u32 *counter_count);
#ifdef FPGA
	VP9EncRet VP9EncGetHwTimePerFrame(VP9EncInst inst,	  i32 enc_index,float *time_use);

#endif

    VP9EncRet VP9EncSetFirstPass(VP9EncInst inst,
                                 const VP9EncFirstPassCfg *pFirstPassCfg);
#if 0
/* Generate taps for scaler. */
    VP9EncRet VP9ComputeDefaultScalerTaps(VP9EncInst inst, ScalerTaps *scalerTaps);

/* Select user defined taps or compute default */
    VP9EncRet VP9EncSetScalerTaps(VP9EncInst inst, const ScalerTaps &scalerTaps);
#endif
  // Get the internal CWL instance
  void * VP9EncGetCWL(VP9EncInst inst);

  void VP9AXISet(VP9EncInst inst, u32 swap_input, i32 axi_rd_id, i32 axi_wr_id, u16 axi_rd_burst, u16 axi_wr_burst);
  void VP9DECSet(VP9EncInst inst, i32 width, i32 height, i32 bitdepth,VP9EncIn *encIn, u32 lumaTable, u32 chromaTable, u32 inputCompress, i32 pass);
#ifndef USE_OLD_DRV
  void VP9OutSet(VP9EncInst inst, size_t ep_base, size_t rc_base, u32 size);
#endif

#ifdef VP9_ERR_TEST
  int VP9MemoryCheck(VP9EncInst inst);
#endif
#ifdef VP9_EDMA_ERR_TEST
  int VP9EDMAErrCheck(VP9EncInst inst);
#endif

#ifdef SECOND_PASS_ONE_THREAD
void VP9SetEnd(VP9EncInst inst);
#endif

  int VP9ShowExisting(VP9EncInst inst);
#ifdef __cplusplus
}
#endif

#endif /*__VP9ENCAPI_H__*/
