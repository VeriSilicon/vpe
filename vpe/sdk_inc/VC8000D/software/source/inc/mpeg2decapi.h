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

#ifndef __MPEG2DECAPI_H__
#define __MPEG2DECAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
#include "decapicommon.h"
#ifdef USE_EXTERNAL_BUFFER
#include "dwl.h"
#endif


/*------------------------------------------------------------------------------
    API type definitions
------------------------------------------------------------------------------*/

/* Return values */
typedef enum {
  MPEG2DEC_OK = 0,
  MPEG2DEC_STRM_PROCESSED = 1,
  MPEG2DEC_PIC_RDY = 2,
  MPEG2DEC_HDRS_RDY = 3,
  MPEG2DEC_HDRS_NOT_RDY = 4,
  MPEG2DEC_PIC_DECODED = 5,
  MPEG2DEC_NONREF_PIC_SKIPPED = 6,/* Skipped non-reference picture */
#ifdef USE_OUTPUT_RELEASE
  MPEG2DEC_ABORTED = 7,
  MPEG2DEC_END_OF_STREAM = 8,
#endif

#ifdef USE_EXTERNAL_BUFFER
  /** Waiting for external buffers allocated. */
  MPEG2DEC_WAITING_FOR_BUFFER = 9,
#endif
#ifdef USE_OUTPUT_RELEASE
  MPEG2DEC_FLUSHED = 10,
#endif
  MPEG2DEC_BUF_EMPTY = 11,

  MPEG2DEC_PARAM_ERROR = -1,
  MPEG2DEC_STRM_ERROR = -2,
  MPEG2DEC_NOT_INITIALIZED = -3,
  MPEG2DEC_MEMFAIL = -4,
  MPEG2DEC_INITFAIL = -5,
  MPEG2DEC_STREAM_NOT_SUPPORTED = -8,
#ifdef USE_EXTERNAL_BUFFER
  MPEG2DEC_EXT_BUFFER_REJECTED = -9, /**<\hideinitializer */
#endif

  MPEG2DEC_NO_DECODING_BUFFER = -10,  /* no available frame for decoding using */
  MPEG2DEC_HW_RESERVED = -254,
  MPEG2DEC_HW_TIMEOUT = -255,
  MPEG2DEC_HW_BUS_ERROR = -256,
  MPEG2DEC_SYSTEM_ERROR = -257,
  MPEG2DEC_DWL_ERROR = -258,
  MPEG2DEC_FORMAT_NOT_SUPPORTED = -1000
} Mpeg2DecRet;

/* decoder output picture format */
typedef enum {
  MPEG2DEC_SEMIPLANAR_YUV420 = 0x020001,
  MPEG2DEC_TILED_YUV420 = 0x020002
} Mpeg2DecOutFormat;

/* DAR (Display aspect ratio) */
typedef enum {
  MPEG2DEC_1_1 = 0x01,
  MPEG2DEC_4_3 = 0x02,
  MPEG2DEC_16_9 = 0x03,
  MPEG2DEC_2_21_1 = 0x04
} Mpeg2DecDARFormat;

typedef struct {
  u32 *virtual_address;
  addr_t bus_address;
} Mpeg2DecLinearMem;

/* Decoder instance */
typedef void *Mpeg2DecInst;

/* Input structure */
typedef struct {
  u8 *stream;         /* Pointer to stream to be decoded              */
  addr_t stream_bus_address;   /* DMA bus address of the input stream */
  u32 data_len;         /* Number of bytes to be decoded                */
  u32 pic_id;
  u32 skip_non_reference; /* Flag to enable decoder skip non-reference
                               * frames to reduce processor load */
} Mpeg2DecInput;

/* Time code */
typedef struct {
  u32 hours;
  u32 minutes;
  u32 seconds;
  u32 pictures;
} Mpeg2DecTime;

typedef struct {
  u8 *strm_curr_pos;
  addr_t strm_curr_bus_address; /* DMA bus address location where the decoding
                                 * ended */
  u32 data_left;
} Mpeg2DecOutput;

/* stream info filled by Mpeg2DecGetInfo */
typedef struct {
  u32 frame_width;
  u32 frame_height;
  u32 coded_width;
  u32 coded_height;
  u32 profile_and_level_indication;
  u32 display_aspect_ratio;
  u32 stream_format;
  u32 video_format;
  u32 video_range;      /* ??? only [0-255] */
  u32 interlaced_sequence;
  enum DecDpbMode dpb_mode;         /* DPB mode; frame, or field interlaced */
#ifdef USE_EXTERNAL_BUFFER
  u32 pic_buff_size;
#endif
  u32 multi_buff_pp_size;
  Mpeg2DecOutFormat output_format;
} Mpeg2DecInfo;

typedef struct {
  u8 *output_picture;
  addr_t output_picture_bus_address;
  u32 frame_width;
  u32 frame_height;
  u32 coded_width;
  u32 coded_height;
  u32 key_picture;
  u32 pic_id;
  u32 decode_id;
  u32 pic_coding_type[2];
  u32 interlaced;
  u32 field_picture;
  u32 top_field;
  u32 first_field;
  u32 repeat_first_field;
  u32 single_field;
  u32 output_other_field;
  u32 repeat_frame_count;
  u32 number_of_err_mbs;
  u32 pic_stride;
  enum DecDpbMode dpb_mode;         /* DPB mode; frame, or field interlaced */
  enum DecPictureFormat output_format;
  Mpeg2DecTime time_code;
} Mpeg2DecPicture;

/* Version information */
typedef struct {
  u32 major;           /* API major version */
  u32 minor;           /* API minor version */

} Mpeg2DecApiVersion;

typedef struct DecSwHwBuild  Mpeg2DecBuild;
#ifdef USE_EXTERNAL_BUFFER
/*!\struct Mpeg2DecBufferInfo_
 * \brief Reference buffer information
 *
 * A structure containing the reference buffer information, filled by
 * Mpeg2DecGetBufferInfo()
 *
 * \typedef Mpeg2DecBufferInfo
 * A typename for #Mpeg2DecBufferInfo_.
 */
typedef struct Mpeg2DecBufferInfo_ {
  u32 next_buf_size;
  u32 buf_num;
  struct DWLLinearMem buf_to_free;
#ifdef ASIC_TRACE_SUPPORT
  u32 is_frame_buffer;
#endif
} Mpeg2DecBufferInfo;
#endif

struct Mpeg2DecConfig {
  enum DecErrorHandling error_handling;
  enum DecDpbFlags dpb_flags;
#ifdef USE_EXTERNAL_BUFFER
  u32 use_adaptive_buffers; // When sequence changes, if old output buffers (number/size) are sufficient for new sequence,
  // old buffers will be used instead of reallocating output buffer.
  u32 guard_size;       // The minimum difference between minimum buffers number and allocated buffers number
  // that will force to return HDRS_RDY even buffers number/size are sufficient
  // for new sequence.
#endif
  DecPicAlignment align;
  u32 cr_first;
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
};

/*------------------------------------------------------------------------------
    Prototypes of Decoder API functions
------------------------------------------------------------------------------*/

Mpeg2DecApiVersion Mpeg2DecGetAPIVersion(void);

Mpeg2DecBuild Mpeg2DecGetBuild(void);

Mpeg2DecRet Mpeg2DecInit(Mpeg2DecInst * dec_inst,
#ifdef USE_EXTERNAL_BUFFER
                         const void *dwl,
#endif
                         enum DecErrorHandling error_handling,
                         u32 num_frame_buffers,
                         enum DecDpbFlags dpb_flags,
                         u32 use_adaptive_buffers,
                         u32 n_guard_size);

Mpeg2DecRet Mpeg2DecDecode(Mpeg2DecInst dec_inst,
                           Mpeg2DecInput * input,
                           Mpeg2DecOutput * output);

Mpeg2DecRet Mpeg2DecGetInfo(Mpeg2DecInst dec_inst, Mpeg2DecInfo * dec_info);

Mpeg2DecRet Mpeg2DecNextPicture(Mpeg2DecInst dec_inst,
                                Mpeg2DecPicture * picture,
                                u32 end_of_stream);
#ifdef USE_OUTPUT_RELEASE
Mpeg2DecRet Mpeg2DecPictureConsumed(Mpeg2DecInst dec_inst,
                                    Mpeg2DecPicture *picture);

Mpeg2DecRet Mpeg2DecEndOfStream(Mpeg2DecInst dec_inst, u32 strm_end_flag);
Mpeg2DecRet Mpeg2DecAbort(Mpeg2DecInst dec_inst);
Mpeg2DecRet Mpeg2DecWaitAfter(Mpeg2DecInst dec_inst);
#endif

void Mpeg2DecRelease(Mpeg2DecInst dec_inst);

Mpeg2DecRet Mpeg2DecPeek(Mpeg2DecInst dec_inst, Mpeg2DecPicture * picture);
#ifdef USE_EXTERNAL_BUFFER
Mpeg2DecRet Mpeg2DecGetBufferInfo(Mpeg2DecInst dec_inst, Mpeg2DecBufferInfo *mem_info);

Mpeg2DecRet Mpeg2DecAddBuffer(Mpeg2DecInst dec_inst, struct DWLLinearMem *info);
#endif
Mpeg2DecRet Mpeg2DecSetInfo(Mpeg2DecInst dec_inst, struct Mpeg2DecConfig *dec_cfg);
/*------------------------------------------------------------------------------
    Prototype of the API trace funtion. Traces all API entries and returns.
    This must be implemented by the application using the decoder API!
    Argument:
        string - trace message, a null terminated string
------------------------------------------------------------------------------*/
void Mpeg2DecTrace(const char *string);

#ifdef __cplusplus
}
#endif

#endif                       /* __MPEG2DECAPI_H__ */
