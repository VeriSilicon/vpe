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

#ifndef __AVSDECAPI_H__
#define __AVSDECAPI_H__

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
  AVSDEC_OK = 0,
  AVSDEC_STRM_PROCESSED = 1,
  AVSDEC_PIC_RDY = 2,
  AVSDEC_HDRS_RDY = 3,
  AVSDEC_HDRS_NOT_RDY = 4,
  AVSDEC_PIC_DECODED = 5,
  AVSDEC_NONREF_PIC_SKIPPED = 6,/* Skipped non-reference picture */
#ifdef USE_OUTPUT_RELEASE
  AVSDEC_ABORTED = 7,
  AVSDEC_END_OF_STREAM = 8,
#endif

#ifdef USE_EXTERNAL_BUFFER
  /** Waiting for external buffers allocated. */
  AVSDEC_WAITING_FOR_BUFFER = 9,
#endif
#ifdef USE_OUTPUT_RELEASE
  AVSDEC_FLUSHED = 10,
#endif
  AVSDEC_BUF_EMPTY = 11,
  AVSDEC_PARAM_ERROR = -1,
  AVSDEC_STRM_ERROR = -2,
  AVSDEC_NOT_INITIALIZED = -3,
  AVSDEC_MEMFAIL = -4,
  AVSDEC_INITFAIL = -5,
  AVSDEC_STREAM_NOT_SUPPORTED = -8,
#ifdef USE_EXTERNAL_BUFFER
  AVSDEC_EXT_BUFFER_REJECTED = -9, /**<\hideinitializer */
#endif
  AVSDEC_NO_DECODING_BUFFER = -10,  /* no available frame for decoding using */
  AVSDEC_HW_RESERVED = -254,
  AVSDEC_HW_TIMEOUT = -255,
  AVSDEC_HW_BUS_ERROR = -256,
  AVSDEC_SYSTEM_ERROR = -257,
  AVSDEC_DWL_ERROR = -258,
  AVSDEC_FORMAT_NOT_SUPPORTED = -1000
} AvsDecRet;

/* decoder output picture format */
typedef enum {
  AVSDEC_SEMIPLANAR_YUV420 = 0x020001,
  AVSDEC_TILED_YUV420 = 0x020002
} AvsDecOutFormat;

/* DAR (Display aspect ratio) */
typedef enum {
  AVSDEC_1_1 = 0x01,
  AVSDEC_4_3 = 0x02,
  AVSDEC_16_9 = 0x03,
  AVSDEC_2_21_1 = 0x04
} AvsDecDARFormat;

/* SAR (Sample aspect ratio) */
/* TODO! */

typedef struct {
  u32 *virtual_address;
  addr_t bus_address;
} AvsDecLinearMem;

/* Decoder instance */
typedef void *AvsDecInst;

/* Input structure */
typedef struct {
  u8 *stream;         /* Pointer to stream to be decoded              */
  addr_t stream_bus_address;   /* DMA bus address of the input stream */
  u32 data_len;         /* Number of bytes to be decoded                */
  u32 pic_id;
  u32 skip_non_reference; /* Flag to enable decoder skip non-reference
                               * frames to reduce processor load */
} AvsDecInput;

/* Time code */
typedef struct {
  u32 hours;
  u32 minutes;
  u32 seconds;
  u32 pictures;
} AvsDecTime;

typedef struct {
  u8 *strm_curr_pos;
  addr_t strm_curr_bus_address; /* DMA bus address location where the decoding
                                 * ended */
  u32 data_left;
} AvsDecOutput;

/* stream info filled by AvsDecGetInfo */
typedef struct {
  u32 frame_width;
  u32 frame_height;
  u32 coded_width;
  u32 coded_height;
  u32 profile_id;
  u32 level_id;
  u32 display_aspect_ratio;
  u32 video_format;
  u32 video_range;
  u32 interlaced_sequence;
  enum DecDpbMode dpb_mode;         /* DPB mode; frame, or field interlaced */
#ifdef USE_EXTERNAL_BUFFER
  u32 pic_buff_size;
#endif
  u32 multi_buff_pp_size;
  AvsDecOutFormat output_format;
} AvsDecInfo;

typedef struct {
  u32 key_picture;
  u32 pic_id;
  u32 decode_id;
  u32 pic_coding_type;
  u32 interlaced;
  u32 field_picture;
  u32 top_field;
  u32 first_field;
  u32 repeat_first_field;
  u32 repeat_frame_count;
  u32 number_of_err_mbs;
  AvsDecTime time_code;
  struct AvsOutputInfo {
    u8 *output_picture;
    addr_t output_picture_bus_address;
    u32 frame_width;
    u32 frame_height;
    u32 coded_width;
    u32 coded_height;
    u32 pic_stride;
    u32 pic_stride_ch;
    enum DecPictureFormat output_format;
  } pictures[4];
} AvsDecPicture;

/* Version information */
typedef struct {
  u32 major;           /* API major version */
  u32 minor;           /* API minor version */

} AvsDecApiVersion;

typedef struct DecSwHwBuild  AvsDecBuild;
#ifdef USE_EXTERNAL_BUFFER
/*!\struct AvsDecBufferInfo_
 * \brief Reference buffer information
 *
 * A structure containing the reference buffer information, filled by
 * AvsDecGetBufferInfo()
 *
 * \typedef AvsDecBufferInfo
 * A typename for #AvsDecBufferInfo_.
 */
typedef struct AvsDecBufferInfo_ {
  u32 next_buf_size;
  u32 buf_num;
  struct DWLLinearMem buf_to_free;
#ifdef ASIC_TRACE_SUPPORT
  u32 is_frame_buffer;
#endif
} AvsDecBufferInfo;
#endif

struct AvsDecConfig {
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
  PpUnitConfig ppu_config[4];
};
/*------------------------------------------------------------------------------
    Prototypes of Decoder API functions
------------------------------------------------------------------------------*/

AvsDecApiVersion AvsDecGetAPIVersion(void);

AvsDecBuild AvsDecGetBuild(void);

AvsDecRet AvsDecInit(AvsDecInst * dec_inst,
#ifdef USE_EXTERNAL_BUFFER
                     const void *dwl,
#endif
                     enum DecErrorHandling error_handling,
                     u32 num_frame_buffers,
                     enum DecDpbFlags dpb_flags,
                     u32 use_adaptive_buffers,
                     u32 n_guard_size);

AvsDecRet AvsDecDecode(AvsDecInst dec_inst,
                       AvsDecInput * input,
                       AvsDecOutput * output);

AvsDecRet AvsDecGetInfo(AvsDecInst dec_inst, AvsDecInfo * dec_info);

AvsDecRet AvsDecNextPicture(AvsDecInst dec_inst,
                            AvsDecPicture * picture,
                            u32 end_of_stream);

#ifdef USE_OUTPUT_RELEASE
AvsDecRet AvsDecPictureConsumed(AvsDecInst dec_inst,
                                AvsDecPicture * picture);

AvsDecRet AvsDecEndOfStream(AvsDecInst dec_inst, u32 strm_end_flag);
#endif

void AvsDecRelease(AvsDecInst dec_inst);

AvsDecRet AvsDecPeek(AvsDecInst dec_inst, AvsDecPicture * picture);
#ifdef USE_EXTERNAL_BUFFER
AvsDecRet AvsDecGetBufferInfo(AvsDecInst dec_inst, AvsDecBufferInfo *mem_info);

AvsDecRet AvsDecAddBuffer(AvsDecInst dec_inst, struct DWLLinearMem *info);
#endif
#ifdef USE_OUTPUT_RELEASE
AvsDecRet AvsDecAbort(AvsDecInst dec_inst);

AvsDecRet AvsDecAbortAfter(AvsDecInst dec_inst);
#endif

AvsDecRet AvsDecSetInfo(AvsDecInst dec_inst, struct AvsDecConfig *dec_cfg);

/*------------------------------------------------------------------------------
    Prototype of the API trace funtion. Traces all API entries and returns.
    This must be implemented by the application using the decoder API!
    Argument:
        string - trace message, a null terminated string
------------------------------------------------------------------------------*/
void AvsDecTrace(const char *string);

#ifdef __cplusplus
}
#endif

#endif                       /* __AVSDECAPI_H__ */
