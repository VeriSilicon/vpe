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
--  Abstract :  H2 VP9 Encoder API
--
------------------------------------------------------------------------------*/

#ifndef __VP9ENCAPI_H__
#define __VP9ENCAPI_H__

#include "base_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

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

    VP9ENC_ERROR = -1,
    VP9ENC_NULL_ARGUMENT = -2,
    VP9ENC_INVALID_ARGUMENT = -3,
    VP9ENC_MEMORY_ERROR = -4,
    VP9ENC_EWL_ERROR = -5,
    VP9ENC_EWL_MEMORY_ERROR = -6,
    VP9ENC_INVALID_STATUS = -7,
    VP9ENC_OUTPUT_BUFFER_OVERFLOW = -8,
    VP9ENC_HW_BUS_ERROR = -9,
    VP9ENC_HW_DATA_ERROR = -10,
    VP9ENC_HW_TIMEOUT = -11,
    VP9ENC_HW_RESERVED = -12,
    VP9ENC_SYSTEM_ERROR = -13,
    VP9ENC_INSTANCE_ERROR = -14,
    VP9ENC_HRD_ERROR = -15,
    VP9ENC_HW_RESET = -16
  } VP9EncRet;

  /* Picture YUV type for initialization */
  typedef enum
  {
    VP9ENC_YUV420_PLANAR = 0,              /* YYYY... UUUU... VVVV...  */
    VP9ENC_YUV420_SEMIPLANAR,              /* YYYY... UVUVUV...        */
    VP9ENC_YUV420_SEMIPLANAR_VU,           /* YYYY... VUVUVU...        */
    VP9ENC_YUV422_INTERLEAVED_YUYV,        /* YUYVYUYV...              */
    VP9ENC_YUV422_INTERLEAVED_UYVY,        /* UYVYUYVY...              */
    VP9ENC_RGB565,                         /* 16-bit RGB 16bpp         */
    VP9ENC_BGR565,                         /* 16-bit RGB 16bpp         */
    VP9ENC_RGB555,                         /* 15-bit RGB 16bpp         */
    VP9ENC_BGR555,                         /* 15-bit RGB 16bpp         */
    VP9ENC_RGB444,                         /* 12-bit RGB 16bpp         */
    VP9ENC_BGR444,                         /* 12-bit RGB 16bpp         */
    VP9ENC_RGB888,                         /* 24-bit RGB 32bpp         */
    VP9ENC_BGR888,                         /* 24-bit RGB 32bpp         */
    VP9ENC_RGB101010,                      /* 30-bit RGB 32bpp         */
    VP9ENC_BGR101010                       /* 30-bit RGB 32bpp         */
  } VP9EncPictureType;

  /* Picture rotation for pre-processing */
  typedef enum
  {
    VP9ENC_ROTATE_0 = 0,
    VP9ENC_ROTATE_90R = 1, /* Rotate 90 degrees clockwise */
    VP9ENC_ROTATE_90L = 2  /* Rotate 90 degrees counter-clockwise */
  } VP9EncPictureRotation;

  /* Picture color space conversion (RGB input) for pre-processing */
  typedef enum
  {
    VP9ENC_RGBTOYUV_BT601 = 0, /* Color conversion according to BT.601 */
    VP9ENC_RGBTOYUV_BT709 = 1, /* Color conversion according to BT.709 */
    VP9ENC_RGBTOYUV_USER_DEFINED = 2   /* User defined color conversion */
  } VP9EncColorConversionType;
  /* Picture type for encoding */
  typedef enum
  {
    VP9ENC_INTRA_FRAME = 0,
    VP9ENC_PREDICTED_FRAME = 1,
    VP9ENC_NOTCODED_FRAME  /* Used just as a return value */
  } VP9EncPictureCodingType;

  /* Reference picture mode for reading and writing */
  typedef enum
  {
    VP9ENC_NO_REFERENCE_NO_REFRESH = 0,
    VP9ENC_REFERENCE = 1,
    VP9ENC_REFRESH = 2,
    VP9ENC_REFERENCE_AND_REFRESH = 3
  } VP9EncRefPictureMode;

  /* Enumerations for adaptive parameters */
  enum
  {
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
  /*------------------------------------------------------------------------------
      3. Structures for API function parameters
  ------------------------------------------------------------------------------*/

  /* Configuration info for initialization
   * Width and height are picture dimensions after rotation
   * Width and height are restricted by level limitations
   */
  typedef struct
  {
    u32 refFrameAmount; /* Amount of reference frame buffers, [1..3]
                             * 1 = only last frame buffered,
                             *     always predict from and refresh ipf,
                             *     stream buffer overflow causes new key frame
                             * 2 = last and golden frames buffered
                             * 3 = last and golden and altref frame buffered  */
    u32 width;           /* Encoded picture width in pixels, multiple of 2 */
    u32 height;          /* Encoded picture height in pixels, multiple of 2 */
    u32 frameRateNum;    /* The stream time scale, [1..1048575] */
    u32 frameRateDenom;  /* Maximum frame rate is frameRateNum/frameRateDenom
                              * in frames/second. The actual frame rate will be
                              * defined by timeIncrement of encoded pictures,
                              * [1..frameRateNum] */


    u32 compressor;
  } VP9EncConfig;

  /* Defining rectangular macroblock area in encoder picture */
  typedef struct
  {
    u32 enable;         /* [0,1] Enables this area */
    u32 top;            /* Top macroblock row inside area [0..heightMbs-1] */
    u32 left;           /* Left macroblock row inside area [0..widthMbs-1] */
    u32 bottom;         /* Bottom macroblock row inside area [top..heightMbs-1] */
    u32 right;          /* Right macroblock row inside area [left..widthMbs-1] */
  } VP9EncPictureArea;

  /* Coding control parameters */
  typedef struct
  {
    /* The following parameters can only be adjusted before stream is
     * started. They affect the stream profile and decoding complexity. */
    u32 interpolationFilter;/* Defines the type of interpolation filter
                                 * for reconstruction. [0..2]
                                 * 0 = Bicubic (6-tap interpolation filter),
                                 * 1 = Bilinear (2-tap interpolation filter),
                                 * 2 = None (Full pixel motion vectors)     */
    u32 filterType;         /* Deblocking loop filter type,
                                 * 0 = Normal deblocking filter,
                                 * 1 = Simple deblocking filter             */

    /* The following parameters can be adjusted between frames.         */
    u32 filterLevel;        /* Deblocking filter level [0..64]
                                 * 0 = No filtering,
                                 * higher level => more filtering,
                                 * VP9ENC_FILTER_LEVEL_AUTO = calculate
                                 *      filter level based on quantization  */
    u32 filterSharpness;    /* Deblocking filter sharpness [0..8],
                                 * VP9ENC_FILTER_SHARPNESS_AUTO = calculate
                                 *      filter sharpness automatically      */
    u32 dctPartitions;      /* Amount of DCT coefficient partitions to
                                 * create for each frame, subject to HW
                                 * limitations on maximum partition amount.
                                 * 0 = 1 DCT residual partition,
                                 * 1 = 2 DCT residual partitions,
                                 * 2 = 4 DCT residual partitions,
                                 * 3 = 8 DCT residual partitions            */
    u32 errorResilient;     /* Enable error resilient stream mode. [0,1]
                                 * This prevents cumulative probability
                                 * updates.                                 */
    u32 splitMv;            /* Split MV mode ie. using more than 1 MV/MB
                                 * 0=disabled, 1=adaptive, 2=enabled        */
    u32 quarterPixelMv;     /* 1/4 pixel motion estimation
                                 * 0=disabled, 1=adaptive, 2=enabled        */
    u32 cirStart;           /* [0..mbTotal] First macroblock for
                                   Cyclic Intra Refresh */
    u32 cirInterval;        /* [0..mbTotal] Macroblock interval for
                                   Cyclic Intra Refresh, 0=disabled */
    VP9EncPictureArea intraArea;  /* Area for forcing intra macroblocks */
    VP9EncPictureArea roi1Area;   /* Area for 1st Region-Of-Interest */
    VP9EncPictureArea roi2Area;   /* Area for 2nd Region-Of-Interest */
    i32 roi1DeltaQp;              /* [-127..0] QP delta value for 1st ROI */
    i32 roi2DeltaQp;              /* [-127..0] QP delta value for 2nd ROI */
#if 0
    u32 deadzone;           /* Enable deadzone optimization. This will
                                 * choose the optimal amount of trailing ones
                                 * for each coded block. This improves
                                 * compression but slows down the encoding.
                                 * Enabled by default. */
#endif
    u32 maxNumPasses;       /* Maximum number of passes to encode input
                                 * frame. */
    VP9EncQualityMetric qualityMetric; /* Setup encoding settings for
                                 * optimizing against defined quality metrics */
    i32 qpDelta[5];         /* [-15, 16] QP difference from picture QP.
                                 * qpDelta[0] = delta for luma DC,
                                 * qpDelta[1] = delta for 2nd order luma DC,
                                 * qpDelta[2] = delta for 2nd order luma AC,
                                 * qpDelta[3] = delta for chroma DC,
                                 * qpDelta[4] = delta for chroma AC,
                                 * 0 = disabled (default),
                                 * VP9ENC_QPDELTA_AUTO = calculate QP delta
                                 *    based on quantization. */
    i32 max_cu_size;  /* Max coding unit size in pixels */
    i32 min_cu_size;  /* Min coding unit size in pixels */
    i32 max_tr_size;  /* Max transform size in pixels */
    i32 min_tr_size;  /* Min transform size in pixels */
    i32 tr_depth_intra; /* Max transform hierarchy depth */
    i32 tr_depth_inter; /* Max transform hierarchy depth */



  } VP9EncCodingCtrl;

  /* Rate control parameters */
  typedef struct
  {
    u32 pictureRc;       /* Adjust QP between pictures, [0,1] */
    u32 pictureSkip;     /* Allow rate control to skip pictures, [0,1] */
    i32 qpHdr;           /* QP for next encoded picture, [-1..127]
                              * -1 = Let rate control calculate initial QP.
                              * qpHdr is used for all pictures if
                              * pictureRc is disabled.                      */
    u32 qpMin;           /* Minimum QP for any picture, [0..127]        */
    u32 qpMax;           /* Maximum QP for any picture, [0..127]        */
    u32 bitPerSecond;    /* Target bitrate in bits/second, this is
                              * needed if pictureRc, mbRc, pictureSkip or
                              * hrd is enabled [10000..60000000]
                              */
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

  /* Encoder input structure */
  typedef struct
  {
    u32 busLuma;         /* Bus address for input picture
                              * planar format: luminance component
                              * semiplanar format: luminance component
                              * interleaved format: whole picture
                              */
    u32 busChromaU;      /* Bus address for input chrominance
                              * planar format: cb component
                              * semiplanar format: both chrominance
                              * interleaved format: not used
                              */
    u32 busChromaV;      /* Bus address for input chrominance
                              * planar format: cr component
                              * semiplanar format: not used
                              * interleaved format: not used
                              */
    u32 *pOutBuf;        /* Pointer to output stream buffer */
    u32 busOutBuf;       /* Bus address of output stream buffer */
    u32 outBufSize;      /* Size of output stream buffer in bytes */
    VP9EncPictureCodingType codingType; /* Proposed picture coding type,
                                             * INTRA/PREDICTED              */
    u32 timeIncrement;   /* The previous picture duration in units
                              * of 1/frameRateNum. 0 for the very first
                              * picture and typically equal to frameRateDenom
                              * for the rest.                               */

    VP9EncRefPictureMode ipf; /* Immediately previous == last frame */
    VP9EncRefPictureMode grf; /* Golden reference frame */
    VP9EncRefPictureMode arf; /* Alternative reference frame */
    u32 layerId;         /* When temporal layers are enabled by
                              * layerBitPerSecond, the layerId defines the
                              * layer this picture belongs to. [0..3]       */
  } VP9EncIn;

  /* Encoder output structure */
  typedef struct
  {
    VP9EncPictureCodingType codingType;     /* Realized picture coding type,
                                                 * INTRA/PREDICTED/NOTCODED   */
    u32 *pOutBuf[9];     /* Pointers to start of each partition in
                              * output stream buffer,
                              * pOutBuf[0] = Frame header + mb mode partition,
                              * pOutBuf[1] = First DCT partition,
                              * pOutBuf[2] = Second DCT partition (if exists)
                              * etc.                                          */
    u32 streamSize[9];   /* Size of each partition of output stream
                              * in bytes.                                     */
    u32 frameSize;       /* Size of output frame in bytes
                              * (==sum of partition sizes)
                              */
    i8 *motionVectors;   /* Pointer to a buffer storing encoded frame MVs,
                              * SAD values, macroblock modes and other
                              * coding information for every macroblock.
                              * See VP9TestBench.c for detailed format.       */
    /* The following three parameters apply when
     * codingType == PREDICTED. They define for each
     * of the reference pictures if it was used for
     * prediction and it it was refreshed. */
    VP9EncRefPictureMode ipf; /* Immediately previous == last frame */
    VP9EncRefPictureMode grf; /* Golden reference frame */
    VP9EncRefPictureMode arf; /* Alternative reference frame */
#if 0
    u32 mse_mul256;      /* Encoded frame Mean Squared Error
                                multiplied by 256.                            */
#endif
    u32 busScaledLuma;   /* Bus address for scaled encoder picture luma   */
    u8 *scaledPicture;   /* Pointer for scaled encoder picture            */
  } VP9EncOut;

  /* Input pre-processing */
  typedef struct
  {
    VP9EncColorConversionType type;
    u16 coeffA;          /* User defined color conversion coefficient */
    u16 coeffB;          /* User defined color conversion coefficient */
    u16 coeffC;          /* User defined color conversion coefficient */
    u16 coeffE;          /* User defined color conversion coefficient */
    u16 coeffF;          /* User defined color conversion coefficient */
  } VP9EncColorConversion;

  typedef struct
  {
    u32 origWidth;                          /* Input camera picture width */
    u32 origHeight;                         /* Input camera picture height*/
    u32 xOffset;                            /* Horizontal offset          */
    u32 yOffset;                            /* Vertical offset            */
    VP9EncPictureType inputType;            /* Input picture color format */
    VP9EncPictureRotation rotation;         /* Input picture rotation     */
    u32 videoStabilization;                 /* Enable video stabilization */
    VP9EncColorConversion colorConversion;  /* Define color conversion
                                                   parameters for RGB input   */
    u32 scaledWidth;    /* Optional down-scaled output picture width,
                              multiple of 4. 0=disabled. [16..width] */
    u32 scaledHeight;   /* Optional down-scaled output picture height,
                              multiple of 2. [96..height]                    */

    u32 scaledOutput;                       /* Enable output of down-scaled
                                                   encoder picture. Dimensions
                                                   specified at Init.         */
    u32 *virtualAddressScaledBuff;
    u32 busAddressScaledBuff;
    u32 sizeScaledBuff;

  } VP9EncPreProcessingCfg;


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

  struct vp9_buffer
  {
    struct vp9_buffer *next;
    u8 *buffer;     /* Data store */
    u32 cnt;      /* Data byte cnt */
    u32 busaddr;      /* Data store bus address */
  };



  /*------------------------------------------------------------------------------
      4. Encoder API function prototypes
  ------------------------------------------------------------------------------*/
  /* Version information */
  VP9EncApiVersion VP9EncGetApiVersion(void);
  VP9EncBuild VP9EncGetBuild(void);

  /* Helper for input format bit-depths */
  u32 VP9EncGetBitsPerPixel(VP9EncPictureType type);

  /* Initialization & release */
  VP9EncRet VP9EncInit(const VP9EncConfig *pEncConfig,
                       VP9EncInst *instAddr);
  VP9EncRet VP9EncRelease(VP9EncInst inst);

  /* Encoder configuration before stream generation */
  VP9EncRet VP9EncSetCodingCtrl(VP9EncInst inst, const VP9EncCodingCtrl *
                                pCodingParams);
  VP9EncRet VP9EncGetCodingCtrl(VP9EncInst inst, VP9EncCodingCtrl *
                                pCodingParams);

  /* Encoder configuration before and during stream generation */
  VP9EncRet VP9EncSetRateCtrl(VP9EncInst inst,
                              const VP9EncRateCtrl *pRateCtrl);
  VP9EncRet VP9EncGetRateCtrl(VP9EncInst inst,
                              VP9EncRateCtrl *pRateCtrl);

  VP9EncRet VP9EncSetPreProcessing(VP9EncInst inst,
                                   const VP9EncPreProcessingCfg *
                                   pPreProcCfg);
  VP9EncRet VP9EncGetPreProcessing(VP9EncInst inst,
                                   VP9EncPreProcessingCfg *pPreProcCfg);

  /* Stream generation */

  /* VP9EncStrmEncode encodes one video frame.
   */
  VP9EncRet VP9EncStrmEncode(VP9EncInst inst, const VP9EncIn *pEncIn,
                             VP9EncOut *pEncOut);

  /* internal encoder testing */
  VP9EncRet VP9EncSetTestId(VP9EncInst inst, u32 testId);
  /* Set motionVectors field of VP9EncOut structure to point macroblock mbNum */
  VP9EncRet VP9EncGetMbInfo(VP9EncInst inst, VP9EncOut *pEncOut,
                            u32 mbNum);
  /*------------------------------------------------------------------------------
      5. Encoder API tracing callback function
  ------------------------------------------------------------------------------*/

  void VP9EncTrace(const char *msg);

#ifdef __cplusplus
}
#endif

#endif
