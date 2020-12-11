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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __VPI_VIDEO_H26XENC_H__
#define __VPI_VIDEO_H26XENC_H__
#include "vpi_error.h"
#include "vpi_video_h26xenc_cfg.h"

extern VPIH26xParamsDef h26x_enc_param_table[];

#ifndef NEXT_MULTIPLE
#define NEXT_MULTIPLE(value, n) (((value) + (n)-1) & ~((n)-1))
#endif

VpiRet vpi_h26xe_init(VpiH26xEncCtx *enc_ctx, VpiH26xEncCfg *enc_cfg);
VpiRet vpi_h26xe_encode(VpiH26xEncCtx *enc_ctx, void *input, void *output);
VpiRet vpi_h26xe_get_packet(VpiH26xEncCtx *enc_ctx, void *outdata);
VpiRet vpi_h26xe_close(VpiH26xEncCtx *enc_ctx);
VpiRet vpi_h26xe_ctrl(VpiH26xEncCtx *enc_ctx, void *vpi_ctrl_type,
                   void *vpi_value);
VpiRet vpi_h26xe_put_frame(VpiH26xEncCtx *enc_ctx, void *indata);
#endif /*__VPI_VIDEO_H26XENC_H__ */
