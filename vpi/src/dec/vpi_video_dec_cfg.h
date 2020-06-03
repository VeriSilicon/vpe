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

#ifndef __VPI_VIDEO_DEC_CFG_H__
#define __VPI_VIDEO_DEC_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "vpi_video_dec.h"

VpiRet vpi_check_out_format_for_trans(VpiDecCtx *vpi_ctx, DecOption *dec_cfg);
VpiRet vpi_dec_init_wrapper(VpiDecCtx *vpi_ctx);
void vpi_dec_set_default_config(VpiDecCtx *vpi_ctx);
void vpi_dec_init_hw_cfg(VpiDecCtx *vpi_ctx, DecOption *dec_cfg);
void vpi_dec_get_tb_cfg(VpiDecCtx *vpi_ctx);
int vpi_dec_cfg_by_seqeuence_info(VpiDecCtx *vpi_ctx);

#ifdef __cplusplus
}
#endif

#endif