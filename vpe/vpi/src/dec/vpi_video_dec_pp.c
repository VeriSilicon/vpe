/*
 * Copyright (c) 2020, VeriSilicon Holdings Co., Ltd. All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

#include "vpi_log.h"
#include "vpi_video_dec_pp.h"

void dump_ppu(PpUnitConfig *ppu_cfg)
{
    int i;
    VPILOGD("%s(%d)\n", __FUNCTION__, __LINE__);
    for (i = 0; i < 4; i++) {
        VPILOGD("ppu_cfg[%d].enabled = %d\n", i, ppu_cfg[i].enabled);
        VPILOGD("ppu_cfg[%d].tiled_e = %d\n", i, ppu_cfg[i].tiled_e);
        VPILOGD("ppu_cfg[%d].crop.enabled = %d\n", i, ppu_cfg[i].crop.enabled);
        VPILOGD("ppu_cfg[%d].crop.x = %d\n", i, ppu_cfg[i].crop.x);
        VPILOGD("ppu_cfg[%d].crop.y = %d\n", i, ppu_cfg[i].crop.y);
        VPILOGD("ppu_cfg[%d].crop.width = %d\n", i, ppu_cfg[i].crop.width);
        VPILOGD("ppu_cfg[%d].crop.height = %d\n", i, ppu_cfg[i].crop.height);
        VPILOGD("ppu_cfg[%d].scale.enabled = %d\n", i,
                ppu_cfg[i].scale.enabled);
        VPILOGD("ppu_cfg[%d].scale.width = %d\n", i, ppu_cfg[i].scale.width);
        VPILOGD("ppu_cfg[%d].scale.height = %d\n", i, ppu_cfg[i].scale.height);
        VPILOGD("ppu_cfg[%d].out_p010 = %d\n", i, ppu_cfg[i].out_p010);
        VPILOGD("ppu_cfg[%d].align = %d\n", i, ppu_cfg[i].align);
        VPILOGD("ppu_cfg[%d].shaper_enabled = %d\n", i,
                ppu_cfg[i].shaper_enabled);
        VPILOGD("\n");
    }
}

VpiRet vpi_dec_parse_ppu_cfg(VpiDecCtx *vpi_ctx, PpUnitConfig *ppu_cfg)
{
    int pp_enabled, pp_count, pp_index;
    ResizeType *p_resizes;
    int resizes_num;
    int i;

    memset(ppu_cfg, 0, sizeof(PpUnitConfig) * 4);
    pp_enabled = 0;
    pp_count   = 0;

    p_resizes   = vpi_ctx->resizes;
    resizes_num = vpi_ctx->resize_num;
    // when no use rfc ,pp0 always on
    pp_enabled                = 1;
    ppu_cfg[0].enabled        = 1;
    ppu_cfg[0].tiled_e        = 1;
#ifndef BUILD_CMODEL
    ppu_cfg[0].align          = 10;
    ppu_cfg[0].shaper_enabled = 1;
#endif

#ifdef BUILD_CMODEL
    pp_enabled          = 1;
    ppu_cfg[0].enabled  = 1;
    ppu_cfg[0].tiled_e  = 1;
    ppu_cfg[0].out_p010 = pDecParam->in_10bit;
#else
#ifndef SUPPORT_TCACHE
    pp_enabled                = 2;
    ppu_cfg[0].enabled        = 2;
    ppu_cfg[0].tiled_e        = 1;
    ppu_cfg[0].align          = 10;
#ifdef SUPPORT_DEC400
    ppu_cfg[0].shaper_enabled = 1;
#endif
#endif
#endif

    // for 1/4 ds 1pass
    if (vpi_ctx->vce_ds_enable) {
        if (resizes_num) {
            VPILOGE("When use vce ds 1pass, don't support other downscaler!\n");
            return VPI_ERR_VALUE;
        }
        pp_enabled         = 1;
        ppu_cfg[1].enabled = 1;
        ppu_cfg[1].tiled_e = 1;
#ifndef BUILD_CMODEL
        ppu_cfg[1].align          = 10;
        ppu_cfg[1].shaper_enabled = 1;
#endif
        ppu_cfg[1].scale.enabled = 1;
        ppu_cfg[1].scale.width   = -2;
        ppu_cfg[1].scale.height  = -2;

        ppu_cfg[2].enabled = 0;
        ppu_cfg[3].enabled = 0;
    } else {
        for (i = pp_count; i < resizes_num; i++) {
            if (p_resizes[i].x == 0 && p_resizes[i].y == 0 &&
                p_resizes[i].cw == 0 && p_resizes[i].ch == 0 &&
                p_resizes[i].sw == 0 && p_resizes[i].sh == 0) {
                continue;
            }

            VPILOGD("pp %d resize %d get param\n", i, i);
            pp_enabled                = 1;
            pp_index                  = i;
            ppu_cfg[pp_index].enabled = 1;
            ppu_cfg[pp_index].tiled_e = 1;

            if (!(p_resizes[i].x == 0 && p_resizes[i].y == 0 &&
                  p_resizes[i].cw == 0 && p_resizes[i].ch == 0)) {
                ppu_cfg[pp_index].crop.enabled = 1;
                ppu_cfg[pp_index].crop.x       = p_resizes[i].x;
                ppu_cfg[pp_index].crop.y       = p_resizes[i].y;
                ppu_cfg[pp_index].crop.width   = p_resizes[i].cw;
                ppu_cfg[pp_index].crop.height  = p_resizes[i].ch;
            }
            if (!(p_resizes[i].sw == 0 && p_resizes[i].sh == 0)) {
                ppu_cfg[pp_index].scale.enabled = 1;
                ppu_cfg[pp_index].scale.width   = p_resizes[i].sw;
                ppu_cfg[pp_index].scale.height  = p_resizes[i].sh;
            }
#ifndef BUILD_CMODEL
            ppu_cfg[pp_index].align          = 10;
            ppu_cfg[pp_index].shaper_enabled = 1;
#endif
        }
    }

    vpi_ctx->pp_enabled = pp_enabled;

    dump_ppu(ppu_cfg);

    return VPI_SUCCESS;
}

void vpi_resolve_pp_overlap_ppu(PpUnitConfig *ppu_cfg,
                                struct TBPpUnitParams *pp_units_params)
{
    /* Override PPU1-3 parameters with tb.cfg */
    uint32_t i;
    memset(ppu_cfg, 0, 4 * sizeof(PpUnitConfig));

    for (i = ppu_cfg[0].enabled ? 1 : 0; i < 4; i++) {
        ppu_cfg[i].enabled        = pp_units_params[i].unit_enabled;
        ppu_cfg[i].cr_first       = pp_units_params[i].cr_first;
        ppu_cfg[i].tiled_e        = pp_units_params[i].tiled_e;
        ppu_cfg[i].crop.enabled   = 0;
        ppu_cfg[i].crop.x         = pp_units_params[i].crop_x;
        ppu_cfg[i].crop.y         = pp_units_params[i].crop_y;
        ppu_cfg[i].crop.width     = pp_units_params[i].crop_width;
        ppu_cfg[i].crop.height    = pp_units_params[i].crop_height;
        ppu_cfg[i].scale.enabled  = pp_units_params[i].unit_enabled;
        ppu_cfg[i].scale.width    = pp_units_params[i].scale_width;
        ppu_cfg[i].scale.height   = pp_units_params[i].scale_height;
        ppu_cfg[i].shaper_enabled = pp_units_params[i].shaper_enabled;
        ppu_cfg[i].monochrome     = pp_units_params[i].monochrome;
        ppu_cfg[i].planar         = pp_units_params[i].planar;
        ppu_cfg[i].out_p010       = pp_units_params[i].out_p010;
        ppu_cfg[i].out_cut_8bits  = pp_units_params[i].out_cut_8bits;
        ppu_cfg[i].align          = DEC_ALIGN_1024B;
        ppu_cfg[i].ystride        = pp_units_params[i].ystride;
        ppu_cfg[i].cstride        = pp_units_params[i].cstride;
        ppu_cfg[i].out_format     = 0;
        ppu_cfg[i].out_be         = 0;
    }
    if (pp_units_params[0].unit_enabled) {
        /* PPU0 */
        ppu_cfg[0].align = pp_units_params[0].align;
        ppu_cfg[0].enabled |= pp_units_params[0].unit_enabled;
        ppu_cfg[0].cr_first |= pp_units_params[0].cr_first;
        ppu_cfg[0].tiled_e |= pp_units_params[0].tiled_e;
        ppu_cfg[0].planar |= pp_units_params[0].planar;
        ppu_cfg[0].out_p010 |= pp_units_params[0].out_p010;
        ppu_cfg[0].out_cut_8bits |= pp_units_params[0].out_cut_8bits;
        if (!ppu_cfg[0].crop.enabled && pp_units_params[0].unit_enabled) {
            ppu_cfg[0].crop.x      = pp_units_params[0].crop_x;
            ppu_cfg[0].crop.y      = pp_units_params[0].crop_y;
            ppu_cfg[0].crop.width  = pp_units_params[0].crop_width;
            ppu_cfg[0].crop.height = pp_units_params[0].crop_height;
        }
        if (ppu_cfg[0].crop.width || ppu_cfg[0].crop.height) {
            ppu_cfg[0].crop.enabled = 1;
        }
        if (!ppu_cfg[0].scale.enabled && pp_units_params[0].unit_enabled) {
            ppu_cfg[0].scale.width  = pp_units_params[0].scale_width;
            ppu_cfg[0].scale.height = pp_units_params[0].scale_height;
        }
        if (ppu_cfg[0].scale.width || ppu_cfg[0].scale.height) {
            ppu_cfg[0].scale.enabled = 1;
        }
        ppu_cfg[0].shaper_enabled = pp_units_params[0].shaper_enabled;
        ppu_cfg[0].monochrome     = pp_units_params[0].monochrome;
        ppu_cfg[0].align          = pp_units_params[0].align;
        if (!ppu_cfg[0].ystride) {
            ppu_cfg[0].ystride = pp_units_params[0].ystride;
        }
        if (!ppu_cfg[0].cstride) {
            ppu_cfg[0].cstride = pp_units_params[0].cstride;
        }
    }
    VPILOGD("ppu_cfg[0].shaper_enabled = %d\n", ppu_cfg[0].shaper_enabled);
    VPILOGD("ppu_cfg[1].shaper_enabled = %d\n", ppu_cfg[1].shaper_enabled);
}

static int split_string(char **tgt, int max, char *src, char *split)
{
    int count;
    char *currp;
    char *p;
    char c;
    int i;
    int last;

    currp = src;
    count = 0;
    i     = 0;
    last  = 0;
    while ((c = *currp++) != '\0') {
        if ((p = strchr(split, c)) == NULL) {
            if (count < max) {
                tgt[count][i++] = c;
            } else {
                VPILOGD("the split count exceeds max num\n");
                return -1;
            }
            last = 1; // 1 means non split char, 0 means split char
        } else {
            if (last == 1) {
                tgt[count][i] = '\0';
                count++;
                i = 0;
            }
            last = 0; // 1 means non split char, 0 means split char
        }
    }
    if (last == 1) {
        tgt[count][i] = '\0';
        count++;
    }

    return count;
}

VpiRet vpi_dec_parse_resize(VpiDecCtx *vpi_ctx, VpiDecOption *dec_cfg)
{
    int i;
#define MAX_SEG_NUM 4
    char strs[MAX_SEG_NUM][256];
    char *pstrs[MAX_SEG_NUM];
    int seg_num = 0;
    char *p, *cp;
    int output_num   = 0;
    char *resize_str = NULL;

    for (i = 0; i < MAX_SEG_NUM; i++) {
        pstrs[i] = &strs[i][0];
    }

    if (dec_cfg->pp_setting) {
        VPILOGD("pp_setting: %s\n", dec_cfg->pp_setting);
        seg_num = split_string(pstrs, MAX_SEG_NUM, dec_cfg->pp_setting, ":");
        if (seg_num == 2) {
            output_num = atoi(pstrs[0]);
            resize_str = pstrs[1];
        } else {
            VPILOGE("resizes syntax error!\n");
            return VPI_ERR_VALUE;
        }
    } else {
        VPILOGD("No valid PP params\n");
        return VPI_ERR_VALUE;
    }

    if (resize_str) {
        VPILOGD("resize_str: %s\n", resize_str);
        seg_num = split_string(pstrs, MAX_SEG_NUM, resize_str, "()");
        if (seg_num <= 0) {
            VPILOGD("can't find resize info!\n");
            return VPI_ERR_VALUE;
        }
    } else {
        return VPI_ERR_VALUE;
    }

    vpi_ctx->resize_num = 0;
    while (vpi_ctx->resize_num < seg_num) {
        cp = strs[vpi_ctx->resize_num];
        VPILOGD("cp: %s\n", cp);
        if ((p = strchr(cp, ',')) != NULL) {
            vpi_ctx->resizes[vpi_ctx->resize_num].x = atoi(cp);
            cp                                      = p + 1;
            VPILOGD("cp: %s\n", cp);
            if ((p = strchr(cp, ',')) != NULL) {
                vpi_ctx->resizes[vpi_ctx->resize_num].y = atoi(cp);
                cp                                      = p + 1;
                VPILOGD("cp: %s\n", cp);
                if ((p = strchr(cp, ',')) != NULL) {
                    vpi_ctx->resizes[vpi_ctx->resize_num].cw = atoi(cp);
                    vpi_ctx->resizes[vpi_ctx->resize_num].ch = atoi(p + 1);
                    cp                                       = p + 1;
                    VPILOGD("cp: %s\n", cp);
                    if ((p = strchr(cp, ',')) != NULL) {
                        cp = p + 1;
                        VPILOGD("cp: %s\n", cp);
                    } else if (vpi_ctx->resize_num != 0) {
                        return VPI_ERR_VALUE;
                    }
                } else {
                    return VPI_ERR_VALUE;
                }
            } else {
                return VPI_ERR_VALUE;
            }
        }
        if ((p = strchr(cp, 'x')) == NULL) {
            if (cp[0] == 'd') {
                int n = atoi(cp + 1);
                if (n != 2 && n != 4 && n != 8) {
                    VPILOGD("only support d2/d4/d8!\n");
                    return VPI_ERR_VALUE;
                }
                vpi_ctx->resizes[vpi_ctx->resize_num].sw = -n;
                vpi_ctx->resizes[vpi_ctx->resize_num].sh = -n;
            } else if (vpi_ctx->resize_num != 0) {
                VPILOGD("can't find swxsh or dn!\n");
                return VPI_ERR_VALUE;
            }
        } else {
            vpi_ctx->resizes[vpi_ctx->resize_num].sw = atoi(cp);
            vpi_ctx->resizes[vpi_ctx->resize_num].sh = atoi(p + 1);
        }
        if (vpi_ctx->resizes[vpi_ctx->resize_num].sw == -1 &&
            vpi_ctx->resizes[vpi_ctx->resize_num].sh == -1) {
            VPILOGE("resize -1x-1 error!\n");
            return VPI_ERR_VALUE;
        }
        VPILOGD("get resize %d %d,%d,%d,%d,%dx%d\n", vpi_ctx->resize_num,
                vpi_ctx->resizes[vpi_ctx->resize_num].x,
                vpi_ctx->resizes[vpi_ctx->resize_num].y,
                vpi_ctx->resizes[vpi_ctx->resize_num].cw,
                vpi_ctx->resizes[vpi_ctx->resize_num].ch,
                vpi_ctx->resizes[vpi_ctx->resize_num].sw,
                vpi_ctx->resizes[vpi_ctx->resize_num].sh);
        vpi_ctx->resize_num++;
    }

    if (output_num < 1 || output_num > 4) {
        VPILOGE("outputs number error\n");
        return VPI_ERR_VALUE;
    }

    if (output_num == 1 && vpi_ctx->resize_num == 1 &&
        (vpi_ctx->resizes[0].x == 0 && vpi_ctx->resizes[0].y == 0 &&
         vpi_ctx->resizes[0].cw == 0 && vpi_ctx->resizes[0].ch == 0 &&
         vpi_ctx->resizes[0].sw == -2 && vpi_ctx->resizes[0].sh == -2)) {
        vpi_ctx->vce_ds_enable = 1;
        vpi_ctx->resizes[1]    = vpi_ctx->resizes[0];
        vpi_ctx->resizes[0].x = vpi_ctx->resizes[0].y = vpi_ctx->resizes[0].cw =
            vpi_ctx->resizes[0].ch                    = vpi_ctx->resizes[0].sw =
                vpi_ctx->resizes[0].sh                = 0;

        vpi_ctx->resize_num++;
    } else if (output_num == 1 && vpi_ctx->resize_num == 2) {
        if (!(vpi_ctx->resizes[0].x == vpi_ctx->resizes[1].x &&
              vpi_ctx->resizes[0].y == vpi_ctx->resizes[1].y &&
              vpi_ctx->resizes[0].cw == vpi_ctx->resizes[1].cw &&
              vpi_ctx->resizes[0].ch == vpi_ctx->resizes[1].ch &&
              vpi_ctx->resizes[1].sw == -2 && vpi_ctx->resizes[1].sh == -2)) {
            VPILOGE("resize param error!\n");
            return VPI_ERR_VALUE;
        }
        vpi_ctx->vce_ds_enable = 1;
    } else if (output_num == vpi_ctx->resize_num + 1) {
        if (vpi_ctx->resize_num) {
            for (i = output_num - 1; i > 0; i--) {
                vpi_ctx->resizes[i] = vpi_ctx->resizes[i - 1];
            }
        }
        vpi_ctx->resizes[0].x = vpi_ctx->resizes[0].y = vpi_ctx->resizes[0].cw =
            vpi_ctx->resizes[0].ch                    = vpi_ctx->resizes[0].sw =
                vpi_ctx->resizes[0].sh                = 0;

        vpi_ctx->resize_num++;
    } else if (output_num != vpi_ctx->resize_num) {
        VPILOGE("resize param error!\n");
        return VPI_ERR_VALUE;
    }

    if ((vpi_ctx->resizes[0].sw || vpi_ctx->resizes[0].sh) &&
        (vpi_ctx->resizes[0].sw != vpi_ctx->resizes[0].cw ||
         vpi_ctx->resizes[0].sh != vpi_ctx->resizes[0].ch)) {
        VPILOGE("resize channel 0 do not support scale!\n");
        return VPI_ERR_VALUE;
    }

    return VPI_SUCCESS;
}

void vpi_dec_disable_all_pp_shaper(struct DecConfig *config)
{
    int i;
    if (!config) {
        return;
    }
    VPILOGD("%s\n", __FUNCTION__);
    for (i = 0; i < 4; i++) {
        config->ppu_cfg[i].shaper_enabled = 0;
    }
}
