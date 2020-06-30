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

#ifndef __H264ASIC_H__
#define __H264ASIC_H__

#include "basetype.h"
#include "dwl.h"
#include "h264hwd_container.h"
#include "h264hwd_storage.h"

#define ASIC_MB_RLC_BUFFER_SIZE     880 /* bytes */
#define ASIC_MB_CTRL_BUFFER_SIZE    8   /* bytes */
#define ASIC_MB_MV_BUFFER_SIZE      64  /* bytes */
#define ASIC_MB_I4X4_BUFFER_SIZE    8   /* bytes */
#define ASIC_CABAC_INIT_BUFFER_SIZE 3680/* bytes */
#define ASIC_SCALING_LIST_SIZE      6*16+2*64
#define ASIC_POC_BUFFER_SIZE        34*4
/* For High10 mode, POC buffer size (136) aligned to 16 bytes */
#define ASIC_POC_BUFFER_SIZE_H10    144

#define X170_DEC_TIMEOUT            0x00FFU
#define X170_DEC_SYSTEM_ERROR       0x0FFFU
#define X170_DEC_HW_RESERVED        0xFFFFU
#define X170_DEC_FATAL_SYSTEM_ERROR 0xFFFFFU
#define X170_DEC_MEM_ALLOC_FAIL     0xFFFFFFU
#define X170_DEC_EDMA_TRANS_FAIL    0xFFFFFFFU


/* asic macroblock types */
typedef enum H264AsicMbTypes {
  HW_P_16x16 = 0,
  HW_P_16x8 = 1,
  HW_P_8x16 = 2,
  HW_P_8x8 = 3,
  HW_I_4x4 = 4,
  HW_I_16x16 = 5,
  HW_I_PCM = 6,
  HW_P_SKIP = 7
} H264AsicMbTypes_t;

u32 AllocateAsicBuffers(decContainer_t * dec_cont,
                        DecAsicBuffers_t * asic_buff, u32 mbs);
void ReleaseAsicBuffers(const void *dwl, DecAsicBuffers_t * asic_buff);

void PrepareIntra4x4ModeData(storage_t * storage,
                             DecAsicBuffers_t * p_asic_buff);
void PrepareMvData(storage_t * storage, DecAsicBuffers_t * p_asic_buff);

void PrepareRlcCount(storage_t * storage, DecAsicBuffers_t * p_asic_buff);

void H264SetupVlcRegs(decContainer_t * dec_cont);

void H264InitRefPicList(decContainer_t *dec_cont);

u32 H264RunAsic(decContainer_t * dec_cont, DecAsicBuffers_t * p_asic_buff);

void H264UpdateAfterHwRdy(decContainer_t *dec_cont, u32 *h264_regs);

void H264ErrorRecover(decContainer_t *dec_cont);

u32 H264CheckHwStatus(decContainer_t *dec_cont);
#endif /* __H264ASIC_H__ */
