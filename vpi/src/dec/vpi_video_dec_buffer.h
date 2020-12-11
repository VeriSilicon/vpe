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

#ifndef __VPI_VIDEO_DEC_BUFFER_H__
#define __VPI_VIDEO_DEC_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "vpi_video_dec.h"

void vpi_dec_buf_list_add(BufLink **head, BufLink *list);
BufLink* vpi_dec_buf_list_delete(BufLink *head);
int vpi_send_packet_to_decode_buffer(VpiDecCtx *vpi_ctx, VpiPacket *vpi_packet,
                                     struct DWLLinearMem stream_buffer);
int vpi_dec_get_stream_buffer_index(VpiDecCtx *vpi_ctx, int status);
void vpi_dec_release_ext_buffers(VpiDecCtx *vpi_ctx);
int vpi_dec_check_buffer_number_for_trans(VpiDecCtx *vpi_ctx);
int vpi_dec_set_pts_decid(VpiDecCtx *vpi_ctx);
void vpi_dec_clear_unused_pts(VpiDecCtx *vpi_ctx);
int vpi_dec_set_pts_dts(VpiDecCtx *vpi_ctx, VpiPacket *pkt);
VpiRet vpi_dec_output_frame(VpiDecCtx *vpi_ctx, VpiFrame *vpi_frame,
                            struct DecPicturePpu *decoded_pic);

#ifdef __cplusplus
}
#endif

#endif
