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

#ifndef __REFBUFFER_H__
#define __REFBUFFER_H__

#include "basetype.h"

typedef enum {
  REFBU_FRAME,
  REFBU_FIELD,
  REFBU_MBAFF
} refbuMode_e;

/* Feature support flags */
#define REFBU_SUPPORT_GENERIC       (1)
#define REFBU_SUPPORT_INTERLACED    (2)
#define REFBU_SUPPORT_DOUBLE        (4)
#define REFBU_SUPPORT_OFFSET        (8)

/* Buffering info */
#define REFBU_BUFFER_SINGLE_FIELD   (1)
#define REFBU_MULTIPLE_REF_FRAMES   (2)
#define REFBU_DISABLE_CHECKPOINT    (4)
#define REFBU_FORCE_ADAPTIVE_SINGLE (8)
#define REFBU_DONT_USE_STATS        (16)
#define REFBU_DISABLE               (32)

#ifndef HANTRO_TRUE
#define HANTRO_TRUE     (1)
#endif /* HANTRO_TRUE */

#ifndef HANTRO_FALSE
#define HANTRO_FALSE    (0)
#endif /* HANTRO_FALSE*/

/* macro to get smaller of two values */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/* macro to get greater of two values */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct memAccess {
  u32 latency;
  u32 nonseq;
  u32 seq;
} memAccess_t;

struct refBuffer;
struct refBuffer {
#if 0
  i32 ox[3];
#endif
  i32 dec_mode_mb_weights[2];
  i32 mb_weight;
  i32 oy[3];
  i32 pic_width_in_mbs;
  i32 pic_height_in_mbs;
  i32 frm_size_in_mbs;
  i32 fld_size_in_mbs;
  i32 num_intra_blk[3];
  i32 coverage[3];
  i32 fld_hits_p[3][2];
  i32 fld_hits_b[3][2];
  i32 fld_cnt;
  i32 mvs_per_mb;
  i32 filter_size;
  /* Thresholds */
  i32 pred_intra_blk;
  i32 pred_coverage;
  i32 checkpoint;
  u32 dec_mode;
  u32 data_excess_max_pct;

  i32 bus_width_in_bits;
  i32 prev_latency;
  i32 num_cycles_for_buffering;
  i32 total_data_for_buffering;
  i32 buffer_penalty;
  i32 avg_cycles_per_mb;
  u32 prev_was_field;
  u32 prev_used_double;
  i32 thr_adj;
  u32 prev_frame_hit_sum;
  memAccess_t curr_mem_model;   /* Clocks per operation, modifiable from
                                 * testbench. */
  memAccess_t mem_access_stats; /* Approximate counts for operations, set
                                 * based on format */
  u32 mem_access_stats_flag;

  /* Support flags */
  u32 interlaced_support;
  u32 double_support;
  u32 offset_support;

  /* Internal test mode */
  void (*test_function)(struct refBuffer*,u32*reg_base,u32 is_intra,u32 mode);

};
void RefbuInit( struct refBuffer *p_refbu, u32 dec_mode, u32 pic_width_in_mbs, u32
                pic_height_in_mbs, u32 support_flags );

void RefbuMvStatistics( struct refBuffer *p_refbu, u32 *reg_base,
                        u32 *p_mv, u32 direct_mvs_available,
                        u32 is_intra_picture );

void RefbuMvStatisticsB( struct refBuffer *p_refbu, u32 *reg_base );

void RefbuSetup( struct refBuffer *p_refbu, u32 *reg_base,
                 refbuMode_e mode,
                 u32 is_intra_frame, u32 is_bframe,
                 u32 ref_pic_id0, u32 refpic_id1,
                 u32 flags );

i32 RefbuGetHitThreshold( struct refBuffer *p_refbu );
u32 RefbuVpxGetPrevFrameStats( struct refBuffer *p_refbu );

#endif /* __REFBUFFER_H__ */
