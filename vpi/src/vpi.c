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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "vpi_api.h"
#include "vpi_error.h"
#include "vpi.h"
#include "vpi_types.h"
#include "vpi_log_manager.h"
#include "vpi_video_dec.h"
#include "vpi_video_prc.h"
#include "vpi_video_h26xenc.h"
#include "vpi_video_vp9enc.h"

#include "transcoder.h"
#include "trans_fd_api.h"
#include "trans_mem_api.h"

#ifdef FB_SYSLOG_ENABLE
#include "syslog_sink.h"
#endif

static VpiCodecCtx *vpi_codec_ctx[MAX_DEVICE_NUM] = {NULL};
static VpiHwCtx *vpi_hw_ctx[MAX_DEVICE_NUM]       = {NULL};
static VpiDevCtx *vpi_dev_ctx[MAX_DEVICE_NUM]     = {NULL};

static int log_enabled = 0;
static int log_cnt     = 0;

typedef struct{
    VpiRet ret;
    char *name;
}Vpi_Error_Name;

static Vpi_Error_Name gVPIError[] = {
    { VPI_ERR_SW, "Software" },
    { VPI_ERR_SYSTEM, "System" },
    { VPI_ERR_NO_AP_MEM, "No AP memory" },
    { VPI_ERR_NO_EP_MEM, "No EP memory" },
    { VPI_ERR_WRONG_STATE, "Wrong State" },
    { VPI_ERR_DEVICE, "error device" },

    { VPI_ERR_ENCODE, "encoding error" },
    { VPI_ERR_EWL, "EWL error" },
    { VPI_ERR_ENCODER_INIT, "encoder init error" },
    { VPI_ERR_ENCODER_OPITION, "encoder opitions within '-enc_params' error" },
    { VPI_ERR_ENCODE_WAITT_BUF, "wait buffer" },

    { VPI_ERR_DECODE, "decode error" },
    { VPI_ERR_DWL, "DWL error" },
    { VPI_ERR_DECODER_INIT, "decoder init error" },
    { VPI_ERR_DECODER_OPITION, "wrong decoder opition" },
    { VPI_ERR_DECODER_DATA, "decoder data error" },
    { VPI_ERR_DECODE_FORMAT, "decoder format error" },

    { VPI_ERR_PP, "PP error" },
    { VPI_ERR_PP_INIT, "PP init error" },
    { VPI_ERR_PP_OPITION, "PP opition error" },

    { VPI_ERR_SPLITER, "spliter error" },
    { VPI_ERR_SPLITER_INIT, "spliter init error" },
    { VPI_ERR_SPLITER_OPITION, "spliter opition error" },

    { VPI_ERR_HWDOWNLOADER, "Hardware downloader error" },
    { VPI_ERR_HWDOWNLOADER_INIT, "Hardware downloader init errir" },
    { VPI_ERR_HWDOWNLOADER_OPITION, "Hardware downloader opition error" },

    { VPI_ERR_HWUPLOADER, "Hardware uploader error" },
    { VPI_ERR_HWUPLOADER_INIT, "Hardware uploader init error" },
    { VPI_ERR_HWUPLOADER_OPITION, "Hardware uploader opition error" },
};

static int vpi_get_hw_ctx(int fd)
{
    int i;

    for (i = 0; i < MAX_DEVICE_NUM; i++) {
        if (vpi_hw_ctx[i] && vpi_hw_ctx[i]->hw_context == fd) {
            return i;
        }
    }
    if (i == MAX_DEVICE_NUM) {
        return -1;
    }
    return -1;
}

static VpiRet vpi_init(VpiCtx vpe_ctx, void *cfg)
{
    VpeVpiCtx *vpe_vpi_ctx = (VpeVpiCtx *)vpe_ctx;
    VpiDecCtx *dec_ctx;
    VpiEncVp9Ctx *vp9enc_ctx;
    VpiH26xEncCtx *h26xenc_ctx;
    VpiPrcCtx *prc_ctx;
    VpiRet ret = VPI_SUCCESS;
    VpiDecOption *dec_option = (VpiDecOption *)cfg;
    int i, idx;

    idx = vpi_get_hw_ctx(vpe_vpi_ctx->fd);
    if (idx == -1) {
        return VPI_ERR_SW;
    }

    switch (vpe_vpi_ctx->plugin) {
    case H264DEC_VPE:
        dec_ctx = (VpiDecCtx *)vpe_vpi_ctx->ctx;
        memset(dec_ctx, 0, sizeof(VpiDecCtx));
        dec_ctx->dec_fmt      = Dec_H264_H10P;
        dec_option->dev_name  = vpi_hw_ctx[idx]->device_name;
        dec_option->task_id   = vpi_hw_ctx[idx]->task_id;
        dec_option->priority  = vpi_hw_ctx[idx]->priority;
        ret                   = vpi_vdec_init(dec_ctx, dec_option);
        if (ret)
            return ret;
        break;

    case HEVCDEC_VPE:
        dec_ctx = (VpiDecCtx *)vpe_vpi_ctx->ctx;
        memset(dec_ctx, 0, sizeof(VpiDecCtx));
        dec_ctx->dec_fmt      = Dec_HEVC;
        dec_option->dev_name  = vpi_hw_ctx[idx]->device_name;
        dec_option->task_id   = vpi_hw_ctx[idx]->task_id;
        dec_option->priority  = vpi_hw_ctx[idx]->priority;
        ret                   = vpi_vdec_init(dec_ctx, cfg);
        if (ret)
            return ret;
        break;

    case VP9DEC_VPE:
        dec_ctx = (VpiDecCtx *)vpe_vpi_ctx->ctx;
        memset(dec_ctx, 0, sizeof(VpiDecCtx));
        dec_ctx->dec_fmt      = Dec_VP9;
        dec_option->dev_name  = vpi_hw_ctx[idx]->device_name;
        dec_option->task_id   = vpi_hw_ctx[idx]->task_id;
        dec_option->priority  = vpi_hw_ctx[idx]->priority;
        ret                   = vpi_vdec_init(dec_ctx, cfg);
        if (ret)
            return ret;
        break;

    case H26XENC_VPE:
        h26xenc_ctx = (VpiH26xEncCtx *)vpe_vpi_ctx->ctx;
        VpiH26xEncCfg *h26x_enc_cfg         = (VpiH26xEncCfg *)cfg;
        h26x_enc_cfg->priority           = vpi_hw_ctx[idx]->priority;
        h26x_enc_cfg->device             = vpi_hw_ctx[idx]->device_name;
        h26x_enc_cfg->frame_ctx->task_id = vpi_hw_ctx[idx]->task_id;
        ret                              = vpi_h26xe_init(h26xenc_ctx, h26x_enc_cfg);
        if (ret)
            return ret;
        break;

    case VP9ENC_VPE:
        vp9enc_ctx                = (VpiEncVp9Ctx *)vpe_vpi_ctx->ctx;
        VpiVp9EncCfg *vp9_enc_cfg = (VpiVp9EncCfg *)cfg;
        vp9_enc_cfg->priority     = vpi_hw_ctx[idx]->priority;
        vp9_enc_cfg->dev_name     = vpi_hw_ctx[idx]->device_name;
        vp9_enc_cfg->task_id      = vpi_hw_ctx[idx]->task_id;
        ret = vpi_venc_vp9_init(vp9enc_ctx, vp9_enc_cfg);
        if (ret)
            return ret;
        break;

    case PP_VPE:
        prc_ctx = (VpiPrcCtx *)vpe_vpi_ctx->ctx;
        memset(prc_ctx, 0, sizeof(VpiPrcCtx));
        prc_ctx->filter_type       = FILTER_PP;
        prc_ctx->ppfilter.params.device   = vpi_hw_ctx[idx]->device_name;
        prc_ctx->ppfilter.params.mem_id   = vpi_hw_ctx[idx]->task_id;
        prc_ctx->ppfilter.params.priority = vpi_hw_ctx[idx]->priority;
        if (vpi_hw_ctx[idx]) {
            ret = vpi_vprc_init(prc_ctx, &prc_ctx->ppfilter.params);
            if (ret)
                return ret;
        }
        break;

    case SPLITER_VPE:
        prc_ctx = (VpiPrcCtx *)vpe_vpi_ctx->ctx;
        memset(prc_ctx, 0, sizeof(VpiPrcCtx));
        prc_ctx->filter_type = FILTER_SPLITER;
        break;

    case HWDOWNLOAD_VPE:
        prc_ctx = (VpiPrcCtx *)vpe_vpi_ctx->ctx;
        memset(prc_ctx, 0, sizeof(VpiPrcCtx));
        prc_ctx->filter_type = FILTER_HW_DOWNLOADER;
        if (vpi_hw_ctx[idx]) {
            ret = vpi_vprc_init(prc_ctx, vpi_hw_ctx[idx]->device_name);
            if (ret)
                return ret;
        }
        break;

    case HWUPLOAD_VPE:
        prc_ctx = (VpiPrcCtx *)vpe_vpi_ctx->ctx;
        memset(prc_ctx, 0, sizeof(VpiPrcCtx));
        VpiHWUploadCfg * hwul_cfg = (VpiHWUploadCfg *)cfg;
        prc_ctx->filter_type      = FILTER_HW_UPLOAD;
        hwul_cfg->task_id         = vpi_hw_ctx[idx]->task_id;
        hwul_cfg->priority        = vpi_hw_ctx[idx]->priority;
        hwul_cfg->device          = vpi_hw_ctx[idx]->device_name;
        if (vpi_hw_ctx[idx]) {
            ret = vpi_vprc_init(prc_ctx, cfg);
            if (ret)
                return ret;
        }
        break;
    default:
        break;
    }

    VPILOGD("plugin %d init finished\n", vpe_vpi_ctx->plugin);
    return ret;
}

static VpiRet vpi_decode_put_packet(VpiCtx vpe_ctx, void *indata)
{
    VpeVpiCtx *vpe_vpi_ctx = (VpeVpiCtx *)vpe_ctx;
    VpiDecCtx *dec_ctx     = (VpiDecCtx *)vpe_vpi_ctx->ctx;
    VpiRet ret             = VPI_SUCCESS;

    switch (vpe_vpi_ctx->plugin) {
    case H264DEC_VPE:
    case HEVCDEC_VPE:
    case VP9DEC_VPE:
        ret = vpi_vdec_put_packet(dec_ctx, indata);
        break;
    case H26XENC_VPE:
    case VP9ENC_VPE:
    case PP_VPE:
    case SPLITER_VPE:
    case HWDOWNLOAD_VPE:
    case HWUPLOAD_VPE:
        VPILOGE("decode_put_packet function is not in current pluging %d",
                vpe_vpi_ctx->plugin);
        ret = VPI_ERR_SW;
        break;
    default:
        break;
    }

    return ret;
}

static int vpi_decode_get_frame(VpiCtx vpe_ctx, void *outdata)
{
    VpeVpiCtx *vpe_vpi_ctx = (VpeVpiCtx *)vpe_ctx;
    VpiDecCtx *dec_ctx     = (VpiDecCtx *)vpe_vpi_ctx->ctx;
    int ret                = 0;

    switch (vpe_vpi_ctx->plugin) {
    case H264DEC_VPE:
    case HEVCDEC_VPE:
    case VP9DEC_VPE:
        ret = vpi_vdec_get_frame(dec_ctx, outdata);
        break;
    case H26XENC_VPE:
    case VP9ENC_VPE:
    case PP_VPE:
    case SPLITER_VPE:
    case HWDOWNLOAD_VPE:
    case HWUPLOAD_VPE:
        VPILOGE("decode_get_frame function is not in current pluging %d",
                vpe_vpi_ctx->plugin);
        ret = VPI_ERR_SW;
        break;
    default:
        break;
    }

    return ret;
}

static int vpi_decode(VpiCtx vpe_ctx, void *indata, void *outdata)
{
    VpeVpiCtx *vpe_vpi_ctx = (VpeVpiCtx *)vpe_ctx;
    VpiDecCtx *dec_ctx     = (VpiDecCtx *)vpe_vpi_ctx->ctx;
    VpiRet ret             = VPI_SUCCESS;

    VPILOGD("plugin %d decode\n", vpe_vpi_ctx->plugin);
    switch (vpe_vpi_ctx->plugin) {
    case H264DEC_VPE:
    case HEVCDEC_VPE:
    case VP9DEC_VPE:
        ret = vpi_vdec_decode(dec_ctx, indata, outdata);
        break;
    case H26XENC_VPE:
    case VP9ENC_VPE:
    case PP_VPE:
    case SPLITER_VPE:
    case HWDOWNLOAD_VPE:
    case HWUPLOAD_VPE:
        VPILOGE("decode funtion is not in current plugin %d",
                vpe_vpi_ctx->plugin);
        ret = VPI_ERR_SW;
        break;
    default:
        break;
    }

    return ret;
}

static int vpi_encode_put_frame(VpiCtx vpe_ctx, void *indata)
{
    VpeVpiCtx *vpe_vpi_ctx = (VpeVpiCtx *)vpe_ctx;
    VpiH26xEncCtx *h26x_enc_ctx;
    VpiEncVp9Ctx *vp9_enc_ctx;
    int ret = 0;

    switch (vpe_vpi_ctx->plugin) {
    case H264DEC_VPE:
    case HEVCDEC_VPE:
    case VP9DEC_VPE:
    case PP_VPE:
    case SPLITER_VPE:
    case HWDOWNLOAD_VPE:
    case HWUPLOAD_VPE:
        VPILOGE("encode_put_frame function is not in current pluging %d",
                vpe_vpi_ctx->plugin);
        ret = VPI_ERR_SW;
        break;
    case H26XENC_VPE:
        h26x_enc_ctx = (VpiH26xEncCtx *)vpe_vpi_ctx->ctx;
        ret = vpi_h26xe_put_frame(h26x_enc_ctx, indata);
        break;
    case VP9ENC_VPE:
        vp9_enc_ctx = (VpiEncVp9Ctx *)vpe_vpi_ctx->ctx;
        ret = vpi_venc_vp9_put_frame(vp9_enc_ctx, indata);
        break;
    default:
        break;
    }

    return ret;
}

static int vpi_encode_get_packet(VpiCtx vpe_ctx, void *outdata)
{
    VpeVpiCtx *vpe_vpi_ctx     = (VpeVpiCtx *)vpe_ctx;
    VpiH26xEncCtx *h26xenc_ctx = NULL;
    VpiEncVp9Ctx *vp9_enc_ctx;
    VpiRet ret                 = VPI_SUCCESS;

    switch (vpe_vpi_ctx->plugin) {
    case H264DEC_VPE:
    case HEVCDEC_VPE:
    case VP9DEC_VPE:
    case PP_VPE:
    case SPLITER_VPE:
    case HWDOWNLOAD_VPE:
    case HWUPLOAD_VPE:
        VPILOGE("encode_get_packet function is not in current pluging %d",
                vpe_vpi_ctx->plugin);
        ret = VPI_ERR_SW;
        break;
    case H26XENC_VPE:
        h26xenc_ctx = (VpiH26xEncCtx *)vpe_vpi_ctx->ctx;
        ret = vpi_h26xe_get_packet(h26xenc_ctx, outdata);
        break;
    case VP9ENC_VPE:
        vp9_enc_ctx = (VpiEncVp9Ctx *)vpe_vpi_ctx->ctx;
        ret = vpi_venc_vp9_get_packet(vp9_enc_ctx, outdata);
        break;
    default:
        break;
    }

    return ret;
}

static int vpi_encode(VpiCtx vpe_ctx, void *indata, void *outdata)
{
    VpeVpiCtx *vpe_vpi_ctx = (VpeVpiCtx *)vpe_ctx;
    VpiH26xEncCtx *h26xenc_ctx;
    VpiEncVp9Ctx *vp9_enc_ctx = (VpiEncVp9Ctx *)vpe_vpi_ctx->ctx;
    VpiRet ret                = VPI_SUCCESS;

    switch (vpe_vpi_ctx->plugin) {
    case H264DEC_VPE:
    case HEVCDEC_VPE:
    case VP9DEC_VPE:
    case PP_VPE:
    case SPLITER_VPE:
    case HWDOWNLOAD_VPE:
    case HWUPLOAD_VPE:
        VPILOGE("encode funtion is not in current plugin %d",
                vpe_vpi_ctx->plugin);
        ret = VPI_ERR_SW;
        break;
    case H26XENC_VPE:
    case VP9ENC_VPE:
        ret = vpi_venc_vp9_encode(vp9_enc_ctx, indata, outdata);
        break;
    default:
        break;
    }

    return ret;
}

static int vpi_process(VpiCtx vpe_ctx, void *indata, void *outdata)
{
    VpeVpiCtx *vpe_vpi_ctx = (VpeVpiCtx *)vpe_ctx;
    VpiPrcCtx *prc_ctx     = (VpiPrcCtx *)vpe_vpi_ctx->ctx;
    VpiRet ret             = VPI_SUCCESS;

    switch (vpe_vpi_ctx->plugin) {
    case H264DEC_VPE:
    case HEVCDEC_VPE:
    case VP9DEC_VPE:
    case H26XENC_VPE:
    case VP9ENC_VPE:
        VPILOGE("process funtion is not in current plugin %d",
                vpe_vpi_ctx->plugin);
        ret = VPI_ERR_SW;
        break;
    case PP_VPE:
    case SPLITER_VPE:
    case HWDOWNLOAD_VPE:
    case HWUPLOAD_VPE:
        ret = vpi_vprc_process(prc_ctx, indata, outdata);
        break;
    default:
        break;
    }

    return ret;
}

static VpiRet vpe_control_interface(void *id, void *indata, void *outdata)
{
    int device_id             = *(int *)id;
    VpiCtrlCmdParam *in_param = (VpiCtrlCmdParam *)indata;
    int idx;

    idx = vpi_get_hw_ctx(device_id);
    if (idx == -1) {
        return -1;
    }
    switch (in_param->cmd) {
        case VPI_CMD_GET_FRAME_BUFFER: {
            VpiFrame **v_frame;
            v_frame = (VpiFrame **)outdata;
            *v_frame = (VpiFrame *)malloc(sizeof(struct VpiFrame));
            break;
        }
        case VPI_CMD_FREE_FRAME_BUFFER: {
            VpiFrame *v_frame;
            v_frame = (VpiFrame *)in_param->data;
            free(v_frame);
            break;
        }
        case VPI_CMD_GET_VPEFRAME_SIZE: {
            int *size = NULL;
            size = (int *)outdata;
            *size = sizeof(VpiFrame);
            return 0;
        }
        case VPI_CMD_GET_PICINFO_SIZE: {
            int *size = NULL;
            size = (int *)outdata;
            *size = sizeof(VpiPicInfo);
            return 0;
        }
        default:
            break;
    }
    return 0;
}

static VpiRet vpi_control(VpiCtx vpe_ctx, void *indata, void *outdata)
{
    VpeVpiCtx *vpe_vpi_ctx    = (VpeVpiCtx *)vpe_ctx;
    VpiDecCtx *dec_ctx        = (VpiDecCtx *)vpe_vpi_ctx->ctx;
    VpiEncVp9Ctx *vp9_enc_ctx = (VpiEncVp9Ctx *)vpe_vpi_ctx->ctx;
    VpiPrcCtx *prc_ctx        = (VpiPrcCtx *)vpe_vpi_ctx->ctx;
    VpiH26xEncCtx *h26xenc_ctx;
    VpiRet ret = VPI_SUCCESS;

    if (0 == vpe_control_interface(vpe_ctx, indata, outdata)) {
        return 0;
    }

    if (vpe_vpi_ctx == NULL || vpe_vpi_ctx->ctx == NULL) {
        VPILOGD("vpe_vpi_ctx %p has been destoryed\n", vpe_vpi_ctx);
        return 0;
    }
    switch (vpe_vpi_ctx->plugin) {
    case H264DEC_VPE:
    case HEVCDEC_VPE:
    case VP9DEC_VPE:
        vpi_vdec_control(dec_ctx, indata, outdata);
        break;

    case H26XENC_VPE:
        h26xenc_ctx = (VpiH26xEncCtx *)vpe_vpi_ctx->ctx;
        ret         = vpi_h26xe_ctrl(h26xenc_ctx, indata, outdata);
        return ret;

    case VP9ENC_VPE:
        ret = vpi_venc_vp9_control(vp9_enc_ctx, indata, outdata);
        return ret;

    case PP_VPE:
    case SPLITER_VPE:
    case HWDOWNLOAD_VPE:
    case HWUPLOAD_VPE:
        vpi_vprc_control(prc_ctx, indata, outdata);
        break;
    default:
        break;
    }

    if (ret) {
        return -1;
    } else {
        return 0;
    }
}

static VpiRet vpi_close(VpiCtx vpe_ctx)
{
    VpeVpiCtx *vpe_vpi_ctx    = (VpeVpiCtx *)vpe_ctx;
    VpiDecCtx *dec_ctx        = (VpiDecCtx *)vpe_vpi_ctx->ctx;
    VpiEncVp9Ctx *vp9_enc_ctx = (VpiEncVp9Ctx *)vpe_vpi_ctx->ctx;
    VpiPrcCtx *prc_ctx        = (VpiPrcCtx *)vpe_vpi_ctx->ctx;
    VpiH26xEncCtx *h26xenc_ctx;
    VpiRet ret = VPI_SUCCESS;
    int idx;

    idx = vpi_get_hw_ctx(vpe_vpi_ctx->fd);
    if (idx == -1) {
        return -1;
    }
    switch (vpe_vpi_ctx->plugin) {
    case H264DEC_VPE:
    case HEVCDEC_VPE:
    case VP9DEC_VPE:
        ret = vpi_vdec_close(dec_ctx);
        break;
    case H26XENC_VPE:
        h26xenc_ctx = (VpiH26xEncCtx *)vpe_vpi_ctx->ctx;
        vpi_h26xe_close(h26xenc_ctx);
        break;
    case VP9ENC_VPE:
        ret = vpi_venc_vp9_close(vp9_enc_ctx);
        break;
    case PP_VPE:
    case SPLITER_VPE:
    case HWDOWNLOAD_VPE:
    case HWUPLOAD_VPE:
        if (vpi_hw_ctx[idx]) {
            vpi_vprc_close(prc_ctx);
        }
        break;
    default:
        break;
    }

    if (ret) {
        return -1;
    } else {
        return 0;
    }
}

static VpiApi vpe_api = {
    vpi_init,
    vpi_decode,
    vpi_encode,
    vpi_decode_put_packet,
    vpi_decode_get_frame,
    vpi_encode_put_frame,
    vpi_encode_get_packet,
    vpi_control,
    vpi_process,
    vpi_close,
};

static VpiRet log_init(LogLevel log_level)
{
    char filename[512];
    time_t now;
    struct tm *tm;

    printf("VPE log_level = %d\n", log_level);
    log_setlevel(log_level);
    if( log_level<= LOG_LEVEL_OFF)
        return VPI_SUCCESS;

    time(&now);
    tm = localtime(&now);

    sprintf(filename, "vpi_%04d%02d%02d_%02d%02d%02d.log", tm->tm_year + 1900,
            tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    printf("VPE log_filename %s\n", filename);

    if (log_open(filename) != VPI_SUCCESS)
        return VPI_ERR_SW;

    log_enabled = 1;

    return VPI_SUCCESS;
}


void vpi_freep(void *arg)
{
    void *val;

    memcpy(&val, arg, sizeof(val));
    memcpy(arg, &(void *){ NULL }, sizeof(val));
    free(val);
}

VpiRet vpi_get_sys_info_struct(VpiSysInfo **sys_info)
{
    *sys_info = malloc(sizeof(VpiSysInfo));
    if (*sys_info == NULL) {
        VPILOGE("Can't allocate sys info struct for APP\n");
        return VPI_ERR_NO_AP_MEM;
    }
    return VPI_SUCCESS;
}

VpiRet vpi_get_media_proc_struct(VpiMediaProc **media_proc)
{
    *media_proc = malloc(sizeof(VpiMediaProc));
    if (*media_proc == NULL) {
        VPILOGE("Can't allocate media proc struct for APP\n");
        return VPI_ERR_NO_AP_MEM;
    }
    return VPI_SUCCESS;
}

VpiRet vpi_create(VpiCtx *ctx, VpiApi **vpi, int fd, VpiPlugin plugin)
{
    VpiDecCtx *dec_ctx;
    VpiEncVp9Ctx *vp9_enc_ctx;
    VpiH26xEncCtx *h26xenc_ctx;
    VpiPrcCtx *prc_ctx;
    VpiCodecCtx *codec_ctx = NULL;
    int i, j;

    if (HWCONTEXT_VPE == plugin) {
        VpiSysInfo *vpi_dev_info = (VpiSysInfo *)(*ctx);
        if (!vpi_dev_info) {
            printf("vpi dev info NULL\n");
            return VPI_ERR_SW;
        }

        if (vpi_dev_info->device >= 0) {
            for (i = 0; i < MAX_DEVICE_NUM; i++) {
                if (vpi_hw_ctx[i] &&
                    vpi_hw_ctx[i]->hw_context == vpi_dev_info->device) {
                    VPILOGE("device has been created\n");
                    return VPI_ERR_SW;
                }
            }
            for (i = 0; i < MAX_DEVICE_NUM; i++) {
                if (!vpi_hw_ctx[i]) {
                    vpi_hw_ctx[i]             = malloc(sizeof(VpiHwCtx));
                    vpi_hw_ctx[i]->sys_info   = (void *)*ctx;
                    vpi_hw_ctx[i]->hw_context = vpi_dev_info->device;
                    if (ioctl(vpi_hw_ctx[i]->hw_context, CB_TRANX_MEM_GET_TASKID,
                              &vpi_hw_ctx[i]->task_id) < 0) {
                        VPILOGE("get task id failed!\n");
                        return VPI_ERR_SW;
                    }
                    vpi_dev_info->task_id = vpi_hw_ctx[i]->task_id;
                    vpi_hw_ctx[i]->priority  = vpi_dev_info->priority;
                    for (j = 0; j < MAX_DEVICE_NUM; j++) {
                        if (vpi_dev_ctx[j] && vpi_dev_ctx[j]->fd == fd) {
                            vpi_hw_ctx[i]->device_name = vpi_dev_ctx[j]->device_name;
                            break;
                        }
                    }
#ifdef FB_SYSLOG_ENABLE
                    printf("sys log level %d\n", vpi_dev_info->sys_log_level);
                    init_syslog_module("system", vpi_dev_info->sys_log_level);
#endif
                    if (!log_enabled) {
                        if (log_init(vpi_dev_info->sys_log_level)) {
                            return VPI_ERR_SW;
                        }
                    }
                    log_cnt++;
                    *vpi = &vpe_api;
                    VPILOGD("hw ctx %d, fd %d\n", i, vpi_dev_info->device);
                    return 0;
                }
            }
            if (i == MAX_DEVICE_NUM) {
                VPILOGE("No empty device\n");
                return VPI_ERR_SW;
            }
        } else {
            VPILOGE("vpi dev handle error\n");
            return VPI_ERR_SW;
        }
    }

    if (NULL == ctx || NULL == vpi) {
        VPILOGE("invalid input ctx %p vpi %p\n", ctx, vpi);
        return VPI_ERR_SW;
    }

    *ctx = NULL;
    *vpi = NULL;

    VPILOGD("enter ctx %p vpi %p, plugin %d\n", ctx, vpi, plugin);
    VpeVpiCtx *vpe_vpi_ctx = malloc(sizeof(VpeVpiCtx));
    if (NULL == vpe_vpi_ctx) {
        VPILOGE("failed to allocate vpe_vpi context\n");
        return VPI_ERR_NO_AP_MEM;
    }
    memset(vpe_vpi_ctx, 0, sizeof(VpeVpiCtx));
    vpe_vpi_ctx->dummy  = 0xFFFFFFFF;
    vpe_vpi_ctx->plugin = plugin;
    vpe_vpi_ctx->fd     = fd;

    for (i = 0; i < MAX_DEVICE_NUM; i++) {
        if (vpi_codec_ctx[i]) {
            if (vpi_codec_ctx[i]->fd == fd) {
                VPILOGD("find %d vpi codec ctx\n", i);
                codec_ctx = vpi_codec_ctx[i];
                goto find_codec_ctx;
            }
        }
    }

    for (i = 0; i < MAX_DEVICE_NUM; i++) {
        if (NULL == vpi_codec_ctx[i]) {
            vpi_codec_ctx[i] = malloc(sizeof(VpiCodecCtx));
            if (NULL == vpi_codec_ctx[i]) {
                VPILOGE("failed to allocate vpi codec context\n");
                return VPI_ERR_NO_AP_MEM;
            }
            memset(vpi_codec_ctx[i], 0, sizeof(VpiCodecCtx));
            vpi_codec_ctx[i]->fd = fd;
            codec_ctx            = vpi_codec_ctx[i];
            break;
        }
    }
    if (i == MAX_DEVICE_NUM) {
        VPILOGE("Can't find valid vpi codec ctx\n");
        return VPI_ERR_SW;
    }

find_codec_ctx:
    switch (plugin) {
    case H264DEC_VPE:
    case HEVCDEC_VPE:
    case VP9DEC_VPE:
        dec_ctx = (VpiDecCtx *)malloc(sizeof(VpiDecCtx));
        memset(dec_ctx, 0, sizeof(VpiDecCtx));
        vpe_vpi_ctx->ctx       = dec_ctx;
        codec_ctx->vpi_dec_ctx = vpe_vpi_ctx;
        codec_ctx->ref_cnt++;
        break;
    case H26XENC_VPE:
        h26xenc_ctx = (VpiH26xEncCtx *)malloc(sizeof(VpiH26xEncCtx));
        if (h26xenc_ctx == NULL) {
            return -1;
        }
        memset(h26xenc_ctx, 0, sizeof(VpiH26xEncCtx));
        vpe_vpi_ctx->ctx       = h26xenc_ctx;
        codec_ctx->vpi_enc_ctx = vpe_vpi_ctx;
        codec_ctx->ref_cnt++;
        break;
    case VP9ENC_VPE:
        vp9_enc_ctx = (VpiEncVp9Ctx *)malloc(sizeof(VpiEncVp9Ctx));
        memset(vp9_enc_ctx, 0, sizeof(VpiEncVp9Ctx));
        vpe_vpi_ctx->ctx       = vp9_enc_ctx;
        codec_ctx->vpi_enc_ctx = vpe_vpi_ctx;
        codec_ctx->ref_cnt++;
        break;
    case PP_VPE:
        prc_ctx = (VpiPrcCtx *)malloc(sizeof(VpiPrcCtx));
        memset(prc_ctx, 0, sizeof(VpiPrcCtx));
        vpe_vpi_ctx->ctx          = prc_ctx;
        codec_ctx->vpi_prc_pp_ctx = vpe_vpi_ctx;
        codec_ctx->ref_cnt++;
        break;
    case SPLITER_VPE:
        prc_ctx = (VpiPrcCtx *)malloc(sizeof(VpiPrcCtx));
        memset(prc_ctx, 0, sizeof(VpiPrcCtx));
        vpe_vpi_ctx->ctx               = prc_ctx;
        codec_ctx->vpi_prc_spliter_ctx = vpe_vpi_ctx;
        codec_ctx->ref_cnt++;
        break;
    case HWDOWNLOAD_VPE:
        prc_ctx = (VpiPrcCtx *)malloc(sizeof(VpiPrcCtx));
        memset(prc_ctx, 0, sizeof(VpiPrcCtx));
        vpe_vpi_ctx->ctx            = prc_ctx;
        codec_ctx->vpi_prc_hwdw_ctx = vpe_vpi_ctx;
        codec_ctx->ref_cnt++;
        break;
    case HWUPLOAD_VPE:
        prc_ctx = (VpiPrcCtx *)malloc(sizeof(VpiPrcCtx));
        memset(prc_ctx, 0, sizeof(VpiPrcCtx));
        vpe_vpi_ctx->ctx          = prc_ctx;
        prc_ctx->filter_type      = FILTER_HW_UPLOAD;
        codec_ctx->ref_cnt++;
        break;
    default:
        break;
    }

    *ctx = vpe_vpi_ctx;
    *vpi = &vpe_api;
    return VPI_SUCCESS;
}

VpiRet vpi_destroy(VpiCtx ctx, int fd)
{
    int i;
    VpiHwCtx *hw_ctx = NULL;

    for (i = 0; i < MAX_DEVICE_NUM; i++) {
        if (vpi_hw_ctx[i] && vpi_hw_ctx[i]->hw_context == fd) {
            hw_ctx = vpi_hw_ctx[i];
            break;
        }
    }
    if (i == MAX_DEVICE_NUM) {
        return VPI_SUCCESS;
    }

    if (ctx == hw_ctx->sys_info) {
        if (hw_ctx->hw_context) {
            if (ioctl(hw_ctx->hw_context, CB_TRANX_MEM_FREE_TASKID,
                      &hw_ctx->task_id) < 0) {
                VPILOGE("free hw context task id failed!\n");
            }

            if (hw_ctx) {
                free(hw_ctx);
                hw_ctx        = NULL;
                vpi_hw_ctx[i] = NULL;
            }
#ifdef FB_SYSLOG_ENABLE
            close_syslog_module();
#endif
            log_cnt--;
            if (!log_cnt) {
                if (log_enabled) {
                    log_close();
                    log_enabled = 0;
                }
            }
        }
        return 0;
    }
    VPILOGD("destroy %p\n", ctx);
    VpeVpiCtx *vpe_vpi_ctx = (VpeVpiCtx *)ctx;
    free(vpe_vpi_ctx->ctx);
    vpe_vpi_ctx->ctx = NULL;
    free(ctx);

    for (i = 0; i < MAX_DEVICE_NUM; i++) {
        if (vpi_codec_ctx[i] && vpi_codec_ctx[i]->fd == fd) {
            vpi_codec_ctx[i]->ref_cnt--;
            if (vpi_codec_ctx[i]->ref_cnt == 0) {
                free(vpi_codec_ctx[i]);
                vpi_codec_ctx[i] = NULL;
                break;
            }
        }
    }

    return VPI_SUCCESS;
}

int vpi_open_hwdevice(const char *device)
{
    int fd;
    int i;

    for (i = 0; i < MAX_DEVICE_NUM; i++) {
        if (!vpi_dev_ctx[i]) {
            vpi_dev_ctx[i] = malloc(sizeof(VpiDevCtx));
            strcpy(vpi_dev_ctx[i]->device_name, device);
            break;
        }
    }
    if (i == MAX_DEVICE_NUM) {
        VPILOGE("No empty device\n");
        return -1;
    }

#ifdef CHECK_MEM_LEAK_TRANS
    TransCheckMemLeakInit();
#endif

    fd = TranscodeOpenFD(vpi_dev_ctx[i]->device_name, O_RDWR);
    vpi_dev_ctx[i]->fd = fd;
    return fd;
}

int vpi_close_hwdevice(int fd)
{
    int i;

    for (i = 0; i < MAX_DEVICE_NUM; i++) {
        if (vpi_dev_ctx[i] && vpi_dev_ctx[i]->fd == fd) {
            break;
        }
    }
    if (i == MAX_DEVICE_NUM) {
        VPILOGE("fd %d device not opened\n", fd);
        return -1;
    }
    free(vpi_dev_ctx[i]);
    vpi_dev_ctx[i] = NULL;

#ifdef CHECK_MEM_LEAK_TRANS
    TransCheckMemLeakGotResult();
#endif

    return TranscodeCloseFD(fd);
}

char *vpi_error_str(int vpi_error)
{
    int i = 0;

    for(i=0; i<sizeof(gVPIError)/sizeof(Vpi_Error_Name); i++){
        if( gVPIError[i].ret == vpi_error)
            return gVPIError[i].name;
    }
    return "VPI Common";
}