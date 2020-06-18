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

#include "vpi_types.h"
#include "vpi_log.h"
#include "vpi_video_dec_buffer.h"
#include "vpi_video_dec_picture_consume.h"
#include "vpi_video_dec_info.h"

void vpi_dec_buf_list_add(BufLink **head, BufLink *list)
{
    BufLink *temp;

    if(NULL == *head) {
        *head = list;
        (*head)->next = NULL;
    } else {
        temp = *head;
        while(temp) {
            if(NULL == temp->next) {
                temp->next = list;
                list->next = NULL;
                return;
            }
            temp = temp->next;
        }
    }
}

BufLink* vpi_dec_buf_list_delete(BufLink *head)
{
    if (NULL == head || NULL == head->next) {
        return NULL;
    }

    return head->next;
}

int vpi_send_packet_to_decode_buffer(VpiDecCtx *vpi_ctx, VpiPacket *vpi_packet,
                                     struct DWLLinearMem stream_buffer)
{
    int ret = 0;

#ifdef NEW_MEM_ALLOC
    if (vpi_packet->data) {
        ret = dwl_edma_rc2ep_nolink(vpi_ctx->dwl_inst,
                                    (uint64_t)vpi_packet->data,
                                    stream_buffer.bus_address,
                                    vpi_packet->size);
    }
#endif

    return ret;
}

int vpi_dec_get_stream_buffer_index(VpiDecCtx *vpi_ctx, int status)
{
    int idx;

    pthread_mutex_lock(&vpi_ctx->dec_thread_mutex);
    idx = vpi_ctx->stream_mem_index;
    if (vpi_ctx->strm_buf_list[idx]->mem_idx == 0xFFFFFFFF) {
        pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
        return 0;
    }
    pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
    return -1;
}

static uint32_t vpi_dec_find_ext_buffer_index(VpiDecCtx *vpi_ctx, addr_t addr)
{
    uint32_t i;

    for (i = 0; i < MAX_BUFFERS; i++) {
        if (vpi_ctx->ext_buffers[i].bus_address == addr) {
            break;
        }
    }

    if (i >= MAX_BUFFERS)
        return -1;

    return i;
}

static uint32_t vpi_dec_find_empty_index(VpiDecCtx *vpi_ctx)
{
    uint32_t i;

    for (i = 0; i < MAX_BUFFERS; i++) {
        if (vpi_ctx->ext_buffers[i].bus_address == 0) {
            break;
        }
    }

    if (i >= MAX_BUFFERS)
        return -1;

    return i;
}

void vpi_dec_release_ext_buffers(VpiDecCtx *vpi_ctx)
{
    int i;

    for (i = 0; i < vpi_ctx->num_buffers; i++) {
        DWLFreeLinear(vpi_ctx->dwl_inst, &vpi_ctx->ext_buffers[i]);
        DWLmemset(&vpi_ctx->ext_buffers[i], 0, sizeof(vpi_ctx->ext_buffers[i]));
    }
}

//adding extra buffer to decoder dynamically
static int vpi_dec_add_extra_buffer(VpiDecCtx *vpi_ctx,
                                    int nb_frames,
                                    uint32_t buffer_size)
{
    enum DecRet rv;
    int i;
    int nb_ext_buf;

    if (nb_frames > vpi_ctx->num_buffers) {

        nb_ext_buf = nb_frames - vpi_ctx->num_buffers;

        if (buffer_size) {
            VPILOGD("buffer size %d\n", buffer_size);
            VPILOGD("nb_ext_buf = %d\n", nb_ext_buf);

            for (i = 0; i < nb_ext_buf; i++) {
                struct DWLLinearMem mem;
                int ret;

                mem.mem_type = DWL_MEM_TYPE_DPB;
                ret = DWLMallocLinear(vpi_ctx->dwl_inst, buffer_size, &mem);
                if (ret) {
                    VPILOGD("DWLMallocLinear ret %d\n", ret);
                    return -1;
                }
                rv = vpi_ctx->vpi_dec_wrapper.add_buffer(vpi_ctx->dec_inst, &mem);
                if (rv != DEC_OK && rv != DEC_WAITING_FOR_BUFFER) {
                    DWLFreeLinear(vpi_ctx->dwl_inst, &mem);
                } else {
                    int id;

                    id = vpi_dec_find_empty_index(vpi_ctx);
                    VPILOGD("find id %d\n", id);
                    if (id < 0) {
                        return -1;
                    }
                    vpi_ctx->ext_buffers[id] = mem;
                    vpi_ctx->buffer_consumed[id] = 1;
                    if (id >= vpi_ctx->num_buffers)
                        vpi_ctx->num_buffers++;
                }
            }
        }
    }

    return 1;
}

int vpi_dec_check_buffer_number_for_trans(VpiDecCtx *vpi_ctx)
{
    int nb_frames;
    struct DecBufferInfo buf_info;
    enum DecRet rv;

    nb_frames = vpi_ctx->max_frames_delay;
    rv = vpi_ctx->vpi_dec_wrapper.get_buffer_info(vpi_ctx->dec_inst, &buf_info);
    if (rv == DEC_WAITING_FOR_BUFFER && buf_info.buf_to_free.bus_address) {
        DWLFreeLinear(vpi_ctx->dwl_inst, &buf_info.buf_to_free);
        int id = vpi_dec_find_ext_buffer_index(vpi_ctx,
                                           buf_info.buf_to_free.bus_address);
        if (id < 0) {
            goto err_exit;
        }
        vpi_ctx->ext_buffers[id].virtual_address = NULL;
        vpi_ctx->ext_buffers[id].bus_address = 0;
        if (id == vpi_ctx->num_buffers - 1)
            vpi_ctx->num_buffers--;
    }

    if (buf_info.buf_num + nb_frames > vpi_ctx->num_buffers) {
        return vpi_dec_add_extra_buffer(vpi_ctx,
                                        buf_info.buf_num + nb_frames,
                                        buf_info.next_buf_size);
    }

    return 0;

err_exit:
    return -1;
}

VpiRet vpi_dec_output_frame(VpiDecCtx *vpi_ctx, VpiFrame *vpi_frame,
                            struct DecPicturePpu *decoded_pic)
{
    int i;
    struct DecPicturePpu *pic = malloc(sizeof(struct DecPicturePpu));
    if (!pic) {
        return VPI_ERR_MALLOC;
    }

    memset(pic, 0, sizeof(struct DecPicturePpu));
    memcpy(pic, decoded_pic, sizeof(struct DecPicturePpu));
    vpi_report_dec_pic_info(vpi_ctx, pic);
    vpi_ctx->cycle_count += pic->pictures[0].picture_info.cycles_per_mb;

    vpi_frame->width       = pic->pictures[1].pic_width;
    vpi_frame->height      = pic->pictures[1].pic_height;
    vpi_frame->linesize[0] = pic->pictures[1].pic_width;
    vpi_frame->linesize[1] = pic->pictures[1].pic_width / 2;
    vpi_frame->linesize[2] = pic->pictures[1].pic_width / 2;
    vpi_frame->key_frame =
        (pic->pictures[1].picture_info.pic_coding_type == DEC_PIC_TYPE_I);

    for (i = 0; i < MAX_STRM_BUFFERS; i++) {
        if (vpi_ctx->time_stamp_info[i].decode_id ==
            pic->pictures[0].picture_info.decode_id) {
            vpi_frame->pts     = vpi_ctx->time_stamp_info[i].pts;
            vpi_frame->pkt_dts = vpi_ctx->time_stamp_info[i].pkt_dts;
            vpi_ctx->time_stamp_info[i].used = 0;
            break;
        }
    }

    if (i == MAX_STRM_BUFFERS) {
        vpi_frame->pts     = VDEC_NOPTS_VALUE;
        vpi_frame->pkt_dts = VDEC_NOPTS_VALUE;
    }
    VPILOGD("width = %d, height = %d, linesize[0] = %d, key_frame = %d\n",
            vpi_frame->width, vpi_frame->height, vpi_frame->linesize[0],
            vpi_frame->key_frame);

    PpUnitConfig *pp0 = &vpi_ctx->vpi_dec_config.ppu_cfg[0];
    if (pp0->enabled == 1) {
        pic->pictures[0].pp_enabled = 0;
    } else {
        pic->pictures[0].pp_enabled = 1;
    }
    for (i = 1; i < 5; i++) {
        PpUnitConfig *pp = &vpi_ctx->vpi_dec_config.ppu_cfg[i - 1];
        if (pp->enabled == 1) {
            pic->pictures[i].pp_enabled = 1;
            VPILOGD("pic.pictures[%d].pp_enabled = %d,comperss_status=%d\n", i,
                    pic->pictures[i].pp_enabled,
                    pic->pictures[i].pic_compressed_status);
        } else {
            pic->pictures[i].pp_enabled = 0;
        }
    }

    for (i = 1; i < 5; i++) {
        VPILOGD("pic.pictures[%d].pp_enabled = %d, \
                comperss_status = %d, bit_depth_luma = %d\n",
                i, pic->pictures[i].pp_enabled,
                pic->pictures[i].pic_compressed_status,
                pic->pictures[i].sequence_info.bit_depth_luma);
    }
    VPILOGD("vpi_ctx->pic_rdy = 1\n");

    if (pic->pictures[1].pp_enabled) {
        //pic[1] means pp0, if pp0 enabled pic_info[0](rfc) should not set
        vpi_frame->pic_info[0].enabled = 0;
    } else {
        vpi_frame->pic_info[0].width   = vpi_frame->width;
        vpi_frame->pic_info[0].height  = vpi_frame->height;
        vpi_frame->pic_info[0].enabled = 1;

        vpi_frame->pic_info[0].picdata.is_interlaced =
            pic->pictures[0].sequence_info.is_interlaced;
        vpi_frame->pic_info[0].picdata.pic_stride = pic->pictures[0].pic_stride;
        vpi_frame->pic_info[0].picdata.crop_out_width =
            pic->pictures[0].sequence_info.crop_params.crop_out_width;
        vpi_frame->pic_info[0].picdata.crop_out_height =
            pic->pictures[0].sequence_info.crop_params.crop_out_height;
        vpi_frame->pic_info[0].picdata.pic_format =
            pic->pictures[0].picture_info.format;
        vpi_frame->pic_info[0].picdata.pic_pixformat =
            pic->pictures[0].picture_info.pixel_format;
        vpi_frame->pic_info[0].picdata.bit_depth_luma =
            pic->pictures[0].sequence_info.bit_depth_luma;
        vpi_frame->pic_info[0].picdata.bit_depth_chroma =
            pic->pictures[0].sequence_info.bit_depth_chroma;
        vpi_frame->pic_info[0].picdata.pic_compressed_status =
            pic->pictures[0].pic_compressed_status;

        if (vpi_ctx->resizes[0].x || vpi_ctx->resizes[0].y ||
            vpi_ctx->resizes[0].cw || vpi_ctx->resizes[0].ch) {
            vpi_frame->pic_info[0].crop.enabled = 1;
            vpi_frame->pic_info[0].crop.x       = vpi_ctx->resizes[0].x;
            vpi_frame->pic_info[0].crop.y       = vpi_ctx->resizes[0].y;
            vpi_frame->pic_info[0].crop.w       = vpi_ctx->resizes[0].cw;
            vpi_frame->pic_info[0].crop.h       = vpi_ctx->resizes[0].ch;
        }
    }

    for (i = 1; i < 5; i++) {
        PpUnitConfig *pp = &vpi_ctx->vpi_dec_config.ppu_cfg[i - 1];

        if (!pic->pictures[i].pp_enabled) {
            continue;
        }

        vpi_frame->pic_info[i].enabled = pic->pictures[i].pp_enabled;
        vpi_frame->pic_info[i].pic_width = pic->pictures[i].pic_width;
        vpi_frame->pic_info[i].pic_height = pic->pictures[i].pic_height;
        if (pp->scale.enabled) {
            vpi_frame->pic_info[i].width = pp->scale.width;
            vpi_frame->pic_info[i].height = pp->scale.height;
        } else {
            if (pp->crop.enabled) {
                vpi_frame->pic_info[i].width = pp->crop.width;
                vpi_frame->pic_info[i].height = pp->crop.height;
            } else {
                //pp0 has no scale, but hevc case 12283&12315 out pic_width*pic_height is not width*height
                vpi_frame->pic_info[i].width = MIN(pic->pictures[i].pic_width, vpi_frame->width);
                vpi_frame->pic_info[i].height = MIN(pic->pictures[i].pic_height, vpi_frame->height);
            }
        }

        vpi_frame->pic_info[i].picdata.is_interlaced =
            pic->pictures[i].sequence_info.is_interlaced;
        vpi_frame->pic_info[i].picdata.pic_stride = pic->pictures[i].pic_stride;
        vpi_frame->pic_info[i].picdata.crop_out_width =
            pic->pictures[i].sequence_info.crop_params.crop_out_width;
        vpi_frame->pic_info[i].picdata.crop_out_height =
            pic->pictures[i].sequence_info.crop_params.crop_out_height;
        vpi_frame->pic_info[i].picdata.pic_format =
            pic->pictures[i].picture_info.format;
        vpi_frame->pic_info[i].picdata.pic_pixformat =
            pic->pictures[i].picture_info.pixel_format;
        vpi_frame->pic_info[i].picdata.bit_depth_luma =
            pic->pictures[i].sequence_info.bit_depth_luma;
        vpi_frame->pic_info[i].picdata.bit_depth_chroma =
            pic->pictures[i].sequence_info.bit_depth_chroma;
        vpi_frame->pic_info[i].picdata.pic_compressed_status =
            pic->pictures[i].pic_compressed_status;

        VPILOGD("vpi_frame->pic_info[%d].enabled = %d\n", i,
                vpi_frame->pic_info[i].enabled);
        VPILOGD("vpi_frame->pic_info[%d].width = %d\n", i,
                vpi_frame->pic_info[i].width);
        VPILOGD("vpi_frame->pic_info[%d].height = %d\n", i,
                vpi_frame->pic_info[i].height);
        VPILOGD("vpi_frame->pic_info[%d].pic_width = %d\n", i,
                vpi_frame->pic_info[i].pic_width);
        VPILOGD("vpi_frame->pic_info[%d].pic_height = %d\n", i,
                vpi_frame->pic_info[i].pic_height);
        VPILOGD("vpi_frame->pic_info[%d].is_interlaced = %d\n", i,
                vpi_frame->pic_info[i].picdata.is_interlaced);
        VPILOGD("vpi_frame->pic_info[%d].pic_format = %d\n", i,
                vpi_frame->pic_info[i].picdata.pic_format);
        VPILOGD("vpi_frame->pic_info[%d].pic_pixformat = %d\n", i,
                vpi_frame->pic_info[i].picdata.pic_pixformat);
        VPILOGD("vpi_frame->pic_info[%d].bit_depth_luma = %d\n", i,
                vpi_frame->pic_info[i].picdata.bit_depth_luma);
        VPILOGD("vpi_frame->pic_info[%d].bit_depth_chroma = %d\n", i,
                vpi_frame->pic_info[i].picdata.bit_depth_chroma);
        VPILOGD("vpi_frame->pic_info[%d].pic_compressed_status = %d\n", i,
                vpi_frame->pic_info[i].picdata.pic_compressed_status);
    }

    VPILOGD("[%d][%d(%dx%d)][%d(%dx%d)][%d(%dx%d)][%d(%dx%d)]\n",
            vpi_frame->pic_info[0].enabled, vpi_frame->pic_info[1].enabled,
            vpi_frame->pic_info[1].width, vpi_frame->pic_info[1].height,
            vpi_frame->pic_info[2].enabled, vpi_frame->pic_info[2].width,
            vpi_frame->pic_info[2].height, vpi_frame->pic_info[3].enabled,
            vpi_frame->pic_info[3].width, vpi_frame->pic_info[3].height,
            vpi_frame->pic_info[4].enabled, vpi_frame->pic_info[4].width,
            vpi_frame->pic_info[4].height);

    if (vpi_ctx->vce_ds_enable) {
        if (!vpi_frame->pic_info[1].enabled ||
            !vpi_frame->pic_info[2].enabled) {
            VPILOGE("When use 1/4 ds pass1 for vce, \
                    pp0 and pp1 should be enabled!\n");
            return VPI_ERR_WRONG_STATE;
        }
        if (vpi_frame->pic_info[0].enabled || vpi_frame->pic_info[3].enabled ||
            vpi_frame->pic_info[4].enabled) {
            VPILOGE("When use 1/4 ds pass1 for vce, \
                    except pp0 and pp1 should not be enabled!\n");
            return VPI_ERR_WRONG_STATE;
        }
        vpi_frame->pic_info[2].flag = 1;
    }

    vpi_frame->data[0]         = (uint8_t *)pic;
    vpi_frame->pic_struct_size = sizeof(struct DecPicturePpu);
    add_dec_pic_wait_consume_list(vpi_ctx, pic);

    return VPI_SUCCESS;
}
