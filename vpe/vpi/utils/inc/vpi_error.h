/*
 * Copyright (c) 2020, VeriSilicon Holdings Co., Ltd. All rights reserved
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

#ifndef __VPI_ERR_H__
#define __VPI_ERR_H__

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
    VPI_SUCCESS                  = SUCCESS,
    VPI_ERR_SW                   = -1,
    VPI_ERR_SYSTEM               = -2,
    VPI_ERR_NO_AP_MEM            = -3,
    VPI_ERR_NO_EP_MEM            = -4,
    VPI_ERR_WRONG_STATE          = -5,
    VPI_ERR_DEVICE               = -6,

    VPI_ERR_ENCODE               = -100,
    VPI_ERR_EWL                  = -101,
    VPI_ERR_ENCODER_INIT         = -102,
    VPI_ERR_ENCODER_OPITION      = -103,
    VPI_ERR_ENCODE_WAITT_BUF     = -104,

    VPI_ERR_DECODE               = -200,
    VPI_ERR_DWL                  = -201,
    VPI_ERR_DECODER_INIT         = -203,
    VPI_ERR_DECODER_OPITION      = -204,
    VPI_ERR_DECODER_DATA         = -205,
    VPI_ERR_DECODE_FORMAT        = -206,

    VPI_ERR_PP                   = -300,
    VPI_ERR_PP_INIT              = -301,
    VPI_ERR_PP_OPITION           = -302,

    VPI_ERR_SPLITER              = -400,
    VPI_ERR_SPLITER_INIT         = -401,
    VPI_ERR_SPLITER_OPITION      = -402,

    VPI_ERR_HWDOWNLOADER         = -500,
    VPI_ERR_HWDOWNLOADER_INIT    = -501,
    VPI_ERR_HWDOWNLOADER_OPITION = -502,

    VPI_ERR_HWUPLOADER           = -600,
    VPI_ERR_HWUPLOADER_INIT      = -601,
    VPI_ERR_HWUPLOADER_OPITION   = -602,

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
