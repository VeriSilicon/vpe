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

#ifndef __VPI_H__
#define __VPI_H__

#include "time.h"
#include "vpi_types.h"

typedef struct VpiBufRef VpiBufRef;

typedef struct VpeVpiCtx {
    uint32_t dummy;
    VpiPlugin plugin;
    void *ctx;
    int fd;
} VpeVpiCtx;

typedef struct VpiHwCtx {
    char *device_name;
    int hw_context;
    int task_id;
    int priority;
    void *sys_info;
} VpiHwCtx;

typedef struct VpiCodecCtx {
    void *vpi_dec_ctx;
    void *vpi_enc_ctx;
    void *vpi_prc_pp_ctx;
    void *vpi_prc_spliter_ctx;
    void *vpi_prc_hwdw_ctx;
    int ref_cnt;
    int fd;
} VpiCodecCtx;

typedef struct statistic {
    uint32_t frame_count;
    uint32_t cycle_mb_avg;
    uint32_t cycle_mb_avg_p1;
    uint32_t cycle_mb_avg_total;
    double ssim_avg;
    uint32_t bitrate_avg;
    uint32_t hw_real_time_avg;
    uint32_t hw_real_time_avg_remove_overlap;
    int total_usage;
    int core_usage_counts[4];
    struct timeval last_frame_encoded_timestamp;
} statistic;

typedef struct VpiDevCtx {
    char device_name[32];
    int fd;
} VpiDevCtx;
#endif
