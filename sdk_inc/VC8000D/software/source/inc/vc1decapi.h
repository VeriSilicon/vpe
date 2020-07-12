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

#ifndef VC1DECAPI_H
#define VC1DECAPI_H

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
typedef enum VC1DecRet {
  VC1DEC_OK = 0,                /* Operation successful.*/
  VC1DEC_PIC_RDY = 1,           /* Picture decoded.*/
  VC1DEC_STRM_PROCESSED = 2,    /* Stream handled, no picture finished.*/
  VC1DEC_HDRS_RDY = 3,          /* Stream headers decoded */
  VC1DEC_END_OF_SEQ = 4,        /* End of video sequence */
  VC1DEC_PIC_DECODED = 5,       /* Picture decoded */
  VC1DEC_RESOLUTION_CHANGED = 6,/* Resolution changed in
                                         multiresolution video */
  VC1DEC_NONREF_PIC_SKIPPED = 7,/* Skipped non-reference picture */

#ifdef USE_OUTPUT_RELEASE
  VC1DEC_END_OF_STREAM = 8,     /* Used in output release thread */
  VC1DEC_ABORTED = 9,           /* Abort the decoder */
#endif

#ifdef USE_EXTERNAL_BUFFER
  /** Waiting for external buffers allocated. */
  VC1DEC_WAITING_FOR_BUFFER = 10,
#endif
#ifdef USE_OUTPUT_RELEASE
  VC1DEC_FLUSHED = 11,
#endif

  VC1DEC_BUF_EMPTY = 12,
  VC1DEC_PARAM_ERROR = -1,      /* Function called with invalid
                                         parameters. */
  VC1DEC_NOT_INITIALIZED = -3,  /* Attempt to decode with
                                         uninitialized Decoder.*/
  VC1DEC_MEMFAIL = -4,          /* Memory failure. */
  VC1DEC_INITFAIL = -5,         /* Decoder initialization failure.
                                         Right shift is not signed. */
  VC1DEC_METADATA_FAIL = -6,    /* Supplied metadata is in wrong
                                         format */
  VC1DEC_STRM_ERROR = -7,       /* Stream error */
#ifdef USE_EXTERNAL_BUFFER
  VC1DEC_EXT_BUFFER_REJECTED = -9,       /**<\hideinitializer */
#endif
  VC1DEC_NO_DECODING_BUFFER = -10,  /* no available frame for decoding using */
  VC1DEC_HW_RESERVED = -254,
  VC1DEC_HW_TIMEOUT = -255,
  VC1DEC_HW_BUS_ERROR = -256,
  VC1DEC_SYSTEM_ERROR = -257,
  VC1DEC_DWL_ERROR = -258,

  VC1DEC_FORMAT_NOT_SUPPORTED = -1000
} VC1DecRet;

/*
* Container for the metadata of the stream.
* Contains a wide range of information about the stream, e.g. what kind of
* tools are needed to decode the stream.
*/
typedef struct {
  u32     max_coded_width;  /**< Specifies the maximum coded width in
                                  *  pixels of picture within the sequence.
                                  *  Valid range [2,8192] (even values).*/
  u32     max_coded_height; /**< Specifies the maximum coded height in
                                  *  pixels of picture within the sequence.
                                  *  Valid range [2,8192] (even values).*/
  u32     vs_transform;    /**< Indicates whether variable sized transform
                                  *  is enabled for the sequence. Valid range [0,1].*/
  u32     overlap;        /**< Indicates whether overlap smoothing is
                                  *  enabled for the sequence. Valid range [0,1].*/
  u32     sync_marker;     /**< Indicates whether there are syncronization markers in
                                  *  the stream. Valid range [0,1].*/
  u32     quantizer;      /**< Indicates quantizer type used for the
                                  *  sequence. Valid range [0,3].*/
  u32     frame_interp;     /**< Indicates whether the INTERPFRM flag (which
                                  *  provides information to display process)
                                  *  exists in the picture headers. Valid range [0,1].*/
  u32     max_bframes;     /**< Specifies the maximum amount of consecutive
                                  *  B-frames within the sequence. Valid range [0,7].*/
  u32     fast_uv_mc;       /**< Indicates whether the rounding of color
                                  *  difference motion vectors is enabled. Valid range [0,1].*/
  u32     extended_mv;     /**< Indicates whether extended motion
                                  *  vectors are enabled for the sequence. Valid range [0,1].*/
  u32     multi_res;       /**< Indicates whether frames may be coded
                                  *  at smaller resolutions than
                                  *  the specified frame resolution. Valid range [0,1].*/
  u32     range_red;       /**< Indicates whether range reduction is used
                                  *  in the sequence. Valid range [0,1].*/
  u32     dquant;         /**< Indicates whether the quantization step
                                  *  may vary within a frame. Valid range [0,2].*/
  u32     loop_filter;     /**< Indicates whether loop filtering is
                                  *  enabled for the sequence. Valid range [0,1].*/
  u32     profile;        /**< Specifies profile of the input video bitstream. */
} VC1DecMetaData;

/*
*   Decoder instance is used for identifying subsequent calls to api
*   functions.
*/
typedef const void *VC1DecInst;

/*
*  Decoder input structure.
*  This is a container to pass data to the Decoder.
*/
typedef struct {

  const u8* stream;     /* Pointer to the video stream. Decoder does
                                  not change the contents of stream buffer.*/
  addr_t stream_bus_address;  /* DMA bus address of the input stream */
  u32 stream_size;        /* Number of bytes in the stream buffer.*/
  u32 pic_id;             /**< User-defined identifier to bind into
                                 *  decoded picture. */
  u32 skip_non_reference; /* Flag to enable decoder skip non-reference
                               * frames to reduce processor load */
} VC1DecInput;

typedef struct {
  u32 data_left;
  u8 *p_stream_curr_pos;
  addr_t strm_curr_bus_address;
} VC1DecOutput;

/*
*   Decoder output structure.
*   This is a container for Decoder output data like decoded picture and its
*   dimensions.
*/
typedef struct {
  u32 key_picture;
  u32 pic_id;
  u32 decode_id[2];
  u32 pic_coding_type[2];
  u32 range_red_frm;

  u32 range_map_yflag;
  u32 range_map_y;
  u32 range_map_uv_flag;
  u32 range_map_uv;

  u32 interlaced;
  u32 field_picture;
  u32 top_field;

  u32 first_field;
  u32 repeat_first_field;
  u32 repeat_frame_count;

  u32 number_of_err_mbs;
  u32 anchor_picture;
  struct VC1OutputInfo {
    u32 frame_width;
    u32 frame_height;
    u32 coded_width;
    u32 coded_height;
    const u8 *output_picture;
    addr_t output_picture_bus_address;
    u32 pic_stride;
    u32 pic_stride_ch;
    enum DecPictureFormat output_format;
  } pictures[4];
} VC1DecPicture;

/* Version information. */
typedef struct {
  u32 major;    /* Decoder API major version number. */
  u32 minor;    /* Decoder API minor version number. */
} VC1DecApiVersion;

typedef struct DecSwHwBuild  VC1DecBuild;

/* decoder  output picture format */
typedef enum {
  VC1DEC_SEMIPLANAR_YUV420 = 0x020001,
  VC1DEC_TILED_YUV420 = 0x020002
} VC1DecOutFormat;

typedef struct {
  VC1DecOutFormat output_format; /* format of the output picture */
  u32 max_coded_width;
  u32 max_coded_height;
  u32 coded_width;
  u32 coded_height;
  u32 par_width;
  u32 par_height;
  u32 frame_rate_numerator;
  u32 frame_rate_denominator;
  u32 interlaced_sequence;
  enum DecDpbMode dpb_mode;         /* DPB mode; frame, or field interlaced */
#ifdef USE_EXTERNAL_BUFFER
  u32 buf_release_flag;
#endif
  u32 multi_buff_pp_size;
} VC1DecInfo;

#ifdef USE_EXTERNAL_BUFFER
/*!\struct VC1DecBufferInfo_
 * \brief Reference buffer information
 *
 * A structure containing the reference buffer information, filled by
 * VC1DecGetBufferInfo()
 *
 * \typedef VC1DecBufferInfo
 * A typename for #VC1DecBufferInfo_.
 */
typedef struct VC1DecBufferInfo_ {
  u32 next_buf_size;
  u32 buf_num;
  struct DWLLinearMem buf_to_free;
#ifdef ASIC_TRACE_SUPPORT
  u32 is_frame_buffer;
#endif
} VC1DecBufferInfo;
#endif

struct VC1DecConfig {
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

VC1DecApiVersion VC1DecGetAPIVersion(void);

VC1DecBuild VC1DecGetBuild(void);

VC1DecRet VC1DecInit(VC1DecInst* dec_inst,
#ifdef USE_EXTERNAL_BUFFER
                     const void *dwl,
#endif
                     const VC1DecMetaData* p_meta_data,
                     enum DecErrorHandling error_handling,
                     u32 num_frame_buffers,
                     enum DecDpbFlags dpb_flags,
                     u32 use_adaptive_buffers,
                     u32 n_guard_size);

VC1DecRet VC1DecDecode( VC1DecInst dec_inst,
                        const VC1DecInput* input,
                        VC1DecOutput* output);

void VC1DecRelease(VC1DecInst dec_inst);

VC1DecRet VC1DecGetInfo(VC1DecInst dec_inst, VC1DecInfo * dec_info);

VC1DecRet VC1DecUnpackMetaData( const u8 *p_buffer, u32 buffer_size,
                                VC1DecMetaData *p_meta_data );

VC1DecRet VC1DecNextPicture(VC1DecInst  dec_inst,
                            VC1DecPicture *picture,
                            u32 end_of_stream);

#ifdef USE_OUTPUT_RELEASE
VC1DecRet VC1DecPictureConsumed(VC1DecInst  dec_inst,
                                VC1DecPicture *picture);

VC1DecRet VC1DecEndOfStream(VC1DecInst dec_inst, u32 strm_end_flag);

VC1DecRet VC1DecAbort(VC1DecInst dec_inst);

VC1DecRet VC1DecAbortAfter(VC1DecInst dec_inst);
#endif

VC1DecRet VC1DecPeek(VC1DecInst  dec_inst, VC1DecPicture *picture);

#ifdef USE_EXTERNAL_BUFFER
VC1DecRet VC1DecGetBufferInfo(VC1DecInst dec_inst, VC1DecBufferInfo *mem_info);

VC1DecRet VC1DecAddBuffer(VC1DecInst dec_inst, struct DWLLinearMem *info);
#endif

VC1DecRet VC1DecSetInfo(VC1DecInst dec_inst, struct VC1DecConfig *dec_cfg);

/*------------------------------------------------------------------------------
    Prototype of the API trace funtion. Traces all API entries and returns.
    This must be implemented by the application using the decoder API!
    Argument:
        string - trace message, a null terminated string
------------------------------------------------------------------------------*/
void VC1DecTrace(const char *string);

#ifdef __cplusplus
}
#endif

#endif /* VC1DECAPI_H */
