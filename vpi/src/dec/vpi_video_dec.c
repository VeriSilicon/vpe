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
#include <stdlib.h>
#include <stdio.h>

#include "deccfg.h"
#include "h264hwd_container.h"

#include "vpi_types.h"
#include "vpi_log.h"
#include "vpi_video_dec.h"
#include "vpi_video_dec_pp.h"
#include "vpi_video_dec_cfg.h"
#include "vpi_video_dec_buffer.h"
#include "vpi_video_h264dec.h"
#include "vpi_video_hevcdec.h"
#include "vpi_video_vp9dec.h"

static void vpi_dec_log_header_init(VpiDecCtx *vpi_ctx, VpiDecOption *dec_cfg)
{
#ifdef FB_SYSLOG_ENABLE
    static char module_name[] = "DEC";
    if (strlen(vpi_ctx->module_name)) {
        vpi_ctx->log_header.module_name = vpi_ctx->module_name;
    } else {
        vpi_ctx->log_header.module_name = module_name;
    }
#ifdef DRV_NEW_ARCH
    vpi_ctx->log_header.device_id = get_deviceId(dec_cfg->dev_name);
#else
    vpi_ctx->log_header.device_id = 0;
#endif
#endif
}

void *decode_process(void *param)
{
    VpiDecCtx *vpi_ctx = (VpiDecCtx *)param;
    int ret = 0;

    while (!vpi_ctx->dec_thread_finish) {
        switch (vpi_ctx->dec_fmt) {
        case Dec_H264_H10P:
            ret = vpi_decode_h264_dec_process(vpi_ctx);
            break;
        case Dec_HEVC:
            ret = vpi_decode_hevc_dec_process(vpi_ctx);
            break;
        case Dec_VP9:
            ret = vpi_decode_vp9_dec_process(vpi_ctx);
            break;
        default:
            break;
        }
        if (ret == 0) {
            usleep(500);
        } else if (ret == -1) {
            break;
        }
    }

    return NULL;
}

static VpiRet vpi_dec_init_decoder(VpiDecCtx *vpi_ctx, void *cfg)
{
    VpiDecOption *dec_cfg = (VpiDecOption *)cfg;
    VpiRet ret         = VPI_SUCCESS;
    uint32_t size, i;

    ret = vpi_check_out_format_for_trans(vpi_ctx, dec_cfg);
    if (ret) {
        VPILOGE("%s %d error ret %d\n", __func__, __LINE__, ret);
        return ret;
    }

    ret = vpi_dec_init_wrapper(vpi_ctx);
    if (ret) {
        VPILOGE("%s error ret %d\n", __func__, __LINE__, ret);
        return ret;
    }

    vpi_dec_set_default_config(vpi_ctx);
#ifdef FB_SYSLOG_ENABLE
    vpi_dec_log_header_init(vpi_ctx, dec_cfg);
#endif
    vpi_dec_init_hw_cfg(vpi_ctx, dec_cfg);
    /* set test bench configuration */
    vpi_tb_set_default_cfg(&vpi_ctx->tb_cfg);
    vpi_resolve_pp_overlap_ppu(vpi_ctx->vpi_dec_config.ppu_cfg,
                               vpi_ctx->tb_cfg.pp_units_params);

    if (dec_cfg->pp_setting && strlen(dec_cfg->pp_setting) > 0) {
        ret = vpi_dec_parse_resize(vpi_ctx, dec_cfg);
        if (ret) {
            VPILOGE("vpi_dec_parse_resize error, ret %d\n", ret);
            return ret;
        }
    } else {
        vpi_ctx->resize_num = 0;
    }

    vpi_dec_parse_ppu_cfg(vpi_ctx, vpi_ctx->vpi_dec_config.ppu_cfg);
    vpi_dec_get_tb_cfg(vpi_ctx);

    vpi_ctx->src_width       = dec_cfg->src_width;
    vpi_ctx->src_height      = dec_cfg->src_height;
    vpi_ctx->frame           = dec_cfg->frame;
    vpi_ctx->dwl_init.mem_id = dec_cfg->task_id;
    vpi_ctx->dwl_inst        = DWLInit(&vpi_ctx->dwl_init);
    if (vpi_ctx->dwl_inst == NULL) {
        VPILOGE("DWLInit# ERROR: DWL Init failed\n");
        return VPI_ERR_UNKNOWN;
    } else {
        VPILOGD("DWLInit#: DWL Init OK\n");
    }

    if (vpi_ctx->dec_fmt == Dec_H264_H10P) {
        ret = vpi_decode_h264_init(vpi_ctx);
        if (ret) {
            VPILOGE("vpi h264 decode init fail\n");
            return ret;
        }

        if (vpi_ctx->rlc_mode) {
            /*Force the decoder into RLC mode */
            ((decContainer_t *)vpi_ctx->dec_inst)->force_rlc_mode = 1;
            ((decContainer_t *)vpi_ctx->dec_inst)->rlc_mode       = 1;
            ((decContainer_t *)vpi_ctx->dec_inst)->try_vlc        = 0;
        }
    } else if (vpi_ctx->dec_fmt == Dec_HEVC) {
        ret = vpi_decode_hevc_init(vpi_ctx);
        if (ret) {
            VPILOGE("vpi hevc decode init fail\n");
            return ret;
        }
    } else if (vpi_ctx->dec_fmt == Dec_VP9) {
        ret = vpi_decode_vp9_init(vpi_ctx);
        if (ret) {
            VPILOGE("vpi vp9 decode init fail\n");
            return ret;
        }
    }
    if (vpi_ctx->enable_mc) {
        size = 4096 * 1165;
    } else {
        //size = vpi_ctx->max_strm_len;
        size = 1 * 1024 * 1024;
    }

    for (i = 0; i < vpi_ctx->allocated_buffers; i++) {
        vpi_ctx->stream_mem_used[i]     = 0;
        vpi_ctx->stream_mem[i].mem_type = DWL_MEM_TYPE_DPB;
        vpi_ctx->strm_buf_list[i]       = malloc(sizeof(BufLink));
        if (NULL == vpi_ctx->strm_buf_list[i]) {
            VPILOGE("UNABLE TO ALLOCATE STREAM BUFFER LIST MEMORY\n");
            return VPI_ERR_MALLOC;
        }
        vpi_ctx->strm_buf_list[i]->mem_idx = 0xFFFFFFFF;
        vpi_ctx->strm_buf_list[i]->next    = NULL;
        if (DWLMallocLinear(vpi_ctx->dwl_inst, size, vpi_ctx->stream_mem + i) !=
            DWL_OK) {
            VPILOGE("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n");
            return VPI_ERR_MALLOC;
        } else {
            VPILOGD("alloc memory for %d stream ,addr=0x%x, size is 0x%x OK\n",
                    i, vpi_ctx->stream_mem[i].virtual_address,
                    vpi_ctx->stream_mem[i].size);
        }
    }
    for (i = 0; i < MAX_BUFFERS; i++) {
        vpi_ctx->frame_buf_list[i] = malloc(sizeof(BufLink));
        if (NULL == vpi_ctx->frame_buf_list[i]) {
            VPILOGE("UNABLE TO ALLOCATE FRAME BUFFER LIST\n");
            return VPI_ERR_MALLOC;
        }
        vpi_ctx->frame_buf_list[i]->item    = NULL;
        vpi_ctx->frame_buf_list[i]->mem_idx = 0xFFFFFFFF;
        vpi_ctx->frame_buf_list[i]->used    = 0;
        vpi_ctx->frame_buf_list[i]->next    = NULL;
    }
    for (i = 0; i < 32; i++) {
        vpi_ctx->rls_strm_buf_list[i] = malloc(sizeof(BufLink));
        if (NULL == vpi_ctx->rls_strm_buf_list[i]) {
            VPILOGE("UNABLE TO ALLOCATE RELEASE STREAM BUFFER LIST MEMORY\n");
            return VPI_ERR_MALLOC;
        }
        vpi_ctx->rls_strm_buf_list[i]->mem_idx = 0xFFFFFFFF;
        vpi_ctx->rls_strm_buf_list[i]->next    = NULL;
        vpi_ctx->rls_strm_buf_list[i]->item    = NULL;
    }
    vpi_ctx->strm_buf_head       = NULL;
    vpi_ctx->frame_buf_head      = NULL;
    vpi_ctx->rls_strm_buf_head   = NULL;
    vpi_ctx->stream_mem_index    = 0;
    vpi_ctx->rls_mem_index       = 0;
    vpi_ctx->pic_decode_number   = 1;
    vpi_ctx->pic_display_number  = 0;
    vpi_ctx->got_package_number  = 0;
    vpi_ctx->prev_width          = 0;
    vpi_ctx->prev_height         = 0;
    vpi_ctx->max_frames_delay    = 0;
    vpi_ctx->output_num          = 0;

    for (i = 0; i < MAX_PTS_DTS_DEPTH; i++) {
        vpi_ctx->time_stamp_info[i].pts     = VDEC_NOPTS_VALUE;
        vpi_ctx->time_stamp_info[i].pkt_dts = VDEC_NOPTS_VALUE;
        vpi_ctx->time_stamp_info[i].used    = 0;
    }
    vpi_ctx->waiting_for_dpb      = 0;
    pthread_mutex_init(&vpi_ctx->dec_thread_mutex, NULL);
    pthread_cond_init(&vpi_ctx->dec_thread_cond, NULL);
    vpi_ctx->dec_thread_finish = 0;
    ret = pthread_create(&vpi_ctx->dec_thread_handle, NULL, decode_process,
                         vpi_ctx);
    if (ret) {
        VPILOGE("Unable to create dec thread\n");
        return VPI_ERR_UNKNOWN;
    }

    return VPI_SUCCESS;
}

VpiRet vpi_vdec_init(VpiDecCtx *vpi_ctx, void *dec_cfg)
{
    VpiRet ret = VPI_SUCCESS;

    switch (vpi_ctx->dec_fmt) {
    case Dec_H264_H10P:
#ifdef FB_SYSLOG_ENABLE
        sprintf(vpi_ctx->module_name, "H264DEC");
#endif
        break;
    case Dec_HEVC:
#ifdef FB_SYSLOG_ENABLE
        sprintf(vpi_ctx->module_name, "HEVCDEC");
#endif
        break;
    case Dec_VP9:
#ifdef FB_SYSLOG_ENABLE
        sprintf(vpi_ctx->module_name, "VP9DEC");
#endif
        break;
    default:
        VPILOGW("Unknown/Not supported format %d", vpi_ctx->dec_fmt);
        ret = VPI_ERR_VALUE;
    }

    ret = vpi_dec_init_decoder(vpi_ctx, dec_cfg);

    return ret;
}

int vpi_vdec_decode(VpiDecCtx *vpi_ctx, void *indata, void *outdata)
{
    int ret = VPI_SUCCESS;

    switch (vpi_ctx->dec_fmt) {
    case Dec_H264_H10P:
        ret = vpi_decode_h264_dec_frame(vpi_ctx, indata, outdata);
        break;
    case Dec_HEVC:
        ret = vpi_decode_hevc_dec_frame(vpi_ctx, indata, outdata);
        break;
    case Dec_VP9:
        ret = vpi_decode_vp9_dec_frame(vpi_ctx, indata, outdata);
        break;
    default:
        VPILOGW("Unknown/Not supported format %d", vpi_ctx->dec_fmt);
        ret = VPI_ERR_VALUE;
    }
    //ret = vpi_dec_decode_frame(vpi_ctx, indata, outdata);

    return ret;
}

int vpi_vdec_put_packet(VpiDecCtx *vpi_ctx, void *indata)
{
    VpiRet ret = VPI_SUCCESS;

    switch (vpi_ctx->dec_fmt) {
    case Dec_H264_H10P:
        ret = vpi_decode_h264_put_packet(vpi_ctx, indata);
        break;
    case Dec_HEVC:
        ret = vpi_decode_hevc_put_packet(vpi_ctx, indata);
        break;
    case Dec_VP9:
        ret = vpi_decode_vp9_put_packet(vpi_ctx, indata);
        break;
    default:
        VPILOGW("Unknown/Not supported format %d", vpi_ctx->dec_fmt);
        ret = -1;
    }
    return ret;
}

int vpi_vdec_get_frame(VpiDecCtx *vpi_ctx, void *outdata)
{
    int ret = VPI_SUCCESS;

    switch (vpi_ctx->dec_fmt) {
    case Dec_H264_H10P:
        ret = vpi_decode_h264_get_frame(vpi_ctx, outdata);
        break;
    case Dec_HEVC:
        ret = vpi_decode_hevc_get_frame(vpi_ctx, outdata);
        break;
    case Dec_VP9:
        ret = vpi_decode_vp9_get_frame(vpi_ctx, outdata);
        break;
    default:
        VPILOGW("Unknown/Not supported format %d", vpi_ctx->dec_fmt);
        ret = VPI_ERR_VALUE;
    }

    return ret;
}

VpiRet vpi_vdec_control(VpiDecCtx *vpi_ctx, void *indata, void *outdata)
{
    VpiCtrlCmdParam *in_param = (VpiCtrlCmdParam *)indata;
    VpiRet ret = VPI_SUCCESS;

    switch (in_param->cmd) {
        case VPI_CMD_DEC_GET_STRM_BUF_PKT: {
            VpiPacket **v_packet;
            v_packet = (VpiPacket **)outdata;
            *v_packet = (VpiPacket *)malloc(sizeof(VpiPacket));
            memset(*v_packet, 0, sizeof(VpiPacket));
            return ret;
        }
        case VPI_CMD_DEC_INIT_OPTION: {
            VpiDecOption **dec_opt;
            dec_opt = (VpiDecOption **)outdata;
            *dec_opt = (VpiDecOption *)malloc(sizeof(VpiDecOption));
            memset(*dec_opt, 0, sizeof(VpiDecOption));
            return ret;
        }
        default:
            break;
    }

    switch (vpi_ctx->dec_fmt) {
    case Dec_H264_H10P:
        ret = vpi_decode_h264_control(vpi_ctx, indata, outdata);
        break;
    case Dec_HEVC:
        ret = vpi_decode_hevc_control(vpi_ctx, indata, outdata);
        break;
    case Dec_VP9:
        ret = vpi_decode_vp9_control(vpi_ctx, indata, outdata);
        break;
    default:
        VPILOGW("Unknown/Not supported format %d", vpi_ctx->dec_fmt);
        ret = VPI_ERR_VALUE;
    }

    return ret;
}

VpiRet vpi_vdec_close(VpiDecCtx *vpi_ctx)
{
    VpiRet ret = VPI_SUCCESS;

    vpi_ctx->dec_thread_finish = 1;

    pthread_join(vpi_ctx->dec_thread_handle, NULL);
    pthread_cond_destroy(&vpi_ctx->dec_thread_cond);
    pthread_mutex_destroy(&vpi_ctx->dec_thread_mutex);

    switch (vpi_ctx->dec_fmt) {
    case Dec_H264_H10P:
        vpi_decode_h264_close(vpi_ctx);
        break;
    case Dec_HEVC:
        vpi_decode_hevc_close(vpi_ctx);
        break;
    case Dec_VP9:
        vpi_decode_vp9_close(vpi_ctx);
        break;
    default:
        VPILOGW("Unknown/Not supported format %d", vpi_ctx->dec_fmt);
        ret = VPI_ERR_VALUE;
    }

    return ret;
}
