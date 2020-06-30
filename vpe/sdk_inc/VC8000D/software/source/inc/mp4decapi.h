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

#ifndef __MP4DECAPI_H__
#define __MP4DECAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
#include "decapicommon.h"
#include "dwl.h"

/*------------------------------------------------------------------------------
    API type definitions
------------------------------------------------------------------------------*/

typedef enum MP4DecStrmFmt_ {
  MP4DEC_MPEG4,
  MP4DEC_SORENSON,
  MP4DEC_CUSTOM_1
} MP4DecStrmFmt;

/* Return values */
typedef enum MP4DecRet_ {
  MP4DEC_OK = 0,
  MP4DEC_STRM_PROCESSED = 1,
  MP4DEC_PIC_RDY = 2,
  MP4DEC_PIC_DECODED = 3,
  MP4DEC_HDRS_RDY = 4,
  MP4DEC_DP_HDRS_RDY = 5,
  MP4DEC_NONREF_PIC_SKIPPED = 6,/* Skipped non-reference picture */

#ifdef USE_OUTPUT_RELEASE
  MP4DEC_ABORTED = 7,
  MP4DEC_END_OF_STREAM = 8,
#endif

#ifdef USE_EXTERNAL_BUFFER
  /** Waiting for external buffers allocated. */
  MP4DEC_WAITING_FOR_BUFFER = 9,
#endif
#ifdef USE_OUTPUT_RELEASE
  MP4DEC_FLUSHED = 10,
#endif

  MP4DEC_BUF_EMPTY = 11,
  MP4DEC_VOS_END = 14,
  MP4DEC_HDRS_NOT_RDY = 15,

  MP4DEC_PARAM_ERROR = -1,
  MP4DEC_STRM_ERROR = -2,
  MP4DEC_NOT_INITIALIZED = -4,
  MP4DEC_MEMFAIL = -5,
  MP4DEC_INITFAIL = -6,
  MP4DEC_FORMAT_NOT_SUPPORTED = -7,
  MP4DEC_STRM_NOT_SUPPORTED = -8,
#ifdef USE_EXTERNAL_BUFFER
  MP4DEC_EXT_BUFFER_REJECTED = -9,       /**<\hideinitializer */
#endif
  MP4DEC_NO_DECODING_BUFFER = -10,  /* no available frame for decoding using */
  MP4DEC_HW_RESERVED = -254,
  MP4DEC_HW_TIMEOUT = -255,
  MP4DEC_HW_BUS_ERROR = -256,
  MP4DEC_SYSTEM_ERROR = -257,
  MP4DEC_DWL_ERROR = -258
} MP4DecRet;

/* decoder output picture format */
typedef enum MP4DecOutFormat_ {
  MP4DEC_SEMIPLANAR_YUV420 = 0x020001,
  MP4DEC_TILED_YUV420 = 0x020002
} MP4DecOutFormat;

typedef struct {
  u32 *virtual_address;
  addr_t bus_address;
} MP4DecLinearMem;

/* Decoder instance */
typedef void *MP4DecInst;

/* Input structure */
typedef struct MP4DecInput_ {
  const u8 *stream;       /* Pointer to stream to be decoded  */
  addr_t stream_bus_address; /* DMA bus address of the input stream */
  u32 data_len;        /* Number of bytes to be decoded                */
  u32 enable_deblock;  /* Enable deblocking of post processed picture  */
  /* NOTE: This parameter is not valid if the decoder
   * is not used in pipeline mode with the post
   * processor i.e. it has no effect on the
   * decoding process */

  u32 pic_id;
  u32 skip_non_reference; /* Flag to enable decoder skip non-reference
                               * frames to reduce processor load */
} MP4DecInput;

/* Time code */
typedef struct TimeCode_ {
  u32 hours;
  u32 minutes;
  u32 seconds;
  u32 time_incr;
  u32 time_res;
} MP4DecTime;

typedef struct MP4DecOutput_ {
  const u8  *strm_curr_pos;
  addr_t strm_curr_bus_address; /* DMA bus address location where the decoding
                                   ended */
  u32 data_left;
} MP4DecOutput;

/* stream info filled by MP4DecGetInfo */
typedef struct MP4DecInfo_ {
  u32 frame_width;
  u32 frame_height;
  u32 coded_width;
  u32 coded_height;
  u32 stream_format;
  u32 profile_and_level_indication;
  u32 video_format;
  u32 video_range;
  u32 par_width;
  u32 par_height;
  u32 user_data_voslen;
  u32 user_data_visolen;
  u32 user_data_vollen;
  u32 user_data_govlen;
  u32 interlaced_sequence;
  enum DecDpbMode dpb_mode;         /* DPB mode; frame, or field interlaced */
#ifdef USE_EXTERNAL_BUFFER
  u32 pic_buff_size;
#endif
  u32 multi_buff_pp_size;
  MP4DecOutFormat output_format;
  u32 gmc_support;
} MP4DecInfo;

/* User data type */
typedef enum {
  MP4DEC_USER_DATA_VOS = 0,
  MP4DEC_USER_DATA_VISO,
  MP4DEC_USER_DATA_VOL,
  MP4DEC_USER_DATA_GOV

} MP4DecUserDataType;

/* User data configuration */
typedef struct {
  MP4DecUserDataType user_data_type;
  u8  *p_user_data_vos;
  u32  user_data_vosmax_len;
  u8  *p_user_data_viso;
  u32  user_data_visomax_len;
  u8  *p_user_data_vol;
  u32  user_data_volmax_len;
  u8  *p_user_data_gov;
  u32  user_data_govmax_len;
} MP4DecUserConf;

/* Version information */
typedef struct MP4DecVersion_ {
  u32 major;    /* API major version */
  u32 minor;    /* API minor version */
} MP4DecApiVersion;

typedef struct DecSwHwBuild  MP4DecBuild;

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
typedef struct MP4DecBufferInfo_ {
  u32 next_buf_size;
  u32 buf_num;
  struct DWLLinearMem buf_to_free;
#ifdef ASIC_TRACE_SUPPORT
  u32 is_frame_buffer;
#endif
} MP4DecBufferInfo;
#endif

typedef struct {
  u32 key_picture;
  u32 pic_id;
  u32 decode_id;
  u32 pic_coding_type;
  u32 interlaced;
  u32 field_picture;
  u32 top_field;
  u32 nbr_of_err_mbs;
  MP4DecTime time_code;
  struct MP4OutputInfo {
    const u8 *output_picture;
    addr_t output_picture_bus_address;
    u32 frame_width;
    u32 frame_height;
    u32 coded_width;
    u32 coded_height;
    u32 pic_stride;
    u32 pic_stride_ch;
    enum DecPictureFormat output_format;
  } pictures[4];
} MP4DecPicture;

struct MP4DecConfig {
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

MP4DecApiVersion MP4DecGetAPIVersion(void);

MP4DecBuild MP4DecGetBuild(void);

MP4DecRet MP4DecInit(MP4DecInst * dec_inst,
#ifdef USE_EXTERNAL_BUFFER
                     const void *dwl,
#endif
                     MP4DecStrmFmt strm_fmt,
                     enum DecErrorHandling error_handling,
                     u32 num_frame_buffers,
                     enum DecDpbFlags dpb_flags,
                     u32 use_adaptive_buffers,
                     u32 n_guard_size);

MP4DecRet MP4DecDecode(MP4DecInst dec_inst,
                       const MP4DecInput   * input,
                       MP4DecOutput        * output);

MP4DecRet MP4DecSetCustomInfo(MP4DecInst * dec_inst,
                        const u32 width,
                        const u32 height );

MP4DecRet MP4DecGetInfo(MP4DecInst dec_inst,
                        MP4DecInfo  * dec_info);

MP4DecRet MP4DecGetUserData(MP4DecInst        dec_inst,
                            const MP4DecInput * input,
                            MP4DecUserConf    * p_user_data_config);

MP4DecRet MP4DecNextPicture(MP4DecInst        dec_inst,
                            MP4DecPicture    *picture,
                            u32               end_of_stream);

#ifdef USE_OUTPUT_RELEASE
MP4DecRet MP4DecPictureConsumed(MP4DecInst    dec_inst,
                                MP4DecPicture *picture);

MP4DecRet MP4DecEndOfStream(MP4DecInst dec_inst, u32 strm_end_flag);
#endif

void  MP4DecRelease(MP4DecInst dec_inst);

MP4DecRet MP4DecPeek(MP4DecInst        dec_inst,
                     MP4DecPicture    *picture);
#ifdef USE_EXTERNAL_BUFFER
MP4DecRet MP4DecGetBufferInfo(MP4DecInst dec_inst, MP4DecBufferInfo *mem_info);

MP4DecRet MP4DecAddBuffer(MP4DecInst dec_inst, struct DWLLinearMem *info);
#endif

MP4DecRet MP4DecSetInfo(MP4DecInst dec_inst, struct MP4DecConfig *dec_cfg);

#ifdef USE_OUTPUT_RELEASE
MP4DecRet MP4DecAbort(MP4DecInst dec_inst);

MP4DecRet MP4DecWaitAfter(MP4DecInst dec_inst);
#endif

/*------------------------------------------------------------------------------
    Prototype of the API trace funtion. Traces all API entries and returns.
    This must be implemented by the application using the decoder API!
    Argument:
        string - trace message, a null terminated string
------------------------------------------------------------------------------*/
void MP4DecTrace(const char *string);

#ifdef __cplusplus
}
#endif

#endif /* __MP4DECAPI_H__ */
