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
#ifndef _PPAPI_INTERNAL_H_
#define _PPAPI_INTERNAL_H_

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "ppapi.h"
#include "basetype.h"
#include "regdrv.h"
#include "deccfg.h"
#include "commonconfig.h"
#include "ppu.h"

typedef struct PPContainer_ {
  u32 pp_regs[TOTAL_X170_REGISTERS];
  u32 in_format;
  u32 in_stride;
  u32 in_height;
  u32 pp_enabled;
  struct DWLLinearMem pp_in_buffer;
  struct DWLLinearMem pp_out_buffer;
  PpUnitIntConfig ppu_cfg[4];
  const void *dwl;
  i32 core_id;
  u32 b_disable_tcache; //1:no tcache, read raw from ep  0:use tcache
} PPContainer;

void PPFlushRegs(PPContainer * pp_c);
void PPRefreshRegs(PPContainer * pp_c);
#endif
