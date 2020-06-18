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

#ifndef H264HWD_SEI_H
#define H264HWD_SEI_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "h264hwd_seq_param_set.h"
#include "h264hwd_stream.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/* enumerated sample aspect ratios, ASPECT_RATIO_M_N means M:N */
enum {
  SEI_BUFFERING_PERIOD = 0,
  SEI_PIC_TIMING,
  SEI_PAN_SCAN_RECT,
  SEI_FILLER_PAYLOAD,
  SEI_USER_DATA_REGISTERED_ITU_T_T35,
  SEI_USER_DATA_UNREGISTERED,
  SEI_RECOVERY_POINT,
  SEI_DEC_REF_PIC_MARKING_REPETITION,
  SEI_SPARE_PIC,
  SEI_SCENE_INFO,
  SEI_SUB_SEQ_INFO,
  SEI_SUB_SEQ_LAYER_CHARACTERISTICS,
  SEI_SUB_SEQ_CHARACTERISTICS,
  SEI_FULL_FRAME_FREEZE,
  SEI_FULL_FRAME_FREEZE_RELEASE,
  SEI_FULL_FRAME_SNAPSHOT,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,
  SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET,
  SEI_FILM_GRAIN_CHARACTERISTICS,
  SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE,
  SEI_STEREO_VIDEO_INFO,
  SEI_POST_FILTER_HINTS,
  SEI_TONE_MAPPING,
  SEI_SCALABILITY_INFO,
  SEI_SUB_PIC_SCALABLE_LAYER,
  SEI_NON_REQUIRED_LAYER_REP,
  SEI_PRIORITY_LAYER_INFO,
  SEI_LAYERS_NOT_PRESENT,
  SEI_LAYER_DEPENDENCY_CHANGE,
  SEI_SCALABLE_NESTING,
  SEI_BASE_LAYER_TEMPORAL_HRD,
  SEI_QUALITY_LAYER_INTEGRITY_CHECK,
  SEI_REDUNDANT_PIC_PROPERTY,
  SEI_TL0_DEP_REP_INDEX,
  SEI_TL_SWITCHING_POINT,
  SEI_PARALLEL_DECODING_INFO,
  SEI_MVC_SCALABLE_NESTING,
  SEI_VIEW_SCALABILITY_INFO,
  SEI_MULTIVIEW_SCENE_INFO,
  SEI_MULTIVIEW_ACQUISITION_INFO,
  SEI_NON_REQUIRED_VIEW_COMPONENT,
  SEI_VIEW_DEPENDENCY_CHANGE,
  SEI_OPERATION_POINTS_NOT_PRESENT,
  SEI_BASE_VIEW_TEMPORAL_HRD,
  SEI_FRAME_PACKING_ARRANGEMENT,
  SEI_GREEN_METADATA=56,

  SEI_MAX_ELEMENTS  //!< number of maximum syntax elements
};


/* structure to store Buffering period SEI parameters */
typedef struct {
  u32 seq_parameter_set_id;
  u32 initial_cpb_removal_delay[32];
  u32 initial_cpb_removal_delay_offset[32];
  u32 exist_flag;
} bufferingPeriodInfo_t;

/* storage for Picture timing SEI parameters */
typedef struct {
  u32 cpb_removal_delay;
  u32 dpb_output_delay;
  u32 pic_struct;
  u32 clock_timestamp_flag[3];
  u32 ct_type;
  u32 nuit_field_based_flag;
  u32 counting_type;
  u32 full_timestamp_flag;
  u32 discontinuity_flag;
  u32 cnt_dropped_flag;
  u32 n_frames;
  u32 seconds_value;
  u32 minutes_value;
  u32 hours_value;
  u32 seconds_flag;
  u32 minutes_flag;
  u32 hours_flag;
  u32 time_offset;
  u32 exist_flag;
} picTimingInfo_t;

typedef struct {
  double clock_tick;
  double initial_arrival_time;
  double prev_nominal_removal_time;
  double final_arrival_time;
  double initial_arrival_time_earliest;
  double nominal_removal_time;
  double nominal_removal_time_first;
  double cpb_removal_time;
  double dpb_output_time;
  double access_unit_size;
  double is_first_au;
  u32 hrd_init_flag;

} computeTimeInfo_t;

typedef struct {
  u32 bumping_flag;
  bufferingPeriodInfo_t buffering_period_info;
  picTimingInfo_t pic_timing_info;
  computeTimeInfo_t  compute_time_info;
} seiParameters_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdDecodeSeiParameters(seqParamSet_t **sps, strmData_t *p_strm_data,
                               seiParameters_t *p_sei_parameters);

u32 h264bsdDecodeBufferingPeriodInfo(seqParamSet_t **sps, strmData_t *p_strm_data,
                                     bufferingPeriodInfo_t *p_buffering_period_info);

u32 h264bsdDecodePicTimingInfo(seqParamSet_t **sps, strmData_t *p_strm_data,
                               picTimingInfo_t *p_pic_timing_info, bufferingPeriodInfo_t *p_buffering_period_info);

u32 h264bsdComputeTimes(seqParamSet_t *sps, seiParameters_t *p_sei_parameters);
#endif /* #ifdef H264HWD_SEI_H */
