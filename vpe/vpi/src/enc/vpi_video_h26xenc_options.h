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

#ifndef __VPI_VIDEO_H26XENC_OPTIONS_H__
#define __VPI_VIDEO_H26XENC_OPTIONS_H__

#define MAX_BPS_ADJUST 20
#define MAX_STREAMS 16
#define MAX_SCENE_CHANGE 20
#include "hevcencapi.h"

typedef struct VPIH26xEncOptions {
    i32 output_rate_numer; /* Output frame rate numerator */
    i32 output_rate_denom; /* Output frame rate denominator */
    i32 input_rate_numer; /* Input frame rate numerator */
    i32 input_rate_denom; /* Input frame rate denominator */
    i32 first_pic;
    i32 last_pic;

    i32 width;
    i32 height;
    i32 lum_width_src;
    i32 lum_height_src;

    i32 force_8bit;
    i32 bitdepth;

    /* for 1/4 resolution first pass, for disable rfc datapath, horOffsetSrc and verOffsetSrc should be 0, so not need ds params*/
    i32 width_ds;
    i32 height_ds;
    i32 lum_widthsrc_ds;
    i32 lum_heightsrc_ds;
    i32 input_format_ds;

    i32 input_format;
    i32 format_customized_type; /*change general format to customized one */

    i32 picture_cnt;
    i32 byte_stream;

    i32 max_cu_size; /* Max coding unit size in pixels */
    i32 min_cu_size; /* Min coding unit size in pixels */
    i32 max_tr_size; /* Max transform size in pixels */
    i32 min_tr_size; /* Min transform size in pixels */
    i32 tr_depth_intra; /* Max transform hierarchy depth */
    i32 tr_depth_inter; /* Max transform hierarchy depth */
    VCEncVideoCodecFormat codec_format; /* Video Codec Format: HEVC/H264/AV1 */

    i32 min_qp_size;

    i32 enable_cabac; /* [0,1] H.264 entropy coding mode, 0 for CAVLC, 1 for CABAC */
    i32 cabac_init_flag;

    /* intra setup*/
    u32 strong_intra_smoothing_enabled_flag;

    i32 cir_start;
    i32 cir_interval;

    i32 intra_area_enable;
    i32 intra_area_left;
    i32 intra_area_top;
    i32 intra_area_right;
    i32 intra_area_bottom;

    i32 pcm_loop_filter_disabled_flag;

    i32 ipcm1_area_left;
    i32 ipcm1_area_top;
    i32 ipcm1_area_right;
    i32 ipcm1_area_bottom;

    i32 ipcm2_area_left;
    i32 ipcm2_area_top;
    i32 ipcm2_area_right;
    i32 ipcm2_area_bottom;
    i32 ipcm_map_enable;
    char *ipcm_map_file;

    char *skip_map_file;
    i32 skip_map_enable;
    i32 skip_map_block_unit;

    i32 roi1_area_enable;
    i32 roi2_area_enable;
    i32 roi3_area_enable;
    i32 roi4_area_enable;
    i32 roi5_area_enable;
    i32 roi6_area_enable;
    i32 roi7_area_enable;
    i32 roi8_area_enable;

    i32 roi1_area_top;
    i32 roi1_area_left;
    i32 roi1_area_bottom;
    i32 roi1_area_right;

    i32 roi2_area_top;
    i32 roi2_area_left;
    i32 roi2_area_bottom;
    i32 roi2_area_right;

    i32 roi1_delta_qp;
    i32 roi2_delta_qp;
    i32 roi1_qp;
    i32 roi2_qp;

    i32 roi3_area_top;
    i32 roi3_area_left;
    i32 roi3_area_bottom;
    i32 roi3_area_right;
    i32 roi3_delta_qp;
    i32 roi3_qp;

    i32 roi4_area_top;
    i32 roi4_area_left;
    i32 roi4_area_bottom;
    i32 roi4_area_right;
    i32 roi4_delta_qp;
    i32 roi4_qp;

    i32 roi5_area_top;
    i32 roi5_area_left;
    i32 roi5_area_bottom;
    i32 roi5_area_right;
    i32 roi5_delta_qp;
    i32 roi5_qp;

    i32 roi6_area_top;
    i32 roi6_area_left;
    i32 roi6_area_bottom;
    i32 roi6_area_right;
    i32 roi6_delta_qp;
    i32 roi6_qp;

    i32 roi7_area_top;
    i32 roi7_area_left;
    i32 roi7_area_bottom;
    i32 roi7_area_right;
    i32 roi7_delta_qp;
    i32 roi7_qp;

    i32 roi8_area_top;
    i32 roi8_area_left;
    i32 roi8_area_bottom;
    i32 roi8_area_right;
    i32 roi8_delta_qp;
    i32 roi8_qp;

    /* Rate control parameters */
    i32 hrd_conformance;
    i32 cpb_size;
    i32 intra_pic_rate; /* IDR interval */

    i32 vbr; /* Variable Bit Rate Control by qpMin */
    i32 qp_hdr;
    i32 qp_min;
    i32 qp_max;
    i32 qp_min_I;
    i32 qp_max_I;
    i32 bit_per_second;
    i32 crf; /*CRF constant */

    i32 bit_var_range_I;

    i32 bit_var_range_P;

    i32 bit_var_range_B;
    u32 u32_static_scene_ibit_percent;

    i32 tol_moving_bitrate; /*tolerance of max Moving bit rate */
    i32 monitor_frames; /*monitor frame length for moving bit rate */
    i32 pic_rc;
    i32 ctb_rc;
    i32 block_rc_size;
    u32 rc_qp_delta_range;
    u32 rc_base_mb_complexity;
    i32 pic_skip;
    i32 pic_qp_delta_min;
    i32 pic_qp_delta_max;
    i32 ctb_rc_row_qp_step;

    float tol_ctb_rc_inter;
    float tol_ctb_rc_intra;

    i32 bitrate_window;
    i32 intra_qp_delta;
    i32 fixed_intra_qp;
    i32 bframe_qp_delta;

    i32 disable_deblocking;

    i32 enable_sao;

    i32 tc_offset;
    i32 beta_offset;

    i32 chroma_qp_offset;

    i32 profile; /*main profile or main still picture profile */
    i32 tier; /*main tier or high tier */
    i32 level; /*main profile level */

    i32 bps_adjust_frame[MAX_BPS_ADJUST];
    i32 bps_adjust_bitrate[MAX_BPS_ADJUST];
    i32 smooth_psnr_in_gop;

    i32 slice_size;

    i32 test_id;

    i32 rotation;
    i32 mirror;
    i32 hor_offset_src;
    i32 ver_offset_src;
    i32 color_conversion;
    i32 scaled_width;
    i32 scaled_height;
    i32 scaled_output_format;

    i32 enable_deblock_override;
    i32 deblock_override;

    i32 enable_scaling_list;

    u32 compressor;

    i32 interlaced_frame;
    i32 field_order;
    i32 video_range;
    i32 ssim;
    i32 sei;
    char *user_data;
    u32 gop_size;
    char *gop_cfg;
    u32 gop_lowdelay;
    i32 out_recon_frame;
    u32 long_term_gap;
    u32 long_term_gap_offset;
    u32 ltr_interval;
    i32 long_term_qp_delta;

    i32 gdr_duration;
    u32 roi_map_delta_qp_block_unit;
    u32 roi_map_delta_qp_enable;
    char *roi_map_delta_qp_file;
    char *roi_map_delta_qp_bin_file;
    char *roi_map_info_bin_file;
    char *roimap_cu_ctrl_info_bin_file;
    char *roimap_cu_ctrl_index_bin_file;
    u32 roi_cu_ctrl_ver;
    u32 roi_qp_delta_ver;
    i32 out_buf_size_max;
    i32 multimode; /* Multi-stream mode, 0--disable, 1--mult-thread, 2--multi-process*/
    char *streamcfg[MAX_STREAMS];
    i32 outfile_format; /*0->hevc, 1->h264, 2->vp9*/

    /*WIENER_DENOISE*/
    i32 noise_reduction_enable;
    i32 noise_low;
    i32 first_frame_sigma;

    i32 bit_depth_luma;
    i32 bit_depth_chroma;

    u32 enable_output_cu_info;

    u32 rdo_level;
    /* low latency */
    i32 input_line_buf_mode;
    i32 input_line_buf_depth;
    i32 amount_per_loop_back;

    u32 hashtype;
    u32 verbose;

    /* for smart */
    i32 smart_mode_enable;
    i32 smart_h264_qp;
    i32 smart_hevc_lum_qp;
    i32 smart_hevc_chr_qp;
    i32 smart_h264_lum_dc_th;
    i32 smart_h264_cb_dc_th;
    i32 smart_h264_cr_dc_th;
    /* threshold for hevc cu8x8/16x16/32x32 */
    i32 smart_hevc_lum_dc_th[3];
    i32 smart_hevc_chr_dc_th[3];
    i32 smart_hevc_lum_ac_num_th[3];
    i32 smart_hevc_chr_ac_num_th[3];
    /* back ground */
    i32 smart_mean_th[4];
    /* foreground/background threashold: maximum foreground pixels in background block */
    i32 smart_pix_num_cnt_th;

    /* constant chroma control */
    i32 const_chroma_en;
    u32 const_cb;
    u32 const_cr;

    i32 scene_change[MAX_SCENE_CHANGE];

    /* for tile */
    i32 tiles_enabled_flag;
    i32 num_tile_columns;
    i32 num_tile_rows;
    i32 loop_filter_across_tiles_enabled_flag;

    /*for skip frame encoding ctr */
    i32 skip_frame_enabled_flag;
    i32 skip_frame_poc;

    /*stride */
    u32 exp_of_input_alignment;
    u32 exp_of_ref_alignment;
    u32 exp_of_ref_ch_alignment;

    /* HDR10 */
    u32 hdr10_display_enable;
    u32 hdr10_dx0;
    u32 hdr10_dy0;
    u32 hdr10_dx1;
    u32 hdr10_dy1;
    u32 hdr10_dx2;
    u32 hdr10_dy2;
    u32 hdr10_wx;
    u32 hdr10_wy;
    u32 hdr10_maxluma;
    u32 hdr10_minluma;

    u32 hdr10_lightlevel_enable;
    u32 hdr10_maxlight;
    u32 hdr10_avglight;

    u32 hdr10_color_enable;
    u32 hdr10_primary;
    u32 hdr10_transfer;
    u32 hdr10_matrix;

    u32 rps_in_slice_header;
    u32 p010_ref_enable;
    u32 vui_timing_info_enable;

    u32 pic_order_cnt_type;
    u32 log2_max_pic_order_cnt_lsb;
    u32 log2_max_frame_num;

    u32 cutree_blkratio;
    i16 gmv[2][2];
    char *gmv_file_name[2];
    char *half_ds_input;

    u32 parallel_core_num;

    u32 dump_register;
    u32 rasterscan;

    u32 stream_buf_chain;
    u32 lookahead_depth;
    u32 stream_multi_segment_mode;
    u32 stream_multi_segment_amount;

    /*add for transcode*/
    i32 enc_index;
    void *trans_handle;

    /*add for new driver*/
#ifdef DRV_NEW_ARCH
    int priority;
    char *device;
    int mem_id;
#endif

} VPIH26xEncOptions;

#define NOCARE (-255)
#define COM_SHORT '0'

#define ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))

typedef enum {
    TYPE_NOARG,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_COLON2,
    TYPE_COLON4,
} EnumOptionType;

enum AVOptionType {
    AV_OPT_TYPE_FLAGS,
    AV_OPT_TYPE_INT,
    AV_OPT_TYPE_INT64,
    AV_OPT_TYPE_DOUBLE,
    AV_OPT_TYPE_FLOAT,
    AV_OPT_TYPE_STRING,
    AV_OPT_TYPE_RATIONAL,
    AV_OPT_TYPE_BINARY, /* offset must point to a pointer immediately followed by an int for the length*/
    AV_OPT_TYPE_DICT,
    AV_OPT_TYPE_UINT64,
    AV_OPT_TYPE_CONST,
    AV_OPT_TYPE_IMAGE_SIZE, /*offset must point to two consecutive integers*/
    AV_OPT_TYPE_PIXEL_FMT,
    AV_OPT_TYPE_SAMPLE_FMT,
    AV_OPT_TYPE_VIDEO_RATE, /* offset must point to AVRational*/
    AV_OPT_TYPE_DURATION,
    AV_OPT_TYPE_COLOR,
    AV_OPT_TYPE_CHANNEL_LAYOUT,
    AV_OPT_TYPE_BOOL,
};

typedef struct {
    char *name;
    char short_name;
    EnumOptionType type;
    int min;
    int max;
    int offset;
    /*int offset1;*/

    union {
        int64_t i64;
        float dbl;
        const char *str;
        struct {
            int32_t min;
            int32_t max;
        } colon2;
    } default_val;

    u32 flag;
    char *help;
} VPIH26xParamsDef;

#endif /* __VPI_VIDEO_H26XENC_OPTIONS_H__ */
