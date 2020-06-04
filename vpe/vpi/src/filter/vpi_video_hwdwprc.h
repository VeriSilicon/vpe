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

#ifndef __VPI_VIDEO_HWDWPRC_H__
#define __VPI_VIDEO_HWDWPRC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "vpi_video_prc.h"

VpiRet vpi_prc_hwdw_init(VpiPrcCtx *vpi_ctx, void *cfg);
VpiRet vpi_prc_hwdw_process(VpiPrcCtx *vpi_ctx, void *indata, void *outdata);
VpiRet vpi_prc_hwdw_control(VpiPrcCtx *vpi_ctx, void *indata, void *outdata);
VpiRet vpi_prc_hwdw_close(VpiPrcCtx *vpi_ctx);

#ifdef __cplusplus
}
#endif

#endif