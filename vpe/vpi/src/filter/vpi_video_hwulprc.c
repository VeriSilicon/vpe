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
#include <fcntl.h>
#include <sys/ioctl.h>

#include "dectypes.h"

#include "transcoder.h"
#include "trans_edma_api.h"
#include "trans_fd_api.h"
#include "hugepage_api.h"

#include "vpi_types.h"
#include "vpi_log.h"
#include "vpi_video_hwulprc.h"
#include "vpi_video_prc.h"

typedef struct {
    uint32_t client_type;
    char * device;
    int mem_id;
} MWLInitParam;

typedef struct {
    uint32_t client_type;
    int fd_memalloc;
    int mem_id;
    char * device;
    EDMA_HANDLE edma_handle;
} MWL;

static i32 mwl_release(const void *instance)
{
    MWL *mwl = (MWL *)instance;

    if (mwl) {
        if (mwl->edma_handle) {
            TRANS_EDMA_release(mwl->edma_handle);
        }

        if (mwl->fd_memalloc > 0) {
            TranscodeCloseFD(mwl->fd_memalloc);
        }

        DWLfree(mwl);
        mwl = NULL;
    }
    return 0;
}

static void * mwl_init(MWLInitParam * param)
{
    MWL *mwl;
    mwl = (MWL *)DWLcalloc(1, sizeof(MWL));
    if (mwl == NULL) {
        VPILOGE("failed to alloc struct MWL struct\n");
        return NULL;
    }

    mwl->client_type = param->client_type;
    mwl->fd_memalloc = -1;
    if (param->device == NULL) {
        VPILOGE("device name error\n");
        goto err;
    }
    mwl->device = param->device;
    mwl->mem_id = param->mem_id;
    mwl->fd_memalloc = TranscodeOpenFD(param->device, O_RDWR | O_SYNC);
    if (mwl->fd_memalloc == -1) {
        VPILOGE("failed to open: %s\n", param->device);
        goto err;
    }

    VPILOGI("mwl: device %s, mem_id %d, fd_memalloc %d \n",
              mwl->device, mwl->mem_id, mwl->fd_memalloc);

    return mwl;
err:
    VPILOGE("FAILED\n");
    mwl_release(mwl);

    return NULL;
}

static i32 mwl_malloc_linear(const void *instance, u32 size,
                             struct DWLLinearMem *info)
{
    MWL *mwl               = (MWL *)instance;
    uint32_t pgsize        = 4096;
    int ret                = 0;
    struct mem_info params = {0};
    uint32_t alloc_flag    = 0;

#define EP_SIDE_EN		(1<<1)
#define RC_SIDE_EN		(1<<2)

    if (mwl == NULL || info == NULL) {
        VPILOGE("mwl %p, info %p\n", mwl, info);
        return DWL_ERROR;
    }

#ifndef NEXT_MULTIPLE
#define NEXT_MULTIPLE(value, n) (((value) + (n) - 1) & ~((n) - 1))
#endif

    info->logical_size       = size;
    size                     = NEXT_MULTIPLE(size, pgsize);
    info->size               = size;
    info->virtual_address    = NULL;
    info->bus_address        = 0;
    info->bus_address_rc     = 0;
    info->virtual_address_ep = NULL;
    params.size              = info->size;

    if (info->mem_type == DWL_MEM_TYPE_CPU_FILE_SINK)
        alloc_flag = RC_SIDE_EN;
    else if (info->mem_type == DWL_MEM_TYPE_DPB)
        alloc_flag = EP_SIDE_EN;
    else
        alloc_flag = EP_SIDE_EN | RC_SIDE_EN;

    if (alloc_flag & EP_SIDE_EN) {
        params.task_id      = mwl->mem_id;
        params.mem_location = EP_SIDE;
        params.size         = info->size;
        VPILOGI("EP: size(%d) task_id(%d)\n", params.size, params.task_id);
        ret = ioctl(mwl->fd_memalloc, CB_TRANX_MEM_ALLOC, &params);
        if (ret) {
            VPILOGE("ERROR! No linear buffer available\n");
            return DWL_ERROR;
        }
        info->bus_address = params.phy_addr;
        info->size        = params.size;
    }
    if (alloc_flag & RC_SIDE_EN) {
        info->virtual_address = fbtrans_get_huge_pages(info->size);
        info->bus_address_rc  = (addr_t)info->virtual_address;
        if (info->virtual_address == NULL)
            return DWL_ERROR;
    }

    return DWL_OK;
}

static void mwl_free_linear(const void *instance, struct DWLLinearMem *info)
{
#ifdef BUILD_CMODEL
    if (info->virtual_address)
        DWLFreeLinear(instance, info);
#else
    MWL *mwl = (MWL *)instance;
    int i;
    int ret;
    struct mem_info params;

    if(info == NULL){
        VPILOGE("info NULL\n");
        return;
    }

#ifndef ENABLE_HUGEPAGE
    if ((info->virtual_address != NULL)
        && (info->virtual_address != MAP_FAILED)
        && (info->size != 0))
        munmap(info->virtual_address, info->size);
#endif

    params.task_id      = mwl->mem_id;
    params.mem_location = EP_SIDE;
    params.phy_addr     = info->bus_address;
    params.size         = info->size;
    params.rc_kvirt     = info->rc_kvirt;
    if (info->bus_address != 0) {
        ret = ioctl(mwl->fd_memalloc, CB_TRANX_MEM_FREE, &params);
    }

#ifndef ENABLE_HUGEPAGE
    params.task_id      = mwl->mem_id;
    params.mem_location = RC_SIDE;
    params.phy_addr     = info->bus_address_rc;
    params.size         = info->size;
    params.rc_kvirt     = info->rc_kvirt;
    if (info->bus_address_rc != 0) {
        ret = ioctl(mwl->fd_memalloc, CB_TRANX_MEM_FREE, &params);
    }
#else
    if (info->virtual_address) {
        fbtrans_free_huge_pages(info->virtual_address, info->size);
        info->virtual_address = NULL;
    }
#endif
#endif
}

int vpi_prc_get_empty_pic(VpiPrcHwUlCtx *ctx)
{
    int i;

    pthread_mutex_lock(&ctx->hw_upload_mutex);
    for (i = 0; i < ctx->mwl_nums; i++) {
        if (ctx->mwl_used[i] == 0) {
            pthread_mutex_unlock(&ctx->hw_upload_mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&ctx->hw_upload_mutex);
    return -1;
}

static int vpi_upload_add_extra_buffer(VpiPrcHwUlCtx *ctx,
                                       int enc_need_buffers_num,
                                       int current_pp_buffers_num)
{
    int i               = 0;
    int ext_buffers_num = enc_need_buffers_num;

    if (ctx == NULL || ext_buffers_num <= 0) {
        VPILOGE("enc_need_buffers_num %d, current_pp_buffers_num %d, error!\n",
                 ext_buffers_num, current_pp_buffers_num);
        return -1;
    }

    ctx->mwl_nums += ext_buffers_num;

    for (i = current_pp_buffers_num;
         i < current_pp_buffers_num + ext_buffers_num;
         i++) {
        ctx->mwl_mem[i].mem_type = DWL_MEM_TYPE_DPB;
        if (mwl_malloc_linear(ctx->mwl, ctx->mwl_item_size,
                             &ctx->mwl_mem[i]) != DWL_OK) {
            VPILOGE("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n");
            return -1;
        }
        VPILOGD("out_buffer i %d, bus_address=0x%llx, bus_address_rc=0x%llx,"
                 "virtual_address %p, virtual_address_ep %p, size %d \n", i,
                ctx->mwl_mem[i].bus_address,
                ctx->mwl_mem[i].bus_address_rc,
                ctx->mwl_mem[i].virtual_address,
                ctx->mwl_mem[i].virtual_address_ep,
                ctx->mwl_mem[i].size);

        ctx->mwl_used[i] = 0;
    }

    VPILOGD("add ext_buffers_num %d, total out_buf_nums %d\n",
             ext_buffers_num, ctx->mwl_nums);

    return 0;
}

static int vpi_upload_check_buffer_number_for_trans(VpiPrcHwUlCtx *ctx)
{
    int enc_need_buffers_num;
    int current_pp_buffers_num;

    enc_need_buffers_num   = ctx->frame->hwupload_max_frames_delay;
    current_pp_buffers_num = ctx->mwl_nums;

    if (ctx->mwl_nums_init + enc_need_buffers_num > current_pp_buffers_num) {
        VPILOGD("out_buf_nums_init %d, enc_need_buffers_num %d,"
                 "current_pp_buffers_num %d\n",
                ctx->mwl_nums_init, enc_need_buffers_num,
                current_pp_buffers_num);
        return vpi_upload_add_extra_buffer(ctx, enc_need_buffers_num,
                                           current_pp_buffers_num );
    }

    return 0;
}

static void  mwl_pic_consume(VpiPrcHwUlCtx *ctx, void *in_data)
{
    VpiFrame *frame               = (VpiFrame *)in_data;
    struct DecPicturePpu *picture = (struct DecPicturePpu *)frame->data[0];
    int i;

    pthread_mutex_lock(&ctx->hw_upload_mutex);

    for (i = 0; i < ctx->mwl_nums; i++) {
        if (ctx->mwl_mem[i].bus_address == picture->pictures[0].luma.bus_address
            && ctx->mwl_used[i] == 1) {
            ctx->mwl_used[i] = 0;
            free(picture);
            break;
        }
    }
    if (i == ctx->mwl_nums) {
        VPILOGE("Can't find valid mwl\n");
    }

    pthread_mutex_unlock(&ctx->hw_upload_mutex);
}

VpiRet vpi_prc_hwul_process(VpiPrcCtx *vpi_ctx, void *indata, void *outdata)
{
    VpiPrcHwUlCtx *ctx = &(vpi_ctx->hwul_ctx);
    struct DWLLinearMem *mwl_mem_item;
    VpiFrame *in_frame  = (VpiFrame *)indata;
    VpiFrame *out_frame = (VpiFrame *)outdata;
    unsigned long linesize32_y, linesize32_uv;
    uint32_t y_size,uv_size;
    int idx, ret;

    pthread_mutex_lock(&ctx->hw_upload_mutex);
    if (vpi_upload_check_buffer_number_for_trans(ctx) < 0) {
        pthread_mutex_unlock(&ctx->hw_upload_mutex);
        return -1;
    }
    pthread_mutex_unlock(&ctx->hw_upload_mutex);

    struct DecPicturePpu *pic = malloc(sizeof(struct DecPicturePpu));
    if (!pic)
        return -1;
    memset(pic, 0, sizeof(struct DecPicturePpu));
    out_frame->data[0] = (void *)pic;

    idx = vpi_prc_get_empty_pic(ctx);
    if (idx == -1) {
        return -1;
    }
    mwl_mem_item = &ctx->mwl_mem[idx];

    if (ctx->format == VPI_FMT_UYVY) {
        linesize32_y  = ((in_frame->linesize[0]+31)/32)*32;
    } else {
        linesize32_y  = ((in_frame->linesize[0]+1023)/1024)*1024;
    }
    linesize32_uv = ((in_frame->linesize[1]+1023)/1024)*1024;
    y_size        = (((in_frame->src_height+7)/8)*8) * linesize32_y;
    uv_size       = in_frame->src_height * linesize32_uv / 2;
    if(ctx->format == VPI_FMT_YUV420P)
        uv_size = in_frame->src_height * linesize32_uv;
    pic->pictures[0].luma.bus_address   = mwl_mem_item->bus_address;
    if (ctx->format != VPI_FMT_UYVY)
        pic->pictures[0].chroma.bus_address = pic->pictures[0].luma.bus_address
                                              + y_size;
    VPILOGD("pic %p luma.bus_address %p, chroma.bus_address %p,"
            "linesize32_y %d, linesize32_uv %d, y_size %d, uv_size %d,"
            "in->height %d, in->linesize[0] %d, in->linesize[1] %d,"
            "in->linesize[2] %d, linesize32_y %d, linesize32_uv %d\n",
            pic, pic->pictures[0].luma.bus_address,
            pic->pictures[0].chroma.bus_address, linesize32_y, linesize32_uv,
            y_size, uv_size, in_frame->src_height, in_frame->linesize[0],
            in_frame->linesize[1], in_frame->linesize[2],
            linesize32_y, linesize32_uv);

    uint8_t *addr_offset = NULL;
    unsigned int i;
    //copy Y lines to hugepage buffer, then rc to ep
    for (i = 0; i < in_frame->src_height; i++) {
        addr_offset = ctx->p_hugepage_buf_y + i*linesize32_y;
        memcpy(addr_offset, in_frame->data[0]+i*in_frame->linesize[0],
               in_frame->linesize[0]);
    }
    ret = TRANS_EDMA_RC2EP_nonlink(vpi_ctx->edma_handle,
                                   (u64)ctx->p_hugepage_buf_y,
                                   pic->pictures[0].luma.bus_address, y_size);
    if (ret) {
        VPILOGE("TRANS_EDMA_RC2EP_nonlink failed. ret %d,"
                "luma.bus_address %p, y_size %d\n",
                ret, pic->pictures[0].luma.bus_address, y_size);
        return -1;
    }

    if (ctx->format != VPI_FMT_UYVY) {
        //copy UV lines to hugepage buffer, then rc to ep.
        //NV12 and P010le chroma: 1 plane, UV
        for (i = 0; i < in_frame->src_height/2; i++) {
            memcpy(ctx->p_hugepage_buf_uv+i*linesize32_uv,
                    in_frame->data[1]+i*in_frame->linesize[1],
                    in_frame->linesize[1]);
        }
        ret = TRANS_EDMA_RC2EP_nonlink(vpi_ctx->edma_handle,
                                       (u64)ctx->p_hugepage_buf_uv,
                                       pic->pictures[0].chroma.bus_address,
                                       uv_size);
        if (ret) {
            VPILOGE("TRANS_EDMA_RC2EP_nonlink failed. ret %d,"
                    "chroma.bus_address %p, uv_size %d\n",
                    ret, pic->pictures[0].chroma.bus_address, uv_size);
            return -1;
        }
    }

    ctx->mwl_used[idx] = 1;
    return 0;
}

VpiRet vpi_prc_hwul_init(VpiPrcCtx *vpi_ctx, void *cfg)
{
    VpiPrcHwUlCtx *ctx = &(vpi_ctx->hwul_ctx);
    VpiHWUploadCfg *vpi_cfg = (VpiHWUploadCfg *)cfg;
    VpiFrame *frame = vpi_cfg->frame;
    uint32_t align_width, align_height;
    int i;

    vpi_ctx->edma_handle = TRANS_EDMA_init(vpi_cfg->device);
    if (vpi_ctx->edma_handle == NULL) {
        VPILOGE("hwupload edma_handle init failed!\n");
        return VPI_ERR_DEVICE;
    }

    frame->raw_format          = vpi_cfg->format;
    frame->pic_info[0].enabled = 1;
    if (vpi_cfg->format == VPI_FMT_P010LE)
        frame->pic_info[0].format = VPI_YUV420_PLANAR_10BIT_P010;
    else if (vpi_cfg->format == VPI_FMT_UYVY)
        frame->pic_info[0].format = VPI_YUV422_INTERLEAVED_UYVY;
    else
        frame->pic_info[0].format = VPI_YUV420_SEMIPLANAR;

    /* FIXME: nv12,p010(all formats) uses 32 align */
    align_width = ((frame->src_width + 31)/32)*32;

    /*For upload filter directly link to encoder(8000E or bigsea) */
    /*Bigsea require 1024 align, so we set 1024 align for 8000E and bigsea */
    if(VPI_FMT_P010LE == vpi_cfg->format)
        align_width = (((frame->src_width*2 + 1023)/1024)*1024)/2;
    else if(VPI_FMT_NV12 == vpi_cfg->format)
        align_width = ((frame->src_width + 1023)/1024)*1024;
    else if(VPI_FMT_UYVY == vpi_cfg->format)
        align_width = ((frame->src_width + 31)/32)*32;
    else
        align_width = ((frame->src_width + 31)/32)*32;
    VPILOGD("aligne_width %d, format %d %d\n",
            align_width, frame->pic_info[0].format, vpi_cfg->format);
    frame->pic_info[0].pic_width  = align_width;
    frame->pic_info[0].pic_height = frame->src_height;
    frame->pic_info[0].width      = frame->src_width;
    frame->pic_info[0].height     = frame->src_height;
    frame->pic_info[0].picdata.is_interlaced = 0;
    frame->pic_info[0].picdata.pic_format    = 2;   //2:DEC_OUT_FRM_RASTER_SCAN
    frame->pic_info[0].picdata.pic_pixformat =
                        VPI_FMT_P010LE == vpi_cfg->format ? 1 : 0;
    frame->pic_info[0].picdata.bit_depth_luma =
                        VPI_FMT_P010LE == vpi_cfg->format ? 10 : 8;
    frame->pic_info[0].picdata.bit_depth_chroma =
                        VPI_FMT_P010LE == vpi_cfg->format ? 10 : 8;
    frame->pic_info[0].picdata.pic_compressed_status = 0;    //0:not compress

    frame->flag |= HWUPLOAD_FLAG;
    ctx->frame   = frame;
    ctx->format  = vpi_cfg->format;

    //malloc EP buffer
    MWLInitParam mwlParam = {DWL_CLIENT_TYPE_ST_PP,
                             vpi_cfg->device,
                             vpi_cfg->task_id};
    ctx->mwl = mwl_init(&mwlParam);
    if (ctx->mwl == NULL) {
        VPILOGE("Transcode demuxer mwl init failed!\n");
        return -1;
    }

    align_height = ((frame->src_height + 31) / 32) * 32;

    ctx->mwl_nums = ctx->mwl_nums_init = 3;
    if (ctx->mwl_nums > MWL_BUF_DEPTH) {
        VPILOGE("TOO MANY BUFFERS REQUEST!\n");
        return -1;
    }

    ctx->i_hugepage_size_y = align_width * align_height;
    if (VPI_FMT_P010LE == vpi_cfg->format ||
        VPI_FMT_UYVY == vpi_cfg->format)
        ctx->i_hugepage_size_y *= 2;

    ctx->p_hugepage_buf_y   = fbtrans_get_huge_pages(ctx->i_hugepage_size_y);
    if(VPI_FMT_UYVY != vpi_cfg->format) {
        ctx->i_hugepage_size_uv = ctx->i_hugepage_size_y/2;
        ctx->p_hugepage_buf_uv  = fbtrans_get_huge_pages(ctx->i_hugepage_size_uv);
    }

    ctx->mwl_item_size  = ctx->i_hugepage_size_y + ctx->i_hugepage_size_uv;
    for(int i = 0; i < ctx->mwl_nums; i++){
        ctx->mwl_mem[i].mem_type = DWL_MEM_TYPE_DPB;
        if (mwl_malloc_linear(ctx->mwl, ctx->mwl_item_size, &ctx->mwl_mem[i])){
            VPILOGE("No memory available for the stream buffer\n");
            return -1;
        }
        VPILOGD("mwl i %d, bus_address=0x%llx, bus_address_rc=0x%llx,"
                "size %d, align_width %d, align_height %d\n",
                i, ctx->mwl_mem[i].bus_address, ctx->mwl_mem[i].bus_address_rc,
                ctx->mwl_mem[i].size, align_width, align_height);

        ctx->mwl_used[i] = 0;
    }

    pthread_mutex_init(&ctx->hw_upload_mutex, NULL);

    return VPI_SUCCESS;
}

VpiRet vpi_prc_hwul_control(VpiPrcCtx *vpi_ctx, void *indata, void *outdata)
{
    VpiPrcHwUlCtx *ctx = &(vpi_ctx->hwul_ctx);
    VpiCtrlCmdParam *in_param = (VpiCtrlCmdParam *)indata;

    switch (in_param->cmd) {
    case VPI_CMD_HWUL_FREE_BUF:
        mwl_pic_consume(ctx, in_param->data);
        break;
    case VPI_CMD_HWDL_INIT_OPTION: {
        VpiHWUploadCfg **cfg;
        cfg  = (VpiHWUploadCfg **)outdata;
        *cfg = (VpiHWUploadCfg *)malloc(sizeof(VpiHWUploadCfg));
        memset(*cfg, 0, sizeof(VpiHWUploadCfg));
        break;
    }
    default:
        break;
    }

    return VPI_SUCCESS;
}

VpiRet vpi_prc_hwul_close(VpiPrcCtx *vpi_ctx)
{
    VpiPrcHwUlCtx *ctx = &(vpi_ctx->hwul_ctx);
    int i;

    if (vpi_ctx->edma_handle) {
        TRANS_EDMA_release(vpi_ctx->edma_handle);
        vpi_ctx->edma_handle = NULL;
    }

    if(ctx->mwl){
        for(i = 0; i < ctx->mwl_nums; i++)
            mwl_free_linear(ctx->mwl, &ctx->mwl_mem[i]);
        mwl_release(ctx->mwl);
        ctx->mwl = NULL;
    }
    pthread_mutex_destroy(&ctx->hw_upload_mutex);

    if(ctx->p_hugepage_buf_y != NULL){
        fbtrans_free_huge_pages(ctx->p_hugepage_buf_y, ctx->i_hugepage_size_y);
        ctx->p_hugepage_buf_y = NULL;
    }
    if(ctx->p_hugepage_buf_uv != NULL){
        fbtrans_free_huge_pages(ctx->p_hugepage_buf_uv,
                                ctx->i_hugepage_size_uv);
        ctx->p_hugepage_buf_uv = NULL;
    }
    return VPI_SUCCESS;
}