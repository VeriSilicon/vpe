/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Holdings Co., Ltd. All rights reserved        --
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

#ifndef __VP6DECAPI_H__
#define __VP6DECAPI_H__

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
typedef enum VP6DecRet_ {
  VP6DEC_OK = 0,
  VP6DEC_STRM_PROCESSED = 1,
  VP6DEC_PIC_RDY = 2,
  VP6DEC_PIC_DECODED = 3,
  VP6DEC_HDRS_RDY = 4,
  VP6DEC_ADVANCED_TOOLS = 5,
#ifdef USE_OUTPUT_RELEASE
  VP6DEC_ABORTED = 7,
  VP6DEC_END_OF_STREAM = 8,
#endif

#ifdef USE_EXTERNAL_BUFFER
  /** Waiting for external buffers allocated. */
  VP6DEC_WAITING_FOR_BUFFER = 9,
#endif
#ifdef USE_OUTPUT_RELEASE
  VP6DEC_FLUSHED = 10,
#endif

  VP6DEC_PARAM_ERROR = -1,
  VP6DEC_STRM_ERROR = -2,
  VP6DEC_NOT_INITIALIZED = -3,
  VP6DEC_MEMFAIL = -4,
  VP6DEC_INITFAIL = -5,
  VP6DEC_HDRS_NOT_RDY = -6,
  VP6DEC_STREAM_NOT_SUPPORTED = -8,
#ifdef USE_EXTERNAL_BUFFER
  VP6DEC_EXT_BUFFER_REJECTED = -9, /**<\hideinitializer */
#endif
  VP6DEC_NO_DECODING_BUFFER = -10,  /** no available frame for decoding using */
  VP6DEC_HW_RESERVED = -254,
  VP6DEC_HW_TIMEOUT = -255,
  VP6DEC_HW_BUS_ERROR = -256,
  VP6DEC_SYSTEM_ERROR = -257,
  VP6DEC_DWL_ERROR = -258,

  VP6DEC_EVALUATION_LIMIT_EXCEEDED = -999,
  VP6DEC_FORMAT_NOT_SUPPORTED = -1000
} VP6DecRet;

/* decoder  output Frame format */
typedef enum VP6DecOutFormat_ {
  VP6DEC_SEMIPLANAR_YUV420 = 0x020001,
  VP6DEC_TILED_YUV420 = 0x020002
} VP6DecOutFormat;

/* Input structure */
typedef struct VP6DecInput_ {
  const u8 *stream;   /* Pointer to the input data */
  addr_t stream_bus_address;   /* DMA bus address of the input stream */
  u32 data_len;         /* Number of bytes to be decoded         */
  u32 pic_id;
} VP6DecInput;

/* Output structure */
typedef struct VP6DecOutput_ {
  u32 data_left;
} VP6DecOutput;

#define VP6_SCALE_MAINTAIN_ASPECT_RATIO     0
#define VP6_SCALE_TO_FIT                    1
#define VP6_SCALE_CENTER                    2
#define VP6_SCALE_OTHER                     3

/* stream info filled by VP6DecGetInfo */
typedef struct VP6DecInfo_ {
  u32 vp6_version;
  u32 vp6_profile;
#ifdef USE_EXTERNAL_BUFFER
  u32 pic_buff_size;
#endif
  u32 frame_width;      /* coded width */
  u32 frame_height;     /* coded height */
  u32 scaled_width;     /* scaled width of the displayed video */
  u32 scaled_height;    /* scaled height of the displayed video */
  u32 scaling_mode;     /* way to scale the frame to output */
  enum DecDpbMode dpb_mode;             /* DPB mode; frame, or field interlaced */
  VP6DecOutFormat output_format;   /* format of the output frame */
} VP6DecInfo;

/* Version information */
typedef struct VP6DecApiVersion_ {
  u32 major;           /* API major version */
  u32 minor;           /* API minor version */
} VP6DecApiVersion;

typedef struct DecSwHwBuild  VP6DecBuild;

/* Output structure for VP6DecNextPicture */
typedef struct VP6DecPicture_ {
  u32 frame_width;        /* pixels width of the frame as stored in memory */
  u32 frame_height;       /* pixel height of the frame as stored in memory */
  u32 coded_width;
  u32 coded_height;
  const u32 *p_output_frame;    /* Pointer to the frame */
  addr_t output_frame_bus_address;  /* DMA bus address of the output frame buffer */
  u32 pic_id;           /* Identifier of the Frame to be displayed */
  u32 decode_id;
  u32 pic_stride;
  u32 pic_coding_type;   /* Picture coding type */
  u32 is_intra_frame;    /* Indicates if Frame is an Intra Frame */
  u32 is_golden_frame;   /* Indicates if Frame is a Golden reference Frame */
  u32 nbr_of_err_mbs;     /* Number of concealed MB's in the frame  */
  enum DecPictureFormat output_format;
} VP6DecPicture;
#ifdef USE_EXTERNAL_BUFFER
/*!\struct VP6DecBufferInfo_
 * \brief Reference buffer information
 *
 * A structure containing the reference buffer information, filled by
 * VP6DecGetBufferInfo()
 *
 * \typedef VP6DecBufferInfo
 * A typename for #VP6DecBufferInfo_.
 */
typedef struct VP6DecBufferInfo_ {
  u32 next_buf_size;
  u32 buf_num;
  struct DWLLinearMem buf_to_free;
#ifdef ASIC_TRACE_SUPPORT
  u32 is_frame_buffer;
#endif
} VP6DecBufferInfo;
#endif

struct VP6DecConfig {
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
/* Decoder instance */
typedef const void *VP6DecInst;

/*------------------------------------------------------------------------------
    Prototypes of Decoder API functions
------------------------------------------------------------------------------*/
VP6DecApiVersion VP6DecGetAPIVersion(void);

VP6DecBuild VP6DecGetBuild(void);

VP6DecRet VP6DecInit(VP6DecInst * dec_inst,
#ifdef USE_EXTERNAL_BUFFER
                     const void *dwl,
#endif
                     enum DecErrorHandling error_handling,
                     u32 num_frame_buffers,
                     enum DecDpbFlags dpb_flags,
                     u32 use_adaptive_buffers,
                     u32 n_guard_size);

void VP6DecRelease(VP6DecInst dec_inst);

VP6DecRet VP6DecDecode(VP6DecInst dec_inst,
                       const VP6DecInput * input, VP6DecOutput * output);

VP6DecRet VP6DecNextPicture(VP6DecInst dec_inst,
                            VP6DecPicture * output, u32 end_of_stream);
#ifdef USE_OUTPUT_RELEASE
VP6DecRet VP6DecPictureConsumed(VP6DecInst dec_inst, VP6DecPicture * output);

VP6DecRet VP6DecEndOfStream(VP6DecInst dec_inst, u32 strm_end_flag);

VP6DecRet VP6DecAbort(VP6DecInst dec_inst);

VP6DecRet VP6DecWaitAfter(VP6DecInst dec_inst);
#endif

VP6DecRet VP6DecGetInfo(VP6DecInst dec_inst, VP6DecInfo * dec_info);

VP6DecRet VP6DecPeek(VP6DecInst dec_inst, VP6DecPicture * output);
#ifdef USE_EXTERNAL_BUFFER
VP6DecRet VP6DecGetBufferInfo(VP6DecInst dec_inst, VP6DecBufferInfo *mem_info);

VP6DecRet VP6DecAddBuffer(VP6DecInst dec_inst, struct DWLLinearMem *info);
#endif

VP6DecRet VP6DecSetInfo(VP6DecInst dec_inst, struct VP6DecConfig *dec_cfg);
/*------------------------------------------------------------------------------
    Prototype of the API trace funtion. Traces all API entries and returns.
    This must be implemented by the application using the decoder API!
    Argument:
        string - trace message, a null terminated string
------------------------------------------------------------------------------*/
void VP6DecTrace(const char *string);

#ifdef __cplusplus
}
#endif

#endif                       /* __VP6DECAPI_H__ */
