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
#include <assert.h>

#include "cwl.h"
#include "dectypes.h"
#include "fb_performance.h"
#include "vp9encapi.h"

#include "vpi.h"
#include "vpi_types.h"
#include "vpi_log.h"
#include "vpi_video_enc_common.h"
#include "vpi_video_vp9enc.h"
#include "vpi_video_vp9enc_utils.h"

#define OFFSET(x) offsetof(VpiEncVp9Setting, x)

VpiEncSetting vp9enc_options[] = {
    { "intra_pic_rate", OFFSET(intra_pic_rate), 0, 0xFFFFF, VPI_ENC_PARA_INT },
    { "bitrate_window", OFFSET(bitrate_window), DEFAULT, 300, VPI_ENC_PARA_INT },
    { "qp_hdr", OFFSET(qp_hdr), DEFAULT, 255, VPI_ENC_PARA_INT },
    { "qp_min", OFFSET(qp_min), DEFAULT, 255, VPI_ENC_PARA_INT },
    { "qp_max", OFFSET(qp_max), DEFAULT, 255, VPI_ENC_PARA_INT },
    { "fixed_intra_qp", OFFSET(fixed_intra_qp), DEFAULT, 255, VPI_ENC_PARA_INT },
    { "pic_rc", OFFSET(pic_rc), DEFAULT, 1, VPI_ENC_PARA_INT },
    { "mcomp_filter_type", OFFSET(mcomp_filter_type), DEFAULT, 4, VPI_ENC_PARA_INT },
    { "force8bit", OFFSET(force_8bit), 0, 1, VPI_ENC_PARA_INT },
    { "effort", OFFSET(effort), DEFAULT, 5, VPI_ENC_PARA_INT },
    { "ref_frame_scheme", OFFSET(ref_frame_scheme), DEFAULT, 5, VPI_ENC_PARA_INT },
    { "filte_level", OFFSET(filter_level), DEFAULT, 64, VPI_ENC_PARA_INT },
    { "filter_sharpness", OFFSET(filter_sharpness), DEFAULT, 8, VPI_ENC_PARA_INT },
    { "lag_in_frames", OFFSET(lag_in_frames), DEFAULT, 25, VPI_ENC_PARA_INT },
    { "passes", OFFSET(passes), DEFAULT, 2, VPI_ENC_PARA_INT },
};

static int vpi_venc_vp9_convert_setting(VpiEncVp9Opition *in,
                                        VpiEncVp9Setting *out)
{
    if (in == NULL || out == NULL) {
        VPILOGE("vpi_venc_vp9_convert_setting parameters error\n");
        return -1;
    }

    if ((in->bit_rate > 10000) && (in->bit_rate < 60000000)) {
        out->bit_per_second = in->bit_rate;
    } else {
        VPILOGW("invalid bit_rate  = %d.\n", in->bit_rate);
    }

    if ((in->frame_rate_numer > 0) && (in->frame_rate_numer < 1048576)) {
        out->input_rate_number = in->frame_rate_numer;
    } else {
        VPILOGW("invalid frame_rate_numer  = %d.\n", in->frame_rate_numer);
    }

    if ((in->frame_rate_denom > 0) && (in->frame_rate_denom < 1048576)) {
        out->input_rate_denom = in->frame_rate_denom;
    } else {
        VPILOGW("invalid frame_rate_denom  = %d.\n", in->frame_rate_denom);
    }

    out->preset        = in->preset;
    out->effort        = in->effort;
    out->lag_in_frames = in->lag_in_frames;
    out->passes        = in->passes;
    out->force_8bit    = in->force_8bit;
    out->mem_id        = in->task_id;
    out->device        = in->dev_name;
    out->priority      = in->priority;

    return 0;
}

int vpi_encode_vp9_enc_process(VpiEncVp9Ctx *ctx)
{
    VpiEncVp9Setting *cfg       = &ctx->vp9_enc_cfg;
    VP9EncIn *enc_in            = &ctx->enc_in;
    VP9EncInst encoder          = ctx->encoder;
    int pic_width               = cfg->lum_width_src;
    int pic_height              = cfg->lum_height_src;
    int i                       = 0;
    int ret                     = 0;
    int lag_status              = 0;
    int input_limit = cfg->lag_in_frames || cfg->ref_frame_scheme == 4 ?
                          cfg->lag_in_frames + 2 : 2;
    u32 usedMinQp = 255, usedMaxQp = 0;
    const int first_pass_only = (cfg->passes == 2 && cfg->pass == 1) ? 1 : 0;
    int idx;

    pthread_mutex_lock(&ctx->enc_thread_mutex);
    if (input_limit < 4)  input_limit = 4;

    ctx->pic_tobe_free  = 0;
    enc_in->write_stats = 1;

    /* Main encoding loop */
    VPILOGD("vpi_venc_vp9_encode end_of_sequence = %d,input_limit = %d,"
            "input_real_count=%d,loop_condition=%d, pp_index=%d\n",
            enc_in->end_of_sequence, input_limit, ctx->input_real_count,
            ctx->loop_condition, ctx->pp_index);

    if (enc_in->end_of_sequence != 1) {
        if (ctx->input_real_count < input_limit) {
            ctx->loop_condition = true;
        } else {
            ctx->loop_condition = false;
        }
    }

    ctx->next = vp9enc_next_pic(cfg->input_rate_number, cfg->input_rate_denom,
                                cfg->output_rate_number, cfg->output_rate_denom,
                                ctx->frame_count, cfg->first_picture);

    if (ctx->loop_condition) {
        ret = vp9enc_send_buffer_to_encoder(ctx, enc_in, cfg);
        if (ret == 0) {
            ctx->frame_count++;
            ctx->input_real_count++;
        } else {
            if (ctx->eos_received == 1) {
                ctx->loop_condition = 0;
                enc_in->end_of_sequence = 1;
            } else {
                VPILOGD("Input is NULL, wait new frame comein\n");
                pthread_mutex_unlock(&ctx->enc_thread_mutex);
                return 0;
            }
        }
    } else {
        enc_in->busLuma        = 0;
        enc_in->busChromaU     = 0;
        enc_in->busLumaTable   = 0;
        enc_in->busChromaTable = 0;
    }

    /* Normal encoding....*/
    ctx->frm_cnt_total++;

    if (cfg->pp_dump) {
        size_t luma_area =
            ((cfg->width + 63) & (~63)) * ((cfg->height + 7) & (~7));
        enc_in->scaledPicture.busLuma = ctx->scaled_pic_mem.busAddress;

        luma_area                         = (luma_area / 32) * 40;
        enc_in->scaledPicture.packed_8bpp = cfg->pp_dump == 2 ? 1 : 0;
        enc_in->scaledPicture.busChroma =
            ctx->scaled_pic_mem.busAddress + luma_area;
    }

    if (ctx->eos_received == 1) {
        ctx->loop_break |=
            !ctx->loop_condition &&
            ctx->frame_count_out == (ctx->input_frm_total + ctx->frame_count);
    }

    VPILOGD("VP9DBG loop_condition=%d, frame_count_out=%d,input_frm_total=%d, frame_count=%d\n",
            ctx->loop_condition, ctx->frame_count_out,
            ctx->input_frm_total, ctx->frame_count);

    if (ctx->loop_break) {
        VPILOGE("VP9DBG loop_break=%d\n", ctx->loop_break);
        ctx->encode_end = 1;
        ctx->enc_thread_finish = 1;
        if (ctx->waiting_for_pkt == 1) {
            pthread_cond_signal(&ctx->enc_thread_cond);
            ctx->waiting_for_pkt = 0;
        }

        pthread_mutex_unlock(&ctx->enc_thread_mutex);
        return 0;
    }

    /* Set rate control*/
    for (i = 0; i < MAX_BPS_ADJUST; i++) {
        if (cfg->bps_adjust_frame[i] &&
            (ctx->code_frame_count == cfg->bps_adjust_frame[i])) {
            ctx->rc.bitPerSecond = cfg->bps_adjust_bitrate[i];
            if ((ret = VP9EncSetRateCtrl(encoder, &ctx->rc)) != VP9ENC_OK)
                vp9enc_print_error_value("VP9EncSetRateCtrl() failed.", ret);
        }
    }

    enc_in->inputdepth = cfg->input_bitdepth;
    enc_in->codingType = VP9ENC_PREDICTED_FRAME;
    /* Force odd frames to be coded as dropable. */
    if (cfg->dropable && (ctx->frame_count & 1)) {
        enc_in->codingType          = VP9ENC_PREDICTED_FRAME;
        enc_in->refresh_frame_flags = 0;
    }
    ctx->frm_cnt_total++;

    if (!cfg->pp_only) {
        vp9enc_test_segmentation(ctx, cfg->width, cfg->height, encoder, cfg);
    }

    if (cfg->passes > 1 && cfg->lag_in_frames == 1 && ctx->frame_count > 2) {
        if (ctx->enc_out.stats.inter_count <
            15 * ctx->enc_out.stats.num_mbs / 100) {
            enc_in->codingType = VP9ENC_INTRA_FRAME;
        }
    }

    idx = vp9enc_get_empty_stream_buffer(ctx);
    if (idx == -1) {
        VPILOGE("Can't find empty stream buffer, return\n");
        pthread_mutex_unlock(&ctx->enc_thread_mutex);
        return 0;
    }

    enc_in->pOutBuf   = ctx->outstream_mem[idx];
    enc_in->busOutBuf = ctx->outbuff_mem.busAddress;
    VP9OutSet(encoder, ctx->outbuff_mem.busAddress,
              (size_t)enc_in->pOutBuf, ctx->outstream_mem_size);

    enc_in->indexTobeEncode = ctx->next;
    enc_in->encindex        = ctx->enc_index;

    VPILOGD("ctx->next = %d\n", ctx->next);

    /*Start encode now...*/
    ret = VP9EncStrmEncode(encoder, enc_in, &ctx->enc_out);
    VP9EncGetRateCtrl(encoder, &ctx->rc);
    switch (ret) {
    case VP9ENC_LAG_FIRST_PASS:
    case VP9ENC_LAG:
        VPILOGD("encoder status=VP9ENC_LAG\n");
        vp9enc_print_frame(ctx, encoder, ctx->next, ret, pic_width, pic_height,
                           0);
        if (first_pass_only) {
            ctx->frame_count_out++;
        }
        enc_in->pOutBuf = NULL;
        lag_status      = 1;
        break;

    case VP9ENC_FRAME_READY:
        /* Calculate stream size and bitrate moving average */
        VPILOGD("VP9ENC_FRAME_READY:ctx->enc_out.codingType=%d;"
                "ctx->enc_out.frameSize = %d;ctx->enc_out.show_frame=%d\n",
                ctx->enc_out.codingType, ctx->enc_out.frameSize,
                ctx->enc_out.show_frame);
        if (ctx->enc_out.frameSize) {
            ctx->stream_size += IVF_FRM_BYTES + ctx->enc_out.frameSize;
        }
        ctx->total_bits += ctx->enc_out.frameSize * 8;
        vp9enc_ma_add_frame(&ctx->ma, ctx->enc_out.frameSize * 8);
        vp9enc_print_frame(ctx, encoder, ctx->next, ret, pic_width, pic_height,
                           ctx->enc_out.show_frame);
        if (ctx->enc_out.frameSize == 0) {
            enc_in->pOutBuf = NULL;
        } else {
            ctx->stream_buf_list[idx]->used      = 1;
            ctx->stream_buf_list[idx]->item      = ctx->outstream_mem[idx];
            ctx->stream_buf_list[idx]->item_size = ctx->enc_out.frameSize;
            ctx->stream_buf_list[idx]->show      = ctx->enc_out.show_frame;
            ctx->stream_buf_list[idx]->pts       = ctx->enc_out.pts;
            ctx->stream_buf_list[idx]->pkt_dts   = ctx->enc_out.pts;
            vp9enc_buf_list_add(&ctx->stream_buf_head, ctx->stream_buf_list[idx]);
            if (ctx->waiting_for_pkt == 1 && ctx->enc_out.show_frame == 1) {
                pthread_cond_signal(&ctx->enc_thread_cond);
                ctx->waiting_for_pkt = 0;
            }
        }

        usedMinQp = MIN(usedMinQp, ctx->rc.currentActiveQp);
        usedMaxQp = MAX(usedMaxQp, ctx->rc.currentActiveQp);

        /* No show frames are just copies of some existing frame,
         * so don't consume
         */
        if (ctx->enc_out.show_frame) {
            ctx->frame_count_out++;
        } else {
            ctx->frm_cnt_total++;
        }
        lag_status = 0;
        ctx->code_frame_count++;
        break;

    case VP9ENC_OUTPUT_BUFFER_OVERFLOW:
        VPILOGD("encoder status=VP9ENC_OUTPUT_BUFFER_OVERFLOW\n");
        vp9enc_print_frame(ctx, encoder, ctx->next, ret, pic_width, pic_height,
                           0);
        if (ctx->enc_out.show_frame) {
            ctx->frame_count_out++;
        }
        break;

    case VP9ENC_FIRSTPASS_FAILED:
        VPILOGE("VP9ENC_FIRSTPASS_FAILED\n");
        goto error;
        break;

    default:
        VPILOGD("encoder status=Unkown\n");
        vp9enc_print_error_value("VP9EncStrmEncode() failed.", ret);
        vp9enc_print_frame(ctx, encoder, ctx->next, ret, pic_width, pic_height,
                           0);
        goto exit;
    }

    enc_in->timeIncrement = cfg->output_rate_denom;
    ctx->pic_tobe_free = 0;
    if (cfg->lag_in_frames == 0) {
        ctx->pic_tobe_free = ctx->next + 1;
        vp9enc_consume_pic(ctx, ctx->pic_tobe_free);
        ctx->input_real_count--;
    } else if (lag_status != 1) {
        if (ctx->enc_out.show_frame == 1) {
            ctx->input_real_count--;
            ctx->pic_tobe_free = ctx->enc_out.indexEncoded + 1;
            vp9enc_consume_pic(ctx, ctx->pic_tobe_free);
        }
    }

exit:

#ifdef FPGA
    vp9enc_hw_performance(ctx);
#endif

    VPILOGD("lag_in_frames=%d,lag_status =%d, show_frame=%d,pic_tobe_free=%d\n",
            cfg->lag_in_frames, lag_status, ctx->enc_out.show_frame,
            ctx->pic_tobe_free);

    /* Print information about encoded frames */
    VPILOGD("encode finished, bitrate target %d bps, actual %d bps (%lu%%),"
            "Used Qp min: %d max: %d\n",
            ctx->rc.bitPerSecond, ctx->bitrate,
            (ctx->rc.bitPerSecond) ?
                (u64)ctx->bitrate * 100 / ctx->rc.bitPerSecond :
                0,
            usedMinQp, usedMaxQp);
    VPILOGD("Total of %lu frames encoded, %lu bytes.\n", ctx->frame_count_out,
            ctx->stream_size);
    pthread_mutex_unlock(&ctx->enc_thread_mutex);
    return 0;

error:
    ctx->pic_tobe_free = 0;
    pthread_mutex_unlock(&ctx->enc_thread_mutex);
    return -1;
}

void *vpi_venc_vp9_process(void *param)
{
    VpiEncVp9Ctx *ctx = (VpiEncVp9Ctx *)param;
    int ret = 0;

    while (!ctx->enc_thread_finish) {
        ret = vpi_encode_vp9_enc_process(ctx);
        if (ret == 0) {
            usleep(500);
        } else if (ret == -1) {
            break;
        }
    }

    return NULL;
}

int vpi_venc_vp9_init(VpiEncVp9Ctx *ctx, void *cfg)
{
    VpiEncVp9Opition *vpi_setting = (VpiEncVp9Opition *)cfg;
    VpiEncVp9Setting *cml         = &ctx->vp9_enc_cfg;
    VpiFrame *frame               = (VpiFrame *)vpi_setting->framectx;
    VpiEncParamSet *para_set = vpi_setting->param_list;
    int num = sizeof(vp9enc_options) / sizeof(VpiEncSetting);
    int ret = 0, i = 0;

    if (ctx == NULL || cfg == NULL) {
        VPILOGE("vpi_venc_vp9_init parameters error\n");
        return -1;
    }
    ctx->firstframe = 0;
    ctx->api_ver    = VP9EncGetApiVersion();

    VPILOGD("VP9 Encoder API version %d.%d\n", ctx->api_ver.major,
            ctx->api_ver.minor);

    ret = vp9enc_set_ppindex(ctx, frame, vpi_setting);
    if (ret != 0) {
        VPILOGE("vp9enc_set_ppindex error\n");
        goto error;
    }

    /* Set default parameters*/
    ret = vp9enc_default_parameters(cml);
    if (ret != 0) {
        VPILOGE("vp9enc_default_parameters error\n");
        goto error;
    }

    /* Update public ffmpeg parameters*/
    ret = vpi_venc_vp9_convert_setting(vpi_setting, cml);
    if (ret != 0) {
        VPILOGE("vpi_venc_vp9_convert_setting error\n");
        goto error;
    }

    /* Update internal setting with "--enc_params" */
    while (para_set != NULL) {
        ret = vpi_enc_set_param(para_set->key, para_set->value, vp9enc_options,
                                num, cml);
        if (ret != 0)
            return ret;
        para_set = para_set->next;
    }

    ret = vp9enc_setpreset(cml);
    if (ret != 0) {
        VPILOGE("vp9enc_setpreset error.\n");
        goto error;
    }

    ret = vp9enc_updatesetting_fromframe(ctx, frame, cml);
    if (ret != 0) {
        VPILOGE("vp9enc_updatesetting_fromframe error.\n");
        goto error;
    }

    vp9enc_get_max_frame_delay(ctx, frame, cml);
    vp9enc_print_setting(cml);
    ret = vp9enc_open(ctx, cml);
    if (ret != 0) {
        VPILOGE("vp9enc_open error = %d\n", ret);
        goto error;
    }

    pthread_mutex_init(&ctx->enc_thread_mutex, NULL);
    pthread_cond_init(&ctx->enc_thread_cond, NULL);
    ctx->enc_thread_finish = 0;
    ret = pthread_create(&ctx->enc_thread_handle, NULL, vpi_venc_vp9_process, ctx);
    if (ret) {
        VPILOGE("Unable to create vp9 enc thread\n");
        goto error;
    }

    ctx->initialized = 1;
    return ret;

error:
    vpi_venc_vp9_close(ctx);
    return -1;
}

int vpi_venc_vp9_encode(VpiEncVp9Ctx *ctx, void *in, void *out)
{
    VpiFrame *input             = (VpiFrame *)in;
    VpiPacket *output           = out;
    VpiEncVp9Setting *cfg       = &ctx->vp9_enc_cfg;
    VP9EncIn *enc_in            = &ctx->enc_in;
    VP9EncInst encoder          = ctx->encoder;
    int pic_width               = cfg->lum_width_src;
    int pic_height              = cfg->lum_height_src;
    int i                       = 0;
    int ret                     = 0;
    int lag_status              = 0;
    int input_limit = cfg->lag_in_frames || cfg->ref_frame_scheme == 4 ?
                          cfg->lag_in_frames + 2 : 2;
    u32 usedMinQp = 255, usedMaxQp = 0;
    const int first_pass_only = (cfg->passes == 2 && cfg->pass == 1) ? 1 : 0;

    if (ctx == NULL || in == NULL || out == NULL) {
        VPILOGE("vpi_vp9enc_encode input error!\n");
    }

    if (input_limit < 4)  input_limit = 4;

    ctx->pic_tobe_free = 0;
    output->size       = 0;
    output->pkt_dts    = 0;
    output->pkt_dts    = 0;

    enc_in->write_stats = 1;

    /* Main encoding loop */
    VPILOGD("vpi_venc_vp9_encode end_of_sequence = %d,input_limit = %d,"
            "input_real_count=%d,loop_condition=%d, pp_index=%d\n",
            enc_in->end_of_sequence, input_limit, ctx->input_real_count,
            ctx->loop_condition, ctx->pp_index);

    if (enc_in->end_of_sequence != 1) {
        if (ctx->input_real_count < input_limit) {
            ctx->loop_condition = true;
        } else {
            ctx->loop_condition = false;
        }
    }

    if (ctx->loop_condition) {
        ret = vp9enc_send_buffer_to_encoder(ctx, enc_in, cfg);
        enc_in->pts = input->pts;
        enc_in->dts = input->pkt_dts;
        if (ret == 0) {
            ctx->frame_count++;
            ctx->input_real_count++;
        } else {
            VPILOGE("Input is NULL, break encode now\n");
            ctx->loop_condition = 0;
        }
    } else {
        enc_in->busLuma        = 0;
        enc_in->busChromaU     = 0;
        enc_in->busLumaTable   = 0;
        enc_in->busChromaTable = 0;
    }

    /* Normal encoding....*/
    ctx->frm_cnt_total++;

    if (cfg->pp_dump) {
        size_t luma_area =
            ((cfg->width + 63) & (~63)) * ((cfg->height + 7) & (~7));
        enc_in->scaledPicture.busLuma = ctx->scaled_pic_mem.busAddress;

        luma_area                         = (luma_area / 32) * 40;
        enc_in->scaledPicture.packed_8bpp = cfg->pp_dump == 2 ? 1 : 0;
        enc_in->scaledPicture.busChroma =
            ctx->scaled_pic_mem.busAddress + luma_area;
    }

    ctx->loop_break |=
        !ctx->loop_condition &&
        ctx->frame_count_out == (ctx->input_frm_total + ctx->frame_count);

    VPILOGD("VP9DBG loop_condition=%d, frame_count_out=%d,input_frm_total=%d, frame_count=%d\n",
            ctx->loop_condition, ctx->frame_count_out,
            ctx->input_frm_total, ctx->frame_count);

    if (ctx->loop_break) {
        VPILOGE("VP9DBG loop_break=%d\n", ctx->loop_break);
        return 0;
    }

    /* Set rate control*/
    for (i = 0; i < MAX_BPS_ADJUST; i++) {
        if (cfg->bps_adjust_frame[i] &&
            (ctx->code_frame_count == cfg->bps_adjust_frame[i])) {
            ctx->rc.bitPerSecond = cfg->bps_adjust_bitrate[i];
            if ((ret = VP9EncSetRateCtrl(encoder, &ctx->rc)) != VP9ENC_OK)
                vp9enc_print_error_value("VP9EncSetRateCtrl() failed.", ret);
        }
    }

    enc_in->inputdepth = cfg->input_bitdepth;
    enc_in->codingType = VP9ENC_PREDICTED_FRAME;
    /* Force odd frames to be coded as dropable. */
    if (cfg->dropable && (ctx->frame_count & 1)) {
        enc_in->codingType          = VP9ENC_PREDICTED_FRAME;
        enc_in->refresh_frame_flags = 0;
    }
    ctx->frm_cnt_total++;

    if (!cfg->pp_only) {
        vp9enc_test_segmentation(ctx, cfg->width, cfg->height, encoder, cfg);
    }

    if (cfg->passes > 1 && cfg->lag_in_frames == 1 && ctx->frame_count > 2) {
        if (ctx->enc_out.stats.inter_count <
            15 * ctx->enc_out.stats.num_mbs / 100) {
            enc_in->codingType = VP9ENC_INTRA_FRAME;
        }
    }

    enc_in->pOutBuf = (u32 *)output->data;
    VP9OutSet(encoder, ctx->outbuff_mem.busAddress, (size_t)enc_in->pOutBuf,
              ctx->outbuff_mem.size);
    enc_in->indexTobeEncode = ctx->next;
    enc_in->encindex        = ctx->enc_index;

    VPILOGD("ctx->next = %d\n", ctx->next);

    /*Start encode now...*/
    ret = VP9EncStrmEncode(encoder, enc_in, &ctx->enc_out);
    VP9EncGetRateCtrl(encoder, &ctx->rc);
    switch (ret) {
    case VP9ENC_LAG_FIRST_PASS:
    case VP9ENC_LAG:
        VPILOGD("encoder status=VP9ENC_LAG\n");
        vp9enc_print_frame(ctx, encoder, ctx->next, ret, pic_width, pic_height,
                           0);
        if (first_pass_only) {
            ctx->frame_count_out++;
        }
        enc_in->pOutBuf = NULL;
        lag_status      = 1;
        break;

    case VP9ENC_FRAME_READY:
        VPILOGD("encoder status=VP9ENC_FRAME_READY\n");
        /* Calculate stream size and bitrate moving average */
        VPILOGD("VP9ENC_FRAME_READY:ctx->enc_out.codingType=%d;"
                "ctx->enc_out.frameSize = %d;ctx->enc_out.show_frame=%d\n",
                ctx->enc_out.codingType, ctx->enc_out.frameSize,
                ctx->enc_out.show_frame);
        if (ctx->enc_out.frameSize) {
            ctx->stream_size += IVF_FRM_BYTES + ctx->enc_out.frameSize;
        }
        ctx->total_bits += ctx->enc_out.frameSize * 8;
        vp9enc_ma_add_frame(&ctx->ma, ctx->enc_out.frameSize * 8);
        vp9enc_print_frame(ctx, encoder, ctx->next, ret, pic_width, pic_height,
                           ctx->enc_out.show_frame);
        if (ctx->enc_out.frameSize == 0) {
            output->size = 0;
        } else {
            if (cfg->lag_in_frames > 0) {
                output->pts     = AV_NOPTS_VALUE;
                output->pkt_dts = AV_NOPTS_VALUE;
            } else {
                output->pts     = ctx->frame_count_out;
                output->pkt_dts = ctx->frame_count_out;
            }
            output->size = ctx->enc_out.frameSize;
        }

        usedMinQp = MIN(usedMinQp, ctx->rc.currentActiveQp);
        usedMaxQp = MAX(usedMaxQp, ctx->rc.currentActiveQp);

        /* No show frames are just copies of some existing frame,
         * so don't consume
         */
        if (ctx->enc_out.show_frame) {
            ctx->frame_count_out++;
        } else {
            ctx->frm_cnt_total++;
        }
        lag_status = 0;
        ctx->code_frame_count++;
        break;

    case VP9ENC_OUTPUT_BUFFER_OVERFLOW:
        VPILOGD("encoder status=VP9ENC_OUTPUT_BUFFER_OVERFLOW\n");
        vp9enc_print_frame(ctx, encoder, ctx->next, ret, pic_width, pic_height,
                           0);
        if (ctx->enc_out.show_frame) {
            ctx->frame_count_out++;
        }
        break;

    case VP9ENC_FIRSTPASS_FAILED:
        VPILOGE("VP9ENC_FIRSTPASS_FAILED\n");
        goto error;
        break;

    default:
        VPILOGD("encoder status=Unkown\n");
        vp9enc_print_error_value("VP9EncStrmEncode() failed.", ret);
        vp9enc_print_frame(ctx, encoder, ctx->next, ret, pic_width, pic_height,
                           0);
        goto exit;
    }

    enc_in->timeIncrement = cfg->output_rate_denom;
    ctx->pic_tobe_free = 0;
    if (cfg->lag_in_frames == 0) {
        ctx->pic_tobe_free = ctx->next + 1;
        ctx->input_real_count--;
    } else if (lag_status != 1) {
        if (ctx->enc_out.show_frame == 1) {
            ctx->input_real_count--;
            ctx->pic_tobe_free = ctx->enc_out.indexEncoded + 1;
        }
    }

exit:

#ifdef FPGA
    vp9enc_hw_performance(ctx);
#endif

    VPILOGD("lag_in_frames=%d,lag_status =%d, show_frame=%d,pic_tobe_free=%d\n",
            cfg->lag_in_frames, lag_status, ctx->enc_out.show_frame,
            ctx->pic_tobe_free);

    /* Print information about encoded frames */
    VPILOGD("encode finished, bitrate target %d bps, actual %d bps (%lu%%),"
            "Used Qp min: %d max: %d\n",
            ctx->rc.bitPerSecond, ctx->bitrate,
            (ctx->rc.bitPerSecond) ?
                (u64)ctx->bitrate * 100 / ctx->rc.bitPerSecond :
                0,
            usedMinQp, usedMaxQp);
    VPILOGD("Total of %lu frames encoded, %lu bytes.\n", ctx->frame_count_out,
            ctx->stream_size);
    return 0;

error:
    ctx->pic_tobe_free = 0;
    return -1;
}

int vpi_venc_vp9_control(VpiEncVp9Ctx *ctx, void *indata, void *outdata)
{
    VpiCtrlCmdParam *cmd            = (VpiCtrlCmdParam *)indata;
    VpiEncVp9Setting *cml           = &ctx->vp9_enc_cfg;
    int ret                         = 0;
    int next_pic                    = 0;

    if (ctx == NULL || indata == NULL) {
        VPILOGE("vpi_venc_vp9_control parameters error\n");
        return -1;
    }

    switch (cmd->cmd) {
    case VPI_CMD_VP9ENC_GET_EMPTY_FRAME_SLOT:
        ret = vp9enc_get_pic_buffer(ctx, outdata);
        break;

    case VPI_CMD_VP9ENC_CONSUME_PIC:
        ret = vp9enc_get_used_pic_mem(ctx, outdata);
        break;

    case VPI_CMD_VP9ENC_GET_FRAME_PACKET:
        ret = vp9enc_get_frame_packet(ctx, outdata);
        break;

    default:
        VPILOGE("vpi_venc_vp9_control: "
                "vpi_venc_vp9_control Invalid typer=%d.\n",
                cmd->cmd);
        ret = -1;
    }
    return ret;
}

int vpi_venc_vp9_put_frame(VpiEncVp9Ctx *ctx, void *indata)
{
    VpiEncVp9Pic *trans_pic = NULL;
    VpiFrame *frame = (VpiFrame *)indata;
    int i;

    pthread_mutex_lock(&ctx->enc_thread_mutex);
    for (i = 0; i < MAX_WAIT_DEPTH; i++) {
        if (ctx->pic_wait_list[i].pic == frame) {
            trans_pic = &ctx->pic_wait_list[i];
            break;
        }
    }
    if (i == MAX_WAIT_DEPTH) {
        pthread_mutex_unlock(&ctx->enc_thread_mutex);
        return -1;
    }

    if (frame->opaque == NULL) {
        VPILOGD("received empty input frame, EOF\n");
        ctx->eos_received = 1;
    } else {
        ctx->poc         = ctx->poc + 1;
        trans_pic->state = 1;
        trans_pic->used  = 0;
        trans_pic->poc   = ctx->poc;
    }

    pthread_mutex_unlock(&ctx->enc_thread_mutex);
    return 0;
}

int vpi_venc_vp9_get_packet(VpiEncVp9Ctx *ctx, void *outdata)
{
    VpiPacket *pkt = (VpiPacket *)outdata;
    Vp9EncBufLink *buf, *buf_next;
    void* out_pkt_mem;
    u8 *pdata;

    pthread_mutex_lock(&ctx->enc_thread_mutex);
    buf = ctx->stream_buf_head;
    out_pkt_mem = buf->item;
    pdata = (u8*)pkt->data;
    memcpy(pdata, (u8*)out_pkt_mem, buf->item_size);
    pkt->size = buf->item_size;
    buf_next = buf->next;

    ctx->stream_buf_head = vp9enc_buf_list_delete(ctx->stream_buf_head);
    if (buf->show) {
        pkt->pts = buf->pts;
        pkt->pkt_dts = buf->pkt_dts;
        buf->used = 0;
        buf->show = 0;
        pthread_mutex_unlock(&ctx->enc_thread_mutex);
        return 0;
    }

    // superframe
    if (buf_next == NULL) {
        VPILOGE("a show frame should exist\n");
        pthread_mutex_unlock(&ctx->enc_thread_mutex);
        return -1;
    }
    out_pkt_mem = buf_next->item;
    pdata += buf->item_size;
    memcpy(pdata, (u8*)out_pkt_mem, buf_next->item_size);
    buf->used = 0;
    buf->show = 0;
    pkt->size += buf_next->item_size;
    ctx->stream_buf_head = vp9enc_buf_list_delete(ctx->stream_buf_head);
    if (buf_next->show) {
        pkt->pts = buf_next->pts;
        pkt->pkt_dts = buf_next->pkt_dts;
        buf_next->used = 0;
        buf_next->show = 0;
    } else {
        VPILOGE("invisible frame\n");
        pthread_mutex_unlock(&ctx->enc_thread_mutex);
        return -1;
    }
    vp9enc_superframe(ctx, pkt);
    pthread_mutex_unlock(&ctx->enc_thread_mutex);
    return 0;
}

int vpi_venc_vp9_close(VpiEncVp9Ctx *ctx)
{
    VP9EncRet ret = 0;

    if (ctx == NULL) {
        VPILOGE("vpi_vp9enc_close ctx is NULL\n");
        return 0;
    }

    if (ctx->initialized == 0) {
        VPILOGE("ctx->initialized=%d, no inited... \n", ctx->initialized);
        return 0;
    }

    ctx->enc_thread_finish = 1;

    pthread_join(ctx->enc_thread_handle, NULL);
    pthread_mutex_destroy(&ctx->enc_thread_mutex);
    pthread_cond_destroy(&ctx->enc_thread_cond);

    if (ctx->encoder_is_open == true) {
        vp9enc_print_total(ctx);
        vp9enc_statistic(ctx);
        vp9enc_free_resource(ctx);
        VPILOGD("Going to release encoder: =%p\n", ctx->encoder);
        if ((ret = VP9EncRelease((VP9EncInst)ctx->encoder)) != VP9ENC_OK) {
            VPILOGE("VP9EncRelease() failed.", ret);
        }
        ctx->encoder_is_open = 0;
    }
    ctx->initialized = 0;
    VPILOGD("vpi_vp9enc_closed\n");
    return ret;
}
