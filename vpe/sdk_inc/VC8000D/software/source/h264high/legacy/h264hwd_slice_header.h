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

#ifndef H264HWD_SLICE_HEADER_H
#define H264HWD_SLICE_HEADER_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

#include "h264hwd_stream.h"
#include "h264hwd_cfg.h"
#include "h264hwd_seq_param_set.h"
#include "h264hwd_pic_param_set.h"
#include "h264hwd_nal_unit.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

enum {
  P_SLICE = 0,
  B_SLICE = 1,
  I_SLICE = 2
};

enum {NO_LONG_TERM_FRAME_INDICES = 0xFFFF};

/* macro to determine if slice is an inter slice, sliceTypes 0 and 5 */
#define IS_P_SLICE(slice_type) (((slice_type) == P_SLICE) || \
    ((slice_type) == P_SLICE + 5))

/* macro to determine if slice is an intra slice, sliceTypes 2 and 7 */
#define IS_I_SLICE(slice_type) (((slice_type) == I_SLICE) || \
    ((slice_type) == I_SLICE + 5))

#define IS_B_SLICE(slice_type) (((slice_type) == B_SLICE) || \
    ((slice_type) == B_SLICE + 5))

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/* structure to store data of one reference picture list reordering operation */
typedef struct {
  u32 reordering_of_pic_nums_idc;
  u32 abs_diff_pic_num;
  u32 long_term_pic_num;
  u32 abs_diff_view_idx;
} refPicListReorderingOperation_t;

/* structure to store reference picture list reordering operations */
typedef struct {
  u32 ref_pic_list_reordering_flag_l0;
  refPicListReorderingOperation_t command[MAX_NUM_REF_PICS+1];
} refPicListReordering_t;

/* structure to store data of one DPB memory management control operation */
typedef struct {
  u32 memory_management_control_operation;
  u32 difference_of_pic_nums;
  u32 long_term_pic_num;
  u32 long_term_frame_idx;
  u32 max_long_term_frame_idx;
} memoryManagementOperation_t;

/* worst case scenario: all MAX_NUM_REF_PICS pictures in the buffer are
 * short term pictures, each one of them is first marked as long term
 * reference picture which is then marked as unused for reference.
 * Additionally, max long-term frame index is set and current picture is
 * marked as long term reference picture. Last position reserved for
 * end memory_management_control_operation command */
#define MAX_NUM_MMC_OPERATIONS (2*MAX_NUM_REF_PICS+2+1)

/* structure to store decoded reference picture marking data */
typedef struct {
  u32 strm_len;
  u32 no_output_of_prior_pics_flag;
  u32 long_term_reference_flag;
  u32 adaptive_ref_pic_marking_mode_flag;
  memoryManagementOperation_t operation[MAX_NUM_MMC_OPERATIONS];
} decRefPicMarking_t;

/* structure to store slice header data decoded from the stream */
typedef struct {
  u32 first_mb_in_slice;
  u32 slice_type;
  u32 pic_parameter_set_id;
  u32 frame_num;
  u32 idr_pic_id;
  u32 poc_length;
  u32 poc_length_hw;
  u32 pic_order_cnt_lsb;
  i32 delta_pic_order_cnt_bottom;
  i32 delta_pic_order_cnt[2];
  u32 redundant_pic_cnt;
  u32 num_ref_idx_active_override_flag;
  u32 num_ref_idx_l0_active;
  u32 num_ref_idx_l1_active;
  i32 slice_qp_delta;
  u32 disable_deblocking_filter_idc;
  i32 slice_alpha_c0_offset;
  i32 slice_beta_offset;
  u32 slice_group_change_cycle;
  refPicListReordering_t ref_pic_list_reordering;
  refPicListReordering_t ref_pic_list_reordering_l1;
  decRefPicMarking_t dec_ref_pic_marking;
  u32 cabac_init_idc;
  u32 field_pic_flag;
  u32 bottom_field_flag;
  u32 direct_spatial_mv_pred_flag;
} sliceHeader_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdDecodeSliceHeader(strmData_t *p_strm_data,
                             sliceHeader_t *p_slice_header,
                             seqParamSet_t *p_seq_param_set,
                             picParamSet_t *p_pic_param_set,
                             nalUnit_t *p_nal_unit);

u32 h264bsdCheckPpsId(strmData_t *p_strm_data, u32 *pps_id, u32 *slice_type);

u32 h264bsdCheckFrameNum(
  strmData_t *p_strm_data,
  u32 max_frame_num,
  u32 *frame_num);

u32 h264bsdCheckIdrPicId(
  strmData_t *p_strm_data,
  u32 max_frame_num,
  nalUnitType_e nal_unit_type,
  u32 field_pic_flag,
  u32 *idr_pic_id);

u32 h264bsdCheckPicOrderCntLsb(
  strmData_t *p_strm_data,
  seqParamSet_t *p_seq_param_set,
  nalUnitType_e nal_unit_type,
  u32 *pic_order_cnt_lsb);

u32 h264bsdCheckDeltaPicOrderCntBottom(
  strmData_t *p_strm_data,
  seqParamSet_t *p_seq_param_set,
  nalUnitType_e nal_unit_type,
  i32 *delta_pic_order_cnt_bottom);

u32 h264bsdCheckDeltaPicOrderCnt(
  strmData_t *p_strm_data,
  seqParamSet_t *p_seq_param_set,
  nalUnitType_e nal_unit_type,
  u32 pic_order_present_flag,
  i32 *delta_pic_order_cnt);

u32 h264bsdCheckRedundantPicCnt(const strmData_t *p_strm_data,
                                const seqParamSet_t *p_seq_param_set,
                                const picParamSet_t *p_pic_param_set,
                                u32 *redundant_pic_cnt);

u32 h264bsdCheckPriorPicsFlag(u32 *no_output_of_prior_pics_flag,
                              const strmData_t *p_strm_data,
                              const seqParamSet_t *p_seq_param_set,
                              const picParamSet_t *p_pic_param_set);

u32 h264bsdFieldPicFlag(strmData_t * p_strm_data,
                        u32 max_frame_num,
                        nalUnitType_e nal_unit_type,
                        u32 field_pic_flag_present,
                        u32 *field_pic_flag);

u32 h264bsdBottomFieldFlag(strmData_t * p_strm_data,
                           u32 max_frame_num,
                           nalUnitType_e nal_unit_type,
                           u32 field_pic_flag,
                           u32 *bottom_field_flag);

u32 h264bsdIsOppositeFieldPic(sliceHeader_t * p_slice_curr,
                              sliceHeader_t * p_slice_prev,
                              u32 *second_field, u32 prev_ref_frame_num,
                              u32 new_picture);

u32 h264bsdCheckFieldPicFlag(strmData_t *p_strm_data,
                             u32 max_frame_num,
                             u32 field_pic_flag_present,
                             u32 *field_pic_flag);

u32 h264bsdCheckBottomFieldFlag(strmData_t *p_strm_data,
                                u32 max_frame_num,
                                u32 field_pic_flag,
                                u32 *bottom_field_flag);

u32 h264bsdCheckFirstMbInSlice(strmData_t * p_strm_data,
                               nalUnitType_e nal_unit_type,
                               u32 * first_mb_in_slice);

#endif /* #ifdef H264HWD_SLICE_HEADER_H */
