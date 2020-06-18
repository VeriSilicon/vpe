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

/**
 *  h26x_enc_open_encoder
 *  Create and configure an encoder instance.
 *
 *  @Params: options  processed comand line options
 *  @Params: p_enc place where to save the new encoder instance
 *  @Return: 0 for success, -1 for error
 */
static int h26x_enc_open_encoder(VPIH26xEncOptions *options, VCEncInst *p_enc,
                                 struct VPIH26xEncCfg *vpi_h26xe_cfg)
{
    VCEncRet ret = -1;
    VCEncConfig cfg;
    VCEncCodingCtrl coding_cfg;
    VCEncRateCtrl rc_cfg;
    VCEncPreProcessingCfg pre_proc_cfg;
    VCEncInst encoder = NULL;
    i32 i;
    EWLInitParam_t param;

    /* Encoder initialization */
    if (options->width == DEFAULT)
        options->width = options->lum_width_src;

    if (options->height == DEFAULT)
        options->height = options->lum_height_src;

    change_cml_customized_format(options);

    /* output_rate_numer */
    if (options->output_rate_numer == DEFAULT) {
        options->output_rate_numer = options->input_rate_numer;
    }

    /* outputRateDenom */
    if (options->output_rate_denom == DEFAULT) {
        options->output_rate_denom = options->input_rate_denom;
    }
    /*cfg.ctb_size = options->max_cu_size; */
    if (options->rotation && options->rotation != 3) {
        cfg.width  = options->height;
        cfg.height = options->width;
    } else {
        cfg.width  = options->width;
        cfg.height = options->height;
    }

    cfg.frameRateDenom = options->output_rate_denom;
    cfg.frameRateNum   = options->output_rate_numer;

    /* intra tools in sps and pps */
    cfg.strongIntraSmoothing = options->strong_intra_smoothing_enabled_flag;
    cfg.streamType =
        (options->byte_stream) ? VCENC_BYTE_STREAM : VCENC_NAL_UNIT_STREAM;
    cfg.level = (IS_H264(options->codec_format) ? VCENC_H264_LEVEL_5_2 :
                                                  VCENC_HEVC_LEVEL_5_1);

    /* hard coded level for H.264 */
    if (options->level != DEFAULT && options->level != 0)
        cfg.level = (VCEncLevel)options->level;

    cfg.tier = VCENC_HEVC_MAIN_TIER;
    if (options->tier != DEFAULT)
        cfg.tier = (VCEncTier)options->tier;

    /*cfg.profile = (IS_H264(options->codec_format) ? VCENC_H264_HIGH_PROFILE : VCENC_HEVC_MAIN_PROFILE);*/
    cfg.profile = (IS_H264(options->codec_format) ? VCENC_H264_MAIN_PROFILE :
                                                    VCENC_HEVC_MAIN_PROFILE);

    cfg.codecFormat = options->codec_format;

    if (options->profile != DEFAULT && options->profile != 0)
        cfg.profile = (VCEncProfile)options->profile;

    /* convert between H.264/HEVC profiles for testing purpose */
    if (IS_H264(options->codec_format)) {
        if ((int)cfg.profile >= VCENC_HEVC_MAIN_PROFILE &&
            cfg.profile < VCENC_HEVC_MAIN_10_PROFILE)
            cfg.profile = VCENC_H264_HIGH_PROFILE;
    } else {
        if (cfg.profile >= VCENC_H264_BASE_PROFILE &&
            cfg.profile <= VCENC_H264_HIGH_PROFILE)
            cfg.profile = VCENC_HEVC_MAIN_PROFILE;
    }

    cfg.bitDepthLuma = 8;
    if (options->bit_depth_luma != DEFAULT && options->bit_depth_luma != 8)
        cfg.bitDepthLuma = options->bit_depth_luma;
    cfg.bitDepthChroma = 8;
    if (options->bit_depth_chroma != DEFAULT && options->bit_depth_chroma != 8)
        cfg.bitDepthChroma = options->bit_depth_chroma;

    /*for fbtrans bitdepth = 9 is not supported, and bitdepth luma != chroma is not supported too -kwu*/
    if ((cfg.bitDepthLuma != 8 && cfg.bitDepthLuma != 10) ||
        (cfg.bitDepthChroma != 8 && cfg.bitDepthChroma != 10) ||
        cfg.bitDepthLuma != cfg.bitDepthChroma) {
        VPILOGE("UNSUPPORT bitdepth!\n");
        goto error_exit;
    }

    if ((options->interlaced_frame && options->gop_size != 1) ||
        IS_H264(options->codec_format)) {
        VPILOGE(
            "OpenEncoder: treat interlace to progressive for gopSize!=1 case\n");
        options->interlaced_frame = 0;
    }
#ifndef BUILD_CMODEL
    if (options->input_format == VCENC_YUV420_SEMIPLANAR_8BIT_FB ||
        options->input_format == VCENC_YUV420_SEMIPLANAR_VU_8BIT_FB ||
        options->input_format == VCENC_YUV420_PLANAR_10BIT_P010_FB
#ifdef SUPPORT_DEC400
        || options->input_format ==
               INPUT_FORMAT_YUV420_SEMIPLANAR_8BIT_COMPRESSED_FB ||
        options->input_format ==
            INPUT_FORMAT_YUV420_SEMIPLANAR_VU_8BIT_COMPRESSED_FB ||
        options->input_format ==
            INPUT_FORMAT_YUV420_PLANAR_10BIT_P010_COMPRESSED_FB
#endif
    ) {
        if (options->exp_of_input_alignment < 10)
            options->exp_of_input_alignment = 10;
    }
    options->exp_of_ref_alignment    = 10;
    options->exp_of_ref_ch_alignment = 10;
#endif

    /*default maxTLayer*/
    cfg.maxTLayers = 1;

    /* Find the max number of reference frame */
    if (options->intra_pic_rate == 1) {
        cfg.refFrameAmount = 0;
    } else {
        u32 maxRefPics    = 0;
        u32 maxTemporalId = 0;
        int idx;
        for (idx = 0; idx < vpi_h26xe_cfg->enc_in.gopConfig.size; idx++) {
            VCEncGopPicConfig *cfg =
                &(vpi_h26xe_cfg->enc_in.gopConfig.pGopPicCfg[idx]);
            if (cfg->codingType != VCENC_INTRA_FRAME) {
                if (maxRefPics < cfg->numRefPics)
                    maxRefPics = cfg->numRefPics;

                if (maxTemporalId < cfg->temporalId)
                    maxTemporalId = cfg->temporalId;
            }
        }
        cfg.refFrameAmount = maxRefPics + options->interlaced_frame +
                             vpi_h26xe_cfg->enc_in.gopConfig.ltrcnt;
        cfg.maxTLayers = maxTemporalId + 1;
    }
    cfg.compressor = options->compressor;
    VPILOGE("%s cfg.compressor = %d\n", __FUNCTION__, cfg.compressor);

    cfg.interlacedFrame    = options->interlaced_frame;
    cfg.enableOutputCuInfo = (options->enable_output_cu_info > 0) ? 1 : 0;
    cfg.rdoLevel           = CLIP3(1, 3, options->rdo_level) - 1;
    VPILOGE("%s(%d)cfg.rdoLevel = %d\n", __FUNCTION__, __LINE__, cfg.rdoLevel);
    cfg.verbose                 = options->verbose;
    cfg.exp_of_input_alignment  = options->exp_of_input_alignment;
    cfg.exp_of_ref_alignment    = options->exp_of_ref_alignment;
    cfg.exp_of_ref_ch_alignment = options->exp_of_ref_ch_alignment;
    cfg.exteralReconAlloc       = 0;
    cfg.P010RefEnable           = options->p010_ref_enable;
    cfg.enableSsim              = options->ssim;
    cfg.ctbRcMode         = (options->ctb_rc != DEFAULT) ? options->ctb_rc : 0;
    cfg.parallelCoreNum   = options->parallel_core_num;
    cfg.pass              = (options->lookahead_depth ? 2 : 0);
    cfg.bPass1AdaptiveGop = (options->gop_size == 0);
    cfg.picOrderCntType   = options->pic_order_cnt_type;
    cfg.dumpRegister      = options->dump_register;
    cfg.rasterscan        = options->rasterscan;
    cfg.log2MaxPicOrderCntLsb = options->log2_max_pic_order_cnt_lsb;
    cfg.log2MaxFrameNum       = options->log2_max_frame_num;
    cfg.lookaheadDepth        = options->lookahead_depth;
    cfg.extDSRatio =
        ((options->lookahead_depth && options->cutree_blkratio) ? 1 : 0);
    if (cfg.extDSRatio) {
        cfg.width_ds = options->width_ds;
        if (options->width_ds == DEFAULT) {
            cfg.width_ds = options->lum_widthsrc_ds;
        }
        cfg.height_ds = options->height_ds;
        if (options->height_ds == DEFAULT) {
            cfg.height_ds = options->lum_heightsrc_ds;
        }
    }
    VPILOGE("[%d] cfg.extDSRatio = %d, and ds res %dx%d\n",
            vpi_h26xe_cfg->enc_index, cfg.extDSRatio, cfg.width_ds,
            cfg.height_ds);
    if (options->parallel_core_num > 1 && cfg.width * cfg.height < 256 * 256) {
        VPILOGE("Disable multicore for small resolution (< 255*255)\n");
        cfg.parallelCoreNum = options->parallel_core_num = 1;
    }
#ifdef FB_SYSLOG_ENABLE
    cfg.enc_index = vpi_h26xe_cfg->enc_index;
#ifdef DRV_NEW_ARCH
    cfg.device_id = vpi_h26xe_cfg->log_header.device_id;
#endif
#endif
    VPILOGE("\n+++ cfg.w = %d, cfg.h = %d\n", cfg.width, cfg.height);

    cfg.perf = calloc(1, sizeof(ENCPERF));
    if (cfg.perf) {
        ENCPERF *perf       = cfg.perf;
        vpi_h26xe_cfg->perf = perf;
        VPILOGE("calloc perf = %p\n", perf);
        pthread_mutex_init(&perf->hwcycle_acc_mutex, NULL);
#ifdef FB_PERFORMANCE_STATIC
        PERFORMANCE_STATIC_INIT(vpi_h26xe_cfg, perf, vcehw);
        PERFORMANCE_STATIC_INIT(vpi_h26xe_cfg, perf, vcehwp1);
        PERFORMANCE_STATIC_INIT(vpi_h26xe_cfg, perf, vce_dummy_0);
        PERFORMANCE_STATIC_INIT(vpi_h26xe_cfg, perf, vce_dummy_1);
        PERFORMANCE_STATIC_INIT(vpi_h26xe_cfg, perf, CU_ANAL);
        PERFORMANCE_STATIC_INIT(vpi_h26xe_cfg, perf, vcehw_total);
        PERFORMANCE_STATIC_INIT(vpi_h26xe_cfg, perf, vce_total);
#endif
    }

    param.clientType = IS_H264(options->codec_format) ?
                           EWL_CLIENT_TYPE_H264_ENC :
                           EWL_CLIENT_TYPE_HEVC_ENC;

#ifdef DRV_NEW_ARCH
    param.device   = vpi_h26xe_cfg->device;
    param.priority = vpi_h26xe_cfg->priority;
    param.mem_id   = vpi_h26xe_cfg->mem_id;
    param.pass     = 0;
#endif
    param.perf      = cfg.perf;
    param.ewl_index = vpi_h26xe_cfg->enc_index;
    if ((vpi_h26xe_cfg->ewl = EWLInit(&param)) == NULL) {
        VPILOGE("OpenEncoder: EWL Initialization failed!\n");
        goto error_exit;
    }
    VPILOGE("ewl is outside initialized.\n");

    /* this is for 2 pass */
    if (options->lookahead_depth != 0) {
        param.pass = 1;
        if ((vpi_h26xe_cfg->two_pass_ewl = EWLInit(&param)) == NULL) {
            VPILOGE("OpenEncoder: EWL for 2 pass Initialization failed!\n");
            goto error_exit;
        }
        VPILOGE("2 pass ewl is outside initialized.\n");
    }

    if ((ret = VCEncInit(&cfg, p_enc, vpi_h26xe_cfg->ewl,
                         vpi_h26xe_cfg->two_pass_ewl)) != VCENC_OK) {
        VPILOGE("VCEncInit failed\n");
        encoder = *p_enc;
        goto error_exit;
    }

    encoder = *p_enc;

    /* Encoder setup: coding control */
    if ((ret = VCEncGetCodingCtrl(encoder, &coding_cfg)) != VCENC_OK) {
        VPILOGE("VCEncGetCodingCtrl failed\n");
        goto error_exit;
    } else {
        if (options->slice_size != DEFAULT)
            coding_cfg.sliceSize = options->slice_size;
        if (options->enable_cabac != DEFAULT)
            coding_cfg.enableCabac = options->enable_cabac;
        if (options->cabac_init_flag != DEFAULT)
            coding_cfg.cabacInitFlag = options->cabac_init_flag;
        coding_cfg.videoFullRange = 0;
        if (options->video_range != DEFAULT)
            coding_cfg.videoFullRange = options->video_range;

        coding_cfg.disableDeblockingFilter = (options->disable_deblocking != 0);
        coding_cfg.tc_Offset               = options->tc_offset;
        coding_cfg.beta_Offset             = options->beta_offset;
        coding_cfg.enableSao               = options->enable_sao;
        coding_cfg.enableDeblockOverride   = options->enable_deblock_override;
        coding_cfg.deblockOverride         = options->deblock_override;

        if (options->sei)
            coding_cfg.seiMessages = 1;
        else
            coding_cfg.seiMessages = 0;

        coding_cfg.gdrDuration = options->gdr_duration;
        coding_cfg.fieldOrder  = options->field_order;

        coding_cfg.cirStart    = options->cir_start;
        coding_cfg.cirInterval = options->cir_interval;

        if (coding_cfg.gdrDuration == 0) {
            coding_cfg.intraArea.top    = options->intra_area_top;
            coding_cfg.intraArea.left   = options->intra_area_left;
            coding_cfg.intraArea.bottom = options->intra_area_bottom;
            coding_cfg.intraArea.right  = options->intra_area_right;
            coding_cfg.intraArea.enable =
                h26x_enc_check_area(&coding_cfg.intraArea, options);
        } else {
            /*intraArea will be used by GDR, customer can not use intraArea when GDR is enabled.*/
            coding_cfg.intraArea.enable = 0;
        }

        coding_cfg.pcm_loop_filter_disabled_flag =
            options->pcm_loop_filter_disabled_flag;

        coding_cfg.ipcm1Area.top    = options->ipcm1_area_top;
        coding_cfg.ipcm1Area.left   = options->ipcm1_area_left;
        coding_cfg.ipcm1Area.bottom = options->ipcm1_area_bottom;
        coding_cfg.ipcm1Area.right  = options->ipcm1_area_right;
        coding_cfg.ipcm1Area.enable =
            h26x_enc_check_area(&coding_cfg.ipcm1Area, options);

        coding_cfg.ipcm2Area.top    = options->ipcm2_area_top;
        coding_cfg.ipcm2Area.left   = options->ipcm2_area_left;
        coding_cfg.ipcm2Area.bottom = options->ipcm2_area_bottom;
        coding_cfg.ipcm2Area.right  = options->ipcm2_area_right;
        coding_cfg.ipcm2Area.enable =
            h26x_enc_check_area(&coding_cfg.ipcm2Area, options);
        coding_cfg.ipcmMapEnable = options->ipcm_map_enable;
        coding_cfg.pcm_enabled_flag =
            (coding_cfg.ipcm1Area.enable || coding_cfg.ipcm2Area.enable ||
             coding_cfg.ipcmMapEnable);

        if (coding_cfg.gdrDuration == 0) {
            coding_cfg.roi1Area.top    = options->roi1_area_top;
            coding_cfg.roi1Area.left   = options->roi1_area_left;
            coding_cfg.roi1Area.bottom = options->roi1_area_bottom;
            coding_cfg.roi1Area.right  = options->roi1_area_right;
            if (h26x_enc_check_area(&coding_cfg.roi1Area, options) &&
                (options->roi1_delta_qp || (options->roi1_qp >= 0)))
                coding_cfg.roi1Area.enable = 1;
            else
                coding_cfg.roi1Area.enable = 0;
        } else {
            coding_cfg.roi1Area.enable = 0;
        }

        coding_cfg.roi2Area.top    = options->roi2_area_top;
        coding_cfg.roi2Area.left   = options->roi2_area_left;
        coding_cfg.roi2Area.bottom = options->roi2_area_bottom;
        coding_cfg.roi2Area.right  = options->roi2_area_right;
        if (h26x_enc_check_area(&coding_cfg.roi2Area, options) &&
            (options->roi2_delta_qp || (options->roi2_qp >= 0)))
            coding_cfg.roi2Area.enable = 1;
        else
            coding_cfg.roi2Area.enable = 0;

        coding_cfg.roi3Area.top    = options->roi3_area_top;
        coding_cfg.roi3Area.left   = options->roi3_area_left;
        coding_cfg.roi3Area.bottom = options->roi3_area_bottom;
        coding_cfg.roi3Area.right  = options->roi3_area_right;
        if (h26x_enc_check_area(&coding_cfg.roi3Area, options) &&
            (options->roi3_delta_qp || (options->roi3_qp >= 0)))
            coding_cfg.roi3Area.enable = 1;
        else
            coding_cfg.roi3Area.enable = 0;

        coding_cfg.roi4Area.top    = options->roi4_area_top;
        coding_cfg.roi4Area.left   = options->roi4_area_left;
        coding_cfg.roi4Area.bottom = options->roi4_area_bottom;
        coding_cfg.roi4Area.right  = options->roi4_area_right;
        if (h26x_enc_check_area(&coding_cfg.roi4Area, options) &&
            (options->roi4_delta_qp || (options->roi4_qp >= 0)))
            coding_cfg.roi4Area.enable = 1;
        else
            coding_cfg.roi4Area.enable = 0;

        coding_cfg.roi5Area.top    = options->roi5_area_top;
        coding_cfg.roi5Area.left   = options->roi5_area_left;
        coding_cfg.roi5Area.bottom = options->roi5_area_bottom;
        coding_cfg.roi5Area.right  = options->roi5_area_right;
        if (h26x_enc_check_area(&coding_cfg.roi5Area, options) &&
            (options->roi5_delta_qp || (options->roi5_qp >= 0)))
            coding_cfg.roi5Area.enable = 1;
        else
            coding_cfg.roi5Area.enable = 0;

        coding_cfg.roi6Area.top    = options->roi6_area_top;
        coding_cfg.roi6Area.left   = options->roi6_area_left;
        coding_cfg.roi6Area.bottom = options->roi6_area_bottom;
        coding_cfg.roi6Area.right  = options->roi6_area_right;
        if (h26x_enc_check_area(&coding_cfg.roi6Area, options) &&
            (options->roi6_delta_qp || (options->roi6_qp >= 0)))
            coding_cfg.roi6Area.enable = 1;
        else
            coding_cfg.roi6Area.enable = 0;

        coding_cfg.roi7Area.top    = options->roi7_area_top;
        coding_cfg.roi7Area.left   = options->roi7_area_left;
        coding_cfg.roi7Area.bottom = options->roi7_area_bottom;
        coding_cfg.roi7Area.right  = options->roi7_area_right;
        if (h26x_enc_check_area(&coding_cfg.roi7Area, options) &&
            (options->roi7_delta_qp || (options->roi7_qp >= 0)))
            coding_cfg.roi7Area.enable = 1;
        else
            coding_cfg.roi7Area.enable = 0;

        coding_cfg.roi8Area.top    = options->roi8_area_top;
        coding_cfg.roi8Area.left   = options->roi8_area_left;
        coding_cfg.roi8Area.bottom = options->roi8_area_bottom;
        coding_cfg.roi8Area.right  = options->roi8_area_right;
        if (h26x_enc_check_area(&coding_cfg.roi8Area, options) &&
            (options->roi8_delta_qp || (options->roi8_qp >= 0)))
            coding_cfg.roi8Area.enable = 1;
        else
            coding_cfg.roi8Area.enable = 0;

        coding_cfg.roi1DeltaQp = options->roi1_delta_qp;
        coding_cfg.roi2DeltaQp = options->roi2_delta_qp;
        coding_cfg.roi3DeltaQp = options->roi3_delta_qp;
        coding_cfg.roi4DeltaQp = options->roi4_delta_qp;
        coding_cfg.roi5DeltaQp = options->roi5_delta_qp;
        coding_cfg.roi6DeltaQp = options->roi6_delta_qp;
        coding_cfg.roi7DeltaQp = options->roi7_delta_qp;
        coding_cfg.roi8DeltaQp = options->roi8_delta_qp;
        coding_cfg.roi1Qp      = options->roi1_qp;
        coding_cfg.roi2Qp      = options->roi2_qp;
        coding_cfg.roi3Qp      = options->roi3_qp;
        coding_cfg.roi4Qp      = options->roi4_qp;
        coding_cfg.roi5Qp      = options->roi5_qp;
        coding_cfg.roi6Qp      = options->roi6_qp;
        coding_cfg.roi7Qp      = options->roi7_qp;
        coding_cfg.roi8Qp      = options->roi8_qp;

        if (coding_cfg.cirInterval)
            VPILOGE("  CIR: %d %d\n", coding_cfg.cirStart,
                    coding_cfg.cirInterval);

        if (coding_cfg.intraArea.enable)
            VPILOGE("  IntraArea: %dx%d-%dx%d\n", coding_cfg.intraArea.left,
                    coding_cfg.intraArea.top, coding_cfg.intraArea.right,
                    coding_cfg.intraArea.bottom);

        if (coding_cfg.ipcm1Area.enable)
            VPILOGE("  IPCM1Area: %dx%d-%dx%d\n", coding_cfg.ipcm1Area.left,
                    coding_cfg.ipcm1Area.top, coding_cfg.ipcm1Area.right,
                    coding_cfg.ipcm1Area.bottom);

        if (coding_cfg.ipcm2Area.enable)
            VPILOGE("  IPCM2Area: %dx%d-%dx%d\n", coding_cfg.ipcm2Area.left,
                    coding_cfg.ipcm2Area.top, coding_cfg.ipcm2Area.right,
                    coding_cfg.ipcm2Area.bottom);

        if (coding_cfg.roi1Area.enable)
            VPILOGE("  ROI 1: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi1Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi1Qp >=
                    0 ? coding_cfg.roi1Qp : coding_cfg.roi1DeltaQp,
                    coding_cfg.roi1Area.left, coding_cfg.roi1Area.top,
                    coding_cfg.roi1Area.right, coding_cfg.roi1Area.bottom);

        if (coding_cfg.roi2Area.enable)
            VPILOGE("  ROI 2: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi2Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi2Qp >=
                    0 ? coding_cfg.roi2Qp : coding_cfg.roi2DeltaQp,
                    coding_cfg.roi2Area.left, coding_cfg.roi2Area.top,
                    coding_cfg.roi2Area.right, coding_cfg.roi2Area.bottom);

        if (coding_cfg.roi3Area.enable)
            VPILOGE("  ROI 3: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi3Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi3Qp >=
                    0 ? coding_cfg.roi3Qp : coding_cfg.roi3DeltaQp,
                    coding_cfg.roi3Area.left, coding_cfg.roi3Area.top,
                    coding_cfg.roi3Area.right, coding_cfg.roi3Area.bottom);

        if (coding_cfg.roi4Area.enable)
            VPILOGE("  ROI 4: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi4Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi4Qp >=
                    0 ? coding_cfg.roi4Qp : coding_cfg.roi4DeltaQp,
                    coding_cfg.roi4Area.left, coding_cfg.roi4Area.top,
                    coding_cfg.roi4Area.right, coding_cfg.roi4Area.bottom);

        if (coding_cfg.roi5Area.enable)
            VPILOGE("  ROI 5: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi5Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi5Qp >=
                    0 ? coding_cfg.roi5Qp : coding_cfg.roi5DeltaQp,
                    coding_cfg.roi5Area.left, coding_cfg.roi5Area.top,
                    coding_cfg.roi5Area.right, coding_cfg.roi5Area.bottom);

        if (coding_cfg.roi6Area.enable)
            VPILOGE("  ROI 6: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi6Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi6Qp >=
                    0 ? coding_cfg.roi6Qp : coding_cfg.roi6DeltaQp,
                    coding_cfg.roi6Area.left, coding_cfg.roi6Area.top,
                    coding_cfg.roi6Area.right, coding_cfg.roi6Area.bottom);

        if (coding_cfg.roi7Area.enable)
            VPILOGE("  ROI 7: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi7Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi7Qp >=
                    0 ? coding_cfg.roi7Qp : coding_cfg.roi7DeltaQp,
                    coding_cfg.roi7Area.left, coding_cfg.roi7Area.top,
                    coding_cfg.roi7Area.right, coding_cfg.roi7Area.bottom);

        if (coding_cfg.roi8Area.enable)
            VPILOGE("  ROI 8: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi8Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi8Qp >=
                    0 ? coding_cfg.roi8Qp : coding_cfg.roi8DeltaQp,
                    coding_cfg.roi8Area.left, coding_cfg.roi8Area.top,
                    coding_cfg.roi8Area.right, coding_cfg.roi8Area.bottom);

        coding_cfg.roiMapDeltaQpEnable = options->roi_map_delta_qp_enable;
        coding_cfg.roiMapDeltaQpBlockUnit =
            options->roi_map_delta_qp_block_unit;

        coding_cfg.RoimapCuCtrl_index_enable =
            (options->roimap_cu_ctrl_index_bin_file != NULL);
        coding_cfg.RoimapCuCtrl_enable =
            (options->roimap_cu_ctrl_info_bin_file != NULL);
        coding_cfg.roiMapDeltaQpBinEnable =
            (options->roi_map_info_bin_file != NULL);
        coding_cfg.RoimapCuCtrl_ver = options->roi_cu_ctrl_ver;
        coding_cfg.RoiQpDelta_ver   = options->roi_qp_delta_ver;

        /* SKIP map */
        coding_cfg.skipMapEnable = options->skip_map_enable;

        coding_cfg.enableScalingList = options->enable_scaling_list;
        coding_cfg.chroma_qp_offset  = options->chroma_qp_offset;

        /* low latency */
        coding_cfg.inputLineBufEn = (options->input_line_buf_mode > 0) ? 1 : 0;
        coding_cfg.inputLineBufLoopBackEn =
            (options->input_line_buf_mode == 1 ||
             options->input_line_buf_mode == 2) ?
                1 :
                0;
        if (options->input_line_buf_depth != DEFAULT)
            coding_cfg.inputLineBufDepth = options->input_line_buf_depth;
        coding_cfg.amountPerLoopBack    = options->amount_per_loop_back;
        coding_cfg.inputLineBufHwModeEn = (options->input_line_buf_mode == 2 ||
                                           options->input_line_buf_mode == 4) ?
                                              1 :
                                              0;
        coding_cfg.inputLineBufCbFunc = VCEncInputLineBufDone;
        coding_cfg.inputLineBufCbData = &(vpi_h26xe_cfg->input_ctb_linebuf);

        /*stream multi-segment */
        coding_cfg.streamMultiSegmentMode = options->stream_multi_segment_mode;
        coding_cfg.streamMultiSegmentAmount =
            options->stream_multi_segment_amount;
        coding_cfg.streamMultiSegCbFunc = &h26x_enc_stream_segment_ready;
        coding_cfg.streamMultiSegCbData = &(vpi_h26xe_cfg->stream_seg_ctl);

        /* denoise */
        coding_cfg.noiseReductionEnable =
            options
                ->noise_reduction_enable; /*0: disable noise reduction; 1: enable noise reduction*/
        if (options->noise_low == 0) {
            coding_cfg.noiseLow = 10;
        } else {
            coding_cfg.noiseLow =
                CLIP3(1, 30,
                      options
                          ->noise_low); /*0: use default value; valid value range: [1, 30]*/
        }

        if (options->first_frame_sigma == 0) {
            coding_cfg.firstFrameSigma = 11;
        } else {
            coding_cfg.firstFrameSigma =
                CLIP3(1, 30, options->first_frame_sigma);
        }

        /* smart */
        coding_cfg.smartModeEnable  = options->smart_mode_enable;
        coding_cfg.smartH264LumDcTh = options->smart_h264_lum_dc_th;
        coding_cfg.smartH264CbDcTh  = options->smart_h264_cb_dc_th;
        coding_cfg.smartH264CrDcTh  = options->smart_h264_cr_dc_th;
        for (i = 0; i < 3; i++) {
            coding_cfg.smartHevcLumDcTh[i] = options->smart_hevc_lum_dc_th[i];
            coding_cfg.smartHevcChrDcTh[i] = options->smart_hevc_chr_dc_th[i];
            coding_cfg.smartHevcLumAcNumTh[i] =
                options->smart_hevc_lum_ac_num_th[i];
            coding_cfg.smartHevcChrAcNumTh[i] =
                options->smart_hevc_chr_ac_num_th[i];
        }
        coding_cfg.smartH264Qp    = options->smart_h264_qp;
        coding_cfg.smartHevcLumQp = options->smart_hevc_lum_qp;
        coding_cfg.smartHevcChrQp = options->smart_hevc_chr_qp;
        for (i = 0; i < 4; i++)
            coding_cfg.smartMeanTh[i] = options->smart_mean_th[i];
        coding_cfg.smartPixNumCntTh = options->smart_pix_num_cnt_th;

        /* tile */
        coding_cfg.tiles_enabled_flag =
            options->tiles_enabled_flag && !IS_H264(options->codec_format);
        coding_cfg.num_tile_columns = options->num_tile_columns;
        coding_cfg.num_tile_rows    = options->num_tile_rows;
        coding_cfg.loop_filter_across_tiles_enabled_flag =
            options->loop_filter_across_tiles_enabled_flag;

        /* HDR10 */
        coding_cfg.Hdr10Display.hdr10_display_enable =
            options->hdr10_display_enable;
        if (options->hdr10_display_enable) {
            coding_cfg.Hdr10Display.hdr10_dx0     = options->hdr10_dx0;
            coding_cfg.Hdr10Display.hdr10_dy0     = options->hdr10_dy0;
            coding_cfg.Hdr10Display.hdr10_dx1     = options->hdr10_dx1;
            coding_cfg.Hdr10Display.hdr10_dy1     = options->hdr10_dy1;
            coding_cfg.Hdr10Display.hdr10_dx2     = options->hdr10_dx2;
            coding_cfg.Hdr10Display.hdr10_dy2     = options->hdr10_dy2;
            coding_cfg.Hdr10Display.hdr10_wx      = options->hdr10_wx;
            coding_cfg.Hdr10Display.hdr10_wy      = options->hdr10_wy;
            coding_cfg.Hdr10Display.hdr10_maxluma = options->hdr10_maxluma;
            coding_cfg.Hdr10Display.hdr10_minluma = options->hdr10_minluma;
        }

        coding_cfg.Hdr10LightLevel.hdr10_lightlevel_enable =
            options->hdr10_lightlevel_enable;
        if (options->hdr10_lightlevel_enable) {
            coding_cfg.Hdr10LightLevel.hdr10_maxlight = options->hdr10_maxlight;
            coding_cfg.Hdr10LightLevel.hdr10_avglight = options->hdr10_avglight;
        }

        coding_cfg.Hdr10Color.hdr10_color_enable = options->hdr10_color_enable;
        if (options->hdr10_color_enable) {
            coding_cfg.Hdr10Color.hdr10_matrix  = options->hdr10_matrix;
            coding_cfg.Hdr10Color.hdr10_primary = options->hdr10_primary;

            if (options->hdr10_transfer == 1)
                coding_cfg.Hdr10Color.hdr10_transfer = VCENC_HDR10_ST2084;
            else if (options->hdr10_transfer == 2)
                coding_cfg.Hdr10Color.hdr10_transfer = VCENC_HDR10_STDB67;
            else
                coding_cfg.Hdr10Color.hdr10_transfer = VCENC_HDR10_BT2020;
        }

        coding_cfg.RpsInSliceHeader = options->rps_in_slice_header;

        if ((ret = VCEncSetCodingCtrl(encoder, &coding_cfg)) != VCENC_OK) {
            VPILOGE("VCEncSetCodingCtrl failed\n");
            goto error_exit;
        }
    }

    /* Encoder setup: rate control */
    if ((ret = VCEncGetRateCtrl(encoder, &rc_cfg)) != VCENC_OK) {
        VPILOGE("VCEncGetRateCtrl failed\n");
        goto error_exit;
    } else {
        VPILOGE("Get rate control: qp %2d qpRange I[%2d, %2d] PB[%2d, %2d] %8d bps  "
                "pic %d skip %d  hrd %d  cpbSize %d bitrateWindow %d "
                "intraQpDelta %2d\n",
                rc_cfg.qpHdr, rc_cfg.qpMinI, rc_cfg.qpMaxI, rc_cfg.qpMinPB,
                rc_cfg.qpMaxPB, rc_cfg.bitPerSecond, rc_cfg.pictureRc,
                rc_cfg.pictureSkip, rc_cfg.hrd, rc_cfg.hrdCpbSize,
                rc_cfg.bitrateWindow, rc_cfg.intraQpDelta);

        if (options->qp_hdr != DEFAULT)
            rc_cfg.qpHdr = options->qp_hdr;
        else
            rc_cfg.qpHdr = -1;
        if (options->qp_min != DEFAULT)
            rc_cfg.qpMinI = rc_cfg.qpMinPB = options->qp_min;
        if (options->qp_max != DEFAULT)
            rc_cfg.qpMaxI = rc_cfg.qpMaxPB = options->qp_max;
        if (options->qp_min_I != DEFAULT)
            rc_cfg.qpMinI = options->qp_min_I;
        if (options->qp_max_I != DEFAULT)
            rc_cfg.qpMaxI = options->qp_max_I;
        if (options->pic_skip != DEFAULT)
            rc_cfg.pictureSkip = options->pic_skip;
        if (options->pic_rc != DEFAULT)
            rc_cfg.pictureRc = options->pic_rc;
        if (options->ctb_rc != DEFAULT)
            rc_cfg.ctbRc = options->ctb_rc;

        rc_cfg.blockRCSize = 0;
        if (options->block_rc_size != DEFAULT)
            rc_cfg.blockRCSize = options->block_rc_size;

        rc_cfg.rcQpDeltaRange = 10;
        if (options->rc_qp_delta_range != DEFAULT)
            rc_cfg.rcQpDeltaRange = options->rc_qp_delta_range;

        rc_cfg.rcBaseMBComplexity = 15;
        if (options->rc_base_mb_complexity != DEFAULT)
            rc_cfg.rcBaseMBComplexity = options->rc_base_mb_complexity;

        if (options->pic_qp_delta_max != DEFAULT)
            rc_cfg.picQpDeltaMax = options->pic_qp_delta_max;
        if (options->pic_qp_delta_min != DEFAULT)
            rc_cfg.picQpDeltaMin = options->pic_qp_delta_min;
        if (options->bit_per_second != DEFAULT)
            rc_cfg.bitPerSecond = options->bit_per_second;
        if (options->bit_var_range_I != DEFAULT)
            rc_cfg.bitVarRangeI = options->bit_var_range_I;
        if (options->bit_var_range_P != DEFAULT)
            rc_cfg.bitVarRangeP = options->bit_var_range_P;
        if (options->bit_var_range_B != DEFAULT)
            rc_cfg.bitVarRangeB = options->bit_var_range_B;

        if (options->tol_moving_bitrate != DEFAULT)
            rc_cfg.tolMovingBitRate = options->tol_moving_bitrate;

        if (options->tol_ctb_rc_inter != DEFAULT)
            rc_cfg.tolCtbRcInter = options->tol_ctb_rc_inter;

        if (options->tol_ctb_rc_intra != DEFAULT)
            rc_cfg.tolCtbRcIntra = options->tol_ctb_rc_intra;

        if (options->ctb_rc_row_qp_step != DEFAULT)
            rc_cfg.ctbRcRowQpStep = options->ctb_rc_row_qp_step;

        rc_cfg.longTermQpDelta = options->long_term_qp_delta;

        if (options->monitor_frames != DEFAULT)
            rc_cfg.monitorFrames = options->monitor_frames;
        else {
            rc_cfg.monitorFrames =
                (options->output_rate_numer + options->output_rate_denom - 1) /
                options->output_rate_denom;
            options->monitor_frames =
                (options->output_rate_numer + options->output_rate_denom - 1) /
                options->output_rate_denom;
        }

        if (rc_cfg.monitorFrames > MOVING_AVERAGE_FRAMES)
            rc_cfg.monitorFrames = MOVING_AVERAGE_FRAMES;

        if (rc_cfg.monitorFrames < 10) {
            rc_cfg.monitorFrames =
                (options->output_rate_numer > options->output_rate_denom) ?
                    10 :
                    LEAST_MONITOR_FRAME;
        }

        if (options->hrd_conformance != DEFAULT)
            rc_cfg.hrd = options->hrd_conformance;

        if (options->cpb_size != DEFAULT)
            rc_cfg.hrdCpbSize = options->cpb_size;

        if (options->intra_pic_rate != 0)
            rc_cfg.bitrateWindow = MIN(options->intra_pic_rate, MAX_GOP_LEN);

        if (options->bitrate_window != DEFAULT)
            rc_cfg.bitrateWindow = options->bitrate_window;

        if (options->intra_qp_delta != DEFAULT)
            rc_cfg.intraQpDelta = options->intra_qp_delta;

        if (options->vbr != DEFAULT)
            rc_cfg.vbr = options->vbr;

        if (options->crf != DEFAULT)
            rc_cfg.crf = options->crf;

        rc_cfg.fixedIntraQp    = options->fixed_intra_qp;
        rc_cfg.smoothPsnrInGOP = options->smooth_psnr_in_gop;
        rc_cfg.u32StaticSceneIbitPercent =
            options->u32_static_scene_ibit_percent;

        VPILOGE("Set rate control: qp %2d qpRange I[%2d, %2d] PB[%2d, %2d] %9d bps  "
                "pic %d skip %d  hrd %d"
                "  cpbSize %d bitrateWindow %d intraQpDelta %2d "
                "fixedIntraQp %2d\n",
                rc_cfg.qpHdr, rc_cfg.qpMinI, rc_cfg.qpMaxI, rc_cfg.qpMinPB,
                rc_cfg.qpMaxPB, rc_cfg.bitPerSecond, rc_cfg.pictureRc,
                rc_cfg.pictureSkip, rc_cfg.hrd, rc_cfg.hrdCpbSize,
                rc_cfg.bitrateWindow, rc_cfg.intraQpDelta, rc_cfg.fixedIntraQp);

        if ((ret = VCEncSetRateCtrl(encoder, &rc_cfg)) != VCENC_OK) {
            VPILOGE("VCEncSetRateCtrl failed\n");
            goto error_exit;
        }
    }

    /* Optional scaled image output */
    if (options->scaled_width * options->scaled_height > 0) {
        i32 dsFrmSize = options->scaled_width * options->scaled_height * 2;
        /* the scaled image size changes frame by frame when testing down-scaling */
        if (options->test_id == 34) {
            dsFrmSize = options->width * options->height * 2;
        }
        if ((options->bit_depth_luma != 8) || (options->bit_depth_chroma != 8))
            dsFrmSize <<= 1;

        if (options->scaled_output_format == 1)
            dsFrmSize = dsFrmSize * 3 / 4;

        if (EWLMallocRefFrm(vpi_h26xe_cfg->ewl, dsFrmSize, 0,
                            &vpi_h26xe_cfg->scaled_picture_mem) != EWL_OK) {
#ifdef USE_OLD_DRV
            vpi_h26xe_cfg->scaled_picture_mem.virtualAddress = NULL;
            vpi_h26xe_cfg->scaled_picture_mem.busAddress     = 0;
#else
            memset(&vpi_h26xe_cfg->scaled_picture_mem, 0,
                   sizeof(EWLLinearMem_t));
#endif
        }
    }

    /* PreP setup */
    if ((ret = VCEncGetPreProcessing(encoder, &pre_proc_cfg)) != VCENC_OK) {
        VPILOGE("VCEncGetPreProcessing failed\n");
        goto error_exit;
    }

    VPILOGE("Get PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d"
            "cc %d : scaling %d\n",
            pre_proc_cfg.origWidth, pre_proc_cfg.origHeight,
            pre_proc_cfg.xOffset, pre_proc_cfg.yOffset, pre_proc_cfg.inputType,
            pre_proc_cfg.rotation, pre_proc_cfg.colorConversion.type,
            pre_proc_cfg.scaledOutput);

    pre_proc_cfg.inputType = (VCEncPictureType)options->input_format;
    pre_proc_cfg.rotation  = (VCEncPictureRotation)options->rotation;
    pre_proc_cfg.mirror    = (VCEncPictureMirror)options->mirror;

    pre_proc_cfg.origWidth  = options->lum_width_src;
    pre_proc_cfg.origHeight = options->lum_height_src;

    if (options->hor_offset_src != DEFAULT)
        pre_proc_cfg.xOffset = options->hor_offset_src;
    if (options->ver_offset_src != DEFAULT)
        pre_proc_cfg.yOffset = options->ver_offset_src;

#ifdef SUPPORT_TCACHE
    if (options->input_format == INPUT_FORMAT_RFC_8BIT_COMPRESSED_FB ||
        options->input_format == INPUT_FORMAT_RFC_10BIT_COMPRESSED_FB) {
        i32 x =
            (options->hor_offset_src == DEFAULT) ? 0 : options->hor_offset_src;
        i32 y =
            (options->ver_offset_src == DEFAULT) ? 0 : options->ver_offset_src;
        i32 w = (options->width == DEFAULT) ? 0 : options->width;
        i32 h = (options->height == DEFAULT) ? 0 : options->height;
        i32 b =
            (options->input_format == INPUT_FORMAT_RFC_10BIT_COMPRESSED_FB) ?
                2 :
                1;
        i32 aw = (((w * b + 31) / 32) * 32) / b;
        i32 ah = (((y + h) + 7) / 8) * 8 - y;
        VPILOGE("options->lum_width_src = %d, x = %d, w = %d, b = %d, aw = %d\n",
                options->lum_width_src, x, w, b, aw);
        if (w) {
            pre_proc_cfg.xOffset   = 0;
            pre_proc_cfg.origWidth = MIN(aw, options->lum_width_src - x);
        }
        VPILOGE("pre_proc_cfg.xOffset = %d, pre_proc_cfg.origWidth = %d\n",
                pre_proc_cfg.xOffset, pre_proc_cfg.origWidth);
        if (ah) {
            pre_proc_cfg.yOffset    = 0;
            pre_proc_cfg.origHeight = ah;
        }
        VPILOGE("pre_proc_cfg.yOffset = %d, pre_proc_cfg.origHeight = %d\n",
                pre_proc_cfg.yOffset, pre_proc_cfg.origHeight);
    }
#endif
    if (options->input_format == INPUT_FORMAT_PP_YUV420_SEMIPLANNAR ||
        options->input_format == INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_VU ||
        options->input_format == INPUT_FORMAT_PP_YUV420_PLANAR_10BIT_P010) {
        i32 x =
            (options->hor_offset_src == DEFAULT) ? 0 : options->hor_offset_src;
        i32 y =
            (options->ver_offset_src == DEFAULT) ? 0 : options->ver_offset_src;
        i32 w = (options->width == DEFAULT) ? 0 : options->width;
        /*i32 h = (options->height == DEFAULT) ? 0 : options->height;*/
        i32 b = (options->input_format ==
                 INPUT_FORMAT_PP_YUV420_PLANAR_10BIT_P010) ?
                    2 :
                    1;
        i32 aw = (((w * b + 31) / 32) * 32) / b;
        VPILOGE("options->lum_width_src = %d, x = %d, w = %d, b = %d, aw = %d\n",
                options->lum_width_src, x, w, b, aw);
        if (x) {
            pre_proc_cfg.xOffset = x;
        }
        VPILOGE("pre_proc_cfg.xOffset = %d, pre_proc_cfg.origWidth = %d\n",
                pre_proc_cfg.xOffset, pre_proc_cfg.origWidth);
        if (y) {
            pre_proc_cfg.yOffset = y;
        }
        VPILOGE("pre_proc_cfg.yOffset = %d, pre_proc_cfg.origHeight = %d\n",
                pre_proc_cfg.yOffset, pre_proc_cfg.origHeight);
    }
    /*add for crop end*/
    if (options->interlaced_frame)
        pre_proc_cfg.origHeight /= 2;

    if (options->color_conversion != DEFAULT)
        pre_proc_cfg.colorConversion.type =
            (VCEncColorConversionType)options->color_conversion;
    if (pre_proc_cfg.colorConversion.type == VCENC_RGBTOYUV_USER_DEFINED) {
        pre_proc_cfg.colorConversion.coeffA     = 20000;
        pre_proc_cfg.colorConversion.coeffB     = 44000;
        pre_proc_cfg.colorConversion.coeffC     = 5000;
        pre_proc_cfg.colorConversion.coeffE     = 35000;
        pre_proc_cfg.colorConversion.coeffF     = 38000;
        pre_proc_cfg.colorConversion.coeffG     = 35000;
        pre_proc_cfg.colorConversion.coeffH     = 38000;
        pre_proc_cfg.colorConversion.LumaOffset = 0;
    }

    if (options->rotation && options->rotation != 3) {
        pre_proc_cfg.scaledWidth  = options->scaled_height;
        pre_proc_cfg.scaledHeight = options->scaled_width;
    } else {
        pre_proc_cfg.scaledWidth  = options->scaled_width;
        pre_proc_cfg.scaledHeight = options->scaled_height;
    }
    pre_proc_cfg.busAddressScaledBuff =
        vpi_h26xe_cfg->scaled_picture_mem.busAddress;
#ifdef USE_OLD_DRV
    pre_proc_cfg.virtualAddressScaledBuff =
        vpi_h26xe_cfg->scaled_picture_mem.virtualAddress;
#else
    pre_proc_cfg.virtualAddressScaledBuff =
        vpi_h26xe_cfg->scaled_picture_mem.rc_virtualAddress;
#endif
    pre_proc_cfg.sizeScaledBuff     = vpi_h26xe_cfg->scaled_picture_mem.size;
    pre_proc_cfg.input_alignment    = 1 << options->exp_of_input_alignment;
    pre_proc_cfg.scaledOutputFormat = options->scaled_output_format;

    VPILOGE("Set PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d"
            "cc %d : scaling %d\n",
            pre_proc_cfg.origWidth, pre_proc_cfg.origHeight,
            pre_proc_cfg.xOffset, pre_proc_cfg.yOffset, pre_proc_cfg.inputType,
            pre_proc_cfg.rotation, pre_proc_cfg.colorConversion.type,
            pre_proc_cfg.scaledOutput, pre_proc_cfg.scaledOutputFormat);

    if (options->scaled_width * options->scaled_height > 0)
        pre_proc_cfg.scaledOutput = 1;

    /* constant chroma control */
    pre_proc_cfg.constChromaEn = options->const_chroma_en;
    if (options->const_cb != DEFAULT)
        pre_proc_cfg.constCb = options->const_cb;
    if (options->const_cr != DEFAULT)
        pre_proc_cfg.constCr = options->const_cr;

    change_to_customized_format(options, &pre_proc_cfg);

    change_format_for_FB(vpi_h26xe_cfg, options, &pre_proc_cfg);

    if (cfg.extDSRatio) {
        pre_proc_cfg.origWidth_ds  = options->lum_widthsrc_ds;
        pre_proc_cfg.origHeight_ds = options->lum_heightsrc_ds;
    }

    if ((ret = VCEncSetPreProcessing(encoder, &pre_proc_cfg)) != VCENC_OK) {
        VPILOGE("VCEncSetPreProcessing failed\n");
        goto error_exit;
    }
    ret = 0;

error_exit:
    return ret;
}

/**
 *  h26x_enc_close_encoder
 *  Release an encoder insatnce.
 *
 *  @Params: encoder the instance to be released
 */
static void h26x_enc_close_encoder(VCEncInst encoder,
                                   struct VPIH26xEncCfg *vpi_h26xe_cfg)
{
    VCEncRet ret;

#ifdef USE_OLD_DRV
    if (vpi_h26xe_cfg->scaled_picture_mem.virtualAddress != NULL)
#endif
        EWLFreeLinear(vpi_h26xe_cfg->ewl, &vpi_h26xe_cfg->scaled_picture_mem);

    if (encoder) {
        if ((ret = VCEncRelease(encoder)) != VCENC_OK) {
        }
    }

    if (vpi_h26xe_cfg->ewl) {
        EWLRelease(vpi_h26xe_cfg->ewl);
        vpi_h26xe_cfg->ewl = NULL;
    }

    if (vpi_h26xe_cfg->two_pass_ewl) {
        EWLRelease(vpi_h26xe_cfg->two_pass_ewl);
        vpi_h26xe_cfg->two_pass_ewl = NULL;
    }

    if (vpi_h26xe_cfg->perf) {
        ENCPERF *perf = vpi_h26xe_cfg->perf;
        pthread_mutex_destroy(&perf->hwcycle_acc_mutex);
#ifdef FB_PERFORMANCE_STATIC
        PERFORMANCE_STATIC_REPORT(vpi_h26xe_cfg, perf, vcehwp1);
        PERFORMANCE_STATIC_VERBOSE(vpi_h26xe_cfg, perf, vcehwp1);
        PERFORMANCE_STATIC_REPORT(vpi_h26xe_cfg, perf, vcehw);
        PERFORMANCE_STATIC_VERBOSE(vpi_h26xe_cfg, perf, vcehw);
        PERFORMANCE_STATIC_REPORT(vpi_h26xe_cfg, perf, CU_ANAL);
        PERFORMANCE_STATIC_VERBOSE(vpi_h26xe_cfg, perf, CU_ANAL);
        PERFORMANCE_STATIC_REPORT(vpi_h26xe_cfg, perf, vcehw_total);
        PERFORMANCE_STATIC_VERBOSE(vpi_h26xe_cfg, perf, vcehw_total);
        PERFORMANCE_STATIC_REPORT(vpi_h26xe_cfg, perf, vce_total);
        PERFORMANCE_STATIC_VERBOSE(vpi_h26xe_cfg, perf, vce_total);
#endif
        free(vpi_h26xe_cfg->perf);
    }
}

/**
 *  h26x_enc_slice_ready
 *
 *  Callback function called by the encoder SW after "slice ready"
 *  interrupt from HW. Note that this function is not necessarily called
 *  after every slice i.e. it is possible that two or more slices are
 *  completed between callbacks.
 */
static void h26x_enc_slice_ready(VCEncSliceReady *slice)
{
    u32 i;
    u32 stream_size;
    u32 pos;
    SliceCtl *ctl = (SliceCtl *)slice->pAppData;
    /* Here is possible to implement low-latency streaming by
     *    * sending the complete slices before the whole frame is completed. */
    if (ctl->multislice_encoding && (ENCH2_SLICE_READY_INTERRUPT)) {
        pos = slice->slicesReadyPrev ?
                  ctl->stream_pos : /* Here we store the slice pointer */
                  0; /* Pointer to beginning of frame */
        stream_size = 0;
        for (i = slice->nalUnitInfoNumPrev; i < slice->nalUnitInfoNum; i++) {
            stream_size += *(slice->sliceSizes + i);
        }

        pos += stream_size;
        /* Store the slice pointer for next callback */
        ctl->stream_pos = pos;
    }
}

/**
 *  h26x_enc_call_vcstart
 *  Call vc8000e sdk's VCEncStrmStart() function
 */
static int h26x_enc_call_vcstart(struct VpiH26xEncCtx *enc_ctx,
                                 VCEncOut *enc_out)
{
    VPIH26xEncOptions *options          = &enc_ctx->options;
    i32 ret                             = OK;
    struct VPIH26xEncCfg *vpi_h26xe_cfg = &enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in                   = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);
    int *stream_size                    = &enc_ctx->stream_size;
    *stream_size                        = 0;

    VPILOGE("&ctx_ctx->vpi_h26xe_cfg[%p]\n", (void *)&enc_ctx->vpi_h26xe_cfg);
    i32 p   = 0;
    int cnt = 1;

    u32 gop_size          = vpi_h26xe_cfg->gop_size;
    enc_ctx->adaptive_gop = (gop_size == 0);

    memset(&enc_ctx->agop, 0, sizeof(enc_ctx->agop));
    enc_ctx->agop.last_gopsize = MAX_ADAPTIVE_GOP_SIZE;

    p_enc_in->gopSize = gop_size;

    enc_ctx->ma.pos = enc_ctx->ma.count = 0;
    enc_ctx->ma.frame_rate_numer        = options->output_rate_numer;
    enc_ctx->ma.frame_rate_denom        = options->output_rate_denom;
    if (options->output_rate_denom)
        enc_ctx->ma.length =
            MAX(LEAST_MONITOR_FRAME,
                MIN(options->monitor_frames, MOVING_AVERAGE_FRAMES));
    else
        enc_ctx->ma.length = MOVING_AVERAGE_FRAMES;

    p_enc_in->timeIncrement          = 0;
    p_enc_in->vui_timing_info_enable = options->vui_timing_info_enable;

    setup_output_buffer(vpi_h26xe_cfg, p_enc_in);

    p_enc_in->hashType = options->hashtype;
    init_slice_ctl(vpi_h26xe_cfg, options);
    init_stream_segment_crl(vpi_h26xe_cfg, options);

    if (options->input_line_buf_mode) {
        if (h26x_enc_init_input_line_buffer(&(vpi_h26xe_cfg->input_ctb_linebuf),
                                            options, p_enc_in,
                                            enc_ctx->hantro_encoder,
                                            vpi_h26xe_cfg)) {
            goto error;
        }
    }

    /* before VCEncStrmStart called */
    h26x_enc_init_pic_config(p_enc_in);

    /* Video, sequence and picture parameter sets */
    for (p = 0; p < cnt; p++) {
        if (VCEncStrmStart(enc_ctx->hantro_encoder, p_enc_in, enc_out)) {
            goto error;
        }

        enc_ctx->total_bits += enc_out->streamSize * 8;
        *stream_size += enc_out->streamSize;
    }
    return ret;

error:
    VPILOGE("%s error\n", __FUNCTION__);
    return -1;
}

/**
 *  h26x_enc_call_vcencoding
 *  Call vc8000e sdk's VCEncStrmEncode() function
 */
static int h26x_enc_call_vcencoding(struct VpiH26xEncCtx *enc_ctx,
                                    VpiH26xEncInAddr *p_addrs, void *indata,
                                    void *outdata)
{
    VPIH26xEncOptions *options = &enc_ctx->options;
    struct VPIH26xEncCfg *vpi_h26xe_cfg =
        (struct VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;
    int *stream_size       = &enc_ctx->stream_size;
    VpiH26xEncInAddr addrs = *p_addrs;
    VCEncIn *p_enc_in      = (VCEncIn *)indata;
    VCEncOut *enc_out      = (VCEncOut *)outdata;

    i32 ret = OK;

    /* IO buffer */
    get_free_iobuffer(vpi_h26xe_cfg);
    setup_slice_ctl(vpi_h26xe_cfg);

    /* Setup encoder input */
    setup_input_buffer(vpi_h26xe_cfg, options, p_enc_in);

    setup_output_buffer(vpi_h26xe_cfg, p_enc_in);

#ifdef SUPPORT_TCACHE
    if ((options->input_format >= INPUT_FORMAT_ARGB_FB &&
         options->input_format <= INPUT_FORMAT_YUV444P) ||
        options->input_format == VCENC_YUV420_PLANAR ||
        options->input_format == VCENC_YUV420_SEMIPLANAR ||
        options->input_format == VCENC_YUV420_SEMIPLANAR_VU ||
        options->input_format == VCENC_YUV420_PLANAR_10BIT_P010) {
    } else
#endif
    {
#ifdef USE_OLD_DRV
#endif
    }

    if (*stream_size) {
        p_enc_in->busOutBuf[0] += *stream_size;
        p_enc_in->pOutBuf[0] = (u32 *)p_enc_in->pOutBuf + *stream_size;
        p_enc_in->outBufSize[0] -= *stream_size;
    }

    if (vpi_h26xe_cfg->enc_index == 0 && options->lookahead_depth &&
        options->cutree_blkratio) {
        if (addrs.bus_luma_ds == 0 || addrs.bus_chroma_ds == 0) {
            return -1;
        }
        p_enc_in->busLuma    = addrs.bus_luma_ds;
        p_enc_in->busChromaU = addrs.bus_chroma_ds;
        p_enc_in->busChromaV = 0;
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
        vpi_h26xe_cfg->ts_lumamem->busAddress   = addrs.bus_luma_table_ds;
        vpi_h26xe_cfg->ts_chromamem->busAddress = addrs.bus_chroma_table_ds;
#endif

        p_enc_in->busLumaOrig    = addrs.bus_luma;
        p_enc_in->busChromaUOrig = addrs.bus_chroma;
        p_enc_in->busChromaVOrig = 0;

    } else {
        p_enc_in->busLuma    = addrs.bus_luma;
        p_enc_in->busChromaU = addrs.bus_chroma;
        p_enc_in->busChromaV = 0;
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
        vpi_h26xe_cfg->ts_lumamem->busAddress   = addrs.bus_luma_table;
        vpi_h26xe_cfg->ts_chromamem->busAddress = addrs.bus_chroma_table;
#endif
    }
    VPILOGE("%s %d\n", __FILE__, __LINE__);
    VPILOGE("p_enc_in->busLuma    = %p\n", p_enc_in->busLuma);
    VPILOGE("p_enc_in->busChromaU = %p\n", p_enc_in->busChromaU);
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
    VPILOGE("TSLumaMem->busAddr = %p\n", vpi_h26xe_cfg->ts_lumamem->busAddress);
    VPILOGE("ts_chromamem->busAd = %p\n",
            vpi_h26xe_cfg->ts_chromamem->busAddress);
#endif

    enc_ctx->frame_cnt_total++;

    format_customized_yuv(vpi_h26xe_cfg, options, &ret);
    if (ret) {
        goto error;
    }
    /*
     *   per-frame test functions
     *
     */

    /* 1. scene changed frames from usr */
    p_enc_in->sceneChange = 0;
    u32 tmp = h26x_enc_next_picture(vpi_h26xe_cfg, vpi_h26xe_cfg->picture_cnt) +
              vpi_h26xe_cfg->first_pic;
    for (int i = 0; i < MAX_SCENE_CHANGE; i++) {
        if (options->scene_change[i] == 0) {
            break;
        }
        if (options->scene_change[i] == tmp) {
            p_enc_in->sceneChange = 1;
            break;
        }
    }
    /* 2. GMV setting from user */
    read_gmv(vpi_h26xe_cfg, p_enc_in, options);

    p_enc_in->codingType =
        (p_enc_in->poc == 0) ? VCENC_INTRA_FRAME : enc_ctx->next_coding_type;
#ifdef TEST_ENCODER_ONLY
#else
#endif
    vpi_h26xe_cfg->input_pic_cnt++;

    if (p_enc_in->sceneChange) {
    }

    if (p_enc_in->codingType == VCENC_INTRA_FRAME &&
        options->gdr_duration == 0 &&
        ((p_enc_in->poc == 0) || (p_enc_in->bIsIDR))) {
        /*refrest IDR poc*/
        p_enc_in->poc = 0;
        if (!options->lookahead_depth)
            enc_ctx->frame_cnt_output = 0;
        vpi_h26xe_cfg->last_idr_picture_cnt = vpi_h26xe_cfg->picture_cnt;
    }
    /* 3. On-fly bitrate setting */
    for (int i = 0; i < MAX_BPS_ADJUST; i++)
        if (options->bps_adjust_frame[i] &&
            (vpi_h26xe_cfg->picture_cnt == options->bps_adjust_frame[i])) {
            enc_ctx->vpi_h26xe_cfg.rc.bitPerSecond =
                options->bps_adjust_bitrate[i];
            if ((ret = VCEncSetRateCtrl(enc_ctx->hantro_encoder,
                                        (VCEncRateCtrl *)&enc_ctx->vpi_h26xe_cfg
                                            .rc)) != VCENC_OK) {
            }
        }
    /* 4. SetupROI-Map */
    if (setup_roi_map_buffer(vpi_h26xe_cfg, options, p_enc_in,
                             enc_ctx->hantro_encoder)) {
        goto error;
    }

    /* 5. encoding specific frame from user: all CU/MB are SKIP */
    p_enc_in->bSkipFrame = options->skip_frame_enabled_flag &&
                           (p_enc_in->poc == options->skip_frame_poc);

    /* 6. low latency */
    if (options->input_line_buf_mode) {
        p_enc_in->lineBufWrCnt =
            VCEncStartInputLineBuffer(&(vpi_h26xe_cfg->input_ctb_linebuf));
    }
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
    p_enc_in->PrivData.PicMemRcBusAddr =
        vpi_h26xe_cfg->picture_mem->rc_busAddress;

    p_enc_in->PrivData.bitDepthLuma    = options->bit_depth_luma;
    p_enc_in->PrivData.input_alignment = vpi_h26xe_cfg->input_alignment;

    if (addrs.bus_luma_ds) {
        p_enc_in->PrivData.lumaSize     = vpi_h26xe_cfg->luma_size_ds;
        p_enc_in->PrivData.chromaSize   = vpi_h26xe_cfg->chroma_size_ds;
        p_enc_in->PrivData.lumWidthSrc  = options->lum_width_src / 2;
        p_enc_in->PrivData.lumHeightSrc = options->lum_height_src / 2;
        p_enc_in->PrivData.inputFormat  = options->input_format_ds;

        p_enc_in->PrivData.TSLumaMemBusAddress   = addrs.bus_luma_table_ds;
        p_enc_in->PrivData.TSChromaMemBusAddress = addrs.bus_chroma_table_ds;
        p_enc_in->PrivData.busLuma               = addrs.bus_luma_ds;
        p_enc_in->PrivData.busChromaU            = addrs.bus_chroma_ds;

    } else {
        p_enc_in->PrivData.lumaSize     = vpi_h26xe_cfg->luma_size;
        p_enc_in->PrivData.chromaSize   = vpi_h26xe_cfg->chroma_size;
        p_enc_in->PrivData.lumWidthSrc  = options->lum_width_src;
        p_enc_in->PrivData.lumHeightSrc = options->lum_height_src;
        p_enc_in->PrivData.inputFormat  = options->input_format;

        p_enc_in->PrivData.TSLumaMemBusAddress =
            addrs.bus_luma_table; /*pic_data->luma_table.bus_address; */
        p_enc_in->PrivData.TSChromaMemBusAddress = addrs.bus_chroma_table;
        /*pic_data->chroma_table.bus_address; */ /*vpi_h26xe_cfg->ts_chromamem->busAddress;*/
        p_enc_in->PrivData.busLuma = addrs.bus_luma;
        /*pic_data->luma.bus_address; */ /*p_enc_in->busLuma;*/
        p_enc_in->PrivData.busChromaU = addrs.bus_chroma;
        /*pic_data->chroma.bus_address; */ /*p_enc_in->busChromaU;*/
    }
    /* for pass two encoder hardware set */
    p_enc_in->PassTwoHWData.bitDepthLuma    = options->bit_depth_luma;
    p_enc_in->PassTwoHWData.inputFormat     = options->input_format;
    p_enc_in->PassTwoHWData.lumaSize        = vpi_h26xe_cfg->luma_size;
    p_enc_in->PassTwoHWData.chromaSize      = vpi_h26xe_cfg->chroma_size;
    p_enc_in->PassTwoHWData.lumWidthSrc     = options->lum_width_src;
    p_enc_in->PassTwoHWData.lumHeightSrc    = options->lum_height_src;
    p_enc_in->PassTwoHWData.input_alignment = vpi_h26xe_cfg->input_alignment;
    p_enc_in->PassTwoHWData.TSLumaMemBusAddress = addrs.bus_luma_table;
    /*pic_data->luma_table.bus_address; */ /*vpi_h26xe_cfg->ts_lumamem->busAddress;*/
    p_enc_in->PassTwoHWData.TSChromaMemBusAddress = addrs.bus_chroma_table;
    /*pic_data->chroma_table.bus_address; */ /*vpi_h26xe_cfg->ts_chromamem->busAddress;*/
    p_enc_in->PassTwoHWData.PicMemRcBusAddr =
        vpi_h26xe_cfg->picture_mem->rc_busAddress;
    p_enc_in->PassTwoHWData.busLuma = addrs.bus_luma;
    /*pic_data->luma.bus_address; */ /*p_enc_in->busLuma;*/
    p_enc_in->PassTwoHWData.busChromaU =
        addrs.bus_chroma; /*pic_data->chroma.bus_address; p_enc_in->busChromaU;*/

    if (options->input_format == INPUT_FORMAT_RFC_8BIT_COMPRESSED_FB ||
        options->input_format == INPUT_FORMAT_RFC_10BIT_COMPRESSED_FB) {
        i32 x =
            (options->hor_offset_src == DEFAULT) ? 0 : options->hor_offset_src;
        i32 y =
            (options->ver_offset_src == DEFAULT) ? 0 : options->ver_offset_src;
        i32 w = (options->width == DEFAULT) ? 0 : options->width;
        i32 h = (options->height == DEFAULT) ? 0 : options->height;
        i32 b =
            (options->input_format == INPUT_FORMAT_RFC_10BIT_COMPRESSED_FB) ?
                2 :
                1;
        i32 aw = (((w * b + 31) / 32) * 32) / b;
        i32 ah = (((y + h) + 7) / 8) * 8 - y;
        if (w) {
            p_enc_in->PassTwoHWData.crop_x = x;
            p_enc_in->PassTwoHWData.crop_w =
                MIN(aw, options->lum_width_src - x);
        }
        if (ah) {
            p_enc_in->PassTwoHWData.crop_y = y;
            p_enc_in->PassTwoHWData.crop_h = ah;
        }
        if ((p_enc_in->PassTwoHWData.crop_x ||
             p_enc_in->PassTwoHWData.crop_w) &&
            p_enc_in->PassTwoHWData.crop_y == 0 &&
            p_enc_in->PassTwoHWData.crop_h == 0) {
            p_enc_in->PassTwoHWData.crop_h =
                p_enc_in->PassTwoHWData.lumHeightSrc;
        }
        if ((p_enc_in->PassTwoHWData.crop_y ||
             p_enc_in->PassTwoHWData.crop_h) &&
            p_enc_in->PassTwoHWData.crop_x == 0 &&
            p_enc_in->PassTwoHWData.crop_w == 0) {
            p_enc_in->PassTwoHWData.crop_w =
                p_enc_in->PassTwoHWData.lumWidthSrc;
        }
        if (p_enc_in->PassTwoHWData.crop_x == 0 &&
            p_enc_in->PassTwoHWData.crop_w == options->lum_width_src &&
            p_enc_in->PassTwoHWData.crop_y == 0 &&
            p_enc_in->PassTwoHWData.crop_h == options->lum_height_src) {
            p_enc_in->PassTwoHWData.crop_w = 0;
            p_enc_in->PassTwoHWData.crop_h = 0;
        }
        p_enc_in->PrivData.crop_x = p_enc_in->PassTwoHWData.crop_x;
        p_enc_in->PrivData.crop_y = p_enc_in->PassTwoHWData.crop_y;
        p_enc_in->PrivData.crop_w = p_enc_in->PassTwoHWData.crop_w;
        p_enc_in->PrivData.crop_h = p_enc_in->PassTwoHWData.crop_h;
    }
#endif
    gettimeofday(&vpi_h26xe_cfg->time_frame_start, 0);
    ret = VCEncStrmEncode(enc_ctx->hantro_encoder, p_enc_in, enc_out,
                          &h26x_enc_slice_ready, vpi_h26xe_cfg->slice_ctl);
    gettimeofday(&vpi_h26xe_cfg->time_frame_end, 0);
    return ret;
error:
    VPILOGE("%s %d: goto error\n", __FILE__, __LINE__);
    return -1;
}

/**
 *  h26x_enc_call_vcflush
 *  Call vc8000e sdk's VCEncFlush() function
 */
static int h26x_enc_call_vcflush(struct VpiH26xEncCtx *enc_ctx,
                                 VCEncOut *enc_out)
{
    i32 ret                             = OK;
    struct VPIH26xEncCfg *vpi_h26xe_cfg = &enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in                   = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);

    ret = VCEncFlush(enc_ctx->hantro_encoder, p_enc_in, enc_out,
                     &h26x_enc_slice_ready);
    {
        switch (ret) {
        case VCENC_FRAME_READY:
            setup_output_buffer(vpi_h26xe_cfg, p_enc_in);
            setup_slice_ctl(vpi_h26xe_cfg);
            vpi_h26xe_cfg->picture_enc_cnt++;
            if (enc_out->streamSize == 0) {
                vpi_h26xe_cfg->picture_cnt++;
                break;
            }
#ifndef USE_OLD_DRV
            if (EWLTransDataEP2RC(vpi_h26xe_cfg->ewl,
                                  vpi_h26xe_cfg->outbuf_mem[0],
                                  vpi_h26xe_cfg->outbuf_mem[0],
                                  enc_out->streamSize))
                ret = -1;
#endif
            break;
        case VCENC_FRAME_ENQUEUE:
            break;
        case VCENC_OUTPUT_BUFFER_OVERFLOW:
            vpi_h26xe_cfg->picture_cnt++;
            break;
        case VCENC_OK:
            /*ctx->flushState = VPIH26X_FLUSH_FINISH;*/
            break;
        default:
            ret = -1;
        }
    }
    return ret;
}

/**
 *  h26x_enc_call_vcend
 *  Call vc8000e sdk's VCEncStrmEnd() function
 */
static int h26x_enc_call_vcend(struct VpiH26xEncCtx *enc_ctx, VCEncOut *enc_out)
{
    i32 ret                             = OK;
    struct VPIH26xEncCfg *vpi_h26xe_cfg = &enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in                   = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);

    setup_output_buffer(vpi_h26xe_cfg, p_enc_in);
    ret = VCEncStrmEnd(enc_ctx->hantro_encoder, p_enc_in, enc_out);
    return ret;
}

/**
 *  h26x_enc_start
 *  Start h26x(h264/hevc) encoding
 */
static int h26x_enc_start(struct VpiH26xEncCtx *enc_ctx, VCEncOut *enc_out)
{
    VPIH26xEncOptions *options = &enc_ctx->options;
    i32 ret                    = OK;
    struct VPIH26xEncCfg *vpi_h26xe_cfg =
        (struct VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);

    h26x_enc_call_vcstart(enc_ctx, enc_out);

    p_enc_in->poc = 0;

    /*default gop size as IPPP*/
    u32 gop_size = vpi_h26xe_cfg->gop_size;
    p_enc_in->gopSize = enc_ctx->next_gop_size =
        (enc_ctx->
         adaptive_gop ? (options->lookahead_depth ? 4 : 1) : gop_size);
    VPILOGE("%s %d p_enc_in->gopSize = %d\n", __FILE__, __LINE__,
            p_enc_in->gopSize);

    VCEncGetRateCtrl(enc_ctx->hantro_encoder,
                     (VCEncRateCtrl *)&enc_ctx->vpi_h26xe_cfg.rc);

    /* Allocate a buffer for user data and read data from file */
    enc_ctx->p_user_data =
        read_userdata(enc_ctx->hantro_encoder, options->user_data);

    vpi_h26xe_cfg->validencoded_framenumber = 0;

    /* Read configuration files for ROI/CuTree/IPCM/GMV ... */
    if (read_config_files(vpi_h26xe_cfg, options))
        goto error;

    return ret;

error:
    VPILOGE("%s error\n", __FUNCTION__);
    return -1;
}

/**
 *  h26x_enc_encoding
 *  The encoding for h26x(h264/hevc) encoder
 */
static int h26x_enc_encode(struct VpiH26xEncCtx *enc_ctx,
                           VpiH26xEncInAddr *p_addrs, VCEncOut *enc_out)
{
    VPIH26xEncOptions *options = &enc_ctx->options;
    i32 ret                    = OK;
    struct VPIH26xEncCfg *vpi_h26xe_cfg =
        (struct VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);
    int *stream_size  = &enc_ctx->stream_size;
    *stream_size      = 0;

    if (vpi_h26xe_cfg->last_pic > 0 &&
        enc_ctx->next_poc > vpi_h26xe_cfg->last_pic) {
        VPILOGE("next_poc = %d, exceed last_pic %d\n", enc_ctx->next_poc,
                vpi_h26xe_cfg->last_pic);
        return -1;
    }

    if (p_addrs->bus_luma == 0 || p_addrs->bus_chroma == 0) {
        VPILOGE("%s(%d) get EOS\n", __FUNCTION__, __LINE__);

        return 0;
    }

    gettimeofday(&vpi_h26xe_cfg->time_frame_start, 0);
    ret = h26x_enc_call_vcencoding(enc_ctx, p_addrs, p_enc_in, enc_out);
    gettimeofday(&vpi_h26xe_cfg->time_frame_end, 0);
    switch (ret) {
    case VCENC_FRAME_ENQUEUE:
        if (enc_ctx->adaptive_gop && options->lookahead_depth) {
            get_next_gop_size(vpi_h26xe_cfg, p_enc_in, enc_ctx->hantro_encoder,
                              &enc_ctx->next_gop_size, &enc_ctx->agop);
        } else if (options
                       ->lookahead_depth) { /* for sync only, not update gopSize*/
            getPass1UpdatedGopSize(
                ((struct vcenc_instance *)enc_ctx->hantro_encoder)
                    ->lookahead.priv_inst);
        }
        if (enc_ctx->flush_state == VPIH26X_FLUSH_TRANSPIC) {
            enc_ctx->next_gop_size = 1;
        }

        enc_ctx->enc_in_bk      = *p_enc_in;
        enc_ctx->picture_cnt_bk = vpi_h26xe_cfg->picture_cnt;
        enc_ctx->next_coding_type =
            VCEncFindNextPic(enc_ctx->hantro_encoder, p_enc_in,
                             enc_ctx->next_gop_size,
                             vpi_h26xe_cfg->enc_in.gopConfig.gopCfgOffset,
                             false);
        vpi_h26xe_cfg->picture_cnt = p_enc_in->picture_cnt;
        p_enc_in->timeIncrement = vpi_h26xe_cfg->output_rate_denom;
        break;
    case VCENC_FRAME_READY:
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
        if (ReleasePass2InputHwTransformer(enc_ctx->hantro_encoder,
                                           &p_enc_in->PassTwoHWData) < 0) {
            goto error;
        }
#endif
        VPILOGE("%s [%d] enc_out->indexEncoded = %d\n", __FILE__, __LINE__,
                enc_out->indexEncoded);
        VPILOGE("enc_out->codingType = %d\n", enc_out->codingType);
        VPILOGE("enc_out->streamSize = %d\n", enc_out->streamSize);

        if (enc_out->codingType != VCENC_NOTCODED_FRAME)
            vpi_h26xe_cfg->picture_enc_cnt++;
        if (enc_out->streamSize == 0) {
            vpi_h26xe_cfg->picture_cnt++;
            break;
        }

        VPILOGE("%s %d\n", __FILE__, __LINE__);

        *stream_size += enc_out->streamSize;

        VPILOGE("%s %d, *stream_size = %d\n", __FILE__, __LINE__, *stream_size);
#ifndef USE_OLD_DRV
        if (EWLTransDataEP2RC(vpi_h26xe_cfg->ewl, vpi_h26xe_cfg->outbuf_mem[0],
                              vpi_h26xe_cfg->outbuf_mem[0], *stream_size))
            goto error;
#endif

        VPILOGE("%s %d ff_vpe_h26xe_processFrame over\n", __FILE__, __LINE__);

        if (enc_ctx->adaptive_gop)
            get_next_gop_size(vpi_h26xe_cfg, p_enc_in, enc_ctx->hantro_encoder,
                              &enc_ctx->next_gop_size, &enc_ctx->agop);
        else if (options->lookahead_depth) /* for sync only, not update gopSize*/
            getPass1UpdatedGopSize(
                ((struct vcenc_instance *)enc_ctx->hantro_encoder)
                    ->lookahead.priv_inst);

        if (enc_ctx->flush_state == VPIH26X_FLUSH_TRANSPIC) {
            enc_ctx->next_gop_size = 1;
        }

        enc_ctx->enc_in_bk      = *p_enc_in;
        enc_ctx->picture_cnt_bk = vpi_h26xe_cfg->picture_cnt;

        enc_ctx->next_coding_type =
            VCEncFindNextPic(enc_ctx->hantro_encoder, p_enc_in,
                             enc_ctx->next_gop_size,
                             vpi_h26xe_cfg->enc_in.gopConfig.gopCfgOffset,
                             false);
        vpi_h26xe_cfg->picture_cnt = p_enc_in->picture_cnt;

        VPILOGE("%s %d p_enc_in->gopSize = %d, vpi_h26xe_cfg->next_gop_size = %d\n",
                __FILE__, __LINE__, p_enc_in->gopSize,
                vpi_h26xe_cfg->next_gop_size);

        if (enc_ctx->p_user_data) {
            /* We want the user data to be written only once so
 *              * we disable the user data and free the memory after
 *                           * first frame has been encoded. */
            VCEncSetSeiUserData(enc_ctx->hantro_encoder, NULL, 0);
            free(enc_ctx->p_user_data);
            enc_ctx->p_user_data = NULL;
        }
        break;
    case VCENC_OUTPUT_BUFFER_OVERFLOW:
        vpi_h26xe_cfg->picture_cnt++;
        break;
    default:
        goto error;
        break;
    }

    VPILOGE("%s %d enc_ctx->frame_cnt_total[%d] \n", __FILE__, __LINE__,
            enc_ctx->frame_cnt_total);

    if (options->profile == VCENC_HEVC_MAIN_STILL_PICTURE_PROFILE)
        return ret;
    return ret;
error:
    return -1;
}

/**
 *  h26x_enc_flush
 *  The flush for h26x(h264/hevc) encoder
 */
static int h26x_enc_flush(struct VpiH26xEncCtx *enc_ctx, VCEncOut *enc_out)
{
    i32 ret                    = OK;

    ret = h26x_enc_call_vcflush(enc_ctx, enc_out);
    if (ret < 0) {
        enc_ctx->flush_state = VPIH26X_FLUSH_FINISH;
        return -1;
    }

    if ((VCENC_FRAME_READY == ret) && (enc_out->streamSize != 0)) {
        /*ff_vpe_h26xe_processFrame(&ctx->vpi_h26xe_ctx, pkt, &enc_out->streamSize, NULL);
 *          *got_packet = 1;
 *                   hantro_consume_stored_pic(avctx, enc_out->indexEncoded);*/
    }
    if ((VCENC_FRAME_ENQUEUE == ret) || (VCENC_OK == ret)) {
        /*got_packet = 1;*/
        if (VCENC_OK == ret) {
            enc_ctx->flush_state = VPIH26X_FLUSH_FINISH;
        }
    }

    return ret;
}

/**
 *  h26x_enc_flush_set
 *  Set the flush state to FLUSH_TRANSPIC for h26x(h264/hevc) encoder
 */
static int h26x_enc_flush_set(struct VpiH26xEncCtx *enc_ctx)
{
    struct VPIH26xEncCfg *vpi_h26xe_cfg =
        (struct VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);

    if (enc_ctx->flush_state == VPIH26X_FLUSH_PREPARE) {
        VPILOGE("%s(%d) get EOS, begin to flush trans pic\n", __FUNCTION__,
                __LINE__);
        enc_ctx->flush_state = VPIH26X_FLUSH_TRANSPIC;

        /*hantro_rest_data_check(avctx); */ /*to check for debug*/

        *p_enc_in                  = enc_ctx->enc_in_bk;
        vpi_h26xe_cfg->picture_cnt = enc_ctx->picture_cnt_bk;
        enc_ctx->next_coding_type =
            VCEncFindNextPic(enc_ctx->hantro_encoder, p_enc_in, 1,
                             vpi_h26xe_cfg->enc_in.gopConfig.gopCfgOffset,
                             false);
        vpi_h26xe_cfg->picture_cnt = p_enc_in->picture_cnt;
        p_enc_in->gopSize       = 1;
        p_enc_in->timeIncrement = vpi_h26xe_cfg->output_rate_denom;
    }

    return 0;
}

/**
 *  h26x_enc_end
 *  The end for h26x(h264/hevc) encoder
 */
static int h26x_enc_end(struct VpiH26xEncCtx *enc_ctx, VCEncOut *enc_out)
{
    if (enc_ctx->encoder_is_end == HANTRO_TRUE)
        return 0;

    h26x_enc_call_vcend(enc_ctx, enc_out);
    enc_ctx->encoder_is_end = HANTRO_TRUE;
    return 0;
}

/**
 *  vpi_h26xe_init
 *  Initialize the h26x(h264/hevc) encoder
 *
 *  @Params: enc_ctx The context of Vpi H26x encoder
 *  @Params: enc_cfg The configure of the encoder
 *  @Return: 0 for success, -1 for error
 */
int vpi_h26xe_init(struct VpiH26xEncCtx *enc_ctx, VpiH26xEncCfg *enc_cfg)
{
    i32 ret = OK;
    i32 i   = 0;
    int max_frames_delay;

    VCEncInst *hantro_encoder  = &enc_ctx->hantro_encoder;
    VPIH26xEncOptions *options = &enc_ctx->options;
    struct VPIH26xEncCfg *vpi_h26xe_cfg =
        (struct VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;

    for (i = 0; i < PIC_INDEX_MAX_NUMBER; i++) {
        if (enc_cfg->frame_ctx->pic_info[i].width == enc_cfg->lum_width_src &&
            enc_cfg->frame_ctx->pic_info[i].height == enc_cfg->lum_height_src) {
            break;
        }
    }
    if (i == PIC_INDEX_MAX_NUMBER) {
        VPILOGE("pp_index %d isn't avalid \n", i);
        goto error_exit;
    }
    enc_ctx->pp_index = i;

    h26x_enc_set_options(enc_ctx, enc_cfg);
    memset(vpi_h26xe_cfg, 0, sizeof(struct VPIH26xEncCfg));
#ifdef FB_SYSLOG_ENABLE
    vpi_h26xe_cfg->log_header.module_name =
        &enc_cfg->module_name[0]; /* &ctx->module_name[0];*/
#ifdef DRV_NEW_ARCH
    vpi_h26xe_cfg->log_header.device_id =
        h26x_enc_get_deviceId(options->device);
#else
    vpi_h26xe_cfg->log_header.device_id = 0;
#endif
#endif

    enc_ctx->encoder_is_start = HANTRO_FALSE;
    enc_ctx->encoder_is_end   = HANTRO_FALSE;
    enc_ctx->trans_flush_pic  = HANTRO_FALSE;
    enc_ctx->flush_state      = VPIH26X_FLUSH_IDLE;
    enc_ctx->picture_cnt_bk   = 0;

    vpi_h26xe_cfg->enc_index = options->enc_index;

    /* the number of output stream buffers */
    vpi_h26xe_cfg->stream_buf_num = options->stream_buf_chain ? 2 : 1;

    /* get GOP configuration */
    vpi_h26xe_cfg->gop_size = MIN(options->gop_size, MAX_GOP_SIZE);
    if (vpi_h26xe_cfg->gop_size == 0 && options->gop_lowdelay) {
        vpi_h26xe_cfg->gop_size = 4;
    }
    memset(enc_ctx->gop_pic_cfg, 0, sizeof(enc_ctx->gop_pic_cfg));
    vpi_h26xe_cfg->enc_in.gopConfig.pGopPicCfg = enc_ctx->gop_pic_cfg;
    memset(enc_ctx->gop_pic_special_cfg, 0,
           sizeof(enc_ctx->gop_pic_special_cfg));
    vpi_h26xe_cfg->enc_in.gopConfig.pGopPicSpecialCfg =
        enc_ctx->gop_pic_special_cfg;
    if ((ret = init_gop_configs(vpi_h26xe_cfg->gop_size, options,
                                &(vpi_h26xe_cfg->enc_in.gopConfig),
                                vpi_h26xe_cfg->enc_in.gopConfig.gopCfgOffset,
                                HANTRO_FALSE)) != 0) {
        VPILOGE("%s,%s,%d, init_gop_configs error", __FILE__, __FUNCTION__,
                __LINE__);
        return -1;
    }
    if (options->lookahead_depth) {
        memset(enc_ctx->gop_pic_cfg_pass2, 0,
               sizeof(enc_ctx->gop_pic_cfg_pass2));
        vpi_h26xe_cfg->enc_in.gopConfig.pGopPicCfg = enc_ctx->gop_pic_cfg_pass2;
        vpi_h26xe_cfg->enc_in.gopConfig.size       = 0;
        memset(enc_ctx->gop_pic_special_cfg, 0,
               sizeof(enc_ctx->gop_pic_special_cfg));
        vpi_h26xe_cfg->enc_in.gopConfig.pGopPicSpecialCfg =
            enc_ctx->gop_pic_special_cfg;
        if ((ret =
                 init_gop_configs(vpi_h26xe_cfg->gop_size, options,
                                  &(vpi_h26xe_cfg->enc_in.gopConfig),
                                  vpi_h26xe_cfg->enc_in.gopConfig.gopCfgOffset,
                                  HANTRO_TRUE)) != 0) {
            return -1;
        }
        vpi_h26xe_cfg->enc_in.gopConfig.pGopPicCfgPass1 = enc_ctx->gop_pic_cfg;
        vpi_h26xe_cfg->enc_in.gopConfig.pGopPicCfg =
            vpi_h26xe_cfg->enc_in.gopConfig.pGopPicCfgPass2 =
                enc_ctx->gop_pic_cfg_pass2;
    }

    if (options->lookahead_depth) {
        //2pass need add lookahead number
        max_frames_delay = 17 + options->lookahead_depth;
    } else {
        if (options->gop_size == 0) {
            max_frames_delay = MAX_GOP_SIZE + 2;
        } else {
            max_frames_delay = options->gop_size + 2;
        }
    }
    if (max_frames_delay > enc_cfg->frame_ctx->max_frames_delay) {
        enc_cfg->frame_ctx->max_frames_delay = max_frames_delay;
    }
    VPILOGD("max_frames_delay = %d\n", enc_cfg->frame_ctx->max_frames_delay);

    /* Encoder initialization */
#ifdef DRV_NEW_ARCH
    vpi_h26xe_cfg->priority = options->priority;
    vpi_h26xe_cfg->device   = options->device;
    if (vpi_h26xe_cfg->device == NULL) {
        goto error_exit;
    }
    vpi_h26xe_cfg->mem_id = options->mem_id;
#endif

    VPILOGE(" hantro_encoder[%p], *hantro_encoder[%p]\n", hantro_encoder,
            *hantro_encoder);
    if ((ret = h26x_enc_open_encoder(options, hantro_encoder, vpi_h26xe_cfg)) !=
        0) {
        goto error_exit;
    }

    vpi_h26xe_cfg->first_pic         = options->first_pic;
    vpi_h26xe_cfg->last_pic          = options->last_pic;
    vpi_h26xe_cfg->input_rate_numer  = options->input_rate_numer;
    vpi_h26xe_cfg->input_rate_denom  = options->input_rate_denom;
    vpi_h26xe_cfg->output_rate_numer = options->output_rate_numer;
    vpi_h26xe_cfg->output_rate_denom = options->output_rate_denom;
    vpi_h26xe_cfg->width             = options->width;
    vpi_h26xe_cfg->height            = options->height;
    vpi_h26xe_cfg->input_alignment =
        (options->exp_of_input_alignment == 0 ?
             0 :
             (1 << options->exp_of_input_alignment));
    vpi_h26xe_cfg->ref_alignment = (options->exp_of_ref_alignment == 0 ?
                                        0 :
                                        (1 << options->exp_of_ref_alignment));
    vpi_h26xe_cfg->ref_ch_alignment =
        (options->exp_of_ref_ch_alignment == 0 ?
             0 :
             (1 << options->exp_of_ref_ch_alignment));
    vpi_h26xe_cfg->format_customized_type = options->format_customized_type;
    vpi_h26xe_cfg->idr_interval           = options->intra_pic_rate;
    vpi_h26xe_cfg->byte_stream            = options->byte_stream;
    vpi_h26xe_cfg->interlaced_frame       = options->interlaced_frame;
    vpi_h26xe_cfg->parallel_core_num      = options->parallel_core_num;
    vpi_h26xe_cfg->buffer_cnt             = vpi_h26xe_cfg->frame_delay =
        vpi_h26xe_cfg->parallel_core_num;
    if (options->lookahead_depth) {
        i32 delay = CUTREE_BUFFER_CNT(options->lookahead_depth) - 1;
        vpi_h26xe_cfg->frame_delay += MAX(delay, 8);

        /* consider gop8->gop4 reorder: 8 4 2 1 3 6 5 7 -> 4 2 1 3 8 6 5 7
 *          *        *      * at least 4 more buffers are needed to avoid buffer overwrite in pass1 before consumed in pass2*/
        vpi_h26xe_cfg->buffer_cnt = vpi_h26xe_cfg->frame_delay + 4;
    }
    vpi_h26xe_cfg->enc_in.gopConfig.idr_interval = vpi_h26xe_cfg->idr_interval;
    vpi_h26xe_cfg->enc_in.gopConfig.gdrDuration  = options->gdr_duration;
    vpi_h26xe_cfg->enc_in.gopConfig.firstPic     = vpi_h26xe_cfg->first_pic;
    vpi_h26xe_cfg->enc_in.gopConfig.lastPic      = vpi_h26xe_cfg->last_pic;
    vpi_h26xe_cfg->enc_in.gopConfig.outputRateNumer =
        vpi_h26xe_cfg->output_rate_numer; /* Output frame rate numerator */
    vpi_h26xe_cfg->enc_in.gopConfig.outputRateDenom =
        vpi_h26xe_cfg->output_rate_denom; /* Output frame rate denominator */
    vpi_h26xe_cfg->enc_in.gopConfig.inputRateNumer =
        vpi_h26xe_cfg->input_rate_numer; /* Input frame rate numerator */
    vpi_h26xe_cfg->enc_in.gopConfig.inputRateDenom =
        vpi_h26xe_cfg->input_rate_denom; /* Input frame rate denominator */
    vpi_h26xe_cfg->enc_in.gopConfig.gopLowdelay = options->gop_lowdelay;
    vpi_h26xe_cfg->enc_in.gopConfig.interlacedFrame =
        vpi_h26xe_cfg->interlaced_frame;

    /* Set the test ID for internal testing,
 *      * the SW must be compiled with testing flags */
    VCEncSetTestId(*hantro_encoder, options->test_id);

    /* Allocate input and output buffers */
    if ((ret = h26x_enc_alloc_res(options, *hantro_encoder, vpi_h26xe_cfg)) !=
        0) {
        goto error_exit;
    }
    return ret;
error_exit:
    if (ret != 0) {
    }

    if (*hantro_encoder) {
        h26x_enc_free_res(*hantro_encoder, vpi_h26xe_cfg);
        h26x_enc_close_encoder(*hantro_encoder, vpi_h26xe_cfg);
        *hantro_encoder =  NULL;
    }

    return ret;
}

/**
 *  vpi_h26xe_encode
 *  Encoding of the h26x(h264/hevc) encoder
 *
 *  @Params: enc_ctx The context of Vpi H26x encoder
 *  @Params: input The input video frame for encoding
 *  @Params: output The output of the encoded ES stream
 *  @Return: 0 for success, -1 for error
 */
int vpi_h26xe_encode(struct VpiH26xEncCtx *enc_ctx, void *input, void *output)
{
    struct DecPicturePpu *pic_ppu = NULL;
    struct DecPicture *pic_data   = NULL;
    VpiH26xEncInAddr *p_addrs     = NULL;
    VPIH26xEncOptions *options    = &enc_ctx->options;
    VpiH26xEncInAddr vpi_h26x_enc_in_addr;

    VpiFrame *vpi_frame = (VpiFrame *)input;
    p_addrs               = &vpi_h26x_enc_in_addr;
    memset(p_addrs, 0, sizeof(vpi_h26x_enc_in_addr));
    if (enc_ctx->find_pict == 1) {
        enc_ctx->find_pict = 0;

        pic_ppu  = (struct DecPicturePpu *)vpi_frame->data[0];
        pic_data = (struct DecPicture *)&pic_ppu->pictures[enc_ctx->pp_index];

        p_addrs->bus_luma         = pic_data->luma.bus_address;
        p_addrs->bus_chroma       = pic_data->chroma.bus_address;
        p_addrs->bus_luma_table   = pic_data->luma_table.bus_address;
        p_addrs->bus_chroma_table = pic_data->chroma_table.bus_address;

        if (options->enc_index == 0 && options->lookahead_depth &&
            options->cutree_blkratio) {
            pic_data = &pic_ppu->pictures[2];
            if (!pic_data->pp_enabled) {
                VPILOGE("%s,%d when need 1/4 1pass input, pp1 should be "
                        "enabled! !!vpi_frame->data[0] = (uint8_t "
                        "*)&ctx->addrs!! \n",
                        __FUNCTION__, __LINE__);
                /*vpi_frame->data[0] = (uint8_t *)&ctx->addrs;*/
                return -1;
            }
            p_addrs->bus_luma_ds         = pic_data->luma.bus_address;
            p_addrs->bus_chroma_ds       = pic_data->chroma.bus_address;
            p_addrs->bus_luma_table_ds   = pic_data->luma_table.bus_address;
            p_addrs->bus_chroma_table_ds = pic_data->chroma_table.bus_address;
        }
    }

    VCEncOut *enc_out     = (VCEncOut *)&enc_ctx->enc_out;
    VCEncIn *p_enc_in     = (VCEncIn *)&(enc_ctx->vpi_h26xe_cfg.enc_in);
    VpiPacket *vpi_packet = (VpiPacket *)output;

    int ret        = 0;
    int flush_ret   = 0;
    if (enc_ctx->flush_state == VPIH26X_FLUSH_IDLE) {
        if (p_addrs->bus_luma == 0 || p_addrs->bus_chroma == 0) {
            enc_ctx->trans_flush_pic = HANTRO_TRUE;
        } else {
            enc_ctx->trans_flush_pic = HANTRO_FALSE;
        }
    }

    if (enc_ctx->no_input_pict == 1) {
        /*-1:no valid pict input*/
        VPILOGE("No data trans from dec, will flush...\n");
        enc_ctx->no_input_pict = 0;
        switch (enc_ctx->flush_state) {
        case VPIH26X_FLUSH_IDLE:
            flush_ret = h26x_enc_encode(enc_ctx, p_addrs, enc_out);
            VPILOGE("+++ vpi_h26xe_ctx.trans_flush_pic = %d\n",
                    enc_ctx->trans_flush_pic);

            if (enc_ctx->trans_flush_pic == HANTRO_TRUE) {
                enc_ctx->flush_state = VPIH26X_FLUSH_PREPARE;
            }
            if (flush_ret != VCENC_ERROR) {
                if (flush_ret == VCENC_FRAME_READY) {
                    ret = VPI_ENC_FLUSH_IDLE_READY;
                }
                if (flush_ret == OK) {
                    ret = VPI_ENC_FLUSH_IDLE_OK;
                }
            } else {
                ret = VPI_ENC_FLUSH_IDLE_ERROR;
            }
            break;

        case VPIH26X_FLUSH_PREPARE:
            VPILOGE(" VPIH26X_FLUSH_PREPARE \n");
            h26x_enc_flush_set(enc_ctx);
            ret = VPI_ENC_FLUSH_PREPARE;
            break;

        case VPIH26X_FLUSH_TRANSPIC: /* flush data in dec fifo */
            flush_ret = h26x_enc_encode(enc_ctx, p_addrs, enc_out);
            VPILOGE("+++ h26x_enc_encode ret = %d, VPIH26X_FLUSH_TRANSPIC \n",
                    flush_ret);
            if (p_addrs->bus_luma == 0 || p_addrs->bus_chroma == 0)
                enc_ctx->flush_state = VPIH26X_FLUSH_ENCDATA;
            if (flush_ret != VCENC_ERROR) {
                if (flush_ret == VCENC_FRAME_READY) {
                    ret = VPI_ENC_FLUSH_TRANSPIC_READY;
                } else {
                    ret = VPI_ENC_FLUSH_TRANSPIC_OK;
                }
            } else {
                ret = VPI_ENC_FLUSH_TRANSPIC_ERROR;
            }
            break;

        case VPIH26X_FLUSH_ENCDATA:
            flush_ret = h26x_enc_flush(enc_ctx, enc_out);
            if (flush_ret == VCENC_FRAME_READY) {
                ret = VPI_ENC_FLUSH_ENCDATA_READY;
            } else if (flush_ret == VCENC_FRAME_ENQUEUE) {
                ret = VPI_ENC_FLUSH_ENCDATA_FRM_ENQUEUE;
            } else if (flush_ret == VCENC_OK) {
                ret = VPI_ENC_FLUSH_ENCDATA_OK;
            } else {
                ret = VPI_ENC_FLUSH_ENCDATA_ERROR;
            }
            break;

        case VPIH26X_FLUSH_FINISH:
            if (enc_ctx->encoder_is_end != HANTRO_TRUE) {
                flush_ret = h26x_enc_end(enc_ctx, enc_out);
                VPILOGE("+++ h26x_enc_end ret = %d, FLUSH_FINISH \n", flush_ret);
                if (flush_ret == VCENC_OK) {
                    ret = VPI_ENC_FLUSH_FINISH_OK;
                } else {
                    VPILOGE("h26x_enc_end error. ret = %d\n", ret);
                    ret = VPI_ENC_FLUSH_FINISH_ERROR;
                }
            } else {
                ret = VPI_ENC_FLUSH_FINISH_END;
            }
            break;
        default:
            VPILOGE("unsupported flush state: %d\n", enc_ctx->flush_state);
            ret = VPI_ENC_FLUSH_ENCDATA_ERROR;
            break;
        }
        vpi_packet->index_encoded = enc_out->indexEncoded;
        if (enc_ctx->trans_flush_pic != HANTRO_TRUE)
            vpi_packet->size = enc_ctx->stream_size;
        else
            vpi_packet->size = enc_out->streamSize;

        /*Simplify the ret at FLUSH stage*/
        switch (ret) {
        case VPI_ENC_FLUSH_IDLE_OK:
            vpi_packet->size = 0;
            ret              = VPI_ENC_FLUSH_IDLE_READY;
            break;
        case VPI_ENC_FLUSH_PREPARE:
            vpi_packet->size = 0;
            break;
        case VPI_ENC_FLUSH_TRANSPIC_OK:
            vpi_packet->size = 0;
            ret              = VPI_ENC_FLUSH_TRANSPIC_READY;
            break;
        case VPI_ENC_FLUSH_ENCDATA_OK:
        case VPI_ENC_FLUSH_ENCDATA_FRM_ENQUEUE:
            vpi_packet->size = 0;
            ret              = VPI_ENC_FLUSH_ENCDATA_READY;
            break;
        default:
            break;
        }

#ifdef USE_OLD_DRV
        vpi_packet->data =
            (uint8_t *)enc_ctx->vpi_h26xe_cfg.outbuf_mem_factory[0][0]
                .virtualAddress;
#else
        vpi_packet->data =
            (uint8_t *)enc_ctx->vpi_h26xe_cfg.outbuf_mem_factory[0][0]
                .rc_virtualAddress;
#endif
        vpi_packet->pts     = enc_out->pts;
        vpi_packet->pkt_dts = enc_ctx->vpi_h26xe_cfg.picture_enc_cnt;

        return ret;

    } else {
        p_enc_in->pts = vpi_frame->pts;
        if (enc_ctx->encoder_is_start == HANTRO_FALSE) {
            enc_ctx->encoder_is_start = HANTRO_TRUE;
            ret                       = h26x_enc_start(enc_ctx, enc_out);
#ifdef USE_OLD_DRV
            vpi_packet->data =
                (uint8_t *)enc_ctx->vpi_h26xe_cfg.outbuf_mem_factory[0][0]
                    .virtualAddress;
#else
            vpi_packet->data =
                (uint8_t *)enc_ctx->vpi_h26xe_cfg.outbuf_mem_factory[0][0]
                    .rc_virtualAddress;
#endif
            if (ret != 0)
                ret = VPI_ENC_START_ERROR;
            else
                ret = VPI_ENC_START_OK;
            vpi_packet->index_encoded = enc_out->indexEncoded;
            if (enc_ctx->trans_flush_pic != HANTRO_TRUE)
                vpi_packet->size = enc_ctx->stream_size;
            else
                vpi_packet->size = enc_out->streamSize;
            vpi_packet->pts     = enc_out->pts;
            vpi_packet->pkt_dts = enc_ctx->vpi_h26xe_cfg.picture_enc_cnt;
            return ret;
        }

        ret                       = h26x_enc_encode(enc_ctx, p_addrs, enc_out);
        vpi_packet->index_encoded = enc_out->indexEncoded;
#ifdef USE_OLD_DRV
        vpi_packet->data =
            (uint8_t *)enc_ctx->vpi_h26xe_cfg.outbuf_mem_factory[0][0]
                .virtualAddress;
#else
        vpi_packet->data =
            (uint8_t *)enc_ctx->vpi_h26xe_cfg.outbuf_mem_factory[0][0]
                .rc_virtualAddress;
#endif
        if (enc_ctx->trans_flush_pic != HANTRO_TRUE)
            vpi_packet->size = enc_ctx->stream_size;
        else
            vpi_packet->size = enc_out->streamSize;
        vpi_packet->pts     = enc_out->pts;
        vpi_packet->pkt_dts = enc_ctx->vpi_h26xe_cfg.picture_enc_cnt;

        if (ret == VCENC_FRAME_READY) {
            return VPI_ENC_ENC_READY;
        } else if (ret == VCENC_OK) {
            return VPI_ENC_ENC_OK;
        } else if (ret == VCENC_FRAME_ENQUEUE) {
            return VPI_ENC_ENC_FRM_ENQUEUE;
        } else {
            return VPI_ENC_ENC_ERROR;
        }
        VPILOGE("%s(%d)\n", __FUNCTION__, __LINE__);
    }
    VPILOGE("%s(%d) \n", __FUNCTION__, __LINE__);

    return 0;
}

/**
 *  vpi_h26xe_ctrl
 *  Handle the control command
 *
 *  @Params: enc_ctx The context of Vpi H26x encoder
 *  @Params: indata The input control command
 *  @Params: outdata The parameter for output
 *  @Return: 0 for success, -1 for error
 */
int vpi_h26xe_ctrl(struct VpiH26xEncCtx *enc_ctx, void *indata, void *outdata)
{
    i32 ret              = OK;
    VpiCtrlCmdParam *cmd = (VpiCtrlCmdParam *)indata;

    struct VPIH26xEncCfg *vpi_h26xe_cfg =
        (struct VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);

    switch (cmd->cmd) {
    case VPI_CMD_H26xENC_GET_NEXT_PIC:
        *(int *)outdata =
            h26x_enc_next_picture(vpi_h26xe_cfg, vpi_h26xe_cfg->picture_cnt) +
            vpi_h26xe_cfg->first_pic;
        p_enc_in->indexTobeEncode = *(int *)outdata;
        break;
    case VPI_CMD_H26xENC_SET_FINDPIC:
        enc_ctx->find_pict = 1;
        break;
    case VPI_CMD_H26xENC_GET_FLUSHSTATE:
        *(int *)outdata = enc_ctx->flush_state;
        break;
    case VPI_CMD_H26xENC_UPDATE_STATISTIC:
        h26x_enc_update_statistic(enc_ctx, (int *)cmd->data);
        break;
    case VPI_CMD_H26xENC_SET_NO_INFRM:
        enc_ctx->no_input_pict = *(int *)cmd->data;
        break;
    default:
        break;
    }
    return ret;
}

/**
 *  vpi_h26xe_close
 *  Close the h26x(h264/hevc) encoder
 *
 *  @Params: enc_ctx The context of Vpi H26x encoder
 *  @Return: 0 for success, -1 for error
 */

int vpi_h26xe_close(struct VpiH26xEncCtx *enc_ctx)
{
    i32 ret                             = OK;
    struct VPIH26xEncCfg *vpi_h26xe_cfg = &enc_ctx->vpi_h26xe_cfg;

    h26x_enc_report(enc_ctx);
    if (enc_ctx != NULL) {
        if (enc_ctx->hantro_encoder != NULL) {
            h26x_enc_free_res(enc_ctx->hantro_encoder, vpi_h26xe_cfg);
            h26x_enc_close_encoder(enc_ctx->hantro_encoder, vpi_h26xe_cfg);
        }
    }
    return ret;
}
