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

#ifndef H264HWD_PIC_PARAM_SET_H
#define H264HWD_PIC_PARAM_SET_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

#include "h264hwd_stream.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/* data structure to store PPS information decoded from the stream */
typedef struct {
  u32 pic_parameter_set_id;
  u32 seq_parameter_set_id;
  u32 pic_order_present_flag;
  u32 num_slice_groups;
  u32 slice_group_map_type;
  u32 *run_length;
  u32 *top_left;
  u32 *bottom_right;
  u32 slice_group_change_direction_flag;
  u32 slice_group_change_rate;
  u32 pic_size_in_map_units;
  u32 *slice_group_id;
  u32 num_ref_idx_l0_active;
  u32 num_ref_idx_l1_active;
  u32 pic_init_qp;
  i32 chroma_qp_index_offset;
  i32 chroma_qp_index_offset2;
  u32 deblocking_filter_control_present_flag;
  u32 constrained_intra_pred_flag;
  u32 redundant_pic_cnt_present_flag;
  u32 entropy_coding_mode_flag;
  u32 weighted_pred_flag;
  u32 weighted_bi_pred_idc;
  u32 transform8x8_flag;
  u32 scaling_matrix_present_flag;
  u32 scaling_list_present[8];
  u32 use_default_scaling[8];
  u8 scaling_list[8][64];
} picParamSet_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdDecodePicParamSet(strmData_t *p_strm_data,
                             picParamSet_t *p_pic_param_set);

#endif /* #ifdef H264HWD_PIC_PARAM_SET_H */
