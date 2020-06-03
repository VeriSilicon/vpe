/*
 * Copyright 2019 VeriSilicon, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
