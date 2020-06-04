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

#ifndef __VPI_VIDEO_DEC_PICTURE_CONSUME_H__
#define __VPI_VIDEO_DEC_PICTURE_CONSUME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "vpi_video_dec.h"

void init_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx);
uint32_t find_dec_pic_wait_consume_index(VpiDecCtx *vpi_ctx, uint8_t *data);
uint32_t find_dec_pic_wait_consume_empty_index(VpiDecCtx *vpi_ctx);
void add_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx, void *data);
void del_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx, uint8_t *data);
void free_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx);

#ifdef __cplusplus
}
#endif

#endif