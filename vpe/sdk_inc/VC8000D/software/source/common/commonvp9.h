/*------------------------------------------------------------------------------
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

#ifndef COMMONSTRUCT_H
#define COMMONSTRUCT_H

#include "basetype.h"
#include "commonconfig.h"

#define VP9_REF_SCALE_SHIFT 14

#define NUM_REF_FRAMES 8
#define NUM_REF_FRAMES_LG2 3

#define ALLOWED_REFS_PER_FRAME 3

#define NUM_FRAME_CONTEXTS_LG2 2
#define NUM_FRAME_CONTEXTS (1 << NUM_FRAME_CONTEXTS_LG2)

#define DCPREDSIMTHRESH 0
#define DCPREDCNTTHRESH 3

#define PREDICTION_PROBS 3

#define DEFAULT_PRED_PROB_0 120
#define DEFAULT_PRED_PROB_1 80
#define DEFAULT_PRED_PROB_2 40

#define VP9_DEF_UPDATE_PROB 252

#define MBSKIP_CONTEXTS 3

#define MAX_MB_SEGMENTS 8
#define MB_SEG_TREE_PROBS (MAX_MB_SEGMENTS - 1)

#define MAX_REF_LF_DELTAS 4
#define MAX_MODE_LF_DELTAS 2

/* Segment Feature Masks */
#define SEGMENT_DELTADATA 0
#define SEGMENT_ABSDATA 1
#define MAX_MV_REFS 9

#define VP9_SWITCHABLE_FILTERS 3 /* number of switchable filters */

#define COMP_PRED_CONTEXTS 2

#define COEF_UPDATE_PROB 252
#define VP9_PROB_HALF 128
#define VP9_NMV_UPDATE_PROB 252
#define VP9_MV_UPDATE_PRECISION 7
#define MV_JOINTS 4
#define MV_CLASSES 11
#define CLASS0_BITS 1
#define CLASS0_SIZE (1 << CLASS0_BITS)
#define MV_OFFSET_BITS (MV_CLASSES + CLASS0_BITS - 2)

#define MV_MAX_BITS (MV_CLASSES + CLASS0_BITS + 2)
#define MV_MAX ((1 << MV_MAX_BITS) - 1)
#define MV_VALS ((MV_MAX << 1) + 1)

#define MAX_ENTROPY_TOKENS 12
#define ENTROPY_NODES 11

/* The first nodes of the entropy probs are unconstrained, the rest are
 * modeled with statistic distribution. */
#define UNCONSTRAINED_NODES 3
#define MODEL_NODES (ENTROPY_NODES - UNCONSTRAINED_NODES)
#define PIVOT_NODE 2  // which node is pivot
#define COEFPROB_MODELS 128

/* Entropy nodes above is divided in two parts, first three probs in part1
 * and the modeled probs in part2. Part1 is padded so that tables align with
 *  32 byte addresses, so there is four bytes for each table. */
#define ENTROPY_NODES_PART1 4
#define ENTROPY_NODES_PART2 8
#define INTER_MODE_CONTEXTS 7

#define INTRA_INTER_CONTEXTS 4
#define COMP_INTER_CONTEXTS 5
#define REF_CONTEXTS 5

#define BLOCK_TYPES 2
#define REF_TYPES 2  // intra=0, inter=1
#define COEF_BANDS 6
#define PREV_COEF_CONTEXTS 6

#define MODULUS_PARAM 13 /* Modulus parameter */

#define ACTIVE_HT 110  // quantization stepsize threshold

#define MAX_MV_REF_CANDIDATES 2

/* Coefficient token alphabet */

#define ZERO_TOKEN 0         /* 0         Extra Bits 0+0 */
#define ONE_TOKEN 1          /* 1         Extra Bits 0+1 */
#define TWO_TOKEN 2          /* 2         Extra Bits 0+1 */
#define THREE_TOKEN 3        /* 3         Extra Bits 0+1 */
#define FOUR_TOKEN 4         /* 4         Extra Bits 0+1 */
#define DCT_VAL_CATEGORY1 5  /* 5-6       Extra Bits 1+1 */
#define DCT_VAL_CATEGORY2 6  /* 7-10      Extra Bits 2+1 */
#define DCT_VAL_CATEGORY3 7  /* 11-18     Extra Bits 3+1 */
#define DCT_VAL_CATEGORY4 8  /* 19-34     Extra Bits 4+1 */
#define DCT_VAL_CATEGORY5 9  /* 35-66     Extra Bits 5+1 */
#define DCT_VAL_CATEGORY6 10 /* 67+       Extra Bits 13+1 */
#define DCT_EOB_TOKEN 11     /* EOB       Extra Bits 0+0 */
#define MAX_ENTROPY_TOKENS 12

#define DCT_EOB_MODEL_TOKEN 3 /* EOB       Extra Bits 0+0 */

typedef u32 vp9_coeff_count[REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
[UNCONSTRAINED_NODES + 1];
typedef u8 vp9_coeff_probs[REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
[UNCONSTRAINED_NODES];

#define BLOCK_SIZE_GROUPS 4

enum BlockSizeType {
  BLOCK_SIZE_AB4X4,
  BLOCK_SIZE_SB4X8,
  BLOCK_SIZE_SB8X4,
  BLOCK_SIZE_SB8X8,
  BLOCK_SIZE_SB8X16,
  BLOCK_SIZE_SB16X8,
  BLOCK_SIZE_MB16X16,
  BLOCK_SIZE_SB16X32,
  BLOCK_SIZE_SB32X16,
  BLOCK_SIZE_SB32X32,
  BLOCK_SIZE_SB32X64,
  BLOCK_SIZE_SB64X32,
  BLOCK_SIZE_SB64X64,
  BLOCK_SIZE_TYPES
};

enum PartitionType {
  PARTITION_NONE,
  PARTITION_HORZ,
  PARTITION_VERT,
  PARTITION_SPLIT,
  PARTITION_TYPES
};

#define PARTITION_PLOFFSET 4  // number of probability models per block size
#define NUM_PARTITION_CONTEXTS (4 * PARTITION_PLOFFSET)

enum FrameType {
  KEY_FRAME = 0,
  INTER_FRAME = 1,
  NUM_FRAME_TYPES,
};

enum MbPredictionMode {
  DC_PRED,  /* average of above and left pixels */
  V_PRED,   /* vertical prediction */
  H_PRED,   /* horizontal prediction */
  D45_PRED, /* Directional 45 deg prediction  [anti-clockwise from 0 deg hor] */
  D135_PRED, /* Directional 135 deg prediction [anti-clockwise from 0 deg hor]
                */
  D117_PRED, /* Directional 112 deg prediction [anti-clockwise from 0 deg hor]
                */
  D153_PRED, /* Directional 157 deg prediction [anti-clockwise from 0 deg hor]
                */
  D27_PRED, /* Directional 22 deg prediction  [anti-clockwise from 0 deg hor] */
  D63_PRED, /* Directional 67 deg prediction  [anti-clockwise from 0 deg hor] */
  TM_PRED,  /* Truemotion prediction */
  NEARESTMV,
  NEARMV,
  ZEROMV,
  NEWMV,
  SPLITMV,
  MB_MODE_COUNT
};

#define VP9_INTRA_MODES (TM_PRED + 1) /* 10 */

#define VP9_INTER_MODES (1 + NEWMV - NEARESTMV)

enum BlockPredictionMode {
  B_DC_PRED, /* average of above and left pixels */
  B_V_PRED,  /* vertical prediction */
  B_H_PRED,  /* horizontal prediction */
  B_D45_PRED,
  B_D135_PRED,
  B_D117_PRED,
  B_D153_PRED,
  B_D27_PRED,
  B_D63_PRED,
  B_TM_PRED,
  LEFT4X4,
  ABOVE4X4,
  ZERO4X4,
  NEW4X4,
  B_MODE_COUNT
};

#define VP9_BINTRAMODES (LEFT4X4)
#define VP9_SUBMVREFS (1 + NEW4X4 - LEFT4X4)

#define SUBMVREF_COUNT 5

#define SUBPEL_SHIFTS 16

/* Integer pel reference mv threshold for use of high-precision 1/8 mv */
#define COMPANDED_MVREF_THRESH 8

#define TX_SIZE_CONTEXTS 2

enum InterpolationFilterType {
  EIGHTTAP_SMOOTH,
  EIGHTTAP,
  EIGHTTAP_SHARP,
  BILINEAR,
  SWITCHABLE /* should be the last one */
};

extern const enum InterpolationFilterType
vp9_switchable_interp[VP9_SWITCHABLE_FILTERS];

enum CompPredModeType {
  SINGLE_PREDICTION_ONLY = 0,
  COMP_PREDICTION_ONLY = 1,
  HYBRID_PREDICTION = 2,
  NB_PREDICTION_TYPES = 3,
};

enum TxfmMode {
  ONLY_4X4 = 0,
  ALLOW_8X8 = 1,
  ALLOW_16X16 = 2,
  ALLOW_32X32 = 3,
  TX_MODE_SELECT = 4,
  NB_TXFM_MODES = 5,
};

enum MvReferenceFrame {
  NONE = -1,
  INTRA_FRAME = 0,
  LAST_FRAME = 1,
  GOLDEN_FRAME = 2,
  ALTREF_FRAME = 3,
  MAX_REF_FRAMES = 4
};

enum SegLevelFeatures {
  SEG_LVL_ALT_Q = 0,
  SEG_LVL_ALT_LF = 1,
  SEG_LVL_REF_FRAME = 2,
  SEG_LVL_SKIP = 3,
  SEG_LVL_MAX = 4
};

enum {
  VP9_SEG_FEATURE_DELTA,
  VP9_SEG_FEATURE_ABS
};

static const int vp9_seg_feature_data_signed[SEG_LVL_MAX] = {1, 1, 0, 0};
static const int vp9_seg_feature_data_max[SEG_LVL_MAX] = {255, 63, 3, 0};

enum TxSize {
  TX_4X4 = 0,
  TX_8X8 = 1,
  TX_16X16 = 2,
  TX_32X32 = 3,
  TX_SIZE_MAX_SB,
};

enum TxType {
  DCT_DCT = 0,
  ADST_DCT = 1,
  DCT_ADST = 2,
  ADST_ADST = 3
};

enum SplitMvPartitioningType {
  PARTITIONING_16X8 = 0,
  PARTITIONING_8X16,
  PARTITIONING_8X8,
  PARTITIONING_4X4,
  NB_PARTITIONINGS,
};

enum PredId {
  PRED_SEG_ID = 0,
  PRED_MBSKIP = 1,
  PRED_SWITCHABLE_INTERP = 2,
  PRED_INTRA_INTER = 3,
  PRED_COMP_INTER_INTER = 4,
  PRED_SINGLE_REF_P1 = 5,
  PRED_SINGLE_REF_P2 = 6,
  PRED_COMP_REF_P = 7,
  PRED_TX_SIZE = 8
};

/* Symbols for coding which components are zero jointly */
enum MvJointType {
  MV_JOINT_ZERO = 0,   /* Zero vector */
  MV_JOINT_HNZVZ = 1,  /* Vert zero, hor nonzero */
  MV_JOINT_HZVNZ = 2,  /* Hor zero, vert nonzero */
  MV_JOINT_HNZVNZ = 3, /* Both components nonzero */
};

/* Symbols for coding magnitude class of nonzero components */
enum MvClassType {
  MV_CLASS_0 = 0,   /* (0, 2]     integer pel */
  MV_CLASS_1 = 1,   /* (2, 4]     integer pel */
  MV_CLASS_2 = 2,   /* (4, 8]     integer pel */
  MV_CLASS_3 = 3,   /* (8, 16]    integer pel */
  MV_CLASS_4 = 4,   /* (16, 32]   integer pel */
  MV_CLASS_5 = 5,   /* (32, 64]   integer pel */
  MV_CLASS_6 = 6,   /* (64, 128]  integer pel */
  MV_CLASS_7 = 7,   /* (128, 256] integer pel */
  MV_CLASS_8 = 8,   /* (256, 512] integer pel */
  MV_CLASS_9 = 9,   /* (512, 1024] integer pel */
  MV_CLASS_10 = 10, /* (1024,2048] integer pel */
};

struct NmvContext {
  /* last bytes of address 41 */
  u8 joints[MV_JOINTS - 1];
  u8 sign[2];
  /* address 42 */
  u8 class0[2][CLASS0_SIZE - 1];
  u8 fp[2][4 - 1];
  u8 class0_hp[2];
  u8 hp[2];
  u8 classes[2][MV_CLASSES - 1];
  /* address 43 */
  u8 class0_fp[2][CLASS0_SIZE][4 - 1];
  u8 bits[2][MV_OFFSET_BITS];
};

struct NmvContextCounts {
  u32 joints[MV_JOINTS];
  u32 sign[2][2];
  u32 classes[2][MV_CLASSES];
  u32 class0[2][CLASS0_SIZE];
  u32 bits[2][MV_OFFSET_BITS][2];
  u32 class0_fp[2][CLASS0_SIZE][4];
  u32 fp[2][4];
  u32 class0_hp[2][2];
  u32 hp[2][2];
};

typedef u8 vp9_prob;

#define VP9HWPAD(x, y) u8 x[y]

/* Adaptive entropy contexts, padding elements are added to have
 * 256 bit aligned tables for HW access.
 * Compile with TRACE_PROB_TABLES to print bases for each table. */
struct Vp9AdaptiveEntropyProbs {
  /* address 32 */
  u8 inter_mode_prob[INTER_MODE_CONTEXTS][4];
  u8 intra_inter_prob[INTRA_INTER_CONTEXTS];

  /* address 33 */
  u8 uv_mode_prob[VP9_INTRA_MODES][8];
  u8 tx8x8_prob[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB - 3];
  u8 tx16x16_prob[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB - 2];
  u8 tx32x32_prob[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB - 1];
  u8 sb_ymode_prob_b[BLOCK_SIZE_GROUPS][1];
  u8 sb_ymode_prob[BLOCK_SIZE_GROUPS][8];

  /* address 37 */
  u8 partition_prob[NUM_FRAME_TYPES][NUM_PARTITION_CONTEXTS][PARTITION_TYPES];

  /* address 41 */
  u8 uv_mode_prob_b[VP9_INTRA_MODES][1];
  u8 switchable_interp_prob[VP9_SWITCHABLE_FILTERS + 1][VP9_SWITCHABLE_FILTERS -
      1];
  u8 comp_inter_prob[COMP_INTER_CONTEXTS];
  u8 mbskip_probs[MBSKIP_CONTEXTS];
  VP9HWPAD(pad1, 1);

  struct NmvContext nmvc;

  /* address 44 */
  u8 single_ref_prob[REF_CONTEXTS][2];
  u8 comp_ref_prob[REF_CONTEXTS];
  VP9HWPAD(pad2, 17);

  /* address 45 */
  u8 prob_coeffs[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
  [ENTROPY_NODES_PART1];
  u8 prob_coeffs8x8[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
  [ENTROPY_NODES_PART1];
  u8 prob_coeffs16x16[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
  [ENTROPY_NODES_PART1];
  u8 prob_coeffs32x32[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
  [ENTROPY_NODES_PART1];
};

/* Entropy contexts */
struct Vp9EntropyProbs {
  /* Default keyframe probs */
  /* Table formatted for 256b memory, probs 0to7 for all tables followed by
   * probs 8toN for all tables.
   * Compile with TRACE_PROB_TABLES to print bases for each table. */

  u8 kf_bmode_prob[VP9_INTRA_MODES][VP9_INTRA_MODES][8];

  /* Address 25 */
  u8 kf_bmode_prob_b[VP9_INTRA_MODES][VP9_INTRA_MODES][1];
  u8 ref_pred_probs[PREDICTION_PROBS];
  u8 mb_segment_tree_probs[MB_SEG_TREE_PROBS];
  u8 segment_pred_probs[PREDICTION_PROBS];
  u8 ref_scores[MAX_REF_FRAMES];
  u8 prob_comppred[COMP_PRED_CONTEXTS];
  VP9HWPAD(pad1, 9);

  /* Address 29 */
  u8 kf_uv_mode_prob[VP9_INTRA_MODES][8];
  u8 kf_uv_mode_prob_b[VP9_INTRA_MODES][1];
  VP9HWPAD(pad2, 6);

  struct Vp9AdaptiveEntropyProbs a; /* Probs with backward adaptation */
};

/* Counters for adaptive entropy contexts */
struct Vp9EntropyCounts {
  u32 inter_mode_counts[INTER_MODE_CONTEXTS][VP9_INTER_MODES - 1][2];
  u32 sb_ymode_counts[BLOCK_SIZE_GROUPS][VP9_INTRA_MODES];
  u32 uv_mode_counts[VP9_INTRA_MODES][VP9_INTRA_MODES];
  u32 partition_counts[NUM_PARTITION_CONTEXTS][PARTITION_TYPES];
  u32 switchable_interp_counts[VP9_SWITCHABLE_FILTERS +
                               1][VP9_SWITCHABLE_FILTERS];
  u32 intra_inter_count[INTRA_INTER_CONTEXTS][2];
  u32 comp_inter_count[COMP_INTER_CONTEXTS][2];
  u32 single_ref_count[REF_CONTEXTS][2][2];
  u32 comp_ref_count[REF_CONTEXTS][2];
  u32 tx32x32_count[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB];
  u32 tx16x16_count[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB - 1];
  u32 tx8x8_count[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB - 2];
  u32 mbskip_count[MBSKIP_CONTEXTS][2];

  struct NmvContextCounts nmvcount;

  u32 count_coeffs[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
  [UNCONSTRAINED_NODES + 1];
  u32 count_coeffs8x8[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
  [UNCONSTRAINED_NODES + 1];
  u32 count_coeffs16x16[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
  [UNCONSTRAINED_NODES + 1];
  u32 count_coeffs32x32[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
  [UNCONSTRAINED_NODES + 1];

  u32 count_eobs[TX_SIZE_MAX_SB][BLOCK_TYPES][REF_TYPES][COEF_BANDS]
  [PREV_COEF_CONTEXTS];
};

#endif /* COMMONSTRUCT_H */
