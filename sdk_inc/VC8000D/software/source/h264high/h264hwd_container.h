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

#ifndef H264HWD_CONTAINER_H
#define H264HWD_CONTAINER_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "h264hwd_storage.h"
#include "h264hwd_util.h"
#include "refbuffer.h"
#include "deccfg.h"
#include "decppif.h"
#include "workaround.h"

#include "h264hwd_dpb_lock.h"
#include "input_queue.h"
#include "ppu.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

#define DEC_UNINITIALIZED   0U
#define DEC_INITIALIZED     1U
#define DEC_BUFFER_EMPTY    2U
#define DEC_NEW_HEADERS     3U
#define DEC_WAIT_FOR_BUFFER 4U

#ifdef USE_RANDOM_TEST
#include <stdio.h>
struct ErrorParams {
  u32 seed;
  u8 truncate_stream;
  char truncate_stream_odds[24];
  u8 swap_bits_in_stream;
  char swap_bit_odds[24];
  u8 lose_packets;
  char packet_loss_odds[24];
  u32 random_error_enabled;
};
#endif

/* asic interface */
typedef struct DecAsicBuffers {
  u32 buff_status;
  struct DWLLinearMem mb_ctrl;
  struct DWLLinearMem mv;
  struct DWLLinearMem intra_pred;
  struct DWLLinearMem residual;
  struct DWLLinearMem *out_buffer;
  struct DWLLinearMem *out_pp_buffer;
  struct DWLLinearMem cabac_init[MAX_ASIC_CORES];
  addr_t ref_pic_list[16];
  u32 max_ref_frm;
  u32 filter_disable;
  i32 chroma_qp_index_offset;
  i32 chroma_qp_index_offset2;
  u32 current_mb;
  u32 not_coded_mask;
  u32 rlc_words;
  u32 pic_size_in_mbs;
  u32 whole_pic_concealed;
  u32 disable_out_writing;
  u32 enable_dmv_and_poc;
} DecAsicBuffers_t;

typedef struct {
  u32 core_id;
  const u8 *stream;
  const void *p_user_data;
  u32 is_field_pic;
  u32 is_bottom_field;
  u32 out_id;
  dpbStorage_t *current_dpb;
  u32 ref_id[16];
#ifdef USE_EC_MC
  u32 is_idr;
#endif
} H264HwRdyCallbackArg;

typedef struct decContainer {
  const void *checksum;
  u32 dec_stat;
  u32 nal_mode;
  u32 pic_number;
  u32 asic_running;
  u32 rlc_mode;
  u32 try_vlc;
  u32 error_conceal;
  u32 error_frame;
  u32 reallocate;
  u8 *p_hw_stream_start;
  addr_t hw_stream_start_bus;
  addr_t buff_start_bus;
  u32 buff_length;
  u32 first_mb_offset;
  u32 error_frame_au;
  u32 original_word;
  u32 hw_bit_pos;
  u32 hw_length;
  u32 stream_pos_updated;
  u32 nal_start_code;
  u32 mode_change;
  u32 gaps_checked_for_this;
  u32 packet_decoded;
  u32 force_rlc_mode;        /* by default stays 0, testing can set it to 1 for RLC mode */

  u32 h264_regs[TOTAL_X170_REGISTERS];
  storage_t storage;       /* h264bsd storage */
  DecAsicBuffers_t asic_buff[1];
  const void *dwl;         /* DWL instance */
  i32 core_id;
  u32 ref_buf_support;
  u32 tiled_mode_support;
  u32 tiled_reference_enable;
  u32 tiled_stride_enable;
  u32 h264_profile_support;
  u32 is8190;
  u32 max_dec_pic_width;
  u32 max_dec_pic_height;
  u32 allow_dpb_field_ordering;
  u32 dpb_mode;
  struct refBuffer ref_buffer_ctrl;

  u32 keep_hw_reserved;
  u32 skip_non_reference;

  workaround_t workarounds;
  u32 frame_num_mask; /* for workaround */
  u32 force_nal_mode;

  DWLHwConfig hw_cfg[MAX_ASIC_CORES];

  FrameBufferList fb_list;
  u32 b_mc; /* flag to indicate MC mode status */
  u32 n_cores;
  struct {
    H264DecMCStreamConsumed *fn;
    const u8 *p_strm_buff; /* stream buffer passed in callback */
    const void *p_user_data; /* user data to be passed in callback */
  } stream_consumed_callback;

  H264HwRdyCallbackArg hw_rdy_callback_arg[MAX_ASIC_CORES];
  i32 poc[34];

  struct pp_ {
    const void *pp_instance;
    void (*PPDecStart) (const void *, const DecPpInterface *);
    void (*PPDecWaitEnd) (const void *);
    void (*PPConfigQuery) (const void *, DecPpQuery *);
    void (*PPNextDisplayId)(const void *, u32); /* set the next PP outpic ID (multibuffer) */
    DecPpInterface dec_pp_if;
    DecPpQuery pp_info;
    const struct DWLLinearMem * sent_pic_to_pp[17]; /* list of pictures sent to pp */
    const struct DWLLinearMem * queued_pic_to_pp[2]; /* queued picture that should be processed next */
    u32 multi_max_id; /* maximum position used in sent_pic_to_pp[] */
  } pp;
#ifdef USE_EXTERNAL_BUFFER
  u32 next_buf_size;  /* size of the requested external buffer */
  u32 ref_buf_size; /* stored reference buffer size, used in dynamical buffer mode*/
  u32 buf_num;        /* number of buffers (with size of next_buf_size) requested to be allocated externally */
  struct DWLLinearMem *buf_to_free;
  u32 buffer_index[2];
  u32 b_mvc;
  u32 n_ext_buf_size;  /* size of external buffers */
  struct DWLLinearMem ext_buffers[MAX_FRAME_BUFFER_NUMBER];  /* external buffers descriptors*/
  u32 ext_buffer_num;  /* number of external buffers added */
#endif
#ifdef USE_OUTPUT_RELEASE
  u32 abort;
#endif
  pthread_mutex_t protect_mutex;

#ifdef USE_RANDOM_TEST
  struct ErrorParams error_params;
  u32 stream_not_consumed;
  u32 prev_input_len;
  FILE *ferror_stream;
#endif

  DecPicAlignment align;  /* buffer alignment for both reference/pp output */
  u32 packet_loss;  /* packet loss flag due to frame num gap */

  u32 error_handling;
  u32 use_video_compressor;

  /* For open B picture handling. */
  u32 first_entry_point;
  u32 entry_is_IDR;
  i32 entry_POC;
  u32 skip_b;

  u32 alloc_buffer;
  u32 no_decoding_buffer;
  u32 dec_result;
  u32 num_read_bytes;

  u32 use_adaptive_buffers;
  u32 n_guard_size;
  u32 cr_first;

  u32 pp_enabled;     /* set to 1 to enable pp */
  u32 dscale_shift_x;
  u32 dscale_shift_y;

  PpUnitIntConfig ppu_cfg[4];

  u32 ppb_mode;   /* postposed picture buffer mode: 0 - FRAME, 1 - FIELD */

  InputQueue pp_buffer_queue;

  u32 low_latency;
  sem_t updated_reg_sem;
  u32 ll_strm_bus_address; /* strm bus address in low latency mode */
  u32 ll_strm_len; /* strm length in low latency mode */
  u32 update_reg_flag; /* the flag indicate if length register need to be updated or not */
  u32 tmp_length; /* used to update hwLength in low latency mode */
  u32 first_update; /* the flag indicate if length register is updated first time or not */
  u32 resolution_change;

  u32 max_strm_len;
  u32 high10p_mode;
  u32 always_out_ref;
} decContainer_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

#endif /* #ifdef H264HWD_CONTAINER_H */
