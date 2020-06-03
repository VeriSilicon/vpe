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

#ifndef __VPI_VIDEO_DEC_TB_DEFS_H__
#define __VPI_VIDEO_DEC_TB_DEFS_H__

#include <stdint.h>

/*------------------------------------------------------------------------------
    Generic data type stuff
------------------------------------------------------------------------------*/

typedef enum { TB_FALSE = 0, TB_TRUE = 1 } TBBool;

/*------------------------------------------------------------------------------
    Test bench configuration
------------------------------------------------------------------------------*/
struct TBParams {
    char packet_by_packet[9];
    char nal_unit_stream[9];
    uint32_t seed_rnd;
    char stream_bit_swap[24];
    char stream_bit_loss[24];
    char stream_packet_loss[24];
    char stream_header_corrupt[9];
    char stream_truncate[9];
    char slice_ud_in_packet[9];
    uint32_t first_trace_frame;

    uint32_t extra_cu_ctrl_eof;
    uint32_t memory_page_size;
    int32_t ref_frm_buffer_size;

    uint32_t unified_reg_fmt;
};

struct TBDecParams {
    char output_picture_endian[14];
    uint32_t bus_burst_length;
    uint32_t asic_service_priority;
    char output_format[12];
    uint32_t latency_compensation;
    char clock_gating[9];
    uint32_t clk_gate_decoder;
    uint32_t clk_gate_decoder_idle;
    char data_discard[9];

    char memory_allocation[9];
    char rlc_mode_forced[9];
    char error_concealment[15];

    uint32_t jpeg_mcus_slice;
    uint32_t jpeg_input_buffer_size;

    uint32_t refbu_enable;
    uint32_t refbu_disable_interlaced;
    uint32_t refbu_disable_double;
    uint32_t refbu_disable_eval_mode;
    uint32_t refbu_disable_checkpoint;
    uint32_t refbu_disable_offset;
    uint32_t refbu_data_excess_max_pct;
    uint32_t refbu_disable_top_bot_sum;

    uint32_t mpeg2_support;
    uint32_t vc1_support;
    uint32_t jpeg_support;
    uint32_t mpeg4_support;
    uint32_t custom_mpeg4_support;
    uint32_t h264_support;
    uint32_t vp6_support;
    uint32_t vp7_support;
    uint32_t vp8_support;
    uint32_t prog_jpeg_support;
    uint32_t sorenson_support;
    uint32_t avs_support;
    uint32_t rv_support;
    uint32_t mvc_support;
    uint32_t webp_support;
    uint32_t ec_support;
    uint32_t max_dec_pic_width;
    uint32_t max_dec_pic_height;
    uint32_t hw_version;
    uint32_t hw_build;
    uint32_t hw_build_id;
    uint32_t bus_width;
    uint32_t bus_width64bit_enable;
    uint32_t latency;
    uint32_t non_seq_clk;
    uint32_t seq_clk;
    uint32_t support_non_compliant;
    uint32_t jpeg_esupport;
    uint32_t hevc_main10_support;
    uint32_t vp9_profile2_support;
    uint32_t rfc_support;
    uint32_t ds_support;
    uint32_t ring_buffer_support;
    uint32_t mrb_prefetch;
    uint32_t format_p010_support;
    uint32_t format_customer1_support;

    uint32_t force_mpeg4_idct;
    uint32_t ch8_pix_ileav_output;

    uint32_t ref_buffer_test_mode_offset_enable;
    int32_t ref_buffer_test_mode_offset_min;
    int32_t ref_buffer_test_mode_offset_max;
    int32_t ref_buffer_test_mode_offset_start;
    int32_t ref_buffer_test_mode_offset_incr;

    uint32_t apf_disable;
    uint32_t apf_threshold_disable;
    int32_t apf_threshold_value;

    uint32_t tiled_ref_support;
    uint32_t stride_support;
    int32_t field_dpb_support;
    int32_t addr64_support;

    uint32_t service_merge_disable;

    uint32_t strm_swap;
    uint32_t pic_swap;
    uint32_t dirmv_swap;
    uint32_t tab0_swap;
    uint32_t tab1_swap;
    uint32_t tab2_swap;
    uint32_t tab3_swap;
    uint32_t rscan_swap;
    uint32_t comp_tab_swap;
    uint32_t max_burst;
    uint32_t ref_double_buffer_enable;

    uint32_t timeout_cycles;

    uint32_t axi_id_rd;
    uint32_t axi_id_rd_unique_enable;
    uint32_t axi_id_wr;
    uint32_t axi_id_wr_unique_enable;
    uint32_t cache_support;
};

struct TBPpParams {
    char output_picture_endian[14];
    char input_picture_endian[14];
    char word_swap[9];
    char word_swap16[9];
    uint32_t bus_burst_length;
    char clock_gating[9];
    char data_discard[9];
    char multi_buffer[9];

    uint32_t max_pp_out_pic_width;
    uint32_t ppd_exists;
    uint32_t dithering_support;
    uint32_t scaling_support;
    uint32_t deinterlacing_support;
    uint32_t alpha_blending_support;
    uint32_t ablend_crop_support;
    uint32_t pp_out_endian_support;
    uint32_t tiled_support;
    uint32_t tiled_ref_support;

    int32_t fast_hor_down_scale_disable;
    int32_t fast_ver_down_scale_disable;
    int32_t vert_down_scale_stripe_disable_support;
    uint32_t pix_acc_out_support;

    uint32_t filter_enabled; /* Enable deblocking filter for MPEG4. */
    uint32_t pipeline_e; /* PP pipeline mode enabled */
    uint32_t tiled_e; /* Tiled output enabled */
    uint32_t in_width; /* PP standalone input width/height in pixel */
    uint32_t in_height;
    uint32_t in_stride; /* PP standalone input stride in bytes */
    uint32_t
        pre_fetch_height; /* PP standalone prefetch reading height: 16 or 64? */
};

/* PP units params */
struct TBPpUnitParams {
    uint32_t unit_enabled;
    uint32_t tiled_e; /* Tiled output enabled */
    uint32_t monochrome; /* PP output luma only */
    uint32_t cr_first; /* CrCb instead of CbCr */
    uint32_t out_p010; /* P010 output */
    uint32_t out_cut_8bits; /* Cut to 8-bit output */
    /* crop input */
    uint32_t crop_x;
    uint32_t crop_y;
    uint32_t crop_width;
    uint32_t crop_height;
    /* scale out */
    uint32_t scale_width;
    uint32_t scale_height;
    uint32_t shaper_enabled;
    uint32_t planar;
    uint32_t ystride; /* Stride for Y/C plane */
    uint32_t cstride;
    uint32_t align; /* alignment for this pp output channel */
};

struct TBCfg {
    struct TBParams tb_params;
    struct TBDecParams dec_params;
    struct TBPpParams pp_params;
    struct TBPpUnitParams pp_units_params[4];
    uint32_t ppu_index;
    uint32_t cache_bypass;
    uint32_t shaper_bypass;
    uint32_t cache_enable;
    uint32_t shaper_enable;
};

void vpi_tb_set_default_cfg(struct TBCfg *tb_cfg);
uint32_t vpi_tb_get_dec_rlc_mode_forced(const struct TBCfg *tb_cfg);
uint32_t vpi_tb_get_dec_clock_gating(const struct TBCfg *tb_cfg);
uint32_t vpi_tb_get_dec_data_discard(const struct TBCfg *tb_cfg);
uint32_t vpi_tb_get_dec_output_picture_endian(const struct TBCfg *tb_cfg);
uint32_t vpi_tb_get_dec_output_format(const struct TBCfg *tb_cfg);
uint32_t vpi_tb_get_dec_service_merge_disable(const struct TBCfg *tb_cfg);
uint32_t vpi_tb_get_dec_bus_width(const struct TBCfg *tb_cfg);

#endif /* __VPI_VIDEO_DEC_TB_DEFS_H__ */
