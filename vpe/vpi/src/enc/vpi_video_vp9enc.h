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

#ifndef __VPI_VIDEO_VP9ENC_H__
#define __VPI_VIDEO_VP9ENC_H__

#include "syslog_sink.h"
#include "vp9encapi.h"
#include "vpi_types.h"
#include "vpi_error.h"

typedef const void *VpiEncVp9Inst;

#define MAX_BPS_ADJUST 20
#define VP9ENC_MOVING_AVERAGE_FRAMES 30

#define FF_PROFILE_VP9_0 0
#define FF_PROFILE_VP9_1 1
#define FF_PROFILE_VP9_2 2
#define FF_PROFILE_VP9_3 3

typedef struct {
    int frame[VP9ENC_MOVING_AVERAGE_FRAMES];
    int length;
    int count;
    int pos;
    int frame_rate_number;
    int frame_rate_denom;
} vp9_ma_s;

typedef struct {
    /*
     * Below parameters are available in ffmpeg by public parameters
    */
    char *preset;
    int effort;
    int lag_in_frames;
    unsigned int passes;

    /*
     * Below parameters are available in ffmpeg through vpevp9-params
    */
    int intra_pic_rate;
    int bitrate_window;
    int qp_hdr;
    int qp_min;
    int qp_max;
    int fixed_intra_qp;
    int pic_rc;
    int mcomp_filter_type;
    int force_8bit;
    int ref_frame_scheme;
    int filter_level;
    int filter_sharpness;

    const char *fpf;
    int first_picture;
    int last_pic;
    int width;
    int height;
    int scale_in_width;
    int scale_in_height;
    int lum_width_src;
    int lum_height_src;
    int hor_offset_src;
    int ver_offset_src;
    int letterbox_left;
    int letterbox_right;
    int letterbox_top;
    int letterbox_bottom;
    int letterbox_ycolor;
    int letterbox_ucolor;
    int letterbox_vcolor;
    bool user_scaler_taps;
    int output_rate_number;
    int output_rate_denom;
    int input_rate_number;
    int input_rate_denom;

    int input_compress;
    int input_format;
    int rotation;
    int flip;
    int color_conv;
    int color_range_conv;

    int bit_per_second;
    int layer_bit_per_second[4];
    int tmp_layers;
    int pic_skip;
    int intra_qp_delta;
    int golden_picture_rate;
    int altref_picture_rate;
    int bps_adjust_frame[MAX_BPS_ADJUST];
    int bps_adjust_bitrate[MAX_BPS_ADJUST];
    int golden_picture_boost;
    int high_prec_mv_enable;
    int me_chroma_weight;
    int lambda[4];
    int split_penalty[4];
    int error_resilient;
    int frm_para_decmode;
    int loop_input;
    int print_ssim;
    int dropable;
    int recondump;
    int trace_mb_timing;
    int adaptive_golden_boost;
    int adaptive_golden_update;
    int quality_metric;
    int qp_delta[3];
    unsigned int out_buf_size_max;
    unsigned int multicore;

    unsigned int lossless;
    unsigned int stats;
    unsigned int frameComp;
    unsigned int segments;
    unsigned int swap_input;
    int axi_rd_id;
    int axi_wr_id;
    unsigned short axi_rd_burst;
    unsigned short axi_wr_burst;
    int min_arf_period;
    int max_arf_period;

    unsigned int pass;
    unsigned int show_hidden_frames;
    int arf_tmp_filter_enabled;
    int arf_tmp_filter_strength;
    int arf_tmp_filter_threshold;
    int arf_tmp_filter_length;
    unsigned int recode;
    void *rc_twopass_stats_in;
    size_t rc_twopass_stats_in_size;
    unsigned int firstpass_scale;
    unsigned int firstpass_scale_method;
    int cq_level;
    int stride;
    int bandwidth_mode;
    int wme_lambda_motion;
    int wme_lambda_zero;

    int bitdepth;
    int pp_dump;
    int pp_only;
    int input_bitdepth;
    int no_input_conversion;
    int truncate10b;
    const char *segment_map;

    unsigned int ssim_count;
    unsigned long perf_cnt_sum;
    unsigned int perf_cnt_min;
    unsigned int perf_cnt_max;
    unsigned long dram_rd_sum;
    unsigned long dram_wr_sum;
    unsigned int dram_rd_max;
    unsigned int dram_wr_max;

    int priority;
    char *device;
    int mem_id;
    int fd;
} VpiEncVp9Setting;

typedef struct {
    /* Below parameters are set by ffmpeg through "control" interface */
    int input_frame_wait_cnt;
    /*
     * Below parameters are sent back to upper layer to through
     * "control" interface.
    */
    int pic_tobe_free;

    VP9EncRet ret;
    VP9EncInst encoder;
    VP9EncApiVersion api_ver;
    VP9EncBuild enc_build;
    VP9EncIn enc_in;
    VP9EncConfig cfg;
    VP9EncCodingCtrl coding_cfg;
    VP9EncRateCtrl rc_config;
    EncPreProcessingCfg preproc_cfg;
    VP9EncOut enc_out;
    VP9EncRateCtrl rc;
    int enc_index;
    int firstframe;
    int pic_width;
    VP9ENCPERF perf;

    const char *input;
    const char *output;
    const char *fpf;

    /* SW/HW shared memories for input/output buffers */
    CWLLinearMem_t picture_mem;
    CWLLinearMem_t outbuff_mem;
    CWLLinearMem_t scaled_pic_mem;

    unsigned short *ref_desc_mem;
    void *rc_twopass_stats_in;
    unsigned char *seg_buf;

    unsigned int src_img_size;
    unsigned long stream_size; /* Size of output stream in bytes */
    unsigned int bitrate; /* Calculate average bitrate of encoded frames */
    vp9_ma_s ma; /* Calculate moving average of bitrate */
    float psnrSum; /* Calculate average PSNR over encoded frames */
    unsigned int psnr_count;
    float ssim_sum;

    int quality_metric;
    int tmp_layers;
    int lambda[4];
    int split_penalty[4];
    unsigned long frame_count_out; /* encoded frame counter */
    int frame_count; /* Frame counter of current input file */
    unsigned long frm_cnt_total; /* Frame counter of all encoded frames */
    unsigned long input_frm_total; /* Frame counter of all input frames */
    unsigned long frm_cnt_hw_total; /*Hw Frame counter of all encoded frames */
    unsigned long
        time_used_hw_total; /*Hw Frame time used of all encoded frames */
    unsigned long code_frame_count;
    unsigned int mbs;
    int next;
    int poc;
    int pp_index;
    int draining_set_flag;
    unsigned int initialized;
    bool encoder_is_open;
    unsigned long total_bits;

    bool loop_condition;
    bool loop_break;
    int input_real_count;

    unsigned int sb_per_row;
    unsigned int tile_pos[16];
    unsigned int tile_cols;

    int inter_draining_cnt;
    unsigned int in_width;
    unsigned int in_height;

    VpiEncVp9Setting vp9_enc_cfg;

} VpiEncVp9Ctx;

int vpi_venc_vp9_init(VpiEncVp9Ctx *vp9_ctx, void *cfg);
int vpi_venc_vp9_encode(VpiEncVp9Ctx *vp9_ctx, void *indata, void *outdata);
int vpi_venc_vp9_close(VpiEncVp9Ctx *vp9_ctx);
int vpi_venc_vp9_control(VpiEncVp9Ctx *ctx, void *indata, void *outdata);

#endif
