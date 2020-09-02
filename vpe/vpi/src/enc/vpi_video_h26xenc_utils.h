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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __VPI_VIDEO_H26XENC_UTILS_H__
#define __VPI_VIDEO_H26XENC_UTILS_H__

#include "vpi_video_h26xenc.h"

FILE *open_file(char *name, char *mode);
void h26x_enc_write_strm(FILE *fout, u32 *strmbuf, u32 size, u32 endian);
i32 setup_roi_map_buffer(VPIH26xEncCfg *tb, VPIH26xEncOptions *options,
                         VCEncIn *p_enc_in, VCEncInst encoder);
/*input YUV format convertion for specific format*/
float get_pixel_width_inbyte(VCEncPictureType type);
FILE *format_customized_yuv(VPIH26xEncCfg *tb,
                            VPIH26xEncOptions *options, i32 *ret);
void change_cml_customized_format(VPIH26xEncOptions *options);
void change_to_customized_format(VPIH26xEncOptions *options,
                                 VCEncPreProcessingCfg *pre_proc_cfg);
/*read&parse input cfg files for different features*/
i32 read_config_files(VPIH26xEncCfg *tb, VPIH26xEncOptions *options);
i32 read_gmv(VPIH26xEncCfg *tb, VCEncIn *p_enc_in,
             VPIH26xEncOptions *options);
i32 parsing_smart_config(char *fname, VPIH26xEncOptions *options);
/*input YUV read*/
i32 read_picture(VPIH26xEncCfg *tb, u32 input_format, u32 src_img_size,
                 u32 src_width, u32 src_height);
u64 h26x_enc_next_picture(VPIH26xEncCfg *tb, int picture_cnt);
void get_aligned_pic_size_byformat(i32 type, u32 width, u32 height,
                                   u32 alignment, u32 *luma_size,
                                   u32 *chroma_size, u32 *picture_size);
u32 get_resolution(char *filename, i32 *p_width, i32 *p_height);
/*GOP pattern file parse*/
int init_gop_configs(int gop_size, VPIH26xEncOptions *options,
                     VCEncGopConfig *gop_cfg, u8 *gop_cfg_offset, bool b_pass2);
/*SEI information from cfg file*/
u8 *read_userdata(VCEncInst encoder, char *name);
/*adaptive gop decision*/
i32 adaptive_gop_decision(VPIH26xEncCfg *tb, VCEncIn *p_enc_in,
                          VCEncInst encoder, i32 *p_next_gop_size,
                          AdapGopCtr *agop);
i32 get_next_gop_size(VPIH26xEncCfg *tb, VCEncIn *p_enc_in,
                      VCEncInst encoder, i32 *p_next_gop_size,
                      AdapGopCtr *agop);
u32 setup_input_buffer(VPIH26xEncCfg *tb, VPIH26xEncOptions *options,
                       VCEncIn *p_enc_in);
void setup_output_buffer(VCEncInst inst, VpiEncOutData *out_buffer,
                         VCEncIn *p_enc_in);
void get_free_iobuffer(VPIH26xEncCfg *tb);
void init_slice_ctl(VPIH26xEncCfg *tb, VPIH26xEncOptions *options);
void setup_slice_ctl(VPIH26xEncCfg *tb);
i32 change_format_for_FB(VPIH26xEncCfg *tb, VPIH26xEncOptions *options,
                         VCEncPreProcessingCfg *pre_proc_cfg);
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
i32 read_table(VPIH26xEncCfg *tb, u32 lum_tbl_size, u32 ch_tbl_size);
#endif
void init_stream_segment_crl(VPIH26xEncCfg *tb, VPIH26xEncOptions *options);
void write_strm_bufs(FILE *fout, VCEncStrmBufs *bufs, u32 offset, u32 size,
                     u32 endian);
void write_nals_bufs(FILE *fout, VCEncStrmBufs *bufs,
                     const u32 *p_nalu_size_buf, u32 num_nalus, u32 offset,
                     u32 hdr_size, u32 endian);
void get_stream_bufs(VCEncStrmBufs *bufs, VPIH26xEncCfg *tb,
                     VPIH26xEncOptions *options, bool encoding);
/* timer help*/
unsigned int utime_diff(struct timeval end, struct timeval start);

int h26x_enc_fifo_release(VpiH26xEncCtx *enc_ctx);
int h26x_enc_fifo_init(VpiH26xEncCtx *enc_ctx);
int h26x_enc_push_outfifo(VpiH26xEncCtx *enc_ctx,
                          VpiEncOutData *enc_pkt);
int h26x_enc_pop_outfifo(VpiH26xEncCtx *enc_ctx,
                         VpiEncOutData **enc_pkt);
int h26x_enc_push_emptyfifo(VpiH26xEncCtx *enc_ctx,
                            VpiEncOutData *enc_pkt);
int h26x_enc_pop_emptyfifo(VpiH26xEncCtx *enc_ctx,
                           VpiEncOutData **enc_pkt);
int h26x_enc_get_pic_buffer(VpiH26xEncCtx *ctx, void *outdata);
int h26x_enc_get_frame_packet(VpiH26xEncCtx *ctx, void *outdata);
int h26x_enc_get_used_pic_mem(VpiH26xEncCtx *ctx, void *mem);
void h26x_enc_consume_pic(VpiH26xEncCtx *ctx, int consume_poc);
int h26x_enc_get_out_buffer(VpiH26xEncCtx *ctx, VpiEncOutData **out_buffer);
void h26x_enc_buf_list_add(H26xEncBufLink **head, H26xEncBufLink *list);
#endif /* __VPI_VIDEO_H26XENC_UTILS_H__ */
