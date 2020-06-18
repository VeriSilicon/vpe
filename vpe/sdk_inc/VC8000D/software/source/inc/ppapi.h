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

#ifndef __PPAPI_H__
#define __PPAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "dectypes.h"
#include "basetype.h"
#include "decapicommon.h"
#include "dwl.h"
#include "decapicommon.h"


#define PP_PIPELINE_DISABLED                            0U
#define PP_PIPELINED_DEC_TYPE_H264                      1U
#define PP_PIPELINED_DEC_TYPE_MPEG4                     2U
#define PP_PIPELINED_DEC_TYPE_JPEG                      3U
#define PP_PIPELINED_DEC_TYPE_VC1                       4U
#define PP_PIPELINED_DEC_TYPE_MPEG2                     5U
#define PP_PIPELINED_DEC_TYPE_VP6                       6U
#define PP_PIPELINED_DEC_TYPE_AVS                       7U
#define PP_PIPELINED_DEC_TYPE_RV                        8U
#define PP_PIPELINED_DEC_TYPE_VP8                       9U
#define PP_PIPELINED_DEC_TYPE_WEBP                     10U

#define PP_PIX_FMT_YCBCR_4_0_0                          0x080000U

#define PP_PIX_FMT_YCBCR_4_2_2_INTERLEAVED              0x010001U
#define PP_PIX_FMT_YCRYCB_4_2_2_INTERLEAVED             0x010005U
#define PP_PIX_FMT_CBYCRY_4_2_2_INTERLEAVED             0x010006U
#define PP_PIX_FMT_CRYCBY_4_2_2_INTERLEAVED             0x010007U
#define PP_PIX_FMT_YCBCR_4_2_2_SEMIPLANAR               0x010002U

#define PP_PIX_FMT_YCBCR_4_2_2_TILED_4X4                0x010008U
#define PP_PIX_FMT_YCRYCB_4_2_2_TILED_4X4               0x010009U
#define PP_PIX_FMT_CBYCRY_4_2_2_TILED_4X4               0x01000AU
#define PP_PIX_FMT_CRYCBY_4_2_2_TILED_4X4               0x01000BU

#define PP_PIX_FMT_YCBCR_4_4_0                          0x010004U

#define PP_PIX_FMT_YCBCR_4_2_0_PLANAR                   0x020000U
#define PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR               0x020001U
#define PP_PIX_FMT_YCBCR_4_2_0_TILED                    0x020002U

#define PP_PIX_FMT_YCBCR_4_1_1_SEMIPLANAR               0x100001U
#define PP_PIX_FMT_YCBCR_4_4_4_SEMIPLANAR               0x200001U

#define PP_PIX_FMT_RGB16_CUSTOM                         0x040000U
#define PP_PIX_FMT_RGB16_5_5_5                          0x040001U
#define PP_PIX_FMT_RGB16_5_6_5                          0x040002U
#define PP_PIX_FMT_BGR16_5_5_5                          0x040003U
#define PP_PIX_FMT_BGR16_5_6_5                          0x040004U

#define PP_PIX_FMT_RGB32_CUSTOM                         0x041000U
#define PP_PIX_FMT_RGB32                                0x041001U
#define PP_PIX_FMT_BGR32                                0x041002U

#define PP_YCBCR2RGB_TRANSFORM_CUSTOM                   0U
#define PP_YCBCR2RGB_TRANSFORM_BT_601                   1U
#define PP_YCBCR2RGB_TRANSFORM_BT_709                   2U

#define PP_ROTATION_NONE                                0U
#define PP_ROTATION_RIGHT_90                            1U
#define PP_ROTATION_LEFT_90                             2U
#define PP_ROTATION_HOR_FLIP                            3U
#define PP_ROTATION_VER_FLIP                            4U
#define PP_ROTATION_180                                 5U

#define PP_PIC_FRAME_OR_TOP_FIELD                       0U
#define PP_PIC_BOT_FIELD                                1U
#define PP_PIC_TOP_AND_BOT_FIELD                        2U
#define PP_PIC_TOP_AND_BOT_FIELD_FRAME                  3U
#define PP_PIC_TOP_FIELD_FRAME                          4U
#define PP_PIC_BOT_FIELD_FRAME                          5U

#define PP_MAX_MULTIBUFFER                              17


typedef const void *PPInst;

typedef enum PPResult_ {
  PP_OK = 0,
  PP_PARAM_ERROR = -1,
  PP_MEMFAIL = -4,

  PP_SET_IN_SIZE_INVALID = -64,
  PP_SET_IN_ADDRESS_INVALID = -65,
  PP_SET_IN_FORMAT_INVALID = -66,
  PP_SET_CROP_INVALID = -67,
  PP_SET_ROTATION_INVALID = -68,
  PP_SET_OUT_SIZE_INVALID = -69,
  PP_SET_OUT_ADDRESS_INVALID = -70,
  PP_SET_OUT_FORMAT_INVALID = -71,
  PP_SET_VIDEO_ADJUST_INVALID = -72,
  PP_SET_RGB_BITMASK_INVALID = -73,
  PP_SET_FRAMEBUFFER_INVALID = -74,
  PP_SET_MASK1_INVALID = -75,
  PP_SET_MASK2_INVALID = -76,
  PP_SET_DEINTERLACE_INVALID = -77,
  PP_SET_IN_STRUCT_INVALID = -78,
  PP_SET_IN_RANGE_MAP_INVALID = -79,
  PP_SET_ABLEND_UNSUPPORTED = -80,
  PP_SET_DEINTERLACING_UNSUPPORTED = -81,
  PP_SET_DITHERING_UNSUPPORTED = -82,
  PP_SET_SCALING_UNSUPPORTED = -83,

  PP_BUSY = -128,
  PP_HW_BUS_ERROR = -256,
  PP_HW_TIMEOUT = -257,
  PP_DWL_ERROR = -258,
  PP_SYSTEM_ERROR = -259,
  PP_DEC_COMBINED_MODE_ERROR = -512,
  PP_DEC_RUNTIME_ERROR = -513
} PPResult;

#if 0
typedef struct PPInImage_ {
  u32 pix_format;
  u32 pic_struct;
  u32 video_range;
  u32 width;
  u32 height;
  addr_t buffer_bus_addr;
  addr_t buffer_cb_bus_addr;
  addr_t buffer_cr_bus_addr;
  addr_t buffer_bus_addr_bot;
  addr_t buffer_bus_addr_ch_bot;
  u32 vc1_advanced_profile;
  u32 vc1_multi_res_enable;
  u32 vc1_range_red_frm;
  u32 vc1_range_map_yenable;
  u32 vc1_range_map_ycoeff;
  u32 vc1_range_map_cenable;
  u32 vc1_range_map_ccoeff;
} PPInImage;

typedef struct PPOutImage_ {
  u32 pix_format;
  u32 width;
  u32 height;
  addr_t buffer_bus_addr;
  addr_t buffer_chroma_bus_addr;
} PPOutImage;

typedef struct PPRgbTransform_ {
  u32 a;
  u32 b;
  u32 c;
  u32 d;
  u32 e;
} PPRgbTransform;

typedef struct PPRgbBitmask_ {
  u32 mask_r;
  u32 mask_g;
  u32 mask_b;
  u32 mask_alpha;
} PPRgbBitmask;

typedef struct PPOutRgb_ {
  u32 rgb_transform;
  i32 contrast;
  i32 brightness;
  i32 saturation;
  u32 alpha;
  u32 transparency;
  PPRgbTransform rgb_transform_coeffs;
  PPRgbBitmask rgb_bitmask;
  u32 dithering_enable;
} PPOutRgb;

typedef struct PPInCropping_ {
  u32 enable;
  /* NOTE: these are coordinates relative to the input picture */
  u32 origin_x;
  u32 origin_y;
  u32 height;
  u32 width;
} PPInCropping;

typedef struct PPOutMask1_ {
  u32 enable;
  /* NOTE: these are coordinates relative to the output image */
  i32 origin_x;
  i32 origin_y;
  u32 height;
  u32 width;
  u32 alpha_blend_ena;
  addr_t blend_component_base;
  /* following parameters are to be set nonzero *only* if HW
   * supports alpha blend cropping. */
  i32 blend_origin_x;
  i32 blend_origin_y;
  u32 blend_width;
  u32 blend_height;
} PPOutMask1;

typedef struct PPOutMask2_ {
  u32 enable;
  /* NOTE: these are coordinates relative to the output image */
  i32 origin_x;
  i32 origin_y;
  u32 height;
  u32 width;
  u32 alpha_blend_ena;
  addr_t blend_component_base;
  /* following parameters are to be set nonzero *only* if HW
   * supports alpha blend cropping. */
  i32 blend_origin_x;
  i32 blend_origin_y;
  u32 blend_width;
  u32 blend_height;
} PPOutMask2;

typedef struct PPOutFrameBuffer_ {
  u32 enable;
  /* NOTE: these are coordinates relative to the framebuffer */
  i32 write_origin_x;
  i32 write_origin_y;
  u32 frame_buffer_width;
  u32 frame_buffer_height;
} PPOutFrameBuffer;

typedef struct PPInRotation_ {
  u32 rotation;
} PPInRotation;

typedef struct PPOutDeinterlace_ {
  u32 enable;
} PPOutDeinterlace;

typedef struct PPConfig_ {
  PPInImage pp_in_img;
  PPInCropping pp_in_crop;
  PPInRotation pp_in_rotation;
  PPOutImage pp_out_img;
  PPOutRgb pp_out_rgb;
  PPOutMask1 pp_out_mask1;
  PPOutMask2 pp_out_mask2;
  PPOutFrameBuffer pp_out_frm_buffer;
  PPOutDeinterlace pp_out_deinterlace;
} PPConfig;

/* Version information */
typedef struct PPApiVersion_ {
  u32 major;           /* PP API major version */
  u32 minor;           /* PP API minor version */
} PPApiVersion;

typedef struct DecSwHwBuild  PPBuild;

typedef struct PPOutput_ {
  addr_t buffer_bus_addr;
  addr_t buffer_chroma_bus_addr;
} PPOutput;

typedef struct PPOutputBuffers_ {
  u32 nbr_of_buffers;
  PPOutput pp_output_buffers[PP_MAX_MULTIBUFFER];
} PPOutputBuffers;
#endif
typedef struct PPConfig_ {
  u32 in_format;
  u32 in_stride;
  u32 in_height;
  u32 in_width;
  struct DWLLinearMem pp_in_buffer;
  struct DWLLinearMem pp_out_buffer;
  PpUnitConfig ppu_config[4];
} PPConfig;

typedef struct PPDecPicture_ {
  struct PPOutputInfo {
    u32 pic_width;        /**< pixels width of the picture as stored in memory */
    u32 pic_height;       /**< pixel height of the picture as stored in memory */
    u32 pic_stride;       /** < pic stride of each pixel line in bytes. */
    u32 pic_stride_ch;
    u32 bit_depth_luma;
    u32 bit_depth_chroma;
    u32 pixel_format;
    u32 pp_enabled;
    const u32 *output_picture;  /**< Pointer to the picture data */
    addr_t output_picture_bus_address;    /**< DMA bus address of the output picture buffer */
    const u32 *output_picture_chroma;  /**< Pointer to the picture data */
    addr_t output_picture_chroma_bus_address;    /**< DMA bus address of the output picture buffer */
    enum DecPictureFormat output_format; /**< Storage format of output picture. */
  } pictures[4];

  struct DecPicturePpu pp_pic;
} PPDecPicture;

//#ifdef SUPPORT_TCACHE
#if 0
typedef struct {
  u32 in_height;
  u32 in_width;
  u32 in_stride;
  u32 in_format;
  u32 out_format;
  u32 dec_frames;
  u32 rgb_cov_bd;
  u32 link_mode;
  struct DWLLinearMem edma_link;
} TcacheParam;

int tcache_init(TcacheParam *param, char *finput);
int enable_tcache(void);
int disable_tcache(void);
int release_tcache(void);
int tcache_read_edma(u32 luma_addr, u32 chroma_addr, char *filename);
#endif

/*------------------------------------------------------------------------------
    Prototypes of PP API functions
------------------------------------------------------------------------------*/
PPResult PPInit(PPInst *p_post_pinst, const void *dwl);

PPResult PPSetInfo(PPInst post_pinst, PPConfig * p_pp_conf);

PPResult PPDecode(PPInst post_pinst);

PPResult PPNextPicture(PPInst post_pinst, PPDecPicture *output);

PPResult PPSetInput(PPInst post_pinst, struct DWLLinearMem input);
PPResult PPSetOutput(PPInst post_pinst, struct DWLLinearMem Output);

void PPRelease(PPInst post_pinst);
#if 0
PPResult PPDecCombinedModeEnable(PPInst post_pinst, const void *dec_inst,
                                 u32 dec_type);

PPResult PPDecCombinedModeDisable(PPInst post_pinst, const void *dec_inst);

PPResult PPGetConfig(PPInst post_pinst, PPConfig *p_pp_conf);

PPResult PPSetConfig(PPInst post_pinst, PPConfig *p_pp_conf);

PPResult PPDecSetMultipleOutput(PPInst post_pinst,
                                const PPOutputBuffers *p_buffers);

PPResult PPDecSwapLastOutputBuffer(PPInst post_pinst,
                                   PPOutput *p_old_buffer,
                                   PPOutput *p_new_buffer);

PPResult PPGetNextOutput(PPInst post_pinst, PPOutput *p_out);

void PPRelease(PPInst p_pp_inst);

PPResult PPGetResult(PPInst post_pinst);

PPApiVersion PPGetAPIVersion(void);

PPBuild PPGetBuild(void);

void PPTrace(const char *string);
#endif

#ifdef __cplusplus
}
#endif

#endif                       /* __PPAPI_H__ */
