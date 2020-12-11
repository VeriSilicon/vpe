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

#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "stddef.h"
#include "vpi_log.h"
#include "vpi_error.h"
#include "vpi_video_enc_common.h"
#include "vpi_video_h26xenc.h"
#include "vpi_video_h26xenc_options.h"

#define DEFAULT_VALUE -255 /*the same with vpe_h26xenc.h*/
#define DEFAULT -255 /*the same with vpe_h26xenc.h*/

#define OPT_FLAG_VCE (1 << 0)
#define OPT_FLAG_CTX (1 << 1)

#define OPT_FLAG_EN (1 << 4)
#define OPT_FLAG_DIS (1 << 5)
#define OPT_FLAG_MULTI (1 << 6)

#define OFFSETM_VCE(x) offsetof(VPIH26xEncOptions, x)
VPIH26xParamsDef h26x_enc_param_table[] = {
    { "force8bit",
      'U',
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(force_8bit),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_EN,
      "force to output 8bit stream." },
    { "intra_pic_rate",
      COM_SHORT,
      TYPE_INT,
      0,
      NOCARE,
      OFFSETM_VCE(intra_pic_rate),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E|Bigsea] Intra picture rate in frames. (default [0])\nForces every Nth frame to be encoded as intra frame." },
    { "bitrate_window",
      COM_SHORT,
      TYPE_INT,
      -255,
      300,
      OFFSETM_VCE(bitrate_window),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E|Bigsea] Bitrate window length in frames. (default [intra_pic_rate])." },
    { "intra_qp_delta",
      COM_SHORT,
      TYPE_INT,
      -255,
      127,
      OFFSETM_VCE(intra_qp_delta),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] Intra QP delta. (default [26], -1..51).\n[Bigsea] Intra QP delta. (default [0], -127..127)." },
    { "qp_hdr",
      COM_SHORT,
      TYPE_INT,
      -1,
      255,
      OFFSETM_VCE(qp_hdr),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] Initial QP used for the first frame. (default [-1], -1..51).\n[Bigsea] Initial QP used for the first frame. (default [-1 or 50], -1..255)." },
    { "qp_min",
      COM_SHORT,
      TYPE_INT,
      0,
      255,
      OFFSETM_VCE(qp_min),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] Minimum frame header QP for any slices. (default [0], 0..51).\n[Bigsea] Minimum frame header QP. (default [10], 0..255)." },
    { "qp_max",
      COM_SHORT,
      TYPE_INT,
      0,
      255,
      OFFSETM_VCE(qp_max),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] Maximum frame header QP for any slices. (default [51], 0..51).\n[Bigsea] Maximum frame header QP. (default [255], 0..255)" },
    { "fixed_intra_qp",
      COM_SHORT,
      TYPE_INT,
      0,
      255,
      OFFSETM_VCE(fixed_intra_qp),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] Fixed Intra QP, 0 = disabled. (default [0], 0..51).\n[Bigsea] Fixed Intra QP, 0 = disabled. (default [0], 0..255)." },
    { "pic_skip",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(pic_skip),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI | OPT_FLAG_EN,
      "[VC8000E|Bigsea] Enable picture skip rate control." },
    { "bitdepth",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(bitdepth),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E|Bigsea] Bitdepth. 0=8-bit, 1=10-bit. [default: keep the same as input]." },
    { "tier",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(tier),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E/HEVC] encoder only (default [0], 0..1)\n0  -main tier\n1  -high tier" },
    { "byte_stream",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(byte_stream),
      { .i64 = 1 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] stream type (default [1], 0..1)\n0 - NAL units. Nal sizes returned in <nal_sizes.txt>\n1 - byte stream according to Hevc Standard Annex B." },
    { "video_range",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(video_range),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E/HEVC] Video signal sample range value in Hevc stream. (default [0], 0..1)\n0 - Y range in [16..235] Cb,Cr in [16..240]\n1 - Y,Cb,Cr range in [0..255]" },
    { "sei",
      COM_SHORT,
      TYPE_INT,
      NOCARE,
      NOCARE,
      OFFSETM_VCE(sei),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI | OPT_FLAG_EN,
      "[VC8000E] enable SEI messages." },
    { "disable_cabac",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(enable_cabac),
      { .i64 = 1 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI | OPT_FLAG_DIS,
      "[VC8000E] disable cabac, use cavlc." },
    { "slice_size",
      COM_SHORT,
      TYPE_INT,
      0,
      NOCARE,
      OFFSETM_VCE(slice_size),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] slice size in number of CTU rows. (default [0], 0..height/ctu_size)\n0 - to encode each picture in one slice\n1..height/ctu_size - to each slice with N CTU row" },
    { "tol_moving_bitrate",
      COM_SHORT,
      TYPE_INT,
      0,
      2000,
      OFFSETM_VCE(tol_moving_bitrate),
      { .i64 = 2000},
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] percent tolerance over target bitrate of moving bit rate (default [2000], 0..2000%%)." },
    { " bit_var_range_I",
      COM_SHORT,
      TYPE_INT,
      10,
      10000,
      OFFSETM_VCE(bit_var_range_I),
      { .i64 = 10000 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] percent variations over average bits per frame for I frame. (default [10000], 10..10000%%)." },
    { " bit_var_range_P",
      COM_SHORT,
      TYPE_INT,
      10,
      10000,
      OFFSETM_VCE(bit_var_range_P),
      { .i64 = 10000 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] percent variations over average bits per frame for P frame. (default [10000], 10..10000%%)." },
    { " bit_var_range_B",
      COM_SHORT,
      TYPE_INT,
      10,
      10000,
      OFFSETM_VCE(bit_var_range_B),
      { .i64 = 10000 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] percent variations over average bits per frame for B frame. (default [10000], 10..10000%%)." },
    { "pic_rc",
      COM_SHORT,
      TYPE_INT,
      -255,
      1,
      OFFSETM_VCE(pic_rc),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI | OPT_FLAG_EN,
      "[VC8000E] enable picture rate control. Calculates new target QP for every frame." },
    { "pic_rc_config",
      COM_SHORT,
      TYPE_STRING,
      NOCARE,
      NOCARE,
      OFFSETM_VCE(pic_rc_path),
      { .str = NULL },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] picture bitstream rate control cfg file path."},
    { "ctb_rc",
      COM_SHORT,
      TYPE_INT,
      0,
      3,
      OFFSETM_VCE(ctb_rc),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI | OPT_FLAG_EN,
      "[VC8000E] CTB QP adjustment mode for Rate Control and Subjective Quality. (default [0], 0..3).\n0 = No CTB QP adjustment (best PSNR).\n1 = CTB QP adjustment for Subjective Quality only.\n2 = CTB QP adjustment for Rate Control only(suggest, best bitrate).\n3 = CTB QP adjustment for both Subjective Quality and Rate Control." },
    { "tol_ctb_rc_inter",
      COM_SHORT,
      TYPE_FLOAT,
      NOCARE,
      NOCARE,
      OFFSETM_VCE(tol_ctb_rc_inter),
      { .dbl = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] Tolerance of Ctb Rate Control for INTER frames. (float point number). (default [0.0])\nCtb Rc will try to limit INTER frame bits within the range of:\n\t[targetPicSize/(1+tol_ctb_rc_inter), targetPicSize*(1+tol_ctb_rc_inter)]." },
    { "tol_ctb_rc_intra",
      COM_SHORT,
      TYPE_FLOAT,
      NOCARE,
      NOCARE,
      OFFSETM_VCE(tol_ctb_rc_intra),
      { .dbl = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] Tolerance of Ctb Rate Control for INTRA frames. (float point number). (default [-1.0])" },
    { "ctb_row_qp_step",
      COM_SHORT,
      TYPE_INT,
      NOCARE,
      NOCARE,
      OFFSETM_VCE(ctb_rc_row_qp_step),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] The maximum accumulated QP adjustment step per CTB Row allowed by Ctb Rate Control.\nDefault value is [4] for H264 and [16] for HEVC.\nQP_step_per_CTB = (ctbRowQpStep / Ctb_per_Row) and limited by maximum = 4." },
    { "pic_qp_delta_range",
      COM_SHORT,
      TYPE_COLON2,
      NOCARE,
      NOCARE,
      OFFSETM_VCE(pic_qp_delta_min),
      { .colon2 = {DEFAULT, DEFAULT} },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] Min:Max. Qp_Delta Range in Picture RC.\nMin: -1..-10 Minimum Qp_Delta in Picture RC. [-2]\nMax:  1..10  Maximum Qp_Delta in Picture RC. [3]" },
    { "hrd_conformance",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(hrd_conformance),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI | OPT_FLAG_EN,
      "[VC8000E] enable HRD conformance. Uses standard defined model to limit bitrate variance." },
    { "cpb_size",
      COM_SHORT,
      TYPE_INT,
      NOCARE,
      NOCARE,
      OFFSETM_VCE(cpb_size),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] HRD Coded Picture Buffer size in bits. (default [0] to enable enable max CPB of current level, suggest 2*bitrate)." },
    { "gop_size",
      COM_SHORT,
      TYPE_INT,
      0,
      8,
      OFFSETM_VCE(gop_size),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] GOP Size. (default [0], 0..8).\n0 for adaptive GOP size; 1~7 for fixed GOP size." },
    { "gop_lowdelay",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(gop_lowdelay),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] Enable default lowDelay GOP configuration if --gopConfig not specified, only valid for GOP size <= 4." },
    { "qp_min_I",
      COM_SHORT,
      TYPE_INT,
      0,
      51,
      OFFSETM_VCE(qp_min_I),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] minimum frame header QP overriding qp_min for I slices. (default [0], 0..51)." },
    { "qp_max_I",
      COM_SHORT,
      TYPE_INT,
      0,
      51,
      OFFSETM_VCE(qp_max_I),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] maximum frame header QP overriding qp_max for I slices. (default [51], 0..51)." },
    { "bframe_qp_delta",
      COM_SHORT,
      TYPE_INT,
      -1,
      51,
      OFFSETM_VCE(bframe_qp_delta),
      { .i64 = -1 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] BFrame QP Delta. (default [-1], -1..51)." },
    { "chroma_qp_offset",
      COM_SHORT,
      TYPE_INT,
      -12,
      12,
      OFFSETM_VCE(chroma_qp_offset),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] Chroma QP offset. (default [0], -12..12)." },
    { "vbr",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(vbr),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI | OPT_FLAG_EN,
      "[VC8000E] enable variable Bit Rate Control by qp_min." },
    { "user_data",
      COM_SHORT,
      TYPE_STRING,
      NOCARE,
      NOCARE,
      OFFSETM_VCE(user_data),
      { .str = NULL },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] SEI User data file name. File is read and inserted as SEI message before first frame." },
    { "intra_area",
      COM_SHORT,
      TYPE_INT,
      NOCARE,
      NOCARE,
      OFFSETM_VCE(intra_area_left),
      { .i64 = -1 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] left:top:right:bottom. CTB coordinates\nspecifying rectangular area of CTBs to force encoding in intra mode." },
    { "ipcm1_area",
      COM_SHORT,
      TYPE_INT,
      NOCARE,
      NOCARE,
      OFFSETM_VCE(ipcm1_area_left),
      { .i64 = -1 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] left:top:right:bottom. CTB coordinates\nspecifying rectangular area of CTBs to force encoding in IPCM mode." },
    { "ipcm2_area",
      COM_SHORT,
      TYPE_INT,
      NOCARE,
      NOCARE,
      OFFSETM_VCE(ipcm2_area_left),
      { .i64 = -1 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] left:top:right:bottom. CTB coordinates\nspecifying rectangular area of CTBs to force encoding in IPCM mode." },
    { "enable_const_chroma",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(const_chroma_en),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI | OPT_FLAG_EN,
      "[VC8000E] enable setting chroma a constant pixel value." },
    { "const_cb",
      COM_SHORT,
      TYPE_INT,
      0,
      1023,
      OFFSETM_VCE(const_cb),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] The constant pixel value for Cb.\n(for 8bit default [128], 0..255, for 10bit default [512], 0..1023)." },
    { "const_cr",
      COM_SHORT,
      TYPE_INT,
      0,
      1023,
      OFFSETM_VCE(const_cr),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E] The constant pixel value for Cr.\n(for 8bit default [128], 0..255, for 10bit default [512], 0..1023)." },
    { "rdo_level",
      COM_SHORT,
      TYPE_INT,
      1,
      3,
      OFFSETM_VCE(rdo_level),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "[VC8000E/HEVC] programable HW RDO Level (default [1], 1..3)." },
    { "disable_ssim",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(ssim),
      { .i64 = 1 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI | OPT_FLAG_DIS,
      "[VC8000E] Disable SSIM Calculation." },
    { "disable_vui_timing_info",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(vui_timing_info_enable),
      { .i64 = 1 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI | OPT_FLAG_DIS,
      "[VC8000E] Disable Write VUI timing info in SPS." },
    { "lookahead_depth",
      COM_SHORT,
      TYPE_INT,
      0,
      40,
      OFFSETM_VCE(lookahead_depth),
      { .i64 = DEFAULT },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "Number of frames to lookahead. Up to 40. [0]" },
    { "mastering_display_en",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(hdr10_display_enable),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_EN,
      "mastering display colour enable" },
    { "display_pri_x0",
      COM_SHORT,
      TYPE_INT,
      0,
      50000,
      OFFSETM_VCE(hdr10_dx0),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "green display primary x" },
    { "display_pri_y0",
      COM_SHORT,
      TYPE_INT,
      0,
      50000,
      OFFSETM_VCE(hdr10_dy0),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "green display primary y" },
    { "display_pri_x1",
      COM_SHORT,
      TYPE_INT,
      0,
      50000,
      OFFSETM_VCE(hdr10_dx1),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "blue display primary x" },
    { "display_pri_y1",
      COM_SHORT,
      TYPE_INT,
      0,
      50000,
      OFFSETM_VCE(hdr10_dy1),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "blue display primary y" },
    { "display_pri_x2",
      COM_SHORT,
      TYPE_INT,
      0,
      50000,
      OFFSETM_VCE(hdr10_dx2),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "red display primary x" },
    { "display_pri_y2",
      COM_SHORT,
      TYPE_INT,
      0,
      50000,
      OFFSETM_VCE(hdr10_dy2),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "red display primary y" },
    { "white_point_x",
      COM_SHORT,
      TYPE_INT,
      0,
      50000,
      OFFSETM_VCE(hdr10_wx),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "white point x" },
    { "white_point_y",
      COM_SHORT,
      TYPE_INT,
      0,
      50000,
      OFFSETM_VCE(hdr10_wy),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "white point y" },
    { "min_luminance",
      COM_SHORT,
      TYPE_UINT,
      0,
      4294967294,      // max_luminance - 1
      OFFSETM_VCE(hdr10_minluma),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "min display mastering luminance" },
    { "max_luminance",
      COM_SHORT,
      TYPE_UINT,
      1,
      4294967295,     // 2^32 - 1
      OFFSETM_VCE(hdr10_maxluma),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "max display mastering luminance" },
    { "light_level_en",
      COM_SHORT,
      TYPE_INT,
      0,
      1,
      OFFSETM_VCE(hdr10_lightlevel_enable),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_EN,
      "content light level enable" },
    { "max_content_light_level",
      COM_SHORT,
      TYPE_INT,
      0,
      65535,      // 2^16 - 1
      OFFSETM_VCE(hdr10_maxlight),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "max content light level" },
    { "max_pic_average_light_level",
      COM_SHORT,
      TYPE_INT,
      0,
      65535,     // 2^16 - 1
      OFFSETM_VCE(hdr10_avglight),
      { .i64 = 0 },
      OPT_FLAG_VCE | OPT_FLAG_MULTI,
      "max pic average light level" },
    { NULL },
};

static int parse_type_arg(VPIH26xParamsDef *p_param_line, void *p)
{
    if (p_param_line->type == TYPE_INT) {
        int value   = p_param_line->default_val.i64;
        *((int *)p) = value;
        VPILOGD("%s set to %d\n", p_param_line->name, *((int *)p));
    } else if (p_param_line->type == TYPE_FLOAT) {
        float value   = p_param_line->default_val.dbl;
        *((float *)p) = value;
    } else if (p_param_line->type == TYPE_STRING) {
        *((const char **)p) = p_param_line->default_val.str;
        VPILOGD("%s set to %s\n", p_param_line->name, *((char **)p));
    } else if (p_param_line->type == TYPE_COLON2) {
        *((int *)p) = p_param_line->default_val.colon2.min;
        *((int *)p+1) = p_param_line->default_val.colon2.max;
   }

    return 0;
}

VpiRet h26x_enc_get_params(VpiH26xEncCtx *vpi_h26xe_ctx)
{
    VPIH26xParamsDef *p_param_line;
    VPIH26xEncOptions *options = (VPIH26xEncOptions *)&vpi_h26xe_ctx->options;
    int i;
    u8 *p = NULL;

    for (i = 0; i < ARRAY_COUNT(h26x_enc_param_table); i++) {
        p_param_line = &h26x_enc_param_table[i];

        if (p_param_line->flag & OPT_FLAG_CTX) {
            /*p = (u8 *)ctx;*/
            VPILOGE("%s,%d, parameter's flag is OPT_FLAG_CTX! IT SHOULDN'T HAPPEN now\n",
                   __FILE__, __LINE__); /*has been move to options*/
            return VPI_ERR_ENCODER_OPITION;
        } else {
            p = (u8 *)options;
        }
        p += p_param_line->offset;
        if (parse_type_arg(p_param_line, p) < 0)
            return VPI_ERR_ENCODER_OPITION;
    }

    return 0;
}

static const VPIH26xParamsDef *find_option(const VPIH26xParamsDef *po,
                                           const char *name)
{
    int len  = strlen(name);
    int flag = 0;

    while (po->name) {
        if (!strncmp(name, po->name, len) && strlen(po->name) == len) {
            flag = 1;
            break;
        }
        po++;
    }
    return ((flag == 1) ? po : NULL);
}

VpiRet h26x_enc_get_params_from_cmd(VpiH26xEncCtx *vpi_h26xe_ctx,
                                 const char *name, char *input_value)
{
    VPIH26xEncOptions *options = (VPIH26xEncOptions *)&vpi_h26xe_ctx->options;
    const VPIH26xParamsDef *p_param_line = NULL;
    u8 *p                                = NULL;

    p_param_line = find_option(vpi_h26xe_ctx->h26x_enc_param_table, name);
    if (!p_param_line) {
        VPILOGE("Can't find option %s\n", name);
        return VPI_ERR_ENCODER_OPITION;
    }

    if (p_param_line->flag & OPT_FLAG_CTX) {
        /*p = (u8 *)ctx;*/
        VPILOGE("%s,%d, parameter's flag is OPT_FLAG_CTX! IT SHOULDN'T HAPPEN now\n",
               __FILE__, __LINE__); /*has been move to options*/
        return VPI_ERR_ENCODER_OPITION;
    } else {
        p = (u8 *)options;
    }

    p += p_param_line->offset;

    if (p_param_line->type == TYPE_INT) {
        int64_t value = atoi(input_value);
        if (p_param_line->min != NOCARE) {
            if (value < p_param_line->min) {
                if (value != 0 && value != DEFAULT_VALUE) {
                    VPILOGE("option %s value %ld less than min %d or not 0 nor DEFAULT_VALUE!\n",
                           p_param_line->name, value, p_param_line->min);
                    return VPI_ERR_ENCODER_OPITION;
                }
            }
        }
        if (p_param_line->max != NOCARE) {
            if (value > p_param_line->max) {
                VPILOGE("option %s value %ld greater than max %d!\n",
                       p_param_line->name, value, p_param_line->max);
                return VPI_ERR_ENCODER_OPITION;
            }
        }
        *((int *)p) = value;
        VPILOGD("%s set to %d\n", p_param_line->name, *((int *)p));
    } else if (p_param_line->type == TYPE_UINT) {
        uint32_t value = atoi(input_value);
        if (p_param_line->min != NOCARE) {
            if (value < p_param_line->min) {
                if (value != 0 && value != DEFAULT_VALUE) {
                    VPILOGE("option %s value %ld less than min %d or not 0 nor DEFAULT_VALUE!\n",
                           p_param_line->name, value, p_param_line->min);
                    return VPI_ERR_ENCODER_OPITION;
                }
            }
        }
        if (p_param_line->max != NOCARE) {
            if (value > p_param_line->max) {
                VPILOGE("option %s value %ld greater than max %d!\n",
                       p_param_line->name, value, p_param_line->max);
                return VPI_ERR_ENCODER_OPITION;
            }
        }
        *((int *)p) = value;
        VPILOGD("%s set to %d\n", p_param_line->name, *((int *)p));
    } else if (p_param_line->type == TYPE_FLOAT) {
        *((float *)p) = atof(input_value);
    } else if (p_param_line->type == TYPE_STRING) {
        *((const char **)p) = input_value;
        VPILOGD("%s set to %s\n", p_param_line->name, *((char **)p));
    } else if (p_param_line->type == TYPE_COLON2 || p_param_line->type == TYPE_COLON4) {
        char strs[ENC_PARAM_MAX_SEG_NUM][ENC_PARAM_MAX_SEG_LEN];
        char * pstrs[ENC_PARAM_MAX_SEG_NUM];
        int i, seg_num;
        for (i = 0; i < ENC_PARAM_MAX_SEG_NUM; i++) {
            pstrs[i] = &strs[i][0];
        }
        seg_num = vpi_enc_split_string((char **)&pstrs, ENC_PARAM_MAX_SEG_NUM, input_value, ":");
        if (seg_num < 0) {
            VPILOGE("error seg_num %d\n", seg_num);
            return VPI_ERR_ENCODER_OPITION;
        }
        if (p_param_line->type == TYPE_COLON2 && seg_num != 2) {
           VPILOGE("seg_num %d not match TYPE_COLON2\n", seg_num);
           return VPI_ERR_ENCODER_OPITION;
        }
        if (p_param_line->type == TYPE_COLON4 && seg_num != 4) {
            VPILOGE("seg_num %d not match TYPE_COLON4\n", seg_num);
            return VPI_ERR_ENCODER_OPITION;
        }
        for (i = 0; i < seg_num; i++) {
            *((int *)p) = atoi(pstrs[i]);
           VPILOGD("%s %d set to %d\n", p_param_line->name, i, *((int *)p));
           p += 4;
        }
   }

   return VPI_SUCCESS;
}
