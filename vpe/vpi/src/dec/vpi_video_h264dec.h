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
