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

#ifndef INSTANCE_H
#define INSTANCE_H

#include "container.h"
#include "enccommon.h"
#include "encpreprocess.h"
#include "encasiccontroller.h"

#include "hevcencapi.h"     /* Callback type from API is reused */
#include "rate_control_picture.h"
#include "hash.h"
#include "sw_cu_tree.h"
#ifdef FB_SYSLOG_ENABLE
#include "syslog_sink.h"
#endif

struct vcenc_instance
{
#ifdef FB_SYSLOG_ENABLE
  LOG_INFO_HEADER log_header;
  char module_name[10];
  int enc_index;
#endif
  ENCPERF * perf;

  u32 encStatus;
  asicData_s asic;
  i32 vps_id;     /* Video parameter set id */
  i32 sps_id;     /* Sequence parameter set id */
  i32 pps_id;     /* Picture parameter set id */
  i32 rps_id;     /* Reference picture set id */

  struct buffer stream;
  struct buffer streams[MAX_CORE_NUM];

  EWLHwConfig_t featureToSupport;
  EWLHwConfig_t asic_core_cfg[MAX_SUPPORT_CORE_NUM];
  int get_cfg_flag;
  u32 asic_hw_id[MAX_SUPPORT_CORE_NUM];
  u32 reserve_core_info;

  u8 *temp_buffer;      /* Data buffer, user set */
  u32 temp_size;      /* Size (bytes) of buffer */
  u32 temp_bufferBusAddress;

  /* add for idr resend ssp header */
  u8 *header_buffer;      /* Data buffer, user set */
  u32 header_size;      /* Size (bytes) of buffer */
  u32 headerBufferBusAddress;

  // SPS&PPS parameters
  i32 max_cu_size;    /* Max coding unit size in pixels */
  i32 min_cu_size;    /* Min coding unit size in pixels */
  i32 max_tr_size;    /* Max transform size in pixels */
  i32 min_tr_size;    /* Min transform size in pixels */
  i32 tr_depth_intra;   /* Max transform hierarchy depth */
  i32 tr_depth_inter;   /* Max transform hierarchy depth */
  i32 width;
  i32 height;
  i32 enableScalingList;  /* */
  VCEncVideoCodecFormat codecFormat;     /* Video Codec Format: HEVC/H264/AV1 */
  i32 pcm_enabled_flag;  /*enable pcm for HEVC*/
  i32 pcm_loop_filter_disabled_flag;  /*pcm filter*/

  // intra setup
  u32 strong_intra_smoothing_enabled_flag;
  u32 constrained_intra_pred_flag;
  i32 enableDeblockOverride;
  i32 disableDeblocking;

  i32 tc_Offset;

  i32 beta_Offset;

  i32 ctbPerFrame;
  i32 ctbPerRow;
  i32 ctbPerCol;

  /* Minimum luma coding block size of coding units that convey
   * cu_qp_delta_abs and cu_qp_delta_sign and default quantization
   * parameter */
  i32 min_qp_size;
  i32 qpHdr;

  i32 levelIdx;   /*level 5.1 =8*/
  i32 level;   /*level 5.1 =8*/

  i32 profile;   /**/
  i32 tier;

  preProcess_s preProcess;

  /* Rate control parameters */
  vcencRateControl_s rateControl;


  struct vps *vps;
  struct sps *sps;

  i32 poc;      /* encoded Picture order count */
  i32 frameNum; /* frame number in decoding order, 0 for IDR, +1 for each reference frame; used for H.264 */
  i32 idrPicId; /* idrPicId in H264, to distinguish subsequent idr pictures */
  u8 *lum;
  u8 *cb;
  u8 *cr;
  i32 chromaQpOffset;
  i32 enableSao;
  i32 output_buffer_over_flow;
  const void *inst;

  /* H.264 MMO */
  i32 h264_mmo_nops;
  i32 h264_mmo_unref[VCENC_MAX_REF_FRAMES];
  i32 h264_mmo_ltIdx[VCENC_MAX_REF_FRAMES];
  i32 h264_mmo_long_term_flag[VCENC_MAX_REF_FRAMES];

  VCEncSliceReadyCallBackFunc sliceReadyCbFunc;
  void *pAppData[MAX_CORE_NUM];         /* User given application specific data */
  u32 frameCnt;
  u32 videoFullRange;
  u32 sarWidth;
  u32 sarHeight;
  i32 fieldOrder;     /* 1=top first, 0=bottom first */
  u32 interlaced;

  i32 gdrEnabled;
  i32 gdrStart;
  i32 gdrDuration;
  i32 gdrCount;
  i32 gdrAverageMBRows;
  i32 gdrMBLeft;
  i32 gdrFirstIntraFrame;
  u32 roi1Enable;
  u32 roi2Enable;
  u32 roi3Enable;
  u32 roi4Enable;
  u32 roi5Enable;
  u32 roi6Enable;
  u32 roi7Enable;
  u32 roi8Enable;
  u32 ctbRCEnable;
  i32 blockRCSize;
  u32 roiMapEnable;
  u32 RoimapCuCtrl_index_enable;
  u32 RoimapCuCtrl_enable;
  u32 RoimapCuCtrl_ver;
  u32 RoiQpDelta_ver;
  u32 numNalus[MAX_CORE_NUM];        /* Amount of NAL units */
  u32 testId;
  i32 created_pic_num; /* number of pictures currently created */
  /* low latency */
  inputLineBuf_s inputLineBuf;
#if USE_TOP_CTRL_DENOISE
    unsigned int uiFrmNum;
    unsigned int uiNoiseReductionEnable;
    int FrmNoiseSigmaSmooth[5];
    int iFirstFrameSigma;
    int iNoiseL;
    int iSigmaCur;
    int iThreshSigmaCur;
    int iThreshSigmaPrev;
    int iSigmaCalcd;
    int iThreshSigmaCalcd;
    int iSliceQPPrev;
#endif
    i32 insertNewPPS;
    i32 insertNewPPSId;
    i32 maxPPSId;

    u32 maxTLayers; /*max temporal layers*/

    u32 rdoLevel;
    hashctx hashctx;

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

  u32 verbose; /* Log printing mode */
  u32 dumpRegister;

  /* for tile*/
  i32 tiles_enabled_flag;
  i32 num_tile_columns;
  i32 num_tile_rows;
  i32 loop_filter_across_tiles_enabled_flag;

  /* L2-cache */
#if 0 //def SUPPORT_CACHE
  void *cache;
  u32 channel_idx;
#endif

#ifdef SUPPORT_DEC400
  void * f2h;
#endif
#ifdef SUPPORT_L2CACHE
  void * l2h;
#endif
  u32 input_alignment;
  u32 ref_alignment;
  u32 ref_ch_alignment;

  Hdr10DisplaySei    Hdr10Display;
  Hdr10LightLevelSei Hdr10LightLevel;
  Hdr10ColorVui      Hdr10Color;

  u32 RpsInSliceHeader;
  HWRPS_CONFIG sHwRps;

  bool rasterscan;

  /* cu tree look ahead queue */
  u32 pass;
  struct cuTreeCtr cuTreeCtl;
  bool bSkipCabacEnable;
  bool bRDOQEnable;
  u32 lookaheadDepth;
  VCEncLookahead lookahead;
  u32 numCuInfoBuf;
  u32 cuInfoBufIdx;
  bool bMotionScoreEnable;
  u32 extDSRatio; /*0=1:1, 1=1:2*/

  // add for ds first pass
  i32 width_ds;
  i32 height_ds;


  /* Multi-core parallel ctr */
  struct sw_picture *pic[MAX_CORE_NUM];
  u32 parallelCoreNum;
  u32 jobCnt;
  u32 reservedCore;
  VCEncStrmBufs streamBufs[MAX_CORE_NUM];  /* output stream buffers */

  /*stream Multi-segment ctrl*/
  streamMultiSeg_s streamMultiSegment;

  /* internal mem for pass 1 */
  EWLLinearMem_t internalMem;

  EWLLinearMem_t outputMem;
};

struct instance
{
  struct vcenc_instance vcenc_instance; /* Visible to user */
  struct vcenc_instance *instance;   /* Instance sanity check */
  struct container container;   /* Encoder internal store */
};

struct container *get_container(struct vcenc_instance *instance);

#define IS_HEVC(a)  (a==VCENC_VIDEO_CODEC_HEVC)
#define IS_H264(a)  (a==VCENC_VIDEO_CODEC_H264)
#define IS_AV1(a)   (a==VCENC_VIDEO_CODEC_AV1)

#endif
