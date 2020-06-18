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

#ifndef H264HWD_STORAGE_H
#define H264HWD_STORAGE_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

#include "h264hwd_cfg.h"
#include "h264hwd_seq_param_set.h"
#include "h264hwd_pic_param_set.h"
#include "h264hwd_macroblock_layer.h"
#include "h264hwd_nal_unit.h"
#include "h264hwd_slice_header.h"
#include "h264hwd_seq_param_set.h"
#include "h264hwd_dpb.h"
#include "h264hwd_pic_order_cnt.h"
#include "h264hwd_sei.h"
#include "input_queue.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

typedef struct {
  u32 slice_id;
  u32 num_decoded_mbs;
  u32 last_mb_addr;
} sliceStorage_t;

/* structure to store parameters needed for access unit boundary checking */
typedef struct {
  nalUnit_t nu_prev[1];
  u32 prev_frame_num;
  u32 prev_mod_frame_num;
  u32 prev_idr_pic_id;
  u32 prev_pic_order_cnt_lsb;
  i32 prev_delta_pic_order_cnt_bottom;
  i32 prev_delta_pic_order_cnt[2];
  u32 prev_field_pic_flag;
  u32 prev_bottom_field_flag;
  u32 first_call_flag;
  u32 new_picture;
} aubCheck_t;

/* storage data structure, holds all data of a decoder instance */
typedef struct {
  /* active paramet set ids and pointers */
  u32 old_sps_id;
  u32 active_pps_id;
  u32 active_sps_id;
  u32 active_view_sps_id[MAX_NUM_VIEWS];
  picParamSet_t *active_pps;
  seqParamSet_t *active_sps;
  seqParamSet_t *active_view_sps[MAX_NUM_VIEWS];
  seqParamSet_t *sps[MAX_NUM_SEQ_PARAM_SETS];
  picParamSet_t *pps[MAX_NUM_PIC_PARAM_SETS];

  /* current slice group map, recomputed for each slice */
  u32 *slice_group_map;

  u32 pic_size_in_mbs;

  /* this flag is set after all macroblocks of a picture successfully
   * decoded -> redundant slices not decoded */
  u32 skip_redundant_slices;
  u32 pic_started;

  /* flag to indicate if current access unit contains any valid slices */
  u32 valid_slice_in_access_unit;

  /* store information needed for handling of slice decoding */
  sliceStorage_t slice[1];

  /* number of concealed macroblocks in the current image */
  u32 num_concealed_mbs;

  /* picId given by application */
  u32 current_pic_id;

  /* macroblock specific storages, size determined by image dimensions */
  mbStorage_t *mb;

  /* flag to store noOutputReordering flag set by the application */
  u32 no_reordering;

  /* pointer to DPB of current view */
  dpbStorage_t *dpb;

  /* DPB */
  dpbStorage_t dpbs[MAX_NUM_VIEWS][2];

  /* structure to store picture order count related information */
  pocStorage_t poc[2];

  /* access unit boundary checking related data */
  aubCheck_t aub[1];

  aubCheck_t tmp_aub[1];
  /* current processed image */
  image_t curr_image[1];

  /* last valid NAL unit header is stored here */
  nalUnit_t prev_nal_unit[1];

  /* last NAL unit */
  nalUnit_t last_nal_unit[1];

  /* slice header, second structure used as a temporary storage while
   * decoding slice header, first one stores last successfully decoded
   * slice header */
  sliceHeader_t *slice_header;
  sliceHeader_t slice_headers[MAX_NUM_VIEWS][2];

  seiParameters_t sei;
  /* fields to store old stream buffer pointers, needed when only part of
   * a stream buffer is processed by h264bsdDecode function */
  u32 prev_buf_not_finished;
  const u8 *prev_buf_pointer;
  u32 prev_bytes_consumed;
  strmData_t strm[1];

  /* macroblock layer structure, there is no need to store this but it
   * would have increased the stack size excessively and needed to be
   * allocated from heap -> easiest to put it here */
  macroblockLayer_t mb_layer[1];

  u32 aso_detected;
  u32 second_field;
  u32 checked_aub; /* signal that AUB was checked already */
  u32 prev_idr_pic_ready; /* for FFWD workaround */

  u32 intra_freeze;
  u32 partial_freeze;
  u32 picture_broken;
  u32 tiled_stride_enable;
  u32 use_video_compressor;

  u32 enable2nd_chroma;     /* by default set according to ENABLE_2ND_CHROMA
                                compiler flag, may be overridden by testbench */

  /* pointers to 2nd chroma output, only available if extension enabled */
  u32 *p_ch2;
  addr_t b_ch2;

  u32 pp_used;
  u32 use_smoothing;
  u32 current_marked;
  u32 prev_pic_width;
  u32 prev_pic_height;
  u32 pending_flush;

  u32 mvc;
  u32 mvc_stream;
  u32 view;
  u32 view_id[MAX_NUM_VIEWS];
  u32 out_view;
  u32 num_views;
  u32 base_opposite_field_pic;
  u32 non_inter_view_ref;
  u32 pp_tiled_e;

  u32 next_view;
  u32 last_base_num_out;

  u32 multi_buff_pp;

  DecPicAlignment align;

  /* PP related variable, inherited from dec container */
  u32 pp_enabled;
  u32 down_scale_x_shift;
  u32 down_scale_y_shift;
  u32 pp_width;
  u32 pp_height;
  InputQueue pp_buffer_queue;
  u32 release_buffer;    /* Flag indicateing to release ext buffers. */
  u32 ext_buffer_size;    /* External buffer size. */
  u32 ext_buffer_added;   /* Flag indicating whether external buffer has been added. */
  u32 ext_buffer_count;   /* total number of external buffers added */
  u32 pp_buffer_size;     /* Next pp buffer size required. */
  u32 pp_buffer_num;      /* Next pp buffers num required */
  u32 ref_buffer_size;    /* Next ref buffers size required */
  u32 always_output_ref;

  u32 n_extra_frm_buffers;  /* extra reference bufferrs requested */

  const dpbOutPicture_t *pending_out_pic;
} storage_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

void h264bsdInitStorage(storage_t * storage);
void h264bsdResetStorage(storage_t * storage);
u32 h264bsdIsStartOfPicture(storage_t * storage);
u32 h264bsdIsEndOfPicture(storage_t * storage);
u32 h264bsdStoreSeqParamSet(storage_t * storage, seqParamSet_t * p_seq_param_set);
u32 h264bsdStorePicParamSet(storage_t * storage, picParamSet_t * p_pic_param_set);
u32 h264bsdActivateParamSets(storage_t * storage, u32 pps_id, u32 slice_type, u32 is_idr);
void h264bsdComputeSliceGroupMap(storage_t * storage,
                                 u32 slice_group_change_cycle);

u32 h264bsdCheckAccessUnitBoundary(strmData_t * strm,
                                   nalUnit_t * nu_next,
                                   storage_t * storage,
                                   u32 * access_unit_boundary_flag, u32 error_check_flag);

u32 h264bsdValidParamSets(storage_t * storage);

u32 h264bsdAllocateSwResources(
  const void *dwl,
  storage_t * storage,
  u32 is_high_supported,
  u32 is_high10_supported,
  u32 n_cores);
#ifdef USE_EXTERNAL_BUFFER
u32 h264bsdMVCAllocateSwResources(const void *dwl, storage_t * storage,
                                  u32 is_high_supported,
                                  u32 is_high10_supported,
                                  u32 n_cores);
#endif
#if USE_OUTPUT_RELEASE
void h264bsdClearStorage(storage_t * storage);
#endif
u32 h264bsdStoreSEIInfoForCurrentPic(storage_t * storage);
#endif /* #ifdef H264HWD_STORAGE_H */
