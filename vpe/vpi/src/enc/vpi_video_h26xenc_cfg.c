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
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include "instance.h"
#include "dectypes.h"

#ifndef USE_OLD_DRV
#ifdef DRV_NEW_ARCH
#include "transcoder.h"
#endif
#endif

#include "vpi_log.h"
#include "vpi.h"
#include "vpi_video_h26xenc_utils.h"
#include "vpi_video_h26xenc.h"
#include "vpi_video_enc_common.h"
#define MAX_GOP_LEN 300

extern u32 getEWLMallocInoutSize(u32 alignment, u32 in_size);
extern int h26x_enc_get_params(struct VpiH26xEncCtx *vpi_h26xe_ctx);
extern int h26x_enc_get_params_from_cmd(struct VpiH26xEncCtx *vpi_h26xe_ctx,
                                        const char *name,
                                        const char *input_value);

static int h26x_enc_get_options(struct VpiH26xEncCtx *vpi_h26xe_ctx)
{
    VPIH26xEncOptions *options = &vpi_h26xe_ctx->options;

    /* set other default params */
    if (options->rdo_level == DEFAULT)
        options->rdo_level = 1;
    if (options->gop_size == DEFAULT)
        options->gop_size = 0;
    if (options->lookahead_depth == DEFAULT)
        options->lookahead_depth = 0;

    if (options->lookahead_depth) {
        options->roi_map_delta_qp_enable = 1;
        options->roi_map_delta_qp_block_unit =
            MAX(1, options->roi_map_delta_qp_block_unit);
    }
#if 0
    VPILOGE("+++ h26x_enc_get_options\n");
    VPILOGE("+++ options->rdo_level = %d\n", options->rdo_level);
    VPILOGE("+++ options->bit_per_second = %d\n", options->bit_per_second);
    VPILOGE("+++ options->input_rate_numer = %d\n",
            options->input_rate_numer);
    VPILOGE("+++ options->input_rate_denom = %d\n",
            options->input_rate_denom);
    VPILOGE("+++ options->output_rate_numer = %d\n",
            options->output_rate_numer);
    VPILOGE("+++ options->output_rate_denom = %d\n",
            options->output_rate_denom);
    VPILOGE("+++ options->intra_pic_rate = %d\n", options->intra_pic_rate);
    VPILOGE("+++ options->bitrate_window = %d\n", options->bitrate_window);
    VPILOGE("+++ options->intra_qp_delta = %d\n", options->intra_qp_delta);
    VPILOGE("+++ options->qp_hdr = %d\n", options->qp_hdr);
    VPILOGE("+++ options->qp_min = %d\n", options->qp_min);
    VPILOGE("+++ options->qp_max = %d\n", options->qp_max);
    VPILOGE("+++ options->fixed_intra_qp = %d\n", options->fixed_intra_qp);

    VPILOGE("+++ options->pic_skip = %d\n", options->pic_skip);
    VPILOGE("+++ options->profile = %d\n", options->profile);
    VPILOGE("+++ options->level = %d\n", options->level);

    VPILOGE("+++ options->tier = %d\n", options->tier);
    VPILOGE("+++ options->exp_of_input_alignment  = %d\n",
            options->exp_of_input_alignment);
    VPILOGE("+++ options->exp_of_ref_alignment    = %d\n",
            options->exp_of_ref_alignment);
    VPILOGE("+++ options->exp_of_ref_ch_alignment = %d\n",
            options->exp_of_ref_ch_alignment);

    VPILOGE("+++ options->byte_stream = %d\n", options->byte_stream);
    VPILOGE("+++ options->video_range = %d\n", options->video_range);
    VPILOGE("+++ options->chroma_qp_offset = %d\n",
            options->chroma_qp_offset);
    VPILOGE("+++ options->gop_size = %d\n", options->gop_size);
    VPILOGE("+++ options->lookahead_depth = %d\n",
            options->lookahead_depth);
#endif
    return 0;
}

static int h26x_enc_set_default_opt(struct VpiH26xEncCtx *vpi_h26xe_ctx,
                                    H26xEncCfg *h26x_enc_cfg)
{
    VPIH26xEncOptions *options = &vpi_h26xe_ctx->options;

    int i;

    options->first_pic = 0;
    options->last_pic  = 0x7fffffff;
    options->input_rate_numer =
        h26x_enc_cfg->input_rate_numer; /*avctx->framerate.num;*/
    options->input_rate_denom =
        h26x_enc_cfg->input_rate_denom; /*avctx->framerate.den;*/
    VPILOGE("options->input_rate_numer = %d, options->input_rate_denom = %d\n",
            options->input_rate_numer, options->input_rate_denom);

    options->output_rate_numer = DEFAULT;
    options->output_rate_denom = DEFAULT;

    options->lum_width_src  = h26x_enc_cfg->lum_width_src; /*avctx->width;*/
    options->lum_height_src = h26x_enc_cfg->lum_height_src; /*avctx->height;*/
    options->hor_offset_src = DEFAULT;
    options->ver_offset_src = DEFAULT;
    options->rotation       = 0;
    switch (h26x_enc_cfg->input_format) {
    case VPI_YUV420_PLANAR:
        options->input_format = VCENC_YUV420_PLANAR;
        break;

    case VPI_YUV420_SEMIPLANAR:
        options->input_format = VCENC_YUV420_SEMIPLANAR;
        break;

    case VPI_YUV420_SEMIPLANAR_VU:
        options->input_format = VCENC_YUV420_SEMIPLANAR_VU;
        break;

    case VPI_YUV420_PLANAR_10BIT_P010:
        options->input_format = VCENC_YUV420_PLANAR_10BIT_P010;
        break;

    default:
        options->input_format = VCENC_YUV420_PLANAR;
        break;
    }
    options->format_customized_type = -1;
    options->width                  = DEFAULT;
    options->height                 = DEFAULT;
    options->max_cu_size            = 64;
    options->min_cu_size            = 8;
    options->max_tr_size            = 16;
    options->min_tr_size            = 4;
    options->tr_depth_intra         = 2; /*mfu =>0*/
    options->tr_depth_inter         = (options->max_cu_size == 64) ? 4 : 3;

    if (!strcmp(h26x_enc_cfg->codec_name, "hevcenc_vpe")) {
        options->codec_format = VCENC_VIDEO_CODEC_HEVC;
    } else if (!strcmp(h26x_enc_cfg->codec_name, "h264enc_vpe")) {
        options->codec_format   = VCENC_VIDEO_CODEC_H264;
        options->max_cu_size    = 16;
        options->min_cu_size    = 8;
        options->max_tr_size    = 16;
        options->min_tr_size    = 4;
        options->tr_depth_intra = 1;
        options->tr_depth_inter = 2;
    }

    options->bit_per_second                      = DEFAULT;
    options->tol_moving_bitrate                  = 2000;
    options->monitor_frames                      = DEFAULT;
    options->u32_static_scene_ibit_percent       = 80;
    options->disable_deblocking                  = 0;
    options->tc_offset                           = -2;
    options->beta_offset                         = 5;
    options->smooth_psnr_in_gop                  = 0;
    options->enable_sao                          = 1;
    options->strong_intra_smoothing_enabled_flag = 0;
    options->pcm_loop_filter_disabled_flag       = 0;
    options->intra_area_left                     = options->intra_area_right =
        options->intra_area_top                  = options->intra_area_bottom =
            -1; /* Disabled */
    options->ipcm1_area_left    = options->ipcm1_area_right =
        options->ipcm1_area_top = options->ipcm1_area_bottom =
            -1; /* Disabled */
    options->ipcm2_area_left    = options->ipcm2_area_right =
        options->ipcm2_area_top = options->ipcm2_area_bottom =
            -1; /* Disabled */
    options->gdr_duration                  = 0;
    options->cabac_init_flag               = 0;
    options->cir_start                     = 0;
    options->cir_interval                  = 0;
    options->enable_deblock_override       = 0;
    options->deblock_override              = 0;
    options->enable_scaling_list           = 0;
    options->compressor                    = 0;
    options->level                         = DEFAULT;
    options->profile                       = DEFAULT;
    options->bit_depth_luma                = DEFAULT;
    options->bit_depth_chroma              = DEFAULT;
    options->block_rc_size                 = DEFAULT;
    options->rc_qp_delta_range             = DEFAULT;
    options->rc_base_mb_complexity         = DEFAULT;
    options->pic_qp_delta_max              = DEFAULT;
    options->long_term_gap                 = 0;
    options->long_term_gap_offset          = 0;
    options->long_term_qp_delta            = 0;
    options->ltr_interval                  = DEFAULT;
    options->out_recon_frame               = 1;
    options->roi_map_delta_qp_block_unit   = 0;
    options->roi_map_delta_qp_enable       = 0;
    options->roi_map_delta_qp_file         = NULL;
    options->roi_map_delta_qp_bin_file     = NULL;
    options->roi_map_info_bin_file         = NULL;
    options->roimap_cu_ctrl_info_bin_file  = NULL;
    options->roimap_cu_ctrl_index_bin_file = NULL;
    options->roi_cu_ctrl_ver               = 0;
    options->roi_qp_delta_ver              = 1;
    options->ipcm_map_enable               = 0;
    options->ipcm_map_file                 = NULL;
    options->roi1_qp                       = DEFAULT;
    options->roi2_qp                       = DEFAULT;
    options->roi3_qp                       = DEFAULT;
    options->roi4_qp                       = DEFAULT;
    options->roi5_qp                       = DEFAULT;
    options->roi6_qp                       = DEFAULT;
    options->roi7_qp                       = DEFAULT;
    options->roi8_qp                       = DEFAULT;
    options->interlaced_frame              = 0;
    options->noise_reduction_enable        = 0;

    /* low latency */
    options->input_line_buf_mode  = 0;
    options->input_line_buf_depth = DEFAULT;
    options->amount_per_loop_back = 0;

    /*stride */
    options->exp_of_input_alignment  = 4;
    options->exp_of_ref_alignment    = 0;
    options->exp_of_ref_ch_alignment = 0;

    options->multimode = 0;

    for (i = 0; i < MAX_STREAMS; i++)
        options->streamcfg[i] = NULL;

    options->enable_output_cu_info = 0;
    options->p010_ref_enable       = 0;
    options->hashtype              = 0;
    options->verbose               = 0;

    /* smart */
    options->smart_mode_enable           = 0;
    options->smart_h264_lum_dc_th        = 5;
    options->smart_h264_cb_dc_th         = 1;
    options->smart_h264_cr_dc_th         = 1;
    options->smart_hevc_lum_dc_th[0]     = 2;
    options->smart_hevc_lum_dc_th[1]     = 2;
    options->smart_hevc_lum_dc_th[2]     = 2;
    options->smart_hevc_chr_dc_th[0]     = 2;
    options->smart_hevc_chr_dc_th[1]     = 2;
    options->smart_hevc_chr_dc_th[2]     = 2;
    options->smart_hevc_lum_ac_num_th[0] = 12;
    options->smart_hevc_lum_ac_num_th[1] = 51;
    options->smart_hevc_lum_ac_num_th[2] = 204;
    options->smart_hevc_chr_ac_num_th[0] = 3;
    options->smart_hevc_chr_ac_num_th[1] = 12;
    options->smart_hevc_chr_ac_num_th[2] = 51;
    options->smart_h264_qp               = 30;
    options->smart_hevc_lum_qp           = 30;
    options->smart_hevc_chr_qp           = 30;
    options->smart_mean_th[0]            = 5;
    options->smart_mean_th[1]            = 5;
    options->smart_mean_th[2]            = 5;
    options->smart_mean_th[3]            = 5;
    options->smart_pix_num_cnt_th        = 0;

    /* constant chroma control */
    options->const_chroma_en = 0;
    options->const_cb        = DEFAULT;
    options->const_cr        = DEFAULT;

    for (i = 0; i < MAX_SCENE_CHANGE; i++)
        options->scene_change[i] = 0;

    options->tiles_enabled_flag                    = 0;
    options->num_tile_columns                      = 1;
    options->num_tile_rows                         = 1;
    options->loop_filter_across_tiles_enabled_flag = 1;

    options->skip_frame_enabled_flag = 0;
    options->skip_frame_poc          = 0;

    /* HDR10 */
    options->hdr10_display_enable = 0;
    options->hdr10_dx0            = 0;
    options->hdr10_dy0            = 0;
    options->hdr10_dx1            = 0;
    options->hdr10_dy1            = 0;
    options->hdr10_dx2            = 0;
    options->hdr10_dy2            = 0;
    options->hdr10_wx             = 0;
    options->hdr10_wy             = 0;
    options->hdr10_maxluma        = 0;
    options->hdr10_minluma        = 0;

    options->hdr10_lightlevel_enable = 0;
    options->hdr10_maxlight          = 0;
    options->hdr10_avglight          = 0;

    options->hdr10_color_enable = 0;
    options->hdr10_primary      = 9;
    options->hdr10_transfer     = 0;
    options->hdr10_matrix       = 9;

    options->pic_order_cnt_type         = 0;
    options->log2_max_pic_order_cnt_lsb = 16;
    options->log2_max_frame_num         = 12;

    options->rps_in_slice_header    = 0;
    options->ssim                   = 1;
    options->vui_timing_info_enable = 1;
    options->cutree_blkratio        = 0;

    /* skip mode */
    options->skip_map_enable     = 0;
    options->skip_map_file       = NULL;
    options->skip_map_block_unit = 0;

    /* Frame level core parallel option */
    options->parallel_core_num = 1;

    /*add for transcode*/
    options->enc_index    = 0;
    options->trans_handle = NULL;

    /*add for new driver*/
#ifdef DRV_NEW_ARCH
    options->priority = 1; /*0-live, 1-vod*/
    options->device   = "/dev/transcoder0";
    options->mem_id   = -1;
#endif

    /* two stream buffer */
    options->stream_buf_chain = 0;

    /*multi-segment of stream buffer */
    options->stream_multi_segment_mode   = 0;
    options->stream_multi_segment_amount = 4;

    /*dump register */
    options->dump_register = 0;

    options->rasterscan = 0;
    /*options->lookahead_depth = 0;
      options->crf = -1; */
    return 0;
}

static int h26x_enc_profile_check(enum VpiH26xCodecID codec, const char *codec_name,
                                  char *profile, VPIH26xEncOptions *options)
{
    if (codec == CODEC_ID_HEVC) {
        if (strcmp(profile, "main") == 0) {
            options->profile = VCENC_HEVC_MAIN_PROFILE;
        } else if (strcmp(profile, "still") == 0) {
            options->profile = VCENC_HEVC_MAIN_STILL_PICTURE_PROFILE;
        } else if (strcmp(profile, "main10") == 0) {
            options->profile = VCENC_HEVC_MAIN_10_PROFILE;
        } else {
            VPILOGE("unknow vce profile %s for %s\n", profile, codec_name);
            return -1;
        }
    } else if (codec == CODEC_ID_H264) {
        if (strcmp(profile, "base") == 0) {
            options->profile = VCENC_H264_BASE_PROFILE;
        } else if (strcmp(profile, "main") == 0) {
            options->profile = VCENC_H264_MAIN_PROFILE;
        } else if (strcmp(profile, "high") == 0) {
            options->profile = VCENC_H264_HIGH_PROFILE;
        } else if (strcmp(profile, "high10") == 0) {
            options->profile = VCENC_H264_HIGH_10_PROFILE;
        } else {
            VPILOGE("unknow vce profile %s for %s\n", profile, codec_name);
            return -1;
        }
    }

    return 0;
}

static int h26x_enc_level_check(enum VpiH26xCodecID codec, const char *codec_name,
                                char *level, VPIH26xEncOptions *options)
{
    if (codec == CODEC_ID_HEVC) {
        if (strcmp(level, "1") == 0) {
            options->level = VCENC_HEVC_LEVEL_1;
        } else if (strcmp(level, "2") == 0) {
            options->level = VCENC_HEVC_LEVEL_2;
        } else if (strcmp(level, "2.1") == 0) {
            options->level = VCENC_HEVC_LEVEL_2_1;
        } else if (strcmp(level, "3") == 0) {
            options->level = VCENC_HEVC_LEVEL_3;
        } else if (strcmp(level, "3.1") == 0) {
            options->level = VCENC_HEVC_LEVEL_3_1;
        } else if (strcmp(level, "4") == 0) {
            options->level = VCENC_HEVC_LEVEL_4;
        } else if (strcmp(level, "4.1") == 0) {
            options->level = VCENC_HEVC_LEVEL_4_1;
        } else if (strcmp(level, "5") == 0) {
            options->level = VCENC_HEVC_LEVEL_5;
        } else if (strcmp(level, "5.1") == 0) {
            options->level = VCENC_HEVC_LEVEL_5_1;
        } else {
            VPILOGE("unsupported vce level %s for %s\n", level, codec_name);
            return -1;
        }
    } else if (codec == CODEC_ID_H264) {
        if (strcmp(level, "1") == 0) {
            options->level = VCENC_H264_LEVEL_1;
        } else if (strcmp(level, "1b") == 0) {
            options->level = VCENC_H264_LEVEL_1_b;
        } else if (strcmp(level, "1.1") == 0) {
            options->level = VCENC_H264_LEVEL_1_1;
        } else if (strcmp(level, "1.2") == 0) {
            options->level = VCENC_H264_LEVEL_1_2;
        } else if (strcmp(level, "1.3") == 0) {
            options->level = VCENC_H264_LEVEL_1_3;
        } else if (strcmp(level, "2") == 0) {
            options->level = VCENC_H264_LEVEL_2;
        } else if (strcmp(level, "2.1") == 0) {
            options->level = VCENC_H264_LEVEL_2_1;
        } else if (strcmp(level, "2.2") == 0) {
            options->level = VCENC_H264_LEVEL_2_2;
        } else if (strcmp(level, "3") == 0) {
            options->level = VCENC_H264_LEVEL_3;
        } else if (strcmp(level, "3.1") == 0) {
            options->level = VCENC_H264_LEVEL_3_1;
        } else if (strcmp(level, "3.2") == 0) {
            options->level = VCENC_H264_LEVEL_3_2;
        } else if (strcmp(level, "4") == 0) {
            options->level = VCENC_H264_LEVEL_4;
        } else if (strcmp(level, "4.1") == 0) {
            options->level = VCENC_H264_LEVEL_4_1;
        } else if (strcmp(level, "4.2") == 0) {
            options->level = VCENC_H264_LEVEL_4_2;
        } else if (strcmp(level, "5") == 0) {
            options->level = VCENC_H264_LEVEL_5;
        } else if (strcmp(level, "5.1") == 0) {
            options->level = VCENC_H264_LEVEL_5_1;
        } else if (strcmp(level, "5.2") == 0) {
            options->level = VCENC_H264_LEVEL_5_2;
        } else {
            VPILOGE("unsupported vce level %s for %s. \n", level, codec_name);
            return -1;
        }
    }

    return 0;
}

static int h26x_enc_get_profile_and_level(struct VpiH26xEncCtx *vpi_h26xe_ctx,
                                          H26xEncCfg *h26x_enc_cfg)
{
    VPIH26xEncOptions *options = &vpi_h26xe_ctx->options;

    if (h26x_enc_cfg->profile) {
        h26x_enc_profile_check(h26x_enc_cfg->codec_id, h26x_enc_cfg->codec_name,
                               h26x_enc_cfg->profile, options);
    }

    if (h26x_enc_cfg->level) {
        h26x_enc_level_check(h26x_enc_cfg->codec_id, h26x_enc_cfg->codec_name,
                             h26x_enc_cfg->level, options);
    }

    return 0;
}

static int h26x_enc_set_vceparam(struct VpiH26xEncCtx *vpi_h26xe_ctx,
                                 H26xEncCfg *h26x_enc_cfg)
{
    VPIH26xEncOptions *options = &vpi_h26xe_ctx->options;

    VPILOGE("vpi_h26xe_ctx->pp_index = %d\n", vpi_h26xe_ctx->pp_index);
    VPILOGE("pic %dx%d\n",
            h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index].width,
            h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index].height);

    if (h26x_enc_get_profile_and_level(vpi_h26xe_ctx, h26x_enc_cfg) != 0) {
        return -1;
    }

    if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
            .picdata.is_interlaced == 0) {
        if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index].width <
            h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index].pic_width) {
            options->lum_width_src =
                h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                    .pic_width;
        } else {
            options->lum_width_src =
                NEXT_MULTIPLE(h26x_enc_cfg->frame_ctx
                                  ->pic_info[vpi_h26xe_ctx->pp_index]
                                  .width,
                              4);
        }
    } else {
        /*10bit may have issue, but h264 don't support 10bit */
        options->lum_width_src =
            h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                .picdata.pic_stride;
    }
    if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index].height <
        h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index].pic_height) {
        options->lum_height_src =
            h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index].pic_height;
    } else {
        options->lum_height_src =
            NEXT_MULTIPLE(h26x_enc_cfg->frame_ctx
                              ->pic_info[vpi_h26xe_ctx->pp_index]
                              .height,
                          4);
    }
    VPILOGE("lumSrc %dx%d\n", options->lum_width_src, options->lum_height_src);

    if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index].width !=
        options->lum_width_src) {
        options->width =
            h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index].width;
    }

    if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index].height !=
        options->lum_height_src) {
        options->height =
            h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index].height;
    }
    VPILOGE("res %dx%d\n", options->width, options->height);

    if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
            .picdata.is_interlaced == 0) {
        if ((vpi_h26xe_ctx->pp_index == VPI_DEC_OUT_RFC) ||
            (vpi_h26xe_ctx->pp_index == VPI_DEC_OUT_PP0)) {
            if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                    .picdata.crop_out_width) {
                if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                        .picdata.crop_out_width <
                    h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                        .width) {
                    options->width =
                        NEXT_MULTIPLE(h26x_enc_cfg->frame_ctx
                                          ->pic_info[vpi_h26xe_ctx->pp_index]
                                          .picdata.crop_out_width,
                                      2);
                }
            }
            if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                    .picdata.crop_out_height) {
                if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                        .picdata.crop_out_height <
                    h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                        .height) {
                    options->height =
                        NEXT_MULTIPLE(h26x_enc_cfg->frame_ctx
                                          ->pic_info[vpi_h26xe_ctx->pp_index]
                                          .picdata.crop_out_height,
                                      2);
                }
            }
        }
    } else {
        if ((vpi_h26xe_ctx->pp_index == VPI_DEC_OUT_RFC) ||
            (vpi_h26xe_ctx->pp_index == VPI_DEC_OUT_PP0)) {
            if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                    .picdata.crop_out_width < options->lum_width_src) {
                options->width =
                    NEXT_MULTIPLE(h26x_enc_cfg->frame_ctx
                                      ->pic_info[vpi_h26xe_ctx->pp_index]
                                      .picdata.crop_out_width,
                                  2);
            }
            if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                    .picdata.crop_out_height < options->lum_height_src) {
                options->height =
                    NEXT_MULTIPLE(h26x_enc_cfg->frame_ctx
                                      ->pic_info[vpi_h26xe_ctx->pp_index]
                                      .picdata.crop_out_height,
                                  2);
            }
        }
    }
    VPILOGE("+++ res %dx%d\n", options->width, options->height);

    switch (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                .picdata.pic_format) {
    case DEC_OUT_FRM_TILED_4X4:
        options->input_format = VCENC_YUV420_SEMIPLANAR_8BIT_FB;
        break;
    case DEC_OUT_FRM_YUV420TILE_P010:
        options->input_format = VCENC_YUV420_PLANAR_10BIT_P010_FB;
        break;
    case DEC_OUT_FRM_YUV420TILE_PACKED:
        options->input_format = VCENC_YUV420_UV_8BIT_TILE_64_4;
        break;
    default:
        options->input_format = VCENC_YUV420_SEMIPLANAR_8BIT_FB;
        break;
    }

    options->bit_depth_luma =
        h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
            .picdata.bit_depth_luma;
    options->bit_depth_chroma =
        h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
            .picdata.bit_depth_chroma;
    options->p010_ref_enable = 1;

    switch (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                .picdata.pic_pixformat) {
    case DEC_OUT_PIXEL_DEFAULT: /*reference frame with no compress and pp out 8bit is this pixel format*/
#ifdef SUPPORT_DEC400
        if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                    .picdata.bit_depth_luma > 8 ||
            h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                    .picdata.bit_depth_chroma > 8) {
            if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                    .picdata.pic_compressed_status > 0) {
                return -1;
            } else {
                options->input_format = VCENC_YUV420_PLANAR_10BIT_P010_FB;
                if (options->codec_format == VCENC_VIDEO_CODEC_H264) {
                    options->profile = VCENC_H264_HIGH_10_PROFILE;
                } else if (options->codec_format == VCENC_VIDEO_CODEC_HEVC) {
                    options->profile = VCENC_HEVC_MAIN_10_PROFILE;
                }
            }
        } else {
            if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                    .picdata.pic_format > DEC_OUT_FRM_NV21TILE) {
                if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                        .picdata.pic_compressed_status == 2) {
                    options->input_format =
                        INPUT_FORMAT_YUV420_SEMIPLANAR_VU_8BIT_COMPRESSED_FB;
                } else if (h26x_enc_cfg->frame_ctx
                               ->pic_info[vpi_h26xe_ctx->pp_index]
                               .picdata.pic_compressed_status == 0) {
                    options->input_format = VCENC_YUV420_SEMIPLANAR_VU_8BIT_FB;
                } else {
                    return -1;
                }
            } else if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                           .picdata.pic_format == DEC_OUT_FRM_RASTER_SCAN) {
                options->input_format = INPUT_FORMAT_PP_YUV420_SEMIPLANNAR;
            } else {
                if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                        .picdata.pic_compressed_status == 2) {
                    options->input_format =
                        INPUT_FORMAT_YUV420_SEMIPLANAR_8BIT_COMPRESSED_FB;
                } else if (h26x_enc_cfg->frame_ctx
                               ->pic_info[vpi_h26xe_ctx->pp_index]
                               .picdata.pic_compressed_status == 0) {
                    options->input_format = VCENC_YUV420_SEMIPLANAR_8BIT_FB;
                } else {
                    return -1;
                }
            }
        }
#else
        if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                    .picdata.bit_depth_luma > 8 ||
            h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                    .picdata.bit_depth_chroma > 8) {
            options->input_format = VCENC_YUV420_PLANAR_10BIT_P010_FB;
            if (options->codec_format == VCENC_VIDEO_CODEC_H264) {
                options->profile = VCENC_H264_HIGH_10_PROFILE;
            } else if (options->codec_format == VCENC_VIDEO_CODEC_HEVC) {
                options->profile = VCENC_HEVC_MAIN_10_PROFILE;
            }
        } else if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                       .picdata.pic_format == DEC_OUT_FRM_RASTER_SCAN) {
            options->input_format = INPUT_FORMAT_PP_YUV420_SEMIPLANNAR;
        } else {
        }
#endif
        break;
    case DEC_OUT_PIXEL_CUT_8BIT:
        options->bit_depth_luma   = 8;
        options->bit_depth_chroma = 8;
        break;
    case DEC_OUT_PIXEL_P010: /*pp out 10bit is this pixel format*/
#ifdef SUPPORT_DEC400
        if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                .picdata.pic_compressed_status == 2) {
            options->input_format =
                INPUT_FORMAT_YUV420_PLANAR_10BIT_P010_COMPRESSED_FB;
        } else if (h26x_enc_cfg->frame_ctx->pic_info[vpi_h26xe_ctx->pp_index]
                       .picdata.pic_compressed_status == 0) {
            options->input_format = VCENC_YUV420_PLANAR_10BIT_P010_FB;
        } else {
            return -1;
        }
#else
        options->input_format = VCENC_YUV420_PLANAR_10BIT_P010_FB;
#endif
        if (options->codec_format == VCENC_VIDEO_CODEC_H264) {
            options->profile = VCENC_H264_HIGH_10_PROFILE;
        } else if (options->codec_format == VCENC_VIDEO_CODEC_HEVC) {
            options->profile = VCENC_HEVC_MAIN_10_PROFILE;
        }
        break;
    case DEC_OUT_PIXEL_RFC:
        VPILOGE("should not get here! RFC output disabled\n");
        return -1;
        break;
    default:
        break;
    }

    if (options->bit_depth_luma > 8 || options->bit_depth_chroma > 8) {
        if (options->force_8bit) {
            options->bit_depth_luma   = 8;
            options->bit_depth_chroma = 8;
            options->profile          = 0;
        } else if (options->bitdepth != DEFAULT_VALUE) {
            if (options->bitdepth == 0) {
                options->bit_depth_luma   = 8;
                options->bit_depth_chroma = 8;
                options->profile          = 0;
            }
        }
    }

    options->exp_of_ref_alignment    = 10;
    options->exp_of_ref_ch_alignment = 10;
    options->compressor              = 2;

    /* get default bitrate or limit bitrate */
    {
        u32 width, height;
        width = (options->width == DEFAULT_VALUE) ? options->lum_width_src :
                                                    options->width;
        height = (options->height == DEFAULT_VALUE) ? options->lum_height_src :
                                                      options->height;

        {
            u32 original_bits_perframe;
            options->bit_per_second = h26x_enc_cfg->bit_per_second;
            original_bits_perframe  = (width * height * 3) / 2;
            original_bits_perframe *= 8;
            if (options->bit_depth_luma > 8) {
                original_bits_perframe *= 2;
            }
            if (options->bit_per_second > original_bits_perframe / 2) {
                options->bit_per_second = original_bits_perframe / 2;
                options->bit_per_second =
                    ((options->bit_per_second + 100000 - 1) / 100000) * 100000;
                VPILOGE("limit bitrate to %d\n", options->bit_per_second);
            }
        }
    }

    if (options->enc_index == 0 && options->lookahead_depth) {
        if (vpi_h26xe_ctx->pp_index != 0 && vpi_h26xe_ctx->pp_index != 1) {
            VPILOGE("enc_index not match with pp_index\n");
            return -1;
        }
        if (h26x_enc_cfg->frame_ctx->pic_info[2].enabled &&
            h26x_enc_cfg->frame_ctx->pic_info[2].flag) {
            options->cutree_blkratio = 1;
            VPILOGE("enable 1/4 resolution first pass\n");

            if (h26x_enc_cfg->frame_ctx->pic_info[2].picdata.is_interlaced ==
                0) {
                if (h26x_enc_cfg->frame_ctx->pic_info[2].width <
                    h26x_enc_cfg->frame_ctx->pic_info[2].pic_width) {
                    options->lum_widthsrc_ds =
                        h26x_enc_cfg->frame_ctx->pic_info[2].pic_width;
                } else {
                    options->lum_widthsrc_ds =
                        NEXT_MULTIPLE(h26x_enc_cfg->frame_ctx->pic_info[2].width,
                                      4);
                }
            } else {
                /*10bit may have issue, but h264 don't support 10bit */
                options->lum_widthsrc_ds =
                    h26x_enc_cfg->frame_ctx->pic_info[2].picdata.pic_stride;
            }
            if (h26x_enc_cfg->frame_ctx->pic_info[2].height <
                h26x_enc_cfg->frame_ctx->pic_info[2].pic_height) {
                options->lum_heightsrc_ds =
                    h26x_enc_cfg->frame_ctx->pic_info[2].pic_height;
            } else {
                options->lum_heightsrc_ds =
                    NEXT_MULTIPLE(h26x_enc_cfg->frame_ctx->pic_info[2].height,
                                  4);
            }
            VPILOGE("ds lumSrc %dx%d\n", options->lum_widthsrc_ds,
                    options->lum_heightsrc_ds);

            options->width_ds  = h26x_enc_cfg->frame_ctx->pic_info[2].width;
            options->height_ds = h26x_enc_cfg->frame_ctx->pic_info[2].height;

            VPILOGE("ds res %dx%d\n", options->width_ds, options->height_ds);

            switch (h26x_enc_cfg->frame_ctx->pic_info[2].picdata.pic_format) {
            case DEC_OUT_FRM_TILED_4X4:
                options->input_format_ds = VCENC_YUV420_SEMIPLANAR_8BIT_FB;
                break;
            case DEC_OUT_FRM_YUV420TILE_P010:
                options->input_format_ds = VCENC_YUV420_PLANAR_10BIT_P010_FB;
                break;
            case DEC_OUT_FRM_YUV420TILE_PACKED:
                options->input_format_ds = VCENC_YUV420_UV_8BIT_TILE_64_4;
                break;
            default:
                options->input_format_ds = VCENC_YUV420_SEMIPLANAR_8BIT_FB;
                break;
            }

            switch (
                h26x_enc_cfg->frame_ctx->pic_info[2].picdata.pic_pixformat) {
            case DEC_OUT_PIXEL_DEFAULT: /*reference frame with no compress and pp out 8bit is this pixel format*/
#ifdef SUPPORT_DEC400
                if (h26x_enc_cfg->frame_ctx->pic_info[2].picdata.bit_depth_luma >
                        8 ||
                    h26x_enc_cfg->frame_ctx->pic_info[2]
                            .picdata.bit_depth_chroma > 8) {
                    if (h26x_enc_cfg->frame_ctx->pic_info[2]
                            .picdata.pic_compressed_status > 0) {
                        return -1;
                    } else {
                        options->input_format_ds =
                            VCENC_YUV420_PLANAR_10BIT_P010_FB;
                    }
                } else {
                    if (h26x_enc_cfg->frame_ctx->pic_info[2].picdata.pic_format >
                        DEC_OUT_FRM_NV21TILE) {
                        if (h26x_enc_cfg->frame_ctx->pic_info[2]
                                .picdata.pic_compressed_status == 2) {
                            options->input_format_ds =
                                INPUT_FORMAT_YUV420_SEMIPLANAR_VU_8BIT_COMPRESSED_FB;
                        } else if (h26x_enc_cfg->frame_ctx->pic_info[2]
                                       .picdata.pic_compressed_status == 0) {
                            options->input_format_ds =
                                VCENC_YUV420_SEMIPLANAR_VU_8BIT_FB;
                        } else {
                            return -1;
                        }
                    } else if (h26x_enc_cfg->frame_ctx->pic_info[2]
                                   .picdata.pic_format ==
                               DEC_OUT_FRM_RASTER_SCAN) {
                        options->input_format_ds =
                            INPUT_FORMAT_PP_YUV420_SEMIPLANNAR;
                    } else {
                        if (h26x_enc_cfg->frame_ctx->pic_info[2]
                                .picdata.pic_compressed_status == 2) {
                            options->input_format_ds =
                                INPUT_FORMAT_YUV420_SEMIPLANAR_8BIT_COMPRESSED_FB;
                        } else if (h26x_enc_cfg->frame_ctx->pic_info[2]
                                       .picdata.pic_compressed_status == 0) {
                            options->input_format_ds =
                                VCENC_YUV420_SEMIPLANAR_8BIT_FB;
                        } else {
                            return -1;
                        }
                    }
                }
#else
                if (h26x_enc_cfg->frame_ctx->pic_info[2].picdata.bit_depth_luma >
                        8 ||
                    h26x_enc_cfg->frame_ctx->pic_info[2]
                            .picdata.bit_depth_chroma > 8) {
                    options->input_format_ds =
                        VCENC_YUV420_PLANAR_10BIT_P010_FB;
                } else if (h26x_enc_cfg->frame_ctx->pic_info[2]
                               .picdata.pic_format == DEC_OUT_FRM_RASTER_SCAN) {
                    options->input_format_ds =
                        INPUT_FORMAT_PP_YUV420_SEMIPLANNAR;
                } else {
                }
#endif
                break;
            case DEC_OUT_PIXEL_P010: /*pp out 10bit is this pixel format*/
#ifdef SUPPORT_DEC400
                if (h26x_enc_cfg->frame_ctx->pic_info[2]
                        .picdata.pic_compressed_status == 2) {
                    options->input_format_ds =
                        INPUT_FORMAT_YUV420_PLANAR_10BIT_P010_COMPRESSED_FB;
                } else if (h26x_enc_cfg->frame_ctx->pic_info[2]
                               .picdata.pic_compressed_status == 0) {
                    options->input_format_ds =
                        VCENC_YUV420_PLANAR_10BIT_P010_FB;
                } else {
                    return -1;
                }
#else
                options->input_format_ds = VCENC_YUV420_PLANAR_10BIT_P010_FB;
#endif
                if (options->codec_format == VCENC_VIDEO_CODEC_H264) {
                    options->profile = VCENC_H264_HIGH_10_PROFILE;
                    /*pCml->level = VCENC_H264_LEVEL_6_2;*/
                } else if (options->codec_format == VCENC_VIDEO_CODEC_HEVC) {
                    options->profile = VCENC_HEVC_MAIN_10_PROFILE;
                    /*pCml->level = 0;*/
                }
                break;
            case DEC_OUT_PIXEL_RFC:
                VPILOGE("should not get here!\n");
                return -1;
            default:
                break;
            }
        }
    }

    return 0;
}

static int h26x_enc_preset_check(char *input_param, VpiH26xPreset *preset)
{
    if ((input_param == NULL) || (preset == NULL))
        return -1;

    if (strcmp(input_param, "superfast") == 0) {
        *preset = VPE_PRESET_SUPERFAST;
    } else if (strcmp(input_param, "fast") == 0) {
        *preset = VPE_PRESET_FAST;
    } else if (strcmp(input_param, "medium") == 0) {
        *preset = VPE_PRESET_MEDIUM;
    } else if (strcmp(input_param, "slow") == 0) {
        *preset = VPE_PRESET_SLOW;
    } else if (strcmp(input_param, "superslow") == 0) {
        *preset = VPE_PRESET_SUPERSLOW;
    } else {
        VPILOGE("unknow vcepreset %s\n", input_param);
        return -1;
    }

    VPILOGE("+++ preset = %d\n", *preset);

    return 0;
}

static int h26x_enc_set_opt_accord_preset(enum VpiH26xCodecID codec,
                                          VPIH26xEncOptions *options,
                                          VpiH26xPreset preset)
{
    if (codec == CODEC_ID_HEVC) {
        switch (preset) {
        case VPE_PRESET_SUPERFAST:
            if (options->intra_qp_delta == DEFAULT)
                options->intra_qp_delta = -2;
            if (options->qp_hdr == DEFAULT)
                options->qp_hdr = -1;
            if (options->pic_rc == DEFAULT)
                options->pic_rc = 1;
            if (options->ctb_rc == DEFAULT)
                options->ctb_rc = 0;
            if (options->gop_size == DEFAULT)
                options->gop_size = 1;
            if (options->rdo_level == DEFAULT)
                options->rdo_level = 1;
            if (options->lookahead_depth == DEFAULT)
                options->lookahead_depth = 0;
            break;
        case VPE_PRESET_FAST:
            if (options->intra_qp_delta == DEFAULT)
                options->intra_qp_delta = -2;
            if (options->qp_hdr == DEFAULT)
                options->qp_hdr = -1;
            if (options->pic_rc == DEFAULT)
                options->pic_rc = 1;
            if (options->ctb_rc == DEFAULT)
                options->ctb_rc = 0;
            if (options->gop_size == DEFAULT)
                options->gop_size = 4;
            if (options->rdo_level == DEFAULT)
                options->rdo_level = 1;
            if (options->lookahead_depth == DEFAULT)
                options->lookahead_depth = 0;
            break;
        case VPE_PRESET_MEDIUM:
            if (options->intra_qp_delta == DEFAULT)
                options->intra_qp_delta = -2;
            if (options->qp_hdr == DEFAULT)
                options->qp_hdr = -1;
            if (options->pic_rc == DEFAULT)
                options->pic_rc = 1;
            if (options->ctb_rc == DEFAULT)
                options->ctb_rc = 0;
            if (options->gop_size == DEFAULT)
                options->gop_size = 4;
            if (options->rdo_level == DEFAULT)
                options->rdo_level = 1;
            if (options->lookahead_depth == DEFAULT)
                options->lookahead_depth = 20;
            break;
        case VPE_PRESET_SLOW:
            if (options->intra_qp_delta == DEFAULT)
                options->intra_qp_delta = -2;
            if (options->qp_hdr == DEFAULT)
                options->qp_hdr = -1;
            if (options->pic_rc == DEFAULT)
                options->pic_rc = 1;
            if (options->ctb_rc == DEFAULT)
                options->ctb_rc = 0;
            if (options->gop_size == DEFAULT)
                options->gop_size = 0;
            if (options->rdo_level == DEFAULT)
                options->rdo_level = 2;
            if (options->lookahead_depth == DEFAULT)
                options->lookahead_depth = 30;
            break;
        case VPE_PRESET_SUPERSLOW:
            if (options->intra_qp_delta == DEFAULT)
                options->intra_qp_delta = -2;
            if (options->qp_hdr == DEFAULT)
                options->qp_hdr = -1;
            if (options->pic_rc == DEFAULT)
                options->pic_rc = 1;
            if (options->ctb_rc == DEFAULT)
                options->ctb_rc = 0;
            if (options->gop_size == DEFAULT)
                options->gop_size = 0;
            if (options->rdo_level == DEFAULT)
                options->rdo_level = 3;
            if (options->lookahead_depth == DEFAULT)
                options->lookahead_depth = 40;
            break;
        case VPE_PRESET_NONE:
            break;
        default:
            VPILOGE("unknow preset %d\n", preset);
            return -1;
        }

    } else if (codec == CODEC_ID_H264) {
        switch (preset) {
        case VPE_PRESET_SUPERFAST:
            if (options->intra_qp_delta == DEFAULT)
                options->intra_qp_delta = -2;
            if (options->qp_hdr == DEFAULT)
                options->qp_hdr = -1;
            if (options->pic_rc == DEFAULT)
                options->pic_rc = 1;
            if (options->ctb_rc == DEFAULT)
                options->ctb_rc = 0;
            if (options->gop_size == DEFAULT)
                options->gop_size = 1;
            if (options->lookahead_depth == DEFAULT)
                options->lookahead_depth = 0;
            break;
        case VPE_PRESET_FAST:
            if (options->intra_qp_delta == DEFAULT)
                options->intra_qp_delta = -2;
            if (options->qp_hdr == DEFAULT)
                options->qp_hdr = -1;
            if (options->pic_rc == DEFAULT)
                options->pic_rc = 1;
            if (options->ctb_rc == DEFAULT)
                options->ctb_rc = 0;
            if (options->gop_size == DEFAULT)
                options->gop_size = 4;
            if (options->lookahead_depth == DEFAULT)
                options->lookahead_depth = 0;
            break;
        case VPE_PRESET_MEDIUM:
            if (options->intra_qp_delta == DEFAULT)
                options->intra_qp_delta = -2;
            if (options->qp_hdr == DEFAULT)
                options->qp_hdr = -1;
            if (options->pic_rc == DEFAULT)
                options->pic_rc = 1;
            if (options->ctb_rc == DEFAULT)
                options->ctb_rc = 0;
            if (options->gop_size == DEFAULT)
                options->gop_size = 4;
            if (options->lookahead_depth == DEFAULT)
                options->lookahead_depth = 20;
            break;
        case VPE_PRESET_SLOW:
            if (options->intra_qp_delta == DEFAULT)
                options->intra_qp_delta = -2;
            if (options->qp_hdr == DEFAULT)
                options->qp_hdr = -1;
            if (options->pic_rc == DEFAULT)
                options->pic_rc = 1;
            if (options->ctb_rc == DEFAULT)
                options->ctb_rc = 0;
            if (options->gop_size == DEFAULT)
                options->gop_size = 0;
            if (options->lookahead_depth == DEFAULT)
                options->lookahead_depth = 30;
            break;
        case VPE_PRESET_SUPERSLOW:
            if (options->intra_qp_delta == DEFAULT)
                options->intra_qp_delta = -2;
            if (options->qp_hdr == DEFAULT)
                options->qp_hdr = -1;
            if (options->pic_rc == DEFAULT)
                options->pic_rc = 1;
            if (options->ctb_rc == DEFAULT)
                options->ctb_rc = 0;
            if (options->gop_size == DEFAULT)
                options->gop_size = 0;
            if (options->lookahead_depth == DEFAULT)
                options->lookahead_depth = 40;
            break;
        case VPE_PRESET_NONE:
            break;
        default:
            VPILOGE("unknow preset %d\n", preset);
            return -1;
        }
    }

    VPILOGE("%s(%d)options->rdo_level = %d\n", __FUNCTION__, __LINE__,
            options->rdo_level);
    return 0;
}

static int h26x_enc_preset_params_set(struct VpiH26xEncCtx *vpi_h26xe_ctx,
                                      H26xEncCfg *h26x_enc_cfg)
{
    VPIH26xEncOptions *options = &vpi_h26xe_ctx->options;

    VpiH26xPreset preset;
    i32 ret = 0;

    VPILOGE("+++ vcepreset %s\n", h26x_enc_cfg->preset);

    if (h26x_enc_cfg->preset) {
        ret = h26x_enc_preset_check(h26x_enc_cfg->preset,
                                    (VpiH26xPreset *)&preset);
        if (ret != 0) {
            return -1;
        }
        ret = h26x_enc_set_opt_accord_preset(h26x_enc_cfg->codec_id, options,
                                             preset);
    }

    return ret;
}

static void h26x_enc_params_value_print(struct VpiH26xEncCtx *vpi_h26xe_ctx)
{
    VPIH26xEncOptions *options = &vpi_h26xe_ctx->options;

    VPILOGE("^^^ options->force_8bit = %d\n", options->force_8bit);
    VPILOGE("^^^ options->bitdepth = %d\n", options->bitdepth);
    VPILOGE("^^^ options->intra_pic_rate = %d\n", options->intra_pic_rate);
    VPILOGE("^^^ options->bitrate_window = %d\n", options->bitrate_window);
    VPILOGE("^^^ options->intra_qp_delta = %d\n", options->intra_qp_delta);

    VPILOGE("^^^ options->qp_hdr = %d\n", options->qp_hdr);
    VPILOGE("^^^ options->qp_min = %d\n", options->qp_min);
    VPILOGE("^^^ options->qp_max = %d\n", options->qp_max);
    VPILOGE("^^^ options->tier = %d\n", options->tier);
    VPILOGE("^^^ options->byte_stream = %d\n", options->byte_stream);
    VPILOGE("^^^ options->video_range = %d\n", options->video_range);

    VPILOGE("^^^ options->pic_rc = %d\n", options->pic_rc);
    VPILOGE("^^^ options->ctb_rc = %d\n", options->ctb_rc);
    VPILOGE("^^^ options->tol_ctb_rc_inter = %f\n", options->tol_ctb_rc_inter);
    VPILOGE("^^^ options->tol_ctb_rc_intra = %f\n", options->tol_ctb_rc_intra);
    VPILOGE("^^^ options->ctb_rc_row_qp_step = %d\n",
            options->ctb_rc_row_qp_step);
    VPILOGE("^^^ options->hrd_conformance = %d\n", options->hrd_conformance);

    VPILOGE("^^^ options->cpb_size = %d\n", options->cpb_size);
    VPILOGE("^^^ options->gop_size = %d\n", options->gop_size);
    VPILOGE("^^^ options->gop_lowdelay = %d\n", options->gop_lowdelay);

    VPILOGE("^^^ options->qp_min_I = %d\n", options->qp_min_I);
    VPILOGE("^^^ options->qp_max_I = %d\n", options->qp_max_I);
    VPILOGE("^^^ options->bframe_qp_delta = %d\n", options->bframe_qp_delta);

    VPILOGE("^^^ options->chroma_qp_offset = %d\n", options->chroma_qp_offset);
    VPILOGE("^^^ options->vbr = %d\n", options->vbr);
    VPILOGE("^^^ options->user_data = %p\n", options->user_data);

    VPILOGE("^^^ options->const_chroma_en = %d\n", options->const_chroma_en);
    VPILOGE("^^^ options->const_cb = %d\n", options->const_cb);
    VPILOGE("^^^ options->const_cr = %d\n", options->const_cr);

    VPILOGE("^^^ options->rdo_level = %d\n", options->rdo_level);
    VPILOGE("^^^ options->ssim = %d\n", options->ssim);
    VPILOGE("^^^ options->vui_timing_info_enable = %d\n",
            options->vui_timing_info_enable);

    VPILOGE("^^^ options->lookahead_depth = %d\n", options->lookahead_depth);
    VPILOGE("^^^ options->crf = %d\n", options->crf);
}

int h26x_enc_set_options(struct VpiH26xEncCtx *vpi_h26xe_ctx,
                         H26xEncCfg *h26x_enc_cfg)
{
    VPIH26xEncOptions *options = &vpi_h26xe_ctx->options;
    i32 ret                    = 0;

    h26x_enc_set_default_opt(vpi_h26xe_ctx, h26x_enc_cfg);
    if ((vpi_h26xe_ctx->pp_index == VPI_DEC_OUT_RFC) ||
        (vpi_h26xe_ctx->pp_index == VPI_DEC_OUT_PP0)) {
        options->enc_index = 0;
    } else {
        options->enc_index = vpi_h26xe_ctx->pp_index - 1;
    }
    VPILOGE("set options->enc_index = %d, h26x_enc_cfg->frame_ctx->task_id:%d\n",
            options->enc_index, h26x_enc_cfg->frame_ctx->task_id);

    options->priority = h26x_enc_cfg->priority;
    options->device   = h26x_enc_cfg->device;
    options->mem_id   = h26x_enc_cfg->frame_ctx->task_id;

    options->crf = h26x_enc_cfg->crf;

    vpi_h26xe_ctx->h26x_enc_param_table =
        (VPIH26xParamsDef *)&h26x_enc_param_table[0];
    h26x_enc_get_params(vpi_h26xe_ctx);
    h26x_enc_params_value_print(vpi_h26xe_ctx);

    VPILOGD("\nh26x_enc_cfg->enc_params:%s\n",h26x_enc_cfg->enc_params);
    if (h26x_enc_cfg->enc_params) {
        char param_name[ENC_PARAMS_NAME_MAX_LEN]   = { "\0" };
        char param_val[ENC_PARAMS_VALUE_MAX_LEN]   = { "\0" };
        char *str            = h26x_enc_cfg->enc_params;
        while (1) {
            str = vpi_enc_get_paraname_paravalue(str, param_name, param_val);
            if (strlen(param_name) != 0 && strlen(param_val) != 0) {
                VPILOGD("[parame_name:%s,param_val:%s]---\n", param_name, param_val);
                if (h26x_enc_get_params_from_cmd(vpi_h26xe_ctx, param_name,
                                                 param_val) < 0) {
                    VPILOGD("Error parsing option '%s=%s'. Please check if it is valid hantro-params option.\n",
                            param_name, param_val);
                }
                str++;
                if (strlen(str) == 0) {
                    VPILOGD("%s,%d, reach to the end\n", __FUNCTION__,__LINE__);
                    break;
                }
            } else {
                VPILOGE("%s,%d, no valid paramters %s\n", __FUNCTION__,__LINE__, str);
                break;
            }
        }
    }

    h26x_enc_params_value_print(vpi_h26xe_ctx);

    /* preset params set */
    ret = h26x_enc_preset_params_set(vpi_h26xe_ctx, h26x_enc_cfg);
    if (ret < 0) {
        VPILOGE("h26x_enc_preset_params_set error, please check your cmd !\n");
    }

    h26x_enc_get_options(vpi_h26xe_ctx);

    /* we set vce param according first decoded pic */
    ret = h26x_enc_set_vceparam(vpi_h26xe_ctx, h26x_enc_cfg);
    if (ret != 0) {
        VPILOGE("h26x_enc_set_vceparam error!\n");
        return ret;
    }

    /* add for debug */
    VPILOGE("+++ options->profile = %d, options->level = %d\n",
            options->profile, options->level);
    return ret;
}

/**
 *h26x_enc_init_pic_config
 *initial pic reference configure
 */
void h26x_enc_init_pic_config(VCEncIn *p_enc_in)
{
    i32 i, j, k, i32Poc;
    i32 max_pic_order_cnt_lsb = 1 << 16;

    ASSERT(p_enc_in != NULL);

    p_enc_in->gopCurrPicConfig.codingType = FRAME_TYPE_RESERVED;
    p_enc_in->gopCurrPicConfig.numRefPics = NUMREFPICS_RESERVED;
    p_enc_in->gopCurrPicConfig.poc        = -1;
    p_enc_in->gopCurrPicConfig.QpFactor   = QPFACTOR_RESERVED;
    p_enc_in->gopCurrPicConfig.QpOffset   = QPOFFSET_RESERVED;
    p_enc_in->gopCurrPicConfig.temporalId = TEMPORALID_RESERVED;
    p_enc_in->i8SpecialRpsIdx             = -1;
    for (k = 0; k < VCENC_MAX_REF_FRAMES; k++) {
        p_enc_in->gopCurrPicConfig.refPics[k].ref_pic     = INVALITED_POC;
        p_enc_in->gopCurrPicConfig.refPics[k].used_by_cur = 0;
    }

    for (k = 0; k < VCENC_MAX_LT_REF_FRAMES; k++)
        p_enc_in->long_term_ref_pic[k] = INVALITED_POC;

    p_enc_in->bIsPeriodUsingLTR  = HANTRO_FALSE;
    p_enc_in->bIsPeriodUpdateLTR = HANTRO_FALSE;

    for (i = 0; i < p_enc_in->gopConfig.special_size; i++) {
        if (p_enc_in->gopConfig.pGopPicSpecialCfg[i].i32Interval <= 0)
            continue;

        if (p_enc_in->gopConfig.pGopPicSpecialCfg[i].i32Ltr == 0)
            p_enc_in->bIsPeriodUsingLTR = HANTRO_TRUE;
        else {
            p_enc_in->bIsPeriodUpdateLTR = HANTRO_TRUE;

            for (k = 0;
                 k < (i32)p_enc_in->gopConfig.pGopPicSpecialCfg[i].numRefPics;
                 k++) {
                i32 i32LTRIdx =
                    p_enc_in->gopConfig.pGopPicSpecialCfg[i].refPics[k].ref_pic;
                if ((IS_LONG_TERM_REF_DELTAPOC(i32LTRIdx)) &&
                    ((p_enc_in->gopConfig.pGopPicSpecialCfg[i].i32Ltr - 1) ==
                     LONG_TERM_REF_DELTAPOC2ID(i32LTRIdx))) {
                    p_enc_in->bIsPeriodUsingLTR = HANTRO_TRUE;
                }
            }
        }
    }

    memset(p_enc_in->bLTR_need_update, 0,
           sizeof(bool) * VCENC_MAX_LT_REF_FRAMES);
    p_enc_in->bIsIDR = HANTRO_TRUE;

    i32Poc = 0;
    /* check current picture encoded as LTR */
    p_enc_in->u8IdxEncodedAsLTR = 0;
    for (j = 0; j < p_enc_in->gopConfig.special_size; j++) {
        if (p_enc_in->bIsPeriodUsingLTR == HANTRO_FALSE)
            break;

        if ((p_enc_in->gopConfig.pGopPicSpecialCfg[j].i32Interval <= 0) ||
            (p_enc_in->gopConfig.pGopPicSpecialCfg[j].i32Ltr == 0))
            continue;

        i32Poc = i32Poc - p_enc_in->gopConfig.pGopPicSpecialCfg[j].i32Offset;

        if (i32Poc < 0) {
            i32Poc += max_pic_order_cnt_lsb;
            if (i32Poc > (max_pic_order_cnt_lsb >> 1))
                i32Poc = -1;
        }

        if ((i32Poc >= 0) &&
            (i32Poc % p_enc_in->gopConfig.pGopPicSpecialCfg[j].i32Interval ==
             0)) {
            /* more than one LTR at the same frame position */
            if (0 != p_enc_in->u8IdxEncodedAsLTR) {
                /* reuse the same POC LTR*/
                p_enc_in->bLTR_need_update
                    [p_enc_in->gopConfig.pGopPicSpecialCfg[j].i32Ltr - 1] =
                    HANTRO_TRUE;
                continue;
            }

            p_enc_in->gopCurrPicConfig.codingType =
                ((i32)p_enc_in->gopConfig.pGopPicSpecialCfg[j].codingType ==
                 FRAME_TYPE_RESERVED) ?
                    p_enc_in->gopCurrPicConfig.codingType :
                    p_enc_in->gopConfig.pGopPicSpecialCfg[j].codingType;
            p_enc_in->gopCurrPicConfig.numRefPics =
                ((i32)p_enc_in->gopConfig.pGopPicSpecialCfg[j].numRefPics ==
                 NUMREFPICS_RESERVED) ?
                    p_enc_in->gopCurrPicConfig.numRefPics :
                    p_enc_in->gopConfig.pGopPicSpecialCfg[j].numRefPics;
            p_enc_in->gopCurrPicConfig.QpFactor =
                (p_enc_in->gopConfig.pGopPicSpecialCfg[j].QpFactor ==
                 QPFACTOR_RESERVED) ?
                    p_enc_in->gopCurrPicConfig.QpFactor :
                    p_enc_in->gopConfig.pGopPicSpecialCfg[j].QpFactor;
            p_enc_in->gopCurrPicConfig.QpOffset =
                (p_enc_in->gopConfig.pGopPicSpecialCfg[j].QpOffset ==
                 QPOFFSET_RESERVED) ?
                    p_enc_in->gopCurrPicConfig.QpOffset :
                    p_enc_in->gopConfig.pGopPicSpecialCfg[j].QpOffset;
            p_enc_in->gopCurrPicConfig.temporalId =
                (p_enc_in->gopConfig.pGopPicSpecialCfg[j].temporalId ==
                 TEMPORALID_RESERVED) ?
                    p_enc_in->gopCurrPicConfig.temporalId :
                    p_enc_in->gopConfig.pGopPicSpecialCfg[j].temporalId;

            if (((i32)p_enc_in->gopConfig.pGopPicSpecialCfg[j].numRefPics !=
                 NUMREFPICS_RESERVED)) {
                for (k = 0; k < (i32)p_enc_in->gopCurrPicConfig.numRefPics;
                     k++) {
                    p_enc_in->gopCurrPicConfig.refPics[k].ref_pic =
                        p_enc_in->gopConfig.pGopPicSpecialCfg[j]
                            .refPics[k]
                            .ref_pic;
                    p_enc_in->gopCurrPicConfig.refPics[k].used_by_cur =
                        p_enc_in->gopConfig.pGopPicSpecialCfg[j]
                            .refPics[k]
                            .used_by_cur;
                }
            }

            p_enc_in->u8IdxEncodedAsLTR =
                p_enc_in->gopConfig.pGopPicSpecialCfg[j].i32Ltr;
            p_enc_in->bLTR_need_update[p_enc_in->u8IdxEncodedAsLTR - 1] =
                HANTRO_TRUE;
        }
    }
}

int h26x_enc_check_area(VCEncPictureArea *area, VPIH26xEncOptions *options)
{
    i32 w = (options->width + options->max_cu_size - 1) / options->max_cu_size;
    i32 h = (options->height + options->max_cu_size - 1) / options->max_cu_size;

    if ((area->left < (u32)w) && (area->right < (u32)w) &&
        (area->top < (u32)h) && (area->bottom < (u32)h))
        return 1;

    return 0;
}

/**
 *h26x_enc_stream_segment_ready
 *Callback function called by the encoder SW after "segment ready" interrupt
 *from HW. Note that this function is called after every segment is ready.
 */
void h26x_enc_stream_segment_ready(void *cb_data)
{
    u8 *stream_base;
    SegmentCtl *ctl = (SegmentCtl *)cb_data;

    if (ctl->stream_multi_seg_en) {
        stream_base =
            ctl->stream_base +
            (ctl->stream_rd_counter % ctl->segment_amount) * ctl->segment_size;

        if (ctl->output_byte_stream == 0 && ctl->start_code_done == 0) {
            const u8 start_code_prefix[4] = { 0x0, 0x0, 0x0, 0x1 };
            fwrite(start_code_prefix, 1, 4, ctl->out_stream_file);
            ctl->start_code_done = 1;
        }
        VPILOGE("<----receive segment irq %d\n", ctl->stream_rd_counter);
        h26x_enc_write_strm(ctl->out_stream_file, (u32 *)stream_base,
                            ctl->segment_size, 0);

        ctl->stream_rd_counter++;
    }
}

void h26x_enc_report(struct VpiH26xEncCtx *enc_ctx)
{
#ifndef BUILD_CMODEL
    if (enc_ctx != NULL) {
        struct VPIH26xEncCfg *vpi_h26xe_cfg =
            (struct VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;

        statistic enc_statistic = { 0 };
        ENCPERF *perf           = vpi_h26xe_cfg->perf;
        int j                   = 0;

        enc_statistic.frame_count = vpi_h26xe_cfg->picture_enc_cnt -
                                    (vpi_h26xe_cfg->parallel_core_num - 1);
        if (enc_statistic.frame_count && vpi_h26xe_cfg->picture_enc_cnt) {
            if (((vpi_h26xe_cfg->width + 15) / 16) *
                    ((vpi_h26xe_cfg->height + 15) / 16) != 0) {
                enc_statistic.cycle_mb_avg =
                    vpi_h26xe_cfg->hwcycle_acc / enc_statistic.frame_count /
                    (((vpi_h26xe_cfg->width + 15) / 16) *
                     ((vpi_h26xe_cfg->height + 15) / 16));
                enc_statistic.cycle_mb_avg_p1 =
                    perf->hwcycle_accp1 / enc_statistic.frame_count /
                    (((vpi_h26xe_cfg->width + 15) / 16) *
                     ((vpi_h26xe_cfg->height + 15) / 16));
                enc_statistic.cycle_mb_avg_total =
                    perf->hwcycle_acc_total / enc_statistic.frame_count /
                    (((vpi_h26xe_cfg->width + 15) / 16) *
                     ((vpi_h26xe_cfg->height + 15) / 16));
            }
            enc_statistic.ssim_avg =
                vpi_h26xe_cfg->ssim_acc / enc_statistic.frame_count;
            if (vpi_h26xe_cfg->output_rate_denom) {
                enc_statistic.bitrate_avg =
                    (enc_ctx->total_bits * vpi_h26xe_cfg->output_rate_numer) /
                    (enc_statistic.frame_count * vpi_h26xe_cfg->output_rate_denom);
            }
            enc_statistic.core_usage_counts[0] = perf->core_usage_counts[0];
            enc_statistic.core_usage_counts[1] = perf->core_usage_counts[1];
            enc_statistic.core_usage_counts[2] = perf->core_usage_counts_p1[0];
            enc_statistic.core_usage_counts[3] = perf->core_usage_counts_p1[1];
            enc_statistic.total_usage = enc_statistic.core_usage_counts[0] +
                                        enc_statistic.core_usage_counts[1] +
                                        enc_statistic.core_usage_counts[2] +
                                        enc_statistic.core_usage_counts[3];
#ifdef FB_PERFORMANCE_STATIC
            enc_statistic.hw_real_time_avg =
                (PERFORMANCE_STATIC_GET_TOTAL(vpi_h26xe_cfg, perf, vcehw) +
                 PERFORMANCE_STATIC_GET_TOTAL(vpi_h26xe_cfg, perf, vcehwp1)) /
                enc_statistic.frame_count;
            enc_statistic.hw_real_time_avg_remove_overlap =
                PERFORMANCE_STATIC_GET_TOTAL(vpi_h26xe_cfg, perf, vcehw_total) /
                enc_statistic.frame_count;
#endif
            enc_statistic.last_frame_encoded_timestamp =
                perf->last_frame_encoded_timestamp;
        }

        VPILOGE(":::ENC[%d] : %d frames, SSIM %.4f, %d Cycles/MB, %d us/frame, %.2f fps, %u bps\n",
                vpi_h26xe_cfg->enc_index, enc_statistic.frame_count,
                enc_statistic.ssim_avg, enc_statistic.cycle_mb_avg_total,
                enc_statistic.hw_real_time_avg,
                (enc_statistic.hw_real_time_avg == 0) ?
                    0.0 :
                    1000000.0 / ((double)enc_statistic.hw_real_time_avg),
                enc_statistic.bitrate_avg);

        if (enc_statistic.cycle_mb_avg_p1) {
            VPILOGE("\tPass 1 : %d Cycles/MB\n", enc_statistic.cycle_mb_avg_p1);
            VPILOGE("\tPass 2 : %d Cycles/MB\n", enc_statistic.cycle_mb_avg);
        }

        if (enc_statistic.hw_real_time_avg >
            enc_statistic.hw_real_time_avg_remove_overlap + 10) {
            VPILOGE("\tremove overlap : %d us/frame, %.2f fps\n",
                    enc_statistic.hw_real_time_avg_remove_overlap,
                    (enc_statistic.hw_real_time_avg_remove_overlap == 0) ?
                        0.0 :
                        1000000.0 / ((double)enc_statistic
                                         .hw_real_time_avg_remove_overlap));
        }

        VPILOGE(":::ENC[%d] Multi-core usage statistics:\n",
                vpi_h26xe_cfg->enc_index);

        if (enc_statistic.total_usage == 0)
            enc_statistic.total_usage = 1;

        for (j = 0; j < 2; j++) {
            if (enc_statistic.core_usage_counts[2] ||
                enc_statistic.core_usage_counts[3])
                VPILOGE("\tPass 1 Slice[%d] used %6d times (%2d%%)\n", j,
                        enc_statistic.core_usage_counts[2 + j],
                        (enc_statistic.core_usage_counts[2 + j] * 100) /
                            enc_statistic.total_usage);
        }
        for (j = 0; j < 2; j++) {
            VPILOGE("\tSlice[%d] used %6d times (%2d%%)\n", j,
                    enc_statistic.core_usage_counts[j],
                    (enc_statistic.core_usage_counts[j] * 100) /
                        enc_statistic.total_usage);
        }
    }
#endif
}

#define DEVICE_PREFIX "/dev/transcoder"

int h26x_enc_get_deviceId(char *dev)
{
    char *p;
    if (dev) {
        p = strstr(dev, DEVICE_PREFIX);
        if (p) {
            p += strlen(DEVICE_PREFIX);
            return atoi(p);
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

/**
 *h26x_enc_alloc_res
 *Allocation of the physical memories used by both SW and HW:
 *the input pictures and the output stream buffer.
 *NOTE! The implementation uses the EWL instance from the encoder
 *for OS independence. This is not recommended in final environment
 *because the encoder will release the EWL instance in case of error.
 *Instead, the memories should be allocated from the OS the same way
 *as inside EWLMallocLinear().
 */
int h26x_enc_alloc_res(VPIH26xEncOptions *cmdl, VCEncInst enc,
                       struct VPIH26xEncCfg *vpi_h26xe_cfg)
{
    i32 ret;
    u32 picture_size    = 0;
    u32 picture_ds_size = 0;
    u32 stream_buf_total_size;
    u32 outbuf_size[2] = { 0, 0 };
    u32 block_size;
    u32 transform_size = 0;
    u32 luma_size = 0, chroma_size = 0;
    u32 luma_size_ds = 0, chroma_size_ds = 0;
    u32 core_idx = 0;
    i32 i_buf;
    u32 alignment = 0;

    alignment =
        (cmdl->format_customized_type != -1 ? 0 :
                                              vpi_h26xe_cfg->input_alignment);
    get_aligned_pic_size_byformat(cmdl->input_format, cmdl->lum_width_src,
                                  cmdl->lum_height_src, alignment, &luma_size,
                                  &chroma_size, &picture_size);

    vpi_h26xe_cfg->luma_size   = luma_size;
    vpi_h26xe_cfg->chroma_size = chroma_size;

    if (cmdl->lookahead_depth && cmdl->cutree_blkratio) {
        get_aligned_pic_size_byformat(cmdl->input_format_ds,
                                      cmdl->lum_widthsrc_ds,
                                      cmdl->lum_heightsrc_ds, alignment,
                                      &luma_size_ds, &chroma_size_ds,
                                      &picture_ds_size);
        vpi_h26xe_cfg->luma_size_ds   = luma_size_ds;
        vpi_h26xe_cfg->chroma_size_ds = chroma_size_ds;
    }
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
    if (cmdl->input_format ==
            INPUT_FORMAT_YUV420_SEMIPLANAR_8BIT_COMPRESSED_FB ||
        cmdl->input_format ==
            INPUT_FORMAT_YUV420_SEMIPLANAR_VU_8BIT_COMPRESSED_FB ||
        cmdl->input_format ==
            INPUT_FORMAT_YUV420_PLANAR_10BIT_P010_COMPRESSED_FB ||
        cmdl->input_format == INPUT_FORMAT_RFC_8BIT_COMPRESSED_FB ||
        cmdl->input_format == INPUT_FORMAT_RFC_10BIT_COMPRESSED_FB) {
        u32 tbl_luma_size, tbl_chroma_size;

        if (cmdl->input_format == INPUT_FORMAT_RFC_8BIT_COMPRESSED_FB ||
            cmdl->input_format == INPUT_FORMAT_RFC_10BIT_COMPRESSED_FB) {
            int n_cbsg    = (cmdl->lum_width_src + 127) / 8 / 16;
            int n_cbsr    = (cmdl->lum_height_src + 7) / 8;
            tbl_luma_size = 16 * n_cbsg * n_cbsr;

            n_cbsg          = (cmdl->lum_width_src + 255) / 16 / 16;
            n_cbsr          = (cmdl->lum_height_src + 7) / 2 / 4;
            tbl_chroma_size = 16 * n_cbsg * n_cbsr;
        } else {
            tbl_luma_size =
                (((luma_size / vpi_h26xe_cfg->input_alignment) * 2 + 7) / 8 +
                 15) &
                (~15);
            tbl_chroma_size =
                (((chroma_size / vpi_h26xe_cfg->input_alignment) * 2 + 7) / 8 +
                 15) &
                (~15);
        }
        vpi_h26xe_cfg->tbl_luma_size   = tbl_luma_size;
        vpi_h26xe_cfg->tbl_chroma_size = tbl_chroma_size;
    }
#endif

#ifdef SUPPORT_TCACHE
#ifdef USE_OLD_DRV
    ret = EWLMallocLinear(((struct vcenc_instance *)enc)->asic.ewl,
                          256 * sizeof(struct dma_link_table),
                          vpi_h26xe_cfg->input_alignment,
                          &vpi_h26xe_cfg->edma_link_buf);
    if (ret != EWL_OK) {
        vpi_h26xe_cfg->edma_link_buf.virtualAddress = NULL;
        return 1;
    }
#else
    ret = EWLMallocHostLinear(((struct vcenc_instance *)enc)->asic.ewl,
                              256 * sizeof(struct dma_link_table),
                              vpi_h26xe_cfg->input_alignment,
                              &vpi_h26xe_cfg->enc_in.PrivData.edma_link_buf);
    if (ret != EWL_OK) {
        return 1;
    }
    ret =
        EWLMallocHostLinear(((struct vcenc_instance *)enc)->asic.ewl,
                            256 * sizeof(struct dma_link_table),
                            vpi_h26xe_cfg->input_alignment,
                            &vpi_h26xe_cfg->enc_in.PassTwoHWData.edma_link_buf);
    if (ret != EWL_OK) {
        return 1;
    }
#endif
#endif
    if (cmdl->format_customized_type != -1) {
        u32 trans_format    = VCENC_FORMAT_MAX;
        u32 input_alignment = 0;
        switch (cmdl->format_customized_type) {
        case 0:
            if (cmdl->input_format == VCENC_YUV420_PLANAR) {
                if (IS_HEVC(cmdl->codec_format)) /*Dahua hevc*/
                    trans_format = VCENC_YUV420_PLANAR_8BIT_DAHUA_HEVC;
                else /*Dahua h264*/
                    trans_format = VCENC_YUV420_PLANAR_8BIT_DAHUA_H264;
            }
            break;
        case 1: {
            if (cmdl->input_format == VCENC_YUV420_SEMIPLANAR ||
                cmdl->input_format == VCENC_YUV420_SEMIPLANAR_VU)
                trans_format = VCENC_YUV420_SEMIPLANAR_8BIT_FB;
            else if (cmdl->input_format == VCENC_YUV420_PLANAR_10BIT_P010)
                trans_format = VCENC_YUV420_PLANAR_10BIT_P010_FB;
            input_alignment = vpi_h26xe_cfg->input_alignment;
            break;
        }
        case 2:
            trans_format = VCENC_YUV420_SEMIPLANAR_101010;
            break;
        case 3:
        case 4:
            trans_format = VCENC_YUV420_8BIT_TILE_64_4;
            break;
        case 5:
            trans_format = VCENC_YUV420_10BIT_TILE_32_4;
            break;
        case 6:
        case 7:
            trans_format = VCENC_YUV420_10BIT_TILE_48_4;
            break;
        case 8:
        case 9:
            trans_format = VCENC_YUV420_8BIT_TILE_128_2;
            break;
        case 10:
        case 11:
            trans_format = VCENC_YUV420_10BIT_TILE_96_2;
            break;
        default:
            break;
        }
        get_aligned_pic_size_byformat(trans_format, cmdl->lum_width_src,
                                      cmdl->lum_height_src, input_alignment,
                                      NULL, NULL, &transform_size);
        vpi_h26xe_cfg->transformed_size = transform_size;
    }
    /* set vpi_h26xe_cfg->picture_mem_factory[core_idx].size */
    for (core_idx = 0; core_idx < vpi_h26xe_cfg->buffer_cnt; core_idx++) {
#ifdef USE_OLD_DRV
        vpi_h26xe_cfg->picture_mem_factory[core_idx].size =
            getEWLMallocSize(picture_size);
#else
        vpi_h26xe_cfg->picture_mem_factory[core_idx].size =
            getEWLMallocInoutSize(vpi_h26xe_cfg->input_alignment, picture_size);
#endif
    }

    /* Limited amount of memory on some test environment */
    if (cmdl->out_buf_size_max == 0)
        cmdl->out_buf_size_max = 12;
    stream_buf_total_size = 4 * vpi_h26xe_cfg->picture_mem_factory[0].size;
    stream_buf_total_size =
        CLIP3(VCENC_STREAM_MIN_BUF0_SIZE,
              (u32)cmdl->out_buf_size_max * 1024 * 1024, stream_buf_total_size);
    stream_buf_total_size =
        (cmdl->stream_multi_segment_mode != 0 ?
             vpi_h26xe_cfg->picture_mem_factory[0].size / 16 :
             stream_buf_total_size);
    outbuf_size[0] = stream_buf_total_size;
    if (vpi_h26xe_cfg->stream_buf_num > 1) {
        /* set small stream buffer0 to test two stream buffers */
        outbuf_size[0] = MAX(vpi_h26xe_cfg->picture_mem_factory[0].size >> 10,
                             VCENC_STREAM_MIN_BUF0_SIZE);
        outbuf_size[1] = (stream_buf_total_size - outbuf_size[0]) /
                         (vpi_h26xe_cfg->stream_buf_num - 1);
    }
    for (core_idx = 0; core_idx < vpi_h26xe_cfg->parallel_core_num;
         core_idx++) {
        for (i_buf = 0; i_buf < vpi_h26xe_cfg->stream_buf_num; i_buf++) {
            i32 size = outbuf_size[i_buf ? 1 : 0];

#ifdef USE_OLD_DRV
            ret = EWLMallocLinear(((struct vcenc_instance *)enc)->asic.ewl,
                                  size, vpi_h26xe_cfg->input_alignment,
                                  &vpi_h26xe_cfg
                                       ->outbuf_mem_factory[core_idx][i_buf]);
            if (ret != EWL_OK) {
                vpi_h26xe_cfg->outbuf_mem_factory[core_idx][i_buf]
                    .virtualAddress = NULL;
                return 1;
            }
#else
            ret =
                EWLMallocInoutLinear(((struct vcenc_instance *)enc)->asic.ewl,
                                     size, vpi_h26xe_cfg->input_alignment,
                                     &vpi_h26xe_cfg
                                          ->outbuf_mem_factory[core_idx][i_buf]);
            if (ret != EWL_OK) {
                /*memset(&vpi_h26xe_cfg->outbuf_mem_factory[core_idx][i_buf], 0, sizeof(EWLLinearMem_t));*/
                return 1;
            }
#endif
        }
    }

    /*allocate delta qp map memory.
      4 bits per block.*/
    block_size =
        ((cmdl->width + cmdl->max_cu_size - 1) & (~(cmdl->max_cu_size - 1))) *
        ((cmdl->height + cmdl->max_cu_size - 1) & (~(cmdl->max_cu_size - 1))) /
        (8 * 8 * 2);
    /* 8 bits per block if ipcm map/absolute roi qp is supported*/
    if (((struct vcenc_instance *)enc)->asic.regs.asicCfg.roiMapVersion >= 1)
        block_size *= 2;
    block_size = ((block_size + 15) & (~15));
#ifdef USE_OLD_DRV
    if (EWLMallocLinear(((struct vcenc_instance *)enc)->asic.ewl,
                        block_size * vpi_h26xe_cfg->buffer_cnt, 0,
                        &vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0]) !=
        EWL_OK) {
        vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0].virtualAddress = NULL;
        return 1;
    }
#else
    if (EWLMallocInoutLinear(((struct vcenc_instance *)enc)->asic.ewl,
                             block_size * vpi_h26xe_cfg->buffer_cnt, 0,
                             &vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0]) !=
        EWL_OK) {
        /*memset(&vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0], 0, sizeof(EWLLinearMem_t));*/
        return 1;
    }
#endif
    i32 total_size = vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0].size;
    for (core_idx = 0; core_idx < vpi_h26xe_cfg->buffer_cnt; core_idx++) {
#ifdef USE_OLD_DRV
        vpi_h26xe_cfg->roi_map_delta_qpmem_factory[core_idx].virtualAddress =
            (u32 *)((ptr_t)vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0]
                        .virtualAddress +
                    core_idx * block_size);
#else
        vpi_h26xe_cfg->roi_map_delta_qpmem_factory[core_idx].rc_virtualAddress =
            (u32 *)((ptr_t)vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0]
                        .rc_virtualAddress +
                    core_idx * block_size);
#endif
        vpi_h26xe_cfg->roi_map_delta_qpmem_factory[core_idx].busAddress =
            vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0].busAddress +
            core_idx * block_size;
        vpi_h26xe_cfg->roi_map_delta_qpmem_factory[core_idx].size =
            (core_idx < vpi_h26xe_cfg->buffer_cnt - 1 ?
                 block_size :
                 total_size - (vpi_h26xe_cfg->buffer_cnt - 1) * block_size);
#ifdef USE_OLD_DRV
        memset(vpi_h26xe_cfg->roi_map_delta_qpmem_factory[core_idx]
                   .virtualAddress,
               0, block_size);
#else
        memset(vpi_h26xe_cfg->roi_map_delta_qpmem_factory[core_idx]
                   .rc_virtualAddress,
               0, block_size);
#endif
        if (((struct vcenc_instance *)enc)->asic.regs.asicCfg.roiMapVersion ==
            3) {
            if (cmdl->roimap_cu_ctrl_info_bin_file != NULL) {
                u8 u8CuInfoSize;
                if (cmdl->roi_cu_ctrl_ver == 3)
                    u8CuInfoSize = 1;
                else if (cmdl->roi_cu_ctrl_ver == 4)
                    u8CuInfoSize = 2;
                else if (cmdl->roi_cu_ctrl_ver == 5)
                    u8CuInfoSize = 6;
                else if (cmdl->roi_cu_ctrl_ver == 6)
                    u8CuInfoSize = 12;
                else /* if((cmdl->roi_cu_ctrl_ver == 7)*/
                    u8CuInfoSize = 14;

#ifdef USE_OLD_DRV
                if (EWLMallocLinear(((struct vcenc_instance *)enc)->asic.ewl,
                                    (block_size * u8CuInfoSize), 0,
                                    &vpi_h26xe_cfg->roimap_cu_ctrl_infomem_factory
                                         [core_idx]) != EWL_OK) {
                    vpi_h26xe_cfg->roimap_cu_ctrl_infomem_factory[core_idx]
                        .virtualAddress = NULL;
                    return 1;
                }
                memset(vpi_h26xe_cfg->roimap_cu_ctrl_infomem_factory[core_idx]
                           .virtualAddress,
                       0, block_size);
#else
                if (EWLMallocInoutLinear(((struct vcenc_instance *)enc)
                                             ->asic.ewl,
                                         (block_size * u8CuInfoSize), 0,
                                         &vpi_h26xe_cfg
                                              ->roimap_cu_ctrl_infomem_factory
                                                  [core_idx]) != EWL_OK) {
                    /*memset(&vpi_h26xe_cfg->roimap_cu_ctrl_infomem_factory[core_idx], 0, sizeof(EWLLinearMem_t));*/
                    return 1;
                }
                memset(vpi_h26xe_cfg->roimap_cu_ctrl_infomem_factory[core_idx]
                           .rc_virtualAddress,
                       0, block_size);
#endif
            }

            if (cmdl->roimap_cu_ctrl_index_bin_file != NULL) {
                block_size = 1 << (IS_H264(cmdl->codec_format) ? 4 : 6);
                block_size =
                    ((cmdl->width + block_size - 1) & (~(block_size - 1))) *
                    ((cmdl->height + block_size - 1) & (~(block_size - 1))) /
                    (block_size * block_size);
#ifdef USE_OLD_DRV
                if (EWLMallocLinear(((struct vcenc_instance *)enc)->asic.ewl,
                                    block_size, 0,
                                    &vpi_h26xe_cfg
                                         ->roimap_cu_ctrl_indexmem_factory
                                             [core_idx]) != EWL_OK) {
                    vpi_h26xe_cfg->roimap_cu_ctrl_indexmem_factory[core_idx]
                        .virtualAddress = NULL;
                    return 1;
                }
                memset(vpi_h26xe_cfg->roimap_cu_ctrl_indexmem_factory[core_idx]
                           .virtualAddress,
                       0, block_size);
#else
                if (EWLMallocInoutLinear(((struct vcenc_instance *)enc)
                                             ->asic.ewl,
                                         block_size, 0,
                                         &vpi_h26xe_cfg
                                              ->roimap_cu_ctrl_indexmem_factory
                                                  [core_idx]) != EWL_OK) {
                    /*memset(&vpi_h26xe_cfg->roimap_cu_ctrl_indexmem_factory[core_idx], 0, sizeof(EWLLinearMem_t));*/
                    return 1;
                }
                memset(vpi_h26xe_cfg->roimap_cu_ctrl_indexmem_factory[core_idx]
                           .rc_virtualAddress,
                       0, block_size);
#endif
            }
        }
    }
    for (core_idx = 0; core_idx < vpi_h26xe_cfg->buffer_cnt; core_idx++) {
#ifdef USE_OLD_DRV
#else
#endif
    }

    for (core_idx = 0; core_idx < vpi_h26xe_cfg->parallel_core_num;
         core_idx++) {
        for (i_buf = 0; i_buf < vpi_h26xe_cfg->stream_buf_num; i_buf++) {
#ifdef USE_OLD_DRV
#else
#endif
        }
    }
    return 0;
}

/**
 *h26x_enc_free_res
 *Release all resources allcoated byt h26x_enc_alloc_res()
 */
void h26x_enc_free_res(VCEncInst enc, struct VPIH26xEncCfg *vpi_h26xe_cfg)
{
    u32 core_idx = 0;
    i32 i_buf;

#ifdef USE_OLD_DRV
    if (vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0].virtualAddress != NULL) {
        for (core_idx = 1; core_idx < vpi_h26xe_cfg->buffer_cnt; core_idx++)
            vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0].size +=
                vpi_h26xe_cfg->roi_map_delta_qpmem_factory[core_idx].size;
        EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                      &vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0]);
    }
    for (core_idx = 0; core_idx < vpi_h26xe_cfg->buffer_cnt; core_idx++) {
        if (vpi_h26xe_cfg->picture_mem_factory[core_idx].virtualAddress !=
            NULL) {
#ifdef SUPPORT_TCACHE
            if (vpi_h26xe_cfg->picture_mem_factory[core_idx].in_host) {
                EWLFreeHostLinear(((struct vcenc_instance *)enc)->asic.ewl,
                                  &vpi_h26xe_cfg->picture_mem_factory[core_idx]);
            } else
#endif
            {
                EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                              &vpi_h26xe_cfg->picture_mem_factory[core_idx]);
            }
        }

        if (vpi_h26xe_cfg->picture_dsmem_factory[core_idx].virtualAddress !=
            NULL)
            EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                          &vpi_h26xe_cfg->picture_dsmem_factory[core_idx]);

        if (vpi_h26xe_cfg->transform_mem_factory[core_idx].virtualAddress !=
            NULL)
            EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                          &vpi_h26xe_cfg->transform_mem_factory[core_idx]);

#ifdef SUPPORT_TCACHE
        if (vpi_h26xe_cfg->edma_link_buf.virtualAddress != NULL)
            EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                          &vpi_h26xe_cfg->edma_link_buf);
#endif

        if (vpi_h26xe_cfg->roimap_cu_ctrl_infomem_factory[core_idx]
                .virtualAddress != NULL)
            EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                          &vpi_h26xe_cfg
                               ->roimap_cu_ctrl_infomem_factory[core_idx]);
        if (vpi_h26xe_cfg->roimap_cu_ctrl_indexmem_factory[core_idx]
                .virtualAddress != NULL)
            EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                          &vpi_h26xe_cfg
                               ->roimap_cu_ctrl_indexmem_factory[core_idx]);
    }
    for (core_idx = 0; core_idx < vpi_h26xe_cfg->parallel_core_num;
         core_idx++) {
        for (i_buf = 0; i_buf < vpi_h26xe_cfg->stream_buf_num; i_buf++) {
            if (vpi_h26xe_cfg->outbuf_mem_factory[core_idx][i_buf]
                    .virtualAddress != NULL)
                EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                              &vpi_h26xe_cfg
                                   ->outbuf_mem_factory[core_idx][i_buf]);
        }
    }
#else /*USE_OLD_DRV */
    for (core_idx = 1; core_idx < vpi_h26xe_cfg->buffer_cnt; core_idx++)
        vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0].size +=
            vpi_h26xe_cfg->roi_map_delta_qpmem_factory[core_idx].size;
    EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                  &vpi_h26xe_cfg->roi_map_delta_qpmem_factory[0]);

    for (core_idx = 0; core_idx < vpi_h26xe_cfg->buffer_cnt; core_idx++) {
        EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                      &vpi_h26xe_cfg->roimap_cu_ctrl_infomem_factory[core_idx]);
        EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                      &vpi_h26xe_cfg->roimap_cu_ctrl_indexmem_factory[core_idx]);
    }
    for (core_idx = 0; core_idx < vpi_h26xe_cfg->parallel_core_num;
         core_idx++) {
        for (i_buf = 0; i_buf < vpi_h26xe_cfg->stream_buf_num; i_buf++) {
            EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                          &vpi_h26xe_cfg->outbuf_mem_factory[core_idx][i_buf]);
        }
    }

#ifdef SUPPORT_TCACHE
    EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                  &vpi_h26xe_cfg->enc_in.PrivData.edma_link_buf);
    EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl,
                  &vpi_h26xe_cfg->enc_in.PassTwoHWData.edma_link_buf);
#endif

#endif
}

/**
 * h26x_enc_init_input_line_buffer
 * get line buffer params for IRQ handle, get address of input line buffer
 */
int h26x_enc_init_input_line_buffer(inputLineBufferCfg *line_buf_cfg,
                                    VPIH26xEncOptions *options, VCEncIn *encIn,
                                    VCEncInst inst,
                                    struct VPIH26xEncCfg *vpi_h26xe_cfg)
{
    VCEncCodingCtrl coding_cfg;
    u32 stride, chroma_stride;
    VCEncGetAlignedStride(options->lum_width_src, options->input_format,
                          &stride, &chroma_stride,
                          vpi_h26xe_cfg->input_alignment);
    VCEncGetCodingCtrl(inst, &coding_cfg);

    memset(line_buf_cfg, 0, sizeof(inputLineBufferCfg));
    line_buf_cfg->depth        = coding_cfg.inputLineBufDepth;
    line_buf_cfg->hwHandShake  = coding_cfg.inputLineBufHwModeEn;
    line_buf_cfg->loopBackEn   = coding_cfg.inputLineBufLoopBackEn;
    line_buf_cfg->inst         = (void *)inst;
    line_buf_cfg->asic         = &(((struct vcenc_instance *)inst)->asic);
    line_buf_cfg->wrCnt        = 0;
    line_buf_cfg->inputFormat  = options->input_format;
    line_buf_cfg->pixOnRow     = stride;
    line_buf_cfg->encWidth     = options->width;
    line_buf_cfg->encHeight    = options->height;
    line_buf_cfg->srcHeight    = options->lum_height_src;
    line_buf_cfg->srcVerOffset = options->ver_offset_src;
    line_buf_cfg->getMbLines   = &VCEncGetEncodedMbLines;
    line_buf_cfg->setMbLines   = &VCEncSetInputMBLines;
    line_buf_cfg->ctbSize      = IS_H264(options->codec_format) ? 16 : 64;
    line_buf_cfg->lumSrc       = vpi_h26xe_cfg->lum;
    line_buf_cfg->cbSrc        = vpi_h26xe_cfg->cb;
    line_buf_cfg->crSrc        = vpi_h26xe_cfg->cr;

    if (VCEncInitInputLineBuffer(line_buf_cfg))
        return -1;

    /* loopback mode */
    if (line_buf_cfg->loopBackEn && line_buf_cfg->lumBuf.buf) {
        VCEncPreProcessingCfg pre_proc_cfg;
        encIn->busLuma    = line_buf_cfg->lumBuf.busAddress;
        encIn->busChromaU = line_buf_cfg->cbBuf.busAddress;
        encIn->busChromaV = line_buf_cfg->crBuf.busAddress;

        /* In loop back mode, data in line buffer start from the line to be encoded */
        VCEncGetPreProcessing(inst, &pre_proc_cfg);
        pre_proc_cfg.yOffset = 0;
        VCEncSetPreProcessing(inst, &pre_proc_cfg);
    }

    return 0;
}

/**
 * h26x_enc_ma_add_frame
 * Add new frame bits for moving average bitrate calculation
 */
static void h26x_enc_ma_add_frame(MaS *ma, i32 frame_size_bits)
{
    ma->frame[ma->pos++] = frame_size_bits;
    if (ma->pos == ma->length)
        ma->pos = 0;
    if (ma->count < ma->length)
        ma->count++;
}

/**
 * h26x_enc_ma
 * Calculate average bitrate of moving window
 */
static i32 h26x_enc_ma(MaS *ma)
{
    i32 i;
    unsigned long long sum = 0; /* Using 64-bits to avoid overflow */

    for (i = 0; i < ma->count; i++)
        sum += ma->frame[i];

    if (!ma->frame_rate_denom)
        return 0;

    sum = sum / ma->count;

    return sum * (ma->frame_rate_numer + ma->frame_rate_denom - 1) /
           ma->frame_rate_denom;
}

int h26x_enc_update_statistic(struct VpiH26xEncCtx *enc_ctx, int *streamSize)
{
    int ret                    = 0;
    VPIH26xEncOptions *options = &enc_ctx->options;
    struct VPIH26xEncCfg *vpi_h26xe_cfg =
        (struct VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);
    VCEncOut *pEncOut = (VCEncOut *)&enc_ctx->enc_out;

    enc_ctx->total_bits += pEncOut->streamSize * 8;
    *streamSize += pEncOut->streamSize;

    p_enc_in->timeIncrement = vpi_h26xe_cfg->output_rate_denom;
    vpi_h26xe_cfg->validencoded_framenumber++;
    h26x_enc_ma_add_frame(&enc_ctx->ma, pEncOut->streamSize * 8);
    vpi_h26xe_cfg->hwcycle_acc += VCEncGetPerformance(enc_ctx->hantro_encoder);

    double ssim =
        pEncOut->ssim[0] * 0.8 + 0.1 * (pEncOut->ssim[1] + pEncOut->ssim[2]);
    vpi_h26xe_cfg->ssim_acc += ssim;

    if ((options->pic_rc == 1) &&
        (vpi_h26xe_cfg->validencoded_framenumber >= enc_ctx->ma.length)) {
        vpi_h26xe_cfg->number_square_of_error++;
        if (vpi_h26xe_cfg->max_error_over_target <
            (h26x_enc_ma(&enc_ctx->ma) - options->bit_per_second))
            vpi_h26xe_cfg->max_error_over_target =
                (h26x_enc_ma(&enc_ctx->ma) - options->bit_per_second);
        if (vpi_h26xe_cfg->max_error_under_target <
            (options->bit_per_second - h26x_enc_ma(&enc_ctx->ma)))
            vpi_h26xe_cfg->max_error_under_target =
                (options->bit_per_second - h26x_enc_ma(&enc_ctx->ma));
        vpi_h26xe_cfg->sum_square_of_error +=
            ((float)(ABS(h26x_enc_ma(&enc_ctx->ma) - options->bit_per_second)) *
             100 / options->bit_per_second);
        vpi_h26xe_cfg->average_square_of_error =
            (vpi_h26xe_cfg->sum_square_of_error /
             vpi_h26xe_cfg->number_square_of_error);
    }
    return ret;
}
