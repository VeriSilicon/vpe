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
--------------------------------------------------------------------------------*/

#ifndef RATE_CONTROL_PICTURE_H
#define RATE_CONTROL_PICTURE_H

#include "base_type.h"
#include "sw_picture.h"
#include "enccommon.h"
#include "hevcSei.h"

#ifdef VSB_TEMP_TEST
#include "video_statistic.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif



  //#include "enccommon.h"
  //#include "H264Sei.h"

  enum
  {
    VCENCRC_OVERFLOW = -1
  };

#define RC_CBR_HRD  0   /* 1 = Constant bit rate model. Must use filler
  * data to conform */

#define CTRL_LEVELS          7  /* DO NOT CHANGE THIS */
#define CHECK_POINTS_MAX    10  /* DO NOT CHANGE THIS */
#define RC_TABLE_LENGTH     10  /* DO NOT CHANGE THIS */
#define INTRA_QPDELTA_DEFAULT  (-5)

#ifndef CTBRC_STRENGTH 
#define QP_FRACTIONAL_BITS  0
#else
#define QP_FRACTIONAL_BITS  8
#endif

#define TOL_CTB_RC_FIX_POINT 7

#define RC_MOVING_AVERAGE_FRAMES 60

#define LEAST_MONITOR_FRAME       3
#define I32_MAX                  (0x7FFFFFFF)

#define IS_CTBRC(x) ((x)&3)
#define IS_CTBRC_FOR_QUALITY(x) ((x)&1)
#define IS_CTBRC_FOR_BITRATE(x) ((x)&2)
#define CLR_CTBRC_FOR_BITRATE(x) {(x)&=(~2);}

#define CTB_RC_QP_STEP_FIXP 16
#define CTB_RC_ROW_FACTOR_FIXP 16

  typedef struct {
      i32 frame[120];
      i32 length;
      i32 count;
      i32 pos;
      i32 frameRateNumer;
      i32 frameRateDenom;
  } rc_ma_s;
  typedef struct {
      u32 intraCu8Num[120];
      u32 skipCu8Num[120];
      u32 PBFrame4NRdCost[120];
      i32 length;
      i32 count;
      i32 pos;
      u32 average_intraCu8Num;
      u32 average_skipCu8Num;
      u32 average_PBFrame4NRdCost;
  } rc_ma_char;
  typedef struct
  {
    i64  a1;               /* model parameter */
    i64  a2;               /* model parameter */
    i32  qp_prev;          /* previous QP */
    i32  qs[RC_TABLE_LENGTH + 1]; /* quantization step size */
    i32  bits[RC_TABLE_LENGTH + 1]; /* Number of bits needed to code residual */
    i32  pos;              /* current position */
    i32  len;              /* current lenght */
    i32  zero_div;         /* a1 divisor is 0 */
    i32  cbr;
    i32  weight;
	i32  frameBitCntLast;
    i32  targetPicSizeLast;
} linReg_s;

  typedef struct
  {
    i32 wordError[CTRL_LEVELS]; /* Check point error bit */
    i32 qpChange[CTRL_LEVELS];  /* Check point qp difference */
    i32 wordCntTarget[CHECK_POINTS_MAX];    /* Required bit count */
    i32 wordCntPrev[CHECK_POINTS_MAX];  /* Real bit count */
    i32 checkPointDistance;
    i32 checkPoints;
  } hevcQpCtrl_s;

  /* Virtual buffer */
  typedef struct
  {
    i32 bufferSize;          /* size of the virtual buffer */
    i32 minBitRate;
    i32 maxBitRate;
    i32 bitRate;             /* input bit rate per second */
    i32 bitPerPic;           /* average number of bits per picture */
    i32 picTimeInc;          /* timeInc since last coded picture */
    i32 timeScale;           /* input frame rate numerator */
    i32 unitsInTic;          /* input frame rate denominator */
    i32 virtualBitCnt;       /* virtual (channel) bit count */
    i32 realBitCnt;          /* real bit count */
    i32 bufferOccupancy;     /* number of bits in the buffer */
    i32 skipFrameTarget;     /* how many frames should be skipped in a row */
    i32 skippedFrames;       /* how many frames have been skipped in a row */
    i32 nonZeroTarget;
    i32 bucketFullness;      /* Leaky Bucket fullness */
    i32 bucketLevel;         /* Leaky Bucket fullness + virtualBitCnt */
    /* new rate control */
    i32 windowRem;
    i32 seconds;             /* Full seconds elapsed */
    i32 averageBitRate;      /* This buffer average bitrate for full seconds */
  } rcVirtualBuffer_s;

  typedef struct
  {
    i32 x0;
    i32 x1;
    i32 xMin;
    i32 started;
    i32 preFrameMad;
    ptr_t ctbMemPreAddr;
    u32* ctbMemPreVirtualAddr;
  } ctbRcModel_s;

  typedef struct
  {
    ctbRcModel_s models[3];
    i32 qpSumForRc;
    i32 qpStep;
    i32 rowFactor;
    ptr_t ctbMemCurAddr;
    u32* ctbMemCurVirtualAddr;
  } ctbRateControl_s;

  typedef struct
  {
      double coeffMin;
      double coeff;
      double count;
      double decay;
      double offset;
      i32 qp;
  } rcPredictor;

  typedef struct
  {
    true_e picRc;
    u32    ctbRc;            /* ctb header qp can vary, check point rc */
    true_e picSkip;          /* Frame Skip enable */
    true_e hrd;              /* HRD restrictions followed or not */
    true_e vbr;              /* Variable Bit Rate Control by qpMin */
    u32 fillerIdx;
    i32 picArea;
    i32 ctbPerPic;            /* Number of macroblock per picture */
    i32 ctbRows;              /* ctb rows in picture */
    i32 ctbCols;              /* ctb columns in picture */
    i32 ctbSize;              /* ctb size */
    i32 coeffCntMax;         /* Number of coeff per picture */
    i32 nonZeroCnt;
    i32 srcPrm;              /* Source parameter */
    i32 qpSum;               /* Qp sum counter */
    i32 qpNum;
    float averageQp;
    u32 sliceTypeCur;
    u32 sliceTypePrev;
    true_e frameCoded;       /* Pic coded information */
    i32 fixedQp;             /* Pic header qp when fixed */
    i32 qpHdr;               /* Pic header qp of current voded picture */
    i32 qpMin;               /* Pic header minimum qp for current picture */
    i32 qpMax;               /* Pic header maximum qp for current picture */
    i32 qpMinI;              /* Pic header minimum qp for I frame, user set */
    i32 qpMaxI;              /* Pic header maximum qp for I frame, user set */
    i32 qpMinPB;             /* Pic header minimum qp for P/B frame, user set */
    i32 qpMaxPB;             /* Pic header maximum qp for P/B frame, user set */
    i32 qpHdrPrev;           /* Pic header qp of previous coded picture */
    i32 qpLastCoded;         /* Quantization parameter of last coded mb */
    i32 qpTarget;            /* Target quantrization parameter */
    u32 estTimeInc;
    i32 outRateNum;
    i32 outRateDenom;
    i32 gDelaySum;
    i32 gInitialDelay;
    i32 gInitialDoffs;
    hevcQpCtrl_s qpCtrl;
    rcVirtualBuffer_s virtualBuffer;
    sei_s sei;
    i32 gBufferMin, gBufferMax;
    /* new rate control */
    linReg_s linReg[4];       /* Data for R-Q model for inter frames */
    linReg_s rError[4];       /* Rate prediction error for inter frames(bits) */
    linReg_s intra;        /* Data for intra frames */
    linReg_s intraError;   /* Prediction error for intra frames */
    linReg_s gop;          /* Data for GOP */
    //linReg_s linReg_BFrame;       /* Data for R-Q model for B frames */
    //linReg_s rError_BFrame;       /* Rate prediction error for B frames(bits) */
    i32 targetPicSize;
    i32 minPicSizeI;
    i32 maxPicSizeI;
    i32 minPicSizeP;
    i32 maxPicSizeP;
    i32 minPicSizeB;
    i32 maxPicSizeB;
    i32 frameBitCnt;
    i32 tolMovingBitRate;
    float f_tolMovingBitRate;
    i32 monitorFrames;
    float tolCtbRcInter;
    float tolCtbRcIntra;
    /* for gop rate control */
    i32 gopQpSum;           /* Sum of current GOP inter frame QPs */
    i32 gopQpDiv;
    i32 gopPQpSum;           /* Sum of current GOP inter frame QPs */
    i32 gopPQpNum;
    i32 gopBitCnt;          /* Current GOP bit count so far */
    i32 gopAvgBitCnt;       /* Previous GOP average bit count */
    u32 frameCnt;
    i32 bitrateWindow;
    i32 windowLen;          /* Bitrate window which tries to match target */
    i32 intraInterval;      /* Distance between two previous I-frames */
    i32 intraIntervalCtr;
    i32 intraQpDelta;
    i32 longTermQpDelta; /* QP delta of the frame using long term reference */
    i32 frameQpDelta;
    u32 fixedIntraQp;
    i32 bpp;
    i32 hierarchial_bit_allocation_total_weight;
    i32 hierarchial_bit_allocation_map[8][8];
    i32 hierarchial_sse[8][8];
    i32 smooth_psnr_in_gop;
    i32 hierarchial_alg_map[8][8];
    i32 hierarchial_decoding_order[8][8];
    i32 hierarchial_bit_allocation_bitmap[8];
    i32 hierarchial_bit_allocation_GOP_size;
    i32 hierarchial_bit_allocation_GOP_size_previous;
    i32 encoded_frame_number;
    u32 gopPoc;
    //CTB_RC
    u32 ctbRcBitsMin; 
    u32 ctbRcBitsMax; 
    u32 ctbRctotalLcuBit; 
    u32 bitsRatio; 
    u32 ctbRcThrdMin; 
    u32 ctbRcThrdMax; 
    i32 seqStart;
    u32 rcQpDeltaRange;
    u32 rcBaseMBComplexity;
    i32 picQpDeltaMin;
    i32 picQpDeltaMax;
    i32 resetIntraPicQp;
    i32 inputSceneChange;

    rc_ma_s ma;
    rc_ma_char ma_char;
    
    u32 rcPicComplexity;
    float complexity;

    i32 minIQp;
    i32 finiteQp;
    i32 gopMulti;
    i32 gopLastBitCnt;
    i32 intraframeBitCnt;
    
    u32 intraCu8Num;
    u32 skipCu8Num;
    u32 PBFrame4NRdCost;
    double reciprocalOfNumBlk8;  /* Multiply this factor to get blk8x8 avg */

    u32 codingType;

    i32 i32MaxPicSize;
    u32 u32PFrameByQPStep;
    i32 qpHdrPrevGop;
    i32 qpHdrPrevI;
    u32 u32PrevIBitCnt;
    u32 u32StaticSceneIbitPercent;

    ctbRateControl_s ctbRateCtrl;

    i32 crf; /*CRF constant*/
    double pbOffset;
    double ipOffset;
    u64 crf_iCostAvg;
    double crf_iQpAvg;
    int crf_iFrames;

    i32 pass;
    double pass1CurCost;
    double pass1GopCost[4];
    double pass1AvgCost[4];
    i32 pass1GopFrameNum[4];
    i32 pass1FrameNum[4];
    rcPredictor rcPred[4];
    i32 predId;
  } vcencRateControl_s;

  /*------------------------------------------------------------------------------
      Function prototypes
  ------------------------------------------------------------------------------*/
  bool_e VCEncInitRc(vcencRateControl_s *rc, u32 newStream);
  void VCEncBeforePicRc(vcencRateControl_s *rc, u32 timeInc, u32 sliceType, bool use_ltr_cur, struct sw_picture* pic);
  i32 VCEncAfterPicRc(vcencRateControl_s *rc, u32 nonZeroCnt, u32 byteCnt,
                     u32 qpSum,u32 qpNum);
  u32 HevcFillerRc(vcencRateControl_s *rc, u32 frameCnt);
  i32 rcCalculate(i32 a, i32 b, i32 c);
  i32 getPredId(vcencRateControl_s *rc);
  double qp2qScale(double qp);


#ifdef __cplusplus
}
#endif


#endif
