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

#include "deccfg.h"
#include "decapicommon.h"
#include "ppu.h"

#include "vpi_types.h"
#include "vpi_log.h"
#include "vpi_video_vp9dec.h"
#include "vpi_video_dec_info.h"
#include "vpi_video_dec_buffer.h"
#include "vpi_video_dec_cfg.h"
#include "vpi_video_dec_picture_consume.h"
#include "vpi_video_dec_pp.h"

VpiRet vpi_dec_vp9_init(const void **inst, struct DecConfig config,
                        const void *dwl)
{
    enum DecPictureFormat format = config.output_format;
    enum DecRet rv               = DEC_OK;

    struct Vp9DecConfig dec_cfg;
    dec_cfg.use_video_freeze_concealment = config.concealment_mode;
    dec_cfg.num_frame_buffers            = 9;
    dec_cfg.dpb_flags                    = 4;
    dec_cfg.use_video_compressor         = config.use_video_compressor;
    dec_cfg.use_ringbuffer               = config.use_ringbuffer;
    dec_cfg.output_format                = format;
    dec_cfg.tile_by_tile                 = config.tile_by_tile;
    if (config.ppu_cfg[0].out_cut_8bits) {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_CUT_8BIT;
    } else if (config.ppu_cfg[0].out_p010) {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_P010;
    } else if (config.ppu_cfg[0].out_be) {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_CUSTOMER1;
    } else {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_DEFAULT;
    }
    memcpy(dec_cfg.ppu_cfg, config.ppu_cfg, sizeof(config.ppu_cfg));
    rv = Vp9DecInit(inst, dwl, &dec_cfg);
    if (rv) {
        VPILOGD("Vp9DecInit ret %s\n", dec_ret_string(rv));
        return VPI_ERR_UNKNOWN;
    }
    return VPI_SUCCESS;
}

VpiRet vpi_dec_vp9_get_info(VpiDecInst inst, struct DecSequenceInfo *info)
{
    struct Vp9DecInfo vp9_info = { 0 };
    enum DecRet rv             = Vp9DecGetInfo(inst, &vp9_info);

    info->pic_width                    = vp9_info.frame_width;
    info->pic_height                   = vp9_info.frame_height;
    info->sar_width                    = 1;
    info->sar_height                   = 1;
    info->crop_params.crop_left_offset = 0;
    info->crop_params.crop_out_width   = vp9_info.coded_width;
    info->crop_params.crop_top_offset  = 0;
    info->crop_params.crop_out_height  = vp9_info.coded_height;
    /* TODO(vmr): Consider adding scaled_width & scaled_height. */
    /* TODO(vmr): output_format? */
    info->num_of_ref_frames   = vp9_info.pic_buff_size;
    info->video_range         = DEC_VIDEO_RANGE_NORMAL;
    info->matrix_coefficients = 0;
    info->is_mono_chrome      = 0;
    info->is_interlaced       = 0;
    info->bit_depth_luma = info->bit_depth_chroma = vp9_info.bit_depth;

    if (rv) {
        VPILOGD("Vp9DecGetInfo ret %s\n", dec_ret_string(rv));
        return VPI_ERR_UNKNOWN;
    }
    return VPI_SUCCESS;
}

VpiRet vpi_dec_vp9_set_info(VpiDecInst inst, struct DecConfig config,
                            struct DecSequenceInfo *Info)
{
    struct Vp9DecConfig dec_cfg;
    enum DecRet rv;

    dec_cfg.use_video_freeze_concealment = config.concealment_mode;
    dec_cfg.num_frame_buffers            = 9;
    dec_cfg.dpb_flags                    = 4;
    dec_cfg.use_video_compressor         = config.use_video_compressor;
    dec_cfg.use_ringbuffer               = config.use_ringbuffer;
    if (config.ppu_cfg[0].out_cut_8bits) {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_CUT_8BIT;
    } else if (config.ppu_cfg[0].out_p010) {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_P010;
    } else if (config.ppu_cfg[0].out_be) {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_CUSTOMER1;
    } else {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_DEFAULT;
    }

    memcpy(dec_cfg.ppu_cfg, config.ppu_cfg, sizeof(config.ppu_cfg));
    if (config.fscale_cfg.fixed_scale_enabled) {
        /* Convert fixed ratio scale to ppu_cfg[0] */
        dec_cfg.ppu_cfg[0].enabled = 1;
        if (!config.ppu_cfg[0].crop.enabled) {
            dec_cfg.ppu_cfg[0].crop.enabled = 1;
            dec_cfg.ppu_cfg[0].crop.x       = 0;
            dec_cfg.ppu_cfg[0].crop.y       = 0;
            dec_cfg.ppu_cfg[0].crop.width   = Info->pic_width;
            dec_cfg.ppu_cfg[0].crop.height  = Info->pic_height;
        }
        if (!config.ppu_cfg[0].scale.enabled) {
            dec_cfg.ppu_cfg[0].scale.enabled = 1;
            dec_cfg.ppu_cfg[0].scale.width =
                dec_cfg.ppu_cfg[0].crop.width / config.fscale_cfg.down_scale_x;
            dec_cfg.ppu_cfg[0].scale.height =
                dec_cfg.ppu_cfg[0].crop.height / config.fscale_cfg.down_scale_y;
        }
    }
    dec_cfg.output_format       = config.output_format;
    dec_cfg.align               = config.align;
    dec_cfg.fixed_scale_enabled = config.fscale_cfg.fixed_scale_enabled;
    /* TODO(min): assume 1-byte aligned is only applied for pp output */
    if (dec_cfg.align == DEC_ALIGN_1B) {
        dec_cfg.align = DEC_ALIGN_64B;
    }
    rv = Vp9DecSetInfo(inst, &dec_cfg);
    if (rv) {
        VPILOGD("Vp9DecSetInfo ret %s\n", dec_ret_string(rv));
        return VPI_ERR_UNKNOWN;
    }
    return VPI_SUCCESS;
}

static void parse_super_frame_index(const uint8_t *data, size_t data_sz,
                                    const uint8_t *buf, size_t buf_sz,
                                    uint32_t sizes[8], int32_t *count)
{
    uint8_t marker;
    uint8_t *buf_end = (uint8_t *)buf + buf_sz;

    if ((data + data_sz - 1) < buf_end) {
        marker = DWLPrivateAreaReadByte(data + data_sz - 1);
    } else {
        marker = DWLPrivateAreaReadByte(data + (int32_t)data_sz - 1 -
                                        (int32_t)buf_sz);
    }
    //marker = data[data_sz - 1];
    *count = 0;

    if ((marker & 0xe0) == 0xc0) {
        const uint32_t frames   = (marker & 0x7) + 1;
        const uint32_t mag      = ((marker >> 3) & 0x3) + 1;
        const uint32_t index_sz = 2 + mag * frames;
        uint8_t index_value;
        uint32_t turn_around = 0;
        if ((data + data_sz - index_sz) < buf_end) {
            index_value = DWLPrivateAreaReadByte(data + data_sz - index_sz);
        } else {
            index_value =
                DWLPrivateAreaReadByte(data + data_sz - index_sz - buf_sz);
            turn_around = 1;
        }

        if (data_sz >= index_sz && index_value == marker) {
            /* found a valid superframe index */
            uint32_t i, j;
            const uint8_t *x = data + data_sz - index_sz + 1;
            if (turn_around) {
                x = data + data_sz - index_sz + 1 - buf_sz;
            }

            for (i = 0; i < frames; i++) {
                uint32_t this_sz = 0;

                for (j = 0; j < mag; j++) {
                    if (x == buf + buf_sz) {
                        x = buf;
                    }
                    this_sz |= DWLPrivateAreaReadByte(x) << (j * 8);
                    x++;
                }
                sizes[i] = this_sz;
            }
            *count = frames;
        }
    }
}

enum DecRet vpi_dec_vp9_next_picture(VpiDecInst inst, struct DecPicturePpu *pic)
{
    enum DecRet rv;
    uint32_t stride, stride_ch, i;
    struct Vp9DecPicture vpic = { 0 };

    addr_t tile_status_bus_address        = 0;
    uint32_t tile_status_address_offset   = 0;

    rv = Vp9DecNextPicture(inst, &vpic);
    if (rv != DEC_PIC_RDY) {
        return rv;
    }

    memset(pic, 0, sizeof(struct DecPicturePpu));
    pic->pictures[0].luma_table.bus_address = vpic.output_rfc_luma_bus_address;
#ifndef NEW_MEM_ALLOC
    pic->pictures[0].luma_table.virtual_address = vpic.output_rfc_luma_base;
#endif
    pic->pictures[0].chroma_table.bus_address =
        vpic.output_rfc_chroma_bus_address;
#ifndef NEW_MEM_ALLOC
    pic->pictures[0].chroma_table.virtual_address = vpic.output_rfc_chroma_base;
#endif
    pic->pictures[0].pic_compressed_status = vpic.use_video_compressor ? 1 : 0;

    uint32_t pic_width_in_cbsy, pic_height_in_cbsy;
    uint32_t pic_width_in_cbsc, pic_height_in_cbsc;
    pic_width_in_cbsy  = (vpic.pictures[0].frame_width + 8 - 1) / 8;
    pic_width_in_cbsy  = NEXT_MULTIPLE(pic_width_in_cbsy, 16);
    pic_width_in_cbsc  = (vpic.pictures[0].frame_width + 16 - 1) / 16;
    pic_width_in_cbsc  = NEXT_MULTIPLE(pic_width_in_cbsc, 16);
    pic_height_in_cbsy = (vpic.pictures[0].frame_height + 8 - 1) / 8;
    pic_height_in_cbsc = (vpic.pictures[0].frame_height / 2 + 4 - 1) / 4;

    uint32_t tbl_sizey =
        NEXT_MULTIPLE(pic_width_in_cbsy * pic_height_in_cbsy, 16);
    uint32_t tbl_sizec =
        NEXT_MULTIPLE(pic_width_in_cbsc * pic_height_in_cbsc, 16);
    pic->pictures[0].luma_table.size   = tbl_sizey;
    pic->pictures[0].chroma_table.size = tbl_sizec;

    for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
        if (vpic.pictures[i].pixel_format == DEC_OUT_PIXEL_RFC) {
            /* Compressed tiled data should be output
             * without being converted to 16 bits.
             * It's treated as a special picture to output. */
            uint32_t bit_depth =
                (vpic.bit_depth_luma == 8 && vpic.bit_depth_luma == 8) ? 8 : 10;
            stride    = vpic.pictures[i].frame_width * 4 * bit_depth / 8;
            stride_ch = stride;
        } else {
            stride    = vpic.pictures[i].pic_stride;
            stride_ch = vpic.pictures[i].pic_stride_ch;
        }
#ifndef NEW_MEM_ALLOC
        pic->pictures[i].luma.virtual_address =
            (uint32_t *)vpic.pictures[i].output_luma_base;
#endif
        pic->pictures[i].luma.bus_address =
            vpic.pictures[i].output_luma_bus_address;
        if (vpic.pictures[i].output_format == DEC_OUT_FRM_TILED_4X4) {
            pic->pictures[i].luma.size =
                stride * NEXT_MULTIPLE(vpic.pictures[i].frame_height, 4) / 4;
            pic->pictures[i].chroma.size =
                stride_ch *
                NEXT_MULTIPLE(vpic.pictures[i].frame_height / 2, 4) / 4;
        } else {
            pic->pictures[i].luma.size = stride * vpic.pictures[i].frame_height;
            pic->pictures[i].chroma.size =
                stride_ch * vpic.pictures[i].frame_height;
        }
#ifndef NEW_MEM_ALLOC
        pic->pictures[i].chroma.virtual_address =
            (uint32_t *)vpic.pictures[i].output_chroma_base;
#endif
        pic->pictures[i].chroma.bus_address =
            vpic.pictures[i].output_chroma_bus_address;
        pic->pictures[i].chroma.size = pic->pictures[i].chroma.size;

        pic->pictures[i].pic_width     = vpic.pictures[i].frame_width;
        pic->pictures[i].pic_height    = vpic.pictures[i].frame_height;
        pic->pictures[i].pic_stride    = vpic.pictures[i].pic_stride;
        pic->pictures[i].pic_stride_ch = vpic.pictures[i].pic_stride_ch;

        /* TODO(vmr): find out for real also if it is B frame */
        pic->pictures[i].picture_info.pic_coding_type =
            vpic.is_intra_frame ? DEC_PIC_TYPE_I : DEC_PIC_TYPE_P;
        pic->pictures[i].sequence_info.pic_width = vpic.pictures[i].frame_width;
        pic->pictures[i].sequence_info.pic_height =
            vpic.pictures[i].frame_height;
        pic->pictures[i].sequence_info.sar_width                    = 1;
        pic->pictures[i].sequence_info.sar_height                   = 1;
        pic->pictures[i].sequence_info.crop_params.crop_left_offset = 0;
        pic->pictures[i].sequence_info.crop_params.crop_out_width =
            vpic.coded_width;
        pic->pictures[i].sequence_info.crop_params.crop_top_offset = 0;
        pic->pictures[i].sequence_info.crop_params.crop_out_height =
            vpic.coded_height;
        pic->pictures[i].sequence_info.video_range = DEC_VIDEO_RANGE_NORMAL;
        pic->pictures[i].sequence_info.matrix_coefficients = 0;
        pic->pictures[i].sequence_info.is_mono_chrome      = 0;
        pic->pictures[i].sequence_info.is_interlaced       = 0;
        pic->pictures[i].sequence_info.num_of_ref_frames =
            pic->pictures[i].sequence_info.num_of_ref_frames;
        pic->pictures[i].picture_info.format = vpic.pictures[i].output_format;
        pic->pictures[i].picture_info.pixel_format =
            vpic.pictures[i].pixel_format;
        pic->pictures[i].picture_info.pic_id            = vpic.pic_id;
        pic->pictures[i].picture_info.decode_id         = vpic.decode_id;
        pic->pictures[i].picture_info.cycles_per_mb     = vpic.cycles_per_mb;
        pic->pictures[i].sequence_info.bit_depth_luma   = vpic.bit_depth_luma;
        pic->pictures[i].sequence_info.bit_depth_chroma = vpic.bit_depth_chroma;
        pic->pictures[i].sequence_info.pic_stride = vpic.pictures[i].pic_stride;
        pic->pictures[i].sequence_info.pic_stride_ch =
            vpic.pictures[i].pic_stride_ch;
        pic->pictures[i].pp_enabled = vpic.pp_enabled;

        if ((vpic.multi_tile_cols == 0) && (i > 0) &&
            (pic->pictures[i].luma.size != 0)) {
            /*mark the dpb total buffer base address*/
            if (tile_status_bus_address == 0) {
#ifndef NEW_MEM_ALLOC
                tile_status_virtual_address =
                    pic->pictures[i].luma.virtual_address;
#endif
                tile_status_bus_address = pic->pictures[i].luma.bus_address;
            }

            tile_status_address_offset +=
                pic->pictures[i].luma.size + PP_LUMA_BUF_RES;
            tile_status_address_offset +=
                pic->pictures[i].chroma.size + PP_CHROMA_BUF_RES;
        }
    }
    for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
        if ((vpic.multi_tile_cols == 0) && (i > 0) &&
            (pic->pictures[i].luma.size != 0)) {
#ifndef NEW_MEM_ALLOC
            pic->pictures[i].luma_table.virtual_address =
                (uint32_t *)((addr_t)tile_status_virtual_address +
                             tile_status_address_offset +
                             DEC400_PPn_Y_TABLE_OFFSET(i - 1));
#endif
            pic->pictures[i].luma_table.bus_address =
                tile_status_bus_address + tile_status_address_offset +
                DEC400_PPn_Y_TABLE_OFFSET(i - 1);
            pic->pictures[i].luma_table.size =
                NEXT_MULTIPLE(pic->pictures[i].luma.size / 1024 / 4 +
                                  ((pic->pictures[i].luma.size % 4096) ? 1 : 0),
                              16);

#ifndef NEW_MEM_ALLOC
            pic->pictures[i].chroma_table.virtual_address =
                (uint32_t *)((addr_t)tile_status_virtual_address +
                             tile_status_address_offset +
                             DEC400_PPn_UV_TABLE_OFFSET(i - 1));
#endif
            pic->pictures[i].chroma_table.bus_address =
                tile_status_bus_address + tile_status_address_offset +
                DEC400_PPn_UV_TABLE_OFFSET(i - 1);
            pic->pictures[i].chroma_table.size =
                NEXT_MULTIPLE(pic->pictures[i].chroma.size / 1024 / 4 +
                                  ((pic->pictures[i].chroma.size % 4096) ? 1 :
                                                                           0),
                              16);

#ifdef SUPPORT_DEC400
            pic->pictures[i].pic_compressed_status =
                vpic.pictures[i].pic_compressed_status;
#else
            pic->pictures[i].pic_compressed_status = 0;
#endif
        }
    }
    return rv;
}

VpiRet vpi_dec_vp9_picture_consumed(VpiDecInst inst, struct DecPicturePpu pic)
{
    struct Vp9DecPicture vpic;
    uint32_t i;
    enum DecRet rv;

    memset(&vpic, 0, sizeof(struct Vp9DecPicture));
    /* TODO chroma base needed? */
    for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
#ifndef NEW_MEM_ALLOC
        vpic.pictures[i].output_luma_base =
            pic.pictures[i].luma.virtual_address;
#endif
        vpic.pictures[i].output_luma_bus_address =
            pic.pictures[i].luma.bus_address;
    }
    vpic.pp_enabled = pic.pictures[0].pp_enabled;
    vpic.is_intra_frame =
        pic.pictures[0].picture_info.pic_coding_type == DEC_PIC_TYPE_I;

    rv = Vp9DecPictureConsumed(inst, &vpic);
    if (rv) {
        VPILOGD("Vp9DecPictureConsumed ret %s\n", dec_ret_string(rv));
        return VPI_ERR_UNKNOWN;
    }
    return VPI_SUCCESS;
}

int vpi_decode_vp9_put_packet(VpiDecCtx *vpi_ctx, void *indata)
{
    VpiPacket *vpi_packet = (VpiPacket *)indata;
    int idx;
    int i;

    pthread_mutex_lock(&vpi_ctx->dec_thread_mutex);
    if (vpi_packet->size == 0) {
        if (vpi_ctx->eos_received) {
            pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
            return 0;
        }
        VPILOGD("received EOS\n");
    }

    idx = vpi_ctx->stream_mem_index;
    if (vpi_ctx->strm_buf_list[idx]->mem_idx != 0xFFFFFFFF) {
        pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
        return 0;
    }

    vpi_ctx->strm_buf_list[idx]->mem_idx   = vpi_ctx->stream_mem_index;
    vpi_ctx->strm_buf_list[idx]->item_size = vpi_packet->size;
    vpi_ctx->strm_buf_list[idx]->item      = (void *)vpi_packet->data;
    vpi_ctx->strm_buf_list[idx]->opaque    = vpi_packet->opaque;
    vpi_dec_buf_list_add(&vpi_ctx->strm_buf_head, vpi_ctx->strm_buf_list[idx]);

    for (i = 0; i < MAX_PTS_DTS_DEPTH; i++) {
        if (vpi_ctx->time_stamp_info[i].used == 0) {
            vpi_ctx->time_stamp_info[i].pts = vpi_packet->pts;
            vpi_ctx->time_stamp_info[i].pkt_dts = vpi_packet->pkt_dts;
            vpi_ctx->time_stamp_info[i].decode_id = vpi_ctx->got_package_number + 1;
            vpi_ctx->time_stamp_info[i].used = 1;
            break;
        }
    }
    vpi_ctx->got_package_number++;
    vpi_ctx->stream_mem_used[idx] = 1;

    vpi_ctx->stream_mem_index++;
    if (vpi_ctx->stream_mem_index == vpi_ctx->allocated_buffers) {
        vpi_ctx->stream_mem_index = 0;
    }


    if (vpi_packet->size == 0) {
        vpi_ctx->eos_received = 1;
    }
    pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
    return vpi_packet->size;
}

int vpi_decode_vp9_get_frame(VpiDecCtx *vpi_ctx, void *outdata)
{
    VpiFrame **out_frame;
    VpiFrame *vpi_frame = NULL;
    int i;
    int ret;

    pthread_mutex_lock(&vpi_ctx->dec_thread_mutex);
    if (vpi_ctx->dec_error == 1) {
        pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
        return 2;
    }

    vpi_ctx->max_frames_delay = vpi_ctx->frame->max_frames_delay;
    ret = vpi_dec_check_buffer_number_for_trans(vpi_ctx);
    if (ret == 1) {
        if (vpi_ctx->waiting_for_dpb == 1) {
            pthread_cond_signal(&vpi_ctx->dec_thread_cond);
            vpi_ctx->waiting_for_dpb = 0;
        }
    } else if (ret == -1) {
        pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
        return 2;
    }

    if (NULL == vpi_ctx->frame_buf_head) {
        if (vpi_ctx->last_pic_flag == 1) {
            ret = 2;
        } else {
            ret = 0;
        }
        pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
        return ret;
    }

    vpi_frame  = (VpiFrame *)vpi_ctx->frame_buf_head->item;
    out_frame  = (VpiFrame **)outdata;
    *out_frame = vpi_frame;
    if (vpi_ctx->output_num == 0) {
        // the first frame
        memcpy(vpi_ctx->frame, vpi_frame, sizeof(VpiFrame));
        vpi_ctx->output_num++;
    }
    for (i = 0; i < MAX_BUFFERS; i++) {
        if (vpi_ctx->frame_buf_head->mem_idx
            == vpi_ctx->frame_buf_list[i]->mem_idx) {
            vpi_ctx->frame_buf_list[i]->mem_idx = 0xFFFFFFFF;
            vpi_ctx->frame_buf_list[i]->used = 0;
            vpi_ctx->frame_buf_list[i]->item = NULL;
            break;
        }
    }
    vpi_ctx->frame_buf_head =
        vpi_dec_buf_list_delete(vpi_ctx->frame_buf_head);
    pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);

    return 1;
}

int vpi_decode_vp9_get_used_strm_mem(VpiDecCtx *vpi_ctx, void *mem)
{
    VpiBufRef **ref;

    ref = (VpiBufRef **)mem;
    if (vpi_ctx->rls_strm_buf_head) {
        *ref = (VpiBufRef *)vpi_ctx->rls_strm_buf_head->opaque;
        vpi_ctx->rls_strm_buf_head =
            vpi_dec_buf_list_delete(vpi_ctx->rls_strm_buf_head);
        return 0;
    } else {
        *ref = NULL;
        return -1;
    }
}

int vpi_decode_vp9_set_frame_buffer(VpiDecCtx *vpi_ctx, void *frame)
{
    VpiFrame *vpi_frame = (VpiFrame *)frame;
    int i;
    int ret = 0;

    pthread_mutex_lock(&vpi_ctx->dec_thread_mutex);
    for (i = 0; i < MAX_BUFFERS; i++) {
        if (vpi_ctx->frame_buf_list[i]->used == 0) {
            vpi_ctx->frame_buf_list[i]->used = 1;
            vpi_ctx->frame_buf_list[i]->item = frame;
            break;
        }
    }
    if (i == MAX_BUFFERS) {
        VPILOGE("no valid frame buffer to store buffer info\n");
        ret = -1;
    }
    vpi_frame->locked         = 1;
    pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
    return ret;
}

static enum DecRet vp9_decode(const void *inst, struct DWLLinearMem input,
                              struct DecOutput *output, uint8_t *stream,
                              uint32_t strm_len, uint32_t pic_id)
{
    enum DecRet rv;
    struct Vp9DecInput vp9_input;
    struct Vp9DecOutput vp9_output;

    memset(&vp9_input, 0, sizeof(vp9_input));
    memset(&vp9_output, 0, sizeof(vp9_output));
    vp9_input.stream = (uint8_t *)stream;
    vp9_input.stream_bus_address =
        input.bus_address + ((addr_t)stream - (addr_t)input.virtual_address);
    vp9_input.data_len           = strm_len;
    vp9_input.buffer             = (uint8_t *)input.virtual_address;
    vp9_input.buffer_bus_address = input.bus_address;
    vp9_input.buff_len           = strm_len;
    vp9_input.pic_id             = pic_id;
    static uint32_t sizes[8];
    static int32_t frames_this_pts, frame_count = 0;
    uint32_t data_sz          = vp9_input.data_len;
    uint32_t data_len         = vp9_input.data_len;
    uint32_t consumed_sz      = 0;
    const uint8_t *data_start = vp9_input.stream;
    const uint8_t *buf_end    = vp9_input.buffer + vp9_input.buff_len;

    /* TODO(vmr): vp9 must not acquire the resources automatically after
    *            successful header decoding. */

    /* TODO: Is this correct place to handle superframe indexes? */
    parse_super_frame_index(vp9_input.stream, data_sz, vp9_input.buffer,
                            input.size, sizes, &frames_this_pts);

    do {
        /* Skip over the superframe index, if present */
        if (data_sz && (DWLPrivateAreaReadByte(data_start) & 0xe0) == 0xc0) {
            const uint8_t marker    = DWLPrivateAreaReadByte(data_start);
            const uint32_t frames   = (marker & 0x7) + 1;
            const uint32_t mag      = ((marker >> 3) & 0x3) + 1;
            const uint32_t index_sz = 2 + mag * frames;
            uint8_t index_value;
            if (data_start + index_sz - 1 < buf_end) {
                index_value = DWLPrivateAreaReadByte(data_start + index_sz - 1);
            } else {
                index_value =
                    DWLPrivateAreaReadByte(data_start + (int32_t)index_sz - 1 -
                                           (int32_t)vp9_input.buff_len);
            }

            if (data_sz >= index_sz && index_value == marker) {
                data_start += index_sz;
                if (data_start >= buf_end) {
                    data_start -= vp9_input.buff_len;
                }
                consumed_sz += index_sz;
                data_sz -= index_sz;
                if (consumed_sz < data_len) {
                    continue;
                } else {
                    frames_this_pts = 0;
                    frame_count     = 0;
                    break;
                }
            }
        }

        /* Use the correct size for this frame, if an index is present. */
        if (frames_this_pts) {
            uint32_t this_sz = sizes[frame_count];

            if (data_sz < this_sz) {
                return DEC_STRM_ERROR;
            }

            data_sz = this_sz;
            // frame_count++;
        }

        vp9_input.stream_bus_address = input.bus_address + (addr_t)data_start -
                                       (addr_t)input.virtual_address;
        vp9_input.stream   = (uint8_t *)data_start;
        vp9_input.data_len = data_sz;
        do {
            rv = Vp9DecDecode(inst, &vp9_input, &vp9_output);
            if (rv == DEC_NO_DECODING_BUFFER) {
                usleep(10);
            }
        } while (rv == DEC_NO_DECODING_BUFFER);
        /* Headers decoded or error occurred */
        if (rv == DEC_HDRS_RDY || rv != DEC_PIC_DECODED) {
            break;
        } else if (frames_this_pts) {
            frame_count++;
        }

        data_start += data_sz;
        consumed_sz += data_sz;
        if (data_start >= buf_end) {
            data_start -= vp9_input.buff_len;
        }

        /* Account for suboptimal termination by the encoder. */
        while (consumed_sz < data_len && *data_start == 0) {
            data_start++;
            consumed_sz++;
            if (data_start >= buf_end) {
                data_start -= vp9_input.buff_len;
            }
        }

        data_sz = data_len - consumed_sz;

    } while (consumed_sz < data_len);

    /* TODO(vmr): output is complete garbage on on VP9.
     * Fix in the code decoding the frame headers. */
    switch (rv) {
    case DEC_HDRS_RDY:
        output->strm_curr_pos         = (uint8_t *)vp9_input.stream;
        output->strm_curr_bus_address = vp9_input.stream_bus_address;
        output->data_left             = vp9_input.data_len;
        break;
#ifdef USE_EXTERNAL_BUFFER
    case DEC_WAITING_FOR_BUFFER:
        output->strm_curr_pos         = (uint8_t *)vp9_input.stream;
        output->strm_curr_bus_address = vp9_input.stream_bus_address;
        output->data_left             = data_len - consumed_sz;
        break;
#endif
    default:
        if ((vp9_input.stream + vp9_input.data_len) >= buf_end) {
            output->strm_curr_pos =
                (uint8_t *)(vp9_input.stream + vp9_input.data_len -
                            vp9_input.buff_len);
            output->strm_curr_bus_address = vp9_input.stream_bus_address +
                                            vp9_input.data_len -
                                            vp9_input.buff_len;
        } else {
            output->strm_curr_pos =
                (uint8_t *)(vp9_input.stream + vp9_input.data_len);
            output->strm_curr_bus_address =
                vp9_input.stream_bus_address + vp9_input.data_len;
        }
        output->data_left = 0;
        break;
    }
    return rv;
}

int vpi_decode_vp9_dec_frame(VpiDecCtx *vpi_ctx, void *indata, void *outdata)
{
    VpiPacket *vpi_packet = (VpiPacket *)indata;
    VpiFrame *vpi_frame   = (VpiFrame *)outdata;
    enum DecRet ret;
    VpiRet vpi_ret;
    int i;

    vpi_ctx->pic_rdy = 0;

    if (vpi_packet->size == 0) {
        VPILOGD("received EOS\n");
        Vp9DecEndOfStream(vpi_ctx->dec_inst);

        ret = vpi_dec_vp9_next_picture(vpi_ctx->dec_inst, &vpi_ctx->pic);
        VPILOGD("vpi_dec_vp9_next_picture ret %d\n", ret);
        if (ret == DEC_PIC_RDY) {
            for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
                VPILOGD("DEC_PIC_RDY pic %d -> %d x %d\n", i,
                        vpi_ctx->pic.pictures[i].pic_width,
                        vpi_ctx->pic.pictures[i].pic_height);
            }
            vpi_ctx->pic_rdy = 1;
            vpi_dec_output_frame(vpi_ctx, vpi_frame, &vpi_ctx->pic);

            vpi_ctx->pic_display_number++;
            VPILOGD("******** %d got frame :data[0]=%p,data[1]=%p\n",
                    vpi_ctx->pic_display_number, vpi_frame->data[0],
                    vpi_frame->data[1]);
            vpi_ctx->pic_rdy = 0;
            return 1;
        } else if (ret == DEC_END_OF_STREAM) {
            vpi_ctx->last_pic_flag = 1;
            VPILOGD("END-OF-STREAM received in output thread\n");
            return 0;
        }
        return 0;
    }

    vpi_ctx->got_package_number++;
    vpi_send_packet_to_decode_buffer(vpi_ctx, vpi_packet,
                                     vpi_ctx
                                         ->stream_mem[vpi_ctx->stream_mem_index]);
    vpi_ctx->vp9_dec_input.stream =
        (uint8_t *)vpi_ctx->stream_mem[vpi_ctx->stream_mem_index]
            .virtual_address;
    vpi_ctx->vp9_dec_input.stream_bus_address =
        vpi_ctx->stream_mem[vpi_ctx->stream_mem_index].bus_address;
    vpi_ctx->vp9_dec_input.data_len = vpi_packet->size;

    if (vpi_dec_check_buffer_number_for_trans(vpi_ctx) == -1)
        return -1;
    do {
        vpi_ctx->vp9_dec_input.pic_id = vpi_ctx->pic_decode_number;

        ret = vp9_decode(vpi_ctx->dec_inst,
                         vpi_ctx->stream_mem[vpi_ctx->stream_mem_index],
                         &vpi_ctx->dec_output, vpi_ctx->vp9_dec_input.stream,
                         vpi_ctx->vp9_dec_input.data_len,
                         vpi_ctx->pic_decode_number);

        VPILOGD("%d Vp9DecDecode ret=%d\n", vpi_ctx->got_package_number, ret);
        print_decode_return(ret);
        switch (ret) {
        case DEC_STREAM_NOT_SUPPORTED: {
            VPILOGE("ERROR: UNSUPPORTED STREAM!\n");
            return -1;
        }
        case DEC_HDRS_RDY: {
            /* Stream headers were successfully decoded
                 * -> stream information is available for query now */

            START_SW_PERFORMANCE;
            //tmp = Vp9DecGetInfo(vpi_ctx->dec_inst, &dec_info);
            vpi_ret =
                vpi_ctx->vpi_dec_wrapper.get_info(vpi_ctx->dec_inst,
                                                  &vpi_ctx->sequence_info);
            END_SW_PERFORMANCE;
            if (vpi_ret != VPI_SUCCESS) {
                VPILOGE("ERROR in getting stream info!\n");
                return -1;
            }
            if (vpi_dec_cfg_by_seqeuence_info(vpi_ctx)) {
                VPILOGE("set dec cfg fail\n");
                return -1;
            }
            vpi_ret =
                vpi_ctx->vpi_dec_wrapper.set_info(vpi_ctx->dec_inst,
                                                  vpi_ctx->vpi_dec_config,
                                                  &vpi_ctx->sequence_info);
#ifdef USE_EXTERNAL_BUFFER
            if (vpi_ctx->sequence_info.num_of_ref_frames !=
                    vpi_ctx->min_buffer_num ||
                (vpi_ctx->sequence_info.pic_width *
                     vpi_ctx->sequence_info.pic_height >
                 vpi_ctx->prev_width * vpi_ctx->prev_height)) {
                /* Reset buffers added and stop adding extra buffers
                     * when a new header comes. */
                vpi_dec_release_ext_buffers(vpi_ctx);
                vpi_ctx->num_buffers    = 0;
                //vpi_ctx->add_extra_flag = 0;
                //vpi_ctx->extra_buffer_num = 0;
            }

            vpi_ret =
                Vp9DecGetBufferInfo(vpi_ctx->dec_inst, &vpi_ctx->vp9_hbuf);
            VPILOGD("Vp9DecGetBufferInfo ret %d\n", vpi_ret);
            VPILOGD("buf_to_free %p, next_buf_size %d, buf_num %d\n",
                    (void *)vpi_ctx->vp9_hbuf.buf_to_free.virtual_address,
                    vpi_ctx->vp9_hbuf.next_buf_size, vpi_ctx->vp9_hbuf.buf_num);
#endif
            vpi_ctx->prev_width     = vpi_ctx->sequence_info.pic_width;
            vpi_ctx->prev_height    = vpi_ctx->sequence_info.pic_height;
            vpi_ctx->min_buffer_num = vpi_ctx->sequence_info.num_of_ref_frames;

            /* Decoder output frame size in planar YUV 4:2:0 */
            vpi_ctx->pic_size = vpi_ctx->sequence_info.pic_width *
                                vpi_ctx->sequence_info.pic_height;
            vpi_ctx->pic_size = (3 * vpi_ctx->pic_size) / 2;

            /* No data consumed when returning DEC_HDRS_RDY. */
            vpi_ctx->dec_output.data_left     = vpi_ctx->vp9_dec_input.data_len;
            vpi_ctx->dec_output.strm_curr_pos = vpi_ctx->vp9_dec_input.stream;

            break;
        }
        case DEC_ADVANCED_TOOLS: {
            /* ASO/STREAM ERROR was noticed in the stream.
                 * The decoder has to reallocate resources */
            assert(vpi_ctx->dec_output.data_left);
            /* we should have some data left */
            /* Used to indicate that picture decoding needs to finalized
                 * prior to corrupting next picture */
            break;
        }
        case DEC_PIC_DECODED:
            /* If enough pictures decoded -> force decoding to end
                 * by setting that no more stream is available */
            /* Increment decoding number for every decoded picture */
            vpi_ctx->pic_decode_number++;
            vpi_ctx->dec_output.data_left = 0;
        case DEC_PENDING_FLUSH:
            /* case DEC_FREEZED_PIC_RDY: */
            /* Picture is now ready */
            vpi_ctx->pic_rdy = 1;
            /* use function Vp9DecNextPicture() to obtain next picture
                 * in display order. Function is called until no more images
                 * are ready for display */
            vpi_ctx->retry = 0;
            break;
        case DEC_STRM_PROCESSED:
        case DEC_NONREF_PIC_SKIPPED:
        case DEC_STRM_ERROR:
            /* Used to indicate that picture decoding needs to finalized
                 * prior to corrupting next picture
                 * pic_rdy = 0; */
            break;
        case DEC_WAITING_FOR_BUFFER: {
#ifdef USE_EXTERNAL_BUFFER
            if (vpi_dec_check_buffer_number_for_trans(vpi_ctx) == -1)
                return -1;
#endif
            break;
        }
        case DEC_OK:
            /* nothing to do, just call again */
            break;
        case DEC_HW_TIMEOUT:
            VPILOGD("Timeout\n");
            return -1;
        default:
            VPILOGD("FATAL ERROR: %d\n", ret);
            return -1;
        }
    } while (vpi_ctx->dec_output.data_left);

    ret = vpi_dec_vp9_next_picture(vpi_ctx->dec_inst, &vpi_ctx->pic);
    VPILOGD("vpi_dec_vp9_next_picture ret %d\n", ret);
    if (ret) {
        print_decode_return(ret);
    }

    vpi_ctx->stream_mem_index++;
    if (vpi_ctx->stream_mem_index == vpi_ctx->allocated_buffers) {
        vpi_ctx->stream_mem_index = 0;
    }
    if (ret == DEC_PIC_RDY) {
        for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
            VPILOGD("DEC_PIC_RDY pic %d -> %d x %d,luma_bus = 0x%x\n", i,
                    vpi_ctx->pic.pictures[i].pic_width,
                    vpi_ctx->pic.pictures[i].pic_height,
                    vpi_ctx->pic.pictures[i].luma.bus_address);
        }

        vpi_ctx->pts     = vpi_packet->pts;
        vpi_ctx->pkt_dts = vpi_packet->pkt_dts;
        vpi_dec_output_frame(vpi_ctx, vpi_frame, &vpi_ctx->pic);
        vpi_ctx->pic_display_number++;
        VPILOGD("******** %d got frame :data[0]=%p,data[1]=%p\n",
                vpi_ctx->pic_display_number, vpi_frame->data[0],
                vpi_frame->data[1]);
        return 1;
    } else if (ret == DEC_END_OF_STREAM) {
        vpi_ctx->last_pic_flag = 1;
        VPILOGD("END-OF-STREAM received in output thread\n");
        return 0;
    }

    return vpi_packet->size - vpi_ctx->dec_output.data_left;
}

static enum DecRet vp9_decode_process(VpiDecCtx *vpi_ctx, const void *inst,
                                      struct DWLLinearMem input,
                                      struct DecOutput *output, uint8_t *stream,
                                      uint32_t strm_len, uint32_t pic_id)
{
    enum DecRet rv;
    struct Vp9DecInput vp9_input;
    struct Vp9DecOutput vp9_output;

    memset(&vp9_input, 0, sizeof(vp9_input));
    memset(&vp9_output, 0, sizeof(vp9_output));
    vp9_input.stream = (uint8_t *)stream;
    vp9_input.stream_bus_address =
        input.bus_address + ((addr_t)stream - (addr_t)input.virtual_address);
    vp9_input.data_len           = strm_len;
    vp9_input.buffer             = (uint8_t *)input.virtual_address;
    vp9_input.buffer_bus_address = input.bus_address;
    vp9_input.buff_len           = strm_len;
    vp9_input.pic_id             = pic_id;
    static uint32_t sizes[8];
    static int32_t frames_this_pts, frame_count = 0;
    uint32_t data_sz          = vp9_input.data_len;
    uint32_t data_len         = vp9_input.data_len;
    uint32_t consumed_sz      = 0;
    const uint8_t *data_start = vp9_input.stream;
    const uint8_t *buf_end    = vp9_input.buffer + vp9_input.buff_len;

    /* TODO(vmr): vp9 must not acquire the resources automatically after
    *            successful header decoding. */

    /* TODO: Is this correct place to handle superframe indexes? */
    parse_super_frame_index(vp9_input.stream, data_sz, vp9_input.buffer,
                            input.size, sizes, &frames_this_pts);

    do {
        /* Skip over the superframe index, if present */
        if (data_sz && (DWLPrivateAreaReadByte(data_start) & 0xe0) == 0xc0) {
            const uint8_t marker    = DWLPrivateAreaReadByte(data_start);
            const uint32_t frames   = (marker & 0x7) + 1;
            const uint32_t mag      = ((marker >> 3) & 0x3) + 1;
            const uint32_t index_sz = 2 + mag * frames;
            uint8_t index_value;
            if (data_start + index_sz - 1 < buf_end) {
                index_value = DWLPrivateAreaReadByte(data_start + index_sz - 1);
            } else {
                index_value =
                    DWLPrivateAreaReadByte(data_start + (int32_t)index_sz - 1 -
                                           (int32_t)vp9_input.buff_len);
            }

            if (data_sz >= index_sz && index_value == marker) {
                data_start += index_sz;
                if (data_start >= buf_end) {
                    data_start -= vp9_input.buff_len;
                }
                consumed_sz += index_sz;
                data_sz -= index_sz;
                if (consumed_sz < data_len) {
                    continue;
                } else {
                    frames_this_pts = 0;
                    frame_count     = 0;
                    break;
                }
            }
        }

        /* Use the correct size for this frame, if an index is present. */
        if (frames_this_pts) {
            uint32_t this_sz = sizes[frame_count];

            if (data_sz < this_sz) {
                return DEC_STRM_ERROR;
            }

            data_sz = this_sz;
            // frame_count++;
        }

        vp9_input.stream_bus_address = input.bus_address + (addr_t)data_start -
                                       (addr_t)input.virtual_address;
        vp9_input.stream   = (uint8_t *)data_start;
        vp9_input.data_len = data_sz;

        do {
            rv = Vp9DecDecode(inst, &vp9_input, &vp9_output);
            if (rv == DEC_NO_DECODING_BUFFER) {
                // waiting for release dpb buffer
                vpi_ctx->waiting_for_dpb = 1;
                VPILOGD("vp9 waiting for buffers\n");
                pthread_cond_wait(&vpi_ctx->dec_thread_cond,
                                  &vpi_ctx->dec_thread_mutex);
            }
        } while (rv == DEC_NO_DECODING_BUFFER);
        /* Headers decoded or error occurred */
        if (rv == DEC_HDRS_RDY || rv != DEC_PIC_DECODED) {
            break;
        } else if (frames_this_pts) {
            frame_count++;
        }

        data_start += data_sz;
        consumed_sz += data_sz;
        if (data_start >= buf_end) {
            data_start -= vp9_input.buff_len;
        }

        /* Account for suboptimal termination by the encoder. */
        while (consumed_sz < data_len && *data_start == 0) {
            data_start++;
            consumed_sz++;
            if (data_start >= buf_end) {
                data_start -= vp9_input.buff_len;
            }
        }

        data_sz = data_len - consumed_sz;

    } while (consumed_sz < data_len);

    /* TODO(vmr): output is complete garbage on on VP9.
     * Fix in the code decoding the frame headers. */
    switch (rv) {
    case DEC_HDRS_RDY:
        output->strm_curr_pos         = (uint8_t *)vp9_input.stream;
        output->strm_curr_bus_address = vp9_input.stream_bus_address;
        output->data_left             = vp9_input.data_len;
        break;
#ifdef USE_EXTERNAL_BUFFER
    case DEC_WAITING_FOR_BUFFER:
        output->strm_curr_pos         = (uint8_t *)vp9_input.stream;
        output->strm_curr_bus_address = vp9_input.stream_bus_address;
        output->data_left             = data_len - consumed_sz;
        break;
#endif
    default:
        if ((vp9_input.stream + vp9_input.data_len) >= buf_end) {
            output->strm_curr_pos =
                (uint8_t *)(vp9_input.stream + vp9_input.data_len -
                            vp9_input.buff_len);
            output->strm_curr_bus_address = vp9_input.stream_bus_address +
                                            vp9_input.data_len -
                                            vp9_input.buff_len;
        } else {
            output->strm_curr_pos =
                (uint8_t *)(vp9_input.stream + vp9_input.data_len);
            output->strm_curr_bus_address =
                vp9_input.stream_bus_address + vp9_input.data_len;
        }
        output->data_left = 0;
        break;
    }
    return rv;
}

static int vpi_decode_vp9_frame_decoding(VpiDecCtx *vpi_ctx)
{
    enum DecRet ret;
    VpiRet vpi_ret;

    do {
        vpi_ctx->vp9_dec_input.pic_id = vpi_ctx->pic_decode_number;

        ret = vp9_decode_process(vpi_ctx, vpi_ctx->dec_inst,
                                 vpi_ctx->stream_mem[vpi_ctx->strm_buf_head->mem_idx],
                                 &vpi_ctx->dec_output,
                                 vpi_ctx->vp9_dec_input.stream,
                                 vpi_ctx->vp9_dec_input.data_len,
                                 vpi_ctx->pic_decode_number);

        VPILOGD("%d Vp9DecDecode ret=%d\n", vpi_ctx->got_package_number, ret);
        print_decode_return(ret);
        switch (ret) {
        case DEC_STREAM_NOT_SUPPORTED: {
            VPILOGE("ERROR: UNSUPPORTED STREAM!\n");
            return -1;
        }
        case DEC_HDRS_RDY: {
            /* Stream headers were successfully decoded
                 * -> stream information is available for query now */

            START_SW_PERFORMANCE;
            vpi_ret =
                vpi_ctx->vpi_dec_wrapper.get_info(vpi_ctx->dec_inst,
                                                  &vpi_ctx->sequence_info);
            END_SW_PERFORMANCE;
            if (vpi_ret != VPI_SUCCESS) {
                VPILOGE("ERROR in getting stream info!\n");
                return -1;
            }
            if (vpi_dec_cfg_by_seqeuence_info(vpi_ctx)) {
                VPILOGE("set dec cfg fail\n");
                return -1;
            }
            vpi_ret =
                vpi_ctx->vpi_dec_wrapper.set_info(vpi_ctx->dec_inst,
                                                  vpi_ctx->vpi_dec_config,
                                                  &vpi_ctx->sequence_info);
#ifdef USE_EXTERNAL_BUFFER
            if (vpi_ctx->sequence_info.num_of_ref_frames !=
                    vpi_ctx->min_buffer_num ||
                (vpi_ctx->sequence_info.pic_width *
                     vpi_ctx->sequence_info.pic_height >
                 vpi_ctx->prev_width * vpi_ctx->prev_height)) {
                /* Reset buffers added and stop adding extra buffers
                     * when a new header comes. */
                vpi_dec_release_ext_buffers(vpi_ctx);
                vpi_ctx->num_buffers    = 0;
            }

            vpi_ret =
                Vp9DecGetBufferInfo(vpi_ctx->dec_inst, &vpi_ctx->vp9_hbuf);
            VPILOGD("Vp9DecGetBufferInfo ret %d\n", vpi_ret);
            VPILOGD("buf_to_free %p, next_buf_size %d, buf_num %d\n",
                    (void *)vpi_ctx->vp9_hbuf.buf_to_free.virtual_address,
                    vpi_ctx->vp9_hbuf.next_buf_size, vpi_ctx->vp9_hbuf.buf_num);
#endif
            vpi_ctx->prev_width     = vpi_ctx->sequence_info.pic_width;
            vpi_ctx->prev_height    = vpi_ctx->sequence_info.pic_height;
            vpi_ctx->min_buffer_num = vpi_ctx->sequence_info.num_of_ref_frames;

            /* Decoder output frame size in planar YUV 4:2:0 */
            vpi_ctx->pic_size = vpi_ctx->sequence_info.pic_width *
                                vpi_ctx->sequence_info.pic_height;
            vpi_ctx->pic_size = (3 * vpi_ctx->pic_size) / 2;

            /* No data consumed when returning DEC_HDRS_RDY. */
            vpi_ctx->dec_output.data_left     = vpi_ctx->vp9_dec_input.data_len;
            vpi_ctx->dec_output.strm_curr_pos = vpi_ctx->vp9_dec_input.stream;

            break;
        }
        case DEC_ADVANCED_TOOLS: {
            /* ASO/STREAM ERROR was noticed in the stream.
                 * The decoder has to reallocate resources */
            assert(vpi_ctx->dec_output.data_left);
            /* we should have some data left */
            /* Used to indicate that picture decoding needs to finalized
                 * prior to corrupting next picture */
            break;
        }
        case DEC_PIC_DECODED:
            /* If enough pictures decoded -> force decoding to end
                 * by setting that no more stream is available */
            /* Increment decoding number for every decoded picture */
            vpi_ctx->pic_decode_number++;
            vpi_ctx->dec_output.data_left = 0;
        case DEC_PENDING_FLUSH:
            /* case DEC_FREEZED_PIC_RDY: */
            /* Picture is now ready */
            vpi_ctx->pic_rdy = 1;
            /* use function Vp9DecNextPicture() to obtain next picture
                 * in display order. Function is called until no more images
                 * are ready for display */
            vpi_ctx->retry = 0;
            break;
        case DEC_STRM_PROCESSED:
        case DEC_NONREF_PIC_SKIPPED:
        case DEC_STRM_ERROR:
            /* Used to indicate that picture decoding needs to finalized
                 * prior to corrupting next picture
                 * pic_rdy = 0; */
            break;
        case DEC_WAITING_FOR_BUFFER: {
#ifdef USE_EXTERNAL_BUFFER
            if (vpi_dec_check_buffer_number_for_trans(vpi_ctx) == -1)
                return -1;
#endif
            break;
        }
        case DEC_OK:
            /* nothing to do, just call again */
            break;
        case DEC_HW_TIMEOUT:
            VPILOGD("Timeout\n");
            return -1;
        default:
            VPILOGD("FATAL ERROR: %d\n", ret);
            return -1;
        }
    } while (vpi_ctx->dec_output.data_left);

    return 0;
}

int vpi_decode_vp9_dec_process(VpiDecCtx *vpi_ctx)
{
    VpiFrame *vpi_frame        = NULL;
    VpiPacket vpi_packet       = {0};
    int ret;
    int i;

    pthread_mutex_lock(&vpi_ctx->dec_thread_mutex);
    if (NULL == vpi_ctx->strm_buf_head && 0 == vpi_ctx->eos_received) {
        pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
        return 0;
    }

    if (vpi_ctx->eos_received == 1) {
        if (vpi_ctx->strm_buf_head && vpi_ctx->strm_buf_head->item_size == 0) {
            // The last frame packet
            Vp9DecEndOfStream(vpi_ctx->dec_inst);
            vpi_ctx->stream_mem_used[vpi_ctx->strm_buf_head->mem_idx] = 0;
            for (i = 0; i < vpi_ctx->allocated_buffers; i++) {
                if (vpi_ctx->strm_buf_list[i]->mem_idx ==
                    vpi_ctx->strm_buf_head->mem_idx) {
                    vpi_ctx->strm_buf_list[i]->mem_idx = 0xFFFFFFFF;
                    break;
                }
            }
            vpi_ctx->strm_buf_head =
                vpi_dec_buf_list_delete(vpi_ctx->strm_buf_head);
            vpi_ctx->eos_handled = 1;
        }
        if (vpi_ctx->eos_handled == 1) {
            ret = vpi_dec_vp9_next_picture(vpi_ctx->dec_inst, &vpi_ctx->pic);
            VPILOGD("vpi_dec_vp9_next_picture ret %d\n", ret);
            if (ret == DEC_PIC_RDY) {
                for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
                    VPILOGD("DEC_PIC_RDY pic %d -> %d x %d\n", i,
                        vpi_ctx->pic.pictures[i].pic_width,
                        vpi_ctx->pic.pictures[i].pic_height);
                }
                for (i = 0; i < MAX_BUFFERS; i++) {
                    if (vpi_ctx->frame_buf_list[i]->used == 1 &&
                        vpi_ctx->frame_buf_list[i]->mem_idx == 0xFFFFFFFF) {
                        vpi_ctx->frame_buf_list[i]->mem_idx = i;
                        break;
                    }
                }
                if (i == MAX_BUFFERS) {
                    // This case should not happen
                    VPILOGE("All frame buffer used out\n");
                    pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
                    return -1;
                }
                vpi_frame = (VpiFrame *)vpi_ctx->frame_buf_list[i]->item;
                vpi_frame->used_cnt = 0;
                vpi_frame->nb_outputs = 1;
                vpi_ctx->pic_rdy = 1;
                vpi_dec_output_frame(vpi_ctx, vpi_frame, &vpi_ctx->pic);
                vpi_dec_buf_list_add(&vpi_ctx->frame_buf_head,
                    vpi_ctx->frame_buf_list[i]);
                vpi_ctx->pic_display_number++;
                VPILOGD("******** %d got frame :data[0]=%p,data[1]=%p\n",
                    vpi_ctx->pic_display_number, vpi_frame->data[0],
                    vpi_frame->data[1]);
                vpi_ctx->pic_rdy = 0;
            } else if (ret == DEC_END_OF_STREAM) {
                vpi_ctx->last_pic_flag = 1;
                VPILOGD("END-OF-STREAM received in output thread\n");
                vpi_ctx->dec_thread_finish = 1;

            }
            pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
            return 0;
        }
    }

    vpi_packet.data = (uint8_t *)vpi_ctx->strm_buf_head->item;
    vpi_packet.size = vpi_ctx->strm_buf_head->item_size;
    if (vpi_ctx->stream_mem[vpi_ctx->strm_buf_head->mem_idx].logical_size <
        vpi_packet.size) {
        VPILOGE("packet size %d is larger than stream mem size %d\n",
                vpi_packet.size,
                vpi_ctx->stream_mem[vpi_ctx->strm_buf_head->mem_idx].logical_size);
        vpi_packet.size =
            vpi_ctx->stream_mem[vpi_ctx->strm_buf_head->mem_idx].logical_size;
    }
    vpi_send_packet_to_decode_buffer(vpi_ctx, &vpi_packet,
                        vpi_ctx->stream_mem[vpi_ctx->strm_buf_head->mem_idx]);
    vpi_ctx->stream_mem[vpi_ctx->strm_buf_head->mem_idx].virtual_address =
        (uint32_t *)vpi_packet.data;

    vpi_ctx->vp9_dec_input.stream =
        (uint8_t *)vpi_ctx->stream_mem[vpi_ctx->strm_buf_head->mem_idx]
            .virtual_address;
    vpi_ctx->vp9_dec_input.stream_bus_address =
        vpi_ctx->stream_mem[vpi_ctx->strm_buf_head->mem_idx].bus_address;
    vpi_ctx->vp9_dec_input.data_len = vpi_ctx->strm_buf_head->item_size;
    VPILOGD("decoding stream size %d\n", vpi_ctx->vp9_dec_input.data_len);

    if (vpi_dec_check_buffer_number_for_trans(vpi_ctx) == -1)
        return -1;
    ret = vpi_decode_vp9_frame_decoding(vpi_ctx);
    if (ret != 0) {
        VPILOGE("VP9 decoding failure\n");
        vpi_ctx->dec_error = 1;
        pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
        return -1;
    }

    if (vpi_ctx->dec_output.data_left == 0) {
        int idx = vpi_ctx->rls_mem_index;

        vpi_ctx->stream_mem_used[vpi_ctx->strm_buf_head->mem_idx] = 0;
        VPILOGD("release stream_mem_index %d\n", vpi_ctx->strm_buf_head->mem_idx);
        vpi_ctx->rls_strm_buf_list[idx]->mem_idx = vpi_ctx->rls_mem_index;
        vpi_ctx->rls_strm_buf_list[idx]->item    = vpi_ctx->strm_buf_head->item;
        vpi_ctx->rls_strm_buf_list[idx]->opaque  = vpi_ctx->strm_buf_head->opaque;
        vpi_dec_buf_list_add(&vpi_ctx->rls_strm_buf_head,
                             vpi_ctx->rls_strm_buf_list[idx]);
        vpi_ctx->rls_mem_index++;
        if (vpi_ctx->rls_mem_index == 32) {
            vpi_ctx->rls_mem_index = 0;
        }
        for (i = 0; i < vpi_ctx->allocated_buffers; i++) {
            if (vpi_ctx->strm_buf_list[i]->mem_idx ==
                vpi_ctx->strm_buf_head->mem_idx) {
                vpi_ctx->strm_buf_list[i]->mem_idx = 0xFFFFFFFF;
                break;
            }
        }
        vpi_ctx->strm_buf_head =
                vpi_dec_buf_list_delete(vpi_ctx->strm_buf_head);
        ret = 0;
    } else {
        ret = -1;
    }

    ret = vpi_dec_vp9_next_picture(vpi_ctx->dec_inst, &vpi_ctx->pic);
    VPILOGD("vpi_dec_vp9_next_picture ret = %d\n", ret);
    if (ret) {
        VPILOGD("vp9_next_picture: %d\n", ret);
        print_decode_return(ret);
    }

    VPILOGD("in vp9_decode_frame return:vpi_packet->size = %d.........\n",
            vpi_packet.size);

    if(ret == DEC_PIC_RDY) {
        for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
            VPILOGD("DEC_PIC_RDY pic %d -> %d x %d\n",
                    i, vpi_ctx->pic.pictures[i].pic_width,
                    vpi_ctx->pic.pictures[i].pic_height);
        }

        for (i = 0; i < MAX_BUFFERS; i++) {
            if (vpi_ctx->frame_buf_list[i]->used == 1 &&
                vpi_ctx->frame_buf_list[i]->mem_idx == 0xFFFFFFFF) {
                vpi_ctx->frame_buf_list[i]->mem_idx = i;
                break;
            }
        }
        if (i == MAX_BUFFERS) {
            // This case should not happen
            VPILOGE("All frame buffer used out\n");
            pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
            return -1;
        }
        vpi_frame = (VpiFrame *)vpi_ctx->frame_buf_list[i]->item;
        vpi_ctx->pic_rdy = 1;
        vpi_frame->nb_outputs = 1;
        vpi_frame->used_cnt = 0;
        vpi_dec_output_frame(vpi_ctx, vpi_frame, &vpi_ctx->pic);
        vpi_dec_buf_list_add(&vpi_ctx->frame_buf_head,
            vpi_ctx->frame_buf_list[i]);
        vpi_ctx->pic_display_number++;
        VPILOGD("******** %d got frame :data[0]=%p,data[1]=%p\n",
            vpi_ctx->pic_display_number,
            vpi_frame->data[0], vpi_frame->data[1]);
    } else if(ret == DEC_END_OF_STREAM) {
        vpi_ctx->last_pic_flag = 1;
        VPILOGD("END-OF-STREAM received\n");
        vpi_ctx->dec_thread_finish = 1;
    }

    pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
    return 1;
}

VpiRet vpi_dec_vp9_end_of_stream(VpiDecInst inst)
{
    enum DecRet rv;

    rv = Vp9DecEndOfStream(inst);
    if (rv) {
        VPILOGD("Vp9DecEndOfStream ret %s\n", dec_ret_string(rv));
        return VPI_ERR_UNKNOWN;
    }
    return VPI_SUCCESS;
}

#ifdef USE_EXTERNAL_BUFFER
enum DecRet vpi_dec_vp9_get_buffer_info(VpiDecInst inst,
                                   struct DecBufferInfo *buf_info)
{
    struct Vp9DecBufferInfo vbuf;
    enum DecRet rv;

    rv = Vp9DecGetBufferInfo(inst, &vbuf);
    if (rv == DEC_OK) {
        memset(buf_info, 0, sizeof(*buf_info));
    } else {
        buf_info->buf_to_free   = vbuf.buf_to_free;
        buf_info->next_buf_size = vbuf.next_buf_size;
        buf_info->buf_num       = vbuf.buf_num;
#ifdef ASIC_TRACE_SUPPORT
        buf_info->is_frame_buffer = vbuf.is_frame_buffer;
#endif
    }

    return rv;
}

enum DecRet vpi_dec_vp9_add_buffer(VpiDecInst inst, struct DWLLinearMem *buf)
{
    return Vp9DecAddBuffer(inst, buf);
}
#endif

void vpi_dec_vp9_release(VpiDecInst inst)
{
    Vp9DecRelease(inst);
}

static VpiRet vp9_picture_consumed_noDWL(VpiDecInst inst,
                                         struct DecPicturePpu *pic)
{
    enum DecRet rv;
    struct Vp9DecPicture hpic;
    uint32_t i;

    memset(&hpic, 0, sizeof(struct Vp9DecPicture));
    /* TODO update chroma luma/chroma base */
    for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
#ifndef NEW_MEM_ALLOC
        hpic.pictures[i].output_luma_base =
            pic->pictures[i].luma.virtual_address;
#endif
        hpic.pictures[i].output_luma_bus_address =
            pic->pictures[i].luma.bus_address;
    }
    hpic.pp_enabled = pic->pictures[0].pp_enabled;
    hpic.is_intra_frame =
        pic->pictures[0].picture_info.pic_coding_type == DEC_PIC_TYPE_I;

    rv = Vp9DecPictureConsumed(inst, &hpic);
    if (rv) {
        VPILOGD("Vp9DecPictureConsumed ret %s\n", dec_ret_string(rv));
        return VPI_ERR_UNKNOWN;
    }
    return VPI_SUCCESS;
}

void vpi_decode_vp9_picture_consume(VpiDecCtx *vpi_ctx, void *data)
{
    VpiFrame *vpi_frame       = (VpiFrame *)data;
    struct DecPicturePpu *pic = (struct DecPicturePpu *)vpi_frame->data[0];

    pthread_mutex_lock(&vpi_ctx->dec_thread_mutex);
    del_dec_pic_wait_consume_list(vpi_ctx, (void *)pic);
    vp9_picture_consumed_noDWL(vpi_ctx->dec_inst, pic);
    free(pic);
    if (vpi_ctx->waiting_for_dpb == 1) {
        pthread_cond_signal(&vpi_ctx->dec_thread_cond);
        vpi_ctx->waiting_for_dpb = 0;
    }
    pthread_mutex_unlock(&vpi_ctx->dec_thread_mutex);
}

static enum DecRet vpi_vp9_init(VpiDecCtx *vpi_ctx, struct DecConfig config,
                                const void *dwl)
{
    enum DecPictureFormat format = config.output_format;
    struct Vp9DecConfig dec_cfg;
    enum DecRet rv;

    memset(&dec_cfg, 0, sizeof(struct Vp9DecConfig));

    dec_cfg.use_video_freeze_concealment = config.concealment_mode;
    dec_cfg.num_frame_buffers            = 9;
    dec_cfg.dpb_flags                    = 4;
    dec_cfg.use_video_compressor         = config.use_video_compressor;
    dec_cfg.use_ringbuffer               = config.use_ringbuffer;
    dec_cfg.output_format                = format;
    dec_cfg.tile_by_tile                 = config.tile_by_tile;

    VPILOGD("Vp9Init config.ppu_cfg[0].out_cut_8bits=%d\n",
            config.ppu_cfg[0].out_cut_8bits);

    if (config.ppu_cfg[0].out_cut_8bits) {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_CUT_8BIT;
    } else if (config.ppu_cfg[0].out_p010) {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_P010;
    } else if (config.ppu_cfg[0].out_be) {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_CUSTOMER1;
    } else {
        dec_cfg.pixel_format = DEC_OUT_PIXEL_DEFAULT;
    }
    memcpy(dec_cfg.ppu_cfg, config.ppu_cfg, sizeof(config.ppu_cfg));

    rv = Vp9DecInit(&vpi_ctx->dec_inst, dwl, &dec_cfg);
    VPILOGD("Vp9DecInit after Vp9DecInit rv = %d, dec_inst=%p!!\n", rv,
            vpi_ctx->dec_inst);
    return rv;
}

VpiRet vpi_decode_vp9_init(VpiDecCtx *vpi_ctx)
{
    enum DecRet rv;
    struct DecConfig config;

    vpi_ctx->min_buffer_num = 0;
    vpi_ctx->max_strm_len   = DEC_X170_MAX_STREAM_VC8000D;
    vpi_ctx->enable_mc      = 0;

    config.disable_picture_reordering = 0;
    config.concealment_mode           = 0;
    config.align                      = DEC_ALIGN_1024B;
    config.decoder_mode               = DEC_NORMAL;
    config.tile_by_tile               = 0;

    vpi_resolve_pp_overlap_ppu(config.ppu_cfg, vpi_ctx->tb_cfg.pp_units_params);

    config.output_format = DEC_OUT_FRM_TILED_4X4;

    VPILOGD("Configuring hardware to output: %s\n",
            config.output_format == DEC_OUT_FRM_RASTER_SCAN ?
                "Semiplanar YCbCr 4:2:0 (four_cc 'NV12')" :
                "4x4 tiled YCbCr 4:2:0");
    config.dwl_inst                        = vpi_ctx->dwl_inst;
    config.max_num_pics_to_decode          = 0;
    config.use_video_compressor            = 0;
    config.use_ringbuffer                  = 1;
    config.mc_cfg.mc_enable                = 0;
    config.mc_cfg.stream_consumed_callback = NULL;
    config.use_video_compressor            = vpi_ctx->disable_dtrc == 1 ? 0 : 1;
    rv = vpi_vp9_init(vpi_ctx, config, vpi_ctx->dwl_inst);
    if (rv != DEC_OK) {
        VPILOGE("DECODER INITIALIZATION FAILED\n");
        return VPI_ERR_UNKNOWN;
    } else {
        VPILOGD("DECODER Vp9Init Init OK\n");
    }

    /* number of stream buffers to allocate */
    vpi_ctx->allocated_buffers = MAX_STRM_BUFFERS;

#ifdef ALWAYS_OUTPUT_REF
    Vp9DecUseExtraFrmBuffers(vpi_ctx->dec_inst, vpi_ctx->use_extra_buffers_num);
#endif

    return VPI_SUCCESS;
}

VpiRet vpi_decode_vp9_control(VpiDecCtx *vpi_ctx, void *indata, void *outdata)
{
    VpiCtrlCmdParam *in_param = (VpiCtrlCmdParam *)indata;
    int *out_num = NULL;
    int ret = VPI_SUCCESS;

    switch (in_param->cmd) {
    case VPI_CMD_DEC_PIC_CONSUME:
        vpi_decode_vp9_picture_consume(vpi_ctx, in_param->data);
        break;
    case VPI_CMD_DEC_STRM_BUF_COUNT:
        out_num = (int*)outdata;
        *out_num = vpi_dec_get_stream_buffer_index(vpi_ctx, 0);
        break;
    case VPI_CMD_DEC_GET_USED_STRM_MEM:
        vpi_decode_vp9_get_used_strm_mem(vpi_ctx, outdata);
        break;
    case VPI_CMD_DEC_SET_FRAME_BUFFER:
        ret = vpi_decode_vp9_set_frame_buffer(vpi_ctx, in_param->data);
        break;
    default:
        break;
    }

    return ret;
}

int vpi_decode_vp9_close(VpiDecCtx *vpi_ctx)
{
    int i;
    int idx;

    while (vpi_ctx->strm_buf_head) {
        if (vpi_ctx->strm_buf_head->item_size) {
            idx = vpi_ctx->rls_mem_index;
            vpi_ctx->rls_strm_buf_list[idx]->mem_idx = vpi_ctx->rls_mem_index;
            vpi_ctx->rls_strm_buf_list[idx]->item    = vpi_ctx->strm_buf_head->item;
            vpi_ctx->rls_strm_buf_list[idx]->opaque  =
                vpi_ctx->strm_buf_head->opaque;
            vpi_dec_buf_list_add(&vpi_ctx->rls_strm_buf_head,
                                 vpi_ctx->rls_strm_buf_list[idx]);

            vpi_ctx->rls_mem_index++;
            if (vpi_ctx->rls_mem_index == 32) {
                vpi_ctx->rls_mem_index = 0;
            }
        }

        vpi_ctx->strm_buf_head =
            vpi_dec_buf_list_delete(vpi_ctx->strm_buf_head);
    }

    if (vpi_ctx->dec_inst) {
        Vp9DecEndOfStream(vpi_ctx->dec_inst);
    }
    vpi_ctx->last_pic_flag = 1;

    for (i = 0; i < vpi_ctx->allocated_buffers; i++) {
        if (vpi_ctx->stream_mem[i].mem_type == DWL_MEM_TYPE_DPB) {
            vpi_ctx->stream_mem[i].virtual_address = NULL;
        }
        if (vpi_ctx->dec_inst) {
            DWLFreeLinear(vpi_ctx->dwl_inst, &vpi_ctx->stream_mem[i]);
        }
        free(vpi_ctx->strm_buf_list[i]);
    }

    for (i = 0; i < MAX_BUFFERS; i++) {
        free(vpi_ctx->frame_buf_list[i]);
    }

    if (vpi_ctx->pic_display_number > 0) {
        vpi_dec_performance_report(vpi_ctx);
    }
    if (vpi_ctx->dec_inst) {
        Vp9DecRelease(vpi_ctx->dec_inst);
    }
    vpi_dec_release_ext_buffers(vpi_ctx);
    if (vpi_ctx->dwl_inst) {
        DWLRelease(vpi_ctx->dwl_inst);
    }

    return 0;
}
