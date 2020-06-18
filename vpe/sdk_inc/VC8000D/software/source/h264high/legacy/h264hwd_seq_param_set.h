/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Holdings Co., Ltd. All rights reserved        --
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

#ifndef H264HWD_SEQ_PARAM_SET_H
#define H264HWD_SEQ_PARAM_SET_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

#include "h264hwd_cfg.h"
#include "h264hwd_stream.h"
#include "h264hwd_vui.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/* structure to store sequence parameter set information decoded from the
 * stream */
typedef struct {
  u32 profile_idc;
  u32 level_idc;
  u8 constrained_set0_flag;
  u8 constrained_set1_flag;
  u8 constrained_set2_flag;
  u8 constrained_set3_flag;
  u32 seq_parameter_set_id;
  u32 max_frame_num;
  u32 pic_order_cnt_type;
  u32 max_pic_order_cnt_lsb;
  u32 delta_pic_order_always_zero_flag;
  i32 offset_for_non_ref_pic;
  i32 offset_for_top_to_bottom_field;
  u32 num_ref_frames_in_pic_order_cnt_cycle;
  i32 *offset_for_ref_frame;
  u32 num_ref_frames;
  u32 gaps_in_frame_num_value_allowed_flag;
  u32 pic_width_in_mbs;
  u32 pic_height_in_mbs;
  u32 frame_cropping_flag;
  u32 frame_crop_left_offset;
  u32 frame_crop_right_offset;
  u32 frame_crop_top_offset;
  u32 frame_crop_bottom_offset;
  u32 vui_parameters_present_flag;
  vuiParameters_t *vui_parameters;
  u32 max_dpb_size;
  u32 frame_mbs_only_flag;
  u32 mb_adaptive_frame_field_flag;
  u32 direct8x8_inference_flag;
  u32 chroma_format_idc;
  u32 mono_chrome;
  u32 bit_depth_luma;
  u32 bit_depth_chroma;
  u32 scaling_matrix_present_flag;
  u32 scaling_list_present[8];
  u32 use_default_scaling[8];
  u8 scaling_list[8][64];

  /* mvc extension */
  struct {
    u32 num_views;
    u32 view_id[MAX_NUM_VIEWS];
  } mvc;
} seqParamSet_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdDecodeSeqParamSet(strmData_t *p_strm_data,
                             seqParamSet_t *p_seq_param_set, u32 mvc_flag);

u32 h264bsdCompareSeqParamSets(seqParamSet_t *p_sps1, seqParamSet_t *p_sps2);

#endif /* #ifdef H264HWD_SEQ_PARAM_SET_H */
