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

#ifndef CONFIG_H_DEFINED
#define CONFIG_H_DEFINED

#include "basetype.h"
/* tile border coefficients of filter */
#define ASIC_VERT_FILTER_RAM_SIZE 8 /* bytes per pixel row */
/* BSD control data of current picture at tile border
 * 128 bits per 4x4 tile = 128/(8*4) bytes per row */
#define ASIC_BSD_CTRL_RAM_SIZE 4 /* bytes per pixel row */

/* Common config for PP max/min size supported */
#define PP_CROP_MIN_WIDTH           48
#define PP_CROP_MIN_HEIGHT          48
#define PP_SCALE_IN_MAX_WIDTH       4096
#define PP_SCALE_IN_MAX_HEIGHT      4096
#define JPEG_PP_SCALE_IN_MAX_WIDTH  16384
#define JPEG_PP_SCALE_IN_MAX_HEIGHT 16384
#define PP_SCALE_OUT_MAX_WIDTH      4096
#define PP_SCALE_OUT_MAX_HEIGHT     2304
#define PP_OUT_FMT_YUV420PACKED 0
#define PP_OUT_FMT_YUV420_P010 1
#define PP_OUT_FMT_YUV420_BIGE 2
#define PP_OUT_FMT_YUV420_8BIT 3
#define PP_OUT_FMT_YUV400 4       /* A.k.a., Monochrome/luma only*/
#define PP_OUT_FMT_YUV400_P010 5
#define PP_OUT_FMT_YUV400_8BIT 6
#define PP_OUT_FMT_IYUVPACKED 7
#define PP_OUT_FMT_IYUV_P010 8
#define PP_OUT_FMT_IYUV_8BIT 9

void SetCommonConfigRegs(const void* dwl,u32 *regs);
void SetLegacyG1CommonConfigRegs(const void* dwl,u32 *regs);
void SetLegacyG2CommonConfigRegs(u32 *regs);

#endif /* CONFIG_H_DEFINED */
