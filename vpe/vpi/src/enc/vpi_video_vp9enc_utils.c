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

#include <string.h>
#include "cwl.h"
#include "dectypes.h"
#include "fb_performance.h"

#include "vpi.h"
#include "vpi_types.h"
#include "vpi_log.h"
#include "vpi_video_vp9enc_utils.h"

#ifdef FB_SYSLOG_ENABLE
#include "syslog_sink.h"
static LOG_INFO_HEADER vp9enc_tb = { "VP9ENC", -1 };
void *tb                         = (void *)&vp9enc_tb;
#define TBPS(fmt, ...) FB_SYSLOG(tb, SYSLOG_SINK_LEV_STAT, fmt, ##__VA_ARGS__)
#define TBPI(fmt, ...) FB_SYSLOG(tb, SYSLOG_SINK_LEV_INFO, fmt, ##__VA_ARGS__)
#define TBPE(fmt, ...)                                                         \
    FB_SYSLOG(tb, SYSLOG_SINK_LEV_ERROR, "%s([%d]): " fmt, __FUNCTION__,       \
              __LINE__, ##__VA_ARGS__)
#define TBPD(fmt, ...)                                                         \
    FB_SYSLOG(tb, SYSLOG_SINK_LEV_DEBUG_SW, "%s([%d]): " fmt, __FUNCTION__,    \
              __LINE__, ##__VA_ARGS__)
#define TBPV(fmt, ...)                                                         \
    FB_SYSLOG(tb, SYSLOG_SINK_LEV_DEBUG_SW_VERBOSE, "%s([%d]): " fmt,          \
              __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define TBP_FLUSH() FB_SYSLOG_FLUSH()
#endif

typedef enum VP9Preset {
    VP9ENC_PRESET_NONE,
    VP9ENC_PRESET_SUPERFAST,
    VP9ENC_PRESET_FAST,
    VP9ENC_PRESET_MEDIUM,
    VP9ENC_PRESET_SLOW,
    VP9ENC_PRESET_SUPERSLOW,
    VP9ENC_PRESET_NUM
} VP9Preset;

static const char firstpass[] = "firstpass.out";

#define KHW_PIC_MEM_HEAD_PADDING 0
#define KHW_PIC_MEM_TAIL_PADDING 16 * 1024
#define STATOTAL PERFORMANCE_STATIC_GET_TOTAL

static int trans_default_bitrate[] = { 100000,  250000,  500000,  1000000,
                                       3000000, 5000000, 10000000 };

static u32 tempora_layer_tic_scale[4][4] = { { 1, 0, 0, 0 },
                                             { 2, 2, 0, 0 },
                                             { 4, 4, 2, 0 },
                                             { 8, 8, 4, 2 } };

u32 random_mz = 1;
u32 random_mw = 1;

static u32 next_rand()
{
    random_mz = 36969 * (random_mz & 0xffff) + (random_mz >> 16);
    random_mw = 18000 * (random_mw & 0xffff) + (random_mw >> 16);
    return (random_mz << 16) + random_mw;
}

static int get_res_index(int w, int h)
{
    if (w * h >= 3840 * 2160) {
        return 6;
    } else if (w * h >= 1920 * 1080) {
        return 5;
    } else if (w * h >= 1280 * 720) {
        return 4;
    } else if (w * h >= 854 * 480) {
        return 3;
    } else if (w * h >= 640 * 360) {
        return 2;
    } else if (w * h >= 428 * 240) {
        return 1;
    }
    return 0;
}

void vp9enc_test_segmentation(VpiEncVp9Ctx *ctx, int pic_width, int pic_height,
                              VP9EncInst encoder, VpiEncVp9Setting *ecfg)
{
    int i = 0;
    static Vp9EncSegmentCtrl segment_ctrl;
    /*static u8 segment_tree_probs[7];
      static u8 segment_pred_probs[3];*/
    int num_sbs = ((pic_width + 63) / 64) * ((pic_height + 63) / 64);
    bool update = false;
    u32 sw_mixed_segment_penalty = 0;

    if (ctx == NULL || encoder == 0) {
        VPILOGE("vp9enc_test_segmentation input error\n");
        return;
    }

    if (!ctx->seg_buf) {
        ctx->seg_buf = (u8 *)malloc(num_sbs * 32 * sizeof(u8));
    }

    segment_ctrl.enable = 1;
    if (ecfg->segment_map == NULL) {
        update                             = true;
        segment_ctrl.filterLevelSegment[0] = -1;
        segment_ctrl.quantSegment[0]       = -1;
        for (i = 1; i < 8; ++i) {
            segment_ctrl.filterLevelSegment[i] = next_rand() % 64;
            segment_ctrl.quantSegment[i]       = next_rand() % 256;
        }
        for (i = 0; i < num_sbs * 32; i++) {
            ctx->seg_buf[i] = next_rand();
        }
        /*for (i = 0; i < 7; i++)
            segment_tree_probs[i] = next_rand();
          for (i = 0; i < 3; i++)
            segment_pred_probs[i] = next_rand();*/
#if 0 /* FIX ME*/
        segment_ctrl.segment_pred_probs =  &segment_pred_probs;
        segment_ctrl.segment_tree_probs =  &segment_tree_probs;
#endif
        sw_mixed_segment_penalty = next_rand() % 65536;
    }

    segment_ctrl.fgbg_map = ctx->seg_buf;
    if (update) {
        VP9EncSetSegmentation(encoder, &segment_ctrl, sw_mixed_segment_penalty);
    }
}

static void free_hw_pic_mem(VP9EncInst enc, CWLLinearMem_t *mem)
{
    if (enc && mem && mem->virtualAddress != NULL) {
        void *cwl = VP9EncGetCWL(enc);
        /* Fix the virtualAddress we munged when allocating */
        mem->virtualAddress =
            ((u8 *)mem->virtualAddress) - KHW_PIC_MEM_HEAD_PADDING;
        CWLFreeRefFrm(cwl, mem);
    }
}

static i32 allocate_hw_linear_mem(VP9EncInst enc, u32 size, CWLLinearMem_t *mem)
{
    i32 ret;
    void *cwl = VP9EncGetCWL(enc);

    /* Here we use the CWL instance directly from the encoder
    * because it is the easiest way to allocate the linear memories
    */
    ret = CWLMallocInoutLinear(cwl, size, mem);
    if (ret != CWL_OK) {
        mem->virtualAddress = NULL;
    }

    return ret;
}

static void free_hw_linear_mem(VP9EncInst enc, CWLLinearMem_t *mem)
{
    void *cwl = VP9EncGetCWL(enc);

    if (enc && mem && mem->virtualAddress) {
        CWLFreeInoutLinear(cwl, mem);
    }
}

void vp9enc_statistic(VpiEncVp9Ctx *ctx)
{
    struct statistic sta;
    int j            = 0;
    VP9ENCPERF *perf = &ctx->perf;

    memset(&sta, 0, sizeof(statistic));
    sta.frame_count = ctx->frame_count_out;
    VPILOGD("ssim_sum=%f, frame number= %d \n", ctx->ssim_sum,
            ctx->frame_count_out);

    if (sta.frame_count) {
        sta.ssim_avg    = ctx->ssim_sum / sta.frame_count;
        sta.bitrate_avg = ctx->bitrate;
#ifdef FB_PERFORMANCE_STATIC
        sta.cycle_mb_avg    = perf->hwcycle_acc / sta.frame_count / ctx->mbs;
        sta.cycle_mb_avg_p1 = perf->hwcycle_accp1 / sta.frame_count / ctx->mbs;
        sta.cycle_mb_avg_total =
            perf->hwcycle_acc_total / sta.frame_count / ctx->mbs;

        sta.hw_real_time_avg =
            (STATOTAL(tb, perf, vp9hw) + STATOTAL(tb, perf, vp9hw_p1)) /
            sta.frame_count;
        sta.hw_real_time_avg_remove_overlap =
            STATOTAL(tb, perf, vp9hw_total) / sta.frame_count;
        sta.last_frame_encoded_timestamp = perf->last_frame_encoded_timestamp;
        sta.core_usage_counts[0]         = perf->core_usage_counts[0];
        sta.core_usage_counts[1]         = perf->core_usage_counts[1];
        sta.core_usage_counts[2]         = perf->core_usage_counts_p1[0];
        sta.core_usage_counts[3]         = perf->core_usage_counts_p1[1];
        sta.total_usage = sta.core_usage_counts[0] + sta.core_usage_counts[1] +
                          sta.core_usage_counts[2] + sta.core_usage_counts[3];
#endif
    }

    VPILOGI("ENC[%d] : %d frames, SSIM %.4f, %d Cycles/MB, \
            %d us/frame, %.2f fps, %u bps\n",
            ctx->enc_index, sta.frame_count, sta.ssim_avg, sta.cycle_mb_avg,
            sta.hw_real_time_avg,
            (sta.hw_real_time_avg == 0) ?
                0.0 :
                1000000.0 / ((double)sta.hw_real_time_avg),
            sta.bitrate_avg);

    if (sta.cycle_mb_avg_p1) {
        VPILOGI("\tPass 1 : %d Cycles/MB\n", sta.cycle_mb_avg_p1);
        VPILOGI("\tPass 2 : %d Cycles/MB\n", sta.cycle_mb_avg);
    }

    if (sta.hw_real_time_avg > sta.hw_real_time_avg_remove_overlap + 10) {
        VPILOGI("\tremove overlap : %d us/frame, %.2f fps\n",
                sta.hw_real_time_avg_remove_overlap,
                (sta.hw_real_time_avg_remove_overlap == 0) ?
                    0.0 :
                    1000000.0 / ((double)sta.hw_real_time_avg_remove_overlap));
    }

    VPILOGI("staENC[%d] Multi-core usage statistics:\n", ctx->enc_index);

    if (sta.total_usage == 0)
        sta.total_usage = 1;

    for (j = 0; j < 2; j++) {
        if (sta.core_usage_counts[2] || sta.core_usage_counts[3]) {
            VPILOGI("\tPass 1 Slice[%d] used %6d times (%2d%%)\n", j,
                    sta.core_usage_counts[2 + j],
                    (sta.core_usage_counts[2 + j] * 100) / sta.total_usage);
        }
    }
    for (j = 0; j < 2; j++) {
        VPILOGI("\tSlice[%d] used %6d times (%2d%%)\n", j,
                sta.core_usage_counts[j],
                (sta.core_usage_counts[j] * 100) / sta.total_usage);
    }

    TBPS("ENC[%d] : %d frames, SSIM %.4f, %d Cycles/MB, \
                            %d us/frame, %.2f fps, %u bps\n",
         ctx->enc_index, sta.frame_count, sta.ssim_avg, sta.cycle_mb_avg,
         sta.hw_real_time_avg,
         (sta.hw_real_time_avg == 0) ?
             0.0 :
             1000000.0 / ((double)sta.hw_real_time_avg),
         sta.bitrate_avg);

    if (sta.cycle_mb_avg_p1) {
        TBPS("\tPass 1 : %d Cycles/MB\n", sta.cycle_mb_avg_p1);
        TBPS("\tPass 2 : %d Cycles/MB\n", sta.cycle_mb_avg);
    }

    if (sta.hw_real_time_avg > sta.hw_real_time_avg_remove_overlap + 10) {
        TBPS("\tremove overlap : %d us/frame, %.2f fps\n",
             sta.hw_real_time_avg_remove_overlap,
             (sta.hw_real_time_avg_remove_overlap == 0) ?
                 0.0 :
                 1000000.0 / ((double)sta.hw_real_time_avg_remove_overlap));
    }

    TBPS("staENC[%d] Multi-core usage statistics:\n", ctx->enc_index);

    if (sta.total_usage == 0)
        sta.total_usage = 1;

    for (j = 0; j < 2; j++) {
        if (sta.core_usage_counts[2] || sta.core_usage_counts[3]) {
            TBPS("\tPass 1 Slice[%d] used %6d times (%2d%%)\n", j,
                 sta.core_usage_counts[2 + j],
                 (sta.core_usage_counts[2 + j] * 100) / sta.total_usage);
        }
    }
    for (j = 0; j < 2; j++) {
        TBPS("\tSlice[%d] used %6d times (%2d%%)\n", j,
             sta.core_usage_counts[j],
             (sta.core_usage_counts[j] * 100) / sta.total_usage);
    }

#ifdef FB_PERFORMANCE_STATIC
    PERFORMANCE_STATIC_REPORT(tb, perf, vp9hw);
    PERFORMANCE_STATIC_VERBOSE(tb, perf, vp9hw);
    PERFORMANCE_STATIC_REPORT(tb, perf, vp9hw_p1);
    PERFORMANCE_STATIC_VERBOSE(tb, perf, vp9hw_p1);
    PERFORMANCE_STATIC_REPORT(tb, perf, vp9hw_total);
    PERFORMANCE_STATIC_VERBOSE(tb, perf, vp9hw_total);
#endif
}

void vp9enc_ma_add_frame(vp9_ma_s *ma, int frame_size_bits)
{
    ma->frame[ma->pos++] = frame_size_bits;

    if (ma->pos == ma->length)
        ma->pos = 0;

    if (ma->count < ma->length)
        ma->count++;
}

void vp9enc_print_error_value(const char *error_desc, int return_bal)
{
    char str[256];

    switch (return_bal) {
    case VP9ENC_ERROR:
        strcpy(str, (const char *)"VP9ENC_ERROR");
        break;
    case VP9ENC_NULL_ARGUMENT:
        strcpy(str, "VP9ENC_NULL_ARGUMENT");
        break;
    case VP9ENC_INVALID_ARGUMENT:
        strcpy(str, "VP9ENC_INVALID_ARGUMENT");
        break;
    case VP9ENC_MEMORY_ERROR:
        strcpy(str, "VP9ENC_MEMORY_ERROR");
        break;
    case VP9ENC_CWL_ERROR:
        strcpy(str, "VP9ENC_CWL_ERROR");
        break;
    case VP9ENC_CWL_MEMORY_ERROR:
        strcpy(str, "VP9ENC_CWL_MEMORY_ERROR");
        break;
    case VP9ENC_INVALID_STATUS:
        strcpy(str, "VP9ENC_INVALID_STATUS");
        break;
    case VP9ENC_OUTPUT_BUFFER_OVERFLOW:
        strcpy(str, "VP9ENC_OUTPUT_BUFFER_OVERFLOW");
        break;
    case VP9ENC_HW_BUS_ERROR:
        strcpy(str, "VP9ENC_HW_BUS_ERROR");
        break;
    case VP9ENC_HW_DATA_ERROR:
        strcpy(str, "VP9ENC_HW_DATA_ERROR");
        break;
    case VP9ENC_HW_TIMEOUT:
        strcpy(str, "VP9ENC_HW_TIMEOUT");
        break;
    case VP9ENC_HW_RESERVED:
        strcpy(str, "VP9ENC_HW_RESERVED");
        break;
    case VP9ENC_SYSTEM_ERROR:
        strcpy(str, "VP9ENC_SYSTEM_ERROR");
        break;
    case VP9ENC_INSTANCE_ERROR:
        strcpy(str, "VP9ENC_INSTANCE_ERROR");
        break;
    case VP9ENC_HRD_ERROR:
        strcpy(str, "VP9ENC_HRD_ERROR");
        break;
    case VP9ENC_HW_RESET:
        strcpy(str, "VP9ENC_HW_RESET");
        break;
    case VP9ENC_FIRSTPASS_FAILED:
        strcpy(str, "VP9ENC_FIRSTPASS_FAILED");
        break;
    default:
        strcpy(str, "UNDEFINED");
    }

    VPILOGD("%s Return value: %s\n", error_desc, str);
}

void vp9enc_hw_performance(VpiEncVp9Ctx *ctx)
{
    VpiEncVp9Setting *cfg      = &ctx->vp9_enc_cfg;
    u64 perf_last_frame        = 0;
    u64 dram_reads_last_frame  = 0;
    u64 dram_writes_last_frame = 0;
    int i                      = 0;
    VP9EncInst encoder         = ctx->encoder;
    VP9ENCPERF *perf           = &ctx->perf;
    HwPerfCounter counters[128];
    u32 counter_count = sizeof(counters) / sizeof(counters[0]);

    VP9EncGetHwPerfCounter(encoder, counters, &counter_count);

    if (counter_count > 0)
        ctx->frm_cnt_hw_total += 1;
#ifdef FB_PERFORMANCE_STATIC
    for (i = 0; i < counter_count; i++) {
        if (counters[i].hw_run_type == 0)
            perf->hwcycle_acc += counters[i].used_hw_cycles;
        if (counters[i].hw_run_type == 1)
            perf->hwcycle_accp1 += counters[i].used_hw_cycles;
        perf->hwcycle_acc_total += counters[i].used_hw_cycles;
    }
#endif

    for (i = 0; i < counter_count; i++) {
        perf_last_frame += counters[i].used_hw_cycles;
        dram_reads_last_frame += counters[i].dram_reads;
        dram_writes_last_frame += counters[i].dram_writes;
    }

    cfg->perf_cnt_sum += perf_last_frame / ctx->mbs;
    cfg->dram_rd_sum += dram_reads_last_frame;
    cfg->dram_wr_sum += dram_writes_last_frame;

    if (perf_last_frame / ctx->mbs < cfg->perf_cnt_min)
        cfg->perf_cnt_min = perf_last_frame / ctx->mbs;
    else if (perf_last_frame / ctx->mbs > cfg->perf_cnt_max)
        cfg->perf_cnt_max = perf_last_frame / ctx->mbs;
    if (dram_reads_last_frame > cfg->dram_rd_max)
        cfg->dram_rd_max = dram_reads_last_frame;
    if (dram_writes_last_frame > cfg->dram_wr_max)
        cfg->dram_wr_max = dram_writes_last_frame;
}

void vp9enc_print_total(VpiEncVp9Ctx *ctx)
{
    VpiEncVp9Setting *ecfg = &ctx->vp9_enc_cfg;
    VP9ENCPERF *perf       = &ctx->perf;

    ctx->time_used_hw_total = (u64)STATOTAL(tb, perf, vp9hw_total);
    if (ctx->frm_cnt_hw_total > 0) {
        VPILOGD("Enc[%d] HW performance: Avg %-6u Min %-6u Max %-6u [clock cycles/MB]|\t \
                HW Time per frame:Avg %f [us]|\n",
                ctx->enc_index,
                ctx->frm_cnt_hw_total ?
                    (u32)(ecfg->perf_cnt_sum / ctx->frm_cnt_hw_total) :
                    0,
                ecfg->perf_cnt_min, ecfg->perf_cnt_max,
                ctx->time_used_hw_total / ctx->frm_cnt_hw_total);
      VPILOGD("HW DRAM BW Read:   Avg %-6u Max %-6u [bytes/MB]\n",
             ctx->frm_cnt_hw_total ?
             (u32)((ecfg->dram_rd_sum*256/8)/ctx->mbs/ctx->frm_cnt_hw_total): 0,
             ctx->frm_cnt_hw_total ?
             (u32)((ecfg->dram_rd_max*256/8)/ctx->mbs) : 0);
      VPILOGD("HW DRAM BW Write: Avg %-6u Max %-6u [bytes/MB]\n",
             ctx->frm_cnt_hw_total ?
             (u32)((ecfg->dram_wr_sum*256/8)/ctx->mbs/ctx->frm_cnt_hw_total): 0,
             ctx->frm_cnt_hw_total ?
             (u32)((ecfg->dram_wr_max*256/8)/ctx->mbs) : 0);
        VPILOGD("HW average ssim value : %f, ssim_sum=%f, \
            frame number= %d,frm_cnt_hw_total=%d \n",
                ctx->ssim_sum / ctx->frame_count_out, ctx->ssim_sum,
                ctx->frame_count_out, ctx->frm_cnt_hw_total);
        VPILOGD("equivalent HW performance per display frame:    Avg %-6u \n",
                (u32)(ecfg->perf_cnt_sum / (ctx->frm_cnt_hw_total + 1)));
        VPILOGD("equivalent HW DRAM BW Read per display frame:  Avg %-6u \n",
                (u32)((ecfg->dram_rd_sum * 256 / 8) / ctx->mbs /
                      (ctx->frm_cnt_hw_total + 1)));
        VPILOGD("equivalent HW DRAM BW Write per display frame: Avg %-6u \n",
                (u32)((ecfg->dram_wr_sum * 256 / 8) / ctx->mbs /
                      (ctx->frm_cnt_hw_total + 1)));
    } else {
        VPILOGE("ctx->frm_cnt_hw_total:[%d] invalid values\n",
                ctx->frm_cnt_hw_total);
    }
}

void vp9enc_print_frame(VpiEncVp9Ctx *ctx, VP9EncInst encoder, u32 frame_number,
                        VP9EncRet ret, int width, int height, int show_frame)
{
    VpiEncVp9Setting *ecfg = &ctx->vp9_enc_cfg;
    u32 i, partSum = 0;
    char msg[2048];
    HwPerfCounter counters[128];
    u32 counter_count = sizeof(counters) / sizeof(counters[0]);
    VP9EncGetHwPerfCounter(encoder, counters, &counter_count);
    u64 total_performance    = 0;
    char result_buffer[1024] = "";
    char run_type;
    char run_count;
    u32 e_count = 1, f_count = 1, t_count = 1, l_count = 1;

    if ((ctx->frame_count_out + 1) && ecfg->output_rate_denom) {
        /* Using 64-bits to avoid overflow */
        u64 tmp = ctx->stream_size / (ctx->frame_count_out + 1);
        tmp *= (u32)ecfg->output_rate_number;

        ctx->bitrate = (u32)(8 * (tmp / (u32)ecfg->output_rate_denom));
    }

    snprintf(&msg[0], 2048, "PIC%3i %3i Qp%3d ", (int)frame_number,
             (int)ctx->frame_count_out, ctx->rc.currentActiveQp);

    snprintf(&msg[strlen(&msg[0])], 2048-strlen(&msg[0]), "%s",
            (ret == VP9ENC_LAG_FIRST_PASS) ? " fp " :
            (ret == VP9ENC_LAG) ? "lag " :
            (ret == VP9ENC_OUTPUT_BUFFER_OVERFLOW) ? "lost" :
            (ret != VP9ENC_FRAME_READY) ? "erro" :
            (ctx->enc_out.codingType == VP9ENC_INTRA_FRAME) ? " I  " :
            (ctx->enc_out.codingType == VP9ENC_PREDICTED_FRAME) ?
            (show_frame == 0 ?" PN " : " P  ") : "skip");

    /* Print bitrate statistics and frame size */
    if (ret != VP9ENC_LAG_FIRST_PASS && ret != VP9ENC_LAG) {
        snprintf(&msg[strlen(&msg[0])], 2048 - strlen(&msg[0]),
                 " |BR%9u |SS%7lu FS%7u | ", ctx->bitrate, ctx->stream_size,
                 ctx->enc_out.frameSize);
    } else {
        snprintf(&msg[strlen(&msg[0])], 2048 - strlen(&msg[0]),
                 " | %9u %9u | %7i %7i | ", 0, 0, 0, 0);
    }

    if (ret != VP9ENC_LAG_FIRST_PASS && ret != VP9ENC_LAG) {
        /* This works only with system model, bases are pointers. */
        if (ctx->enc_out.codingType != VP9ENC_NOTCODED_FRAME &&
            ecfg->recondump &&
            (ctx->enc_out.show_frame || ecfg->show_hidden_frames)) {
        }
    }

    /* Print size of each partition in bytes */
    if (0) {
        if (ecfg->passes == 2 && ecfg->pass == 1) {
            snprintf(&msg[strlen(&msg[0])], 2048 - strlen(&msg[0]),
                     "%2d %3d %7d", 0, 0, ctx->enc_out.frameSize);
        } else {
            snprintf(&msg[strlen(&msg[0])], 2048 - strlen(&msg[0]),
                     "%2d %3d %7d", ctx->enc_out.frameSize ? IVF_FRM_BYTES : 0,
                     ctx->enc_out.streamSize[0], ctx->enc_out.streamSize[1]);
        }
        snprintf(&msg[strlen(&msg[0])], 2048 - strlen(&msg[0]), " |");
    }

    if (ecfg->print_ssim && (!ecfg->frameComp || ecfg->recondump)) {
        float psnr = 0;
        if (ctx->enc_out.codingType != VP9ENC_NOTCODED_FRAME &&
            !ecfg->rotation && !ecfg->flip && show_frame) {
            /*psnr = PrintPSNR(encoder, ecfg, width, height);*/
        } else {
            snprintf(&msg[strlen(&msg[0])], 2048 - strlen(&msg[0]),
                     " %5.2f %5.2f %5.2f | %5.2f %5.2f |", 0.0, 0.0, 0.0, 0.0,
                     0.0);
        }
        if (psnr) {
            ctx->psnrSum += psnr;
            ctx->psnr_count++;
        }
    }

    for (i = 0; i < counter_count; i++) {
        HwPerfCounter hwPerfCounter = counters[i];
        switch (hwPerfCounter.hw_run_type) {
        default:

        case 0:
            run_type  = (ctx->enc_out.show_frame == 0) ? 'N' : 'E';
            run_count = e_count++;
            break;

        case 1:
            run_type  = 'F';
            run_count = f_count++;
            break;

        case 2:
            run_type  = 'T';
            run_count = t_count++;
            break;

        case 3:
            run_type  = 'L';
            run_count = l_count++;
            break;
        }
        sprintf(result_buffer, "%s %c%d:cycles%5d DR%6d DW%6d|", result_buffer,
                run_type, run_count, hwPerfCounter.used_hw_cycles / ctx->mbs,
                hwPerfCounter.dram_reads * 256 / 8 / ctx->mbs,
                hwPerfCounter.dram_writes * 256 / 8 / ctx->mbs);
        total_performance += hwPerfCounter.used_hw_cycles;
    }

    if (ecfg->print_ssim && (ecfg->recondump == 0)) {
        float ssim_frame = (ctx->enc_out.ssim_lu * 8 + ctx->enc_out.ssim_cb +
                            ctx->enc_out.ssim_cr) /
                           10;
        snprintf(&msg[strlen(&msg[0])], 2048 - strlen(&msg[0]),
                 " SSIM value: lu%.4f cb%.4f cr%.4f  frame ssim: %.4f|",
                 ctx->enc_out.ssim_lu, ctx->enc_out.ssim_cb,
                 ctx->enc_out.ssim_cr, ssim_frame);
        if (ctx->enc_out.show_frame == 1) {
            ctx->ssim_sum += ssim_frame;
        }
    }
    snprintf(&msg[strlen(&msg[0])], 2048 - strlen(&msg[0]), "%5d cycles / mb| ",
             (u32)total_performance / ctx->mbs);
    snprintf(&msg[strlen(&msg[0])], 2048 - strlen(&msg[0]), "\n");
    TBPI("%s", &msg[0]);
    fflush(stdout);
    if (ctx->enc_out.frameSize) {
        /* Check that partition sizes match frame size */
        for (i = 0; i < 9; i++)
            partSum += ctx->enc_out.streamSize[i];
        if (ctx->enc_out.frameSize != partSum)
            VPILOGE("ERROR: Frame size doesn't match partition sizes!\n");
    }
}

static int vp9_pre_encode(VpiEncVp9Ctx *ctx)
{
    VpiEncVp9Setting *ecfg = &ctx->vp9_enc_cfg;
    VP9EncInst encoder     = ctx->encoder;
    VP9EncIn *enc_instance = &ctx->enc_in;
    int src_img_size;

    /* Set the window length for bitrate moving average calculation */
    ctx->ma.pos = ctx->ma.count = 0;
    ctx->ma.frame_rate_number   = ecfg->output_rate_number;
    ctx->ma.frame_rate_denom    = ecfg->output_rate_denom;

    if (ecfg->output_rate_denom)
        ctx->ma.length =
            MAX(1, MIN(ecfg->output_rate_number / ecfg->output_rate_denom,
                       MOVING_AVERAGE_FRAMES));
    else
        ctx->ma.length = MOVING_AVERAGE_FRAMES;

    enc_instance->busOutBuf  = ctx->outbuff_mem.busAddress;
    enc_instance->outBufSize = ctx->outbuff_mem.size;

    /* Source Image Size */
    u32 lu_w = ecfg->lum_width_src;
    u32 lu_h = ecfg->lum_height_src;
    u32 ch_w = (ecfg->lum_width_src + 1) / 2;
    u32 ch_h = (ecfg->lum_height_src + 1) / 2;

    if (!ecfg->no_input_conversion) {
        /* source file is planar YUV 8b */
        src_img_size = lu_w * lu_h + 2 * ch_w * ch_h;
    } else {
        switch (ecfg->input_format) {
        default:
        case ENC_YUV420_PLANAR:
        case ENC_YUV420_PLANAR_4x4:
        case ENC_YUV420_SEMIPLANAR:
        case ENC_YUV420_SEMIPLANAR_VU:
        case ENC_YUV420_SEMIPLANAR_TILED4x4:
        case ENC_YUV420_SEMIPLANAR_TILED8x4:
        case ENC_YUV420_SEMIPLANAR_P010:
        case ENC_YUV420_SEMIPLANAR_P010_TILED4x4:
            src_img_size = lu_w * lu_h + 2 * ch_w * ch_h;
            break;

        case ENC_YUV422_INTERLEAVED_YUYV:
        case ENC_YUV422_INTERLEAVED_UYVY:
            /* Odd widths are rounded to even with interleaved format. */
            ch_h = ecfg->lum_height_src;
            lu_w += lu_w % 2;
            src_img_size = lu_w * lu_h + 2 * ch_w * ch_h;
            break;

        case ENC_RGBA:
        case ENC_BGRA:
            src_img_size = lu_w * lu_h * 4;
            break;
        }

        if (ecfg->input_bitdepth == 1)
            src_img_size = 5 * src_img_size / 4;
        else if (ecfg->input_bitdepth == 2 && ecfg->input_format != ENC_RGBA &&
                 ecfg->input_format != ENC_BGRA)
            src_img_size *= 2;
    }

    ctx->src_img_size = src_img_size;

    if (!ecfg->pp_only) {
        VP9EncGetRateCtrl(encoder, &ctx->rc);
    }
    /* First frame always has zero time increment */
    enc_instance->timeIncrement   = 0;
    enc_instance->end_of_sequence = 0;

    /* TODO: testing stats output from regular encoding pass */
    enc_instance->write_stats = 1;
    ctx->loop_condition       = true;
    ctx->loop_break           = false;
    ctx->input_real_count     = 0;

    return 0;
}

static int vp9_allocate_resource(VpiEncVp9Ctx *ctx)
{
    VpiEncVp9Setting *ecfg = &ctx->vp9_enc_cfg;
    VP9EncInst encoder     = ctx->encoder;
    u32 pictureSize;
    u32 outbufSize;
    int ret;
    /* 64 multiple horizontal stride will fit all */
    const u32 w = (ecfg->lum_width_src + 4095) & (~4095);
    const u32 h = (ecfg->lum_height_src + 7) & (~7);

    if (ecfg->input_compress == 1) {
        /* data from DEC compress */
        u32 hh      = NEXT_MULTIPLE((ecfg->lum_height_src + 7) & (~7), 64);
        pictureSize = (w * hh *
                       VP9EncGetBitsPerPixel(ecfg->input_format,
                                             (ecfg->input_bitdepth != 0))) /
                      8;
        pictureSize += 16 * 1024 * 4;

    } else if (ecfg->input_compress == 2) {
        /* data from DTRC compress */
        u32 bit = 0;
        if (ecfg->input_format == ENC_YUV420_SEMIPLANAR_P010) {
            bit = 10;
        } else if (ecfg->input_format == ENC_YUV420_SEMIPLANAR) {
            bit = 8;
        }
        pictureSize = (w * h * bit) / 8 * 3 / 2;
        /*
       * the max table size  is 150kbytes, so i give 160kb for luma,
       * and 160kb for chroma.
       */
        pictureSize += 160 * 1024 * 2;
    } else {
        pictureSize = (w * h *
                       VP9EncGetBitsPerPixel(ecfg->input_format,
                                             (ecfg->input_bitdepth != 0))) /
                      8;
    }
    pictureSize += 32 * 3;
    ctx->picture_mem.virtualAddress    = NULL;
    ctx->outbuff_mem.virtualAddress    = NULL;
    ctx->ref_desc_mem                  = NULL;
    ctx->scaled_pic_mem.virtualAddress = NULL;

    /* Limited amount of memory on some test environment */
    outbufSize = ecfg->width * ecfg->height * 3 / 2 * 2;
    if (ecfg->lossless)
        outbufSize += outbufSize / 2;

    /* round to 4k to avoid triggering model dram checker */
    outbufSize &= (~4095);

    void *cwl = VP9EncGetCWL(encoder);
    ret = CWLMallocEpLinear(cwl, outbufSize, &ctx->outbuff_mem);
    if (ret != CWL_OK) {
        VPILOGE("Failed to allocate output buffer!\n");
        return 1;
    }

    pictureSize =
        ((ecfg->width + 63) & (~63)) * ((ecfg->height + 63) & (~63)) * 3 / 2;
    if (ecfg->frameComp) {
        ctx->ref_desc_mem = (u16 *)malloc(pictureSize * sizeof(u16));
        if (ctx->ref_desc_mem == NULL) {
            VPILOGE("Failed to allocate reference pic_info dump buffer!\n");
            return 1;
        }
    }

    if (ecfg->pp_dump) {
        pictureSize =
            ((ecfg->width + 63) & (~63)) * ((ecfg->height + 7) & (~7)) * 3 / 2;
        /* be ready for packed 10bpp 8x4 tiles */
        pictureSize = (pictureSize / 32) * 40;
        VPILOGD("Allocate scaled_pic_mem size = %d\n", pictureSize);
        ret =
            allocate_hw_linear_mem(encoder, pictureSize, &ctx->scaled_pic_mem);
        if (ret != CWL_OK) {
            VPILOGE("Failed to allocate scaled picture output buffer!\n");
            return 1;
        }
    }

    if (!outbufSize || !ctx->outbuff_mem.size) {
        VPILOGE("Invalid encoder input buffer %d bytes, "
                "outbuffer size %d bytes\n",
                outbufSize, ctx->outbuff_mem.size);
        return 1;
    }
    VPILOGD("Input buffer size: %d bytes\n", pictureSize);
    VPILOGD("Output buffer size: %d bytes\n", ctx->outbuff_mem.size);

    return 0;
}

void vp9enc_free_resource(VpiEncVp9Ctx *ctx)
{
    VpiEncVp9Setting *ecfg = &ctx->vp9_enc_cfg;
    VP9EncInst enc         = ctx->encoder;

    VPILOGD("start free resource!!\n");
    void *cwl = VP9EncGetCWL(enc);
    CWLFreeEpLinear(cwl, &ctx->outbuff_mem);

    if (ecfg->pp_dump) {
        free_hw_pic_mem(enc, &ctx->scaled_pic_mem);
    }

    if (ctx->ref_desc_mem) {
        free(ctx->ref_desc_mem);
        ctx->ref_desc_mem = NULL;
    }
    if (ctx->rc_twopass_stats_in) {
        free(ctx->rc_twopass_stats_in);
        ctx->rc_twopass_stats_in = NULL;
    }

    if (ctx->seg_buf) {
        free(ctx->seg_buf);
        ctx->seg_buf = 0;
    }
}

static int vp9_set_rate_control(VpiEncVp9Ctx *ctx, VpiEncVp9Setting *ecfg)
{
    int ret            = 0;
    VP9EncInst encoder = ctx->encoder;
    VP9EncRateCtrl rc_config;
    u32 f, l;

    if (encoder == NULL || ecfg == NULL) {
        VPILOGE("vp9_set_rate_control input error\n");
        return -1;
    }

    if ((ret = VP9EncGetRateCtrl(encoder, &rc_config)) != VP9ENC_OK) {
        VPILOGE("VP9EncGetRateCtrl() failed.", ret);
        return -1;
    }

    VPILOGD("Get rate control: qp=%d [%d..%d] %dbps,"
            " picRc=%d gop=%d\n",
            rc_config.qpHdr, rc_config.qpMin, rc_config.qpMax,
            rc_config.bitPerSecond, rc_config.pictureRc,
            rc_config.bitrateWindow);

    if (ecfg->pic_rc != DEFAULT)
        rc_config.pictureRc = ecfg->pic_rc;
    if (ecfg->pic_skip != DEFAULT)
        rc_config.pictureSkip = ecfg->pic_skip;
    if (ecfg->qp_hdr != DEFAULT)
        rc_config.qpHdr = ecfg->qp_hdr;
    else if (rc_config.pictureRc)
        rc_config.qpHdr = -1;
    if (ecfg->qp_min != DEFAULT)
        rc_config.qpMin = ecfg->qp_min;
    if (ecfg->qp_max != DEFAULT)
        rc_config.qpMax = ecfg->qp_max;
    if (ecfg->bit_per_second != DEFAULT)
        rc_config.bitPerSecond = ecfg->bit_per_second;
    if (ecfg->layer_bit_per_second[0])
        rc_config.layerBitPerSecond[0] = ecfg->layer_bit_per_second[0];
    if (ecfg->layer_bit_per_second[1])
        rc_config.layerBitPerSecond[1] = ecfg->layer_bit_per_second[1];
    if (ecfg->layer_bit_per_second[2])
        rc_config.layerBitPerSecond[2] = ecfg->layer_bit_per_second[2];
    if (ecfg->layer_bit_per_second[3])
        rc_config.layerBitPerSecond[3] = ecfg->layer_bit_per_second[3];
    if (ecfg->bitrate_window != DEFAULT)
        rc_config.bitrateWindow = ecfg->bitrate_window;
    if (ecfg->intra_qp_delta != DEFAULT)
        rc_config.intraQpDelta = ecfg->intra_qp_delta;
    if (ecfg->fixed_intra_qp != DEFAULT)
        rc_config.fixedIntraQp = ecfg->fixed_intra_qp;
    if (ecfg->intra_pic_rate != 0)
        rc_config.intraPictureRate = ecfg->intra_pic_rate;
    if (ecfg->golden_picture_rate != DEFAULT)
        rc_config.goldenPictureRate = ecfg->golden_picture_rate;
    if (ecfg->altref_picture_rate != DEFAULT)
        rc_config.altrefPictureRate = ecfg->altref_picture_rate;
    if (ecfg->golden_picture_boost != DEFAULT)
        rc_config.goldenPictureBoost = ecfg->golden_picture_boost;
    if (ecfg->adaptive_golden_boost != DEFAULT)
        rc_config.adaptiveGoldenBoost = ecfg->adaptive_golden_boost;
    if (ecfg->adaptive_golden_update != DEFAULT)
        rc_config.adaptiveGoldenUpdate = ecfg->adaptive_golden_update;

    if (rc_config.layerBitPerSecond[0]) {
        /* Enables tmp layers, disables golden&altref boost&update */
        ecfg->tmp_layers                    = 1;
        rc_config.adaptiveGoldenBoost       = rc_config.adaptiveGoldenUpdate =
            rc_config.goldenPictureBoost    = rc_config.goldenPictureRate =
                rc_config.altrefPictureRate = 0;
        if (rc_config.layerBitPerSecond[1])
            ecfg->tmp_layers++;
        if (rc_config.layerBitPerSecond[2])
            ecfg->tmp_layers++;
        if (rc_config.layerBitPerSecond[3])
            ecfg->tmp_layers++;

        /* Set framerates for individual layers, fixed pattern */
        f                                = ecfg->output_rate_denom;
        l                                = ecfg->tmp_layers - 1;
        rc_config.layerFrameRateDenom[0] = f * tempora_layer_tic_scale[l][0];
        rc_config.layerFrameRateDenom[1] = f * tempora_layer_tic_scale[l][1];
        rc_config.layerFrameRateDenom[2] = f * tempora_layer_tic_scale[l][2];
        rc_config.layerFrameRateDenom[3] = f * tempora_layer_tic_scale[l][3];

        VPILOGD("Layer rate control: base=%d bps, 1st=%d bps, "
                "2nd=%d bps, 3rd=%d bps\n",
                rc_config.layerBitPerSecond[0], rc_config.layerBitPerSecond[1],
                rc_config.layerBitPerSecond[2], rc_config.layerBitPerSecond[3]);
        VPILOGD("tics: base=%d, 1st=%d, 2nd=%d, 3rd=%d\n",
                rc_config.layerFrameRateDenom[0],
                rc_config.layerFrameRateDenom[1],
                rc_config.layerFrameRateDenom[2],
                rc_config.layerFrameRateDenom[3]);
    }

    VPILOGD("Set rate control: qp=%3d [%2d..%3d] %8d bps,"
            "picRc=%d gop=%d, goldenBoost=%d,"
            "goldenUpdate=%d\n",
            rc_config.qpHdr, rc_config.qpMin, rc_config.qpMax,
            rc_config.bitPerSecond, rc_config.pictureRc,
            rc_config.bitrateWindow, rc_config.adaptiveGoldenBoost,
            rc_config.adaptiveGoldenUpdate);

    if ((ret = VP9EncSetRateCtrl(encoder, &rc_config)) != VP9ENC_OK) {
        VPILOGE("VP9EncSetRateCtrl() failed.", ret);
    }
    return ret;
}

static int vp9_set_coding_control(VpiEncVp9Ctx *ctx, VpiEncVp9Setting *ecfg)
{
    VP9EncCodingCtrl coding_cfg;
    VP9EncInst encoder = ctx->encoder;
    int ret            = 0;

    if (encoder == NULL || ecfg == NULL) {
        VPILOGE("vp9_set_coding_control input error\n");
        return -1;
    }

    /* Encoder setup: coding control */
    if ((ret = VP9EncGetCodingCtrl(encoder, &coding_cfg)) != VP9ENC_OK) {
        VPILOGE("VP9EncGetCodingCtrl() failed.", ret);
        return -1;
    }

    if (ecfg->error_resilient != DEFAULT)
        coding_cfg.errorResilient = ecfg->error_resilient;
    coding_cfg.frameParallelDecodingMode = ecfg->frm_para_decmode;
    if (ecfg->mcomp_filter_type != DEFAULT)
        coding_cfg.interpolationFilter = ecfg->mcomp_filter_type;
    if (ecfg->filter_level != DEFAULT)
        coding_cfg.filterLevel = ecfg->filter_level;
    if (ecfg->filter_sharpness != DEFAULT)
        coding_cfg.filterSharpness = ecfg->filter_sharpness;

    if (ecfg->recode) {
        if (ecfg->effort != DEFAULT)
            ecfg->effort += (MAX_EFFORT_LEVEL + 1);
        else {
            ecfg->effort = coding_cfg.effort + (MAX_EFFORT_LEVEL + 1);
        }
    }

    if (ecfg->effort != DEFAULT)
        coding_cfg.effort = ecfg->effort;
    if (ecfg->quality_metric != DEFAULT)
        coding_cfg.qualityMetric = (VP9EncQualityMetric)(ecfg->quality_metric);
    if (ecfg->ref_frame_scheme != DEFAULT)
        coding_cfg.refFrameScheme = ecfg->ref_frame_scheme;
    if (ecfg->min_arf_period != DEFAULT)
        coding_cfg.minArfPeriod = ecfg->min_arf_period;
    if (ecfg->max_arf_period != DEFAULT)
        coding_cfg.maxArfPeriod = ecfg->max_arf_period;
    if (ecfg->arf_tmp_filter_length != DEFAULT)
        coding_cfg.arfTemporalFilterLength = ecfg->arf_tmp_filter_length;
    if (ecfg->arf_tmp_filter_strength != DEFAULT)
        coding_cfg.arfTemporalFilterStrength = ecfg->arf_tmp_filter_strength;
    if (ecfg->arf_tmp_filter_threshold != DEFAULT)
        coding_cfg.arfTemporalFilterThreshold = ecfg->arf_tmp_filter_threshold;
    if (ecfg->cq_level != DEFAULT)
        coding_cfg.cqLevel = ecfg->cq_level;

    coding_cfg.qpDelta[0] = ecfg->qp_delta[0];
    coding_cfg.qpDelta[1] = ecfg->qp_delta[1];
    coding_cfg.qpDelta[2] = ecfg->qp_delta[2];

    if (ecfg->lambda[0] != DEFAULT) {
        coding_cfg.rdoLambda[0] = ecfg->lambda[0];
        coding_cfg.rdoLambda[1] = ecfg->lambda[1];
        coding_cfg.rdoLambda[2] = ecfg->lambda[2];
        coding_cfg.rdoLambda[3] = ecfg->lambda[3];
    }
    if (ecfg->split_penalty[0] != DEFAULT) {
        coding_cfg.splitPenalty[0] = ecfg->split_penalty[0];
        coding_cfg.splitPenalty[1] = ecfg->split_penalty[1];
        coding_cfg.splitPenalty[2] = ecfg->split_penalty[2];
        coding_cfg.splitPenalty[3] = ecfg->split_penalty[3];
    }
    if (ecfg->wme_lambda_motion != DEFAULT)
        coding_cfg.wmeLambdaMotion = ecfg->wme_lambda_motion;
    if (ecfg->wme_lambda_zero != DEFAULT)
        coding_cfg.wmeLambdaZero = ecfg->wme_lambda_zero;
    if (ecfg->bandwidth_mode != DEFAULT)
        coding_cfg.bandwidthMode = (VP9EncBandwidthMode)(ecfg->bandwidth_mode);
    if (ecfg->high_prec_mv_enable != DEFAULT)
        coding_cfg.highPrecMvEnable = ecfg->high_prec_mv_enable;
    if (ecfg->me_chroma_weight != DEFAULT)
        coding_cfg.meChromaWeight = ecfg->me_chroma_weight;

    VPILOGD("Set coding control: mcompFilterType=%d errorResilient=%d"
            " frameParallel=%d filterLevel=%d\nfilterSharpness=%d "
            " qpDeltas=%d,%d,%d, tune=%s refFrameScheme=%d effort=%d"
            " minArfPeriod=%d maxArfPeriod=%d\n",
            coding_cfg.interpolationFilter, coding_cfg.errorResilient,
            coding_cfg.frameParallelDecodingMode, coding_cfg.filterLevel,
            coding_cfg.filterSharpness, coding_cfg.qpDelta[0],
            coding_cfg.qpDelta[1], coding_cfg.qpDelta[2],
            coding_cfg.qualityMetric == VP9ENC_QM_SSIM ? "ssim" : "psnr",
            coding_cfg.refFrameScheme, coding_cfg.effort,
            coding_cfg.minArfPeriod, coding_cfg.maxArfPeriod);

    TBPI("Set coding control: mcompFilterType=%d errorResilient=%d"
         " frameParallel=%d filterLevel=%d\nfilterSharpness=%d"
         " qpDeltas=%d,%d,%d, tune=%s refFrameScheme=%d effort=%d"
         " minArfPeriod=%d maxArfPeriod=%d\n",
         coding_cfg.interpolationFilter, coding_cfg.errorResilient,
         coding_cfg.frameParallelDecodingMode, coding_cfg.filterLevel,
         coding_cfg.filterSharpness, coding_cfg.qpDelta[0],
         coding_cfg.qpDelta[1], coding_cfg.qpDelta[2],
         coding_cfg.qualityMetric == VP9ENC_QM_SSIM ? "ssim" : "psnr",
         coding_cfg.refFrameScheme, coding_cfg.effort, coding_cfg.minArfPeriod,
         coding_cfg.maxArfPeriod);

    VPILOGD("ARF filter params: strength=%d length=%d\n",
            coding_cfg.arfTemporalFilterStrength,
            coding_cfg.arfTemporalFilterLength);

    coding_cfg.lossless = ecfg->lossless ? 1 : 0;

    if (coding_cfg.lossless)
        VPILOGD("Lossless encoding!\n\n");
    if (coding_cfg.interpolationFilter == 3)
        coding_cfg.interpolationFilter = 4; //to workaround vp9 original bug.

    ret = VP9EncSetCodingCtrl(encoder, &coding_cfg);
    if (ret != VP9ENC_OK) {
        VPILOGE("VP9EncSetCodingCtrl() failed, error no=%d\n", ret);
    }
    return ret;
}

static int vp9_set_pre_processing(VpiEncVp9Ctx *ctx, VpiEncVp9Setting *ecfg,
                                  EncPreProcessingCfg *proccfg)
{
    int ret            = 0;
    VP9EncInst encoder = ctx->encoder;

    if (encoder == NULL || ecfg == NULL) {
        VPILOGE("vp9_set_pre_processing input error\n");
        return -1;
    }

    if ((ret = VP9EncGetPreProcessing(encoder, proccfg)) != VP9ENC_OK) {
        VPILOGE("VP9EncGetPreProcessing() failed.", ret);
        return -1;
    }

    VPILOGD("Get PreP: input:%4ux%u "
            "crop::%4ux%u from offset:%4ux%u "
            "scale:%4ux%u -> %4ux%u "
            "format:%i rgb_cc:%i range_conv:%i rotation:%i flip:%i\n",
            proccfg->origWidth, proccfg->origHeight, proccfg->cropWidth,
            proccfg->cropHeight, proccfg->xOffset, proccfg->yOffset,
            proccfg->cropWidth, proccfg->cropHeight, ecfg->width, ecfg->height,
            proccfg->inputType, proccfg->colorConversion,
            proccfg->colorRangeConversion, proccfg->rotation,
            proccfg->flipType);

    EncPictureType type = (EncPictureType)ecfg->input_format;

    /* convert 10b and 16b to encoder internal types */
    if (ecfg->input_bitdepth == 1) {
        type = ENC_YUV420_PLANAR_4x4_10b;
    } else if (ecfg->input_bitdepth == 2) {
        if (ecfg->input_format == ENC_YUV420_PLANAR) {
            type = ENC_YUV420_PLANAR_10BE;
        } else if (ecfg->input_format == ENC_YUV420_PLANAR_4x4) {
            type = ENC_YUV420_PLANAR_4x4_16b;
        } else if (ecfg->input_format == ENC_RGBA) {
            type = ENC_RGB10;
        } else if (ecfg->input_format == ENC_BGRA) {
            type = ENC_BGR10;
        }
    } else if (ecfg->input_format == ENC_YUV420_PLANAR_4x4) {
        type = ENC_YUV420_PLANAR_4x4_8b;
    }
    proccfg->pp_only              = ecfg->pp_only;
    proccfg->inputType            = type;
    proccfg->colorConversion      = ecfg->color_conv;
    proccfg->colorRangeConversion = ecfg->color_range_conv;
    proccfg->rotation             = ecfg->rotation;
    proccfg->flipType             = ecfg->flip;
    proccfg->letterboxLeft        = ecfg->letterbox_left;
    proccfg->letterboxRight       = ecfg->letterbox_right;
    proccfg->letterboxTop         = ecfg->letterbox_top;
    proccfg->letterboxBottom      = ecfg->letterbox_bottom;
    proccfg->letterboxYColor      = 64;
    proccfg->letterboxUColor      = 512;
    proccfg->letterboxVColor      = 512;

    if (ecfg->letterbox_ycolor != DEFAULT) {
        proccfg->letterboxYColor = ecfg->letterbox_ycolor;
    }
    if (ecfg->letterbox_ucolor != DEFAULT) {
        proccfg->letterboxUColor = ecfg->letterbox_ucolor;
    }
    if (ecfg->letterbox_vcolor != DEFAULT) {
        proccfg->letterboxVColor = ecfg->letterbox_vcolor;
    }

    switch (proccfg->inputType) {
    case ENC_YUV420_PLANAR:
        proccfg->origWidth = (ecfg->lum_width_src + 2047) & (~2047);
        break;
    case ENC_YUV420_PLANAR_10BE:
        proccfg->origWidth = (ecfg->lum_width_src + 1023) & (~1023);
        break;
    case ENC_YUV420_SEMIPLANAR:
        proccfg->origWidth = (ecfg->lum_width_src + 1023) & (~1023);
        break;
    case ENC_YUV420_SEMIPLANAR_VU:
        proccfg->origWidth = (ecfg->lum_width_src + 31) & (~31);
        break;
    case ENC_YUV422_INTERLEAVED_YUYV:
    case ENC_YUV422_INTERLEAVED_UYVY:
    case ENC_YUV420_SEMIPLANAR_TILED8x4:
        proccfg->origWidth = (ecfg->lum_width_src + 15) & (~15);
        break;
    case ENC_YUV420_PLANAR_4x4_10b:
        proccfg->origWidth = (ecfg->lum_width_src + 63) & (~63);
        break;
    case ENC_YUV420_SEMIPLANAR_P010_TILED4x4:
        proccfg->origWidth = (ecfg->lum_width_src + 127) & (~127);
        break;
    case ENC_YUV420_SEMIPLANAR_P010:
        proccfg->origWidth = (ecfg->lum_width_src + 511) & (~511);
        break;
    case ENC_YUV420_SEMIPLANAR_TILED4x4:
        proccfg->origWidth = (ecfg->lum_width_src + 255) & (~255);
        break;
    case ENC_YUV420_PLANAR_4x4_8b:
    case ENC_YUV420_PLANAR_4x4_16b:
    default:
        proccfg->origWidth = (ecfg->lum_width_src + 7) & (~7);
    }

    ecfg->stride        = proccfg->origWidth;
    proccfg->origHeight = ecfg->lum_height_src;

    if (ecfg->scale_in_width != DEFAULT)
        proccfg->cropWidth = ecfg->scale_in_width;
    if (ecfg->scale_in_height != DEFAULT)
        proccfg->cropHeight = ecfg->scale_in_height;

    if (ecfg->hor_offset_src != DEFAULT)
        proccfg->xOffset = ecfg->hor_offset_src;
    if (ecfg->ver_offset_src != DEFAULT)
        proccfg->yOffset = ecfg->ver_offset_src;

    VPILOGD("Set PreP: input %4ux%u "
            "crop:%4ux%u from offset %4ux%u "
            "scale:%4ux%u -> %4ux%u "
            "format=%i rgb_cc=%i range_conv=%i rotation=%i flip=%i\n",
            proccfg->origWidth, proccfg->origHeight, proccfg->cropWidth,
            proccfg->cropHeight, proccfg->xOffset, proccfg->yOffset,
            proccfg->cropWidth, proccfg->cropHeight, ecfg->width, ecfg->height,
            proccfg->inputType, proccfg->colorConversion,
            proccfg->colorRangeConversion, proccfg->rotation,
            proccfg->flipType);
    TBPI("Set PreP: input %4ux%u "
         "crop:%4ux%u from offset %4ux%u "
         "scale:%4ux%u -> %4ux%u "
         "format=%i rgb_cc=%i range_conv=%i rotation=%i flip=%i\n",
         proccfg->origWidth, proccfg->origHeight, proccfg->cropWidth,
         proccfg->cropHeight, proccfg->xOffset, proccfg->yOffset,
         proccfg->cropWidth, proccfg->cropHeight, ecfg->width, ecfg->height,
         proccfg->inputType, proccfg->colorConversion,
         proccfg->colorRangeConversion, proccfg->rotation, proccfg->flipType);

    if ((ret = VP9EncSetPreProcessing(encoder, proccfg)) != VP9ENC_OK) {
        VPILOGE("VP9EncSetPreProcessing() failed.", ret);
    }
    return (int)ret;
}

static int vp9_set_first_pass(VpiEncVp9Ctx *ctx, VpiEncVp9Setting *ecfg,
                              EncPreProcessingCfg *proccfg)
{
    VP9EncFirstPassCfg fpCfg;
    VP9EncInst encoder = ctx->encoder;
    int ret            = 0;
    int min_scale      = (VP9ENC_MIN_ENC_WIDTH * 100) / ecfg->width;
    unsigned scale     = MIN(MAX(min_scale, (int)ecfg->firstpass_scale), 100);

    if (encoder == NULL || ecfg == NULL) {
        VPILOGE("vp9_set_first_pass input error\n");
        return -1;
    }

    if (ecfg->passes != 2) {
        VPILOGE("vp9_set_first_pass passs=%d\n", ecfg->passes);
        return 0;
    }

    memset(&fpCfg, 0, sizeof(fpCfg));
    if ((ecfg->width >= 3840 && ecfg->height >= 2160) ||
        (ecfg->width >= 2160 && ecfg->height >= 3840)) {
        ecfg->firstpass_scale = 50;
    }

    fpCfg.pass = ecfg->pass;
    /* workaround for the difference between c model and fpga */
    if (ecfg->firstpass_scale_method) {
        ecfg->firstpass_scale_method = 0;
    }

    /* calculate limit for down scaling based on the smallest supported enc size */
    min_scale = MAX(min_scale, (VP9ENC_MIN_ENC_HEIGHT * 100) / ecfg->height);

    fpCfg.origWidth       = proccfg->origWidth;
    fpCfg.origHeight      = proccfg->origHeight;
    fpCfg.width           = ((ecfg->width * scale) / 100);
    fpCfg.height          = ((ecfg->height * scale) / 100);
    fpCfg.xOffset         = proccfg->xOffset;
    fpCfg.yOffset         = proccfg->yOffset;
    fpCfg.cropWidth       = proccfg->cropWidth;
    fpCfg.cropHeight      = proccfg->cropHeight;
    fpCfg.letterboxLeft   = proccfg->letterboxLeft;
    fpCfg.letterboxRight  = proccfg->letterboxRight;
    fpCfg.letterboxTop    = proccfg->letterboxTop;
    fpCfg.letterboxBottom = proccfg->letterboxBottom;
    fpCfg.letterboxYColor = proccfg->letterboxYColor;
    fpCfg.letterboxUColor = proccfg->letterboxUColor;
    fpCfg.letterboxVColor = proccfg->letterboxVColor;
    fpCfg.pp_only         = proccfg->pp_only;

    if (scale != 100 && ecfg->firstpass_scale_method == 0) {
        /* original crop offset + first pass further crop */
        fpCfg.xOffset += ((ecfg->width - fpCfg.width) / 2);
        fpCfg.yOffset += ((ecfg->height - fpCfg.height) / 2);
        fpCfg.cropWidth  = fpCfg.width;
        fpCfg.cropHeight = fpCfg.height;

        if ((ecfg->width >= 3840 && ecfg->height >= 2160) ||
            (ecfg->width >= 2160 && ecfg->height >= 3840)) {
            u32 align = 1024;
            /* dec400 requirement */
            switch (proccfg->inputType) {
            case ENC_YUV420_SEMIPLANAR:
                fpCfg.xOffset = (fpCfg.xOffset + align - 1) & (~(align - 1));
                fpCfg.yOffset = (fpCfg.yOffset + (2 - 1)) & (~(2 - 1));
                break;
            case ENC_YUV420_SEMIPLANAR_P010_TILED4x4:
                fpCfg.xOffset =
                    (fpCfg.xOffset + align / 8 - 1) & (~(align / 8 - 1));
                fpCfg.yOffset = (fpCfg.yOffset + (4 - 1)) & (~(4 - 1));
                break;
            case ENC_YUV420_SEMIPLANAR_P010:
                fpCfg.xOffset =
                    (fpCfg.xOffset + align / 2 - 1) & (~(align / 2 - 1));
                fpCfg.yOffset = (fpCfg.yOffset + (2 - 1)) & (~(2 - 1));
                break;
            case ENC_YUV420_SEMIPLANAR_TILED4x4:
                fpCfg.xOffset =
                    (fpCfg.xOffset + align / 4 - 1) & (~(align / 4 - 1));
                fpCfg.yOffset = (fpCfg.yOffset + (4 - 1)) & (~(4 - 1));
                break;
            default:
                fpCfg.cropWidth = fpCfg.width;
            }
        }
    }

    if (fpCfg.pass == 2) {
        VPILOGD("First pass stats from file!\n");
        fpCfg.stats = (Vp9EncFirstPassStats *)ecfg->rc_twopass_stats_in;
        fpCfg.num_stats =
            ecfg->rc_twopass_stats_in_size / sizeof(Vp9EncFirstPassStats);
    } else {
        VPILOGD("First pass config: src %4ux%u\n"
                "crop  %4ux%u from offset %4ux%u\n"
                "scale %4ux%u -> %4ux%u\n",
                fpCfg.origWidth, fpCfg.origHeight, fpCfg.cropWidth,
                fpCfg.cropHeight, fpCfg.xOffset, fpCfg.yOffset, fpCfg.cropWidth,
                fpCfg.cropHeight, fpCfg.width, fpCfg.height);
        TBPI("First pass config: src %4ux%u,"
             "crop,%4ux%u from offset %4ux%u,"
             "scale,%4ux%u -> %4ux%u\n",
             fpCfg.origWidth, fpCfg.origHeight, fpCfg.cropWidth,
             fpCfg.cropHeight, fpCfg.xOffset, fpCfg.yOffset, fpCfg.cropWidth,
             fpCfg.cropHeight, fpCfg.width, fpCfg.height);
    }

    if ((ret = VP9EncSetFirstPass(encoder, &fpCfg)) != VP9ENC_OK) {
        vp9enc_print_error_value("VP9EncSetFirstPass() failed.", ret);
    }
    return (int)ret;
}

static int convert_encoder_config(VpiEncVp9Ctx *ctx, VP9EncConfig *cfg,
                                  VpiEncVp9Setting *ecfg)
{
    if (ctx == NULL || cfg == NULL || ecfg == NULL) {
        VPILOGE("allocate_hw_linear_mem input error\n");
        return -1;
    }
    /* input resolution == encoded resolution if not defined */
    if (ecfg->width == DEFAULT_VALUE) {
        ecfg->width = ecfg->lum_width_src;
    }
    if (ecfg->height == DEFAULT_VALUE) {
        ecfg->height = ecfg->lum_height_src;
    }
    cfg->width            = ecfg->width;
    cfg->height           = ecfg->height;
    cfg->bitdepth         = (VP9EncBitDepth)ecfg->bitdepth;
    cfg->enable_multicore = ecfg->multicore;
    cfg->calculate_ssim   = ecfg->print_ssim;
    cfg->frameComp        = ecfg->frameComp;
    ctx->mbs = ((cfg->width + 15) / 16) * ((cfg->height + 15) / 16);

    /* output frame rate == input frame rate if not defined */
    if (ecfg->output_rate_number == DEFAULT) {
        ecfg->output_rate_number = ecfg->input_rate_number;
    }
    if (ecfg->output_rate_denom == DEFAULT) {
        ecfg->output_rate_denom = ecfg->input_rate_denom;
    }
    cfg->frameRateDenom = ecfg->output_rate_denom;
    cfg->frameRateNum   = ecfg->output_rate_number;

    /* TODO: sw runs first pass after regular coding ->
     * need to set this to e.g. 12
     * to get stats for 10 frames ahead low latency 2pass
     */
    if (ecfg->passes > 1 && ecfg->lag_in_frames == 1) {
        ;
    } else if (ecfg->passes > 1 && ecfg->lag_in_frames < 6) {
        ecfg->lag_in_frames = 6;
    } else if (ecfg->ref_frame_scheme == 4 && ecfg->lag_in_frames < 2) {
        ecfg->lag_in_frames = 2;
    }

    cfg->lookaheadCount = ecfg->lag_in_frames;
    cfg->passes         = ecfg->passes != 1 ? 2 : 1;
    cfg->temporalFilter = cfg->lookaheadCount && ecfg->arf_tmp_filter_enabled;
    cfg->trace_ctx      = NULL;
    cfg->trace          = NULL;

    VPILOGD("Init config: size %dx%d %d/%d fps,"
            "lag %d, passes %d, arf tmp filter %d, bitdepth=%d\n",
            cfg->width, cfg->height, cfg->frameRateNum, cfg->frameRateDenom,
            cfg->lookaheadCount, cfg->passes, cfg->temporalFilter,
            cfg->bitdepth);

    if (cfg->frameComp) {
        VPILOGE("Reference frame compression enabled!\n\n");
    }

    cfg->device    = ecfg->device;
    cfg->priority  = ecfg->priority;
    cfg->mem_id    = ecfg->mem_id;
    cfg->cwl_index = ctx->enc_index;
    cfg->perf      = &ctx->perf;

    return 0;
}

/*
 * Below are all public functions
 */
int vp9enc_next_pic(int input_rate_number, int input_rate_denom,
                    int output_rate_number, int output_rate_denom,
                    int frame_count, int first_picture)
{
    u32 sift;
    u32 skip;
    u32 numer;
    u32 denom;
    u32 next;

    numer = (u32)input_rate_number * (u32)output_rate_denom;
    denom = (u32)input_rate_denom * (u32)output_rate_number;

    if (numer >= denom) {
        sift = 9;
        do {
            sift--;
        } while (((numer << sift) >> sift) != numer);
    } else {
        sift = 17;
        do {
            sift--;
        } while (((numer << sift) >> sift) != numer);
    }
    skip = (numer << sift) / denom;
    next = (((u32)frame_count * skip) >> sift) + (u32)first_picture;

    return (int)next;
}

void vp9enc_print_setting(VpiEncVp9Setting *ecfg)
{
    int i = 0;

    VPILOGD("\n\n--- Start VpiEncVp9Setting dump ---\n");
    VPILOGD("preset=%s\n", ecfg->preset);
    VPILOGD("effort=%d\n", ecfg->effort);
    VPILOGD("lag_in_frames=%d\n", ecfg->lag_in_frames);
    VPILOGD("passes=%d\n\n", ecfg->passes);

    VPILOGD("intra_pic_rate=%d\n", ecfg->intra_pic_rate);
    VPILOGD("bitrateWindow=%d\n", ecfg->bitrate_window);
    VPILOGD("qpHdr=%d\n", ecfg->qp_hdr);
    VPILOGD("qpMin=%d\n", ecfg->qp_min);
    VPILOGD("qpMax=%d\n", ecfg->qp_max);
    VPILOGD("fixedIntraQp=%d\n", ecfg->fixed_intra_qp);
    VPILOGD("picRc=%d\n", ecfg->pic_rc);
    VPILOGD("mcompFilterType=%d\n", ecfg->mcomp_filter_type);
    VPILOGD("refFrameScheme=%d\n", ecfg->ref_frame_scheme);
    VPILOGD("filterLevel=%d\n", ecfg->filter_level);
    VPILOGD("filterSharpness=%d\n\n", ecfg->filter_sharpness);

    VPILOGD("output_rate_number=%d\n", ecfg->output_rate_number);
    VPILOGD("output_rate_denom=%d\n", ecfg->output_rate_denom);
    VPILOGD("input_rate_number=%d\n", ecfg->input_rate_number);
    VPILOGD("input_rate_denom=%d\n", ecfg->input_rate_denom);
    VPILOGD("input_compress=%d\n", ecfg->input_compress);
    VPILOGD("inputFormat=%d\n", ecfg->input_format);
    VPILOGD("rotation=%d\n", ecfg->rotation);
    VPILOGD("flip=%d\n", ecfg->flip);
    VPILOGD("colorConversion=%d\n", ecfg->color_conv);
    VPILOGD("colorRangeConversion=%d\n", ecfg->color_range_conv);
    VPILOGD("bitPerSecond=%d\n", ecfg->bit_per_second);
    for (i = 0; i < 4; i++) {
        VPILOGD("layerBitPerSecond[%d]=%d\n", i, ecfg->layer_bit_per_second[i]);
    }
    VPILOGD("tmp_layers=%d\n", ecfg->tmp_layers);
    VPILOGD("picSkip=%d\n", ecfg->pic_skip);
    VPILOGD("intraQpDelta=%d\n", ecfg->intra_qp_delta);
    VPILOGD("goldenPictureRate=%d\n", ecfg->golden_picture_rate);
    VPILOGD("altrefPictureRate=%d\n", ecfg->altref_picture_rate);
    VPILOGD("goldenPictureBoost=%d\n", ecfg->golden_picture_boost);
    VPILOGD("highPrecMvEnable=%d\n", ecfg->high_prec_mv_enable);

    for (i = 0; i < 4; i++) {
        VPILOGD("lambda[%d]=%d\n", i, ecfg->lambda[i]);
    }
    for (i = 0; i < 4; i++) {
        VPILOGD("split_penalty[%d]=%d\n", i, ecfg->split_penalty[i]);
    }
    VPILOGD("meChromaWeight=%d\n", ecfg->me_chroma_weight);
    VPILOGD("error_resilient=%d\n", ecfg->error_resilient);
    VPILOGD("frm_para_decmode=%d\n", ecfg->frm_para_decmode);
    VPILOGD("loop_input=%d\n", ecfg->loop_input);
    VPILOGD("dropable=%d\n", ecfg->dropable);
    VPILOGD("adaptiveGoldenBoost=%d\n", ecfg->adaptive_golden_boost);
    VPILOGD("adaptiveGoldenUpdate=%d\n", ecfg->adaptive_golden_update);
    VPILOGD("quality_metric=%d\n", ecfg->quality_metric);
    for (i = 0; i < 3; i++) {
        VPILOGD("qpDelta[%d]=%d\n", i, ecfg->qp_delta[i]);
    }
    VPILOGD("outBufSizeMax=%d\n", ecfg->out_buf_size_max);
    VPILOGD("lossless=%d\n", ecfg->lossless);
    VPILOGD("stats=%d\n", ecfg->stats);
    VPILOGD("segments=%d\n", ecfg->segments);
    VPILOGD("min_arf_period=%d\n", ecfg->min_arf_period);
    VPILOGD("max_arf_period=%d\n", ecfg->max_arf_period);
    VPILOGD("pass=%d\n", ecfg->pass);
    VPILOGD("show_hidden_frames=%d\n", ecfg->show_hidden_frames);
    VPILOGD("arf_tmp_filter_enabled=%d\n", ecfg->arf_tmp_filter_enabled);
    VPILOGD("arf_tmp_filter_strength=%d\n", ecfg->arf_tmp_filter_strength);
    VPILOGD("arf_tmp_filter_threshold=%d\n", ecfg->arf_tmp_filter_threshold);
    VPILOGD("arf_tmp_filter_length=%d\n", ecfg->arf_tmp_filter_length);
    VPILOGD("firstpass_scale=%d\n", ecfg->firstpass_scale);
    VPILOGD("firstpass_scale_method=%d\n", ecfg->firstpass_scale_method);
    VPILOGD("cq_level=%d\n", ecfg->cq_level);
    VPILOGD("stride=%d\n", ecfg->stride);
    VPILOGD("bandwidth_mode=%d\n", ecfg->bandwidth_mode);
    VPILOGD("wme_lambda_motion=%d\n", ecfg->wme_lambda_motion);
    VPILOGD("wme_lambda_zero=%d\n", ecfg->wme_lambda_zero);
    VPILOGD("no_input_conversion=%d\n", ecfg->no_input_conversion);
    VPILOGD("truncate10b=%d\n", ecfg->truncate10b);
    VPILOGD("bitdepth=%d\n", ecfg->bitdepth);
    VPILOGD("inputBitdepth=%d\n", ecfg->input_bitdepth);
    VPILOGD("inputFormat=%d\n", ecfg->input_format);
    VPILOGD("lumWidthSrc=%d\n", ecfg->lum_width_src);
    VPILOGD("lumHeightSrc=%d\n", ecfg->lum_height_src);
    VPILOGD("width=%d\n", ecfg->width);
    VPILOGD("height=%d\n", ecfg->height);
    VPILOGD("priority=%d\n", ecfg->priority);
    VPILOGD("device=%s\n", ecfg->device);
    VPILOGD("mem_id=%d\n", ecfg->mem_id);
    VPILOGD("fd=%d\n", ecfg->fd);
    VPILOGD("--- VpiEncVp9Setting dump finished---\n\n");
}

int vp9enc_default_parameters(VpiEncVp9Setting *ecfg)
{
    /*init the ecfg options which not opened*/
    ecfg->width            = DEFAULT;
    ecfg->height           = DEFAULT;
    ecfg->bitdepth         = VP9ENC_8BIT;
    ecfg->scale_in_width   = DEFAULT;
    ecfg->scale_in_height  = DEFAULT;
    ecfg->lum_width_src    = DEFAULT;
    ecfg->lum_height_src   = DEFAULT;
    ecfg->letterbox_left   = 0;
    ecfg->letterbox_right  = 0;
    ecfg->letterbox_top    = 0;
    ecfg->letterbox_bottom = 0;
    ecfg->letterbox_ycolor = DEFAULT;
    ecfg->letterbox_ucolor = DEFAULT;
    ecfg->letterbox_vcolor = DEFAULT;
    ecfg->user_scaler_taps = false;
    ecfg->hor_offset_src   = 0;
    ecfg->ver_offset_src   = 0;
    ecfg->input_format     = 0;

    ecfg->fpf                = firstpass;
    ecfg->first_picture      = 0;
    ecfg->last_pic           = 99;
    ecfg->input_rate_number  = 30;
    ecfg->input_rate_denom   = 1;
    ecfg->output_rate_number = DEFAULT;
    ecfg->output_rate_denom  = DEFAULT;
    ecfg->bit_per_second     = DEFAULT;

    /* Default settings are get from API and not changed in testbench */
    ecfg->color_conv       = 0;
    ecfg->color_range_conv = 0;
    ecfg->force_8bit       = 0;

    ecfg->qp_hdr                 = DEFAULT;
    ecfg->qp_min                 = DEFAULT;
    ecfg->qp_max                 = DEFAULT;
    ecfg->bitrate_window         = DEFAULT;
    ecfg->pic_rc                 = DEFAULT;
    ecfg->intra_pic_rate         = 0;
    ecfg->pic_skip               = 0;
    ecfg->intra_qp_delta         = DEFAULT;
    ecfg->fixed_intra_qp         = DEFAULT;
    ecfg->golden_picture_rate    = DEFAULT;
    ecfg->altref_picture_rate    = DEFAULT;
    ecfg->golden_picture_boost   = DEFAULT;
    ecfg->adaptive_golden_boost  = DEFAULT;
    ecfg->adaptive_golden_update = DEFAULT;
    ecfg->quality_metric         = DEFAULT;
    ecfg->ref_frame_scheme       = DEFAULT;
    ecfg->loop_input             = 0;

    ecfg->error_resilient          = DEFAULT;
    ecfg->filter_level             = DEFAULT;
    ecfg->filter_sharpness         = DEFAULT;
    ecfg->mcomp_filter_type        = DEFAULT;
    ecfg->me_chroma_weight         = DEFAULT;
    ecfg->high_prec_mv_enable      = DEFAULT;
    ecfg->lambda[0]                = DEFAULT;
    ecfg->split_penalty[0]         = DEFAULT;
    ecfg->wme_lambda_zero          = DEFAULT;
    ecfg->wme_lambda_motion        = DEFAULT;
    ecfg->bandwidth_mode           = DEFAULT;
    ecfg->min_arf_period           = DEFAULT;
    ecfg->max_arf_period           = DEFAULT;
    ecfg->arf_tmp_filter_enabled   = 0;
    ecfg->arf_tmp_filter_strength  = DEFAULT;
    ecfg->arf_tmp_filter_threshold = DEFAULT;
    ecfg->arf_tmp_filter_length    = DEFAULT;

    ecfg->cq_level    = 0;
    ecfg->stats       = 0;
    ecfg->segments    = 0;
    ecfg->segment_map = NULL;
    ecfg->axi_rd_id = ecfg->axi_wr_id = 0;
    ecfg->out_buf_size_max            = 8;
    ecfg->pass                        = 0;
    ecfg->show_hidden_frames          = 0;
    ecfg->recondump                   = 0;
    ecfg->firstpass_scale             = 100;
    ecfg->firstpass_scale_method      = 0;

    ecfg->frameComp           = 1;
    ecfg->qp_delta[0]         = 0;
    ecfg->qp_delta[1]         = 0;
    ecfg->qp_delta[2]         = 0;
    ecfg->rotation            = 0;
    ecfg->flip                = 0;
    ecfg->no_input_conversion = 1;

    ecfg->perf_cnt_sum = 0;
    ecfg->perf_cnt_min = ~0;
    ecfg->perf_cnt_max = 0;
    ecfg->dram_rd_sum  = 0;
    ecfg->dram_wr_sum  = 0;
    ecfg->dram_rd_max  = 0;
    ecfg->dram_wr_max  = 0;

    ecfg->print_ssim     = 1;
    ecfg->multicore      = 0;
    ecfg->input_bitdepth = 0;
    ecfg->pp_dump        = 0;
    ecfg->pp_only        = 0;
    ecfg->recode         = 0;
    ecfg->ssim_count     = 0;

    return 0;
}

int vp9enc_open(VpiEncVp9Ctx *ctx, VpiEncVp9Setting *ecfg)
{
    VP9EncRet ret;
    VP9EncConfig cfg;
    VP9ENCPERF *perf = &ctx->perf;
    EncPreProcessingCfg proccfg;

    if (ctx == NULL || ecfg == NULL) {
        VPILOGE("vp9enc_open input error.\n");
        return -VP9ENC_NULL_ARGUMENT;
    }

#ifdef FB_SYSLOG_ENABLE
    //sprintf(&tb.module_name, "%s%d", tb_default.module_name, ctx->enc_index);
    vp9enc_tb.device_id = get_deviceId(ecfg->device);
#endif

    if (ecfg->lossless && ecfg->segments) {
        VPILOGE("Cannot have segments enabled in lossless mode!",
                VP9ENC_INVALID_ARGUMENT);
        return -VP9ENC_ERROR;
    }

    if (convert_encoder_config(ctx, &cfg, ecfg) != 0) {
        VPILOGE("VP9SetEncoderCfg() failed:ret=%d\n");
        return -VP9ENC_ERROR;
    }

    pthread_mutex_init(&perf->hwcycle_acc_mutex, NULL);
#ifdef FB_PERFORMANCE_STATIC
    PERFORMANCE_STATIC_INIT(tb, perf, vp9hw);
    PERFORMANCE_STATIC_INIT(tb, perf, vp9hw_p1);
    PERFORMANCE_STATIC_INIT(tb, perf, VP9_dummy_0);
    PERFORMANCE_STATIC_INIT(tb, perf, VP9_dummy_1);
    PERFORMANCE_STATIC_INIT(tb, perf, vp9hw_total);
    PERFORMANCE_STATIC_INIT(tb, perf, vp9_total);
#endif

    ret = VP9EncInit(&cfg, &ctx->encoder);
    if (ret != VP9ENC_OK) {
        VPILOGE("VP9EncInit() failed, error no=%d\n", ret);
        return -VP9ENC_ERROR;
    }

    VP9AXISet(ctx->encoder, ecfg->swap_input, ecfg->axi_rd_id, ecfg->axi_wr_id,
              ecfg->axi_rd_burst, ecfg->axi_wr_burst);

    /* Encoder setup: rate control */
    if (vp9_set_rate_control(ctx, ecfg) != 0) {
        VPILOGE("vp9_set_rate_control() failed\n");
        return -VP9ENC_ERROR;
    }

    if (vp9_set_coding_control(ctx, ecfg) != 0) {
        VPILOGE("vp9_set_coding_control() failed\n");
        return -VP9ENC_ERROR;
    }

    memset(&proccfg, 0, sizeof(proccfg));
    if (vp9_set_pre_processing(ctx, ecfg, &proccfg) != 0) {
        VPILOGE("vp9_set_pre_processing() failed\n");
        return -VP9ENC_ERROR;
    }

    if (vp9_set_first_pass(ctx, ecfg, &proccfg) != 0) {
        VPILOGE("vp9_set_first_pass() failed\n");
        return -VP9ENC_ERROR;
    }

    ret = vp9_allocate_resource(ctx);
    if (ret != 0) {
        VPILOGE("ff_hantro_encodevp9_get_stream_buffer failed\n");
        return -VP9ENC_MEMORY_ERROR;
    }

    ret = vp9_pre_encode(ctx);
    if (ret != 0) {
        VPILOGE("vp9_pre_encode failed\n");
        return -VP9ENC_ERROR;
    }

    ctx->poc              = 0;
    ctx->next             = 0;
    ctx->code_frame_count = 0;
    ctx->frm_cnt_hw_total = 0;
    ctx->ssim_sum         = 0;
    ctx->total_bits       = 0;
    ctx->in_width         = 0;
    ctx->in_height        = 0;
    ctx->encoder_is_open  = true;

    return ret;
}

int vp9enc_set_ppindex(VpiEncVp9Ctx *ctx, VpiFrame *frame, VpiEncVp9Opition *cfg)
{
    int i = 0, pp_index = 0;

    for (i = 0; i < PIC_INDEX_MAX_NUMBER; i++) {
        if (frame->pic_info[i].width == cfg->width &&
            frame->pic_info[i].height == cfg->height) {
            pp_index = i;
            VPILOGD("Find pp_index = %d, [w=%d, h=%d]\n", pp_index,
                    cfg->width, cfg->height);
            break;
        }
    }

    if (i == PIC_INDEX_MAX_NUMBER) {
        VPILOGE("pp_index %d isn't vaild\n", i);
        return -1;
    }

    if ((pp_index == DEC_OUT_RFC) || (pp_index == DEC_OUT_PP0)) {
        ctx->enc_index = 0;
    } else {
        ctx->enc_index = pp_index - 1;
    }
    ctx->pp_index = pp_index;
    VPILOGD("pp_index = %d, enc_index = %d\n", pp_index, ctx->enc_index);
    return 0;
}

/*
 * convert VpiEncVp9Setting preset to detail setting
 */
int vp9enc_setpreset(VpiEncVp9Setting *ecfg)
{
    VP9Preset preset = VP9ENC_PRESET_NONE;
    VPILOGD("VpeEnc Preset: %s\n", ecfg->preset);

    if (ecfg->preset) {
        if (strcmp(ecfg->preset, "superfast") == 0) {
            preset = VP9ENC_PRESET_SUPERFAST;
        } else if (strcmp(ecfg->preset, "fast") == 0) {
            preset = VP9ENC_PRESET_FAST;
        } else if (strcmp(ecfg->preset, "medium") == 0) {
            preset = VP9ENC_PRESET_MEDIUM;
        } else if (strcmp(ecfg->preset, "slow") == 0) {
            preset = VP9ENC_PRESET_SLOW;
        } else if (strcmp(ecfg->preset, "superslow") == 0) {
            preset = VP9ENC_PRESET_SUPERSLOW;
        } else {
            VPILOGD("unknow VpeEnc preset %s\n", ecfg->preset);
            preset = VP9ENC_PRESET_NONE;
            return -1;
        }
    }

    switch (preset) {
    case VP9ENC_PRESET_SUPERFAST:
        if (ecfg->qp_hdr == DEFAULT)
            ecfg->qp_hdr = -1;
        if (ecfg->pic_rc == DEFAULT)
            ecfg->pic_rc = 1;
        if (ecfg->effort == DEFAULT)
            ecfg->effort = 0;
        if (ecfg->mcomp_filter_type == DEFAULT)
            ecfg->mcomp_filter_type = 4;
        if (ecfg->ref_frame_scheme == DEFAULT)
            ecfg->ref_frame_scheme = 0;
        if (ecfg->lag_in_frames == DEFAULT)
            ecfg->lag_in_frames = 0;
        break;

    case VP9ENC_PRESET_FAST:
        if (ecfg->qp_hdr == DEFAULT)
            ecfg->qp_hdr = -1;
        if (ecfg->pic_rc == DEFAULT)
            ecfg->pic_rc = 1;
        if (ecfg->effort == DEFAULT)
            ecfg->effort = 0;
        if (ecfg->mcomp_filter_type == DEFAULT)
            ecfg->mcomp_filter_type = 4;
        if (ecfg->ref_frame_scheme == DEFAULT)
            ecfg->ref_frame_scheme = 4;
        if (ecfg->lag_in_frames == DEFAULT)
            ecfg->lag_in_frames = 7;
        break;

    case VP9ENC_PRESET_MEDIUM:
        if (ecfg->qp_hdr == DEFAULT)
            ecfg->qp_hdr = -1;
        if (ecfg->pic_rc == DEFAULT)
            ecfg->pic_rc = 1;
        if (ecfg->effort == DEFAULT)
            ecfg->effort = 0;
        if (ecfg->mcomp_filter_type == DEFAULT)
            ecfg->mcomp_filter_type = 4;
        if (ecfg->ref_frame_scheme == DEFAULT)
            ecfg->ref_frame_scheme = 4;
        if (ecfg->lag_in_frames == DEFAULT)
            ecfg->lag_in_frames = 12;
        if (ecfg->passes == DEFAULT)
            ecfg->passes = 2;
        break;

    case VP9ENC_PRESET_SLOW:
        if (ecfg->qp_hdr == DEFAULT)
            ecfg->qp_hdr = -1;
        if (ecfg->pic_rc == DEFAULT)
            ecfg->pic_rc = 1;
        if (ecfg->effort == DEFAULT)
            ecfg->effort = 1;
        if (ecfg->mcomp_filter_type == DEFAULT)
            ecfg->mcomp_filter_type = 4;
        if (ecfg->ref_frame_scheme == DEFAULT)
            ecfg->ref_frame_scheme = 4;
        if (ecfg->lag_in_frames == DEFAULT)
            ecfg->lag_in_frames = 25;
        if (ecfg->passes == DEFAULT)
            ecfg->passes = 2;
        break;

    case VP9ENC_PRESET_SUPERSLOW: /* 2 pass */
        if (ecfg->qp_hdr == DEFAULT)
            ecfg->qp_hdr = -1;
        if (ecfg->pic_rc == DEFAULT)
            ecfg->pic_rc = 1;
        if (ecfg->effort == DEFAULT)
            ecfg->effort = 2;
        if (ecfg->mcomp_filter_type == DEFAULT)
            ecfg->mcomp_filter_type = 4;
        if (ecfg->ref_frame_scheme == DEFAULT)
            ecfg->ref_frame_scheme = 4;
        if (ecfg->lag_in_frames == DEFAULT)
            ecfg->lag_in_frames = 25;
        if (ecfg->passes == DEFAULT)
            ecfg->passes = 2;
        break;

    case VP9ENC_PRESET_NONE:
        break;
    default:
        VPILOGE("unknow preset %d\n", preset);
        break;
    }

    /* check preset options to DEFAULT */
    if (ecfg->qp_hdr == DEFAULT)
        ecfg->qp_hdr = -1;
    if (ecfg->pic_rc == DEFAULT)
        ecfg->pic_rc = 1;
    if (ecfg->effort == DEFAULT)
        ecfg->effort = DEFAULT;
    if (ecfg->mcomp_filter_type == DEFAULT)
        ecfg->mcomp_filter_type = 4;
    if (ecfg->ref_frame_scheme == DEFAULT)
        ecfg->ref_frame_scheme = DEFAULT;
    if (ecfg->lag_in_frames == DEFAULT)
        ecfg->lag_in_frames = 7;
    if (ecfg->passes == DEFAULT)
        ecfg->passes = 1;

    return 0;
}

/*
 * Update VpiEncVp9Ctx->VpiEncVp9Setting according to input frame info.
 */
int vp9enc_updatesetting_fromframe(VpiEncVp9Ctx *ctx, VpiFrame *in,
                                   VpiEncVp9Setting *out)
{
    VpiEncVp9Setting *ecfg = out;
    VpiFrame *frame        = in;
    u32 width, height;
    VpiPicInfo *pic_info = &frame->pic_info[ctx->pp_index];
    VpiPicData *pic_data = &pic_info->picdata;

    if (!ctx || !in || !out) {
        VPILOGE("vp9enc_statistic input error\n");
        return -1;
    }

    ecfg->lum_width_src  = pic_info->width;
    ecfg->lum_height_src = NEXT_MULTIPLE(pic_info->height, 8);

    if (pic_info->height != ecfg->lum_height_src) {
        ecfg->height = pic_info->height;
    }

    if (pic_data->is_interlaced == 0) {
        if ((ctx->pp_index == DEC_OUT_RFC) || (ctx->pp_index == DEC_OUT_PP0)) {
            if (ctx->in_width) {
                if (pic_info->width != ctx->in_width) {
                    ecfg->width = NEXT_MULTIPLE(ctx->in_width, 2);
                }
            } else if (pic_data->crop_out_width) {
                if (pic_data->crop_out_width < pic_info->width) {
                    ecfg->width = NEXT_MULTIPLE(pic_data->crop_out_width, 2);
                }
            }
            if (ctx->in_height) {
                if (pic_info->height != ctx->in_height) {
                    ecfg->height = NEXT_MULTIPLE(ctx->in_height, 2);
                }
            } else if (pic_data->crop_out_height) {
                if (pic_data->crop_out_height < pic_info->height) {
                    ecfg->height = NEXT_MULTIPLE(pic_data->crop_out_height, 2);
                }
            }
        }
    } else {
        if ((ctx->pp_index == DEC_OUT_RFC) || (ctx->pp_index == DEC_OUT_PP0)) {
            if (pic_data->crop_out_width < ecfg->lum_width_src) {
                ecfg->width = NEXT_MULTIPLE(pic_data->crop_out_width, 2);
            }
            if (pic_data->crop_out_height < ecfg->lum_height_src) {
                ecfg->height = NEXT_MULTIPLE(pic_data->crop_out_height, 2);
            }
        }
    }

    switch (pic_data->pic_format) {
    case DEC_OUT_FRM_TILED_4X4:
        ecfg->input_format   = ENC_YUV420_SEMIPLANAR_TILED4x4;
        ecfg->input_compress = 0;
        ecfg->input_bitdepth = 0;
        break;
    case DEC_OUT_FRM_YUV420TILE_P010:
        ecfg->input_format   = ENC_YUV420_SEMIPLANAR_P010_TILED4x4;
        ecfg->input_compress = 0;
        ecfg->input_bitdepth = 2;
        break;
    case DEC_OUT_FRM_YUV420TILE_PACKED:
        ecfg->input_format   = ENC_YUV420_SEMIPLANAR;
        ecfg->input_compress = 2;
        break;
    case DEC_OUT_FRM_RASTER_SCAN:
        ecfg->input_format   = ENC_YUV420_SEMIPLANAR;
        ecfg->input_compress = 0;
        ecfg->input_bitdepth = 0;
        break;
    default:
        ecfg->input_format   = ENC_YUV420_SEMIPLANAR_TILED4x4;
        ecfg->input_compress = 0;
        break;
    }

    ecfg->input_bitdepth = pic_data->bit_depth_luma;
    VPILOGD("pic_compressed_status = %d\n", pic_data->pic_compressed_status);

    if (pic_data->pic_compressed_status == 2) {
        ecfg->input_compress = 1;
    } else if (pic_data->pic_compressed_status == 1) {
        ecfg->input_compress = 2;
    } else {
        ecfg->input_compress = 0;
    }

    if (pic_data->bit_depth_luma > 8 || pic_data->bit_depth_chroma > 8) {
        ecfg->input_bitdepth = 2;
        ecfg->bitdepth       = 1;
    } else {
        ecfg->input_bitdepth = 0;
        ecfg->bitdepth       = 0;
    }

    switch (pic_data->pic_pixformat) {
    case DEC_OUT_PIXEL_DEFAULT:
        if (pic_data->bit_depth_luma > 8 || pic_data->bit_depth_chroma > 8) {
            ecfg->input_bitdepth = 2;
        } else {
            ecfg->input_bitdepth = 0;
        }
        break;

    case DEC_OUT_PIXEL_CUT_8BIT:
        ecfg->input_bitdepth = 0;
        break;
    case DEC_OUT_PIXEL_P010: /* pp out 10bit is this pixel format */
        ecfg->input_bitdepth = 2;
        ecfg->input_format   = ENC_YUV420_SEMIPLANAR_P010_TILED4x4;
        break;
    case DEC_OUT_PIXEL_RFC:
        VPILOGE("should not get here! RFC output disabled\n");
        return -1;
        break;

    default:
        break;
    }

    /* FIXME: to check how bitdepth need to be set */
    ecfg->bitdepth = (ecfg->input_bitdepth == 2) ? 1 : 0;
    if (ecfg->input_bitdepth == 2) {
        if (ecfg->force_8bit) {
            ecfg->bitdepth = 0;
        } else if (ecfg->bitdepth != DEFAULT_VALUE) {
            if (ecfg->bitdepth == 0) {
                ecfg->bitdepth = 0;
            }
        }
    }

    width = (ecfg->width == DEFAULT_VALUE) ? ecfg->lum_width_src : ecfg->width;
    height =
        (ecfg->height == DEFAULT_VALUE) ? ecfg->lum_height_src : ecfg->height;
    VPILOGD("check bitrate by width %d height %d\n", width, height);

    if (ecfg->bit_per_second == DEFAULT_VALUE) {
        int res_index        = get_res_index(width, height);
        ecfg->bit_per_second = trans_default_bitrate[res_index];
        VPILOGD("get default bitrate %d\n", ecfg->bit_per_second);
    } else {
        u32 original_bits_perframe;
        original_bits_perframe = (width * height * 3) / 2;
        original_bits_perframe *= 8;
        if (ecfg->bitdepth > 8) {
            original_bits_perframe *= 2;
        }
        if (ecfg->bit_per_second > original_bits_perframe / 2) {
            ecfg->bit_per_second = original_bits_perframe / 2;
            ecfg->bit_per_second =
                ((ecfg->bit_per_second + 100000 - 1) / 100000) * 100000;
            VPILOGD("limit bitrate to %d\n", ecfg->bit_per_second);
        }
    }

    return 0;
}

void vp9enc_get_max_frame_delay(VpiEncVp9Ctx *ctx, VpiFrame *v_frame,
                                VpiEncVp9Setting *v_setting)
{
    int max_size, max_frames_delay;

    max_frames_delay = 10;

    max_size = (v_setting->lag_in_frames || (v_setting->ref_frame_scheme == 4))
               ? (v_setting->lag_in_frames + 2) : 2;
    if (max_size < 4)
        max_size = 4;
    max_frames_delay += max_size;

    if (max_frames_delay > v_frame->max_frames_delay) {
        v_frame->max_frames_delay = max_frames_delay;
    }
    VPILOGD("vpeframe->max_frames_delay = %d\n", v_frame->max_frames_delay);
}

/*
* Set Input buffer to VP9EncIn
* input: enc_data
* output: enc_instance
* return: o is good, others if input data is NULL or empty
*/
int vp9enc_send_buffer_to_encoder(VP9EncIn *enc_instance, int pp_index,
                                  VpiFrame *input, VpiEncVp9Setting *ecfg)
{
    struct DecPicture *enc_data = NULL;
    EncInAddr addrs             = { 0, 0, 0, 0 };
    struct DecPicturePpu *pic = pic = (struct DecPicturePpu *)input->data[0];

    if (pic) {
        enc_data = &pic->pictures[pp_index];
        VPILOGD("Dump pic: %ld %ld %ld %ld\n", enc_data->luma.bus_address,
                enc_data->chroma.bus_address, enc_data->luma_table.bus_address,
                enc_data->chroma_table.bus_address);
    }

    if (!enc_data) {
        VPILOGE("Input picture is NULL, clean enc_instance buffer\n");
        goto error;
    }

    addrs.bus_luma         = enc_data->luma.bus_address;
    addrs.bus_chroma       = enc_data->chroma.bus_address;
    addrs.bus_luma_table   = enc_data->luma_table.bus_address;
    addrs.bus_chroma_table = enc_data->chroma_table.bus_address;

    if (addrs.bus_luma != 0) {
        enc_instance->busLuma        = addrs.bus_luma;
        enc_instance->busChromaU     = addrs.bus_chroma;
        enc_instance->busLumaTable   = addrs.bus_luma_table;
        enc_instance->busChromaTable = addrs.bus_chroma_table;
        enc_instance->compress       = ecfg->input_compress;
        enc_instance->width          = ecfg->lum_width_src;
        enc_instance->height         = ecfg->lum_height_src;
        enc_instance->encode_width   = ecfg->width;
        enc_instance->encode_height  = ecfg->height;
        enc_instance->pts            = input->pts;
        enc_instance->dts            = input->pkt_dts;
        VPILOGD(" busLuma[%p], busCU[%p], busLT[%p],"
                "busCT[%p],compress=%d,width=%d,height=%d\n",
                enc_instance->busLuma, enc_instance->busChromaU,
                enc_instance->busLumaTable, enc_instance->busChromaTable,
                enc_instance->compress, ecfg->width, ecfg->height);
    } else {
        VPILOGE("Input data is zero, clean enc_instance buffer\n");
        goto error;
    }
    return 0;

error:
    enc_instance->busLuma         = 0;
    enc_instance->busChromaU      = 0;
    enc_instance->busLumaTable    = 0;
    enc_instance->busChromaTable  = 0;
    enc_instance->end_of_sequence = 1;

    return -1;
}
