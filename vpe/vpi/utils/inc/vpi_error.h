/*
 * Copyright (c) 2020, VeriSilicon Inc. All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __VPI_ERROR_H__
#define __VPI_ERROR_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NDEBUG
#include <assert.h>
#endif

#ifndef SUCCESS
#define SUCCESS (0)
#endif

typedef enum {
    VPI_SUCCESS = SUCCESS,

    VPI_ERR_UNKNOWN      = -1,
    VPI_ERR_MALLOC       = -2,
    VPI_ERR_WRONG_STATE  = -3,
    VPI_ERR_OPEN_FILE    = -4,
    VPI_ERR_WRONG_PLUGIN = -5,
    VPI_ERR_VALUE        = -6,
    VPI_ERR_READ_BIT     = -7,
    VPI_ERR_TIMEOUT      = -8,
    VPI_ERR_PERM         = -9,
    VPI_ERR_NO_SW_RSC    = -10,
    VPI_ERR_NO_HW_RSC    = -11,
    VPI_ERR_IOCTL        = -12,
    VPI_ERR_INVALID_PARAM= -13,

    VPI_ERR_BASE = -1000,

    /* The error in stream processing */
    VPI_ERR_LIST_STREAM    = VPI_ERR_BASE - 1,
    VPI_ERR_INIT           = VPI_ERR_BASE - 2,
    VPI_ERR_VPU_CODEC_INIT = VPI_ERR_BASE - 3,
    VPI_ERR_STREAM         = VPI_ERR_BASE - 4,
    VPI_ERR_FATAL_THREAD   = VPI_ERR_BASE - 5,
    VPI_ERR_NOMEM          = VPI_ERR_BASE - 6,
    VPI_ERR_PROTOL         = VPI_ERR_BASE - 7,
    VPI_FAIL_SPLIT_FRAME   = VPI_ERR_BASE - 8,
    VPI_ERR_VPUHW          = VPI_ERR_BASE - 9,
    VPI_EOS_STREAM_REACHED = VPI_ERR_BASE - 11,
    VPI_ERR_BUFFER_FULL    = VPI_ERR_BASE - 12,
    VPI_ERR_DISPLAY_FULL   = VPI_ERR_BASE - 13,
} VpiRet;

/* ASSERT */
#ifndef ASSERT
#ifndef NDEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VPI_ERROR_H__ */
