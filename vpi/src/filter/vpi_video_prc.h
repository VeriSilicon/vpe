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

#ifndef __VPI_VIDEO_PRC_H__
#define __VPI_VIDEO_PRC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "vpi_error.h"
#include "trans_edma_api.h"
#include "vpi_video_pp.h"

typedef enum FilterType {
    FILTER_NULL,
    FILTER_PP,
    FILTER_SPLITER,
    FILTER_HW_DOWNLOADER
} FilterType;

typedef struct VpiPrcCtx {
    FilterType filter_type;

    /*hwdownload*/
    EDMA_HANDLE edma_handle;
    int pp_index;

    /*pp filter*/
    VpiPPFilter ppfilter;
} VpiPrcCtx;

VpiRet vpi_vprc_init(VpiPrcCtx *vpi_ctx, void *prc_cfg);
VpiRet vpi_vprc_process(VpiPrcCtx *vpi_ctx, void *indata, void *outdata);
VpiRet vpi_vprc_control(VpiPrcCtx *vpi_ctx, void *indata, void *outdata);
int vpi_vprc_close(VpiPrcCtx *vpi_ctx);

VpiRet vpi_prc_pp_init(VpiPrcCtx *vpi_ctx, void *cfg);
VpiRet vpi_prc_pp_control(VpiPrcCtx *vpi_ctx, void *indata, void *outdata);
VpiRet vpi_prc_pp_process(VpiPrcCtx *vpi_ctx, void *indata, void *outdata);
VpiRet vpi_prc_pp_close(VpiPrcCtx *ctx);

#ifdef __cplusplus
}
#endif

#endif
