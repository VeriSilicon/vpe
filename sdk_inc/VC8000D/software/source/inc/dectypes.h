/*------------------------------------------------------------------------------
--Copyright (c) 2015-2017, VeriSilicon Holdings Co., Ltd. All rights reserved --
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

#ifndef DECTYPES_H
#define DECTYPES_H

#include "basetype.h"
#include "dwl.h"
#include "decapicommon.h"

enum DecCodec {
  DEC_VP9,
  DEC_HEVC,
  DEC_H264_H10P
};

/*!\enum DecRet
 * Return values for API
 */
enum DecRet {
  /** Success */
  DEC_OK = 0, /**<\hideinitializer */
  /** Stream processed */
  DEC_STRM_PROCESSED = 1, /**<\hideinitializer */
  /** Picture available for output */
  DEC_PIC_RDY = 2, /**<\hideinitializer */
  /** Picture decoded */
  DEC_PIC_DECODED = 3, /**<\hideinitializer */
  /** New stream headers decoded */
  DEC_HDRS_RDY = 4, /**<\hideinitializer */
  /** Advanced coding tools detected in stream */
  DEC_ADVANCED_TOOLS = 5, /**<\hideinitializer */
  /** Output pictures must be retrieved before continuing decode */
  DEC_PENDING_FLUSH = 6, /**<\hideinitializer */
  /** Skipped decoding non-reference picture */
  DEC_NONREF_PIC_SKIPPED = 7, /**<\hideinitializer */
  /** End-of-stream state set in the decoder */
  DEC_END_OF_STREAM = 8,         /**<\hideinitializer */
  /** Waiting for external buffers allocated. */
  DEC_WAITING_FOR_BUFFER = 9,    /**<\hideinitializer */
  /** Decoder is aborted */
  DEC_ABORTED = 10,              /**<\hideinitializer */
  /** All pics have been flushed */
  DEC_FLUSHED = 11,              /**<\hideinitializer */
  DEC_BUF_EMPTY = 12,              /**<\hideinitializer */
  DEC_STREAM_ERROR_DEDECTED = 13,
  DEC_RESOLUTION_CHANGE = 16,
  DEC_PARAM_ERROR = -1,          /**<\hideinitializer */
  DEC_STRM_ERROR = -2,           /**<\hideinitializer */
  DEC_NOT_INITIALIZED = -3,      /**<\hideinitializer */
  DEC_MEMFAIL = -4,              /**<\hideinitializer */
  DEC_INITFAIL = -5,             /**<\hideinitializer */
  DEC_HDRS_NOT_RDY = -6,         /**<\hideinitializer */
  DEC_STREAM_NOT_SUPPORTED = -8, /**<\hideinitializer */
  DEC_EXT_BUFFER_REJECTED = -9,    /**<\hideinitializer */
  DEC_INFOPARAM_ERROR = -10,
  /** External buffer rejected. (Too much than requested) */
  DEC_NO_DECODING_BUFFER = -99,
  DEC_HW_RESERVED = -254,        /**<\hideinitializer */
  DEC_HW_TIMEOUT = -255,         /**<\hideinitializer */
  DEC_HW_BUS_ERROR = -256,       /**<\hideinitializer */
  DEC_SYSTEM_ERROR = -257,       /**<\hideinitializer */
  DEC_DWL_ERROR = -258,          /**<\hideinitializer */
  DEC_FATAL_SYSTEM_ERROR = -259,       /**<\hideinitializer */
  DEC_FORMAT_NOT_SUPPORTED =
    -1000 /**<\hideinitializer */
    /* TODO(vmr): Prune what is not needed from these. */
};

/* cropping info */
struct DecCropParams {
  u32 crop_left_offset;
  u32 crop_out_width;
  u32 crop_top_offset;
  u32 crop_out_height;
};

/* Input structure */
struct DecInput {
  struct DWLLinearMem buffer; /**< Pointer to the input buffer. */
  u8* stream[2];              /** < stream[0]: stream start before inputting. */
  /** < stream[1]: stream end after inputting. */
  u32 data_len;               /**< Number of bytes to be decoded. */
};

/** Error concealment mode */
enum DecErrorConcealment {
  DEC_PICTURE_FREEZE = 0,
  DEC_INTRA_FREEZE = 1
};

typedef void DecMCStreamConsumed(void *stream, void *p_user_data);

struct DecMCConfig {
  u32 mc_enable;
  /*! Application provided callback for stream buffer processed. */
  DecMCStreamConsumed *stream_consumed_callback;
};

/** Decoder initialization params */
struct DecConfig {
  u32 disable_picture_reordering;
  enum DecPictureFormat output_format; /**< Format of the output picture */
  struct DWL dwl; /**< Pointers to the struct DWL functions. */
  const void* dwl_inst;       /**< struct DWL instance. */
  u32 max_num_pics_to_decode; /**< Limits the decoding to N pictures. 0 for
                                   unlimited. */
  enum DecErrorConcealment concealment_mode;
  enum DecDecoderMode decoder_mode;
  DecPicAlignment align;
  struct DecFixedScaleCfg fscale_cfg;   /* for fixed ratio config only */
  PpUnitConfig ppu_cfg[4];
  u32 use_video_compressor;
  u32 use_ringbuffer;
  u32 use_8bits_output;
  u32 use_p010_output;
  u32 use_bige_output;  /* Output pixel format. */
  u32 tile_by_tile;
  struct DecMCConfig mc_cfg;
};

/** Sample range of the YCbCr samples in the decoded picture. */
enum DecVideoRange {
  DEC_VIDEO_RANGE_NORMAL = 0x0, /**< Sample range [16, 235] */
  DEC_VIDEO_RANGE_FULL = 0x1    /**< Sample range [0, 255] */
};

/* Video sequence information. */
struct DecSequenceInfo {
  u32 pic_width;                    /**< decoded picture width in pixels */
  u32 pic_height;                   /**< decoded picture height in pixels */
  u32 sar_width;                    /**< sample aspect ratio */
  u32 sar_height;                   /**< sample aspect ratio */
  struct DecCropParams crop_params; /**< Cropping parameters for the picture */
  enum DecVideoRange video_range;   /**< YUV sample video range */
  u32 matrix_coefficients; /**< matrix coefficients RGB->YUV conversion */
  u32 is_mono_chrome;      /**< is sequence monochrome */
  u32 is_interlaced;       /**< is sequence interlaced */
  u32 num_of_ref_frames;   /**< Maximum number of reference frames */
  u32 bit_depth_luma;    /* Bit depth of stored picture */
  u32 bit_depth_chroma;
  u32 pic_stride;       /* Byte width of the picture as stored in memory */
  u32 pic_stride_ch;
  u32 main10_profile;
};
#if 1
/** Picture coding type */
enum DecPicCodingType {
  DEC_PIC_TYPE_I           = 0,
  DEC_PIC_TYPE_P           = 1,
  DEC_PIC_TYPE_B           = 2,
  DEC_PIC_TYPE_D           = 3,
  DEC_PIC_TYPE_FI          = 4,
  DEC_PIC_TYPE_BI          = 5
};
#endif

/* Picture specific information. */
struct DecPictureInfo {
  enum DecPicCodingType pic_coding_type; /**< Picture coding type */
  u32 is_corrupted;             /**< Tells whether picture is corrupted */
  enum DecPictureFormat format; /**< Color format of the picture */
  enum DecPicturePixelFormat pixel_format; /**< Pixel format of the picture */
  u32 cycles_per_mb;            /**< Avarage decoding time in cycles per mb */
  u32 pic_id;                   /**< Identifier for the picture to be decoded */
  u32 decode_id;
};

/* Structure to carry information about decoded pictures. */
struct DecPicture {
  struct DecSequenceInfo sequence_info; /**< Sequence coding parameters used */
  struct DWLLinearMem luma;             /**< Buffer properties */
  struct DWLLinearMem chroma;           /**< Buffer properties */
  struct DWLLinearMem luma_table;           /**< Buffer properties *//*sunny add for tile status address*/
  struct DWLLinearMem chroma_table;           /**< Buffer properties *//*sunny add for tile status address*/
  struct DecPictureInfo picture_info;   /**< Picture specific parameters */
  u32 pic_width;
  u32 pic_height;
  u32 pic_stride;
  u32 pic_stride_ch;
  u32 pp_enabled;
  u32 pic_compressed_status;/*0 no compress; 1 dtrc compress; 2 dec400 compress*/
};

struct DecPicturePpu {
  struct DecPicture pictures[DEC_MAX_OUT_COUNT];
};

#ifdef USE_EXTERNAL_BUFFER
struct DecBufferInfo {
  u32 next_buf_size;
  u32 buf_num;
  struct DWLLinearMem buf_to_free;
#ifdef ASIC_TRACE_SUPPORT
  u32 is_frame_buffer;
#endif
};

enum BufferType {
  REFERENCE_BUFFER = 0, /* reference + compression table + DMV*/
  RASTERSCAN_OUT_BUFFER,
  DOWNSCALE_OUT_BUFFER,
  TILE_EDGE_BUFFER,  /* filter mem + bsd control mem */
  SEGMENT_MAP_BUFFER, /* segment map */
  MISC_LINEAR_BUFFER, /* tile info + prob table + entropy context counter */
  BUFFER_TYPE_NUM
};

#define IS_EXTERNAL_BUFFER(config, type) (((config) >> (type)) & 1)
#endif

#endif /* DECTYPES_H */
