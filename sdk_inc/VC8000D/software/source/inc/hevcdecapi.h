/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Inc. All rights reserved        --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#ifndef HEVCDECAPI_H
#define HEVCDECAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "basetype.h"
#include "decapicommon.h"
#include "dectypes.h"
#include "sw_util.h"

/*------------------------------------------------------------------------------
    API type definitions
------------------------------------------------------------------------------*/

/* Decoder instance */
typedef const void *HevcDecInst;

/* Input structure */
struct HevcDecInput {
  u8 *stream;             /* Pointer to the input */
  addr_t stream_bus_address; /* DMA bus address of the input stream */
  u32 data_len;           /* Number of bytes to be decoded         */
  u8 *buffer;
  addr_t buffer_bus_address;
  u32 buff_len;           /* Stream buffer byte length         */
  u32 pic_id;             /* Identifier for the picture to be decoded */
  u32 *raster_out_y;
  addr_t raster_out_bus_address_y; /* DMA bus address of the input stream */
  u32 *raster_out_c;
  addr_t raster_out_bus_address_c;
};

/* Output structure */
struct HevcDecOutput {
  u8 *strm_curr_pos; /* Pointer to stream position where decoding ended */
  u32 strm_curr_bus_address; /* DMA bus address location where the decoding
                                ended */
  u32 data_left; /* how many bytes left undecoded */
};

/* cropping info */
struct HevcCropParams {
  u32 crop_left_offset;
  u32 crop_out_width;
  u32 crop_top_offset;
  u32 crop_out_height;
};

/* stream info filled by HevcDecGetInfo */
struct HevcDecInfo {
  u32 pic_width;   /* decoded picture width in pixels */
  u32 pic_height;  /* decoded picture height in pixels */
  u32 video_range; /* samples' video range */
  u32 matrix_coefficients;
  struct HevcCropParams crop_params;   /* display cropping information */
  enum DecPictureFormat output_format; /* format of the output picture */
  enum DecPicturePixelFormat pixel_format; /* format of the pixels in output picture */
  u32 sar_width;                       /* sample aspect ratio */
  u32 sar_height;                      /* sample aspect ratio */
  u32 mono_chrome;                     /* is sequence monochrome */
  u32 interlaced_sequence;             /* is sequence interlaced */
  u32 dpb_mode;      /* DPB mode; frame, or field interlaced */
  u32 pic_buff_size; /* number of picture buffers allocated&used by decoder */
  u32 multi_buff_pp_size; /* number of picture buffers needed in
                             decoder+postprocessor multibuffer mode */
  u32 bit_depth;     /* bit depth per pixel stored in memory */
  u32 pic_stride;         /* Byte width of the pixel as stored in memory */

  /* for HDR */
  u32 transfer_characteristics;
};

/* Output structure for HevcDecNextPicture */
struct HevcDecPicture {
  struct HevcCropParams crop_params; /* cropping parameters */
  u32 pic_id;         /* Identifier of the picture to be displayed */
  u32 decode_id;      /*Identifier of the decoing order of the picture */
  u32 is_idr_picture; /* Indicates if picture is an IDR picture */
  u32 pic_corrupt;    /* Indicates that picture is corrupted */
  u32 bit_depth_luma;
  u32 bit_depth_chroma;
  u32 pp_enabled;
  const u32 *output_rfc_luma_base;   /* Pointer to the rfc table */
  addr_t output_rfc_luma_bus_address;
  const u32 *output_rfc_chroma_base; /* Pointer to the rfc chroma table */
  addr_t output_rfc_chroma_bus_address; /* Bus address of the chrominance table */
  u32 rfc_compressed;
  u32 cycles_per_mb;   /* Avarage cycle count per macroblock */
  u32 multi_tile_cols;  /* multiple tile columns exist in the pictures */
  struct HevcDecInfo dec_info; /* Stream info by HevcDecGetInfo */
  struct HevcOutputInfo {
    u32 pic_width;  /* pixels width of the picture as stored in memory */
    u32 pic_height; /* pixel height of the picture as stored in memory */
    u32 pic_stride;       /* Byte width of the picture as stored in memory */
    u32 pic_stride_ch;
    const u32 *output_picture;         /* Pointer to the picture */
    addr_t output_picture_bus_address;    /* DMA bus address of the output picture buffer */
    const u32 *output_picture_chroma;         /* Pointer to the picture */
    addr_t output_picture_chroma_bus_address;    /* DMA bus address of the output picture buffer */
    enum DecPictureFormat output_format;
    enum DecPicturePixelFormat pixel_format;
    u32 pic_compressed_status;
  } pictures[DEC_MAX_OUT_COUNT];
  u32 main10_profile;
};

struct HevcDecConfig {
  u32 no_output_reordering;
  u32 use_video_freeze_concealment;
  u32 use_video_compressor;
  u32 use_ringbuffer;
  u32 tile_by_tile;
  enum DecDecoderMode decoder_mode;
#ifdef USE_EXTERNAL_BUFFER
  u32 use_adaptive_buffers; // When sequence changes, if old output buffers (number/size) are sufficient for new sequence,
  // old buffers will be used instead of reallocating output buffer.
  u32 guard_size;       // The minimum difference between minimum buffers number and allocated buffers number
  // that will force to return HDRS_RDY even buffers number/size are sufficient
  // for new sequence.
#endif
  DecPicAlignment align;
#if 0
  struct {
    u32 enabled;  // whether cropping is enabled
    u32 x;        // cropping start x
    u32 y;        // cropping start y
    u32 width;    // cropping width
    u32 height;   // cropping height
  } crop;
  struct {
    u32 enabled;  // whether scaling is enabled
    u32 width;    // scaled output width
    u32 height;   // scaled output height
  } scale;
#else
  PpUnitConfig ppu_cfg[4];
#endif
  enum DecPictureFormat output_format;
  enum DecPicturePixelFormat pixel_format;
};

typedef struct DecSwHwBuild HevcDecBuild;

#ifdef USE_EXTERNAL_BUFFER
struct HevcDecBufferInfo {
  u32 next_buf_size;
  u32 buf_num;
  struct DWLLinearMem buf_to_free;
#ifdef ASIC_TRACE_SUPPORT
  u32 is_frame_buffer;
#endif
};
#endif
/*------------------------------------------------------------------------------
    Prototypes of Decoder API functions
------------------------------------------------------------------------------*/

HevcDecBuild HevcDecGetBuild(void);

enum DecRet HevcDecInit(HevcDecInst *dec_inst, const void *dwl, struct HevcDecConfig *dec_cfg);

enum DecRet HevcDecUseExtraFrmBuffers(HevcDecInst dec_inst, u32 n);

void HevcDecRelease(HevcDecInst dec_inst);

enum DecRet HevcDecDecode(HevcDecInst dec_inst,
                          const struct HevcDecInput *input,
                          struct HevcDecOutput *output);

enum DecRet HevcDecNextPicture(HevcDecInst dec_inst,
                               struct HevcDecPicture *picture);

enum DecRet HevcDecPictureConsumed(HevcDecInst dec_inst,
                                   const struct HevcDecPicture *picture);

enum DecRet HevcDecEndOfStream(HevcDecInst dec_inst);

enum DecRet HevcDecGetInfo(HevcDecInst dec_inst, struct HevcDecInfo *dec_info);

enum DecRet HevcDecPeek(HevcDecInst dec_inst, struct HevcDecPicture *output);

#ifdef USE_EXTERNAL_BUFFER
enum DecRet HevcDecAddBuffer(HevcDecInst dec_inst, struct DWLLinearMem *info);

enum DecRet HevcDecGetBufferInfo(HevcDecInst dec_inst, struct HevcDecBufferInfo *mem_info);

enum DecRet HevcDecAbort(HevcDecInst dec_inst);

enum DecRet HevcDecAbortAfter(HevcDecInst dec_inst);
#endif

void HevcDecSetNoReorder(HevcDecInst dec_inst, u32 no_reorder);

enum DecRet HevcDecSetInfo(HevcDecInst dec_inst, struct HevcDecConfig *dec_cfg);

void HevcDecUpdateStrm(struct strmInfo info);

void HevcDecUpdateStrmInfoCtrl(HevcDecInst dec_inst, u32 last_flag, u32 strm_bus_addr);
#ifdef __cplusplus
}
#endif

#endif /* HEVCDECAPI_H */
