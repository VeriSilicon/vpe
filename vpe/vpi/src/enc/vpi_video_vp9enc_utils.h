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

#ifndef VPI_VIDEO_VP9ENC_UTILS_H
#define VPI_VIDEO_VP9ENC_UTILS_H

#include "vpi_types.h"
#include "vpi_video_vp9enc.h"

#define ENC_MAX_BUFFERS 22
#define NEXT_MULTIPLE(value, n) (((value) + (n)-1) & ~((n)-1))
#define ALIGN(a) (1 << (a))
#undef VP9ENC_MIN_ENC_WIDTH
#define VP9ENC_MIN_ENC_WIDTH 66
#undef VP9ENC_MAX_ENC_WIDTH
#define VP9ENC_MAX_ENC_WIDTH 8128
#undef VP9ENC_MIN_ENC_HEIGHT
#define VP9ENC_MIN_ENC_HEIGHT 66
#undef VP9ENC_MAX_ENC_HEIGHT
#define VP9ENC_MAX_ENC_HEIGHT 8128
#undef MAX_TILE_W
#define MAX_TILE_W 8 // 512 pixels
#undef MIN_TILE_W
#define MIN_TILE_W 4 // 256 pixels
#undef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#undef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#undef DEFAULT_EFFORT_LEVEL
#define DEFAULT_EFFORT_LEVEL 0
#undef MIN_EFFORT_LEVEL
#define MIN_EFFORT_LEVEL -1
#undef MAX_EFFORT_LEVEL
#define MAX_EFFORT_LEVEL 3
#undef MIN_EFFORT_SWITCHABLE_MCOMP_FILTER
/* Minimum level where switchable mcomp filter allowed */
#define MIN_EFFORT_SWITCHABLE_MCOMP_FILTER 2
#define VP9ERR_OUTPUT stdout

/* Value for parameter to use API default */
#define DEFAULT -255
#define DEFAULT_VALUE -255

/* Intermediate Video File Format */
#define IVF_HDR_BYTES 32
#define IVF_FRM_BYTES 12
#define MAX_WAIT_DEPTH 44

/* The maximum amount of frames for bitrate moving average calculation */
#define MOVING_AVERAGE_FRAMES 30

#define AV_NOPTS_VALUE ((int64_t)UINT64_C(0x8000000000000000))

enum {
    DEC_OUT_RFC,
    DEC_OUT_PP0,
    DEC_OUT_PP1,
    DEC_OUT_PP2,
    DEC_OUT_PP3,
};

typedef struct ENC_IN_ADDR_s {
    size_t bus_luma;
    size_t bus_chroma;
    size_t bus_luma_table;
    size_t bus_chroma_table;
} EncInAddr;

int vp9enc_default_parameters(VpiEncVp9Setting *cfg);
int vp9enc_default_parameters(VpiEncVp9Setting *cfg);
int vp9enc_updatesetting_fromframe(VpiEncVp9Ctx *ctx, VpiFrame *in,
                                   VpiEncVp9Setting *out);
int vp9enc_setpreset(VpiEncVp9Setting *cfg);
int vp9enc_updatesetting_fromframe(VpiEncVp9Ctx *ctx, VpiFrame *in,
                                   VpiEncVp9Setting *out);
void vp9enc_get_max_frame_delay(VpiEncVp9Ctx *ctx, VpiFrame *v_frame,
                                VpiEncVp9Setting *v_setting);
void vp9enc_print_setting(VpiEncVp9Setting *cfg);
int vp9enc_open(VpiEncVp9Ctx *ctx, VpiEncVp9Setting *cfg);
void vp9enc_print_error_value(const char *errorDesc, int retVal);
void vp9enc_test_segmentation(VpiEncVp9Ctx *ctx, int pic_width, int pic_height,
                              VP9EncInst encoder, VpiEncVp9Setting *cfg);
void vp9enc_print_frame(VpiEncVp9Ctx *ctx, VP9EncInst encoder, u32 frame_number,
                        VP9EncRet ret, int width, int height, int show_frame);
void vp9enc_ma_add_frame(vp9_ma_s *ma, int frameSizeBits);
int vp9enc_next_pic(int input_rate_numer, int input_rate_denom,
                    int output_rate_numer, int output_rate_denom,
                    int frame_count, int first_picture);
void vp9enc_print_total(VpiEncVp9Ctx *ctx);
void vp9enc_statistic(VpiEncVp9Ctx *ctx);
void vp9enc_hw_performance(VpiEncVp9Ctx *ctx);
void vp9enc_free_resource(VpiEncVp9Ctx *ctx);
int vp9enc_set_ppindex(VpiEncVp9Ctx *ctx, VpiFrame *frame, VpiEncVp9Opition *cfg);
int vp9enc_send_buffer_to_encoder(VP9EncIn *enc_instance,
                                  struct DecPicture *enc_data,
                                  VpiEncVp9Setting *cfg);

#endif
