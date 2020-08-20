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

#ifndef RASTER_BUFFER_MGR_H
#define RASTER_BUFFER_MGR_H

#include "basetype.h"
#include "dwl.h"

typedef void* RasterBufferMgr;

struct RasterBufferParams {
  struct DWLLinearMem* tiled_buffers;
  u32 num_buffers;
  u32 width;
  u32 height;
  u32 size;     /* size of buffer, added to support multi-ppu */
#ifdef USE_EXTERNAL_BUFFER
  u32 ext_buffer_config;
#endif
  const void* dwl;
};

RasterBufferMgr RbmInit(struct RasterBufferParams params);
struct DWLLinearMem *RbmGetPpBuffer(RasterBufferMgr instance,
                                        struct DWLLinearMem key);
struct DWLLinearMem RbmGetTiledBuffer(RasterBufferMgr instance,
                                      struct DWLLinearMem buffer);
void RbmRelease(RasterBufferMgr inst);

#ifdef USE_EXTERNAL_BUFFER
struct DWLLinearMem RbmNextReleaseBuffer(RasterBufferMgr inst);
void RbmAddPpBuffer(RasterBufferMgr instance, struct DWLLinearMem *pp_buffer, i32 i);
struct DWLLinearMem * RbmReturnPpBuffer(RasterBufferMgr instance, const u32 *addr);
void RbmReturnAllPpBuffer(RasterBufferMgr instance);
void RbmResetPpBuffer(RasterBufferMgr instance);
void RbmSetPpBufferUsed(RasterBufferMgr instance, const u32 *addr);
void RbmWaitPending(RasterBufferMgr instance);
void RbmSetAbortStatus(RasterBufferMgr instance);
void RbmClearAbortStatus(RasterBufferMgr instance);
u32 RbmWaitPpBufferNotUsed(RasterBufferMgr instance);
#endif

#endif /* RASTER_BUFFER_MGR_H */
