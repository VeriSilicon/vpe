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

#ifndef H264HWD_NAL_UNIT_H
#define H264HWD_NAL_UNIT_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "h264hwd_stream.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/* macro to determine if NAL unit pointed by p_nal_unit contains an IDR slice */
#define IS_IDR_NAL_UNIT(p_nal_unit) \
    ((p_nal_unit)->nal_unit_type == NAL_CODED_SLICE_IDR || \
     ((p_nal_unit)->nal_unit_type == NAL_CODED_SLICE_EXT && \
      (p_nal_unit)->non_idr_flag == 0))

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

typedef enum {
  NAL_UNSPECIFIED = 0,
  NAL_CODED_SLICE = 1,
  NAL_CODED_SLICE_DP_A = 2,
  NAL_CODED_SLICE_DP_B = 3,
  NAL_CODED_SLICE_DP_C = 4,
  NAL_CODED_SLICE_IDR = 5,
  NAL_SEI = 6,
  NAL_SEQ_PARAM_SET = 7,
  NAL_PIC_PARAM_SET = 8,
  NAL_ACCESS_UNIT_DELIMITER = 9,
  NAL_END_OF_SEQUENCE = 10,
  NAL_END_OF_STREAM = 11,
  NAL_FILLER_DATA = 12,
  NAL_SPS_EXT = 13,
  NAL_PREFIX = 14,
  NAL_SUBSET_SEQ_PARAM_SET = 15,
  NAL_CODED_SLICE_AUX = 19,
  NAL_CODED_SLICE_EXT = 20,
  NAL_MAX_TYPE_VALUE = 31
} nalUnitType_e;

typedef struct {
  nalUnitType_e nal_unit_type;
  u32 nal_ref_idc;
  u32 svc_extension_flag;
  u32 non_idr_flag;
  u32 priority_id;
  u32 view_id;
  u32 temporal_id;
  u32 anchor_pic_flag;
  u32 inter_view_flag;
} nalUnit_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 h264bsdDecodeNalUnit(strmData_t * p_strm_data, nalUnit_t * p_nal_unit);

#endif /* #ifdef H264HWD_NAL_UNIT_H */
