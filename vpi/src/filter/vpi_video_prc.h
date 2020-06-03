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
