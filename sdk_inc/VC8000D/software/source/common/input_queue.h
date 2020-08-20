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

#ifndef _INPUT_QUEUE_H_
#define _INPUT_QUEUE_H_

#include "basetype.h"
#include "dwl.h"

/* InputQueue is picture queue indexing module, which manages the
 * buffer references on client's behalf. Module will maintain an index of
 * available and used buffers. When free  buffer is not available module
 * will block the thread asking for a buffer index. */

typedef void* InputQueue; /* Opaque instance variable. */

#define MAX_PIC_BUFFERS 78 //68 //34

/* Functions to initialize and release the InputQueue. */
InputQueue InputQueueInit(i32 n_buffers);
void InputQueueRelease(InputQueue queue);

/* Function to get free buffers from the queue. Returns negative value if the
   free buffer was not found and the buffer count is below the limit. Otherwise
   blocks until the requested buffer is available. */
struct DWLLinearMem *InputQueueGetBuffer(InputQueue queue, u32 wait);

/* Function to wait until all buffers are in available status. */
u32 InputQueueWaitNotUsed(InputQueue queue);

void InputQueueWaitBufNotUsed(InputQueue queue, const u32 *addr);

void InputQueueSetBufAsUsed(InputQueue queue, const u32 *addr);

void InputQueueAddBuffer(InputQueue queue, struct DWLLinearMem *buffer);

/* Return buffer to input queue so that it can be used freely. */
struct DWLLinearMem *InputQueueReturnBuffer(InputQueue queue, const u32 *addr);

u32 InputQueueFindBufferId(InputQueue queue, const u32 *addr);

void InputQueueWaitPending(InputQueue queue);

#ifdef USE_EXTERNAL_BUFFER
void InputQueueSetAbort(InputQueue queue);
void InputQueueClearAbort(InputQueue queue);
void InputQueueReturnAllBuffer(InputQueue queue);
void InputQueueReset(InputQueue queue);
#endif
#endif /* _INPUT_QUEUE_H_ */
