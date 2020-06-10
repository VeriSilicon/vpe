/*
 * Copyright 2020 VeriSilicon, Inc.
 *
 *	The program is distributed under terms of BSD license.
 *	You can obtain the copy of the license by visiting:
 *
 *	http://www.opensource.org/licenses/bsd-license.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>

#include "deccfg.h"

#include "vpi_types.h"
#include "vpi_log.h"
#include "vpi_video_dec_cfg.h"
#include "vpi_video_dec_tb_defs.h"
#include "vpi_video_dec_pp.h"
#include "vpi_video_h264dec.h"
#include "vpi_video_hevcdec.h"
#include "vpi_video_vp9dec.h"

VpiRet vpi_check_out_format_for_trans(VpiDecCtx *vpi_ctx, DecOption *dec_cfg)
{
    VPILOGD("transcode %d\n", dec_cfg->transcode);
    if (dec_cfg->transcode == 0) {
        // decoder only
        vpi_ctx->enc_type = VPI_ENC_NONE;
        //vpi_ctx->vce_ds_enable = 0;
        vpi_ctx->disable_dec400 = 1;
        vpi_ctx->disable_dtrc   = 1;
        return VPI_SUCCESS;
    } else {
        vpi_ctx->enc_type = VPI_ENC_VALID;
        // vpi_ctx->vce_ds_enable = 0;
        vpi_ctx->disable_dec400 = 0;
        vpi_ctx->disable_dtrc   = 0;
        return VPI_SUCCESS;
    }
}

VpiRet vpi_dec_init_wrapper(VpiDecCtx *vpi_ctx)
{
    switch (vpi_ctx->dec_fmt) {
    case Dec_HEVC:
        vpi_ctx->vpi_dec_wrapper.init     = vpi_dec_hevc_init;
        vpi_ctx->vpi_dec_wrapper.get_info = vpi_dec_hevc_get_info;
        vpi_ctx->vpi_dec_wrapper.set_info = vpi_dec_hevc_set_info;
        //vpi_ctx->vpi_dec_wrapper.decode = vpi_dec_hevc_decode;
        vpi_ctx->vpi_dec_wrapper.next_picture = vpi_dec_hevc_next_picture;
        vpi_ctx->vpi_dec_wrapper.picture_consumed =
            vpi_dec_hevc_picture_consumed;
        vpi_ctx->vpi_dec_wrapper.end_of_stream = vpi_dec_hevc_end_of_stream;
        vpi_ctx->vpi_dec_wrapper.release       = vpi_dec_hevc_release;
        vpi_ctx->vpi_dec_wrapper.use_extra_frm_buffers =
            HevcDecUseExtraFrmBuffers;
#ifdef USE_EXTERNAL_BUFFER
        vpi_ctx->vpi_dec_wrapper.get_buffer_info = vpi_dec_hevc_get_buffer_info;
        vpi_ctx->vpi_dec_wrapper.add_buffer      = vpi_dec_hevc_add_buffer;
#endif
        break;
    case Dec_VP9:
        vpi_ctx->vpi_dec_wrapper.init     = vpi_dec_vp9_init;
        vpi_ctx->vpi_dec_wrapper.get_info = vpi_dec_vp9_get_info;
        vpi_ctx->vpi_dec_wrapper.set_info = vpi_dec_vp9_set_info;
        //vpi_ctx->vpi_dec_wrapper.decode = vpi_dec_vp9_decode;
        vpi_ctx->vpi_dec_wrapper.next_picture = vpi_dec_vp9_next_picture;
        vpi_ctx->vpi_dec_wrapper.picture_consumed =
            vpi_dec_vp9_picture_consumed;
        vpi_ctx->vpi_dec_wrapper.end_of_stream = vpi_dec_vp9_end_of_stream;
        vpi_ctx->vpi_dec_wrapper.release       = vpi_dec_vp9_release;
        vpi_ctx->vpi_dec_wrapper.use_extra_frm_buffers =
            Vp9DecUseExtraFrmBuffers;
#ifdef USE_EXTERNAL_BUFFER
        vpi_ctx->vpi_dec_wrapper.get_buffer_info = vpi_dec_vp9_get_buffer_info;
        vpi_ctx->vpi_dec_wrapper.add_buffer      = vpi_dec_vp9_add_buffer;
#endif
        break;
    case Dec_H264_H10P:
        vpi_ctx->vpi_dec_wrapper.init     = vpi_dec_h264_init;
        vpi_ctx->vpi_dec_wrapper.get_info = vpi_dec_h264_get_info;
        vpi_ctx->vpi_dec_wrapper.set_info = vpi_dec_h264_set_info;
        //vpi_ctx->vpi_dec_wrapper.decode = HantroDecH264Decode;
        vpi_ctx->vpi_dec_wrapper.next_picture = vpi_dec_h264_next_picture;
        vpi_ctx->vpi_dec_wrapper.picture_consumed =
            vpi_dec_h264_picture_consumed;
        vpi_ctx->vpi_dec_wrapper.end_of_stream = vpi_dec_h264_end_of_stream;
        vpi_ctx->vpi_dec_wrapper.release       = vpi_dec_h264_release;
        vpi_ctx->vpi_dec_wrapper.use_extra_frm_buffers =
            H264DecUseExtraFrmBuffers;
#ifdef USE_EXTERNAL_BUFFER
        vpi_ctx->vpi_dec_wrapper.get_buffer_info = vpi_dec_h264_get_buffer_info;
        vpi_ctx->vpi_dec_wrapper.add_buffer      = vpi_dec_h264_add_buffer;
#endif
        break;

    default:
        VPILOGW("Unknown/Not supported format %d", vpi_ctx->dec_fmt);
        return VPI_ERR_VALUE;
    }
    return VPI_SUCCESS;
}

void vpi_dec_set_default_config(VpiDecCtx *vpi_ctx)
{
    if (vpi_ctx->dec_fmt == Dec_H264_H10P) {
        vpi_ctx->vpi_dec_config.mc_cfg.mc_enable                = 1;
        vpi_ctx->vpi_dec_config.mc_cfg.stream_consumed_callback = NULL;
        vpi_ctx->vpi_dec_config.use_ringbuffer                  = 0;
    } else {
        vpi_ctx->vpi_dec_config.mc_cfg.mc_enable                = 0;
        vpi_ctx->vpi_dec_config.mc_cfg.stream_consumed_callback = NULL;
        vpi_ctx->vpi_dec_config.use_ringbuffer                  = 1;
    }
    vpi_ctx->vpi_dec_config.disable_picture_reordering     = 0;
    vpi_ctx->vpi_dec_config.concealment_mode               = DEC_EC_FAST_FREEZE;
    vpi_ctx->vpi_dec_config.align                          = DEC_ALIGN_1024B;
    vpi_ctx->vpi_dec_config.decoder_mode                   = DEC_NORMAL;
    vpi_ctx->vpi_dec_config.tile_by_tile                   = 0;
    vpi_ctx->vpi_dec_config.fscale_cfg.fixed_scale_enabled = 0;
    vpi_ctx->vpi_dec_config.use_video_compressor           = 1;

    vpi_ctx->vpi_dec_config.max_num_pics_to_decode = 0;
    vpi_ctx->vpi_dec_config.output_format          = DEC_OUT_FRM_TILED_4X4;
    vpi_ctx->vpi_dec_config.use_8bits_output       = 0;
    vpi_ctx->vpi_dec_config.use_bige_output        = 0;
    vpi_ctx->vpi_dec_config.use_p010_output        = 0;
}

void vpi_dec_init_hw_cfg(VpiDecCtx *vpi_ctx, DecOption *dec_cfg)
{
    vpi_ctx->stream_stop           = NULL;
    vpi_ctx->align                 = DEC_ALIGN_1024B;
    vpi_ctx->clock_gating          = DEC_X170_INTERNAL_CLOCK_GATING;
    vpi_ctx->data_discard          = DEC_X170_DATA_DISCARD_ENABLE;
    vpi_ctx->latency_comp          = DEC_X170_LATENCY_COMPENSATION;
    vpi_ctx->output_picture_endian = DEC_X170_OUTPUT_PICTURE_ENDIAN;
    vpi_ctx->bus_burst_length      = DEC_X170_BUS_BURST_LENGTH;
    vpi_ctx->asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
    vpi_ctx->output_format         = DEC_X170_OUTPUT_FORMAT;
    vpi_ctx->service_merge_disable = DEC_X170_SERVICE_MERGE_DISABLE;
    if (vpi_ctx->dec_fmt == Dec_H264_H10P) {
        vpi_ctx->tiled_output         = DEC_REF_FRM_RASTER_SCAN;
        vpi_ctx->dwl_init.client_type = DWL_CLIENT_TYPE_H264_DEC;
        vpi_ctx->mb_error_concealment = 0;
    } else if (vpi_ctx->dec_fmt == Dec_HEVC) {
        vpi_ctx->tiled_output         = DEC_REF_FRM_TILED_DEFAULT;
        vpi_ctx->dwl_init.client_type = DWL_CLIENT_TYPE_HEVC_DEC;
    } else if (vpi_ctx->dec_fmt == Dec_VP9) {
        vpi_ctx->tiled_output         = DEC_REF_FRM_TILED_DEFAULT;
        vpi_ctx->dwl_init.client_type = DWL_CLIENT_TYPE_VP9_DEC;
        vpi_ctx->mb_error_concealment = 0;
    }
    vpi_ctx->dpb_mode                       = DEC_DPB_FRAME;
    vpi_ctx->pp_units_params_from_cmd_valid = 0;
    vpi_ctx->dwl_init.dec_dev_name          = dec_cfg->dev_name;
    vpi_ctx->dwl_init.priority              = dec_cfg->priority;

    memset(vpi_ctx->ext_buffers, 0, sizeof(vpi_ctx->ext_buffers));
    vpi_ctx->buffer_release_flag = 1;
    vpi_ctx->cycle_count         = 0;
}

void vpi_dec_get_tb_cfg(VpiDecCtx *vpi_ctx)
{
    struct TBCfg *tb_cfg = &vpi_ctx->tb_cfg;

    tb_cfg->pp_params.pipeline_e = 1;
    if (vpi_ctx->dec_fmt == Dec_H264_H10P || vpi_ctx->dec_fmt == Dec_VP9) {
        vpi_ctx->rlc_mode     = vpi_tb_get_dec_rlc_mode_forced(tb_cfg);
        vpi_ctx->clock_gating = vpi_tb_get_dec_clock_gating(tb_cfg);
        vpi_ctx->data_discard = vpi_tb_get_dec_data_discard(tb_cfg);
        vpi_ctx->latency_comp = tb_cfg->dec_params.latency_compensation;
        vpi_ctx->output_picture_endian =
            vpi_tb_get_dec_output_picture_endian(tb_cfg);
        vpi_ctx->bus_burst_length = tb_cfg->dec_params.bus_burst_length;
        vpi_ctx->asic_service_priority =
            tb_cfg->dec_params.asic_service_priority;
        vpi_ctx->output_format = vpi_tb_get_dec_output_format(tb_cfg);
        vpi_ctx->service_merge_disable =
            vpi_tb_get_dec_service_merge_disable(tb_cfg);
        vpi_ctx->seed_rnd = tb_cfg->tb_params.seed_rnd;
    }

    if (vpi_ctx->dec_fmt == Dec_VP9) {
        vpi_ctx->bus_width = vpi_tb_get_dec_bus_width(tb_cfg);

        vpi_ctx->strm_swap  = tb_cfg->dec_params.strm_swap;
        vpi_ctx->pic_swap   = tb_cfg->dec_params.pic_swap;
        vpi_ctx->dirmv_swap = tb_cfg->dec_params.dirmv_swap;
        vpi_ctx->tab0_swap  = tb_cfg->dec_params.tab0_swap;
        vpi_ctx->tab1_swap  = tb_cfg->dec_params.tab1_swap;
        vpi_ctx->tab2_swap  = tb_cfg->dec_params.tab2_swap;
        vpi_ctx->tab3_swap  = tb_cfg->dec_params.tab3_swap;
        vpi_ctx->rscan_swap = tb_cfg->dec_params.rscan_swap;
        vpi_ctx->max_burst  = tb_cfg->dec_params.max_burst;

        vpi_ctx->double_ref_buffer =
            tb_cfg->dec_params.ref_double_buffer_enable;
        vpi_ctx->timeout_cycles = tb_cfg->dec_params.timeout_cycles;
    }
}

int vpi_dec_cfg_by_seqeuence_info(VpiDecCtx *vpi_ctx)
{
    /* process pp size -1/-2/-4/-8. */
    VPILOGD("%s\n", __FUNCTION__);

    enum DecRet rv_info      = DEC_OK;
    struct DecConfig *config = &vpi_ctx->vpi_dec_config;

    int i;
    uint32_t alignh = vpi_ctx->sequence_info.is_interlaced ? 4 : 2;
    uint32_t alignw = 2;

    VPILOGD("sequence_info: %dx%d, (%d,%d,%dx%d)\n",
            vpi_ctx->sequence_info.pic_width, vpi_ctx->sequence_info.pic_height,
            vpi_ctx->sequence_info.crop_params.crop_left_offset,
            vpi_ctx->sequence_info.crop_params.crop_top_offset,
            vpi_ctx->sequence_info.crop_params.crop_out_width,
            vpi_ctx->sequence_info.crop_params.crop_out_height);
    dump_ppu(config->ppu_cfg);

    if (config->ppu_cfg[0].scale.enabled &&
        ((config->ppu_cfg[0].scale.width != config->ppu_cfg[0].crop.width) ||
         (config->ppu_cfg[0].scale.height != config->ppu_cfg[0].crop.height))) {
        VPILOGE("pp0 do not support scale!\n");
        return -1;
    }

    for (i = 1; i < 4; i++) {
        if (config->ppu_cfg[i].scale.width == -1 ||
            config->ppu_cfg[i].scale.width == -2 ||
            config->ppu_cfg[i].scale.width == -4 ||
            config->ppu_cfg[i].scale.width == -8 ||
            config->ppu_cfg[i].scale.height == -1 ||
            config->ppu_cfg[i].scale.height == -2 ||
            config->ppu_cfg[i].scale.height == -4 ||
            config->ppu_cfg[i].scale.height == -8) {
            uint32_t original_width  = vpi_ctx->sequence_info.pic_width;
            uint32_t original_height = vpi_ctx->sequence_info.pic_height;
            if (config->ppu_cfg[i].scale.width == -1 &&
                config->ppu_cfg[i].scale.height == -1) {
                VPILOGE("ppu_cfg[%d] scale width and scale height \
                        should not be -1 at the same time\n",
                        i);
                rv_info = DEC_INFOPARAM_ERROR;
                break;
            }
            if (vpi_ctx->sequence_info.crop_params.crop_out_width !=
                original_width) {
                original_width =
                    vpi_ctx->sequence_info.crop_params.crop_out_width;
            }
            if (vpi_ctx->sequence_info.crop_params.crop_out_height !=
                original_height) {
                original_height =
                    vpi_ctx->sequence_info.crop_params.crop_out_height;
            }
            if (config->ppu_cfg[i].crop.enabled) {
                if (config->ppu_cfg[i].crop.width != original_width) {
                    original_width = config->ppu_cfg[i].crop.width;
                }
                if (config->ppu_cfg[i].crop.height != original_height) {
                    original_height = config->ppu_cfg[i].crop.height;
                }
            }
            VPILOGD("original_width = %d, original_height = %d\n",
                    original_width, original_height);
            if (config->ppu_cfg[i].scale.width == -1) {
                config->ppu_cfg[i].scale.width =
                    NEXT_MULTIPLE((original_width *
                                   config->ppu_cfg[i].scale.height) /
                                      original_height,
                                  alignw);
                config->ppu_cfg[i].scale.height =
                    NEXT_MULTIPLE(config->ppu_cfg[i].scale.height, alignh);
            } else if (config->ppu_cfg[i].scale.height == -1) {
                config->ppu_cfg[i].scale.width =
                    NEXT_MULTIPLE(config->ppu_cfg[i].scale.width, alignw);
                config->ppu_cfg[i].scale.height =
                    NEXT_MULTIPLE((original_height *
                                   config->ppu_cfg[i].scale.width) /
                                      original_width,
                                  alignh);
            } else if (config->ppu_cfg[i].scale.width == -2 &&
                       config->ppu_cfg[i].scale.height == -2) {
                config->ppu_cfg[i].scale.width =
                    NEXT_MULTIPLE(original_width / 2, alignw);
                config->ppu_cfg[i].scale.height =
                    NEXT_MULTIPLE(original_height / 2, alignh);
            } else if (config->ppu_cfg[i].scale.width == -4 &&
                       config->ppu_cfg[i].scale.height == -4) {
                config->ppu_cfg[i].scale.width =
                    NEXT_MULTIPLE(original_width / 4, alignw);
                config->ppu_cfg[i].scale.height =
                    NEXT_MULTIPLE(original_height / 4, alignh);
            } else if (config->ppu_cfg[i].scale.width == -8 &&
                       config->ppu_cfg[i].scale.height == -8) {
                config->ppu_cfg[i].scale.width =
                    NEXT_MULTIPLE(original_width / 8, alignw);
                config->ppu_cfg[i].scale.height =
                    NEXT_MULTIPLE(original_height / 8, alignh);
            } else {
                VPILOGE("pp %d scale setting error!!!\n", i);
                rv_info = DEC_INFOPARAM_ERROR;
                break;
            }
            VPILOGD("config->ppu_cfg[%d].scale.width = %d, \
                     config->ppu_cfg[%d].scale.height = %d\n",
                    i, config->ppu_cfg[i].scale.width, i,
                    config->ppu_cfg[i].scale.height);
        }
    }
    if (rv_info == DEC_INFOPARAM_ERROR) {
        return -1;
    }

    /* Adjust user cropping params based on cropping params from seq info. */
    if (vpi_ctx->sequence_info.crop_params.crop_left_offset != 0 ||
        vpi_ctx->sequence_info.crop_params.crop_top_offset != 0 ||
        vpi_ctx->sequence_info.crop_params.crop_out_width !=
            vpi_ctx->sequence_info.pic_width ||
        vpi_ctx->sequence_info.crop_params.crop_out_height !=
            vpi_ctx->sequence_info.pic_height) {
        VPILOGD("%s(%d)\n", __FUNCTION__, __LINE__);
        for (i = 1; i < 4; i++) {
            if (!config->ppu_cfg[i].enabled) {
                continue;
            }

            if (!config->ppu_cfg[i].crop.enabled) {
                config->ppu_cfg[i].crop.x =
                    vpi_ctx->sequence_info.crop_params.crop_left_offset;
                config->ppu_cfg[i].crop.y =
                    vpi_ctx->sequence_info.crop_params.crop_top_offset;
                config->ppu_cfg[i].crop.width =
                    NEXT_MULTIPLE(vpi_ctx->sequence_info.crop_params
                                      .crop_out_width,
                                  2);
                config->ppu_cfg[i].crop.height =
                    NEXT_MULTIPLE(vpi_ctx->sequence_info.crop_params
                                      .crop_out_height,
                                  2);
            } else {
                config->ppu_cfg[i].crop.x +=
                    vpi_ctx->sequence_info.crop_params.crop_left_offset;
                config->ppu_cfg[i].crop.y +=
                    vpi_ctx->sequence_info.crop_params.crop_top_offset;
                if (!config->ppu_cfg[i].crop.width) {
                    config->ppu_cfg[i].crop.width =
                        vpi_ctx->sequence_info.crop_params.crop_out_width;
                }
                if (!config->ppu_cfg[i].crop.height) {
                    config->ppu_cfg[i].crop.height =
                        vpi_ctx->sequence_info.crop_params.crop_out_height;
                }
            }
            config->ppu_cfg[i].enabled      = 1;
            config->ppu_cfg[i].crop.enabled = 1;

            VPILOGD("config->ppu_cfg[%d].crop.x = %d, \
                     config->ppu_cfg[%d].crop.y = %d\n",
                    i, config->ppu_cfg[i].crop.x, i, config->ppu_cfg[i].crop.y);
            VPILOGD("config->ppu_cfg[%d].crop.width = %d, \
                     config->ppu_cfg[%d].crop.height = %d\n",
                    i, config->ppu_cfg[i].crop.width, i,
                    config->ppu_cfg[i].crop.height);
        }
    }

    {
        // for bypass mode or 10bit case use pp0 datapath for performance
        if (config->ppu_cfg[0].enabled == 2) {
            VPILOGD("sequence_info.bit_depth_luma = %d, \
                     sequence_info.bit_depth_chroma = %d\n",
                    vpi_ctx->sequence_info.bit_depth_luma,
                    vpi_ctx->sequence_info.bit_depth_chroma);
            if (vpi_ctx->sequence_info.bit_depth_luma > 8 ||
                vpi_ctx->sequence_info.bit_depth_chroma > 8) {
                VPILOGD("adaptive to enable pp0\n");
                config->ppu_cfg[0].enabled = 1;
            } else {
                VPILOGD("adaptive to disable pp0\n");
                config->ppu_cfg[0].enabled = 0;
            }
        }
    }

    double sr[4], sr_sum     = 0;
    double sr_p[4], sr_p_sum = 0;
    int max_input_pic_hit        = 1080;
    int max_output_pic_hit[4]    = { 0, 1080, 720, 360 };
    int pp_max_output_pic_hit[4] = { 0, 0, 1280, 640 };
    int shaper_en_num            = 0;

    //check PP setting legal
    for (i = 0; i < 4; i++) {
        if (config->ppu_cfg[i].enabled == 1) {
            if (pp_max_output_pic_hit[i] == 0) {
                if (config->ppu_cfg[i].scale.height >
                    vpi_ctx->sequence_info.pic_height) {
                    VPILOGE("PP[%d] Height setting is illegal: %d > MAX(%d)\n",
                            i, config->ppu_cfg[i].scale.height,
                            vpi_ctx->sequence_info.pic_height);
                    rv_info = DEC_INFOPARAM_ERROR;
                    break;
                }
                if (config->ppu_cfg[i].scale.width >
                    vpi_ctx->sequence_info.pic_width) {
                    VPILOGE("PP[%d] Width setting is illegal: %d > MAX(%d)\n",
                            i, config->ppu_cfg[i].scale.width,
                            vpi_ctx->sequence_info.pic_width);
                    rv_info = DEC_INFOPARAM_ERROR;
                    break;
                }
            } else {
                if (config->ppu_cfg[i].scale.height >
                    pp_max_output_pic_hit[i]) {
                    VPILOGE("PP[%d] Height setting is illegal: %d > MAX(%d)\n",
                            i, config->ppu_cfg[i].scale.height,
                            pp_max_output_pic_hit[i]);
                    rv_info = DEC_INFOPARAM_ERROR;
                    break;
                }
                if (config->ppu_cfg[i].scale.width > pp_max_output_pic_hit[i]) {
                    VPILOGE("PP[%d] Width setting is illegal: %d > MAX(%d)\n",
                            i, config->ppu_cfg[i].scale.width,
                            pp_max_output_pic_hit[i]);
                    rv_info = DEC_INFOPARAM_ERROR;
                    break;
                }
            }
        }
    }
    if (rv_info == DEC_INFOPARAM_ERROR) {
        return -1;
    }

    if (vpi_ctx->disable_dec400) {
        //disable all shaper for pp
        vpi_dec_disable_all_pp_shaper(&vpi_ctx->vpi_dec_config);
    } else {
        //check SR ratio capability
        if (vpi_ctx->sequence_info.pic_height > 1088) {
            shaper_en_num = 3;
        } else {
            shaper_en_num = 2;
        }

        for (i = 0; i < 4; i++) {
            sr[i] = (double)max_output_pic_hit[i] / max_input_pic_hit;
            sr_sum += sr[i];
        }

        for (i = 0; i < 4; i++) {
            if (config->ppu_cfg[i].enabled == 1) {
                //10bit stream disable shaper when shaper_enabled = 2
                if ((i == 0) && (config->ppu_cfg[i].shaper_enabled == 2)) {
                    if ((vpi_ctx->sequence_info.bit_depth_luma > 8) ||
                        (vpi_ctx->sequence_info.bit_depth_chroma > 8)) {
                        //disable when 10bit
                        config->ppu_cfg[i].shaper_enabled = 0;
                        VPILOGD("ppu_cfg[%d].shaper_enabled disabled\n", i);
                        continue;
                    } else {
                        //enable when 8bit
                        config->ppu_cfg[i].shaper_enabled = 1;
                    }
                }

                if (config->ppu_cfg[i].scale.enabled == 0) {
                    sr_p[i] = 1;
                } else if (config->ppu_cfg[i].crop.enabled) {
                    sr_p[i] = (double)config->ppu_cfg[i].scale.height /
                              config->ppu_cfg[i].crop.height;
                } else {
                    sr_p[i] = (double)config->ppu_cfg[i].scale.height /
                              vpi_ctx->sequence_info.pic_height;
                }
                sr_p_sum += sr_p[i];
            }
            VPILOGD("PP%d enabled=%d, SR_sum=%f, SR_P_sum=%f\n", i,
                    config->ppu_cfg[i].enabled, sr_sum, sr_p_sum);
            if ((shaper_en_num > 0) && (sr_p_sum < sr_sum) &&
                (config->ppu_cfg[i].enabled == 1)) {
                config->ppu_cfg[i].shaper_enabled = 1;
                shaper_en_num--;
            } else {
                config->ppu_cfg[i].shaper_enabled = 0;
                VPILOGD("ppu_cfg[%d].shaper_enabled disabled\n", i);
            }
        }
    }

    VPILOGD("in %s : %d ppu cfg :\n", __func__, __LINE__);

    if (vpi_ctx->enc_type == VPI_ENC_NONE) {
        config->ppu_cfg[0].enabled = 1;
        for (int i = 0; i < 4; i++) {
            if (config->ppu_cfg[i].enabled == 1) {
                config->ppu_cfg[i].tiled_e        = 0;
                config->ppu_cfg[i].shaper_enabled = 0;
                VPILOGD("ppu_cfg[%d].shaper_enabled disabled\n", i);
                config->ppu_cfg[i].align    = DEC_ALIGN_1B;
                config->ppu_cfg[i].out_p010 = 1;
            }
        }
    } else if (vpi_ctx->sequence_info.is_interlaced) {
        config->ppu_cfg[0].enabled = 1;
        for (int i = 0; i < 4; i++) {
            if (config->ppu_cfg[i].enabled == 1) {
                config->ppu_cfg[i].tiled_e        = 0;
                config->ppu_cfg[i].shaper_enabled = 0;
                config->ppu_cfg[i].align          = DEC_ALIGN_1024B;
            }
        }
    }

    dump_ppu(config->ppu_cfg);

    return 0;
}
