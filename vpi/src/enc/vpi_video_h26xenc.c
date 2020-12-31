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
#include "hugepage_api.h"
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
static VpiRet h26x_enc_open_encoder(VPIH26xEncOptions *options, VCEncInst *p_enc,
                                 VPIH26xEncCfg *vpi_h26xe_cfg)
{
    VCEncRet ret = -1;
    VpiRet vpi_ret = VPI_SUCCESS;
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
        return VPI_ERR_ENCODER_OPITION;
    }

    if ((options->interlaced_frame && options->gop_size != 1) ||
        IS_H264(options->codec_format)) {
        VPILOGD(
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
    VPILOGD("%s cfg.compressor = %d\n", __FUNCTION__, cfg.compressor);

    cfg.interlacedFrame    = options->interlaced_frame;
    cfg.enableOutputCuInfo = (options->enable_output_cu_info > 0) ? 1 : 0;
    cfg.rdoLevel           = CLIP3(1, 3, options->rdo_level) - 1;
    VPILOGD("%s(%d)cfg.rdoLevel = %d\n", __FUNCTION__, __LINE__, cfg.rdoLevel);
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
    VPILOGD("[%d] cfg.extDSRatio = %d, and ds res %dx%d\n",
            vpi_h26xe_cfg->enc_index, cfg.extDSRatio, cfg.width_ds,
            cfg.height_ds);
    if (options->parallel_core_num > 1 && cfg.width * cfg.height < 256 * 256) {
        VPILOGD("Disable multicore for small resolution (< 255*255)\n");
        cfg.parallelCoreNum = options->parallel_core_num = 1;
    }
#ifdef FB_SYSLOG_ENABLE
    cfg.enc_index = vpi_h26xe_cfg->enc_index;
#ifdef DRV_NEW_ARCH
    cfg.device_id = vpi_h26xe_cfg->log_header.device_id;
#endif
#endif
    VPILOGD("\n+++ cfg.w = %d, cfg.h = %d\n", cfg.width, cfg.height);
#ifdef CHECK_MEM_LEAK_TRANS
    cfg.perf = EWLcalloc(1, sizeof(ENCPERF));
#else
    cfg.perf = calloc(1, sizeof(ENCPERF));
#endif
    if (cfg.perf) {
        ENCPERF *perf       = cfg.perf;
        vpi_h26xe_cfg->perf = perf;
        VPILOGD("calloc perf = %p\n", perf);
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
        return VPI_ERR_ENCODER_INIT;
    }
    VPILOGD("ewl is outside initialized.\n");

    /* this is for 2 pass */
    if (options->lookahead_depth != 0) {
        param.pass = 1;
        if ((vpi_h26xe_cfg->two_pass_ewl = EWLInit(&param)) == NULL) {
            VPILOGE("OpenEncoder: EWL for 2 pass Initialization failed!\n");
            return VPI_ERR_ENCODER_INIT;
        }
        VPILOGD("2 pass ewl is outside initialized.\n");
    }

    if ((ret = VCEncInit(&cfg, p_enc, vpi_h26xe_cfg->ewl,
                         vpi_h26xe_cfg->two_pass_ewl)) != VCENC_OK) {
        VPILOGE("VCEncInit failed\n");
        encoder = *p_enc;
        return VPI_ERR_ENCODER_INIT;
    }

    encoder = *p_enc;
    /* Encoder setup: coding control */
    if ((ret = VCEncGetCodingCtrl(encoder, &coding_cfg)) != VCENC_OK) {
        VPILOGE("VCEncGetCodingCtrl failed\n");
        return VPI_ERR_ENCODER_INIT;
    } else {
        if (options->slice_size != DEFAULT)
            coding_cfg.sliceSize = options->slice_size;
        if (options->enable_cabac != DEFAULT)
            coding_cfg.enableCabac = options->enable_cabac;
        if (options->cabac_init_flag != DEFAULT)
            coding_cfg.cabacInitFlag = options->cabac_init_flag;
        coding_cfg.vuiVideoFullRange = 0;
        if (options->video_range != DEFAULT)
            coding_cfg.vuiVideoFullRange = options->video_range;

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
            VPILOGD("  CIR: %d %d\n", coding_cfg.cirStart,
                    coding_cfg.cirInterval);

        if (coding_cfg.intraArea.enable)
            VPILOGD("  IntraArea: %dx%d-%dx%d\n", coding_cfg.intraArea.left,
                    coding_cfg.intraArea.top, coding_cfg.intraArea.right,
                    coding_cfg.intraArea.bottom);

        if (coding_cfg.ipcm1Area.enable)
            VPILOGD("  IPCM1Area: %dx%d-%dx%d\n", coding_cfg.ipcm1Area.left,
                    coding_cfg.ipcm1Area.top, coding_cfg.ipcm1Area.right,
                    coding_cfg.ipcm1Area.bottom);

        if (coding_cfg.ipcm2Area.enable)
            VPILOGD("  IPCM2Area: %dx%d-%dx%d\n", coding_cfg.ipcm2Area.left,
                    coding_cfg.ipcm2Area.top, coding_cfg.ipcm2Area.right,
                    coding_cfg.ipcm2Area.bottom);

        if (coding_cfg.roi1Area.enable)
            VPILOGD("  ROI 1: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi1Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi1Qp >=
                    0 ? coding_cfg.roi1Qp : coding_cfg.roi1DeltaQp,
                    coding_cfg.roi1Area.left, coding_cfg.roi1Area.top,
                    coding_cfg.roi1Area.right, coding_cfg.roi1Area.bottom);

        if (coding_cfg.roi2Area.enable)
            VPILOGD("  ROI 2: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi2Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi2Qp >=
                    0 ? coding_cfg.roi2Qp : coding_cfg.roi2DeltaQp,
                    coding_cfg.roi2Area.left, coding_cfg.roi2Area.top,
                    coding_cfg.roi2Area.right, coding_cfg.roi2Area.bottom);

        if (coding_cfg.roi3Area.enable)
            VPILOGD("  ROI 3: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi3Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi3Qp >=
                    0 ? coding_cfg.roi3Qp : coding_cfg.roi3DeltaQp,
                    coding_cfg.roi3Area.left, coding_cfg.roi3Area.top,
                    coding_cfg.roi3Area.right, coding_cfg.roi3Area.bottom);

        if (coding_cfg.roi4Area.enable)
            VPILOGD("  ROI 4: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi4Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi4Qp >=
                    0 ? coding_cfg.roi4Qp : coding_cfg.roi4DeltaQp,
                    coding_cfg.roi4Area.left, coding_cfg.roi4Area.top,
                    coding_cfg.roi4Area.right, coding_cfg.roi4Area.bottom);

        if (coding_cfg.roi5Area.enable)
            VPILOGD("  ROI 5: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi5Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi5Qp >=
                    0 ? coding_cfg.roi5Qp : coding_cfg.roi5DeltaQp,
                    coding_cfg.roi5Area.left, coding_cfg.roi5Area.top,
                    coding_cfg.roi5Area.right, coding_cfg.roi5Area.bottom);

        if (coding_cfg.roi6Area.enable)
            VPILOGD("  ROI 6: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi6Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi6Qp >=
                    0 ? coding_cfg.roi6Qp : coding_cfg.roi6DeltaQp,
                    coding_cfg.roi6Area.left, coding_cfg.roi6Area.top,
                    coding_cfg.roi6Area.right, coding_cfg.roi6Area.bottom);

        if (coding_cfg.roi7Area.enable)
            VPILOGD("  ROI 7: %s %d  %dx%d-%dx%d\n",
                    coding_cfg.roi7Qp >= 0 ? "QP" : "QP Delta",
                    coding_cfg.roi7Qp >=
                    0 ? coding_cfg.roi7Qp : coding_cfg.roi7DeltaQp,
                    coding_cfg.roi7Area.left, coding_cfg.roi7Area.top,
                    coding_cfg.roi7Area.right, coding_cfg.roi7Area.bottom);

        if (coding_cfg.roi8Area.enable)
            VPILOGD("  ROI 8: %s %d  %dx%d-%dx%d\n",
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
            options->noise_reduction_enable; /*0: disable noise reduction; 1:
                                                enable noise reduction*/
        if (options->noise_low == 0) {
            coding_cfg.noiseLow = 10;
        } else {
            coding_cfg.noiseLow =
                CLIP3(1, 30, options->noise_low); /*0: use default value; valid
                                                     value range: [1, 30]*/
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
        if (!IS_H264(options->codec_format)) {
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

            coding_cfg.vuiColorDescription.vuiColorDescripPresentFlag =
                    options->vui_color_flag;
            if (options->vui_color_flag) {
                coding_cfg.vuiColorDescription.vuiMatrixCoefficients =
                        options->vui_matrix_coefficients;
                coding_cfg.vuiColorDescription.vuiColorPrimaries =
                        options->vui_color_primaries;
                coding_cfg.vuiColorDescription.vuiTransferCharacteristics =
                        options->vui_transfer_characteristics;
            }
        }

        coding_cfg.vuiVideoFormat                = options->vui_video_format;
        coding_cfg.vuiVideoSignalTypePresentFlag = options->vui_video_signal_type_en;
        coding_cfg.sampleAspectRatioHeight       = options->vui_aspect_ratio_width;
        coding_cfg.sampleAspectRatioWidth        = options->vui_aspect_ratio_height;

        coding_cfg.RpsInSliceHeader = options->rps_in_slice_header;

        if ((ret = VCEncSetCodingCtrl(encoder, &coding_cfg)) != VCENC_OK) {
            VPILOGE("VCEncSetCodingCtrl failed\n");
            return VPI_ERR_ENCODER_INIT;
        }
    }

    /* Encoder setup: rate control */
    if ((ret = VCEncGetRateCtrl(encoder, &rc_cfg)) != VCENC_OK) {
        VPILOGE("VCEncGetRateCtrl failed\n");
        return VPI_ERR_ENCODER_INIT;
    } else {
        VPILOGD("Get rate control: qp %2d qpRange I[%2d, %2d] PB[%2d, %2d] %8d bps  "
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

        VPILOGD("Set rate control: qp %2d qpRange I[%2d, %2d] PB[%2d, %2d] %9d bps  "
                "pic %d skip %d  hrd %d"
                "  cpbSize %d bitrateWindow %d intraQpDelta %2d "
                "fixedIntraQp %2d\n",
                rc_cfg.qpHdr, rc_cfg.qpMinI, rc_cfg.qpMaxI, rc_cfg.qpMinPB,
                rc_cfg.qpMaxPB, rc_cfg.bitPerSecond, rc_cfg.pictureRc,
                rc_cfg.pictureSkip, rc_cfg.hrd, rc_cfg.hrdCpbSize,
                rc_cfg.bitrateWindow, rc_cfg.intraQpDelta, rc_cfg.fixedIntraQp);

        if ((ret = VCEncSetRateCtrl(encoder, &rc_cfg)) != VCENC_OK) {
            VPILOGE("VCEncSetRateCtrl failed\n");
            return VPI_ERR_ENCODER_INIT;
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
        return VPI_ERR_ENCODE;
    }

    VPILOGD("Get PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d"
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
        VPILOGD("options->lum_width_src = %d, x = %d, w = %d, b = %d, aw = %d\n",
                options->lum_width_src, x, w, b, aw);
        if (w) {
            pre_proc_cfg.xOffset   = 0;
            pre_proc_cfg.origWidth = MIN(aw, options->lum_width_src - x);
        }
        VPILOGD("pre_proc_cfg.xOffset = %d, pre_proc_cfg.origWidth = %d\n",
                pre_proc_cfg.xOffset, pre_proc_cfg.origWidth);
        if (ah) {
            pre_proc_cfg.yOffset    = 0;
            pre_proc_cfg.origHeight = ah;
        }
        VPILOGD("pre_proc_cfg.yOffset = %d, pre_proc_cfg.origHeight = %d\n",
                pre_proc_cfg.yOffset, pre_proc_cfg.origHeight);
    }
#endif
    if (options->input_format == INPUT_FORMAT_PP_YUV420_SEMIPLANNAR ||
        options->input_format == INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_VU ||
        options->input_format == INPUT_FORMAT_PP_YUV420_PLANAR_10BIT_P010 ||
        options->input_format == INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_YUV420P) {
        i32 x =
            (options->hor_offset_src == DEFAULT) ? 0 : options->hor_offset_src;
        i32 y =
            (options->ver_offset_src == DEFAULT) ? 0 : options->ver_offset_src;
        i32 w = (options->width == DEFAULT) ? 0 : options->width;
        i32 h = (options->height == DEFAULT) ? 0 : options->height;
        i32 b = (options->input_format ==
                 INPUT_FORMAT_PP_YUV420_PLANAR_10BIT_P010) ?
                    2 :
                    1;
        i32 aw = (((w * b + 31) / 32) * 32) / b;
        VPILOGD("options->lum_width_src = %d, x = %d, w = %d, b = %d, aw = %d\n",
                options->lum_width_src, x, w, b, aw);
        if (x) {
            pre_proc_cfg.xOffset = x;
        }
        VPILOGD("pre_proc_cfg.xOffset = %d, pre_proc_cfg.origWidth = %d\n",
                pre_proc_cfg.xOffset, pre_proc_cfg.origWidth);
        if (y) {
            pre_proc_cfg.yOffset = y;
        }
        VPILOGD("pre_proc_cfg.yOffset = %d, pre_proc_cfg.origHeight = %d\n",
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

    VPILOGD("Set PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d"
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

    /* for edma trans raw from rc to ep, need to close the dummy regisiter.*/
    pre_proc_cfg.b_close_dummy_regs = options->b_close_dummy_regs;

    if ((ret = VCEncSetPreProcessing(encoder, &pre_proc_cfg)) != VCENC_OK) {
        VPILOGE("VCEncSetPreProcessing failed\n");
        return VPI_ERR_ENCODER_INIT;
    }

    return vpi_ret;
}

/**
 *  h26x_enc_close_encoder
 *  Release an encoder insatnce.
 *
 *  @Params: encoder the instance to be released
 */
static VpiRet h26x_enc_close_encoder(VCEncInst encoder,
                                   VPIH26xEncCfg *vpi_h26xe_cfg)
{
    VCEncRet ret;
    VpiRet vpi_ret = VPI_SUCCESS;

#ifdef USE_OLD_DRV
    if (vpi_h26xe_cfg->scaled_picture_mem.virtualAddress != NULL)
#endif
    EWLFreeLinear(vpi_h26xe_cfg->ewl, &vpi_h26xe_cfg->scaled_picture_mem);

    if (encoder) {
        if ((ret = VCEncRelease(encoder)) != VCENC_OK) {
            VPILOGE("VCEncRelease error!\n");
            vpi_ret = VPI_ERR_ENCODE;
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
#ifdef CHECK_MEM_LEAK_TRANS
        EWLfree(vpi_h26xe_cfg->perf);
#else
        free(vpi_h26xe_cfg->perf);
#endif
    }

    return vpi_ret;
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

/* add for IDR */
static int idr_poc_array_init(VpiH26xEncCtx *ctx)
{
    int i = 0;
    for (i = 0; i < MAX_IDR_ARRAY_DEPTH; i++) {
        ctx->idr_poc_array[i] = -1;
    }
    ctx->next_idr_poc     = 0;
    ctx->poc_store_idx    = 0;
    ctx->next_idr_poc_idx = 0;
    ctx->update_idr_poc   = HANTRO_TRUE;

    return 0;
}

static int store_idr_poc(VpiH26xEncCtx *ctx, int idr_poc)
{
    if (ctx->idr_poc_array[ctx->poc_store_idx % MAX_IDR_ARRAY_DEPTH] != -1) {
        return -1;
    }

    ctx->idr_poc_array[ctx->poc_store_idx % MAX_IDR_ARRAY_DEPTH] = idr_poc;
    ctx->poc_store_idx++;

    return 0;
}

static int clear_idr_poc(VpiH26xEncCtx *ctx)
{
    ctx->idr_poc_array[(ctx->next_idr_poc_idx + MAX_IDR_ARRAY_DEPTH - 1) %
                       MAX_IDR_ARRAY_DEPTH] = -1;

    return 0;
}

static int update_next_idr_poc(VpiH26xEncCtx *ctx, int *idr_poc)
{
    if (ctx->idr_poc_array[ctx->next_idr_poc_idx % MAX_IDR_ARRAY_DEPTH] == -1) {
        return -1;
    }

    *idr_poc = ctx->idr_poc_array[ctx->next_idr_poc_idx % MAX_IDR_ARRAY_DEPTH];
    ctx->next_idr_poc_idx++;
    ctx->update_idr_poc = HANTRO_FALSE;
    return 0;
}

static int calc_flush_data_coding_type(VpiH26xEncCtx *ctx)
{
    VPIH26xEncOptions *options   = &ctx->options;
    VPIH26xEncCfg *vpi_h26xe_cfg = &ctx->vpi_h26xe_cfg;
    VCEncIn *pEncIn              = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);

    if (ctx->key_frame_flag == HANTRO_TRUE) {
        if (pEncIn->gopPicIdx == pEncIn->gopSize - 1) {
            ctx->next_gop_start = vpi_h26xe_cfg->input_pic_cnt;
            int length          = ctx->next_idr_poc - ctx->next_gop_start;
            if (length == 0) {
                ctx->idr_flag = FRAME_IDR;
            }
        }
    }

    ctx->next_gop_size = 1;

    pEncIn->forceIDR =
        ((ctx->idr_flag == FRAME_IDR) ? HANTRO_TRUE :
                                        HANTRO_FALSE);
    pEncIn->resendPPS = pEncIn->resendSPS = pEncIn->resendVPS =
        ((ctx->idr_flag == FRAME_IDR) ? 1 : 0);

    if (ctx->idr_flag == FRAME_IDR) {
        ctx->next_coding_type =
            VCEncFindNextPic(ctx->hantro_encoder, pEncIn, ctx->next_gop_size,
                             pEncIn->gopConfig.gopCfgOffset, true);
        clear_idr_poc(ctx);
        ctx->update_idr_poc = HANTRO_TRUE;

    } else {
        ctx->next_coding_type =
            VCEncFindNextPic(ctx->hantro_encoder, pEncIn, ctx->next_gop_size,
                             pEncIn->gopConfig.gopCfgOffset, false);
    }

    if (pEncIn->bIsIDR == HANTRO_TRUE) {
        ctx->idr_flag = ctx->force_idr ? FRAME_IDR2NORMAL : FRAME_NORMAL;
    } else {
        ctx->idr_flag = FRAME_NORMAL;
    }

    return 0;
}

static int calc_next_coding_type(VpiH26xEncCtx *ctx)
{
    VPIH26xEncOptions *options   = &ctx->options;
    VPIH26xEncCfg *vpi_h26xe_cfg = &ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in              = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);

    if (ctx->key_frame_flag == HANTRO_TRUE) {
        if (ctx->next_poc == 0) {
            p_enc_in->gopSize = 1;
        }

        if (p_enc_in->gopPicIdx == p_enc_in->gopSize - 1) {
            ctx->next_gop_start = vpi_h26xe_cfg->input_pic_cnt;
            int length          = ctx->next_idr_poc - ctx->next_gop_start;

            if (length > ctx->gop_len) {
            } else {
                if ((length >= 1) && (length <= ctx->gop_len)) {
                    if (length > 4)
                        ctx->next_gop_size = 4;
                    else
                        ctx->next_gop_size = length;
                } else if (length == 0) {
                    ctx->next_gop_size = 1;
                    ctx->idr_flag      = FRAME_IDR;
                }
            }
        }
    }

    p_enc_in->forceIDR =
        ((ctx->idr_flag == FRAME_IDR) ? HANTRO_TRUE :
                                        HANTRO_FALSE);
    p_enc_in->resendPPS = p_enc_in->resendSPS = p_enc_in->resendVPS =
        ((ctx->idr_flag == FRAME_IDR) ? 1 : 0);

    if (ctx->idr_flag == FRAME_IDR) {
        ctx->next_coding_type =
            VCEncFindNextPic(ctx->hantro_encoder, p_enc_in, ctx->next_gop_size,
                             p_enc_in->gopConfig.gopCfgOffset, true);
        clear_idr_poc(ctx);
        ctx->update_idr_poc = HANTRO_TRUE;

    } else if (ctx->idr_flag == FRAME_IDR2NORMAL) {
        p_enc_in->gopSize = ctx->next_gop_size = (ctx->adaptive_gop ? (options->lookahead_depth ? 4 : 1) : vpi_h26xe_cfg->gop_size);

        ctx->next_gop_start = vpi_h26xe_cfg->input_pic_cnt;
        int length_predict  = ctx->next_idr_poc - ctx->next_gop_start;

        if ((length_predict > 0) && (length_predict < ctx->next_gop_size)) {
            p_enc_in->gopSize = ctx->next_gop_size = length_predict;
        }

        ctx->next_coding_type =
            VCEncFindNextPic(ctx->hantro_encoder, p_enc_in, ctx->next_gop_size,
                             p_enc_in->gopConfig.gopCfgOffset, false);
    } else {
        ctx->next_coding_type =
            VCEncFindNextPic(ctx->hantro_encoder, p_enc_in, ctx->next_gop_size,
                             p_enc_in->gopConfig.gopCfgOffset, false);
    }

    if (p_enc_in->bIsIDR == HANTRO_TRUE) {
        ctx->idr_flag = ctx->force_idr ? FRAME_IDR2NORMAL : FRAME_NORMAL;
    } else {
        ctx->idr_flag = FRAME_NORMAL;
    }

    return 0;
}

/**
 *  h26x_enc_call_vcstart
 *  Call vc8000e sdk's VCEncStrmStart() function
 */
static VpiRet h26x_enc_call_vcstart(VpiH26xEncCtx *enc_ctx,
                                 VCEncOut *enc_out)
{
    VPIH26xEncOptions *options   = &enc_ctx->options;
    VPIH26xEncCfg *vpi_h26xe_cfg = &enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in            = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);
    int *stream_size             = &enc_ctx->stream_size;
    VpiEncOutData *out_buffer    = NULL;
    VCEncRet ret;
    i32 p   = 0;
    int cnt = 1;

    enc_ctx->adaptive_gop = (vpi_h26xe_cfg->gop_size == 0);
    memset(&enc_ctx->agop, 0, sizeof(enc_ctx->agop));

    /* The first output buffer */
    out_buffer = &enc_ctx->enc_pkt[0];
    setup_output_buffer(enc_ctx->hantro_encoder, out_buffer, p_enc_in);

    //p_enc_in->hashType = options->hashtype;
    init_slice_ctl(vpi_h26xe_cfg, options);
    init_stream_segment_crl(vpi_h26xe_cfg, options);

    if (options->input_line_buf_mode) {
        if (h26x_enc_init_input_line_buffer(&(vpi_h26xe_cfg->input_ctb_linebuf),
                                            options, p_enc_in,
                                            enc_ctx->hantro_encoder,
                                            vpi_h26xe_cfg)) {
            VPILOGE("init input line buffer failed\n");
            return VPI_ERR_ENCODE;
        }
    }

    /* before VCEncStrmStart called */
    h26x_cfg_init_pic(vpi_h26xe_cfg, options, &enc_ctx->ma, &enc_ctx->agop);
    h26x_enc_init_pic_config(p_enc_in, vpi_h26xe_cfg, options);
    enc_ctx->next_gop_size = p_enc_in->gopSize;

    /* Video, sequence and picture parameter sets */
    *stream_size = 0;
    for (p = 0; p < cnt; p++) {
        ret = VCEncStrmStart(enc_ctx->hantro_encoder, p_enc_in, enc_out);
        if (ret) {
            VPILOGE("VCEncStrmStart failed, ret %d\n", ret);
            return VPI_ERR_ENCODE;
        }

        enc_ctx->total_bits += enc_out->streamSize * 8;
        *stream_size += enc_out->streamSize;
    }
    enc_ctx->header_size = *stream_size;
    if (enc_ctx->header_size) {
        enc_ctx->header_data = malloc(enc_ctx->header_size);
        if (!enc_ctx->header_data) {
            VPILOGE("fail to malloc for header\n");
            return VPI_ERR_NO_AP_MEM;
        }
        memcpy(enc_ctx->header_data,
               out_buffer->outbuf_mem->rc_virtualAddress,
               enc_ctx->header_size);
    }

    return VPI_SUCCESS;
}

static VpiRet h26x_encode_end(VpiH26xEncCtx *enc_ctx)
{
    VPIH26xEncCfg *cfg         = &enc_ctx->vpi_h26xe_cfg;
    VPIH26xEncOptions *options = &enc_ctx->options;
    VCEncRet ret               = VCENC_OK;
    VCEncIn *p_enc_in          = (VCEncIn *)&(cfg->enc_in);
    VCEncOut *p_enc_out        = (VCEncOut *)&enc_ctx->enc_out;
    VpiEncOutData *out_buffer  = NULL;
    int i;

    if (enc_ctx->encoder_is_end == HANTRO_TRUE) return 0;

#ifdef VCE_MEM_ERR_TEST
    if (VceMemoryCheck(options) != 0){
        VPILOGE("vce force memory error in function\n");
        return VPI_ERR_ENCODE;
    }
#endif

    VPILOGD("cometo %s\n", __FUNCTION__);
    out_buffer = &enc_ctx->enc_pkt[0];
    setup_output_buffer(enc_ctx->hantro_encoder, out_buffer, p_enc_in);
    ret = VCEncStrmEnd(enc_ctx->hantro_encoder, p_enc_in, p_enc_out);

#ifdef VCE_MEM_ERR_TEST
    if (VceMemoryCheck(options) != 0){
       VPILOGE("vce force VCEncStrmEnd return error\n");
       return VCENC_ERROR;
    }
#endif

    if (ret == VCENC_OK) {
        i = h26x_enc_get_empty_stream_buffer(enc_ctx);
        if (i == -1) {
            VPILOGD("Can't found empty stream buffer\n");
            return VPI_ERR_ENCODE;
        }
        out_buffer              = &enc_ctx->outstream_pkt[i];
        out_buffer->header_size = p_enc_out->streamSize;
        out_buffer->header_data = enc_ctx->header_data;
        out_buffer->end_data    = HANTRO_TRUE;
        enc_ctx->flush_state    = VPIH26X_FLUSH_ENCEND;
        enc_ctx->h26xe_thd_end  = 1;
        enc_ctx->encode_end     = 1;
        enc_ctx->stream_buf_list[i]->used = 1;
        enc_ctx->stream_buf_list[i]->item = &enc_ctx->outstream_pkt[i];
        pthread_mutex_lock(&enc_ctx->h26xe_thd_mutex);
        h26x_enc_buf_list_add(&enc_ctx->stream_buf_head,
                               enc_ctx->stream_buf_list[i]);
        if (enc_ctx->waiting_for_pkt == 1) {
            pthread_cond_signal(&enc_ctx->h26xe_thd_cond);
            enc_ctx->waiting_for_pkt = 0;
        }
        pthread_mutex_unlock(&enc_ctx->h26xe_thd_mutex);
    } else {
        VPILOGE("VCEncStrmEnd ret is %d\n", ret);
        return VPI_ERR_ENCODE;
    }

    enc_ctx->encoder_is_end = HANTRO_TRUE;

    return VPI_SUCCESS;
}

/**
 *  h26x_enc_start
 *  Start h26x(h264/hevc) encoding
 */
static int h26x_enc_start(VpiH26xEncCtx *enc_ctx, VCEncOut *enc_out)
{
    VPIH26xEncOptions *options  = &enc_ctx->options;
    VpiRet ret                = VPI_SUCCESS;
    VPIH26xEncCfg *vpi_h26xe_cfg = (VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);

    ret = h26x_enc_call_vcstart(enc_ctx, enc_out);
    if (ret != 0) {
        VPILOGE("h26x_enc_call_vcstart failure!\n");
        return VPI_ERR_ENCODE;
    }

    VCEncGetRateCtrl(enc_ctx->hantro_encoder,
                     (VCEncRateCtrl *)&vpi_h26xe_cfg->rc);

    /* Allocate a buffer for user data and read data from file */
    enc_ctx->p_user_data =
        read_userdata(enc_ctx->hantro_encoder, options->user_data);

    /* Read configuration files for ROI/CuTree/IPCM/GMV ... */
    if (read_config_files(vpi_h26xe_cfg, options)){
        VPILOGE("read_config_files failure!\n");
        return VPI_ERR_ENCODE;
    }

    return ret;
}

/**
 *  h26x_enc_flush_set
 *  Set the flush state to FLUSH_TRANSPIC for h26x(h264/hevc) encoder
 */
static int h26x_enc_flush_set(VpiH26xEncCtx *enc_ctx)
{
    VPIH26xEncCfg *vpi_h26xe_cfg = (VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in            = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);

    VPILOGD("ctx %p get EOS, begin to flush trans pic\n", enc_ctx);
    enc_ctx->flush_state = VPIH26X_FLUSH_TRANSPIC;

    *p_enc_in                         = enc_ctx->enc_in_bk;
    vpi_h26xe_cfg->enc_in.picture_cnt = enc_ctx->picture_cnt_bk;
    calc_flush_data_coding_type(enc_ctx);
    p_enc_in->gopSize       = 1;
    p_enc_in->timeIncrement = vpi_h26xe_cfg->output_rate_denom;

    return 0;
}

static VpiRet h26x_enc_send_pic(VpiH26xEncCtx *enc_ctx, int poc_need,
                             VpiH26xEncInAddr *p_addrs, int64_t *pts)
{
    VPIH26xEncOptions *options = &enc_ctx->options;
    VPIH26xEncCfg *cfg         = (VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;
    VpiEncH26xPic *p_trans     = NULL;
    VCEncIn *p_enc_in          = (VCEncIn *)&cfg->enc_in;
    int i;

    struct DecPicturePpu *pic_ppu = NULL;
    struct DecPicture *pic_data = NULL;

    memset(p_addrs, 0, sizeof(VpiH26xEncInAddr));

    //find the need_poc
    for (i = 0; i < MAX_WAIT_DEPTH; i++) {
        pthread_mutex_lock(&enc_ctx->pic_wait_list[i].pic_mutex);
        if (enc_ctx->pic_wait_list[i].state == 1) {
            p_trans = &enc_ctx->pic_wait_list[i];
            VPILOGD("ctx %p, %d, poc %d, need_poc %d\n",
                     enc_ctx, i, p_trans->poc, poc_need);
            if (p_trans->poc == poc_need) {
                /* add to get new vui info */
                if (poc_need == 0) {
                    VCEncCodingCtrl coding_cfg;
                    if (VCEncGetCodingCtrl(enc_ctx->hantro_encoder, &coding_cfg) != VCENC_OK) {
                        VPILOGE("VCEncGetCodingCtrl failed\n");
                        return -1;
                    } else {
                        if (cfg->color_range == VPICOL_RANGE_JPEG) {
                            options->video_range = 1;
                        } else {
                            options->video_range = 0;
                        }

                        options->vui_video_signal_type_en |= options->video_range;
                        coding_cfg.vuiVideoSignalTypePresentFlag = options->vui_video_signal_type_en;
                        coding_cfg.vuiVideoFullRange = options->video_range;

                        if (VCEncSetCodingCtrl(enc_ctx->hantro_encoder, &coding_cfg) != VCENC_OK) {
                            VPILOGE("VCEncSetCodingCtrl failed\n");
                            return -1;
                        }
                    }
                }

                pic_ppu = (struct DecPicturePpu *)p_trans->pic->data[0];
                if (pic_ppu == NULL) {
                    VPILOGE("enc get frame[%d] error\n", poc_need);
                    return VPI_ERR_ENCODE;
                }
                pic_data =
                    (struct DecPicture *)&pic_ppu->pictures[enc_ctx->pp_index];
                p_trans->in_pass_one_queue = 1;
                p_trans->used              = 1;
                pthread_mutex_unlock(&p_trans->pic_mutex);
                break;
            } else {
                pthread_mutex_unlock(&p_trans->pic_mutex);
            }
        } else {
            pthread_mutex_unlock(&enc_ctx->pic_wait_list[i].pic_mutex);
        }
    }

    if (i == MAX_WAIT_DEPTH) {
        VPILOGE("No needed pict, waitting for the next.\n");
        return VPI_ERR_ENCODE_WAITT_BUF;
    }

    p_addrs->bus_luma         = pic_data->luma.bus_address;
    p_addrs->bus_chroma       = pic_data->chroma.bus_address;
    p_addrs->bus_luma_table   = pic_data->luma_table.bus_address;
    p_addrs->bus_chroma_table = pic_data->chroma_table.bus_address;

    VPILOGD("[%d] busLuma[%p], busCU[%p], busLT[%p], busCT[%p]\n",
            options->enc_index, p_addrs->bus_luma, p_addrs->bus_chroma,
            p_addrs->bus_luma_table, p_addrs->bus_chroma_table);

    if (options->enc_index == 0
        && options->lookahead_depth
        && options->cutree_blkratio) {
        VPILOGD("[%d] enter downSample 1pass\n", options->enc_index);

        pic_data = &pic_ppu->pictures[2];
        if (!pic_data->pp_enabled) {
            VPILOGE("when need 1/4 1pass input, pp1 should be enabled!\n");
            return VPI_ERR_ENCODER_OPITION;
        }

        p_addrs->bus_luma_ds         = pic_data->luma.bus_address;
        p_addrs->bus_chroma_ds       = pic_data->chroma.bus_address;
        p_addrs->bus_luma_table_ds   = pic_data->luma_table.bus_address;
        p_addrs->bus_chroma_table_ds = pic_data->chroma_table.bus_address;

        VPILOGD("[%d] busLumaDs[%p], busCUDs[%p], busLTDs[%p], busCTDs[%p]\n",
                options->enc_index, p_addrs->bus_luma_ds,
                p_addrs->bus_chroma_ds,
                p_addrs->bus_luma_table_ds, p_addrs->bus_chroma_table_ds);
    }

    if (p_trans->pic->pts == VID_NOPTS_VALUE) {
        p_enc_in->pts = p_trans->poc;
    } else {
        p_enc_in->pts = p_trans->pic->pts;
    }
    *pts = p_enc_in->pts;

    return VPI_SUCCESS;
}

static VpiRet h26x_enc_process_frame(VpiH26xEncCtx *enc_ctx,
                                  VpiEncOutData *out_buffer)
{
    VPIH26xEncOptions *options = &enc_ctx->options;
    VPIH26xEncCfg *cfg         = (VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in          = (VCEncIn *)&(cfg->enc_in);
    VCEncOut *p_enc_out        = (VCEncOut *)&enc_ctx->enc_out;
    VpiEncOutData *outstrm_buf = NULL;
    VpiRet ret               = VPI_SUCCESS;
    int i, idx;
    int pkt_size;
    int new_size;
    EWLLinearMem_t *mem;

    SliceCtl *ctl   = cfg->slice_ctl_out;
    int stream_size = p_enc_out->streamSize;

    if (out_buffer == NULL) {
        VPILOGE("out_buffer is NULL.\n");
        return VPI_ERR_ENCODE;
    }

    if(options->lookahead_depth && p_enc_out->codingType == VCENC_INTRA_FRAME)
        enc_ctx->frame_cnt_output = 0;

    VCEncGetRateCtrl(enc_ctx->hantro_encoder, (VCEncRateCtrl *)&cfg->rc);
    /* Write scaled encoder picture to file, packed yuyv 4:2:2 format */
    //WriteScaled((u32 *)p_enc_out->scaledPicture, enc_ctx->hantro_encoder);
    /* write cu encoding information to file <cuInfo.txt> */
    //WriteCuInformation(cfg, enc_ctx->hantro_encoder, p_enc_out, options, cfg->picture_enc_cnt-1, p_enc_in->poc);

    p_enc_in->timeIncrement = cfg->output_rate_denom;

    if (stream_size != 0) {
        idx = h26x_enc_get_empty_stream_buffer(enc_ctx);
        if (idx == -1) {
            VPILOGE("no empty stream buffer\n");
            return VPI_ERR_ENCODE;
        }
        outstrm_buf = &enc_ctx->outstream_pkt[idx];
        if (cfg->picture_enc_cnt == 1) {
            outstrm_buf->header_data    = enc_ctx->header_data; //ctx->outPkt->data;
            outstrm_buf->header_size    = enc_ctx->header_size; //ctx->outPkt->size;
        } else {
            if (p_enc_out->resendSPS) {
                VPILOGD("has sps header size = %d\n", p_enc_out->header_size);
                outstrm_buf->header_data = p_enc_out->header_buffer;
                outstrm_buf->header_size = p_enc_out->header_size;
            } else if (p_enc_out->codingType == VCENC_INTRA_FRAME) {
                // add SPS/PPS before IDR for RTP/UDP case
                outstrm_buf->header_data = enc_ctx->header_data;
                outstrm_buf->header_size = enc_ctx->header_size;
            } else {
                outstrm_buf->header_data = NULL;
                outstrm_buf->header_size = 0;
            }
        }

        outstrm_buf->resend_header = p_enc_out->resendSPS;
        outstrm_buf->stream_size   = p_enc_out->streamSize; //streamSize; //pEncOut->streamSize;
        outstrm_buf->pts           = p_enc_out->pts; //tb->input_pic_cnt; //pEncOut->pts;
        outstrm_buf->coding_type   = p_enc_out->codingType;

        if (cfg->first_pts != VID_NOPTS_VALUE) {
            if ((p_enc_out->indexEncoded == 0) || (p_enc_out->codingType == VCENC_INTRA_FRAME)) {
                outstrm_buf->dts = p_enc_out->pts - 8;
                if (p_enc_out->indexEncoded && (outstrm_buf->dts <= cfg->last_out_dts)) {
                    outstrm_buf->dts = cfg->last_out_dts + 1;
                }
            } else {
                outstrm_buf->dts = cfg->last_out_dts + 1;
            }

            if (outstrm_buf->dts >= cfg->first_pts) {
                if (cfg->pts_offset == 0) {
                    cfg->pts_offset = cfg->picture_enc_cnt - 1;
                }
                outstrm_buf->dts += cfg->pts_fix[ ((cfg->picture_enc_cnt-1)-cfg->pts_offset) % 100];
                if (outstrm_buf->dts > outstrm_buf->pts) {
                    outstrm_buf->dts = cfg->last_out_dts + 1;
                }
            }
            if (cfg->last_out_dts && outstrm_buf->dts <= cfg->last_out_dts) {
                outstrm_buf->dts =  cfg->last_out_dts + 1;
            }
        } else {
            if (p_enc_out->indexEncoded == 0) {
                outstrm_buf->dts = p_enc_out->pts - 8;
            } else {
                outstrm_buf->dts = cfg->last_out_dts + 1;
            }
        }
        cfg->last_out_dts = outstrm_buf->dts;
        VPILOGD("out_buffer->pts = %ld, out_buffer->dts = %ld, poc @%d\n",
                 outstrm_buf->pts, outstrm_buf->dts, p_enc_out->indexEncoded);
        VPILOGD("pts %ld, dts %ld, poc @%d\n",
                 (outstrm_buf->pts*1000)/30, (outstrm_buf->dts*1000)/30,
                  p_enc_out->indexEncoded);

        for (i = 0; i < 3; i++) {
            outstrm_buf->ssim[i] = p_enc_out->ssim[i];
        }
        outstrm_buf->max_slice_stream_size = p_enc_out->maxSliceStreamSize;
        outstrm_buf->index_encoded         = p_enc_out->indexEncoded;

        pkt_size = outstrm_buf->resend_header ? outstrm_buf->stream_size :
                        (outstrm_buf->stream_size + outstrm_buf->header_size);

        if (pkt_size > enc_ctx->stream_buf_list[idx]->item_size) {
            VPILOGD("packet size is too large(%d > %d @%d), re-allocing\n",
                 pkt_size, enc_ctx->stream_buf_list[idx]->item_size, idx);
            fbtrans_free_huge_pages(enc_ctx->outstream_mem[idx],
                                    enc_ctx->stream_buf_list[idx]->item_size);
            new_size = NEXT_MULTIPLE(pkt_size, 0x10000);
            enc_ctx->outstream_mem[idx] = fbtrans_get_huge_pages(new_size);
            if (enc_ctx->outstream_mem[idx] == NULL) {
                VPILOGE("get %d size huge page failed\n");
                return VPI_ERR_NO_AP_MEM;
            }
            mem = enc_ctx->outstream_pkt[idx].outbuf_mem;
            mem->rc_busAddress = (ptr_t)enc_ctx->outstream_mem[idx];
            mem->size          = new_size;

            enc_ctx->stream_buf_list[idx]->item_size = new_size;
            outstrm_buf->outbuf_mem->size = new_size;
        }

        VPILOGI("src %p, dst %p\n",
                 out_buffer->outbuf_mem, outstrm_buf->outbuf_mem);
        VPILOGI("idx %d\n", idx);
        VPILOGI("src addr %p, dst addr %p\n",
                 out_buffer->outbuf_mem->busAddress,
                 outstrm_buf->outbuf_mem->rc_busAddress);
        VPILOGI("src size %d, dst size %d\n",
                 out_buffer->outbuf_mem->size, outstrm_buf->outbuf_mem->size);
        VPILOGI("pkt_size %d\n", pkt_size);
        ret = EWLTransDataEP2RC(cfg->ewl, out_buffer->outbuf_mem,
                                outstrm_buf->outbuf_mem, pkt_size);
        if (ret) {
            VPILOGE("copy failed, ret %d\n", ret);
            VPILOGD("pkt_size %d\n", pkt_size);
            return VPI_ERR_SYSTEM;
        }
        enc_ctx->stream_buf_list[idx]->used = 1;
        enc_ctx->stream_buf_list[idx]->item = &enc_ctx->outstream_pkt[idx];
        pthread_mutex_lock(&enc_ctx->h26xe_thd_mutex);
        h26x_enc_buf_list_add(&enc_ctx->stream_buf_head,
                               enc_ctx->stream_buf_list[idx]);
        pthread_mutex_unlock(&enc_ctx->h26xe_thd_mutex);
    }

    return VPI_SUCCESS;
}

/**
 *  h26x_enc_call_vcflush
 *  Call vc8000e sdk's VCEncFlush() function
 */
static VpiRet h26x_enc_call_vcflush(VpiH26xEncCtx *enc_ctx,
                                 VCEncOut *enc_out)
{
    VCEncRet ret                 = VCENC_OK;
    VPIH26xEncCfg *vpi_h26xe_cfg = &enc_ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in            = (VCEncIn *)&(vpi_h26xe_cfg->enc_in);
    VpiEncOutData *out_buffer    = NULL;
    VpiRet ret_value             = VPI_SUCCESS;

    VPILOGD("ctx %p call vcflush\n", enc_ctx);
    out_buffer = &enc_ctx->enc_pkt[0];
    setup_output_buffer(enc_ctx->hantro_encoder, out_buffer, p_enc_in);
    ret = VCEncFlush(enc_ctx->hantro_encoder, p_enc_in, enc_out,
                     &h26x_enc_slice_ready);
    VPILOGD("VCEncFlush ret %d\n", ret);
    switch (ret) {
       case VCENC_FRAME_READY:
            setup_slice_ctl(vpi_h26xe_cfg);
            vpi_h26xe_cfg->picture_enc_cnt++;
            if (enc_out->streamSize == 0) {
                vpi_h26xe_cfg->enc_in.picture_cnt ++;
                break;
            }
            ret_value = h26x_enc_process_frame(enc_ctx, out_buffer);
            if (ret_value!=0) {
                VPILOGE("process error return %d\n", ret_value);
                goto error;
            }
            pthread_mutex_lock(&enc_ctx->h26xe_thd_mutex);
            if (enc_ctx->waiting_for_pkt == 1) {
                pthread_cond_signal(&enc_ctx->h26xe_thd_cond);
                enc_ctx->waiting_for_pkt = 0;
            }
            pthread_mutex_unlock(&enc_ctx->h26xe_thd_mutex);
            h26x_enc_consume_pic(enc_ctx, enc_out->indexEncoded);
            break;
        case VCENC_FRAME_ENQUEUE:
            break;
        case VCENC_OUTPUT_BUFFER_OVERFLOW:
            vpi_h26xe_cfg->enc_in.picture_cnt ++;
            break;
        case VCENC_OK:
            enc_ctx->flush_state = VPIH26X_FLUSH_FINISH;
            break;
        default:
            goto error;
    }

    return ret_value;

error:
    enc_ctx->flush_state = VPIH26X_FLUSH_FINISH;
    return ret_value;
}

VpiRet h26x_enc_frame(VpiH26xEncCtx *ctx)
{
    VPIH26xEncOptions *options = &ctx->options;
    VPIH26xEncCfg *cfg         = (VPIH26xEncCfg *)&ctx->vpi_h26xe_cfg;
    VpiRet ret                 = VPI_SUCCESS;
    VCEncIn *p_enc_in          = (VCEncIn *)&cfg->enc_in;
    VCEncOut *p_enc_out        = (VCEncOut *)&ctx->enc_out;

    VpiEncOutData *out_buffer = NULL;
    VpiH26xEncInAddr addrs    = { 0 };
    int64_t pict_pts          = 0;
    u32 src_img_size;

    u32 i, tmp;
    VCEncRet retValue = 0;
    int width_chroma_align32 = 0;
    int buf_index = -1;

    /* IO buffer */
    get_free_iobuffer(cfg);
    setup_slice_ctl(cfg);

    /* Setup encoder input */
    src_img_size = setup_input_buffer(cfg, options, p_enc_in);

#ifdef SUPPORT_TCACHE
    if ((options->input_format >= INPUT_FORMAT_ARGB_FB
        && options->input_format <= INPUT_FORMAT_YUV444P)
        || options->input_format == VCENC_YUV420_PLANAR
        || options->input_format == VCENC_YUV420_SEMIPLANAR
        || options->input_format == VCENC_YUV420_SEMIPLANAR_VU
        || options->input_format == VCENC_YUV420_PLANAR_10BIT_P010) {
        VPILOGD("raw input through TCache\n");
    } else
#endif
    {
#ifdef USE_OLD_DRV
#endif
    }

    VPILOGD("ctx %p picture_cnt = %d. \n", ctx, p_enc_in->picture_cnt);

    if (p_enc_in->picture_cnt > ctx->poc_bak) {
        ctx->delta_poc = p_enc_in->picture_cnt - ctx->poc_bak;
        if (ctx->delta_poc != 0)
            ctx->delta_poc--;
    }
    ctx->poc_bak = p_enc_in->picture_cnt;

    if (ctx->update_idr_poc == HANTRO_TRUE) {
        update_next_idr_poc(ctx, (int *)&ctx->next_idr_poc);
    }

    ctx->next_poc = h26x_enc_next_picture(cfg, p_enc_in->picture_cnt)
                        + cfg->first_pic;

    p_enc_in->indexTobeEncode = ctx->next_poc;
    VPILOGD("ctx->next_poc = %d, ctx->delta_poc = %d\n",
             ctx->next_poc, ctx->delta_poc);

    if (ctx->flush_state == VPIH26X_FLUSH_IDLE) {
        ret = h26x_enc_send_pic(ctx, ctx->next_poc, &addrs, &pict_pts);
        if (ret < 0) {
            if (ret == VPI_ERR_ENCODE_WAITT_BUF) {
                VPILOGD("[%d]ctx->next_poc = %d. wait next pict\n",
                        cfg->enc_index, ctx->next_poc);
                ctx->trans_flush_pic = HANTRO_TRUE;
                pthread_mutex_lock(&ctx->h26xe_thd_mutex);
                if (ctx->waiting_for_pkt == 1) {
                    ctx->got_frame--;
                    pthread_cond_signal(&ctx->h26xe_thd_cond);
                    ctx->waiting_for_pkt = 0;
                }
                pthread_mutex_unlock(&ctx->h26xe_thd_mutex);
                return VPI_SUCCESS;
            } else {
                return ret;
            }
        }
    } else if (ctx->flush_state == VPIH26X_FLUSH_TRANSPIC) {
        VPILOGI("Begin to flush trans DATA\n");

        if(h26x_enc_send_pic(ctx, ctx->next_poc, &addrs, &pict_pts) < 0) {
            ctx->flush_state = VPIH26X_FLUSH_ENCDATA;
            VPILOGD("No Data Trans, begin to enter VCE Flush\n");
            return VPI_SUCCESS;
        }
    } else {
        /* not should be here */
        VPILOGE("There is some error internal.\n");
        return VPI_ERR_ENCODE;
    }
  //p_enc_in->pts = pict_pts; //??? need or not

    if (cfg->last_pic > 0 && ctx->next_poc > cfg->last_pic) {
        VPILOGE("next_poc = %d, exceed lastPic %d\n",
                 ctx->next_poc, cfg->last_pic);
        return VPI_ERR_ENCODE;
    }

    if (!addrs.bus_luma) {
        VPILOGD("get EOS\n");
        return VPI_SUCCESS;
    }
    if (p_enc_in->poc == 0) {
        p_enc_in->resendSPS = p_enc_in->resendPPS = p_enc_in->resendVPS = 1;
    }

    if (cfg->enc_index == 0
        && options->lookahead_depth
        && options->cutree_blkratio) {
        VPILOGD("set 1pass ds addr\n");

        if (addrs.bus_luma_ds == 0 || addrs.bus_chroma_ds == 0) {
            VPILOGE("1pass ds addr should not be zero!\n");
            return VPI_ERR_ENCODE;
        }

        p_enc_in->busLuma    = addrs.bus_luma_ds;
        p_enc_in->busChromaU = addrs.bus_chroma_ds;
        p_enc_in->busChromaV = 0;
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
        cfg->ts_lumamem->busAddress   = addrs.bus_luma_ds;
        cfg->ts_chromamem->busAddress = addrs.bus_chroma_ds;
#endif

        p_enc_in->busLumaOrig    = addrs.bus_luma;
        p_enc_in->busChromaUOrig = addrs.bus_chroma;
        if (INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_YUV420P
                == options->input_format) {
            width_chroma_align32     = ((options->width / 2 + 31) / 32) * 32;
            p_enc_in->busChromaVOrig = addrs.bus_luma +
                                    width_chroma_align32 * options->height / 2;
        } else {
            p_enc_in->busChromaVOrig = 0;
        }
    } else {
        p_enc_in->busLuma    = addrs.bus_luma;
        p_enc_in->busChromaU = addrs.bus_chroma;
        if (INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_YUV420P
                == options->input_format) {
            width_chroma_align32 = ((options->width / 2 + 31) / 32) * 32;
            p_enc_in->busChromaV = addrs.bus_chroma +
                                width_chroma_align32 * options->height / 2;
        } else {
            p_enc_in->busChromaV = 0;
        }
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
        cfg->ts_lumamem->busAddress   = addrs.bus_luma_table;
        cfg->ts_chromamem->busAddress = addrs.bus_chroma_table;
#endif
    }

    VPILOGD("p_enc_in->busLuma    = %p\n", p_enc_in->busLuma);
    VPILOGD("p_enc_in->busChromaU = %p\n", p_enc_in->busChromaU);

#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
    VPILOGD("TSLumaMem->busAddr = %p\n", cfg->ts_lumamem->busAddress);
    VPILOGD("TSChromaMem->busAd = %p\n", cfg->ts_chromamem->busAddress);
#endif

    ctx->frame_cnt_total++;

    format_customized_yuv(cfg, options, &ret);
    if (ret) {
        VPILOGE("Format customized yuv failed\n");
        goto error;
    }

    /*
     * per-frame test functions
     */

    /* 1. scene changed frames from usr*/
    p_enc_in->sceneChange = 0;
    tmp = h26x_enc_next_picture(cfg, p_enc_in->picture_cnt) + cfg->first_pic;
    for (int i = 0; i < MAX_SCENE_CHANGE; i++) {
        if (options->scene_change[i] == 0) {
            break;
        }
        if (options->scene_change[i] == tmp) {
            p_enc_in->sceneChange = 1;
            break;
        }
    }

    /* 2. GMV setting from user*/
    read_gmv(cfg, p_enc_in, options);

    p_enc_in->codingType =
        (p_enc_in->poc == 0) ? VCENC_INTRA_FRAME : ctx->next_coding_type;
    cfg->input_pic_cnt++;

    if (p_enc_in->codingType == VCENC_INTRA_FRAME &&
        options->gdr_duration == 0 &&
        ((p_enc_in->poc == 0) || (p_enc_in->bIsIDR))) {
        if (!options->lookahead_depth)
            ctx->frame_cnt_output = 0;
    }

    /* 3. On-fly bitrate setting */
    if (options->pic_rc == 1) {
        u32 n_bps;
        VCEncRateCtrl cur_rc;

        if (get_cfg_rc_bitrate(options, &n_bps) == 0) {
            if (( VCEncGetRateCtrl(ctx->hantro_encoder,
                                        &cur_rc)) != VCENC_OK) {
                VPILOGE("VCEncGetRateCtrl failed\n");
                goto error;
            } else {
                if (cur_rc.bitPerSecond != n_bps) {
                    cfg->rc.bitPerSecond = n_bps;
                    VPILOGD("Adjusting bitrate to: %d\n", cfg->rc.bitPerSecond);
                    if ((VCEncSetRateCtrl(ctx->hantro_encoder,
                                    (VCEncRateCtrl *)&cfg->rc)) != VCENC_OK) {
                        VPILOGE("VCEncSetRateCtrl() failed.\n");
                        goto error;
                    }
                }
            }
        }
    }

    /* 4. SetupROI-Map */
    if (setup_roi_map_buffer(cfg, options, p_enc_in, ctx->hantro_encoder)) {
        VPILOGE("Failed to setup ROI map buffer\n");
        goto error;
    }
    /* 5. encoding specific frame from user: all CU/MB are SKIP*/
    p_enc_in->bSkipFrame = options->skip_frame_enabled_flag &&
                           (p_enc_in->poc == options->skip_frame_poc);
    /* 6. low latency */
    if (options->input_line_buf_mode) {
        p_enc_in->lineBufWrCnt =
            VCEncStartInputLineBuffer(&(cfg->input_ctb_linebuf));
    }
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
    p_enc_in->PrivData.PicMemRcBusAddr = cfg->picture_mem->rc_busAddress;

    p_enc_in->PrivData.bitDepthLuma    = options->bit_depth_luma;
    p_enc_in->PrivData.input_alignment = cfg->input_alignment;

    if (addrs.bus_luma_ds) {
        p_enc_in->PrivData.lumaSize     = cfg->luma_size_ds;
        p_enc_in->PrivData.chromaSize   = cfg->chroma_size_ds;
        p_enc_in->PrivData.lumWidthSrc  = options->lum_width_src / 2;
        p_enc_in->PrivData.lumHeightSrc = options->lum_height_src / 2;
        p_enc_in->PrivData.inputFormat  = options->input_format_ds;

        p_enc_in->PrivData.TSLumaMemBusAddress   = addrs.bus_luma_table_ds;
        p_enc_in->PrivData.TSChromaMemBusAddress = addrs.bus_chroma_table_ds;
        p_enc_in->PrivData.busLuma               = addrs.bus_luma_ds;
        p_enc_in->PrivData.busChromaU            = addrs.bus_chroma_ds;
    } else {
        p_enc_in->PrivData.lumaSize     = cfg->luma_size;
        p_enc_in->PrivData.chromaSize   = cfg->chroma_size;
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
        if (addrs.bus_chroma) {
            width_chroma_align32 = ((options->width / 2 + 31) / 32) * 32;
            p_enc_in->PrivData.busChromaV =
                addrs.bus_chroma + width_chroma_align32 * options->height / 2;
        }
        VPILOGD("Priv busLuma %p, busChromaU %p, busChromaV %p \n",
            p_enc_in->PrivData.busLuma, p_enc_in->PrivData.busChromaU,
            p_enc_in->PrivData.busChromaV);
    }
    /* for pass two encoder hardware set */
    p_enc_in->PassTwoHWData.bitDepthLuma    = options->bit_depth_luma;
    p_enc_in->PassTwoHWData.inputFormat     = options->input_format;
    p_enc_in->PassTwoHWData.lumaSize        = cfg->luma_size;
    p_enc_in->PassTwoHWData.chromaSize      = cfg->chroma_size;
    p_enc_in->PassTwoHWData.lumWidthSrc     = options->lum_width_src;
    p_enc_in->PassTwoHWData.lumHeightSrc    = options->lum_height_src;
    p_enc_in->PassTwoHWData.input_alignment = cfg->input_alignment;
    p_enc_in->PassTwoHWData.TSLumaMemBusAddress = addrs.bus_luma_table;
    /*pic_data->luma_table.bus_address; */ /*vpi_h26xe_cfg->ts_lumamem->busAddress;*/
    p_enc_in->PassTwoHWData.TSChromaMemBusAddress = addrs.bus_chroma_table;
    /*pic_data->chroma_table.bus_address; */ /*vpi_h26xe_cfg->ts_chromamem->busAddress;*/
    p_enc_in->PassTwoHWData.PicMemRcBusAddr =
        cfg->picture_mem->rc_busAddress;
    p_enc_in->PassTwoHWData.busLuma = addrs.bus_luma;
    /*pic_data->luma.bus_address; */ /*p_enc_in->busLuma;*/
    p_enc_in->PassTwoHWData.busChromaU =
        addrs.bus_chroma; /*pic_data->chroma.bus_address; p_enc_in->busChromaU;*/
    if (INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_YUV420P == options->input_format) {
        width_chroma_align32 = ((options->width / 2 + 31) / 32) * 32;
        p_enc_in->PassTwoHWData.busChromaV =
            addrs.bus_chroma + width_chroma_align32 * options->height / 2;
    }
    VPILOGD("Pass2 busLuma %p, busChromaU %p, busChromaV %p \n",
        p_enc_in->PassTwoHWData.busLuma, p_enc_in->PassTwoHWData.busChromaU,
        p_enc_in->PassTwoHWData.busChromaV);

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

    out_buffer = &ctx->enc_pkt[0];
    setup_output_buffer(ctx->hantro_encoder, out_buffer, p_enc_in);

    gettimeofday(&cfg->time_frame_start, 0);
    retValue = VCEncStrmEncode(ctx->hantro_encoder, p_enc_in, p_enc_out,
                          &h26x_enc_slice_ready, cfg->slice_ctl);
    gettimeofday(&cfg->time_frame_end, 0);
    if (retValue != VCENC_FRAME_ENQUEUE) {
        VPILOGD("h26x_consume_stored_pic[%d] \n", p_enc_out->indexEncoded);
        h26x_enc_consume_pic(ctx, p_enc_out->indexEncoded);
    }
    VPILOGD("VCEncStrmEncode ret %d\n", retValue);
    switch (retValue)
    {
        case VCENC_FRAME_ENQUEUE:
            //Adaptive GOP size decision
            if (ctx->adaptive_gop && options->lookahead_depth) {
                VPILOGD("got nextGopSize[%d] before from getNextGopSize\n",
                            ctx->next_gop_size);
                get_next_gop_size(cfg, p_enc_in, ctx->hantro_encoder,
                                   &ctx->next_gop_size, &ctx->agop);
                VPILOGD("got nextGopSize[%d] from func getNextGopSize\n",
                                   ctx->next_gop_size);
            } else if(options->lookahead_depth) { // for sync only, not update gopSize
                getPass1UpdatedGopSize(
                          ((struct vcenc_instance *)ctx->hantro_encoder)
                                                    ->lookahead.priv_inst);
            }

            if (ctx->flush_state == VPIH26X_FLUSH_TRANSPIC) {
                ctx->next_gop_size = 1;
            }

            ctx->enc_in_bk      = *p_enc_in;
            ctx->picture_cnt_bk = cfg->enc_in.picture_cnt;

            calc_next_coding_type(ctx);
            p_enc_in->timeIncrement = cfg->output_rate_denom;

            pthread_mutex_lock(&ctx->h26xe_thd_mutex);
            if (ctx->waiting_for_pkt == 1) {
                ctx->got_frame--;
                pthread_cond_signal(&ctx->h26xe_thd_cond);
                ctx->waiting_for_pkt = 0;
            }
            pthread_mutex_unlock(&ctx->h26xe_thd_mutex);
            break;
        case VCENC_FRAME_READY:
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
            if (ReleasePass2InputHwTransformer(ctx->hantro_encoder,
                                               &p_enc_in->PassTwoHWData) < 0) {
                goto error;
            }
#endif
            VPILOGD("encOut->indexEncoded = %d\n", p_enc_out->indexEncoded);
            VPILOGD("encOut->codingType = %d\n", p_enc_out->codingType);
            VPILOGD("encOut->streamSize = %d\n", p_enc_out->streamSize);

            if (p_enc_out->codingType != VCENC_NOTCODED_FRAME)
                cfg->picture_enc_cnt++;
            if (p_enc_out->streamSize == 0) {
                cfg->enc_in.picture_cnt++;
                break;
            }
            VPILOGD("out_buffer %p\n", out_buffer);
            ret = h26x_enc_process_frame(ctx, out_buffer);
            if (ret != 0) {
                VPILOGE("process error return %d\n", ret);
                goto error;
            }
            pthread_mutex_lock(&ctx->h26xe_thd_mutex);
            if (ctx->waiting_for_pkt == 1) {
                ctx->got_frame--;
                pthread_cond_signal(&ctx->h26xe_thd_cond);
                ctx->waiting_for_pkt = 0;
            }
            pthread_mutex_unlock(&ctx->h26xe_thd_mutex);
            //Adaptive GOP size decision
            if (ctx->adaptive_gop) {
                get_next_gop_size(cfg, p_enc_in, ctx->hantro_encoder,
                                   &ctx->next_gop_size, &ctx->agop);
            } else if (options->lookahead_depth) { // for sync only, not update gopSize
                getPass1UpdatedGopSize(
                          ((struct vcenc_instance *)ctx->hantro_encoder)
                                                    ->lookahead.priv_inst);
            }

            if (ctx->flush_state == VPIH26X_FLUSH_TRANSPIC) {
                ctx->next_gop_size = 1;
            }

            ctx->enc_in_bk      = *p_enc_in;
            ctx->picture_cnt_bk = cfg->enc_in.picture_cnt;

            /* add for IDR */
            VPILOGD("ctx->nextIDRPoc = %d, ctx->poc = %d, ctx->next_poc = %d,"
                     "picture_enc_cnt = %d, encIn.picture_cnt = %d\n",
                     ctx->next_idr_poc, ctx->poc, ctx->next_poc,
                     cfg->picture_enc_cnt, cfg->enc_in.picture_cnt);
            VPILOGD("gopPicIdx = %d, gopSize = %d, ctx->nextGopSize = %d\n",
                     p_enc_in->gopPicIdx, p_enc_in->gopSize,
                     ctx->next_gop_size);

            calc_next_coding_type(ctx);
            VPILOGD("pEncIn->gopSize = %d, tb->nextGopSize = %d\n",
                     p_enc_in->gopSize, cfg->next_gop_size);

            if (ctx->p_user_data) {
            /* We want the user data to be written only once so
             * we disable the user data and free the memory after
             * first frame has been encoded. */
                VCEncSetSeiUserData(ctx->hantro_encoder, NULL, 0);
                EWLfree(ctx->p_user_data);
                ctx->p_user_data = NULL;
            }
            break;
        case VCENC_OUTPUT_BUFFER_OVERFLOW:
            cfg->enc_in.picture_cnt++;
            break;
        default:
            VPILOGE("VCEncStrmEncode ret is %d\n", ret);
            goto error;
            break;
    }

    VPILOGD("ctx->frame_cnt_total[%d] \n", ctx->frame_cnt_total);
    return ret;

error:
    buf_index = h26x_enc_get_empty_stream_buffer(ctx);
    if (buf_index == -1) {
        VPILOGD("Can't found empty stream buffer\n");
        return VPI_ERR_ENCODE;
    }
    out_buffer           = &ctx->outstream_pkt[buf_index];
    out_buffer->end_data = HANTRO_TRUE;
    ctx->flush_state     = VPIH26X_FLUSH_ENCEND;
    ctx->h26xe_thd_end   = 1;
    ctx->encode_end      = 1;

    ctx->stream_buf_list[buf_index]->used = 1;
    ctx->stream_buf_list[buf_index]->item = &ctx->outstream_pkt[buf_index];
    pthread_mutex_lock(&ctx->h26xe_thd_mutex);
    h26x_enc_buf_list_add(&ctx->stream_buf_head,
                           ctx->stream_buf_list[buf_index]);
    if (ctx->waiting_for_pkt == 1) {
        pthread_cond_signal(&ctx->h26xe_thd_cond);
        ctx->waiting_for_pkt = 0;
    }
    pthread_mutex_unlock(&ctx->h26xe_thd_mutex);
    VCEncSetError(ctx->hantro_encoder);
    VPILOGE("encode() fails %p\n", ctx->hantro_encoder);
    return VPI_ERR_ENCODE;
}

VpiRet h26x_enc_frame_process(VpiH26xEncCtx *ctx)
{
    VPIH26xEncOptions *options = &ctx->options;
    VPIH26xEncCfg *cfg         = (VPIH26xEncCfg *)&ctx->vpi_h26xe_cfg;
    VCEncIn *p_enc_in          = (VCEncIn *)&(cfg->enc_in);
    VCEncOut *p_enc_out        = (VCEncOut *)&ctx->enc_out;

    VpiRet ret               = VPI_SUCCESS;
    int streamSize = 0;
    int flush_ret = 0;
    int *got_packet = 0;
    int i = 0;

    if (ctx->flush_state == VPIH26X_FLUSH_ERROR
       || ctx->flush_state == VPIH26X_FLUSH_ENCEND) {
           return VPI_SUCCESS;
    }

    if ((ctx->inject_frm_cnt < ctx->hold_buf_num)
       && ctx->force_idr
       && ctx->eos_received == 0) {
        return VPI_SUCCESS;
    }

    if (ctx->got_frame != 1 && ctx->eos_received == 0) {
        return VPI_SUCCESS;
    }

    switch (ctx->flush_state) {
        case VPIH26X_FLUSH_IDLE:
            ctx->trans_flush_pic = HANTRO_FALSE;
            flush_ret = h26x_enc_frame(ctx);
            VPILOGD("ctx %p +++ h26x_enc_frame ret = %d\n", ctx, flush_ret);
            VPILOGD("+++ ctx->trans_flush_pic = %d\n", ctx->trans_flush_pic);

            if (flush_ret < 0) {
                /* need error process */
                VPILOGE("flush_ret %d\n", flush_ret);
                goto error;
            }

            if (ctx->trans_flush_pic == HANTRO_TRUE
                 && ctx->eos_received == 1) {
                ctx->flush_state = VPIH26X_FLUSH_PREPARE;
            }
            break;

        case VPIH26X_FLUSH_PREPARE:
            h26x_enc_flush_set(ctx);
            break;

        case VPIH26X_FLUSH_TRANSPIC: /* flush data in dec fifo */
            flush_ret = h26x_enc_frame(ctx);
            VPILOGD("ctx %p h26x_enc_frame ret = %d\n", ctx, flush_ret);

            if ((flush_ret == -1) || (flush_ret == VCENC_NULL_ARGUMENT)) {
                /* need error process */
                goto error;
            }
            break;

        case VPIH26X_FLUSH_ENCDATA:
            flush_ret = h26x_enc_call_vcflush(ctx, p_enc_out);
            if (flush_ret < 0) {
                VPILOGE("hantro_encode_flush error. ret = %d\n", ret);
                goto error;
            }
            break;

        case VPIH26X_FLUSH_FINISH:
            ret = h26x_encode_end(ctx);
            if (ret == -1) {
                VPILOGE("h26x_encode_end error. ret = %d\n", ret);
                goto error;
            } else if (ret == 1) {
                return VPI_SUCCESS;
            }
            break;

        default:
            break;
    }

    return VPI_SUCCESS;

error:
    ctx->flush_state = VPIH26X_FLUSH_ERROR;
    VPILOGE("%s got error, will return\n", __FUNCTION__);
    return VPI_ERR_ENCODE;
}

void *h26x_encode_process(void *arg)
{
    VpiH26xEncCtx *enc_ctx = (VpiH26xEncCtx *)arg;
    VpiPacket *pkt;
    VpiRet ret               = VPI_SUCCESS;

    while (!enc_ctx->h26xe_thd_end) {
        ret = h26x_enc_frame_process(enc_ctx);
        if (ret == 0) {
            usleep(500);
        } else if (ret == -1) {
            break;
        }
    }

    return NULL;
}

/**
 *  vpi_h26xe_init
 *  Initialize the h26x(h264/hevc) encoder
 *
 *  @Params: enc_ctx The context of Vpi H26x encoder
 *  @Params: enc_cfg The configure of the encoder
 *  @Return: 0 for success, -1 for error
 */
VpiRet vpi_h26xe_init(VpiH26xEncCtx *enc_ctx, VpiH26xEncCfg *enc_cfg)
{
    VpiRet ret               = VPI_SUCCESS;
    i32 i   = 0;
    int max_frames_delay;

    VCEncOut *enc_out            = (VCEncOut *)&enc_ctx->enc_out;
    VCEncInst *hantro_encoder    = &enc_ctx->hantro_encoder;
    VPIH26xEncOptions *options   = &enc_ctx->options;
    VPIH26xEncCfg *vpi_h26xe_cfg = (VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;

    for (i = 0; i < PIC_INDEX_MAX_NUMBER; i++) {
        if (enc_cfg->frame_ctx->pic_info[i].width == enc_cfg->lum_width_src &&
            enc_cfg->frame_ctx->pic_info[i].height == enc_cfg->lum_height_src) {
            break;
        }
    }
    if (i == PIC_INDEX_MAX_NUMBER) {
        VPILOGE("pp_index %d isn't avalid \n", i);
        ret = VPI_ERR_ENCODE;
        goto error_exit;
    }
    enc_ctx->pp_index = i;

    ret = h26x_enc_set_options(enc_ctx, enc_cfg);
    if (ret != 0) {
        goto error_exit;
    }
    memset(vpi_h26xe_cfg, 0, sizeof(VPIH26xEncCfg));
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
    enc_ctx->got_frame        = 0;

    vpi_h26xe_cfg->enc_index = options->enc_index;

    /* the number of output stream buffers */
    vpi_h26xe_cfg->stream_buf_num = MAX_OUT_BUF_NUM;
    vpi_h26xe_cfg->outbuf_index   = -1;

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
        return VPI_ERR_ENCODER_OPITION;
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
            return VPI_ERR_ENCODER_OPITION;
        }
        vpi_h26xe_cfg->enc_in.gopConfig.pGopPicCfgPass1 = enc_ctx->gop_pic_cfg;
        vpi_h26xe_cfg->enc_in.gopConfig.pGopPicCfg =
            vpi_h26xe_cfg->enc_in.gopConfig.pGopPicCfgPass2 =
                enc_ctx->gop_pic_cfg_pass2;
    }

    VPILOGD("lookahead_depth %d, gop_size %d\n",
             options->lookahead_depth, options->gop_size);
    if (options->lookahead_depth) {
        /*2pass need add lookahead number*/
        max_frames_delay = 17 + options->lookahead_depth;
    } else {
        if (options->gop_size == 0) {
            if (enc_ctx->force_idr) {
                /* need store (GOP_size + 1) of data ahead of to calc idr */
                max_frames_delay = MAX_GOP_SIZE + 2 + 1 + MAX_GOP_SIZE + 1;
            } else {
                /* superfast need store 1 more buf,
                when 3 outpus(4 outputs total) occur error need exit */
                max_frames_delay = MAX_GOP_SIZE + 2 + 1;
            }
        } else {
            if (enc_ctx->force_idr) {
                max_frames_delay =
                    options->gop_size + 2 + 1 + options->gop_size + 1;
            } else {
                max_frames_delay = options->gop_size + 2 + 1;
            }
        }
    }

    /*Set the number of hold buffers*/
    if (enc_ctx->force_idr) {
        if (options->lookahead_depth != 0) {
            enc_ctx->hold_buf_num = max_frames_delay;
        } else {
            enc_ctx->hold_buf_num =
                enc_ctx->gop_len + 1 + enc_ctx->gop_len;
        }
    }
    if (max_frames_delay > enc_cfg->frame_ctx->max_frames_delay) {
        enc_cfg->frame_ctx->max_frames_delay = max_frames_delay;
    }
    if (((enc_cfg->frame_ctx->flag & HWUPLOAD_FLAG) == 1) &&
        ((enc_cfg->frame_ctx->flag & PP_FLAG) == 0)) {
        /* hwupload link to encoder directly, no pp filter */
        if (max_frames_delay > enc_cfg->frame_ctx->hwupload_max_frames_delay) {
            enc_cfg->frame_ctx->hwupload_max_frames_delay = max_frames_delay;
        }
        VPILOGD("vpeframe->hwupload_max_frames_delay = %d\n",
            enc_cfg->frame_ctx->hwupload_max_frames_delay);
    }
    VPILOGD("max_frames_delay = %d, lookahead_depth=%d\n",
            enc_cfg->frame_ctx->max_frames_delay, options->lookahead_depth);
    /* Encoder initialization */
#ifdef DRV_NEW_ARCH
    vpi_h26xe_cfg->priority = options->priority;
    vpi_h26xe_cfg->device   = options->device;
    if (vpi_h26xe_cfg->device == NULL) {
        goto error_exit;
    }
    vpi_h26xe_cfg->mem_id = options->mem_id;
#endif
    enc_ctx->num_dec_max = enc_cfg->frame_ctx->max_frames_delay;

    VPILOGD(" hantro_encoder[%p], *hantro_encoder[%p]\n", hantro_encoder,
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
         * at least 4 more buffers are needed to
         * avoid buffer overwrite in pass1 before consumed in pass2*/
        vpi_h26xe_cfg->buffer_cnt = vpi_h26xe_cfg->frame_delay + 4;
    }
    VPILOGD("buffer_cnt %d\n", vpi_h26xe_cfg->buffer_cnt);
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

    /* Set the test ID for internal testing, the SW must be compiled with
     * testing flags
     */
    VCEncSetTestId(*hantro_encoder, options->test_id);

    /* Allocate input and output buffers */
    if ((ret = h26x_enc_alloc_res(enc_ctx, *hantro_encoder)) != 0) {
        goto error_exit;
    }

    /*Init the IDR POC array*/
    enc_ctx->inject_frm_cnt = 0;
    enc_ctx->gop_len =
        (vpi_h26xe_cfg->gop_size == 0) ? 8 : vpi_h26xe_cfg->gop_size;

    idr_poc_array_init(enc_ctx);

    h26x_enc_outbuf_init(enc_ctx);

    if (enc_ctx->encoder_is_start == HANTRO_FALSE) {
        enc_ctx->encoder_is_start = HANTRO_TRUE;

        ret = h26x_enc_start(enc_ctx, enc_out);
        if (ret != 0) {
            ret = VPI_ERR_ENCODE;
            goto error_exit;
        } else {
            enc_ctx->inject_frm_cnt = 1; /*IDR need store data ahead*/
            ret = VPI_SUCCESS;
        }
    }
    pthread_mutex_init(&enc_ctx->h26xe_thd_mutex, NULL);
    for (i = 0; i < MAX_WAIT_DEPTH; i++) {
        pthread_mutex_init(&enc_ctx->pic_wait_list[i].pic_mutex, NULL);
    }
    pthread_cond_init(&enc_ctx->h26xe_thd_cond, NULL);
    enc_ctx->h26xe_thd_end = 0;
    ret = pthread_create(&enc_ctx->h26xe_thd_handle, NULL, h26x_encode_process,
                         enc_ctx);

    return ret;

error_exit:
    VPILOGE("H26x encoder init failed, ret %d\n", ret);
    if (*hantro_encoder) {
        h26x_enc_outbuf_uninit(enc_ctx);
        h26x_enc_free_res(enc_ctx, *hantro_encoder);
        h26x_enc_close_encoder(*hantro_encoder, vpi_h26xe_cfg);
        *hantro_encoder = NULL;
    }

    return ret;
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
int vpi_h26xe_ctrl(VpiH26xEncCtx *enc_ctx, void *indata, void *outdata)
{
    int ret              = 0;
    VpiCtrlCmdParam *cmd = (VpiCtrlCmdParam *)indata;

    switch (cmd->cmd) {
    case VPI_CMD_ENC_GET_EMPTY_FRAME_SLOT:
        ret = h26x_enc_get_pic_buffer(enc_ctx, outdata);
        break;
    case VPI_CMD_ENC_CONSUME_PIC:
        ret = h26x_enc_get_used_pic_mem(enc_ctx, outdata);
        break;
    case VPI_CMD_ENC_GET_FRAME_PACKET:
        ret = h26x_enc_get_frame_packet(enc_ctx, outdata);
        break;
    case VPI_CMD_ENC_GET_EXTRADATA_SIZE:
        ret = h26x_enc_get_extradata_size(enc_ctx, outdata);
        break;
    case VPI_CMD_ENC_GET_EXTRADATA:
        ret = h26x_enc_get_extradata(enc_ctx, cmd->data);
        break;
    case VPI_CMD_ENC_INIT_OPTION: {
        VpiH26xEncCfg **enc_opt;
        enc_opt = (VpiH26xEncCfg **)outdata;
        *enc_opt = (VpiH26xEncCfg *)malloc(sizeof(VpiH26xEncCfg));
        memset(*enc_opt, 0, sizeof(VpiH26xEncCfg));
        return ret;
    }
    default:
        break;
    }
    return ret;
}

VpiRet vpi_h26xe_put_frame(VpiH26xEncCtx *enc_ctx, void *indata)
{
    VpiEncH26xPic *trans_pic = NULL;
    VpiFrame *frame          = (VpiFrame *)indata;
    VPIH26xEncCfg *cfg       = &enc_ctx->vpi_h26xe_cfg;
    int i, pic_num;

    pthread_mutex_lock(&enc_ctx->h26xe_thd_mutex);
    if (enc_ctx->flush_state == VPIH26X_FLUSH_ERROR) {
        for (i = 0; i < MAX_WAIT_DEPTH; i++) {
            if (enc_ctx->rls_pic_list[i]->used == 0) {
                enc_ctx->rls_pic_list[i]->item = frame->opaque;
                enc_ctx->rls_pic_list[i]->used = 1;
                break;
            }
        }
        h26x_enc_buf_list_add(&enc_ctx->rls_pic_head, enc_ctx->rls_pic_list[i]);
        pthread_mutex_unlock(&enc_ctx->h26xe_thd_mutex);
        return 0;
    }
    pthread_mutex_unlock(&enc_ctx->h26xe_thd_mutex);

    /* add to get new vui info */
    if (cfg->enc_first_frame == 0) {
        cfg->enc_first_frame = 1;
        cfg->color_range     = frame->color_range;
        VPILOGD("pict->color_range = %d\n", frame->color_range);
    }

    for (i = 0; i < MAX_WAIT_DEPTH; i++) {
        if (enc_ctx->pic_wait_list[i].pic == frame) {
            trans_pic = &enc_ctx->pic_wait_list[i];
            pic_num = i;
            break;
        }
    }

    if (i == MAX_WAIT_DEPTH) {
        return -1;
    }

    if (frame->opaque == NULL) {
        VPILOGD("ctx %p received empty input frame, EOF\n", enc_ctx);
        pthread_mutex_lock(&trans_pic->pic_mutex);
        trans_pic->poc               = -1;
        trans_pic->in_pass_one_queue = 0;
        trans_pic->state             = 1;
        pthread_mutex_unlock(&trans_pic->pic_mutex);
        enc_ctx->eos_received        = 1;
        if (enc_ctx->force_idr) {
            pthread_mutex_lock(&enc_ctx->h26xe_thd_mutex);
            enc_ctx->inject_frm_cnt = enc_ctx->hold_buf_num + 1;
            pthread_mutex_unlock(&enc_ctx->h26xe_thd_mutex);
        }
    } else {
        pthread_mutex_lock(&enc_ctx->h26xe_thd_mutex);
        enc_ctx->got_frame++;
        pthread_mutex_unlock(&enc_ctx->h26xe_thd_mutex);

        pthread_mutex_lock(&trans_pic->pic_mutex);
        trans_pic->state             = 1;
        trans_pic->used              = 0;
        trans_pic->in_pass_one_queue = 0;
        trans_pic->poc               = enc_ctx->poc;
        pthread_mutex_unlock(&trans_pic->pic_mutex);
        for (i = 0; i < MAX_WAIT_DEPTH; i++) {
            VpiFrame *wait_frame, *input_frame;
            if (enc_ctx->pic_wait_list[i].state == 1 && i != pic_num) {
                wait_frame = enc_ctx->pic_wait_list[i].pic;
                if (wait_frame->data[0] == frame->data[0]) {
                    pthread_mutex_lock(&trans_pic->pic_mutex);
                    input_frame = (VpiFrame *)frame->vpi_opaque;
                    input_frame->nb_outputs++;
                    pthread_mutex_unlock(&trans_pic->pic_mutex);
                    break;
                }
            }
        }
        /* add for IDR */
        if (frame->key_frame && enc_ctx->force_idr) {
            if (enc_ctx->poc > 0) {
                enc_ctx->key_frame_flag = HANTRO_TRUE;
                store_idr_poc(enc_ctx, enc_ctx->poc);
            }
        }
        if (enc_ctx->poc == 0) {
            cfg->first_pts  = frame->pts;
            cfg->pts_fix[0] = 0;
        } else {
            cfg->pts_fix[enc_ctx->poc%100] = frame->pts - (cfg->last_in_pts+1);
        }
        VPILOGD("last_in_pts %ld, pts %ld, tb->pts_fix[%d] = %d, poc @%d\n",
                 cfg->last_in_pts, frame->pts, enc_ctx->poc % 100,
                 cfg->pts_fix[enc_ctx->poc % 100], enc_ctx->poc);
        cfg->last_in_pts = frame->pts;

        enc_ctx->poc++;
        if (enc_ctx->force_idr) {
            pthread_mutex_lock(&enc_ctx->h26xe_thd_mutex);
            enc_ctx->inject_frm_cnt++;
            pthread_mutex_unlock(&enc_ctx->h26xe_thd_mutex);
        }
    }

    return 0;
}

VpiRet vpi_h26xe_get_packet(VpiH26xEncCtx *enc_ctx, void *outdata)
{
    VpiRet ret               = VPI_SUCCESS;
    VPIH26xEncCfg *cfg         = &enc_ctx->vpi_h26xe_cfg;
    VpiEncOutData *out_buffer  = NULL;
    H26xEncBufLink *buf        = NULL;
    VpiPacket *vpi_packet      = (VpiPacket *)outdata;
    VPIH26xEncOptions *options = &enc_ctx->options;
    EWLLinearMem_t packet_mem;

    pthread_mutex_lock(&enc_ctx->h26xe_thd_mutex);
    buf = enc_ctx->stream_buf_head;
    enc_ctx->stream_buf_head =
            h26x_enc_buf_list_delete(enc_ctx->stream_buf_head);
    pthread_mutex_unlock(&enc_ctx->h26xe_thd_mutex);
    out_buffer = (VpiEncOutData *)buf->item;
    if (vpi_packet->size != 0) {
        packet_mem.rc_busAddress = (ptr_t)vpi_packet->data;
        packet_mem.size          = vpi_packet->size;

        if (out_buffer->header_size != 0 &&
            out_buffer->resend_header != HANTRO_TRUE) {
            packet_mem.rc_busAddress += out_buffer->header_size;
            packet_mem.size          -= out_buffer->header_size;
            memcpy(vpi_packet->data, out_buffer->header_data,
                    out_buffer->header_size);
            out_buffer->header_size = 0;
        }

        memcpy((void *)packet_mem.rc_busAddress,
               (void *)out_buffer->outbuf_mem->rc_busAddress,
               packet_mem.size);

        if (out_buffer->resend_header) {
            memcpy(vpi_packet->data, out_buffer->header_data,
                    out_buffer->header_size);
        }
    }

    vpi_packet->pts     = out_buffer->pts;
    vpi_packet->pkt_dts = out_buffer->dts;
    vpi_packet->flags   = (out_buffer->coding_type == VCENC_INTRA_FRAME) ?
                           VPI_PKT_FLAG_KEY : 0;
    enc_ctx->output_pic_cnt++;
    VPILOGD("enc pts %ld, dts %ld, cnt %d, size %d, flags %d\n",
             vpi_packet->pts, vpi_packet->pkt_dts,
             enc_ctx->output_pic_cnt, vpi_packet->size, vpi_packet->flags);

    enc_ctx->total_bits += out_buffer->stream_size * 8;

    cfg->validencoded_framenumber++;
    h26x_enc_ma_add_frame(&enc_ctx->ma, out_buffer->stream_size*8);

    cfg->hwcycle_acc += VCEncGetPerformance(enc_ctx->hantro_encoder);

    VPILOGD("(instance: %x)Encoded frame%i poc=%d bits=%d TotalBits=%lu "
                " averagebitrate=%lu HWCycles=%d maxSliceBytes=%d\n",
                enc_ctx,
                enc_ctx->output_pic_cnt - 1 - (cfg->parallel_core_num - 1),
                out_buffer->index_encoded,
                out_buffer->stream_size*8,
                enc_ctx->total_bits,
                (enc_ctx->total_bits * cfg->output_rate_denom) /
                     ((enc_ctx->output_pic_cnt - (cfg->parallel_core_num-1))
                                * cfg->output_rate_denom),
                VCEncGetPerformance(enc_ctx->hantro_encoder),
                out_buffer->max_slice_stream_size);

    double ssim = out_buffer->ssim[0] * 0.8 +
                  0.1 * (out_buffer->ssim[1] + out_buffer->ssim[2]);
    VPILOGI("SSIM %.4f SSIM Y %.4f U %.4f V %.4f\n", ssim,
             out_buffer->ssim[0], out_buffer->ssim[1], out_buffer->ssim[2]);
    cfg->ssim_acc += ssim;

    if ((options->pic_rc == 1) &&
        (cfg->validencoded_framenumber >= enc_ctx->ma.length)) {
        cfg->number_square_of_error++;
        if (cfg->max_error_over_target <
            (h26x_enc_ma(&enc_ctx->ma) - options->bit_per_second))
            cfg->max_error_over_target =
                (h26x_enc_ma(&enc_ctx->ma) - options->bit_per_second);
        if (cfg->max_error_under_target <
            (options->bit_per_second - h26x_enc_ma(&enc_ctx->ma)))
            cfg->max_error_under_target =
                (options->bit_per_second - h26x_enc_ma(&enc_ctx->ma));
        cfg->sum_square_of_error += ((float)(ABS(h26x_enc_ma(&enc_ctx->ma) -
                   options->bit_per_second)) * 100 / options->bit_per_second);
        cfg->average_square_of_error =
                    (cfg->sum_square_of_error/cfg->number_square_of_error);
        VPILOGD("    RateControl(movingBitrate=%d MaxOvertarget=%d%% "
                    "MaxUndertarget=%d%% AveDeviationPerframe=%f%%)\n",
                h26x_enc_ma(&enc_ctx->ma),
                cfg->max_error_over_target * 100 / options->bit_per_second,
                cfg->max_error_under_target * 100 / options->bit_per_second,
                cfg->average_square_of_error);
    }

    buf->used = 0;
    return ret;
}
/**
 *  vpi_h26xe_close
 *  Close the h26x(h264/hevc) encoder
 *
 *  @Params: enc_ctx The context of Vpi H26x encoder
 *  @Return: 0 for success, -1 for error
 */

VpiRet vpi_h26xe_close(VpiH26xEncCtx *enc_ctx)
{
    VpiRet ret                      = VPI_SUCCESS;
    VPIH26xEncCfg *vpi_h26xe_cfg = &enc_ctx->vpi_h26xe_cfg;
    int i;

    enc_ctx->h26xe_thd_end = 1;

    h26x_enc_report(enc_ctx);
    if (enc_ctx != NULL) {
        if (enc_ctx->hantro_encoder != NULL) {
            pthread_join(enc_ctx->h26xe_thd_handle, NULL);
            pthread_mutex_destroy(&enc_ctx->h26xe_thd_mutex);
            for (i = 0; i < MAX_WAIT_DEPTH; i++) {
                pthread_mutex_destroy(&enc_ctx->pic_wait_list[i].pic_mutex);
            }
            pthread_cond_destroy(&enc_ctx->h26xe_thd_cond);

            h26x_enc_outbuf_uninit(enc_ctx);
            h26x_enc_free_res(enc_ctx, enc_ctx->hantro_encoder);
            h26x_enc_close_encoder(enc_ctx->hantro_encoder, vpi_h26xe_cfg);
        }
    }

    return ret;
}
