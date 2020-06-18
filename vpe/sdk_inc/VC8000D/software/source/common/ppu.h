/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Holdings Co., Ltd. All rights reserved        --
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

#ifndef __PPU__
#define __PPU__

#include "basetype.h"
#include "decapicommon.h"
#include "vpufeature.h"

struct PPUnitRegs {
  u32 PP_OUT_E_U;
  u32 PP_OUT_TILE_E_U;
  u32 PP_OUT_MODE;
  u32 PP_CR_FIRST;
  u32 PP_OUT_SWAP_U;
  u32 HOR_SCALE_MODE_U;
  u32 VER_SCALE_MODE_U;
  u32 OUT_FORMAT_U;
  u32 SCALE_HRATIO_U;
  u32 SCALE_WRATIO_U;
  u32 WSCALE_INVRA_U;
  u32 HSCALE_INVRA_U;
  u32 PP_OUT_LU_BASE_U_MSB;
  u32 PP_OUT_LU_BASE_U_LSB;
  u32 PP_OUT_CH_BASE_U_MSB;
  u32 PP_OUT_CH_BASE_U_LSB;
  u32 PP_OUT_Y_STRIDE;
  u32 PP_OUT_C_STRIDE;
  u32 FLIP_MODE_U;
  u32 CROP_STARTX_U;
  u32 ROTATION_MODE_U;
  u32 CROP_STARTY_U;
  u32 PP_IN_WIDTH_U;
  u32 PP_IN_HEIGHT_U;
  u32 PP_OUT_WIDTH_U;
  u32 PP_OUT_HEIGHT_U;
  u32 PP_OUT_LU_BOT_BASE_U_MSB;
  u32 PP_OUT_LU_BOT_BASE_U_LSB;
  u32 PP_OUT_CH_BOT_BASE_U_MSB;
  u32 PP_OUT_CH_BOT_BASE_U_LSB;
};

extern struct PPUnitRegs ppu_regs[4];

/* PPU internal config */
/* Compared with PpUnitConfig, more internal member variables are added,
   which should not be exposed to external user. */
typedef struct _PpUnitIntConfig {
  u32 enabled;    /* PP unit enabled */
  u32 tiled_e;    /* PP unit tiled4x4 output enabled */
  u32 cr_first;   /* CrCb instead of CbCr */
  u32 luma_offset;   /* luma offset of current PPU to pp buffer start address */
  u32 chroma_offset;
  u32 pixel_width;   /* pixel bit depth store in external memory */
  u32 false_crop;
  u32 false_scale;
  u32 shaper_enabled;
  u32 planar;        /* Planar output */
  DecPicAlignment align;    /* alignment for current PPU */
  u32 ystride;
  u32 cstride;
  u32 false_ystride;
  u32 false_cstride;
  struct {
    u32 enabled;  /* whether cropping is enabled */
    u32 x;        /* cropping start x */
    u32 y;        /* cropping start y */
    u32 width;    /* cropping width */
    u32 height;   /* cropping height */
  } crop;
  struct {
    u32 enabled;  /* whether scaling is enabled */
    u32 width;    /* scaled output width */
    u32 height;   /* scaled output height */
  } scale;
  u32 monochrome; /* PP output monochrome (luma only) */
  u32 out_format;
  u32 out_p010;
  u32 out_cut_8bits;
} PpUnitIntConfig;

u32 CheckPpUnitConfig(struct DecHwFeatures *hw_feature,
                      u32 in_width,
                      u32 in_height,
                      u32 interlace,
                      PpUnitIntConfig *ppu_cfg);
u32 CalcPpUnitBufferSize(PpUnitIntConfig *ppu_cfg, u32 mono_chrome);

void PpUnitSetIntConfig(PpUnitIntConfig *ppu_int_cfg, PpUnitConfig *ppu_ext_cfg, u32 pixel_width, u32 frame_only,u32 mono_chrome);

void dump_ppuconfig(const void * inst, PpUnitConfig * ppu_cfg); //kwu add for debug

/*sunny add some extend space for bigsea dummy read and dec400 table in extern buffer*/
/*
the externbuffer content is :
[pp0Y]
PP_LUMA_BUF_RES
[pp0UV]
PP_CHROMA_BUF_RES
[pp1Y]
PP_LUMA_BUF_RES
[pp1UV]
PP_CHROMA_BUF_RES
[pp2Y]
PP_LUMA_BUF_RES
[pp2UV]
PP_CHROMA_BUF_RES
[pp3Y]
PP_LUMA_BUF_RES
[pp3UV]
PP_CHROMA_BUF_RES
bigsea_dummy_space_1KB 
pp0_Y_dec400_table_8KB 
pp0_UV_dec400_table_8KB 
pp1_Y_dec400_table_8KB 
pp1_UV_dec400_table_8KB 
pp2_Y_dec400_table_8KB 
pp2_UV_dec400_table_8KB 
pp3_Y_dec400_table_8KB 
pp3_UV_dec400_table_8KB 

*/
#define BIGSEA_DUMMY_SPACE 1024
#define DEC400_YUV_TABLE_SIZE (8*1024)
#define DEC400_PP_TABLE_SIZE (2*DEC400_YUV_TABLE_SIZE)
#define DEC400_TABLE_OFFSET (BIGSEA_DUMMY_SPACE)
#define DEC400_PPn_TABLE_OFFSET(n) (BIGSEA_DUMMY_SPACE + ((n)*(DEC400_PP_TABLE_SIZE)))
#define DEC400_PPn_Y_TABLE_OFFSET(n) (DEC400_PPn_TABLE_OFFSET(n))
#define DEC400_PPn_UV_TABLE_OFFSET(n) (DEC400_PPn_TABLE_OFFSET(n) + DEC400_YUV_TABLE_SIZE)

#define PP_LUMA_BUF_RES 2048
#define PP_CHROMA_BUF_RES 1024



#endif /* __PPU__ */
