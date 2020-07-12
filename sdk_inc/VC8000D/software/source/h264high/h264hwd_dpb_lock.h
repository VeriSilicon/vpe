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

#ifndef H264HIGH_H264HWD_DPB_LOCK_H_
#define H264HIGH_H264HWD_DPB_LOCK_H_

#include "basetype.h"
#include "h264decapi.h"
#include "h264hwd_conflicts.h"

#include <dwlthread.h>

#define MAX_FRAME_BUFFER_NUMBER    78 // 68 //34
#define FB_NOT_VALID_ID             ~0U

#define FB_HW_OUT_FIELD_TOP         0x10U
#define FB_HW_OUT_FIELD_BOT         0x20U
#define FB_HW_OUT_FRAME             (FB_HW_OUT_FIELD_TOP | FB_HW_OUT_FIELD_BOT)
#define ABORT_MARKER                 2
#define FLUSH_MARKER                 3

typedef struct FrameBufferStatus_ {
  u32 n_ref_count;
  u32 b_used;
  const void * data;
} FrameBufferStatus;

typedef struct OutElement_ {
  u32 mem_idx;
  H264DecPicture pic;
} OutElement;

typedef struct FrameBufferList_ {
  int b_initialized;
  struct FrameBufferStatus_ fb_stat[MAX_FRAME_BUFFER_NUMBER];
  struct OutElement_ out_fifo[MAX_FRAME_BUFFER_NUMBER];
  int wr_id;
  int rd_id;
  int free_buffers;
  int num_out;
#ifdef USE_OUTPUT_RELEASE
  u32 abort;
  u32 flush_all;
#endif
  struct {
    int id;
    const void * desc;
  } last_out;

  sem_t out_count_sem;
  pthread_mutex_t out_count_mutex;
  pthread_cond_t out_empty_cv;
  pthread_mutex_t ref_count_mutex;
  pthread_cond_t ref_count_cv;
  pthread_cond_t hw_rdy_cv;
} FrameBufferList;

struct dpbStorage;
struct H264DecPicture_;

u32 InitList(FrameBufferList *fb_list);
void ReleaseList(FrameBufferList *fb_list);

u32 AllocateIdUsed(FrameBufferList *fb_list, const void * data);
u32 AllocateIdFree(FrameBufferList *fb_list, const void * data);
void ReleaseId(FrameBufferList *fb_list, u32 id);
void * GetDataById(FrameBufferList *fb_list, u32 id);
u32 GetIdByData(FrameBufferList *fb_list, const void *data);

void IncrementRefUsage(FrameBufferList *fb_list, u32 id);
void DecrementRefUsage(FrameBufferList *fb_list, u32 id);

void IncrementDPBRefCount(struct dpbStorage *dpb);
void DecrementDPBRefCount(struct dpbStorage *dpb);

void MarkHWOutput(FrameBufferList *fb_list, u32 id, u32 type);
void ClearHWOutput(FrameBufferList *fb_list, u32 id, u32 type, u32 pp_enabled);

void MarkTempOutput(FrameBufferList *fb_list, u32 id);
void ClearOutput(FrameBufferList *fb_list, u32 id);

void FinalizeOutputAll(FrameBufferList *fb_list);
void RemoveTempOutputAll(FrameBufferList *fb_list);

#ifdef USE_OUTPUT_RELEASE
void RemoveOutputAll(FrameBufferList *fb_list);
void SetAbortStatusInList(FrameBufferList *fb_list);
void ClearAbortStatusInList(FrameBufferList *fb_list);
void ResetOutFifoInList(FrameBufferList *fb_list);
#endif

u32 GetFreePicBuffer(FrameBufferList * fb_list, u32* old_id, u32* is_free);
void SetFreePicBuffer(FrameBufferList * fb_list, u32 id);
u32 GetFreeBufferCount(FrameBufferList *fb_list);

void PushOutputPic(FrameBufferList *fb_list, const struct H264DecPicture_ *pic,
                   u32 id);
u32 PeekOutputPic(FrameBufferList *fb_list, struct H264DecPicture_ *pic);
u32 PopOutputPic(FrameBufferList *fb_list, u32 id);

void MarkOutputPicInfo(FrameBufferList *fb_list, u32 id, u32 errors, u32 cycles);

u32 IsBufferReferenced(FrameBufferList *fb_list, u32 id);
u32 IsOutputEmpty(FrameBufferList *fb_list);
u32 IsBufferOutput(FrameBufferList *fb_list, u32 id);

#ifdef _HAVE_PTHREAD_H
void WaitOutputEmpty(FrameBufferList *fb_list);
void WaitListNotInUse(FrameBufferList *fb_list);
#else
int WaitOutputEmpty(FrameBufferList *fb_list);
int WaitListNotInUse(FrameBufferList *fb_list);
#endif

void AbortList(FrameBufferList *fb_list);
void MarkIdAllocated(FrameBufferList *fb_list, u32 id);
void MarkIdFree(FrameBufferList *fb_list, u32 id);
void ClearTempOut(FrameBufferList *fb_list, u32 id);
#endif  /*  H264HIGH_H264HWD_DPB_LOCK_H_ */
