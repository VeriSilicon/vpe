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

#ifndef SW_CU_TREE_H
#define SW_CU_TREE_H

#include "base_type.h"
#include "sw_put_bits.h"
#include "sw_picture.h"
#include "enccommon.h"
#include "math.h"
#include <pthread.h>

#define CU_TREE_VERSION 1
#define CU_TREE_QP 10

#define AGOP_MOTION_TH 8

#define DEFAULT_MAX_HIE_DEPTH 2

#define CUTREE_BUFFER_NUM X265_BFRAME_MAX

#define X265_TYPE_IDR           0x0001
#define X265_TYPE_I             0x0002
#define X265_TYPE_P             0x0003
#define X265_TYPE_BREF          0x0004  /* Non-disposable B-frame */
#define X265_TYPE_B             0x0005
#define X265_TYPE_BLDY          0x0006
#define IS_X265_TYPE_I(x) ((x) == X265_TYPE_I || (x) == X265_TYPE_IDR)
#define IS_X265_TYPE_B(x) ((x) == X265_TYPE_B || (x) == X265_TYPE_BREF)
#define IS_X265_TYPE_P(x) ((x) == X265_TYPE_P || (x) == X265_TYPE_BLDY)
#define X265_CODING_TYPE(x) (IS_X265_TYPE_I(x) ? VCENC_INTRA_FRAME : (IS_X265_TYPE_B(x) ? VCENC_BIDIR_PREDICTED_FRAME : VCENC_PREDICTED_FRAME))
#define CUTREE_BUFFER_CNT(depth) ((depth)+MAX_GOP_SIZE-1)

/* Arbitrary limitations as a sanity check. */
#define MAX_FRAME_DURATION 1.00
#define MIN_FRAME_DURATION 0.01
#define MAX_FRAME_DURATION_FIX8 256
#define MIN_FRAME_DURATION_FIX8 3

#define CLIP_DURATION(f) CLIP3(MIN_FRAME_DURATION, MAX_FRAME_DURATION, f)
#define CLIP_DURATION_FIX8(f) CLIP3(MIN_FRAME_DURATION_FIX8, MAX_FRAME_DURATION_FIX8, f)

#define LOWRES_COST_SHIFT CU_INFO_COST_BITS
#define LOWRES_COST_MASK  ((1 << LOWRES_COST_SHIFT) - 1)

#define X265_MIN(a, b) ((a) < (b) ? (a) : (b))
#define X265_MAX(a, b) ((a) > (b) ? (a) : (b))

#if defined(_MSC_VER)
#define X265_LOG2F(x) (logf((float)(x)) * 1.44269504088896405f)
#define X265_LOG2(x) (log((double)(x)) * 1.4426950408889640513713538072172)
#else
#define X265_LOG2F(x) log2f(x)
#define X265_LOG2(x)  log2(x)
#endif
#define X265_LOG2I(x) log2_fixpoint(x, 8) // Q24.8

#define INVALID_INDEX 0x3f

struct MV
{
   int16_t x;
   int16_t y;
};

/* output buffer */
typedef struct {
  struct node *next;
  u32 *pOutBuf;        /* Pointer to output stream buffer */
  ptr_t busOutBuf;       /* Bus address of output stream buffer */
  u32 outBufSize;      /* Size of output stream buffer in bytes */
} OutputBuffer;

/* Lookahead ctrl */
typedef struct {
#ifndef _WIN32
  pthread_t tid_lookahead;
  pthread_t tid_cutree;
#else
  HANDLE tid_lookahead;
  HANDLE tid_cutree;
#endif
  VCEncInst priv_inst;
  struct queue jobs;
  struct queue output;
  struct queue outbuf;
  pthread_mutex_t job_mutex;
  pthread_cond_t job_cond;
  pthread_mutex_t output_mutex;
  pthread_cond_t output_cond;
  bool bFlush;
  bool bError;
  VCEncRet status;
  i32 lastPoc;
  i32 lastGopPicIdx;
  i32 lastGopSize;
  VCEncPictureCodingType lastCodingType;
  i32 picture_cnt;
  i32 last_idr_picture_cnt;
  OutputBuffer internal_mem;
} VCEncLookahead;

/* frame level output of cu tree */
struct FrameOutput 
{
    //pixel *buffer[4];

    int    frameNum;         // Presentation frame number
    int    poc;              // Presentation frame number
    int    sliceType;        // Slice type decided by lookahead
    int    qp;
    double cost;
    char typeChar;
    u32 gopSize;
    double costGop[4];
    i32 FrameNumGop[4];
    double costAvg[4];
    i32 FrameTypeNum[4];
};
typedef struct {
  struct node *next;
  VCEncIn encIn;
  VCEncOut encOut;
  VCEncRet status;
  struct FrameOutput frame;
  char *qpDeltaBuf;
} VCEncLookaheadJob;

/* lowres buffers, sizes and strides */
struct Lowres 
{
    struct node *next;
    //pixel *buffer[4];

    int    frameNum;         // Presentation frame number
    int    poc;              // Presentation frame number
    int    sliceType;        // Slice type decided by lookahead
    int    qp;
//    int    width;            // width of lowres frame in pixels
//    int    lines;            // height of lowres frame in pixel lines
//    int    leadingBframes;   // number of leading B frames for P or I
//
//    bool   bScenecut;        // Set to false if the frame cannot possibly be part of a real scenecut.
//    bool   bKeyframe;
//    bool   bLastMiniGopBFrame;
//
//    double ipCostRatio;
//
//    /* lookahead output data */
//    int64_t   costEst[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
//    int64_t   costEstAq[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
//    int32_t*  rowSatds[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
//    int       intraMbs[X265_BFRAME_MAX + 2];
    int32_t*  intraCost;
//    uint8_t*  intraMode;
//    int64_t   satdCost;
//    uint16_t* lowresCostForRc;
    uint32_t* lowresCosts[X265_BFRAME_MAX + 2][X265_BFRAME_MAX + 2];
//    int32_t*  lowresMvCosts[2][X265_BFRAME_MAX + 2];
    struct MV*       lowresMvs[2][X265_BFRAME_MAX + 2];
//    uint32_t  maxBlocksInRow;
//    uint32_t  maxBlocksInCol;
    uint32_t  maxBlocksInRowFullRes;
//    uint32_t  maxBlocksInColFullRes;
//
//    /* used for vbvLookahead */
//    int       plannedType[X265_LOOKAHEAD_MAX + 1];
//    int64_t   plannedSatd[X265_LOOKAHEAD_MAX + 1];
//    int       indB;
//    int       bframes;
//
//    /* rate control / adaptive quant data */
    int32_t*   qpAqOffset;      // AQ QP offset values for each 16x16 CU
    int32_t*   qpCuTreeOffset;  // cuTree QP offset values for each 16x16 CU
//    double*   qpAqMotionOffset;
    int*      invQscaleFactor; // qScale values for qp Aq Offsets
    int*      invQscaleFactor8x8; // temporary buffer for qg-size 8
//    uint32_t* blockVariance;
//    uint64_t  wp_ssd[3];       // This is different than SSDY, this is sum(pixel^2) - sum(pixel)^2 for entire frame
//    uint64_t  wp_sum[3];
//    uint64_t  frameVariance;
//
//    /* cutree intermediate data */
    uint32_t* propagateCost;
    int weightedCostDelta[X265_BFRAME_MAX + 2];
//    //ReferencePlanes weightedRef[X265_BFRAME_MAX + 2];
//
    i32 p0;
    i32 p1;

    u32 cost;
    i32 predId;
    i32 gopEncOrder;
    char typeChar;
    i32 gopEnd;
    i32 gopSize;

    u32 motionScore[2][2];
    i32 motionNum[2];

    i32 aGopSize;
    i32 hieDepth;
    VCEncLookaheadJob *job;
    u32 ctuOffsetIdx;
    u32 cuDataIdx;
    u32 inRoiMapDeltaBinIdx;
    u32 outRoiMapDeltaQpIdx;
    u32 propagateCostIdx;
};
struct agop_res {
  struct node *next;
  int agop_size;
} ;
struct cuTreeCtr
{
    ENCPERF * perf;
    
    /* qComp sets the quantizer curve compression factor. It weights the frame
     * quantizer based on the complexity of residual (measured by lookahead).
     * Default value is 0.6. Increasing it to 1 will effectively generate CQP */
    double    qCompress;

    /* Enable weighted prediction in B slices. Default is disabled */
    int       bEnableWeightedBiPred;
    
    /* When enabled, the encoder will use the B frame in the middle of each
     * mini-GOP larger than 2 B frames as a motion reference for the surrounding
     * B frames.  This improves compression efficiency for a small performance
     * penalty.  Referenced B frames are treated somewhere between a B and a P
     * frame by rate control.  Default is enabled. */
    int       bBPyramid;
    
    /* The number of frames that must be queued in the lookahead before it may
     * make slice decisions. Increasing this value directly increases the encode
     * latency. The longer the queue the more optimally the lookahead may make
     * slice decisions, particularly with b-adapt 2. When cu-tree is enabled,
     * the length of the queue linearly increases the effectiveness of the
     * cu-tree analysis. Default is 40 frames, maximum is 250 */
    int       lookaheadDepth;

  
    /* Numerator and denominator of frame rate */
    uint32_t  fpsNum;
    uint32_t  fpsDenom;

    /* Sets the size of the VBV buffer in kilobits. Default is zero */
    int       vbvBufferSize;
   
    /* pre-lookahead */
    int           unitSize;
    int           unitCount;
    int           widthInUnit;
    int           heightInUnit;
    int           m_cuTreeStrength;
    u32 roiMapEnable;
    u32 width;
    u32 height;
    u32 max_cu_size;
    u32 bHWMultiPassSupport;
  
    int qgSize;

    int32_t*          m_scratch;         // temp buffer for cutree propagate
    int           frameNum;
    
    int nLookaheadFrames;         // frames in queue
    struct Lowres *lookaheadFramesBase[2*X265_LOOKAHEAD_MAX];
    struct Lowres **lookaheadFrames;

    double costCur;
    
    double costGop[4];
    i32 FrameNumGop[4];
    double costAvg[4];
    i32 FrameTypeNum[4];
    u32 costGopInt[4];
    u32 costAvgInt[4];

    i32 curTypeChar;
    i32 gopSize;
    i32 nextGopSize;
    i32 bBHierachy;
    bool bUpdateGop;
    i32 latestGopSize;
    i32 maxHieDepth;
    i32 cuInfoToRead;

    i32 inQpDeltaBlkSize;
    u32 dsRatio;
    VCEncInst pEncInst;
    EWLLinearMem_t roiMapDeltaQpMemFactory[CUTREE_BUFFER_NUM];
    u32 roiMapRefCnt[CUTREE_BUFFER_NUM];
    EWLLinearMem_t propagateCostMemFactory[X265_LOOKAHEAD_MAX+X265_BFRAME_MAX];
    u32 propagateCostRefCnt[X265_LOOKAHEAD_MAX+X265_BFRAME_MAX];
    VCEncVideoCodecFormat codecFormat;     /* Video Codec Format: HEVC/H264/AV1 */
#ifndef _WIN32
    pthread_t tid_cutree;
#else
    HANDLE tid_cutree;
#endif
    pthread_mutex_t cutree_mutex;
    pthread_cond_t cutree_cond;
    pthread_mutex_t roibuf_mutex;
    pthread_cond_t roibuf_cond;
    pthread_mutex_t cuinfobuf_mutex;
    pthread_cond_t cuinfobuf_cond;
    pthread_mutex_t agop_mutex;
    pthread_cond_t agop_cond;
    bool bFlush;
    bool bError;
    bool terminated;
    struct queue jobs;
    struct queue agop;
    i32 job_cnt;
    i32 output_cnt;
    i32 total_frames;

    // for cutree asic
    asicData_s asic;
    ptr_t  ctuOffset_Base;
    size_t ctuOffset_frame_size;
    ptr_t  cuData_Base;
    size_t cuData_frame_size;
    ptr_t  inRoiMapDeltaBin_Base;
    size_t inRoiMapDeltaBin_frame_size;
    ptr_t  outRoiMapDeltaQp_Base;
    size_t outRoiMapDeltaQp_frame_size;
    ptr_t  propagateCost_Base;
    size_t propagateCost_frame_size;
    struct FrameOutput output[9];
    i32 rem_frames;
    u64 commands[X265_LOOKAHEAD_MAX+X265_BFRAME_MAX];
    i32 num_cmds;
};

//Public API
/*
  called by VCEncStrmStart()
 */
VCEncRet cuTreeInit(struct cuTreeCtr* m_param, VCEncInst inst, const VCEncConfig *config);

/*
  called by VCEncStrmEncode()
 */
VCEncRet cuTreeAddFrame(VCEncInst inst, VCEncLookaheadJob *job);

/*
  called by VCEncStrmEnd()
 */
void PutRoiMapBufferToBufferPool(struct cuTreeCtr *m_param, u8 *addr);
void cuTreeRelease(struct cuTreeCtr* m_param, i32 error);
void cuTreeFlush(struct cuTreeCtr* m_param, i32 error);
i32 getFramePredId (i32 type);
i32 getPass1UpdatedGopSize(VCEncInst inst);
VCEncRet waitCuInfoBufPass1(struct vcenc_instance *vcenc_instance);

/*
 * Lookahead releated
 */


/* Initialization & release */
u8 *GetRoiMapBufferFromBufferPool(struct cuTreeCtr *m_param, ptr_t *busAddr);
VCEncRet StartLookaheadThread(VCEncLookahead *lookahead);
VCEncRet TerminateLookaheadThread(VCEncLookahead *lookahead, i32 error);
bool AddPictureToLookahead(VCEncLookahead *lookahead, const VCEncIn *pEncIn, VCEncOut *pEncOut);
VCEncLookaheadJob *GetLookaheadOutput(VCEncLookahead *lookahead, bool bFlush);
void ReleaseLookaheadPicture(VCEncLookahead *lookahead, VCEncLookaheadJob *output);
bool LookaheadEnqueueOutput(VCEncLookahead *lookahead, VCEncLookaheadJob *output);
/* cu tree asic */
VCEncRet VCEncCuTreeInit(struct cuTreeCtr *m_param);
VCEncRet VCEncCuTreeProcessOneFrame(struct cuTreeCtr *m_param);
VCEncRet VCEncCuTreeRelease(struct cuTreeCtr *pEncInst);
#endif
