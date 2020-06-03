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

#ifndef __VPI_VIDEO_H264DEC_H__
#define __VPI_VIDEO_H264DEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "vpi_video_dec.h"

#ifndef NEXT_MULTIPLE
#define NEXT_MULTIPLE(value, n) (((value) + (n)-1) & ~((n)-1))
#endif

VpiRet vpi_dec_h264_init(const void **inst, struct DecConfig config,
                         const void *dwl);
VpiRet vpi_dec_h264_get_info(VpiDecInst inst, struct DecSequenceInfo *info);
VpiRet vpi_dec_h264_set_info(VpiDecInst inst, struct DecConfig config,
                             struct DecSequenceInfo *info);
enum DecRet vpi_dec_h264_next_picture(VpiDecInst inst,
                                      struct DecPicturePpu *pic);
VpiRet vpi_dec_h264_picture_consumed(VpiDecInst inst, struct DecPicturePpu pic);
VpiRet vpi_dec_h264_end_of_stream(VpiDecInst inst);
void vpi_dec_h264_release(VpiDecInst inst);
VpiRet vpi_dec_h264_use_extra_frm_buffers(const VpiDecInst inst, uint32_t num);
#ifdef USE_EXTERNAL_BUFFER
enum DecRet vpi_dec_h264_get_buffer_info(VpiDecInst inst,
                                         struct DecBufferInfo *buf_info);
enum DecRet vpi_dec_h264_add_buffer(VpiDecInst inst, struct DWLLinearMem *buf);
#endif

int vpi_decode_h264_put_packet(VpiDecCtx *vpi_ctx, void *indata);
int vpi_decode_h264_get_frame(VpiDecCtx *vpi_ctx, void *outdata);
int vpi_decode_h264_dec_process(VpiDecCtx *vpi_ctx);
VpiRet vpi_decode_h264_init(VpiDecCtx *vpi_ctx);
int vpi_decode_h264_dec_frame(VpiDecCtx *vpi_ctx, void *indata, void *outdata);
VpiRet vpi_decode_h264_control(VpiDecCtx *vpi_ctx, void *indata, void *outdata);
int vpi_decode_h264_close(VpiDecCtx *vpi_ctx);

#ifdef __cplusplus
}
#endif

#endif
