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

#ifndef BQUEUE_H_DEFINED
#define BQUEUE_H_DEFINED

#include "basetype.h"
#include "dwlthread.h"

struct BufferQueue {
  u32 *pic_i;
  u32 ctr;
  u32 queue_size;
  u32 prev_anchor_slot;
#ifdef USE_OUTPUT_RELEASE
  u32* buf_used;
  u32 abort;
  pthread_mutex_t buf_release_mutex;
  pthread_cond_t buf_release_cv;
#endif
};

#define BQUEUE_UNUSED (u32)(0xffffffff)

u32 BqueueInit(struct BufferQueue *bq, u32 num_buffers);
void BqueueRelease(struct BufferQueue *bq);
u32 BqueueNext(struct BufferQueue *bq, u32 ref0, u32 ref1, u32 ref2, u32 b_pic);
void BqueueDiscard(struct BufferQueue *bq, u32 buffer);
#ifdef USE_OUTPUT_RELEASE
u32  BqueueInit2( struct BufferQueue *bq, u32 num_buffers );
void BqueueRelease2( struct BufferQueue *bq );
u32  BqueueNext2( struct BufferQueue *bq, u32 ref0, u32 ref1, u32 ref2, u32 b_pic );
void BqueuePictureRelease( struct BufferQueue *bq, u32 buffer );
u32 BqueueWaitNotInUse( struct BufferQueue *bq);
void BqueueSetBufferAsUsed(struct BufferQueue *bq, u32 buffer);
u32 BqueueWaitBufNotInUse(struct BufferQueue *bq, u32 buffer);
void BqueueSetAbort(struct BufferQueue *bq);
void BqueueClearAbort(struct BufferQueue *bq);
#ifdef USE_EXTERNAL_BUFFER
void BqueueEmpty(struct BufferQueue *bq);
#endif
#endif

#endif /* BQUEUE_H_DEFINED */
