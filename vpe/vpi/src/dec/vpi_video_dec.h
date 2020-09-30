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

#ifndef __VPI_VIDEO_DEC_H__
#define __VPI_VIDEO_DEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#include <time.h>

// VPE Codec SDK Header File
#include "basetype.h"
#include "dectypes.h"
#include "dwl.h"
#include "h264decapi.h"
#include "hevcdecapi.h"
#include "vp9decapi.h"

#include "vpi_types.h"
#include "vpi_video_dec_tb_defs.h"
#include "vpi_error.h"
#include "vpi.h"

#ifdef SW_PERFORMANCE
#define INIT_SW_PERFORMANCE                                                    \
    double dec_cpu_time    = 0;                                                \
    clock_t dec_start_time = 0;                                                \
    clock_t dec_end_time   = 0;
#else
#define INIT_SW_PERFORMANCE
#endif

#ifdef SW_PERFORMANCE
#define START_SW_PERFORMANCE dec_start_time = clock();
#else
#define START_SW_PERFORMANCE
#endif

#ifdef SW_PERFORMANCE
#define END_SW_PERFORMANCE                                                     \
    dec_end_time = clock();                                                    \
    dec_cpu_time += ((double)(dec_end_time - dec_start_time)) / CLOCKS_PER_SEC;
#else
#define END_SW_PERFORMANCE
#endif

#ifdef SW_PERFORMANCE
#define FINALIZE_SW_PERFORMANCE printf("SW_PERFORMANCE %0.5f\n", dec_cpu_time);
#else
#define FINALIZE_SW_PERFORMANCE
#endif

#ifdef SW_PERFORMANCE
#define FINALIZE_SW_PERFORMANCE_PP                                             \
    printf("SW_PERFORMANCE_PP %0.5f\n", dec_cpu_time);
#else
#define FINALIZE_SW_PERFORMANCE_PP
#endif

#define MAX_BUFFERS 78
#define MAX_WAIT_FOR_CONSUME_BUFFERS 100
#define VDEC_NOPTS_VALUE ((int64_t)UINT64_C(0x8000000000000000))
/* one extra stream buffer so that we can decode ahead,
 * and be ready when core has finished
 */
#define MAX_STRM_BUFFERS (MAX_ASIC_CORES + 1)
#define MAX_PTS_DTS_DEPTH 78

#ifdef FB_SYSLOG_ENABLE
#include "syslog_sink.h"
#define VPI_DEC_INFO_PRINT(fmt, ...)                                           \
    FB_SYSLOG(&vpi_ctx->log_header, SYSLOG_SINK_LEV_INFO, fmt, ##__VA_ARGS__)
#define VPI_DEC_ERROR_PRINT(fmt, ...)                                          \
    FB_SYSLOG(&vpi_ctx->log_header, SYSLOG_SINK_LEV_ERROR, "%s([%d]): " fmt,   \
              __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define VPI_DEC_DEBUG_PRINT(fmt, ...)                                          \
    FB_SYSLOG(&vpi_ctx->log_header, SYSLOG_SINK_LEV_DEBUG_SW,                  \
              "%s([%d]): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define VPI_DEC_DEBUGV_PRINT(fmt, ...)                                         \
    FB_SYSLOG(&vpi_ctx->log_header, SYSLOG_SINK_LEV_DEBUG_SW_VERBOSE,          \
              "%s([%d]): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

typedef const void *VpiDecInst;

typedef enum DecFormat { Dec_NULL, Dec_VP9, Dec_HEVC, Dec_H264_H10P } DecFormat;

typedef enum VpiEncType {
    VPI_ENC_NONE,
    VPI_ENC_VALID
} VpiEncType;

typedef struct ResizeType {
    int x;
    int y;
    int cw;
    int ch;
    int sw;
    int sh;
} ResizeType;

typedef struct BufLink {
    void *item;
    void *opaque;
    uint32_t mem_idx;
    int item_size;
    int used;
    struct BufLink *next;
}BufLink;

struct DecOutput {
    uint8_t *strm_curr_pos;
    addr_t strm_curr_bus_address;
    uint32_t data_left;
    uint8_t *strm_buff;
    addr_t strm_buff_bus_address;
    uint32_t buff_size;
};

typedef struct VpiDecPicWaitForConsume {
    struct DecPicturePpu *pic;
    uint8_t wait_for_consume;
} VpiDecPicWaitForConsume;

typedef struct TimeStampInfo {
    int64_t pts;
    int64_t pkt_dts;
    uint32_t decode_id;
    int used;
} TimeStampInfo;

struct VpiDecWrapper {
    void *inst;
    VpiRet (*init)(const void **inst, struct DecConfig config, const void *dwl);
    VpiRet (*get_info)(VpiDecInst inst, struct DecSequenceInfo *info);
    VpiRet (*set_info)(VpiDecInst inst, struct DecConfig info,
                       struct DecSequenceInfo *Info);
    VpiRet (*decode)(VpiDecInst inst, struct DWLLinearMem input,
                     struct DecOutput *output, uint8_t *stream,
                     uint32_t strm_len, uint32_t pic_id, void *p_user_data);
    enum DecRet (*next_picture)(VpiDecInst inst, struct DecPicturePpu *pic);
    VpiRet (*picture_consumed)(VpiDecInst inst, struct DecPicturePpu pic);
    VpiRet (*end_of_stream)(VpiDecInst inst);
    void (*release)(VpiDecInst inst);
#ifdef USE_EXTERNAL_BUFFER
    enum DecRet (*get_buffer_info)(VpiDecInst inst,
                                   struct DecBufferInfo *buf_info);
    enum DecRet (*add_buffer)(VpiDecInst inst, struct DWLLinearMem *buf);
#endif
    enum DecRet (*use_extra_frm_buffers)(const VpiDecInst inst, uint32_t n);
};

typedef struct VpiDecCtx {
    VpiDecInst dec_inst;
    const void *dwl_inst;
    struct DWLInitParam dwl_init;

    // system cfg
    DecFormat dec_fmt;
    VpiEncType enc_type;
    uint8_t disable_dec400;
    uint8_t disable_dtrc;
    uint32_t low_latency;
    uint32_t process_end_flag;
    uint32_t enable_mvc;
    uint32_t bus_width;

    struct VpiDecWrapper vpi_dec_wrapper;
    struct DecConfig vpi_dec_config;

    // picture consume
    pthread_mutex_t consume_mutex;
    VpiDecPicWaitForConsume wait_for_consume_list[MAX_WAIT_FOR_CONSUME_BUFFERS];
    uint32_t wait_consume_num;

#ifdef FB_SYSLOG_ENABLE
    LOG_INFO_HEADER log_header;
    char module_name[16];
#endif

    // tb cfg
    uint8_t *stream_stop;
    DecPicAlignment align; /* default: 128 bytes alignment */
    uint32_t clock_gating;
    uint32_t data_discard;
    uint32_t latency_comp;
    uint32_t output_picture_endian;
    uint32_t bus_burst_length;
    uint32_t asic_service_priority;
    uint32_t output_format;
    uint32_t service_merge_disable;
    uint32_t tiled_output;
    uint32_t dpb_mode;
    uint8_t pp_units_params_from_cmd_valid;
    struct TBPpUnitParams pp_units_params_from_cmd[4];
    uint32_t enable_mc;
    uint32_t mb_error_concealment;
    uint32_t rlc_mode;
    uint32_t seed_rnd;
    uint32_t strm_swap;
    uint32_t pic_swap;
    uint32_t dirmv_swap;
    uint32_t tab0_swap;
    uint32_t tab1_swap;
    uint32_t tab2_swap;
    uint32_t tab3_swap;
    uint32_t rscan_swap;
    uint32_t max_burst;
    uint32_t double_ref_buffer;
    uint32_t timeout_cycles;
    struct TBCfg tb_cfg;

    // external frame buffer
    uint32_t use_extra_buffers_num;
    uint32_t buffer_release_flag;
    uint32_t cycle_count; /* Sum of average cycles/mb counts */
    uint32_t num_buffers; /* external buffers allocated yet. */
    uint32_t buffer_size;
    uint32_t min_buffer_num;
    struct DWLLinearMem ext_buffers[MAX_BUFFERS];
    uint32_t buffer_consumed[MAX_BUFFERS];
    BufLink *frame_buf_head;
    BufLink *frame_buf_list[MAX_BUFFERS];
    uint32_t frame_buf_list_idx;
    BufLink *frame_stored_list[200];
    uint32_t frame_stored_num;
    int max_frames_delay;
    int output_num;

    // resize
    ResizeType resizes[4];
    int resize_num;
    int vce_ds_enable;
    uint32_t pp_enabled;

    // stream buffer
    long int max_strm_len;
    uint32_t allocated_buffers;
    uint32_t stream_mem_index;
    struct DWLLinearMem stream_mem[MAX_STRM_BUFFERS];
    uint32_t stream_mem_used[MAX_STRM_BUFFERS];
    BufLink *strm_buf_head;
    BufLink *strm_buf_list[MAX_STRM_BUFFERS];
    TimeStampInfo time_stamp_info[MAX_PTS_DTS_DEPTH];
    uint32_t eos_received;
    uint32_t eos_handled;
    BufLink *rls_strm_buf_head;
    BufLink *rls_strm_buf_list[32];
    uint32_t rls_mem_index;
    int eos_flush;
    int64_t last_pts;

    // dec statistics data
    uint32_t pic_display_number;
    uint32_t pic_decode_number;
    uint32_t prev_width;
    uint32_t prev_height;
    uint32_t got_package_number;
    uint32_t last_pic_flag;
    uint32_t max_num_pics;

    // decode status
    uint32_t pic_rdy;
    uint32_t hdrs_rdy;
    uint32_t res_changed;
    int dec_error;
    int64_t pts;
    int64_t pkt_dts;
    int64_t duration;
    struct DecSequenceInfo sequence_info;

    // decode process
    pthread_t dec_thread_handle;
    pthread_mutex_t dec_thread_mutex;
    pthread_cond_t dec_thread_cond;
    int waiting_for_dpb;
    int dec_thread_finish;

    // pic info
    struct DecPicturePpu pic;
    uint32_t pic_size;
    int src_width;
    int src_height;

    // vpe_codec_sdk
#ifdef USE_EXTERNAL_BUFFER
    H264DecBufferInfo h264_hbuf;
    struct HevcDecBufferInfo hevc_hbuf;
    struct Vp9DecBufferInfo vp9_hbuf;
#endif
    H264DecOutput h264_dec_output;
    H264DecInput h264_dec_input;
    struct HevcDecInput hevc_dec_input;
    struct DecOutput dec_output;
    struct Vp9DecInput vp9_dec_input;

    /* output file writing disable */
    uint32_t retry;

    VpiFrame *frame;

    int init_finish;
} VpiDecCtx;

VpiRet vpi_vdec_init(VpiDecCtx *, void *);
int vpi_vdec_decode(VpiDecCtx *, void *, void *);
int vpi_vdec_put_packet(VpiDecCtx *, void *);
int vpi_vdec_get_frame(VpiDecCtx *, void *);
VpiRet vpi_vdec_control(VpiDecCtx *, void *, void *);
VpiRet vpi_vdec_close(VpiDecCtx *);

#ifdef __cplusplus
}
#endif

#endif
