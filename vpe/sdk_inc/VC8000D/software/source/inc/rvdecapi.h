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

#ifndef __RVDECAPI_H__
#define __RVDECAPI_H__

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
  RVDEC_OK = 0,
  RVDEC_STRM_PROCESSED = 1,
  RVDEC_PIC_RDY = 2,
  RVDEC_HDRS_RDY = 3,
  RVDEC_HDRS_NOT_RDY = 4,
  RVDEC_PIC_DECODED = 5,
  RVDEC_NONREF_PIC_SKIPPED = 6,/* Skipped non-reference picture */
#ifdef USE_OUTPUT_RELEASE
  RVDEC_ABORTED = 7,
  RVDEC_END_OF_STREAM = 8,
#endif
#ifdef USE_EXTERNAL_BUFFER
  /** Waiting for external buffers allocated. */
  RVDEC_WAITING_FOR_BUFFER = 9,
#endif
#ifdef USE_OUTPUT_RELEASE
  RVDEC_FLUSHED = 10,
#endif

  RVDEC_BUF_EMPTY = 11,
  RVDEC_PARAM_ERROR = -1,
  RVDEC_STRM_ERROR = -2,
  RVDEC_NOT_INITIALIZED = -3,
  RVDEC_MEMFAIL = -4,
  RVDEC_INITFAIL = -5,
  RVDEC_STREAM_NOT_SUPPORTED = -8,
#ifdef USE_EXTERNAL_BUFFER
  RVDEC_EXT_BUFFER_REJECTED = -9, /**<\hideinitializer */
#endif

  RVDEC_NO_DECODING_BUFFER = -10,  /* no available frame for decoding using */
  RVDEC_HW_RESERVED = -254,
  RVDEC_HW_TIMEOUT = -255,
  RVDEC_HW_BUS_ERROR = -256,
  RVDEC_SYSTEM_ERROR = -257,
  RVDEC_DWL_ERROR = -258,
  RVDEC_FORMAT_NOT_SUPPORTED = -1000
} RvDecRet;

/* decoder output picture format */
typedef enum {
  RVDEC_SEMIPLANAR_YUV420 = 0x020001,
  RVDEC_TILED_YUV420 = 0x020002
} RvDecOutFormat;

typedef struct {
  u32 *virtual_address;
  addr_t bus_address;
} RvDecLinearMem;

/* Decoder instance */
typedef void *RvDecInst;

typedef struct {
  u32 offset;
  u32 is_valid;
} RvDecSliceInfo;

/* Input structure */
typedef struct {
  u8 *stream;             /* Pointer to stream to be decoded              */
  addr_t stream_bus_address;    /* DMA bus address of the input stream */
  u32 data_len;             /* Number of bytes to be decoded                */
  u32 pic_id;
  u32 timestamp;       /* timestamp of current picture from rv frame header.
                              * NOTE: timestamp of a B-frame should be adjusted referring
                * to its forward reference frame's timestamp */

  u32 slice_info_num;    /* The number of slice offset entries. */
  RvDecSliceInfo *slice_info;     /* Pointer to the slice_info.
                                         * It contains offset value of each slice
                                         * in the data buffer, including start point "0"
                                         * and end point "data_len" */
  u32 skip_non_reference; /* Flag to enable decoder skip non-reference
                               * frames to reduce processor load */
} RvDecInput;

typedef struct {
  u8 *strm_curr_pos;
  addr_t strm_curr_bus_address;          /* DMA bus address location where the decoding
                                          * ended */
  u32 data_left;
} RvDecOutput;

/* stream info filled by RvDecGetInfo */
typedef struct {
  u32 frame_width;
  u32 frame_height;
  u32 coded_width;
  u32 coded_height;
  u32 multi_buff_pp_size;
#ifdef USE_EXTERNAL_BUFFER
  u32 pic_buff_size;
#endif
  enum DecDpbMode dpb_mode;         /* DPB mode; frame, or field interlaced */
  RvDecOutFormat output_format;
} RvDecInfo;

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
  u32 pic_coding_type;
  u32 number_of_err_mbs;
  u32 pic_stride;
  enum DecPictureFormat output_format;
} RvDecPicture;

/* Version information */
typedef struct {
  u32 major;           /* API major version */
  u32 minor;           /* API minor version */

} RvDecApiVersion;

typedef struct DecSwHwBuild  RvDecBuild;

#ifdef USE_EXTERNAL_BUFFER
/*!\struct RvDecBufferInfo_
 * \brief Reference buffer information
 *
 * A structure containing the reference buffer information, filled by
 * RvDecGetBufferInfo()
 *
 * \typedef RvDecBufferInfo
 * A typename for #RvDecBufferInfo_.
 */
typedef struct RvDecBufferInfo_ {
  u32 next_buf_size;
  u32 buf_num;
  struct DWLLinearMem buf_to_free;
#ifdef ASIC_TRACE_SUPPORT
  u32 is_frame_buffer;
#endif
} RvDecBufferInfo;
#endif

struct RvDecConfig {
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

RvDecApiVersion RvDecGetAPIVersion(void);

RvDecBuild RvDecGetBuild(void);

RvDecRet RvDecInit(RvDecInst * dec_inst,
#ifdef USE_EXTERNAL_BUFFER
                   const void *dwl,
#endif
                   enum DecErrorHandling error_handling,
                   u32 frame_code_length,
                   u32 *frame_sizes,
                   u32 rv_version,
                   u32 max_frame_width, u32 max_frame_height,
                   u32 num_frame_buffers,
                   enum DecDpbFlags dpb_flags,
                   u32 use_adaptive_buffers,
                   u32 n_guard_size);

RvDecRet RvDecDecode(RvDecInst dec_inst,
                     RvDecInput * input,
                     RvDecOutput * output);

RvDecRet RvDecGetInfo(RvDecInst dec_inst, RvDecInfo * dec_info);

RvDecRet RvDecNextPicture(RvDecInst dec_inst,
                          RvDecPicture * picture,
                          u32 end_of_stream);
#ifdef USE_OUTPUT_RELEASE
RvDecRet RvDecPictureConsumed(RvDecInst dec_inst,
                              RvDecPicture * picture);

RvDecRet RvDecEndOfStream(RvDecInst dec_inst, u32 strm_end_flag);
#endif

void RvDecRelease(RvDecInst dec_inst);

RvDecRet RvDecPeek(RvDecInst dec_inst, RvDecPicture * picture);
#ifdef USE_EXTERNAL_BUFFER
RvDecRet RvDecGetBufferInfo(RvDecInst dec_inst, RvDecBufferInfo *mem_info);

RvDecRet RvDecAddBuffer(RvDecInst dec_inst, struct DWLLinearMem *info);
#endif
RvDecRet RvDecSetInfo(RvDecInst dec_inst, struct RvDecConfig *dec_cfg);
#ifdef USE_OUTPUT_RELEASE
RvDecRet RvDecAbort(RvDecInst dec_inst);

RvDecRet RvDecAbortAfter(RvDecInst dec_inst);
#endif

/*------------------------------------------------------------------------------
    Prototype of the API trace funtion. Traces all API entries and returns.
    This must be implemented by the application using the decoder API!
    Argument:
        string - trace message, a null terminated string
------------------------------------------------------------------------------*/
void RvDecTrace(const char *string);

#ifdef __cplusplus
}
#endif

#endif                       /* __RVDECAPI_H__ */
