/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Inc. All rights reserved        --
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

#ifndef H264HWD_MACROBLOCK_LAYER_H
#define H264HWD_MACROBLOCK_LAYER_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

#include "h264hwd_stream.h"
#include "h264hwd_slice_header.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/* Macro to determine if a mb is an intra mb */
#define IS_INTRA_MB(a) ((a).mb_type > 5)

/* Macro to determine if a mb is an I_PCM mb */
#define IS_I_PCM_MB(a) ((a).mb_type == 31)

typedef enum MbType {
  P_Skip = 0,
  P_L0_16x16 = 1,
  P_L0_L0_16x8 = 2,
  P_L0_L0_8x16 = 3,
  P_8x8 = 4,
  P_8x8ref0 = 5,
  I_4x4 = 6,
  I_16x16_0_0_0 = 7,
  I_16x16_1_0_0 = 8,
  I_16x16_2_0_0 = 9,
  I_16x16_3_0_0 = 10,
  I_16x16_0_1_0 = 11,
  I_16x16_1_1_0 = 12,
  I_16x16_2_1_0 = 13,
  I_16x16_3_1_0 = 14,
  I_16x16_0_2_0 = 15,
  I_16x16_1_2_0 = 16,
  I_16x16_2_2_0 = 17,
  I_16x16_3_2_0 = 18,
  I_16x16_0_0_1 = 19,
  I_16x16_1_0_1 = 20,
  I_16x16_2_0_1 = 21,
  I_16x16_3_0_1 = 22,
  I_16x16_0_1_1 = 23,
  I_16x16_1_1_1 = 24,
  I_16x16_2_1_1 = 25,
  I_16x16_3_1_1 = 26,
  I_16x16_0_2_1 = 27,
  I_16x16_1_2_1 = 28,
  I_16x16_2_2_1 = 29,
  I_16x16_3_2_1 = 30,
  I_PCM = 31
} mbType_e;

typedef enum SubMbType {
  P_L0_8x8 = 0,
  P_L0_8x4 = 1,
  P_L0_4x8 = 2,
  P_L0_4x4 = 3
} subMbType_e;

typedef enum MbPartMode {
  MB_P_16x16 = 0,
  MB_P_16x8,
  MB_P_8x16,
  MB_P_8x8
} mbPartMode_e;

typedef enum SubMbPartMode {
  MB_SP_8x8 = 0,
  MB_SP_8x4,
  MB_SP_4x8,
  MB_SP_4x4
} subMbPartMode_e;

typedef enum MbPartPredMode {
  PRED_MODE_INTRA4x4 = 0,
  PRED_MODE_INTRA16x16,
  PRED_MODE_INTER
} mbPartPredMode_e;

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

typedef struct MV {
  i16 hor;
  i16 ver;
} mv_t;

typedef struct MbPred {
  u32 prev_intra4x4_pred_mode_flag[16];
  u32 rem_intra4x4_pred_mode[16];
  u32 intra_chroma_pred_mode;
} mbPred_t;

typedef struct SubMbPred {
  subMbType_e sub_mb_type[4];
} subMbPred_t;

typedef struct {
  u16 rlc[468];
  u8 total_coeff[28];
} residual_t;

typedef struct MacroblockLayer {
  /*u32 disable_deblocking_filter_idc; */
  i32 filter_offset_a;
  i32 filter_offset_b;
  u32 disable_deblocking_filter_idc;
  mbType_e mb_type;
  u32 coded_block_pattern;
  i32 mb_qp_delta;
  mbPred_t mb_pred;
  subMbPred_t sub_mb_pred;
  residual_t residual;
} macroblockLayer_t;

typedef struct MbStorage {
  mbType_e mb_type;
  mbType_e mb_type_asic;
  u32 slice_id;
  /*u32 disable_deblocking_filter_idc; */
  /*i32 filter_offset_a; */
  /*i32 filter_offset_b; */
  u32 qp_y;
  /*i32 chromaQpIndexOffset; */
  u8 total_coeff[24];
  u8 intra4x4_pred_mode[16];
  u8 intra4x4_pred_mode_asic[16];
  /* u32 refPic[4]; */
  u8 ref_idx_l0[4];
  u8 ref_id[4];
  mv_t mv[16];
  u32 decoded;
  struct MbStorage *mb_a;
  struct MbStorage *mb_b;
  struct MbStorage *mb_c;
  struct MbStorage *mb_d;
} mbStorage_t;

struct cabac_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdDecodeMacroblockLayerCavlc(strmData_t * p_strm_data,
                                      macroblockLayer_t * p_mb_layer,
                                      mbStorage_t * p_mb,
                                      const sliceHeader_t * p_slice_hdr );

u32 h264bsdDecodeMacroblockLayerCabac(strmData_t * p_strm_data,
                                      macroblockLayer_t * p_mb_layer,
                                      mbStorage_t * p_mb,
                                      const sliceHeader_t * p_slice_hdr,
                                      struct cabac_s * p_cabac );

u32 h264bsdNumSubMbPart(subMbType_e sub_mb_type);

subMbPartMode_e h264bsdSubMbPartMode(subMbType_e sub_mb_type);

u32 h264bsdPredModeIntra16x16(mbType_e mb_type);

mbPartPredMode_e h264bsdMbPartPredMode(mbType_e mb_type);

#endif /* #ifdef H264HWD_MACROBLOCK_LAYER_H */
