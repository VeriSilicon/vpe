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

#ifndef __VPI_VIDEO_HWULPRC_H__
#define __VPI_VIDEO_HWULPRC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dwl.h"
//#include "vpi_video_prc.h"

#define MWL_BUF_DEPTH 68

typedef struct {
    int state;
    VpiFrame *pic;
    struct DWLLinearMem mwl_mem;
} VpiHwUlPic;

typedef struct VpiPrcHwUlCtx {
    void *mwl;

    int mwl_nums_init;
    u32 mwl_item_size;

    uint8_t *p_hugepage_buf_y;
    uint8_t *p_hugepage_buf_uv;
    uint32_t i_hugepage_size_y;
    uint32_t i_hugepage_size_uv;

    VpiFrame *frame;
    VpiPixsFmt format;

    int mwl_nums;

    pthread_mutex_t hw_upload_mutex;

    VpiHwUlPic pic_list[MWL_BUF_DEPTH];
}VpiPrcHwUlCtx;



#ifdef __cplusplus
}
#endif

#endif