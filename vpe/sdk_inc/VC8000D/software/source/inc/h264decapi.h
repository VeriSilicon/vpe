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

#ifndef __H264DECAPI_H__
#define __H264DECAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
#include "decapicommon.h"
#include "dectypes.h"
#include "dwl.h"

/*!\defgroup H264API H.264 Decoder API */

/*! \addtogroup H264API
 *  @{
 */

/*------------------------------------------------------------------------------
    API type definitions
------------------------------------------------------------------------------*/

/*!\typedef H264DecInst
 * \brief Decoder instance
 */
typedef const void *H264DecInst;

/*!\struct H264DecInput_
 * \brief Decode input structure
 *
 * \typedef H264DecInput
 * A typename for #H264DecInput_.
 */
typedef struct H264DecInput_ {
  const u8 *stream;   /**< Pointer to the input buffer*/
  addr_t stream_bus_address; /**< DMA bus address of the input buffer */
  u32 data_len;          /**< Number of bytes to be decoded         */
  u32 buffer_len;
  u32 pic_id;            /**< Identifier for the next picture to be decoded */
  u32 skip_non_reference; /**< Flag to enable decoder to skip non-reference
                               * frames to reduce processor load */
  void *p_user_data; /* user data to be passed in multicore callback */
} H264DecInput;

/*!\struct H264DecOutput_
 * \brief Decode output structure
 *
 * \typedef H264DecOutput
 * A typename for #H264DecOutput_.
 */
typedef struct H264DecOutput_ {
  u8 *strm_curr_pos;    /**< Pointer to stream position where decoding ended */
  addr_t strm_curr_bus_address; /**< DMA bus address location where the decoding ended */
  u32 data_left;        /**< how many bytes left unprocessed */
} H264DecOutput;

/*!\struct H264CropParams_
 * \brief Picture cropping information
 *
 * \typedef H264CropParams
 * A typename for #H264CropParams_.
 */
typedef struct H264CropParams_ {
  u32 crop_left_offset;
  u32 crop_out_width;
  u32 crop_top_offset;
  u32 crop_out_height;
} H264CropParams;

/*!\struct H264DecPicture_
 * \brief Decoded picture information
 *
 * Parameters of a decoded picture, filled by H264DecNextPicture().
 *
 * \typedef H264DecPicture
 * A typename for #H264DecPicture_.
 */
typedef struct H264DecPicture_ {
  u32 sar_width;        /**< pixels width of the picture as stored in memory */
  u32 sar_height;       /**< pixel height of the picture as stored in memory */
  H264CropParams crop_params;  /**< cropping parameters */
  u32 pic_id;           /**< Identifier of the picture to be displayed */
  u32 decode_id[2];
  u32 pic_coding_type[2];   /**< Picture coding type */
  u32 is_idr_picture[2];    /**< Indicates if picture is an IDR picture */
  u32 nbr_of_err_mbs;     /**< Number of concealed MB's in the picture  */
  u32 interlaced;      /**< flag, non-zero for interlaced picture */
  u32 field_picture;    /**< flag, non-zero if interlaced and only one field present */
  u32 top_field;        /**< flag, if only one field, non-zero signals TOP field otherwise BOTTOM */
  u32 view_id;          /**< Identifies the view to which the output picture belongs */
  u32 pic_struct;       /**< pic_struct extracted from pic timing SEI */
  u32 bit_depth_luma;   /**< bit depth of luma plane */
  u32 bit_depth_chroma; /**< bit depth of chroma plane */
  u32 cycles_per_mb;
  struct H264OutputInfo {
    u32 pic_width;        /**< pixels width of the picture as stored in memory */
    u32 pic_height;       /**< pixel height of the picture as stored in memory */
    u32 pic_stride;       /** < pic stride of each pixel line in bytes. */
    u32 pic_stride_ch;
    const u32 *output_picture;  /**< Pointer to the picture data */
    addr_t output_picture_bus_address;    /**< DMA bus address of the output picture buffer */
    const u32 *output_picture_chroma;  /**< Pointer to the chroma picture data */
    addr_t output_picture_chroma_bus_address;    /**< DMA bus address of the output picture buffer */
	const u32 *output_picture_Y_table;  /**< Pointer to the picture data */
	addr_t output_picture_Y_table_bus_address;    /**< DMA bus address of the output picture buffer */
	u32 output_picture_Y_table_size;
	const u32 *output_picture_UV_table;  /**< Pointer to the picture data */
	addr_t output_picture_UV_table_bus_address;    /**< DMA bus address of the output picture buffer */
	u32 output_picture_UV_table_size;
	u32 pic_compressed_status;
    enum DecPictureFormat output_format; /**< Storage format of output picture. */
  } pictures[DEC_MAX_OUT_COUNT];
} H264DecPicture;

/*!\struct H264DecInfo_
 * \brief Decoded stream information
 *
 * A structure containing the decoded stream information, filled by
 * H264DecGetInfo()
 *
 * \typedef H264DecInfo
 * A typename for #H264DecInfo_.
 */
typedef struct H264DecInfo_ {
  u32 pic_width;        /**< decoded picture width in pixels */
  u32 pic_height;       /**< decoded picture height in pixels */
  u32 video_range;      /**< samples' video range */
  u32 matrix_coefficients;
  H264CropParams crop_params;  /**< display cropping information */
  enum DecPictureFormat output_format;  /**< format of the output picture */
  u32 sar_width;        /**< sample aspect ratio */
  u32 sar_height;       /**< sample aspect ratio */
  u32 mono_chrome;      /**< is sequence monochrome */
  u32 interlaced_sequence;      /**< is sequence interlaced */
  u32 dpb_mode;         /**< DPB mode; frame, or field interlaced */
  u32 pic_buff_size;     /**< number of picture buffers allocated and used by decoder */
  u32 multi_buff_pp_size; /**< number of picture buffers needed in decoder+postprocessor multibuffer mode */
  u32 bit_depth_luma;
  u32 bit_depth_chroma;
} H264DecInfo;

/*!\struct H264DecApiVersion_
 * \brief API Version information
 *
 * A structure containing the major and minor version number of the API.
 *
 * \typedef H264DecApiVersion
 * A typename for #H264DecApiVersion_.
 */
typedef struct H264DecApiVersion_ {
  u32 major;           /**< API major version */
  u32 minor;           /**< API minor version */
} H264DecApiVersion;

/*!\typedef H264DecBuild
 * \brief Build information
 *
 * A typename for #DecSwHwBuild containing the build information
 * of the decoder.
 */
typedef struct DecSwHwBuild  H264DecBuild;

typedef struct {
  u32 low_latency;
  addr_t strm_bus_addr;
  addr_t strm_bus_start_addr;
  u8* strm_vir_addr;
  u8* strm_vir_start_addr;
  u32 last_flag;
}strmInfo;


#ifdef USE_EXTERNAL_BUFFER
/*!\struct H264DecBufferInfo_
 * \brief Reference buffer information
 *
 * A structure containing the reference buffer information, filled by
 * H264DecGetBufferInfo()
 *
 * \typedef H264DecBufferInfo
 * A typename for #H264DecBufferInfo_.
 */
typedef struct H264DecBufferInfo_ {
  u32 next_buf_size;
  u32 buf_num;
  struct DWLLinearMem buf_to_free;
} H264DecBufferInfo;
#endif

/*!\brief Stream consumed callback prototype
 *
 * This callback is invoked by the decoder to notify the application that
 * a stream buffer was fully processed and can be reused.
 *
 * \param stream base address of a buffer that was set as input when
 *                calling H264DecDecode().
 * \param p_user_data application provided pointer to some private data.
 *                  This is set at decoder initialization time.
 *
 * \sa H264DecMCInit();
 */
typedef void H264DecMCStreamConsumed(void *stream, void *p_user_data);

/*!\struct H264DecMCConfig_
 * \brief Multicore decoder init configuration
 *
 * \typedef H264DecMCConfig
 *  A typename for #H264DecMCConfig_.
 */
typedef struct H264DecMCConfig_ {
  u32 mc_enable;
  /*! Application provided callback for stream buffer processed. */
  H264DecMCStreamConsumed *stream_consumed_callback;

} H264DecMCConfig;

struct H264DecConfig {
  u32 no_output_reordering;
  enum DecErrorHandling error_handling;
  u32 use_display_smoothing;
  enum DecDpbFlags dpb_flags;
  enum DecDecoderMode decoder_mode;
#ifdef USE_EXTERNAL_BUFFER
  u32 use_adaptive_buffers; // When sequence changes, if old output buffers (number/size) are sufficient for new sequence,
  // old buffers will be used instead of reallocating output buffer.
  u32 guard_size;       // The minimum difference between minimum buffers number and allocated buffers number
  // that will force to return HDRS_RDY even buffers number/size are sufficient
  // for new sequence.
#endif
  DecPicAlignment align;
  u32 error_conceal;
  PpUnitConfig ppu_config[4];
};

/*------------------------------------------------------------------------------
    Prototypes of Decoder API functions
------------------------------------------------------------------------------*/

/*!\brief Get API version information
 *
 * Return the version information of the SW API.
 * Static implementation, does not require a decoder instance.
 */
H264DecApiVersion H264DecGetAPIVersion(void);

/*!\brief Read SW/HW build information
 *
 * Returns the hardware and software build information of the decoder.
 * Static implementation, does not require a decoder instance.
 */
H264DecBuild H264DecGetBuild(void *dwl_inst);

/*!\brief Create a single core decoder instance
 *
 * Single core decoder can decode both byte streams and NAL units.
 * FMO and ASO streams are supported also, but these will require an
 * internal switch to less hardware acceleration. For FMO and ASO stream
 * the entropy decoding is done in software.
 *
 * Every instance has to be released with H264DecRelease().
 *
 *\note Use H264DecMCInit() for creating an instance with multicore support.
 *
 */
enum DecRet H264DecInit(H264DecInst *dec_inst,
#ifdef USE_EXTERNAL_BUFFER
                       const void *dwl,
#endif
                       enum DecDecoderMode decoder_mode,
                       u32 no_output_reordering,
                       enum DecErrorHandling error_handling,
                       u32 use_display_smoothing,
                       enum DecDpbFlags dpb_flags,
                       u32 cr_first,
                       u32 use_adaptive_buffers,
                       u32 n_guard_size,
                        H264DecMCConfig *p_mcinit_cfg);


/*!\brief Set decoder info (configuration)
 *
 * Set decoder configuration including PP parameters.
 *
 *\note To be called after H264DecGetInfo().
 *
 */
enum DecRet H264DecSetInfo(H264DecInst dec_inst,
                          struct H264DecConfig *dec_cfg);


/*!\brief Enable MVC decoding
 *
 * Use this to enable decoding of MVC streams. If not enabled, the decoder
 * can only decode the base view of an MVC streams.
 * MVC decoding has to be enabled  before any attempt to decode stream data
 * with H264DecDecode().
 *
 * \retval #DEC_OK for success.
 * \returns A negative return value signals an error.
 */
enum DecRet H264DecSetMvc(H264DecInst dec_inst);

/*!\brief Release a decoder instance
 *
 * H264DecRelease closes the decoder instance \c dec_inst and releases all
 * internally allocated resources.
 *
 * When connected with the Hantro HW post-processor, the post-processor
 * must be disconnected before releasing the decoder instance.
 * Refer to \ref PPDOC "PP API manual" for details.
 *
 * \param dec_inst instance to be released
 *
 * \retval #DEC_OK for success
 * \returns A negative return value signals an error.
 *
 * \sa H264DecInit()
 * \sa H264DecMCInit()
 *
 */
void H264DecRelease(H264DecInst dec_inst);

/*!\brief Decode data
 *
 * This function decodes one or more NAL units from the current stream.
 * The input buffer shall contain one of the following:
 *      - Exactly one NAL unit and nothing else.
 *      - One or more NAL units in byte stream format, as defined in
 *        Annex B of the standard [1].
 *
 *  The decoder automatically detects the format of the stream data and
 *  decodes NAL units until the whole buffer is processed or decoding of
 *  a picture is finished. The calling application may set \e pic_id field
 *  in the input structure to a unique value and use this to link a picture
 *  obtained from #H264DecNextPicture to a certain decoded stream data.
 *  This might be useful for example when the application obtains
 *  composition or display time for pictures by external means and needs
 *  to know when to display a picture returned by #H264DecNextPicture.
 *
 * \retval #DEC_STRM_PROCESSED
            All the data in the stream buffer processed. Stream buffer must
            be updated before calling #H264DecDecode again.
 * \retval #DEC_PIC_DECODED
            A new picture decoded. Single core mode has to call
             #H264DecNextPicture to check if there are any pictures
             available for displaying.
 * \retval #DEC_HDRS_RDY
 *          Headers decoded and activated. Stream header information is now
 *          readable with the function #H264DecGetInfo.
 * \retval #DEC_ADVANCED_TOOLS
 *          Current stream utilizes advanced coding tools, ASO and/or FMO.
 *          Decoder starts entropy decoding in software (much slower) and
 *          has to reinitialize.
 *
 * \returns A negative return value signals an error.
 */
enum DecRet H264DecDecode(H264DecInst dec_inst,
                         const H264DecInput *input,
                         H264DecOutput *output);

/*!\brief Read next picture in display order
 *
 * \warning Do not use with multicore instances!
 *       Use instead H264DecMCNextPicture().
 *
 * \sa H264DecMCNextPicture()
 */
enum DecRet H264DecNextPicture(H264DecInst dec_inst,
                              H264DecPicture *picture, u32 end_of_stream);

/*!\brief Read decoded stream information
 *
 */
#ifdef USE_OUTPUT_RELEASE
/*!\brief Release the picture buffer after H264DecNextPicture() called
 *
 * \warning Do not use with multicore instances!
 *       Use instead H264DecMCPictureConsumed().
 */
enum DecRet H264DecPictureConsumed(H264DecInst dec_inst,
                                  const H264DecPicture *picture);

enum DecRet H264DecEndOfStream(H264DecInst dec_inst, u32 strm_end_flag);
enum DecRet H264DecAbort(H264DecInst dec_inst);
enum DecRet H264DecAbortAfter(H264DecInst dec_inst);
#endif

enum DecRet H264DecGetInfo(H264DecInst dec_inst, H264DecInfo *dec_info);

/*!\brief Read last decoded picture
 *
 * \warning Do not use with multicore instances!
 *       Use instead H264DecMCNextPicture().
 *
 * \sa H264DecMCNextPicture()
 */
enum DecRet H264DecPeek(H264DecInst dec_inst, H264DecPicture *picture);

#ifdef USE_EXTERNAL_BUFFER

/*!\brief Add externally allocated memories to reference buffer
*
*/
enum DecRet H264DecAddBuffer(H264DecInst dec_inst, struct DWLLinearMem *info);


/*!\brief Read reference buffer information
 *
 */
enum DecRet H264DecGetBufferInfo(H264DecInst dec_inst, H264DecBufferInfo *mem_info);
#endif

/*!\brief Let decoder output just as decoding order, that is for SCAN.
 */
enum DecRet H264DecSetNoReorder(H264DecInst dec_inst, u32 no_output_reordering);


void H264DecUpdateStrm(strmInfo info);
void H264DecUpdateStrmInfoCtrl(H264DecInst dec_inst, u32 last_flag, u32 strm_bus_addr);

/*!\brief Read number of HW cores available
 *
 * Multicore specific.
 *
 * Static implementation, does not require a decoder instance.
 *
 * \returns The number of available hardware decoding cores.
 */
u32 H264DecMCGetCoreCount(void *dwl_inst);

/*!\brief API internal tracing function prototype
 *
 * Traces all API entries and returns. This must be implemented by
 * the application using the decoder API.
 *
 * \param string Pointer to a NULL terminated char string.
 *
 */
void H264DecTrace(const char *string);

/*!\example h264dectrace.c
  * This is an example of how to implement H264DecTrace() in an application.
  *
  *!\example h264decmc_output_handling.c
  * This is an example of how to handle the decoder output in a
  * multi-threaded application.
  */

enum DecRet H264DecUseExtraFrmBuffers(H264DecInst dec_inst, u32 n);


/*! @}*/
#ifdef __cplusplus
}
#endif

#endif                       /* __H264DECAPI_H__ */
