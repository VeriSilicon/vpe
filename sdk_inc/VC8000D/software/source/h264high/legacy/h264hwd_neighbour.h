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

#ifndef H264HWD_NEIGHBOUR_H
#define H264HWD_NEIGHBOUR_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

#include "h264hwd_macroblock_layer.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

typedef enum {
  MB_A = 0,
  MB_B,
  MB_C,
  MB_D,
  MB_CURR,
  MB_NA = 0xFF
} neighbourMb_e;

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

typedef struct {
  neighbourMb_e   mb;
  u8             index;
} neighbour_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

void h264bsdInitMbNeighbours(mbStorage_t *p_mb_storage, u32 pic_width,
                             u32 pic_size_in_mbs);

/*@null@*/ mbStorage_t* h264bsdGetNeighbourMb(mbStorage_t *p_mb,
    neighbourMb_e neighbour);

u32 h264bsdIsNeighbourAvailable(mbStorage_t *p_mb,
                                /*@null@*/ mbStorage_t *p_neighbour);

const neighbour_t* h264bsdNeighbour4x4BlockA(u32 block_index);
const neighbour_t* h264bsdNeighbour4x4BlockB(u32 block_index);
const neighbour_t* h264bsdNeighbour4x4BlockC(u32 block_index);
const neighbour_t* h264bsdNeighbour4x4BlockD(u32 block_index);

#endif /* #ifdef H264HWD_NEIGHBOUR_H */
