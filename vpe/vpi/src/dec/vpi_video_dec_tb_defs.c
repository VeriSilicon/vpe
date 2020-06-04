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

#include "deccfg.h"
#include "ppcfg.h"

#include "vpi_video_dec_tb_defs.h"

void vpi_tb_set_default_cfg(struct TBCfg *tb_cfg)
{
    /* TbParams */
    strcpy(tb_cfg->tb_params.packet_by_packet, "DISABLED");
    strcpy(tb_cfg->tb_params.nal_unit_stream, "DISABLED");
    tb_cfg->tb_params.seed_rnd = 1;
    strcpy(tb_cfg->tb_params.stream_bit_swap, "0");
    strcpy(tb_cfg->tb_params.stream_bit_loss, "0");
    strcpy(tb_cfg->tb_params.stream_packet_loss, "0");
    strcpy(tb_cfg->tb_params.stream_header_corrupt, "DISABLED");
    strcpy(tb_cfg->tb_params.stream_truncate, "DISABLED");
    strcpy(tb_cfg->tb_params.slice_ud_in_packet, "DISABLED");
    tb_cfg->tb_params.memory_page_size                    = 1;
    tb_cfg->tb_params.ref_frm_buffer_size                 = -1;
    tb_cfg->tb_params.first_trace_frame                   = 0;
    tb_cfg->tb_params.extra_cu_ctrl_eof                   = 0;
    tb_cfg->tb_params.unified_reg_fmt                     = 0;
    tb_cfg->dec_params.force_mpeg4_idct                   = 0;
    tb_cfg->dec_params.ref_buffer_test_mode_offset_enable = 0;
    tb_cfg->dec_params.ref_buffer_test_mode_offset_min    = -256;
    tb_cfg->dec_params.ref_buffer_test_mode_offset_start  = -256;
    tb_cfg->dec_params.ref_buffer_test_mode_offset_max    = 255;
    tb_cfg->dec_params.ref_buffer_test_mode_offset_incr   = 16;
    tb_cfg->dec_params.apf_threshold_disable              = 1;
    tb_cfg->dec_params.apf_threshold_value                = -1;
    tb_cfg->dec_params.apf_disable                        = 0;
    tb_cfg->dec_params.field_dpb_support                  = 0;
    tb_cfg->dec_params.service_merge_disable              = 0;

    /* Enable new features by default */
    tb_cfg->dec_params.hevc_main10_support   = 1;
    tb_cfg->dec_params.vp9_profile2_support  = 1;
    tb_cfg->dec_params.ds_support            = 1;
    tb_cfg->dec_params.rfc_support           = 1;
    tb_cfg->dec_params.ring_buffer_support   = 1;
    tb_cfg->dec_params.addr64_support        = 1;
    tb_cfg->dec_params.service_merge_disable = 0;
    tb_cfg->dec_params.mrb_prefetch          = 1;

    /* Output pixel format by default */
    tb_cfg->dec_params.format_p010_support      = 1;
    tb_cfg->dec_params.format_customer1_support = 1;

    /* DecParams */
#if (DEC_X170_OUTPUT_PICTURE_ENDIAN == DEC_X170_BIG_ENDIAN)
    strcpy(tb_cfg->dec_params.output_picture_endian, "BIG_ENDIAN");
#else
    strcpy(tb_cfg->dec_params.output_picture_endian, "LITTLE_ENDIAN");
#endif

    tb_cfg->dec_params.bus_burst_length      = DEC_X170_BUS_BURST_LENGTH;
    tb_cfg->dec_params.asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;

#if (DEC_X170_OUTPUT_FORMAT == DEC_X170_OUTPUT_FORMAT_RASTER_SCAN)
    strcpy(tb_cfg->dec_params.output_format, "RASTER_SCAN");
#else
    strcpy(tb_cfg->dec_params.output_format, "TILED");
#endif

    tb_cfg->dec_params.latency_compensation = DEC_X170_LATENCY_COMPENSATION;

    tb_cfg->dec_params.clk_gate_decoder = DEC_X170_INTERNAL_CLOCK_GATING;
    tb_cfg->dec_params.clk_gate_decoder_idle =
        DEC_X170_INTERNAL_CLOCK_GATING_RUNTIME;

#if (DEC_X170_INTERNAL_CLOCK_GATING == 0)
    strcpy(tb_cfg->dec_params.clock_gating, "DISABLED");
#else
    strcpy(tb_cfg->dec_params.clock_gating, "ENABLED");
#endif

#if (DEC_X170_DATA_DISCARD_ENABLE == 0)
    strcpy(tb_cfg->dec_params.data_discard, "DISABLED");
#else
    strcpy(tb_cfg->dec_params.data_discard, "ENABLED");
#endif

    strcpy(tb_cfg->dec_params.memory_allocation, "INTERNAL");
    strcpy(tb_cfg->dec_params.rlc_mode_forced, "DISABLED");
    strcpy(tb_cfg->dec_params.error_concealment, "PICTURE_FREEZE");
    tb_cfg->dec_params.stride_support         = 0;
    tb_cfg->dec_params.jpeg_mcus_slice        = 0;
    tb_cfg->dec_params.jpeg_input_buffer_size = 0;
    tb_cfg->dec_params.ch8_pix_ileav_output   = 0;

    tb_cfg->dec_params.tiled_ref_support = tb_cfg->pp_params.tiled_ref_support =
        0;

    tb_cfg->dec_params.refbu_enable              = 0;
    tb_cfg->dec_params.refbu_disable_interlaced  = 1;
    tb_cfg->dec_params.refbu_disable_double      = 1;
    tb_cfg->dec_params.refbu_disable_eval_mode   = 1;
    tb_cfg->dec_params.refbu_disable_checkpoint  = 1;
    tb_cfg->dec_params.refbu_disable_offset      = 1;
    tb_cfg->dec_params.refbu_disable_top_bot_sum = 1;
#ifdef DEC_X170_REFBU_ADJUST_VALUE
    tb_cfg->dec_params.refbu_data_excess_max_pct = DEC_X170_REFBU_ADJUST_VALUE;
#else
    tb_cfg->dec_params.refbu_data_excess_max_pct = 130;
#endif

    tb_cfg->dec_params.mpeg2_support         = 1;
    tb_cfg->dec_params.vc1_support           = 3; /* Adv profile */
    tb_cfg->dec_params.jpeg_support          = 1;
    tb_cfg->dec_params.mpeg4_support         = 2; /* ASP */
    tb_cfg->dec_params.h264_support          = 3; /* High */
    tb_cfg->dec_params.vp6_support           = 1;
    tb_cfg->dec_params.vp7_support           = 1;
    tb_cfg->dec_params.vp8_support           = 1;
    tb_cfg->dec_params.prog_jpeg_support     = 1;
    tb_cfg->dec_params.sorenson_support      = 1;
    tb_cfg->dec_params.custom_mpeg4_support  = 1; /* custom feature 1 */
    tb_cfg->dec_params.avs_support           = 2; /* AVS Plus */
    tb_cfg->dec_params.rv_support            = 1;
    tb_cfg->dec_params.mvc_support           = 1;
    tb_cfg->dec_params.webp_support          = 1;
    tb_cfg->dec_params.ec_support            = 0;
    tb_cfg->dec_params.jpeg_esupport         = 0;
    tb_cfg->dec_params.support_non_compliant = 1;
    tb_cfg->dec_params.max_dec_pic_width     = 4096;
    tb_cfg->dec_params.max_dec_pic_height    = 2304;
    tb_cfg->dec_params.hw_version            = 18001;
    tb_cfg->dec_params.hw_build              = 7000;
    tb_cfg->dec_params.hw_build_id           = 0xFFFF;

    tb_cfg->dec_params.bus_width   = DEC_X170_BUS_WIDTH;
    tb_cfg->dec_params.latency     = DEC_X170_REFBU_LATENCY;
    tb_cfg->dec_params.non_seq_clk = DEC_X170_REFBU_NONSEQ;
    tb_cfg->dec_params.seq_clk     = DEC_X170_REFBU_SEQ;

    tb_cfg->dec_params.strm_swap     = HANTRODEC_STREAM_SWAP;
    tb_cfg->dec_params.pic_swap      = HANTRODEC_STREAM_SWAP;
    tb_cfg->dec_params.dirmv_swap    = HANTRODEC_STREAM_SWAP;
    tb_cfg->dec_params.tab0_swap     = HANTRODEC_STREAM_SWAP;
    tb_cfg->dec_params.tab1_swap     = HANTRODEC_STREAM_SWAP;
    tb_cfg->dec_params.tab2_swap     = HANTRODEC_STREAM_SWAP;
    tb_cfg->dec_params.tab3_swap     = HANTRODEC_STREAM_SWAP;
    tb_cfg->dec_params.rscan_swap    = HANTRODEC_STREAM_SWAP;
    tb_cfg->dec_params.comp_tab_swap = HANTRODEC_STREAM_SWAP;
    tb_cfg->dec_params.max_burst     = HANTRODEC_MAX_BURST;
    tb_cfg->dec_params.ref_double_buffer_enable =
        HANTRODEC_INTERNAL_DOUBLE_REF_BUFFER;
    tb_cfg->dec_params.timeout_cycles = HANTRODEC_TIMEOUT_OVERRIDE;
#ifdef DEC_X170_REFBU_SEQ
    tb_cfg->dec_params.apf_threshold_value =
        DEC_X170_REFBU_NONSEQ / DEC_X170_REFBU_SEQ;
#else
    tb_cfg->dec_params.apf_threshold_value       = DEC_X170_REFBU_NONSEQ;
#endif
    tb_cfg->dec_params.apf_disable      = DEC_X170_APF_DISABLE;
    tb_cfg->dec_params.clk_gate_decoder = DEC_X170_INTERNAL_CLOCK_GATING;
    tb_cfg->dec_params.clk_gate_decoder_idle =
        DEC_X170_INTERNAL_CLOCK_GATING_RUNTIME;
    tb_cfg->dec_params.axi_id_rd               = DEC_X170_AXI_ID_R;
    tb_cfg->dec_params.axi_id_rd_unique_enable = DEC_X170_AXI_ID_R_E;
    tb_cfg->dec_params.axi_id_wr               = DEC_X170_AXI_ID_W;
    tb_cfg->dec_params.axi_id_wr_unique_enable = DEC_X170_AXI_ID_W_E;

    /* PpParams */
    strcpy(tb_cfg->pp_params.output_picture_endian, "PP_CFG");
    strcpy(tb_cfg->pp_params.input_picture_endian, "PP_CFG");
    strcpy(tb_cfg->pp_params.word_swap, "PP_CFG");
    strcpy(tb_cfg->pp_params.word_swap16, "PP_CFG");
    tb_cfg->pp_params.bus_burst_length = PP_X170_BUS_BURST_LENGTH;

    strcpy(tb_cfg->pp_params.multi_buffer, "DISABLED");

#if (PP_X170_INTERNAL_CLOCK_GATING == 0)
    strcpy(tb_cfg->pp_params.clock_gating, "DISABLED");
#else
    strcpy(tb_cfg->pp_params.clock_gating, "ENABLED");
#endif

#if (DEC_X170_DATA_DISCARD_ENABLE == 0)
    strcpy(tb_cfg->pp_params.data_discard, "DISABLED");
#else
    strcpy(tb_cfg->pp_params.data_discard, "ENABLED");
#endif

    tb_cfg->pp_params.ppd_exists             = 1;
    tb_cfg->pp_params.dithering_support      = 1;
    tb_cfg->pp_params.scaling_support        = 1; /* Lo/Hi performance? */
    tb_cfg->pp_params.deinterlacing_support  = 1;
    tb_cfg->pp_params.alpha_blending_support = 1;
    tb_cfg->pp_params.ablend_crop_support    = 0;
    tb_cfg->pp_params.pp_out_endian_support  = 1;
    tb_cfg->pp_params.tiled_support          = 1;
    tb_cfg->pp_params.max_pp_out_pic_width   = 4096;

    tb_cfg->pp_params.fast_hor_down_scale_disable = 0;
    tb_cfg->pp_params.fast_ver_down_scale_disable = 0;
    /* tb_cfg->pp_params.ver_downscale_stripes_disable = 0; */

    /* MPEG4 deblocking filter disabled as default. */
    tb_cfg->pp_params.filter_enabled = 0;

    tb_cfg->pp_params.pix_acc_out_support                    = 1;
    tb_cfg->pp_params.vert_down_scale_stripe_disable_support = 0;
    tb_cfg->pp_params.pipeline_e                             = 1;
    tb_cfg->pp_params.pre_fetch_height                       = 16;
    tb_cfg->ppu_index                                        = -1;
    memset(tb_cfg->pp_units_params, 0, sizeof(tb_cfg->pp_units_params));

    if (tb_cfg->dec_params.hw_build == 7020 ||
        tb_cfg->dec_params.hw_version == 18001) {
        tb_cfg->dec_params.tiled_ref_support = 1;
        tb_cfg->dec_params.field_dpb_support = 1;
    }
}

/*------------------------------------------------------------------------------

   <++>.<++>  Function: tb_get_dec_rlc_mode_forced

        Functional description:
          Gets the integer values of decoder rlc mode forced.

        Inputs:

        Outputs:

------------------------------------------------------------------------------*/
uint32_t vpi_tb_get_dec_rlc_mode_forced(const struct TBCfg *tb_cfg)
{
    if (strcmp(tb_cfg->dec_params.rlc_mode_forced, "ENABLED") == 0) {
        return 1;
    } else if (strcmp(tb_cfg->dec_params.rlc_mode_forced, "DISABLED") == 0) {
        return 0;
    } else {
        return -1;
    }
}

uint32_t vpi_tb_get_dec_clock_gating(const struct TBCfg *tb_cfg)
{
    return tb_cfg->dec_params.clk_gate_decoder ? 1 : 0;
}

uint32_t vpi_tb_get_dec_data_discard(const struct TBCfg *tb_cfg)
{
    if (strcmp(tb_cfg->dec_params.data_discard, "ENABLED") == 0) {
        return 1;
    } else if (strcmp(tb_cfg->dec_params.data_discard, "DISABLED") == 0) {
        return 0;
    } else {
        return -1;
    }
}

uint32_t vpi_tb_get_dec_output_picture_endian(const struct TBCfg *tb_cfg)
{
    if (strcmp(tb_cfg->dec_params.output_picture_endian, "BIG_ENDIAN") == 0) {
        return DEC_X170_BIG_ENDIAN;
    } else if (strcmp(tb_cfg->dec_params.output_picture_endian,
                      "LITTLE_ENDIAN") == 0) {
        return DEC_X170_LITTLE_ENDIAN;
    } else {
        return -1;
    }
}

uint32_t vpi_tb_get_dec_output_format(const struct TBCfg *tb_cfg)
{
    if (strcmp(tb_cfg->dec_params.output_format, "RASTER_SCAN") == 0) {
        return DEC_X170_OUTPUT_FORMAT_RASTER_SCAN;
    } else if (strcmp(tb_cfg->dec_params.output_format, "TILED") == 0) {
        return DEC_X170_OUTPUT_FORMAT_TILED;
    } else {
        return -1;
    }
}

uint32_t vpi_tb_get_dec_service_merge_disable(const struct TBCfg *tb_cfg)
{
    return tb_cfg->dec_params.service_merge_disable;
}

uint32_t vpi_tb_get_dec_bus_width(const struct TBCfg *tb_cfg)
{
    return tb_cfg->dec_params.bus_width;
}