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

#ifndef H264HWD_VUI_H
#define H264HWD_VUI_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

#include "h264hwd_stream.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

#define MAX_CPB_CNT 32

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/* enumerated sample aspect ratios, ASPECT_RATIO_M_N means M:N */
enum {
  ASPECT_RATIO_UNSPECIFIED = 0,
  ASPECT_RATIO_1_1,
  ASPECT_RATIO_12_11,
  ASPECT_RATIO_10_11,
  ASPECT_RATIO_16_11,
  ASPECT_RATIO_40_33,
  ASPECT_RATIO_24_11,
  ASPECT_RATIO_20_11,
  ASPECT_RATIO_32_11,
  ASPECT_RATIO_80_33,
  ASPECT_RATIO_18_11,
  ASPECT_RATIO_15_11,
  ASPECT_RATIO_64_33,
  ASPECT_RATIO_160_99,
  ASPECT_RATIO_EXTENDED_SAR = 255
};

/* structure to store Hypothetical Reference Decoder (HRD) parameters */
typedef struct {
  u32 cpb_cnt;
  u32 bit_rate_scale;
  u32 cpb_size_scale;
  u32 bit_rate_value[MAX_CPB_CNT];
  u32 cpb_size_value[MAX_CPB_CNT];
  u32 cbr_flag[MAX_CPB_CNT];
  u32 initial_cpb_removal_delay_length;
  u32 cpb_removal_delay_length;
  u32 dpb_output_delay_length;
  u32 time_offset_length;
} hrdParameters_t;

/* storage for VUI parameters */
typedef struct {
  u32 aspect_ratio_present_flag;
  u32 aspect_ratio_idc;
  u32 sar_width;
  u32 sar_height;
  u32 overscan_info_present_flag;
  u32 overscan_appropriate_flag;
  u32 video_signal_type_present_flag;
  u32 video_format;
  u32 video_full_range_flag;
  u32 colour_description_present_flag;
  u32 colour_primaries;
  u32 transfer_characteristics;
  u32 matrix_coefficients;
  u32 chroma_loc_info_present_flag;
  u32 chroma_sample_loc_type_top_field;
  u32 chroma_sample_loc_type_bottom_field;
  u32 timing_info_present_flag;
  u32 num_units_in_tick;
  u32 time_scale;
  u32 fixed_frame_rate_flag;
  u32 nal_hrd_parameters_present_flag;
  hrdParameters_t nal_hrd_parameters;
  u32 vcl_hrd_parameters_present_flag;
  hrdParameters_t vcl_hrd_parameters;
  u32 low_delay_hrd_flag;
  u32 pic_struct_present_flag;
  u32 bitstream_restriction_flag;
  u32 motion_vectors_over_pic_boundaries_flag;
  u32 max_bytes_per_pic_denom;
  u32 max_bits_per_mb_denom;
  u32 log2_max_mv_length_horizontal;
  u32 log2_max_mv_length_vertical;
  u32 num_reorder_frames;
  u32 max_dec_frame_buffering;
  u32 update_hrdparameter_flag;
  u32 error_hrdparameter_flag;
} vuiParameters_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdDecodeVuiParameters(strmData_t *p_strm_data,
                               vuiParameters_t *p_vui_parameters);

u32 h264bsdDecodeHrdParameters(
  strmData_t *p_strm_data,
  hrdParameters_t *p_hrd_parameters);

#endif /* #ifdef H264HWD_VUI_H */
