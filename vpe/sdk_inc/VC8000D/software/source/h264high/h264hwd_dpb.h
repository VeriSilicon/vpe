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

#ifndef H264HWD_DPB_H
#define H264HWD_DPB_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "h264decapi.h"

#include "h264hwd_slice_header.h"
#include "h264hwd_image.h"
#include "h264hwd_dpb_lock.h"

#include "dwl.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/* enumeration to represent status of buffered image */
typedef enum {
  UNUSED = 0,
  NON_EXISTING,
  SHORT_TERM,
  LONG_TERM,
  EMPTY
} dpbPictureStatus_e;

/* structure to represent a buffered picture */
typedef struct dpbPicture {
  u32 mem_idx;
  struct DWLLinearMem *data;
  struct DWLLinearMem *ds_data;
  i32 pic_num;
  u32 frame_num;
  i32 pic_order_cnt[2];
  dpbPictureStatus_e status[2];
  u32 to_be_displayed;
  u32 pic_id;
  u32 pic_code_type[2];
  i32 num_err_mbs;
  u32 is_idr[2];
  u32 decode_id[2];
  u32 is_field_pic;
  u32 is_bottom_field;
  u32 tiled_mode;
  H264CropParams crop;
  double dpb_output_time[2];
  u32 pic_struct;
  u32 pic_width;
  u32 pic_height;
  u32 sar_width;
  u32 sar_height;
  u32 bit_depth_luma;
  u32 bit_depth_chroma;
  u32 openB_flag;
  u32 corrupted_first_field_or_frame;
  u32 corrupted_second_field;
  u32 mono_chrome;
  u32 cycles_per_mb;
} dpbPicture_t;

/* structure to represent display image output from the buffer */
typedef struct {
  u32 mem_idx;
  struct DWLLinearMem *data;
  struct DWLLinearMem *pp_data;
  u32 pic_id;
  u32 pic_code_type[2];
  i32 num_err_mbs;
  u32 is_idr[2];
  u32 decode_id[2];
  u32 interlaced;
  u32 field_picture;
  u32 top_field;
  u32 tiled_mode;
  H264CropParams crop;
  u32 pic_struct;
  u32 pic_width;
  u32 pic_height;
  u32 sar_width;
  u32 sar_height;
  u32 bit_depth_luma;
  u32 bit_depth_chroma;
  u32 mono_chrome;
  u32 corrupted_second_field;
  u32 is_openb;
  u32 cycles_per_mb;
} dpbOutPicture_t;

typedef struct buffStatus {
  u32 n_ref_count;
  u32 usage_mask;
} buffStatus_t;

/* structure to represent DPB */
typedef struct dpbStorage {
  dpbPicture_t buffer[16 + 1];
  u32 list[16 + 1];
  dpbPicture_t *current_out;
  double cpb_removal_time;
  u32 bumping_flag;
  u32 current_out_pos;
  dpbOutPicture_t *out_buf;
  u32 num_out;
  u32 out_index_w;
  u32 out_index_r;
  u32 max_ref_frames;
  u32 dpb_size;
  u32 max_frame_num;
  u32 max_long_term_frame_idx;
  u32 num_ref_frames;
  u32 fullness;
  u32 prev_ref_frame_num;
  u32 last_contains_mmco5;
  u32 no_reordering;
  u32 flushed;
  u32 pic_size_in_mbs;
  u32 dir_mv_offset;
  u32 sync_mc_offset;
  /* compression table offset to dpb buffer base address */
  u32 cbs_ytbl_offset;
  u32 cbs_ctbl_offset;

  struct DWLLinearMem poc;
  u32 delayed_out;
  u32 delayed_id;
  u32 interlaced;
  u32 ch2_offset;

  u32 tot_buffers;
#ifdef USE_OUTPUT_RELEASE
  u32 tot_buffers_reserved;
#endif
  struct DWLLinearMem pic_buffers[MAX_FRAME_BUFFER_NUMBER];
  u32 pic_buff_id[MAX_FRAME_BUFFER_NUMBER];

  /* flag to prevent output when display smoothing is used and second field
   * of a picture was just decoded */
  u32 no_output;

  u32 prev_out_idx;

  u32 pic_num_invalid[16+1+16+1];
  u32 invalid_pic_num_count;

  FrameBufferList *fb_list;
  u32 ref_id[16];
  u32 pic_width;
  u32 pic_height;
  u32 bit_depth_luma;
  u32 bit_depth_chroma;
  u32 mono_chrome;

  /* Try to recover DPB from chaos with error streams. */
  u32 try_recover_dpb;

#ifdef USE_EXTERNAL_BUFFER
  u32 use_adaptive_buffers;
  u32 n_guard_size;
  u32 b_updated;
  u32 n_ext_buf_size_added;   /* size of external buffer added */
  u32 n_new_pic_size;        /* pic size for new sequence (temp). */
#endif

  void *storage;
} dpbStorage_t;

typedef struct dpbInitParams {
  u32 pic_size_in_mbs;
  u32 pic_width_in_mbs;
  u32 pic_height_in_mbs;
  u32 n_extra_frm_buffers;
  u32 dpb_size;
  u32 max_ref_frames;
  u32 max_frame_num;
  u32 no_reordering;
  u32 display_smoothing;
  u32 mono_chrome;
  u32 is_high_supported;
  u32 enable2nd_chroma;
  u32 multi_buff_pp;
  u32 n_cores;
  u32 mvc_view;
  u32 pp_width;
  u32 pp_height;
  u32 pp_stride;
  u32 pixel_width;
  u32 is_high10_supported;  /* high10 progressive profile mode */
  u32 tbl_sizey;            /* compression table for Y/C */
  u32 tbl_sizec;
} dpbInitParams_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdInitDpb(
//#ifndef USE_EXTERNAL_BUFFER
  const void *dwl,
//#endif
  dpbStorage_t * dpb,
  struct dpbInitParams *p_dpb_params);

u32 h264bsdResetDpb(
//#ifndef USE_EXTERNAL_BUFFER
  const void *dwl,
//#endif
  dpbStorage_t * dpb,
  struct dpbInitParams *p_dpb_params);

void h264bsdInitRefPicList(dpbStorage_t * dpb);

void *h264bsdAllocateDpbImage(dpbStorage_t * dpb);

i32 h264bsdGetRefPicData(const dpbStorage_t * dpb, u32 index);
u8 *h264bsdGetRefPicDataVlcMode(const dpbStorage_t * dpb, u32 index,
                                u32 field_mode);

u32 h264bsdReorderRefPicList(dpbStorage_t * dpb,
                             refPicListReordering_t * order,
                             u32 curr_frame_num, u32 num_ref_idx_active);

u32 h264bsdReorderRefPicListCheck(dpbStorage_t * dpb,
                                  refPicListReordering_t * order,
                                  u32 curr_frame_num, u32 num_ref_idx_active,
                                  u32 gaps_in_frame_num_value_allowed_flag,
                                  u32 base_opposite_field_pic,
                                  u32 field_pic_flag);

u32 h264bsdMarkDecRefPic(dpbStorage_t * dpb,
                         /*@null@ */ const decRefPicMarking_t * mark,
                         const image_t * image, u32 frame_num, i32 *pic_order_cnt,
                         u32 is_idr, u32 pic_id, u32 num_err_mbs, u32 tiled_mode, u32 pic_code_type );

u32 h264bsdCheckGapsInFrameNum(dpbStorage_t * dpb, u32 frame_num, u32 is_ref_pic,
                               u32 gaps_allowed);

/*@null@*/ dpbOutPicture_t *h264bsdDpbOutputPicture(dpbStorage_t * dpb);

void h264bsdFlushDpb(dpbStorage_t * dpb);

void h264bsdFreeDpb(
//#ifndef USE_EXTERNAL_BUFFER
  const void *dwl,
//#endif
  dpbStorage_t * dpb);

void ShellSort(dpbStorage_t * dpb, u32 *list, u32 type, i32 par);
void ShellSortF(dpbStorage_t * dpb, u32 *list, u32 type, /*u32 parity,*/ i32 par);

void SetPicNums(dpbStorage_t * dpb, u32 curr_frame_num);

void h264DpbUpdateOutputList(dpbStorage_t * dpb);
void h264DpbAdjStereoOutput(dpbStorage_t * dpb, u32 target_count);
#ifdef USE_OUTPUT_RELEASE
void h264EmptyDpb(dpbStorage_t *dpb);
void h264DpbStateReset(dpbStorage_t *dpb);
#endif

u32 h264DpbHRDBumping(dpbStorage_t * dpb);
void h264ClearBump(dpbStorage_t * dpb);
void h264DpbRecover(dpbStorage_t *dpb, u32 curr_frame_num, i32 curr_poc,
                    u32 error_handling);
void h264RemoveNoBumpOutput(dpbStorage_t *dpb, u32 size);
u32 h264FindDpbBufferId(dpbStorage_t *dpb);
void RemoveTempPpOutputAll(dpbStorage_t *dpb);
void RemoveUnmarkedPpBuffer(dpbStorage_t *dpb);
#endif /* #ifdef H264HWD_DPB_H */
