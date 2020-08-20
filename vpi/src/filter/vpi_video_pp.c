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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "vpi_types.h"
#include "vpi_log.h"
#include "vpi_video_pp.h"
#include "vpi_video_prc.h"
#include "vpi_error.h"

#include "ppinternal.h"
#include "dectypes.h"
#include "hugepage_api.h"
#include "trans_fd_api.h"
#include "trans_edma_api.h"
#include "vpi_video_dec_tb_defs.h"
#include "fifo.h"

#define ALIGN(a) (1 << (a))
#define XALIGN(x, n) (((x) + ((n)-1)) & (~((n)-1)))
#define WIDTH_ALIGN 16
#define HEIGHT_ALIGN 8
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define NEXT_MULTIPLE(value, n) (((value) + (n)-1) & ~((n)-1))
#define PP_IN_HEIGHT_ALIGN 8
#define PP_OUT_HEIGHT_ALIGN 4
#define PP_FETCH_ALIGN 32
#define HUGE_PGAE_SIZE (2 * 1024 * 1024)
#define NUM_OUT 1

extern u32 dec_pp_in_blk_size;
static struct TBCfg tb_cfg;

typedef struct VpiMwlInitParam {
    u32 client_type;
    char *device;
    int mem_id;
} VpiMwlInitParam;

typedef struct VpiMwl {
    u32 client_type;
    int fd_memalloc;
    int mem_id;
    char *device;
    EDMA_HANDLE edma_handle;
} VpiMwl;

extern int TCACHE_config(TCACHE_HANDLE thd, TCACHE_PARAM *pParam);

#ifdef PP_MEM_ERR_TEST
static int pp_memory_err_cnt    = 0;
static int pp_memory_err_shadow = 0;

static int pp_memory_check()
{
    int ret = 0;
    if (pp_memory_err_cnt++ == pp_memory_err_shadow) {
        VPILOGE("[%s,%d],pp_memory_err_cnt %d, pp_mem_err_test %d, force "
                "malloc "
                "memory error!!!\n",
                __FUNCTION__, __LINE__, pp_memory_err_cnt,
                pp_memory_err_shadow);
        ret = -1;
    }
    return ret;
}
#endif

#ifdef PP_EDMA_ERR_TEST
static int pp_edma_err_cnt    = 0;
static int pp_edma_err_shadow = 0;

static int pp_edma_eerror_check()
{
    int ret = 0;
    if (pp_edma_err_cnt++ == pp_edma_err_shadow) {
        VPILOGE("[%s,%d],edma_err_cnt %d, pp_edma_err_test %d, force edma "
                "transfer error!!!\n",
                __FUNCTION__, __LINE__, pp_edma_err_cnt, pp_edma_err_shadow);
        ret = -1;
    }
    return ret;
}
#endif

static i32 pp_mwl_release(const void *instance)
{
    VpiMwl *mwl = (VpiMwl *)instance;

    if (mwl) {
        if (mwl->edma_handle) {
            TRANS_EDMA_release(mwl->edma_handle);
        }

        if (mwl->fd_memalloc > 0) {
            TranscodeCloseFD(mwl->fd_memalloc);
        }
#ifdef CHECK_MEM_LEAK_TRANS
        DWLfree(mwl);
#else
        free(mwl);
#endif
        mwl = NULL;
    }
    return 0;
}

static void *pp_mwl_init(VpiMwlInitParam *param)
{
    VpiMwl *mwl;
#ifdef CHECK_MEM_LEAK_TRANS
    mwl = (VpiMwl *)DWLcalloc(1, sizeof(VpiMwl));
#else
    mwl = (VpiMwl *)calloc(1, sizeof(VpiMwl));
#endif
    if (mwl == NULL) {
        VPILOGE("%s", "failed to alloc struct VpiMwl\n");
        return NULL;
    }

#ifdef PP_MEM_ERR_TEST
    if (pp_memory_check() != 0) {
        VPILOGE("[%s,%d]PP force memory error in function\n", __FUNCTION__,
                __LINE__);
        return NULL;
    }
#endif

    mwl->client_type = param->client_type;
    mwl->fd_memalloc = -1;
    if (param->device == NULL) {
        VPILOGE("device name error\n");
        goto err;
    }
    mwl->device      = param->device;
    mwl->mem_id      = param->mem_id;
    mwl->fd_memalloc = TranscodeOpenFD(param->device, O_RDWR | O_SYNC);
    if (mwl->fd_memalloc == -1) {
        VPILOGD("failed to open: %s\n", param->device);
        goto err;
    }
    mwl->edma_handle = TRANS_EDMA_init(param->device);
#ifdef PP_MEM_ERR_TEST
    if (pp_memory_check() != 0) {
        VPILOGE("[%s,%d]PP force memory error in function\n", __FUNCTION__,
                __LINE__);
        goto err;
    }
#endif
    return mwl;

err:
    pp_mwl_release(mwl);
    return NULL;
}

static i32 pp_mwl_malloc_linear(const void *instance, u32 size,
                                struct DWLLinearMem *info)
{
    VpiMwl *mwl = (VpiMwl *)instance;

#define getpagesize() (1)
    u32 pgsize             = getpagesize();
    int ret                = 0;
    struct mem_info params = { 0 };
    u32 alloc_flag         = 0;

#define EP_SIDE_EN (1 << 1)
#define RC_SIDE_EN (1 << 2)

    assert(mwl != NULL);
    assert(info != NULL);

    info->logical_size    = size;
    size                  = NEXT_MULTIPLE(size, pgsize);
    info->size            = size;
    info->virtual_address = NULL;
    info->bus_address     = 0;
    params.size           = info->size;

    info->bus_address_rc     = 0;
    info->virtual_address_ep = NULL;

    if (info->mem_type == DWL_MEM_TYPE_CPU_FILE_SINK)
        alloc_flag = RC_SIDE_EN;
    else if (info->mem_type == DWL_MEM_TYPE_DPB)
        alloc_flag = EP_SIDE_EN;
    else
        alloc_flag = EP_SIDE_EN | RC_SIDE_EN;

    if (alloc_flag & EP_SIDE_EN) {
        params.task_id      = mwl->mem_id;
        params.mem_location = EP_SIDE;
        params.size         = info->size;
        ret = ioctl(mwl->fd_memalloc, CB_TRANX_MEM_ALLOC, &params);
        if (ret) {
            VPILOGD("%s", "ERROR! No linear buffer available\n");
            return DWL_ERROR;
        }

#ifdef PP_MEM_ERR_TEST
        if (pp_memory_check() != 0) {
            VPILOGE("[%s,%d]PP force memory error in function\n", __FUNCTION__,
                    __LINE__);
            return DWL_ERROR;
        }
#endif

        info->bus_address = params.phy_addr;
        info->size        = params.size;
    }
    if (alloc_flag & RC_SIDE_EN) {
        info->virtual_address = fbtrans_get_huge_pages(info->size);
        info->bus_address_rc  = (addr_t)info->virtual_address;
        if (info->virtual_address == NULL)
            return DWL_ERROR;

#ifdef PP_MEM_ERR_TEST
        if (pp_memory_check() != 0) {
            VPILOGE("[%s,%d]PP force memory error in function\n", __FUNCTION__,
                    __LINE__);
            return DWL_ERROR;
        }
#endif
    }

    return DWL_OK;
}

static void pp_mwl_free_linear(const void *instance, struct DWLLinearMem *info)
{
    VpiMwl *mwl = (VpiMwl *)instance;
    int ret;
    struct mem_info params;

    assert(info != NULL);

    params.task_id      = mwl->mem_id;
    params.mem_location = EP_SIDE;
    params.phy_addr     = info->bus_address;
    params.size         = info->size;
    params.rc_kvirt     = info->rc_kvirt;
    if (info->bus_address != 0) {
        ret = ioctl(mwl->fd_memalloc, CB_TRANX_MEM_FREE, &params);
        VPILOGD("ret = %d\n", ret);
    }

    if (info->virtual_address) {
        fbtrans_free_huge_pages(info->virtual_address, info->size);
        info->virtual_address = NULL;
    }
}

static int pp_split_string(char **tgt, int max, char *src, char *split)
{
    int count   = 0;
    char *currp = src;
    char *p;
    char c;
    int i    = 0;
    int last = 0;

    while ((c = *currp++) != '\0') {
        if ((p = strchr(split, c)) == NULL) {
            if (count < max) {
                tgt[count][i++] = c;
            } else {
                VPILOGE("the split count exceeds max num\n");
                return -1;
            }
            last = 1;
        } else {
            if (last == 1) {
                tgt[count][i] = '\0';
                count++;
                i = 0;
            }
            last = 0;
        }
    }
    if (last == 1) {
        tgt[count][i] = '\0';
        count++;
    }

    return count;
}

static int pp_parse_low_res(VpiPPFilter *filter)
{
    int i = 0;
#define MAX_SEG_NUM 4
    char strs[MAX_SEG_NUM][256];
    char *pstrs[MAX_SEG_NUM];
    int seg_num = 0;
    char *p, *cp;

    for (i = 0; i < MAX_SEG_NUM; i++)
        pstrs[i] = &strs[i][0];

    filter->low_res_num = 0;

    if (!filter->low_res)
        return 0;

    seg_num =
        pp_split_string((char **)&pstrs, MAX_SEG_NUM, filter->low_res, "()");
    if (seg_num <= 0) {
        VPILOGE("can't find low_res info!\n");
        return -1;
    }

    while (filter->low_res_num < seg_num) {
        cp = strs[filter->low_res_num];
        if ((p = strchr(cp, ',')) != NULL) {
            filter->resizes[filter->low_res_num].x = atoi(cp);
            cp                                     = p + 1;
            if ((p = strchr(cp, ',')) != NULL) {
                filter->resizes[filter->low_res_num].y = atoi(cp);
                cp                                     = p + 1;
                if ((p = strchr(cp, ',')) != NULL) {
                    filter->resizes[filter->low_res_num].cw = atoi(cp);
                    filter->resizes[filter->low_res_num].ch = atoi(p + 1);
                    cp                                      = p + 1;
                    if ((p = strchr(cp, ',')) != NULL) {
                        cp = p + 1;
                    } else if (filter->low_res_num != 0) {
                        return -1;
                    }
                } else {
                    return -1;
                }
            } else {
                return -1;
            }
        }
        if ((p = strchr(cp, 'x')) == NULL) {
            if (cp[0] == 'd') {
                int n = atoi(cp + 1);
                if (n != 2 && n != 4 && n != 8) {
                    return -1;
                }
                filter->resizes[filter->low_res_num].sw = -n;
                filter->resizes[filter->low_res_num].sh = -n;
            } else if (filter->low_res_num != 0) {
                return -1;
            }
        } else {
            filter->resizes[filter->low_res_num].sw = atoi(cp);
            filter->resizes[filter->low_res_num].sh = atoi(p + 1);
        }
        if (filter->resizes[filter->low_res_num].sw == -1 &&
            filter->resizes[filter->low_res_num].sh == -1) {
            return -1;
        }
        VPILOGD("get resize %d: %d,%d, %d,%d, %dx%d\n", filter->low_res_num,
                filter->resizes[filter->low_res_num].x,
                filter->resizes[filter->low_res_num].y,
                filter->resizes[filter->low_res_num].cw,
                filter->resizes[filter->low_res_num].ch,
                filter->resizes[filter->low_res_num].sw,
                filter->resizes[filter->low_res_num].sh);
        filter->low_res_num++;
    }
    return 0;
}

static void pp_dump_config(PPClient *pp)
{
    VpiPPConfig *test_cfg = &pp->pp_config;
    PPConfig *dec_cfg     = &pp->dec_cfg;
    int i;

    VPILOGD("\ndump VpiPPConfig\n");

    VPILOGD("in_width = %d\n", test_cfg->in_width);
    VPILOGD("in_height = %d\n", test_cfg->in_height);
    VPILOGD("in_width_align = %d\n", test_cfg->in_width_align);
    VPILOGD("in_height_align = %d\n", test_cfg->in_height_align);
    VPILOGD("in_stride = %d\n", test_cfg->in_stride);
    VPILOGD("align = %d\n", test_cfg->align);
    VPILOGD("max_num_pics = %d\n", test_cfg->max_num_pics);
    VPILOGD("frame_size = %d\n", test_cfg->frame_size);
    VPILOGD("in_p010 = %d\n", test_cfg->in_p010);
    VPILOGD("compress_bypass = %d\n", test_cfg->compress_bypass);
    VPILOGD("cache_enable = %d\n", test_cfg->cache_enable);
    VPILOGD("shaper_enable = %d\n", test_cfg->shaper_enable);
    VPILOGD("pp_enabled = %d\n", test_cfg->pp_enabled);
    VPILOGD("out_p010 = %d\n", test_cfg->out_p010);

#ifdef SUPPORT_TCACHE
    TcacheContext *tcg = &pp->tcache_config;

    VPILOGD("\ndump TcacheContext\n");

    VPILOGD("t_in_fmt = %d\n", tcg->t_in_fmt);
    VPILOGD("t_out_fmt = %d\n", tcg->t_out_fmt);
    VPILOGD("rgb_cov_bd = %d\n", tcg->rgb_cov_bd);

    VPILOGD("t_in_width = %d\n", tcg->t_in_width);
    VPILOGD("t_in_height = %d\n", tcg->t_in_height);
    VPILOGD("t_wplanes = %d\n", tcg->t_wplanes);
    for (i = 0; i < tcg->t_wplanes; i++) {
        VPILOGD("t_in_stride[%d] = %d\n", i, tcg->t_in_stride[i]);
        VPILOGD("t_in_align_stride[%d] = %d\n", i, tcg->t_in_align_stride[i]);
        VPILOGD("t_in_plane_height[%d] = %d\n", i, tcg->t_in_plane_height[i]);
        VPILOGD("t_in_plane_size[%d] = %d\n", i, tcg->t_in_plane_size[i]);
    }
    VPILOGD("t_out_width = %d\n", tcg->t_out_width);
    VPILOGD("t_out_height = %d\n", tcg->t_out_height);
    VPILOGD("t_rplanes = %d\n", tcg->t_rplanes);
    for (i = 0; i < tcg->t_rplanes; i++) {
        VPILOGD("t_out_stride[%d] = %d\n", i, tcg->t_out_stride[i]);
        VPILOGD("t_out_align_stride[%d] = %d\n", i, tcg->t_out_align_stride[i]);
        VPILOGD("t_out_plane_height[%d] = %d\n", i, tcg->t_out_plane_height[i]);
        VPILOGD("t_out_plane_size[%d] = %d\n", i, tcg->t_out_plane_size[i]);
    }
#endif

    VPILOGD("\ndump PPConfig\n");
    VPILOGD("in_format = %d\n", dec_cfg->in_format);
    VPILOGD("in_stride = %d\n", dec_cfg->in_stride);
    VPILOGD("in_height = %d\n", dec_cfg->in_height);
    VPILOGD("in_width = %d\n", dec_cfg->in_width);

    VPILOGD("dump VpiPPConfig finished\n\n");
}

static int pp_add_extra_buffer(PPClient *pp, int need_buf_num, int cur_buf_num)
{
    int i           = 0;
    int ext_buf_num = 0;

    PPInst pp_inst = pp->pp_inst;
    ext_buf_num    = need_buf_num;

    if (pp == NULL || ext_buf_num <= 0) {
        VPILOGD("enc_need_buffers_num %d, current_pp_buffers_num %d, error!\n",
                ext_buf_num, cur_buf_num);
        return -1;
    }

    VPILOGD("pp_add_extra_buffer cur_buf_num=%d, ext_buf_num=%d\n", cur_buf_num,
            ext_buf_num);

    for (i = cur_buf_num; i < cur_buf_num + ext_buf_num; i++) {
        pp->pp_out_buffer[i].mem_type = DWL_MEM_TYPE_DPB;
        if (DWLMallocLinear(((PPContainer *)pp_inst)->dwl, pp->out_buf_size,
                            &pp->pp_out_buffer[i]) != DWL_OK) {
            VPILOGE("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n");
            return -1;
        }

        FifoPush(pp->pp_out_Fifo, &pp->pp_out_buffer[i],
                 FIFO_EXCEPTION_DISABLE);
    }
    pp->out_buf_nums += ext_buf_num;

    return 0;
}

static int pp_check_buffer_number_for_trans(PPClient *pp)
{
    int enc_need_buffers_num;
    int current_pp_buffers_num;

    enc_need_buffers_num   = pp->max_frames_delay;
    current_pp_buffers_num = pp->out_buf_nums;

    VPILOGD("out_buf_nums_init=%d, enc_need_buffers_num=%d, "
            "current_pp_buffers_num=%d\n",
            pp->out_buf_nums_init, enc_need_buffers_num,
            current_pp_buffers_num);

    if (pp->out_buf_nums_init + enc_need_buffers_num > current_pp_buffers_num) {
        return pp_add_extra_buffer(pp, enc_need_buffers_num,
                                   current_pp_buffers_num);
    }

    return 0;
}

static int pp_parse_params(PPClient *pp, VpiPPParams *params)
{
#ifdef SUPPORT_TCACHE
    TcacheContext *tcache_cfg = &pp->tcache_config;
    int i;
#endif
    VpiPPConfig *test_cfg = &pp->pp_config;

    VPILOGD("pp dump input parameters:\n");
    VPILOGD("\twidth=%d\n", params->width);
    VPILOGD("\theight=%d\n", params->height);
    VPILOGD("\tin_format=%s\n", params->in_format);
    VPILOGD("\tin_10bit=%d\n", params->in_10bit);
    VPILOGD("\talign=%d\n", params->align);
    VPILOGD("\tin_file_name=%s\n", params->in_file_name);
    VPILOGD("\tcompress_bypass=%d\n", params->compress_bypass);
    VPILOGD("\tcache_enable=%d\n", params->cache_enable);
    VPILOGD("\tshaper_enable=%d\n", params->shaper_enable);
    VPILOGD("\tnum_of_decoded_pics= %d\n", params->num_of_decoded_pics);

    test_cfg->max_num_pics    = params->num_of_decoded_pics;
    test_cfg->in_width        = params->width;
    test_cfg->in_height       = params->height;
    test_cfg->in_p010         = params->in_10bit;
    test_cfg->align           = params->align;
    test_cfg->compress_bypass = params->compress_bypass;

#ifdef SUPPORT_CACHE
    test_cfg->cache_enable  = params->cache_enable;
    test_cfg->shaper_enable = params->shaper_enable;
#endif

#ifdef SUPPORT_TCACHE
    tcache_cfg->t_in_width  = params->width;
    tcache_cfg->t_in_height = NEXT_MULTIPLE(params->height, PP_IN_HEIGHT_ALIGN);
#endif

    if (!strcmp(params->in_format, "nv12")) {
        test_cfg->in_p010 = 0;
#ifdef SUPPORT_TCACHE
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_NV12;
#endif
    } else if (!strcmp(params->in_format, "p010le")) {
        test_cfg->in_p010 = 1;
#ifdef SUPPORT_TCACHE
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_P010LE;
#endif
#ifdef SUPPORT_TCACHE
    } else if (!strcmp(params->in_format, "yuv420p")) {
        test_cfg->in_p010    = 0;
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_YUV420P;
    } else if (!strcmp(params->in_format, "iyuv")) {
        test_cfg->in_p010    = 0;
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_YUV420P;
    } else if (!strcmp(params->in_format, "yuv422p")) {
        test_cfg->in_p010    = 0;
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_YUV422P;
    } else if (!strcmp(params->in_format, "nv21")) {
        test_cfg->in_p010    = 0;
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_NV21;
    } else if (!strcmp(params->in_format, "yuv420p10le")) {
        test_cfg->in_p010    = 1;
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_YUV420P10LE;
    } else if (!strcmp(params->in_format, "yuv420p10be")) {
        test_cfg->in_p010    = 1;
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_YUV420P10BE;
    } else if (!strcmp(params->in_format, "yuv422p10le")) {
        test_cfg->in_p010    = 1;
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_YUV422P10LE;
    } else if (!strcmp(params->in_format, "yuv422p10be")) {
        test_cfg->in_p010    = 1;
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_YUV422P10BE;
    } else if (!strcmp(params->in_format, "p010be")) {
        test_cfg->in_p010    = 1;
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_P010BE;
    } else if (!strcmp(params->in_format, "yuv444p")) {
        test_cfg->in_p010    = 0;
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_YUV444P;
    } else if (!strcmp(params->in_format, "rgb24")) {
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_RGB24;
        if (params->in_10bit) {
            test_cfg->in_p010      = 1;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_10;
        } else {
            test_cfg->in_p010      = 0;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_8;
        }
    } else if (!strcmp(params->in_format, "bgr24")) {
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_BGR24;
        if (params->in_10bit) {
            test_cfg->in_p010      = 1;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_10;
        } else {
            test_cfg->in_p010      = 0;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_8;
        }
    } else if (!strcmp(params->in_format, "argb")) {
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_ARGB;
        if (params->in_10bit) {
            test_cfg->in_p010      = 1;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_10;
        } else {
            test_cfg->in_p010      = 0;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_8;
        }
    } else if (!strcmp(params->in_format, "rgba")) {
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_RGBA;
        if (params->in_10bit) {
            test_cfg->in_p010      = 1;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_10;
        } else {
            test_cfg->in_p010      = 0;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_8;
        }
    } else if (!strcmp(params->in_format, "abgr")) {
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_ABGR;
        if (params->in_10bit) {
            test_cfg->in_p010      = 1;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_10;
        } else {
            test_cfg->in_p010      = 0;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_8;
        }
    } else if (!strcmp(params->in_format, "bgra")) {
        tcache_cfg->t_in_fmt = TCACHE_PIX_FMT_BGRA;
        if (params->in_10bit) {
            test_cfg->in_p010      = 1;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_10;
        } else {
            test_cfg->in_p010      = 0;
            tcache_cfg->rgb_cov_bd = TCACHE_COV_BD_8;
        }
#endif
    } else if (!strcmp(params->in_format, "vpe")){
        if(params->in_10bit)
            test_cfg->in_p010 = 1;
        else
            test_cfg->in_p010 = 0;
    } else {
        VPILOGE("unsupport input format %s!!!\n", params->in_format);
        return -1;
    }

    VPILOGD("pp get test config:\n");
    VPILOGD("\tin_width=%d\n", test_cfg->in_width);
    VPILOGD("\tin_height=%d\n", test_cfg->in_height);
    VPILOGD("\tin_p010=%d\n", test_cfg->in_p010);
    VPILOGD("\talign=%d\n", test_cfg->align);

#ifdef SUPPORT_TCACHE
    tcache_cfg->t_out_fmt =
        tcache_get_output_format(tcache_cfg->t_in_fmt,
                                 test_cfg->in_p010 ? 10 : 8);
    if (tcache_cfg->t_out_fmt == -1) {
        VPILOGE("tcache get output format failed!\n");
        return -1;
    }
    VPILOGD("set tcache intput format %d -> output format %d\n",
            tcache_cfg->t_in_fmt, tcache_cfg->t_out_fmt);

    u32 maxbottom = 0;
    u32 maxright  = 0;
    for (i = 0; i < 4; i++) {
        if (!params->ppu_cfg[i].crop.enabled)
            continue;
        u32 thisbottom = NEXT_MULTIPLE(params->ppu_cfg[i].crop.y +
                                           params->ppu_cfg[i].crop.height,
                                       2);
        if (thisbottom > maxbottom) {
            maxbottom = thisbottom;
        }
        u32 thisright =
            params->ppu_cfg[i].crop.x + params->ppu_cfg[i].crop.width;
        if (thisright > maxright) {
            maxright = thisright;
        }
    }
    if ((maxbottom || maxright) &&
        (maxbottom < params->height || maxright < params->width)) {
        test_cfg->crop_enabled = 1;
        test_cfg->crop_w       = maxright;
        test_cfg->crop_h       = maxbottom;
    }
    VPILOGD("\tcrop_enabled=%d\n", test_cfg->crop_enabled);
    VPILOGD("\tcrop_w=%d\n", test_cfg->crop_w);
    VPILOGD("\tcrop_h=%d\n", test_cfg->crop_h);
#endif

    return 0;
}

#ifdef SUPPORT_TCACHE
static void pp_tcache_calc_size(PPClient *pp)
{
    u32 i;
    TcacheContext *tcg = &pp->tcache_config;

    tcg->t_wplanes = tcache_get_planes(tcg->t_in_fmt);

    for (i = 0; i < tcg->t_wplanes; i++) {
        tcg->t_in_stride[i] =
            tcache_get_stride(tcg->t_in_width, tcg->t_in_fmt, i, 1);
        tcg->t_in_align_stride[i] = tcache_get_stride_align(
            NEXT_MULTIPLE(tcg->t_in_stride[i], TCACHE_RAW_INPUT_ALIGNMENT));
        tcg->t_in_plane_height[i] =
            tcache_get_height(NEXT_MULTIPLE(tcg->t_in_height, 4), tcg->t_in_fmt,
                              i);
        tcg->t_in_plane_size[i] =
            tcg->t_in_align_stride[i] * tcg->t_in_plane_height[i];
        VPILOGV("tcache read plane %d, stride %d align to %d, height "
                "%d\n",
                i, tcg->t_in_stride[i], tcg->t_in_align_stride[i],
                tcg->t_in_plane_height[i]);
    }

    tcg->t_out_width  = tcg->t_in_width;
    tcg->t_out_height = tcg->t_in_height;

    tcg->t_rplanes = tcache_get_planes(tcg->t_out_fmt);

    for (i = 0; i < tcg->t_rplanes; i++) {
        tcg->t_out_stride[i] =
            tcache_get_stride(tcg->t_out_width, tcg->t_out_fmt, i, 1);
        tcg->t_out_align_stride[i] = tcache_get_stride_align(
            NEXT_MULTIPLE(tcg->t_out_stride[i], PP_FETCH_ALIGN));
        tcg->t_out_plane_height[i] =
            tcache_get_height(NEXT_MULTIPLE(tcg->t_out_height,
                                            PP_OUT_HEIGHT_ALIGN),
                              tcg->t_out_fmt, i);
        tcg->t_out_plane_size[i] =
            tcg->t_out_align_stride[i] * tcg->t_out_plane_height[i];
        VPILOGV("tcache write plane %d, stride %d align to %d, height "
                "%d\n",
                i, tcg->t_out_stride[i], tcg->t_out_align_stride[i],
                tcg->t_out_plane_height[i]);
    }
}

static int pp_tcache_config(PPClient *pp)
{
    u32 edma_link_ep_base[MAX_INPUT_PLANE + MAX_OUTPUT_PLANE] = {
        0x00000000, 0x02000000, 0x03000000, 0x02000000, 0x01000000
    };

    int ret;
    u32 i, plane, wplanes;
    u32 link_size = 0;
    u32 offset[MAX_INPUT_PLANE];
    u32 offset_ep[MAX_INPUT_PLANE];
    u32 block_size[MAX_INPUT_PLANE];
    struct dma_link_table *edma_link;
    u64 edma_link_rc_base[3];
    TcacheContext *tcg = &pp->tcache_config;
    TCACHE_PARAM tcache_param;
    TCACHE_PARAM *pParam  = &tcache_param;
    VpiPPConfig *test_cfg = &pp->pp_config;
    u32 valid_in_plane_size[MAX_INPUT_PLANE];
    u32 valid_out_plane_size[MAX_OUTPUT_PLANE];

    memset(pParam, 0, sizeof(*pParam));

    pParam->RGB_COV_A = 0x4c85;
    pParam->RGB_COV_B = 0x962b;
    pParam->RGB_COV_C = 0x1d50;
    pParam->RGB_COV_E = 0x9090;
    pParam->RGB_COV_F = 0xb694;

    pParam->write_format = tcg->t_in_fmt;
    pParam->write_bd     = tcg->rgb_cov_bd;

    pParam->dtrc_read_enable   = 0;
    pParam->hs_enable          = 1;
    pParam->hs_dma_ch          = 0;
    pParam->hs_go_toggle_count = 0xf4;
    pParam->hs_ce              = tcg->t_wplanes;

    edma_link_rc_base[0] = pp->pp_in_buffer->buffer.bus_address_rc;
    edma_link_rc_base[1] =
        edma_link_rc_base[0] +
        tcache_get_block_size(tcg->t_in_align_stride[0], tcg->t_in_fmt, 0);
    edma_link_rc_base[2] =
        edma_link_rc_base[1] +
        tcache_get_block_size(tcg->t_in_align_stride[1], tcg->t_in_fmt, 1);

    for (plane = 0; plane < tcg->t_wplanes; plane++) {
        pParam->writeStride[plane]    = tcg->t_in_align_stride[plane];
        pParam->writeStartAddr[plane] = edma_link_ep_base[plane] & 0x03FFFFFF;

        if (test_cfg->crop_enabled && test_cfg->crop_h < tcg->t_in_height) {
            valid_in_plane_size[plane] =
                tcg->t_in_align_stride[plane] *
                tcache_get_height(test_cfg->crop_h, tcg->t_in_fmt, plane);
        } else {
            valid_in_plane_size[plane] = tcg->t_in_plane_size[plane];
        }
        VPILOGV("valid_in_plane_size[%d] = %d\n", plane,
                valid_in_plane_size[plane]);

#ifdef ENABLE_HW_HANDSHAKE
        pParam->writeEndAddr[plane] =
            pParam->writeStartAddr[plane] + valid_in_plane_size[plane] - 1;
#else
        if (tcg->t_in_height <= 256) {
            if (test_cfg->crop_enabled && test_cfg->crop_h < tcg->t_in_height) {
                pParam->writeEndAddr[plane] =
                    pParam->writeStartAddr[plane] +
                    tcg->t_in_align_stride[plane] *
                        tcache_get_height(test_cfg->crop_h, tcg->t_in_fmt,
                                          plane) -
                    1;
            } else {
                pParam->writeEndAddr[plane] = pParam->writeStartAddr[plane] +
                                              tcg->t_in_plane_size[plane] - 1;
            }
        } else {
            if (test_cfg->crop_enabled && test_cfg->crop_h < 256) {
                pParam->writeEndAddr[plane] =
                    pParam->writeStartAddr[plane] +
                    tcg->t_in_align_stride[plane] *
                        tcache_get_height(test_cfg->crop_h, tcg->t_in_fmt,
                                          plane) -
                    1;
            } else {
                pParam->writeEndAddr[plane] =
                    pParam->writeStartAddr[plane] +
                    tcg->t_in_align_stride[plane] *
                        tcache_get_height(256, tcg->t_in_fmt, plane) -
                    1;
            }
        }
#endif
        block_size[plane] = tcache_get_block_size(tcg->t_in_align_stride[plane],
                                                  tcg->t_in_fmt, plane);
        link_size += (valid_in_plane_size[plane] + block_size[plane] - 1) /
                     block_size[plane];
        VPILOGV("start addr: 0x%08x, end addr: 0x%08x, block size: %d, link "
                "size: %d\n",
                pParam->writeStartAddr[plane], pParam->writeEndAddr[plane],
                block_size[plane], link_size);
    }

    edma_link = (struct dma_link_table *)pp->edma_link.virtual_address;
    memset(edma_link, 0, (link_size + 1) * sizeof(struct dma_link_table));
    wplanes = tcg->t_wplanes;

    offset[0] = offset[1] = offset[2] = 0;
    offset_ep[0] = offset_ep[1] = offset_ep[2] = 0;
    for (i = 0; i < link_size; i++) {
        if (i == link_size - 1)
            edma_link[i].control = 0x00000009;
        else
            edma_link[i].control = 0x00000001;

        edma_link[i].size =
            MIN((u32)block_size[i % wplanes],
                valid_in_plane_size[i % wplanes] - offset_ep[i % wplanes]);
        edma_link[i].sar_low =
            (edma_link_rc_base[i % wplanes] + offset[i % wplanes]) & 0xffffffff;
        edma_link[i].sar_high =
            (edma_link_rc_base[i % wplanes] + offset[i % wplanes]) >> 32;
#ifdef ENABLE_HW_HANDSHAKE
        edma_link[i].dst_low =
            (edma_link_ep_base[i % wplanes] + offset_ep[i % wplanes]) &
            0xffffffff;
#else
        edma_link[i].dst_low =
            (edma_link_ep_base[i % wplanes] +
             (offset_ep[i % wplanes] % (block_size[i % wplanes] * 4))) &
            0xffffffff;
#endif
        edma_link[i].dst_high = 0;
        offset[i % wplanes] += HUGE_PGAE_SIZE;
        offset_ep[i % wplanes] += edma_link[i].size;
    }

    VPILOGV("edma link table:\n");
    for (i = 0; i < link_size; i++) {
        VPILOGV(" i : %d\n", i);
        VPILOGV("\t%#010X\n", edma_link[i].control);
        VPILOGV("\t%#010X\n", edma_link[i].size);
        VPILOGV("\t%#010X\n", edma_link[i].sar_low);
        VPILOGV("\t%#010X\n", edma_link[i].sar_high);
        VPILOGV("\t%#010X\n", edma_link[i].dst_low);
        VPILOGV("\t%#010X\n", edma_link[i].dst_high);
    }

#ifndef ENABLE_HW_HANDSHAKE
    {
        typedef u32(*TCacheLink)[10];
        TCacheLink tcache_link;
        u32 val;
        u32 last_height;
        int i, j;

        tcache_link = (TCacheLink)((u8 *)pp->edma_link.virtual_address +
                              link_size * sizeof(struct dma_link_table));
        memset(tcache_link, 0, 2 * 10 * sizeof(u32));

        if (test_cfg->crop_enabled && test_cfg->crop_h < tcg->t_in_height) {
            last_height = test_cfg->crop_h % 256;
        } else {
            last_height = tcg->t_in_height % 256;
        }
        if (last_height == 0) {
            last_height = 256;
        }

        val = (pParam->write_format & ((1 << 5) - 1)) << 0;
        val |= (pParam->write_bd & ((1 << 2) - 1)) << 5;
        tcache_link[0][0] = val;
        tcache_link[0][1] = pParam->writeStartAddr[0];
        tcache_link[0][2] = pParam->writeEndAddr[0];
        tcache_link[0][3] = pParam->writeStride[0];
        tcache_link[0][4] = pParam->writeStartAddr[1];
        tcache_link[0][5] = pParam->writeEndAddr[1];
        tcache_link[0][6] = pParam->writeStride[1];
        tcache_link[0][7] = pParam->writeStartAddr[2];
        tcache_link[0][8] = pParam->writeEndAddr[2];
        tcache_link[0][9] = pParam->writeStride[2];

        tcache_link[1][0] = val;
        tcache_link[1][1] = pParam->writeStartAddr[0];
        tcache_link[1][2] =
            pParam->writeStartAddr[0] +
            pParam->writeStride[0] *
                tcache_get_height(last_height, tcg->t_in_fmt, 0) -
            1;
        tcache_link[1][3] = pParam->writeStride[0];
        tcache_link[1][4] = pParam->writeStartAddr[1];
        tcache_link[1][5] =
            pParam->writeStartAddr[1] +
            pParam->writeStride[1] *
                tcache_get_height(last_height, tcg->t_in_fmt, 1) -
            1;
        tcache_link[1][6] = pParam->writeStride[1];
        tcache_link[1][7] = pParam->writeStartAddr[2];
        tcache_link[1][8] =
            pParam->writeStartAddr[2] +
            pParam->writeStride[2] *
                tcache_get_height(last_height, tcg->t_in_fmt, 2) -
            1;
        tcache_link[1][9] = pParam->writeStride[2];

        VPILOGV("tcache link table:\n");
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 10; j++) {
                VPILOGV("\t%#010X\n", tcache_link[i][j]);
            }
        }
    }
#endif

    pParam->read_format = tcg->t_out_fmt;
    pParam->read_count  = 0;
    pParam->read_client = 0;

    for (plane = 0; plane < tcg->t_rplanes; plane++) {
        pParam->readStride[plane]      = tcg->t_out_align_stride[plane];
        pParam->readValidStride[plane] = tcg->t_out_align_stride[plane];

        if (test_cfg->crop_enabled) {
            valid_out_plane_size[plane] =
                tcg->t_out_align_stride[plane] *
                tcache_get_height(test_cfg->crop_h, tcg->t_out_fmt, plane);
            if (tcg->t_out_fmt == TCACHE_PIX_FMT_NV12 ||
                test_cfg->crop_w < tcg->t_out_width) {
                u32 crop_align =
                    (tcg->t_out_fmt == TCACHE_PIX_FMT_NV12) ? 1 : 32;
                pParam->readValidStride[plane] =
                    tcache_get_stride(test_cfg->crop_w, tcg->t_out_fmt, plane,
                                      crop_align);
            }
        } else {
            valid_out_plane_size[plane] = tcg->t_out_plane_size[plane];
        }

        pParam->readStartAddr[plane] =
            edma_link_ep_base[MAX_INPUT_PLANE + plane] & 0x03FFFFFF;
        pParam->readEndAddr[plane] =
            pParam->readStartAddr[plane] + valid_out_plane_size[plane] -
            (pParam->readStride[plane] - pParam->readValidStride[plane]) - 1;

        VPILOGV("start addr: 0x%08x, end addr: 0x%08x, read stride: "
                "%d, read valid stride: %d\n",
                pParam->readStartAddr[plane], pParam->readEndAddr[plane],
                pParam->readStride[plane], pParam->readValidStride[plane]);
    }

    if (test_cfg->crop_enabled && test_cfg->crop_h < tcg->t_in_height) {
        pParam->hs_image_height =
            tcache_get_height(test_cfg->crop_h, tcg->t_in_fmt, 0);
    } else {
        pParam->hs_image_height = tcg->t_in_height;
    }
#ifdef ENABLE_HW_HANDSHAKE
    pParam->hs_enable = 1;
#else
    pParam->hs_enable = 0;
#endif

    TCACHE_config((TCACHE_HANDLE)pp->tcache_handle, (TCACHE_PARAM *)pParam);
    VPILOGV("config read link\n");

#ifdef ENABLE_HW_HANDSHAKE
    ret =
        TRANS_EDMA_RC2EP_link_config(pp->edma_handle,
                                     pp->edma_link.bus_address_rc,
                                     pp->edma_link.virtual_address, link_size);
#else
    ret = TRANS_EDMA_RC2EP_link_config(pp->edma_handle,
                                       pp->edma_link.bus_address_rc,
                                       pp->edma_link.virtual_address, link_size,
                                       MIN(link_size, tcg->t_wplanes * 4));
#endif
    if (ret < 0)
        return -1;

#ifdef PP_EDMA_ERR_TEST
    if (pp_edma_eerror_check() != 0) {
        VPILOGE("[%s,%d]PP force edma error in function\n", __FUNCTION__,
                __LINE__);
        return -1;
    }
#endif
    pp->dec_cfg.pp_in_buffer.bus_address = pParam->readStartAddr[0];
    return 0;
}

#endif

static int pp_calc_pic_size(PPClient *pp)
{
    VpiPPConfig *test_cfg = &pp->pp_config;
#ifdef SUPPORT_TCACHE
    TcacheContext *tcache_cfg = &pp->tcache_config;
#endif
    int i;

#ifdef SUPPORT_TCACHE
    test_cfg->in_stride =
        NEXT_MULTIPLE(test_cfg->in_width * (test_cfg->in_p010 ? 2 : 1),
                      PP_FETCH_ALIGN);
#else
    test_cfg->in_stride =
        NEXT_MULTIPLE(test_cfg->in_width * (test_cfg->in_p010 ? 2 : 1), 16);
#endif
    test_cfg->in_width_align =
        test_cfg->in_stride / (test_cfg->in_p010 ? 2 : 1);
    test_cfg->in_height_align =
        NEXT_MULTIPLE(test_cfg->in_height, PP_IN_HEIGHT_ALIGN);
    test_cfg->frame_size =
        (test_cfg->in_stride * test_cfg->in_height_align * 3) / 2;

#ifdef SUPPORT_TCACHE
    pp_tcache_calc_size(pp);
    test_cfg->frame_size = 0;
    for (i = 0; i < tcache_cfg->t_rplanes; i++) {
        test_cfg->frame_size += tcache_cfg->t_in_plane_size[i];
    }
#endif
    return 0;
}

static int pp_buf_init(PPClient *pp)
{
    u32 i;
    PPInst pp_inst        = pp->pp_inst;
    VpiPPConfig *test_cfg = &pp->pp_config;
    PPConfig *dec_cfg     = &pp->dec_cfg;
    u32 pp_out_byte_per_pixel;
    u32 pp_width, pp_height, pp_stride, pp_stride_2;
    u32 pp_buff_size;

    /* calc output buffer size */
    pp->out_buf_size = 0;
    for (i = 0; i < 4; i++) {
        if (!dec_cfg->ppu_config[i].enabled)
            continue;

        pp_out_byte_per_pixel =
            (dec_cfg->ppu_config[i].out_cut_8bits || test_cfg->in_p010 == 0) ?
                1 :
                ((dec_cfg->ppu_config[i].out_p010 ||
                  (dec_cfg->ppu_config[i].tiled_e && test_cfg->in_p010)) ?
                     2 :
                     1);

        if (dec_cfg->ppu_config[i].tiled_e) {
            pp_width  = NEXT_MULTIPLE(dec_cfg->ppu_config[i].scale.width, 4);
            pp_height = NEXT_MULTIPLE(dec_cfg->ppu_config[i].scale.height,
                                      PP_OUT_HEIGHT_ALIGN) /
                        4;
            pp_stride = NEXT_MULTIPLE(4 * pp_width * pp_out_byte_per_pixel,
                                      ALIGN(test_cfg->align));
            dec_cfg->ppu_config[i].ystride = pp_stride;
            dec_cfg->ppu_config[i].cstride = pp_stride;
            dec_cfg->ppu_config[i].align   = test_cfg->align;
            VPILOGD("pp[%d] luma, pp_width=%d, pp_height=%d, "
                    "pp_stride=%d(0x%x), pp_out_byte_per_pixel=%d, "
                    "align=%d\n",
                    i, pp_width, pp_height, pp_stride, pp_stride,
                    pp_out_byte_per_pixel, test_cfg->align);
            pp_buff_size = pp_stride * pp_height;
            pp_buff_size += PP_LUMA_BUF_RES;
            /* chroma */
            if (!dec_cfg->ppu_config[i].monochrome) {
                pp_height =
                    NEXT_MULTIPLE(dec_cfg->ppu_config[i].scale.height / 2,
                                  PP_OUT_HEIGHT_ALIGN) /
                    4; /*fix height align 2*/
                VPILOGD("pp[%d] chroma, pp_height=%d\n", i, pp_height);
                pp_buff_size += pp_stride * pp_height;
                pp_buff_size += PP_CHROMA_BUF_RES;
            }
            VPILOGD("pp[%d] pp_buff_size=%d(0x%x)\n", i, pp_buff_size,
                    pp_buff_size);
        } else {
            pp_width  = dec_cfg->ppu_config[i].scale.width;
            pp_height = dec_cfg->ppu_config[i].scale.height;
            pp_stride = NEXT_MULTIPLE(pp_width * pp_out_byte_per_pixel, 4);
            pp_stride_2 =
                NEXT_MULTIPLE(pp_width / 2 * pp_out_byte_per_pixel, 4);
            pp_buff_size = pp_stride * pp_height;
            pp_buff_size += PP_LUMA_BUF_RES;
            if (!dec_cfg->ppu_config[i].monochrome) {
                if (!dec_cfg->ppu_config[i].planar)
                    pp_buff_size += pp_stride * pp_height / 2;
                else
                    pp_buff_size += pp_stride_2 * pp_height;
                pp_buff_size += PP_LUMA_BUF_RES;
            }
            VPILOGD("pp[%d], tile_e=0 pp_width=%d , pp_height=%d , "
                    "pp_stride=%d(0x%x),pp_out_byte_per_pixel=%d\n",
                    i, pp_width, pp_height, pp_stride, pp_stride,
                    pp_out_byte_per_pixel);
            VPILOGD("pp[%d], tile_e=0 pp_buff_size=%d(0x%x)\n", i, pp_buff_size,
                    pp_buff_size);
        }
        pp->out_buf_size += NEXT_MULTIPLE(pp_buff_size, 16);
    }

#ifdef SUPPORT_DEC400
    pp->dec_table_offset = pp->out_buf_size;
    pp->out_buf_size += DEC400_PPn_TABLE_OFFSET(4);
#endif

#ifdef SUPPORT_TCACHE
    pp->edma_link.mem_type = DWL_MEM_TYPE_CPU_FILE_SINK;
    if (DWLMallocLinear(((PPContainer *)pp_inst)->dwl, 0x4000,
                        &pp->edma_link) != DWL_OK) {
        VPILOGE("UNABLE TO ALLOCATE EDMA LINK BUFFER MEMORY\n");
        goto end;
    }
    VPILOGD("[%s,%d]edma_link: bus_address=0x%llx, bus_address_rc=0x%llx, "
            "virtual_address %p, virtual_address_ep %p, size %d \n",
            __FUNCTION__, __LINE__, pp->edma_link.bus_address,
            pp->edma_link.bus_address_rc, pp->edma_link.virtual_address,
            pp->edma_link.virtual_address_ep, pp->edma_link.size);
#endif

#ifdef PP_MEM_ERR_TEST
    if (pp_memory_check() != 0) {
        VPILOGE("[%s,%d]PP force memory error in function\n", __FUNCTION__,
                __LINE__);
        return -1;
    }
#endif

    /*used for buffer management*/
    FifoInit(OUTPUT_BUF_DEPTH, &pp->pp_out_Fifo);
    pp->out_buf_nums_init = 3;
    pp->out_buf_nums      = pp->out_buf_nums_init + pp->max_frames_delay;
    if (pp->out_buf_nums > OUTPUT_BUF_DEPTH) {
        VPILOGE("TOO MANY BUFFERS REQUEST, max_frames_delay=%d\n",
                pp->max_frames_delay);
        goto end;
    }
    for (i = 0; i < pp->out_buf_nums; i++) {
        pp->pp_out_buffer[i].mem_type = DWL_MEM_TYPE_DPB;
        VPILOGD("Malloc No.%d pp out buffrer,(size=%d)\n", i, pp->out_buf_size);
        if (DWLMallocLinear(((PPContainer *)pp_inst)->dwl, pp->out_buf_size,
                            &pp->pp_out_buffer[i]) != DWL_OK) {
            VPILOGE("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n");
            goto end;
        }

        VPILOGD("[%s,%d]out_buffer i %d, bus_address=0x%llx, "
                "bus_address_rc=0x%llx, virtual_address %p, virtual_address_ep "
                "%p, size %d \n",
                __FUNCTION__, __LINE__, i, pp->pp_out_buffer[i].bus_address,
                pp->pp_out_buffer[i].bus_address_rc,
                pp->pp_out_buffer[i].virtual_address,
                pp->pp_out_buffer[i].virtual_address_ep,
                pp->pp_out_buffer[i].size);

#ifdef PP_MEM_ERR_TEST
        if (pp_memory_check() != 0) {
            VPILOGE("[%s,%d]PP force memory error in function\n", __FUNCTION__,
                    __LINE__);
            return -1;
        }
#endif

        FifoPush(pp->pp_out_Fifo, &pp->pp_out_buffer[i],
                 FIFO_EXCEPTION_DISABLE);
    }

    return 0;

end:
    return -1;
}

static void pp_request_buf(PPClient *pp)
{
    struct DWLLinearMem *pp_out_buffer;

    pthread_mutex_lock(&pp->pp_mutex);
    FifoPop(pp->pp_out_Fifo, (FifoObject *)&pp_out_buffer,
            FIFO_EXCEPTION_DISABLE);
    pp->dec_cfg.pp_out_buffer = *pp_out_buffer;
    VPILOGD("[pp] get buffer bus address=%p, virtual address=%p\n",
            (void *)pp_out_buffer->bus_address, pp_out_buffer->virtual_address);
    pthread_mutex_unlock(&pp->pp_mutex);
}

static int pp_buf_release(PPClient *pp)
{
    u32 i;
    PPInst pp_inst = pp->pp_inst;

    pthread_mutex_lock(&pp->pp_mutex);
    if (pp->pp_out_Fifo) {
        FifoRelease(pp->pp_out_Fifo);
        pp->pp_out_Fifo = NULL;
    }
    pthread_mutex_unlock(&pp->pp_mutex);

    if (pp->pp_inst != NULL) {
        for (i = 0; i < pp->out_buf_nums; i++) {
            if (pp->pp_out_buffer[i].bus_address ||
                pp->pp_out_buffer[i].bus_address_rc)
                DWLFreeLinear(((PPContainer *)pp_inst)->dwl,
                              &pp->pp_out_buffer[i]);
        }
#ifdef SUPPORT_TCACHE
        if (pp->edma_link.bus_address || pp->edma_link.bus_address_rc)
            DWLFreeLinear(((PPContainer *)pp_inst)->dwl, &pp->edma_link);
#endif
    }
    return 0;
}

static void pp_get_next_pic(PPClient *pp, PPDecPicture *hpic)
{
    u32 i, j;
    u32 index      = 0;
    PPInst pp_inst = pp->pp_inst;
    PPContainer *pp_c;
    struct DecPicturePpu *pic = &hpic->pp_pic;

    pp_c = (PPContainer *)pp_inst;

    for (i = 0; i < DEC_MAX_OUT_COUNT; i++) {
        if (!hpic->pictures[i].pp_enabled) {
            /* current mem alloc, the dec table is added after pic mem for each
             * ppN. */
            index += 1;
            continue;
        }

        /*pp0->vce is not the real scenario, just use to test performance.*/
        if (i == 0) {
            j = 0;
        } else {
            j = i + 1;
        }

        pic->pictures[j].sequence_info.bit_depth_luma =
            hpic->pictures[i].bit_depth_luma;
        pic->pictures[j].sequence_info.bit_depth_chroma =
            hpic->pictures[i].bit_depth_chroma;

        pic->pictures[j].sequence_info.pic_width = hpic->pictures[i].pic_width;
        pic->pictures[j].sequence_info.pic_height =
            hpic->pictures[i].pic_height;

        pic->pictures[j].picture_info.format = hpic->pictures[i].output_format;
        pic->pictures[j].picture_info.pixel_format =
            hpic->pictures[i].pixel_format;

        pic->pictures[j].pic_width     = hpic->pictures[i].pic_width;
        pic->pictures[j].pic_height    = hpic->pictures[i].pic_height;
        pic->pictures[j].pic_stride    = hpic->pictures[i].pic_stride;
        pic->pictures[j].pic_stride_ch = hpic->pictures[i].pic_stride_ch;
        pic->pictures[j].pp_enabled    = hpic->pictures[i].pp_enabled;

#ifdef SUPPORT_TCACHE
        pic->pictures[j].luma.virtual_address   = NULL;
        pic->pictures[j].chroma.virtual_address = NULL;
#else
        pic->pictures[j].luma.virtual_address =
            (u32 *)hpic->pictures[i].output_picture;
        pic->pictures[j].chroma.virtual_address =
            (u32 *)hpic->pictures[i].output_picture_chroma;
#endif
        pic->pictures[j].luma.bus_address =
            hpic->pictures[i].output_picture_bus_address;
        pic->pictures[j].chroma.bus_address =
            hpic->pictures[i].output_picture_chroma_bus_address;

        if ((hpic->pictures[i].output_format == DEC_OUT_FRM_TILED_4X4) &&
            hpic->pictures[i].pp_enabled) {
            pic->pictures[j].luma.size =
                pic->pictures[j].pic_stride * hpic->pictures[i].pic_height / 4;
            if (((hpic->pictures[i].pic_height / 4) & 1) == 1)
                pic->pictures[j].chroma.size =
                    pic->pictures[j].pic_stride_ch *
                    (hpic->pictures[i].pic_height / 4 + 1) / 2;
            else
                pic->pictures[j].chroma.size = pic->pictures[j].pic_stride_ch *
                                               hpic->pictures[i].pic_height / 8;
        } else {
            pic->pictures[j].luma.size =
                pic->pictures[j].pic_stride * hpic->pictures[i].pic_height;
            pic->pictures[j].chroma.size = pic->pictures[j].pic_stride_ch *
                                           hpic->pictures[i].pic_height / 2;
        }

#ifdef SUPPORT_DEC400
        pic->pictures[j].pic_compressed_status      = 2;
        pic->pictures[j].luma_table.virtual_address = NULL;
        pic->pictures[j].luma_table.bus_address =
            pp_c->pp_out_buffer.bus_address + pp->dec_table_offset +
            DEC400_PPn_Y_TABLE_OFFSET(index);
        pic->pictures[j].chroma_table.virtual_address = NULL;
        pic->pictures[j].chroma_table.bus_address =
            pp_c->pp_out_buffer.bus_address + pp->dec_table_offset +
            DEC400_PPn_UV_TABLE_OFFSET(index);
        index++;
#else
        pic->pictures[j].pic_compressed_status = 0;
#endif

        VPILOGV("[pp] dump pic info: \n \
           pic->pictures[%d].sequence_info.bit_depth_luma = %d \n \
           pic->pictures[%d].sequence_info.bit_depth_chroma = %d \n \
           pic->pictures[%d].picture_info.format = %d \n \
           pic->pictures[%d].picture_info.pixel_format = %d \n \
           pic->pictures[%d].pic_width = %d \n \
           pic->pictures[%d].pic_height = %d \n \
           pic->pictures[%d].pic_stride = %d \n \
           pic->pictures[%d].pic_stride_ch = %d \n \
           pic->pictures[%d].pp_enabled = %d \n \
           pic->pictures[%d].luma.virtual_address = %p \n \
           pic->pictures[%d].luma.bus_address = 0x%08lx \n \
           pic->pictures[%d].chroma.virtual_address =%p \n \
           pic->pictures[%d].chroma.bus_address = 0x%08lx \n \
           pic->pictures[%d].luma.size = %d \n \
           pic->pictures[%d].chroma.size = %d \n \
           pic->pictures[%d].luma_table.virtual_address  = %p \n \
           pic->pictures[%d].luma_table.bus_address = 0x%08lx \n \
           pic->pictures[%d].chroma_table.virtual_address  = %p \n \
           pic->pictures[%d].chroma_table.bus_address = 0x%08lx \n",
                j, pic->pictures[j].sequence_info.bit_depth_luma, j,
                pic->pictures[j].sequence_info.bit_depth_chroma, j,
                pic->pictures[j].picture_info.format, j,
                pic->pictures[j].picture_info.pixel_format, j,
                pic->pictures[j].pic_width, j, pic->pictures[j].pic_height, j,
                pic->pictures[j].pic_stride, j, pic->pictures[j].pic_stride_ch,
                j, pic->pictures[j].pp_enabled, j,
                pic->pictures[j].luma.virtual_address, j,
                pic->pictures[j].luma.bus_address, j,
                pic->pictures[j].chroma.virtual_address, j,
                pic->pictures[j].chroma.bus_address, j,
                pic->pictures[j].luma.size, j, pic->pictures[j].chroma.size, j,
                pic->pictures[j].luma_table.virtual_address, j,
                pic->pictures[j].luma_table.bus_address, j,
                pic->pictures[j].chroma_table.virtual_address, j,
                pic->pictures[j].chroma_table.bus_address);
    }
    return;
}

void *pp_trans_demuxer_init(VpiPPParams *params)
{
    void *mwl                = NULL;
    VpiMwlInitParam mwl_para = { DWL_CLIENT_TYPE_ST_PP, params->device,
                                 params->mem_id };

    mwl_para.mem_id = params->mem_id;
    mwl_para.device = params->device;
    mwl             = pp_mwl_init(&mwl_para);
    if (mwl == NULL) {
        VPILOGE("Transcode demuxer mwl init failed!\n");
        return NULL;
    }
    return mwl;
}

static int vpi_pp_input_buf_init(VpiPPFilter *filter)
{
    int height   = filter->params.height;
    int mwl_size = ((height + 63) / 64) * (2 * 1024 * 1024);
    const void *mwl;

    mwl = filter->mwl;
    if (mwl == NULL) {
        VPILOGE("Transcode demuxer mwl error!\n");
        return -1;
    }

    filter->buffers.buffer.mem_type = DWL_MEM_TYPE_CPU_FILE_SINK;
    if (pp_mwl_malloc_linear(mwl, mwl_size, &filter->buffers.buffer)) {
        VPILOGE("No memory available for the stream buffer\n");
        return -1;
    }

    VPILOGD("stream buffer %p %p, mwl malloc buffer size %d\n",
            &filter->buffers, filter->buffers.buffer.virtual_address, mwl_size);
    VPILOGD("[%s@%d], buffer %p, size %d \n", __FUNCTION__, __LINE__,
            filter->buffers.buffer, filter->buffers.buffer.size);
    VPILOGD("[%s,%d]input_buf: bus_address=0x%llx, bus_address_rc=0x%llx, "
            "virtual_address %p, virtual_address_ep %p, size %d \n",
            __FUNCTION__, __LINE__, filter->buffers.buffer.bus_address,
            filter->buffers.buffer.bus_address_rc,
            filter->buffers.buffer.virtual_address,
            filter->buffers.buffer.virtual_address_ep,
            filter->buffers.buffer.size);

#ifdef PP_MEM_ERR_TEST
    if (pp_memory_check() != 0) {
        VPILOGE("[%s,%d]PP force memory error in function\n", __FUNCTION__,
                __LINE__);
        return -1;
    }
#endif

    /* Initialize stream to buffer start */
    filter->buffers.stream[0] = (u8 *)filter->buffers.buffer.virtual_address;
    filter->buffers.stream[1] = (u8 *)filter->buffers.buffer.virtual_address;

    return 0;
}

pp_raw_parser_inst pp_raw_parser_open(VpiPixsFmt format, int width, int height)
{
    VpiRawParser *inst;
    int i;
#ifdef CHECK_MEM_LEAK_TRANS
    inst = DWLcalloc(1, sizeof(VpiRawParser));
#else
    inst = calloc(1, sizeof(VpiRawParser));
#endif
    if (inst == NULL) {
        VPILOGE("alloc failed!\n");
        goto err_exit;
    }

    inst->img_width  = width;
    inst->img_height = height;
    inst->format     = format;

    VPILOGD("img_width = %d, img_height = %d, format = %d\n", inst->img_width,
            inst->img_height, inst->format);

    switch (inst->format) {
    case VPI_FMT_YUV420P:
        inst->planes          = 3;
        inst->byte_width[0]   = inst->img_width;
        inst->height[0]       = inst->img_height;
        inst->height_align[0] = XALIGN(inst->img_height, HEIGHT_ALIGN);
        inst->byte_width[1] = inst->byte_width[2] = (inst->img_width + 1) / 2;
        inst->height[1] = inst->height[2] = (inst->img_height + 1) / 2;
        inst->height_align[1]             = inst->height_align[2] =
            inst->height_align[0] / 2;
        break;
    case VPI_FMT_NV12:
    case VPI_FMT_NV21:
    case VPI_FMT_VPE:
        inst->planes        = 2;
        inst->byte_width[0] = inst->byte_width[1] = inst->img_width;
        inst->height[0]                           = inst->img_height;
        inst->height_align[0] = XALIGN(inst->img_height, HEIGHT_ALIGN);
        inst->height[1]       = (inst->img_height + 1) / 2;
        inst->height_align[1] = inst->height_align[0] / 2;
        break;
    case VPI_FMT_YUV420P10LE:
    case VPI_FMT_YUV420P10BE:
        inst->planes        = 3;
        inst->byte_width[0] = inst->img_width * 2;
        inst->byte_width[1] = inst->byte_width[2] =
            ((inst->img_width + 1) / 2) * 2;
        inst->height[0]       = inst->img_height;
        inst->height_align[0] = XALIGN(inst->img_height, HEIGHT_ALIGN);
        inst->height[1] = inst->height[2] = (inst->img_height + 1) / 2;
        inst->height_align[1]             = inst->height_align[2] =
            inst->height_align[0] / 2;
        break;
    case VPI_FMT_P010LE:
    case VPI_FMT_P010BE:
        inst->planes        = 2;
        inst->byte_width[0] = inst->byte_width[1] = inst->img_width * 2;
        inst->height[0]                           = inst->img_height;
        inst->height_align[0] = XALIGN(inst->img_height, HEIGHT_ALIGN);
        inst->height[1]       = (inst->img_height + 1) / 2;
        inst->height_align[1] = inst->height_align[0] / 2;
        break;
    case VPI_FMT_YUV422P:
        inst->planes        = 3;
        inst->byte_width[0] = inst->img_width;
        inst->byte_width[1] = inst->byte_width[2] = (inst->img_width + 1) / 2;
        inst->height[0] = inst->height[1] = inst->height[2] = inst->img_height;
        inst->height_align[0] = inst->height_align[1] = inst->height_align[2] =
            XALIGN(inst->img_height, HEIGHT_ALIGN);
        break;
    case VPI_FMT_YUV422P10LE:
    case VPI_FMT_YUV422P10BE:
        inst->planes        = 3;
        inst->byte_width[0] = inst->img_width * 2;
        inst->byte_width[1] = inst->byte_width[2] =
            ((inst->img_width + 1) / 2) * 2;
        inst->height[0] = inst->height[1] = inst->height[2] = inst->img_height;
        inst->height_align[0] = inst->height_align[1] = inst->height_align[2] =
            XALIGN(inst->img_height, HEIGHT_ALIGN);
        break;
    case VPI_FMT_YUV444P:
        inst->planes        = 3;
        inst->byte_width[0] = inst->byte_width[1] = inst->byte_width[2] =
            inst->img_width;
        inst->height[0] = inst->height[1] = inst->height[2] = inst->img_height;
        inst->height_align[0] = inst->height_align[1] = inst->height_align[2] =
            XALIGN(inst->img_height, HEIGHT_ALIGN);
        break;
    case VPI_FMT_RGB24:
    case VPI_FMT_BGR24:
        inst->planes          = 1;
        inst->byte_width[0]   = inst->img_width * 3;
        inst->height[0]       = inst->img_height;
        inst->height_align[0] = XALIGN(inst->img_height, HEIGHT_ALIGN);
        break;
    case VPI_FMT_ARGB:
    case VPI_FMT_RGBA:
    case VPI_FMT_ABGR:
    case VPI_FMT_BGRA:
        inst->planes          = 1;
        inst->byte_width[0]   = inst->img_width * 4;
        inst->height[0]       = inst->img_height;
        inst->height_align[0] = XALIGN(inst->img_height, HEIGHT_ALIGN);
        break;
    default:
        VPILOGE("error format!\n");
        goto err_exit;
    }

    for (i = 0; i < inst->planes; i++) {
        inst->stride[i]     = XALIGN(inst->byte_width[i], WIDTH_ALIGN);
        inst->plane_size[i] = inst->stride[i] * inst->height_align[i];
        inst->frame_size += inst->byte_width[i] * inst->height[i];
        VPILOGD("plane %d: stride = %d, height_align = %d, plane_size = %d\n",
                i, inst->stride[i], inst->height_align[i], inst->plane_size[i]);
    }

    inst->entry_num           = (inst->img_height + 63) / 64;
    inst->hugepage_frame_size = inst->entry_num * HUGE_PGAE_SIZE;
    VPILOGD("inst->entry_num = %d, inst->hugepage_frame_size = %d\n",
            inst->entry_num, inst->hugepage_frame_size);

    return (pp_raw_parser_inst)inst;

err_exit:
    if (inst) {
#ifdef CHECK_MEM_LEAK_TRANS
        DWLfree(inst);
#else
        free(inst);
#endif
        inst = NULL;
    }

    return NULL;
}

static int pp_raw_parsert_read_frame(pp_raw_parser_inst instance, u8 *buffer,
                                     u8 *stream[2], i32 *size, u8 rb,
                                     VpiFrame *frame)
{
    VpiRawParser *inst = (VpiRawParser *)instance;
    int read_len, total_len = 0;
    int i, h;
    u8 *plane_buf, *pBuf;
    u8 *src_buf = NULL;
    u32 rest_height, cur_height;
    int e;
    u8 *pEntryBuf;

    if (inst == NULL || buffer == NULL)
        return -1;

    if (*size < inst->hugepage_frame_size) {
        *size = inst->hugepage_frame_size;
        return -1;
    }
    plane_buf = buffer;
    for (i = 0; i < inst->planes; i++) {
        pEntryBuf   = plane_buf;
        rest_height = inst->height[i];

        src_buf = frame->data[i];

        for (e = 0; e < inst->entry_num; e++) {
            pBuf = pEntryBuf;
            cur_height =
                MIN(tcache_get_block_height(inst->format, i), rest_height);
            VPILOGV("entry %d plane %d, cur_height = %d, pEntryBuf = %p\n", e,
                    i, cur_height, pEntryBuf);

            for (h = 0; h < cur_height; h++) {
                read_len = inst->byte_width[i];
                memcpy(pBuf, src_buf, inst->byte_width[i]);
                src_buf += frame->linesize[i];
                if (read_len <= 0)
                    return read_len;

                total_len += read_len;
                pBuf += inst->stride[i];
            }
            pEntryBuf += HUGE_PGAE_SIZE;
            rest_height -= cur_height;
        }
        plane_buf += tcache_get_block_size(inst->stride[i], inst->format, i);
        VPILOGV("plane %d read bytes: %d\n", i, total_len);
    }

    if (total_len < inst->frame_size)
        return -1;

    return total_len;
}

static int pp_send_packet(VpiPPFilter *filter, struct DecInput **input,
                          VpiFrame *frame)
{
    i32 size, len;
    struct DecInput *buffer;

    buffer = &filter->buffers;
    size   = buffer->buffer.size;

    len = pp_raw_parsert_read_frame(filter->inst,
                                    (u8 *)buffer->buffer.virtual_address,
                                    buffer->stream, &size, 0, frame);
    if (len <= 0) {
        VPILOGD("%s(%d),RawParserReadFrame failed!\n", __FUNCTION__, __LINE__);
        return -1;
    } else {
        buffer->data_len = len;
    }
    *input = &filter->buffers;
    return 0;
}

static void pp_resolve_params_overlap(VpiPPParams *params,
                                      struct TBPpUnitParams *pp_units_params,
                                      u32 standalone)
{
    /* Override PPU1-3 parameters with tb.cfg */
    u32 i, pp_enabled;

    /* transcoder need cml have higher priority */
    if (params->pp_enabled)
        return;

    for (i = params->ppu_cfg[0].enabled ? 1 : 0; i < 4; i++) {
        params->ppu_cfg[i].enabled      = pp_units_params[i].unit_enabled;
        params->ppu_cfg[i].cr_first     = pp_units_params[i].cr_first;
        params->ppu_cfg[i].tiled_e      = pp_units_params[i].tiled_e;
        params->ppu_cfg[i].crop.enabled = pp_units_params[i].unit_enabled;
        ;
        params->ppu_cfg[i].crop.x        = pp_units_params[i].crop_x;
        params->ppu_cfg[i].crop.y        = pp_units_params[i].crop_y;
        params->ppu_cfg[i].crop.width    = pp_units_params[i].crop_width;
        params->ppu_cfg[i].crop.height   = pp_units_params[i].crop_height;
        params->ppu_cfg[i].scale.enabled = pp_units_params[i].unit_enabled;
        ;
        params->ppu_cfg[i].scale.width    = pp_units_params[i].scale_width;
        params->ppu_cfg[i].scale.height   = pp_units_params[i].scale_height;
        params->ppu_cfg[i].shaper_enabled = pp_units_params[i].shaper_enabled;
        params->ppu_cfg[i].monochrome     = pp_units_params[i].monochrome;
        params->ppu_cfg[i].planar         = pp_units_params[i].planar;
        params->ppu_cfg[i].out_p010       = pp_units_params[i].out_p010;
        params->ppu_cfg[i].out_cut_8bits  = pp_units_params[i].out_cut_8bits;
        params->ppu_cfg[i].align          = params->align;
        params->ppu_cfg[i].ystride        = pp_units_params[i].ystride;
        params->ppu_cfg[i].cstride        = pp_units_params[i].cstride;
        params->ppu_cfg[i].align          = params->align;
    }
    if (params->ppu_cfg[0].enabled) {
        /* PPU0 */
        params->ppu_cfg[0].align = params->align;
        params->ppu_cfg[0].enabled |= pp_units_params[0].unit_enabled;
        params->ppu_cfg[0].cr_first |= pp_units_params[0].cr_first;
        if (params->hw_format != DEC_OUT_FRM_RASTER_SCAN)
            params->ppu_cfg[0].tiled_e |= pp_units_params[0].tiled_e;
        params->ppu_cfg[0].planar |= pp_units_params[0].planar;
        params->ppu_cfg[0].out_p010 |= pp_units_params[0].out_p010;
        params->ppu_cfg[0].out_cut_8bits |= pp_units_params[0].out_cut_8bits;
        if (!params->ppu_cfg[0].crop.enabled &&
            pp_units_params[0].unit_enabled) {
            params->ppu_cfg[0].crop.x      = pp_units_params[0].crop_x;
            params->ppu_cfg[0].crop.y      = pp_units_params[0].crop_y;
            params->ppu_cfg[0].crop.width  = pp_units_params[0].crop_width;
            params->ppu_cfg[0].crop.height = pp_units_params[0].crop_height;
        }
        if (params->ppu_cfg[0].crop.width || params->ppu_cfg[0].crop.height)
            params->ppu_cfg[0].crop.enabled = 1;
        if (!params->ppu_cfg[0].scale.enabled &&
            pp_units_params[0].unit_enabled) {
            params->ppu_cfg[0].scale.width  = pp_units_params[0].scale_width;
            params->ppu_cfg[0].scale.height = pp_units_params[0].scale_height;
        }
        if (params->ppu_cfg[0].scale.width || params->ppu_cfg[0].scale.height)
            params->ppu_cfg[0].scale.enabled = 1;
        params->ppu_cfg[0].shaper_enabled = pp_units_params[0].shaper_enabled;
        params->ppu_cfg[0].monochrome     = pp_units_params[0].monochrome;
        params->ppu_cfg[0].align          = params->align;
        if (!params->ppu_cfg[0].ystride)
            params->ppu_cfg[0].ystride = pp_units_params[0].ystride;
        if (!params->ppu_cfg[0].cstride)
            params->ppu_cfg[0].cstride = pp_units_params[0].cstride;
    }
    pp_enabled =
        pp_units_params[0].unit_enabled || pp_units_params[1].unit_enabled ||
        pp_units_params[2].unit_enabled || pp_units_params[3].unit_enabled;
    if (pp_enabled)
        params->pp_enabled = 1;
    if (standalone) { /* pp standalone mode */
        params->pp_standalone = 1;
        if (!params->pp_enabled && !params->fscale_cfg.fixed_scale_enabled) {
            /* No pp enabled explicitly, then enable fixed ratio pp (1:1) */
            params->fscale_cfg.down_scale_x        = 1;
            params->fscale_cfg.down_scale_y        = 1;
            params->fscale_cfg.fixed_scale_enabled = 1;
            params->pp_enabled                     = 1;
        }
    }
}

static int vpi_ppclient_release(PPClient *pp_client)
{
    if (pp_client != NULL) {
        pp_buf_release(pp_client);
        PPRelease(pp_client->pp_inst);
        if (pp_client->dwl != NULL)
            DWLRelease(pp_client->dwl);

#ifdef CHECK_MEM_LEAK_TRANS
        DWLfree(pp_client);
#else
        free(pp_client);
#endif
        pp_client = NULL;
    }
    return 0;
}

static PPClient *pp_client_init(VpiPPFilter *filter)
{
    VpiPPParams *params   = &filter->params;
    PPClient *pp_client   = NULL;
    PPInst pp_inst        = NULL;
    VpiPPConfig *test_cfg = NULL;
    VpiFrame *in_frame    = filter->frame;
    int ret;
    u32 i;
    PPConfig *dec_cfg = NULL;
    struct DWLInitParam dwl_init;
    u32 alignh = 2;
    u32 alignw = 2;

#ifdef CHECK_MEM_LEAK_TRANS
    pp_client = (PPClient *)DWLcalloc(1, sizeof(PPClient));
#else
    pp_client = (PPClient *)calloc(1, sizeof(PPClient));
#endif
    if (pp_client == NULL) {
        VPILOGE("pp client malloc failed!!!\n");
        goto err_exit;
    }

#ifdef PP_MEM_ERR_TEST
    if (pp_memory_check() != 0) {
        VPILOGE("[%s,%d]PP force memory error in function\n", __FUNCTION__,
                __LINE__);
        goto err_exit;
    }
#endif

    pp_client->param = *params;

    /* set test bench configuration */
    vpi_tb_set_default_cfg(&tb_cfg);

    if (tb_cfg.pp_params.pre_fetch_height == 16)
        dec_pp_in_blk_size = 0;
    else if (tb_cfg.pp_params.pre_fetch_height == 64)
        dec_pp_in_blk_size = 1;

    /* process pp size -1/-2/-4/-8. */
    for (i = 1; i < 4; i++) {
        if (pp_client->param.ppu_cfg[i].scale.width == -1 ||
            pp_client->param.ppu_cfg[i].scale.width == -2 ||
            pp_client->param.ppu_cfg[i].scale.width == -4 ||
            pp_client->param.ppu_cfg[i].scale.width == -8 ||
            pp_client->param.ppu_cfg[i].scale.height == -1 ||
            pp_client->param.ppu_cfg[i].scale.height == -2 ||
            pp_client->param.ppu_cfg[i].scale.height == -4 ||
            pp_client->param.ppu_cfg[i].scale.height == -8) {
            u32 original_width  = pp_client->param.width;
            u32 original_height = pp_client->param.height;
            if (pp_client->param.ppu_cfg[i].scale.width == -1 &&
                pp_client->param.ppu_cfg[i].scale.height == -1) {
                VPILOGE("pp %d scale setting error!!!\n", i);
                goto err_exit;
            }
            if (pp_client->param.ppu_cfg[i].crop.enabled) {
                if (pp_client->param.ppu_cfg[i].crop.width != original_width) {
                    original_width = pp_client->param.ppu_cfg[i].crop.width;
                }
                if (pp_client->param.ppu_cfg[i].crop.height !=
                    original_height) {
                    original_height = pp_client->param.ppu_cfg[i].crop.height;
                }
            }
            VPILOGD("original_width = %d, original_height = %d\n",
                    original_width, original_height);
            if (pp_client->param.ppu_cfg[i].scale.width == -1) {
                pp_client->param.ppu_cfg[i].scale.width =
                    NEXT_MULTIPLE((original_width *
                                   pp_client->param.ppu_cfg[i].scale.height) /
                                      original_height,
                                  alignw);
                pp_client->param.ppu_cfg[i].scale.height =
                    NEXT_MULTIPLE(pp_client->param.ppu_cfg[i].scale.height,
                                  alignh);
            } else if (pp_client->param.ppu_cfg[i].scale.height == -1) {
                pp_client->param.ppu_cfg[i].scale.width =
                    NEXT_MULTIPLE(pp_client->param.ppu_cfg[i].scale.width,
                                  alignw);
                pp_client->param.ppu_cfg[i].scale.height =
                    NEXT_MULTIPLE((original_height *
                                   pp_client->param.ppu_cfg[i].scale.width) /
                                      original_width,
                                  alignh);
            } else if (pp_client->param.ppu_cfg[i].scale.width == -2 &&
                       pp_client->param.ppu_cfg[i].scale.height == -2) {
                pp_client->param.ppu_cfg[i].scale.width =
                    NEXT_MULTIPLE(original_width / 2, alignw);
                pp_client->param.ppu_cfg[i].scale.height =
                    NEXT_MULTIPLE(original_height / 2, alignh);
            } else if (pp_client->param.ppu_cfg[i].scale.width == -4 &&
                       pp_client->param.ppu_cfg[i].scale.height == -4) {
                pp_client->param.ppu_cfg[i].scale.width =
                    NEXT_MULTIPLE(original_width / 4, alignw);
                pp_client->param.ppu_cfg[i].scale.height =
                    NEXT_MULTIPLE(original_height / 4, alignh);
            } else if (pp_client->param.ppu_cfg[i].scale.width == -8 &&
                       pp_client->param.ppu_cfg[i].scale.height == -8) {
                pp_client->param.ppu_cfg[i].scale.width =
                    NEXT_MULTIPLE(original_width / 8, alignw);
                pp_client->param.ppu_cfg[i].scale.height =
                    NEXT_MULTIPLE(original_height / 8, alignh);
            } else {
                VPILOGE("pp %d scale setting error!!!\n", i);
                goto err_exit;
            }
            VPILOGD("pp_client->param.ppu_cfg[%d].scale.width = %d, "
                    "pp_client->param.ppu_cfg[%d].scale.height = %d\n",
                    i, pp_client->param.ppu_cfg[i].scale.width, i,
                    pp_client->param.ppu_cfg[i].scale.height);
        }
    }

    ret      = pp_parse_params(pp_client, &pp_client->param);
    test_cfg = &pp_client->pp_config;
    if (ret < 0) {
        VPILOGE("pp_parse_params failed!\n");
        goto err_exit;
    }

    dwl_init.client_type  = DWL_CLIENT_TYPE_ST_PP;
    dwl_init.dec_dev_name = params->device;
    dwl_init.mem_id       = params->mem_id;
    dwl_init.priority     = params->priority;
    pp_client->dwl        = (void *)DWLInit(&dwl_init);
    if (pp_client->dwl == NULL) {
        VPILOGE("pp DWLInit failed!\n");
        goto err_exit;
    }

    /* initialize decoder. If unsuccessful -> exit */
    ret = PPInit((PPInst *)&pp_client->pp_inst, (const void *)pp_client->dwl);
    if (ret != PP_OK) {
        VPILOGE("PP init failed\n");
        goto err_exit;
    }
    pp_inst = pp_client->pp_inst;

    if (pp_calc_pic_size(pp_client) < 0) {
        VPILOGE("pp_calc_pic_size failed!\n");
        goto err_exit;
    }

    VPILOGD("ALIGN TCACHE size in_width_align=%d, in_stride=%d\n",
            pp_client->pp_config.in_width_align,
            pp_client->pp_config.in_stride);

    if (filter->b_disable_tcache) {
        pp_client->pp_config.in_stride = in_frame->pic_info[0].pic_width;
    }
    if(in_frame->raw_format == VPI_FMT_NV12)
        in_frame->pic_info[0].format = VPI_YUV420_SEMIPLANAR;
    else if(in_frame->raw_format == VPI_FMT_YUV420P)
        in_frame->pic_info[0].format = VPI_YUV420_SEMIPLANAR_YUV420P;
    else
        in_frame->pic_info[0].format = VPI_YUV420_SEMIPLANAR_VU;

    /* Override command line options and tb.cfg for PPU0. */
    /* write pp params form tb_cfg.pp_units_params to pp_client->param.ppu_cfg
       or keep cml params - kwu */
    pp_resolve_params_overlap(&pp_client->param, tb_cfg.pp_units_params,
                              !tb_cfg.pp_params.pipeline_e);

#ifdef SUPPORT_CACHE
    /* resolve shaper setting */
    for (i = 0; i < 4; i++) {
        if (!pp_client->param.ppu_cfg[i].enabled)
            continue;
        pp_client->param.ppu_cfg[i].shaper_enabled = test_cfg->shaper_enable;
    }
#endif

    if (pp_client->param.fscale_cfg.fixed_scale_enabled) {
        u32 crop_w = pp_client->param.ppu_cfg[0].crop.width;
        u32 crop_h = pp_client->param.ppu_cfg[0].crop.height;
        if (!crop_w)
            crop_w = test_cfg->in_width;
        if (!crop_h)
            crop_h = test_cfg->in_height;
        pp_client->param.ppu_cfg[0].scale.width =
            (crop_w / pp_client->param.fscale_cfg.down_scale_x) & ~0x1;
        pp_client->param.ppu_cfg[0].scale.height =
            (crop_h / pp_client->param.fscale_cfg.down_scale_y) & ~0x1;
        pp_client->param.ppu_cfg[0].scale.enabled = 1;
        pp_client->param.ppu_cfg[0].enabled       = 1;
    }
    pp_client->pp_enabled = pp_client->param.pp_enabled;

    /* resolve pp align and set crop - kwu add */
    if (test_cfg->in_width_align != test_cfg->in_width ||
        test_cfg->in_height_align != test_cfg->in_height) {
        for (i = 1; i < 4; i++) {
            if (!pp_client->param.ppu_cfg[i].enabled)
                continue;
            if (!pp_client->param.ppu_cfg[i].crop.enabled) {
                pp_client->param.ppu_cfg[i].crop.enabled = 1;
                pp_client->param.ppu_cfg[i].crop.x       = 0;
                pp_client->param.ppu_cfg[i].crop.y       = 0;
                pp_client->param.ppu_cfg[i].crop.width   = test_cfg->in_width;
                pp_client->param.ppu_cfg[i].crop.height  = test_cfg->in_height;
            }
            VPILOGD("[pp] enable pp[%d] crop [%d, %d, %d, %d]\n", i,
                    pp_client->param.ppu_cfg[i].crop.x,
                    pp_client->param.ppu_cfg[i].crop.y,
                    pp_client->param.ppu_cfg[i].crop.width,
                    pp_client->param.ppu_cfg[i].crop.height);
        }
    }
    /* write pp config to dec_cfg->ppu_config - kwu */
    dec_cfg = &pp_client->dec_cfg;
    memcpy(dec_cfg->ppu_config, pp_client->param.ppu_cfg,
           sizeof(pp_client->param.ppu_cfg));

    for (i = 0; i < 4; i++) {
        if (dec_cfg->ppu_config[i].tiled_e && pp_client->param.in_10bit)
            dec_cfg->ppu_config[i].out_cut_8bits = 0;
        if (pp_client->param.in_10bit == 0) {
            dec_cfg->ppu_config[i].out_cut_8bits = 0;
            dec_cfg->ppu_config[i].out_p010      = 0;
        }

        VPILOGD("dec_cfg->ppu_config[%d].crop.width %d, "
                "dec_cfg->ppu_config[%d].crop.height %d \n",
                i, dec_cfg->ppu_config[i].crop.width, i,
                dec_cfg->ppu_config[i].crop.height);
        if (!dec_cfg->ppu_config[i].crop.width ||
            !dec_cfg->ppu_config[i].crop.height) {
            dec_cfg->ppu_config[i].crop.x = dec_cfg->ppu_config[i].crop.y = 0;
            dec_cfg->ppu_config[i].crop.width  = test_cfg->in_width_align;
            dec_cfg->ppu_config[i].crop.height = test_cfg->in_height_align;

            VPILOGD("dec_cfg->ppu_config[%d].crop.width %d, "
                    "dec_cfg->ppu_config[%d].crop.height %d \n",
                    i, dec_cfg->ppu_config[i].crop.width, i,
                    dec_cfg->ppu_config[i].crop.height);
        }

        VPILOGD("dec_cfg->ppu_config[%d].scale.width %d, "
                "dec_cfg->ppu_config[%d].scale.height %d \n",
                i, dec_cfg->ppu_config[i].scale.width, i,
                dec_cfg->ppu_config[i].scale.height);
        if (!dec_cfg->ppu_config[i].scale.width ||
            !dec_cfg->ppu_config[i].scale.height) {
            dec_cfg->ppu_config[i].scale.width =
                dec_cfg->ppu_config[i].crop.width;
            dec_cfg->ppu_config[i].scale.height =
                dec_cfg->ppu_config[i].crop.height;

            VPILOGD("dec_cfg->ppu_config[%d].scale.width %d, "
                    "dec_cfg->ppu_config[%d].scale.height %d \n",
                    i, dec_cfg->ppu_config[i].scale.width, i,
                    dec_cfg->ppu_config[i].scale.height);
        }
    }

    dec_cfg->in_format = test_cfg->in_p010;
    dec_cfg->in_stride = test_cfg->in_stride;
    dec_cfg->in_height = test_cfg->in_height_align;
    dec_cfg->in_width  = test_cfg->in_width_align;

    for (i = 0; i < 4; i++) {
        if (!dec_cfg->ppu_config[i].enabled)
            continue;
    }

    pp_client->disable_tcache   = filter->b_disable_tcache;
    pp_client->max_frames_delay = filter->frame->max_frames_delay;
    ret = pp_buf_init(pp_client);
    if (ret < 0) {
        VPILOGE("pp_buf_init failed!\n");
        goto err_exit;
    }
    pp_dump_config(pp_client);
    ret = PPSetInfo(pp_inst, dec_cfg);
    if (ret != PP_OK) {
        VPILOGE("encode fails for Invalid pp parameters\n");
        goto err_exit;
    }
    return pp_client;
err_exit:
    vpi_ppclient_release(pp_client);
    return NULL;
}

static void pp_report_pp_pic_info(struct DecPicturePpu *picture)
{
    char info_string[2048];
    static char *pic_types[] = { "IDR", "Non-IDR (P)", "Non-IDR (B)" };

    sprintf(&info_string[0], "pic_id %2d, type %s, ",
            picture->pictures[0].picture_info.pic_id,
            pic_types[picture->pictures[0].picture_info.pic_coding_type]);
    if (picture->pictures[0].picture_info.cycles_per_mb) {
        sprintf(&info_string[strlen(info_string)], " %4d cycles / mb,",
                picture->pictures[0].picture_info.cycles_per_mb);
    }

    sprintf(&info_string[strlen(info_string)],
            " %d x %d, Crop: (%d, %d), %d x %d %s",
            picture->pictures[0].sequence_info.pic_width,
            picture->pictures[0].sequence_info.pic_height,
            picture->pictures[0].sequence_info.crop_params.crop_left_offset,
            picture->pictures[0].sequence_info.crop_params.crop_top_offset,
            picture->pictures[0].sequence_info.crop_params.crop_out_width,
            picture->pictures[0].sequence_info.crop_params.crop_out_height,
            picture->pictures[0].picture_info.is_corrupted ? "CORRUPT" : "");

    VPILOGD("%s\n", info_string);
}

static int pp_output_frame(VpiPPFilter *filter, VpiFrame *out,
                           PPDecPicture *hipc)
{
    VpiPPParams *params       = &filter->pp_client->param;
    struct DecPicturePpu *pic = malloc(sizeof(struct DecPicturePpu));

    if (!pic)
        return -1;

#ifdef PP_MEM_ERR_TEST
    if (pp_memory_check() != 0) {
        VPILOGE("PP force memory error in function\n");
        goto err_exit;
    }
#endif

    memset(pic, 0, sizeof(struct DecPicturePpu));
    memcpy(pic, &hipc->pp_pic, sizeof(struct DecPicturePpu));
    VPILOGD("[%s@%d]DecPicturePpu pic %p,  PPDecPicture hipc->pp_pic %p\n",
            __FUNCTION__, __LINE__, pic, &hipc->pp_pic);
    pp_report_pp_pic_info(pic);

    memset(out, 0, sizeof(VpiFrame));
    if (pic->pictures[0].pic_width > params->width) {
        out->width = params->width;
    } else {
        out->width = pic->pictures[0].pic_width;
    }
    if (pic->pictures[0].pic_height > params->height) {
        out->height = params->height;
    } else {
        out->height = pic->pictures[0].pic_height;
    }
    out->linesize[0] = pic->pictures[0].pic_width;
    out->linesize[1] = pic->pictures[0].pic_width / 2;
    out->linesize[2] = pic->pictures[0].pic_width / 2;
    out->key_frame =
        (pic->pictures[0].picture_info.pic_coding_type == DEC_PIC_TYPE_I);

    VPILOGD("out->width = %d, out->height = %d, out->linesize[0] = %d, "
            "out->key_frame = %d\n",
            out->width, out->height, out->linesize[0], out->key_frame);

    /*always using pp0, never pp1*/
    pic->pictures[0].pp_enabled = 1;
    pic->pictures[1].pp_enabled = 0;

    for (int i = 2; i < 5; i++) {
        if (params->ppu_cfg[i - 1].enabled == 1) {
            pic->pictures[i].pp_enabled = 1;
        } else {
            pic->pictures[i].pp_enabled = 0;
        }
    }

    for (int i = 0; i < 5; i++)
        VPILOGD("pic.pictures[%d].pp_enabled = "
                "%d,comperss_status=%d,bit_depth_luma=%d\n",
                i, pic->pictures[i].pp_enabled,
                pic->pictures[i].pic_compressed_status,
                pic->pictures[i].sequence_info.bit_depth_luma);

    VPILOGD("fb_dec_ctx->picRdy = 1\n");

    out->pic_info[0].enabled = 1;
    if (params->ppu_cfg[0].scale.width != 0 &&
        params->ppu_cfg[0].scale.height != 0) {
        out->pic_info[0].width  = params->ppu_cfg[0].scale.width;
        out->pic_info[0].height = params->ppu_cfg[0].scale.height;
    } else {
        out->pic_info[0].width  = out->width;
        out->pic_info[0].height = out->height;
    }

    VPILOGD("out->pic_info[0].width %d, out->pic_info[0].height %d\n",
            out->pic_info[0].width, out->pic_info[0].height);

    out->pic_info[0].picdata.is_interlaced =
        pic->pictures[0].sequence_info.is_interlaced;
    out->pic_info[0].picdata.pic_stride = pic->pictures[0].pic_stride;

    out->pic_info[0].picdata.pic_format = pic->pictures[0].picture_info.format;
    out->pic_info[0].picdata.pic_pixformat =
        pic->pictures[0].picture_info.pixel_format;
    out->pic_info[0].picdata.bit_depth_luma =
        pic->pictures[0].sequence_info.bit_depth_luma;
    out->pic_info[0].picdata.bit_depth_chroma =
        pic->pictures[0].sequence_info.bit_depth_chroma;
    out->pic_info[0].picdata.pic_compressed_status =
        pic->pictures[0].pic_compressed_status;

    if (pic->pictures[1].pp_enabled == 1) {
        out->pic_info[1].enabled = 1;
        out->pic_info[1].width   = pic->pictures[0].pic_width;
        out->pic_info[1].height  = pic->pictures[0].pic_height;
    } else {
        out->pic_info[1].enabled = 0;
        out->pic_info[1].width   = pic->pictures[0].pic_width;
        out->pic_info[1].height  = pic->pictures[0].pic_height;
    }

    for (int i = 2; i < 5; i++) {
        if (params->ppu_cfg[i - 1].enabled == 1) {
            out->pic_info[i].enabled = 1;
            out->pic_info[i].width   = params->ppu_cfg[i - 1].scale.width;
            out->pic_info[i].height  = params->ppu_cfg[i - 1].scale.height;
            out->pic_info[i].picdata.is_interlaced =
                pic->pictures[i].sequence_info.is_interlaced;
            out->pic_info[i].picdata.pic_stride = pic->pictures[i].pic_stride;
            out->pic_info[i].picdata.crop_out_width =
                pic->pictures[i].sequence_info.crop_params.crop_out_width;
            out->pic_info[i].picdata.crop_out_height =
                pic->pictures[i].sequence_info.crop_params.crop_out_height;
            out->pic_info[i].picdata.pic_format =
                pic->pictures[i].picture_info.format;
            out->pic_info[i].picdata.pic_pixformat =
                pic->pictures[i].picture_info.pixel_format;
            out->pic_info[i].picdata.bit_depth_luma =
                pic->pictures[i].sequence_info.bit_depth_luma;
            out->pic_info[i].picdata.bit_depth_chroma =
                pic->pictures[i].sequence_info.bit_depth_chroma;
            out->pic_info[i].picdata.pic_compressed_status =
                pic->pictures[i].pic_compressed_status;
        }
    }
    VPILOGD("out->pic_info[0].picdata.pic_compressed_status = %d.\n",
            out->pic_info[0].picdata.pic_compressed_status);

    VPILOGD("output "
            "frames:[%d(%dx%d)][%d(%dx%d)][%d(%dx%d)][%d(%dx%d)][%d(%dx%d)]\n",
            out->pic_info[0].enabled, out->pic_info[0].width,
            out->pic_info[0].height, out->pic_info[1].enabled,
            out->pic_info[1].width, out->pic_info[1].height,
            out->pic_info[2].enabled, out->pic_info[2].width,
            out->pic_info[2].height, out->pic_info[3].enabled,
            out->pic_info[3].width, out->pic_info[3].height,
            out->pic_info[4].enabled, out->pic_info[4].width,
            out->pic_info[4].height);

    if (filter->vce_ds_enable) {
        if (!out->pic_info[0].enabled || !out->pic_info[2].enabled) {
            VPILOGE("When use 1/4 ds pass1 for vce, pp0 and pp1 should be "
                    "enabled!\n");
            goto err_exit;
        }
        if (out->pic_info[1].enabled || out->pic_info[3].enabled ||
            out->pic_info[4].enabled) {
            VPILOGE("When use 1/4 ds pass1 for vce, except pp0 and pp1 should "
                    "not be enabled!\n");
            goto err_exit;
        }
        VPILOGD("set flag to 1\n");
        out->pic_info[2].flag = 1;
    }

    out->data[0]         = (uint8_t *)pic;
    out->pic_struct_size = sizeof(struct DecPicturePpu);

#ifdef PP_MEM_ERR_TEST
    if (pp_memory_check() != 0) {
        VPILOGE("PP force memory error in function\n");
        goto err_exit;
    }
#endif
    return 0;

err_exit:
    return -1;
}

static int pp_trans_formats(VpiPixsFmt format, char **pp_format, int *in_10bit,
                            int force_10bit)
{
    if (format == VPI_FMT_YUV420P) {
        *pp_format = "yuv420p";
        *in_10bit  = 0;
    } else if (format == VPI_FMT_YUV422P) {
        *pp_format = "yuv422p";
        *in_10bit  = 0;
    } else if (format == VPI_FMT_NV12) {
        *pp_format = "nv12";
        *in_10bit  = 0;
    } else if (format == VPI_FMT_NV21) {
        *pp_format = "nv21";
        *in_10bit  = 0;
    } else if (format == VPI_FMT_YUV420P10LE) {
        *pp_format = "yuv420p10le";
        *in_10bit  = 1;
    } else if (format == VPI_FMT_YUV420P10BE) {
        *pp_format = "yuv420p10be";
        *in_10bit  = 1;
    } else if (format == VPI_FMT_YUV422P10LE) {
        *pp_format = "yuv422p10le";
        *in_10bit  = 1;
    } else if (format == VPI_FMT_YUV422P10BE) {
        *pp_format = "yuv422p10be";
        *in_10bit  = 1;
    } else if (format == VPI_FMT_P010LE) {
        *pp_format = "p010le";
        *in_10bit  = 1;
    } else if (format == VPI_FMT_P010BE) {
        *pp_format = "p010be";
        *in_10bit  = 1;
    } else if (format == VPI_FMT_YUV444P) {
        *pp_format = "yuv444p";
        *in_10bit  = 0;
    } else if (format == VPI_FMT_RGB24) {
        *pp_format = "rgb24";
        *in_10bit  = 0;
        if (force_10bit)
            *in_10bit = 1;
    } else if (format == VPI_FMT_BGR24) {
        *pp_format = "bgr24";
        *in_10bit  = 0;
        if (force_10bit)
            *in_10bit = 1;
    } else if (format == VPI_FMT_ARGB) {
        *pp_format = "argb";
        *in_10bit  = 0;
        if (force_10bit)
            *in_10bit = 1;
    } else if (format == VPI_FMT_RGBA) {
        *pp_format = "rgba";
        *in_10bit  = 0;
        if (force_10bit)
            *in_10bit = 1;
    } else if (format == VPI_FMT_ABGR) {
        *pp_format = "abgr";
        *in_10bit  = 0;
        if (force_10bit)
            *in_10bit = 1;
    } else if (format == VPI_FMT_BGRA) {
        *pp_format = "bgra";
        *in_10bit  = 0;
        if (force_10bit)
            *in_10bit = 1;
    } else if (format == VPI_FMT_VPE) {
        *pp_format = "vpe";
        if (force_10bit)
            *in_10bit = 1;
    } else {
        VPILOGE("Transcoder unsupport input format %d!!!\n", format);
        return -1;
    }
    return 0;
}

static void pp_setup_defaul_params(VpiPPParams *params)
{
    params->read_mode               = STREAMREADMODE_FRAME;
    params->fscale_cfg.down_scale_x = 1;
    params->fscale_cfg.down_scale_y = 1;
    params->is_ringbuffer           = 1;
    params->display_cropped         = 0;
    params->tile_by_tile            = 0;
    params->mc_enable               = 0;
    memset(params->ppu_cfg, 0, sizeof(params->ppu_cfg));
}

static int pp_set_params(VpiPPFilter *filter)
{
    int ret             = 0;
    int pp_index        = 0;
    VpiPPParams *params = &filter->params;
    VpiFrame *frame     = filter->frame;

    /* check low_res and output numbers */
    if (filter->nb_outputs < 1 || filter->nb_outputs > 4) {
        VPILOGE("outputs number error\n");
        return -1;
    }

    if (filter->nb_outputs == 1 && filter->low_res_num == 1 &&
        (filter->resizes[0].x == 0 && filter->resizes[0].y == 0 &&
         filter->resizes[0].cw == 0 && filter->resizes[0].ch == 0 &&
         filter->resizes[0].sw == -2 && filter->resizes[0].sh == -2)) {
        VPILOGD("set vce_ds_enable to 1\n");

        filter->vce_ds_enable = 1;
        filter->resizes[1]    = filter->resizes[0];
        filter->resizes[0].x = filter->resizes[0].y = filter->resizes[0].cw =
            filter->resizes[0].ch                   = filter->resizes[0].sw =
                filter->resizes[0].sh               = 0;
        filter->low_res_num++;

    } else if (filter->nb_outputs == 1 && filter->low_res_num == 2) {
        if (!(filter->resizes[0].x == filter->resizes[1].x &&
              filter->resizes[0].y == filter->resizes[1].y &&
              filter->resizes[0].cw == filter->resizes[1].cw &&
              filter->resizes[0].ch == filter->resizes[1].ch &&
              filter->resizes[1].sw == -2 && filter->resizes[1].sh == -2)) {
            VPILOGE("low_res param error!\n");
            return -1;
        }

        filter->vce_ds_enable = 1;

    } else if (filter->nb_outputs == filter->low_res_num + 1) {
        if (filter->low_res_num) {
            for (int i = filter->nb_outputs - 1; i > 0; i--) {
                filter->resizes[i] = filter->resizes[i - 1];
            }
        }

        filter->resizes[0].x = filter->resizes[0].y = filter->resizes[0].cw =
            filter->resizes[0].ch                   = filter->resizes[0].sw =
                filter->resizes[0].sh               = 0;

        filter->low_res_num++;

    } else if (filter->nb_outputs != filter->low_res_num) {
        VPILOGE("low_res param error!\n");
        return -1;
    }

    VPILOGD("pp decoder resizes info summay:\n");
    for (int i = 0; i < filter->low_res_num; i++) {
        VPILOGD("low_res %d : (%d,%d, %d,%d, %dx%d)\n", i, filter->resizes[i].x,
                filter->resizes[i].y, filter->resizes[i].cw,
                filter->resizes[i].ch, filter->resizes[i].sw,
                filter->resizes[i].sh);
    }

    pp_setup_defaul_params(params);
    params->width  = filter->w;
    params->height = filter->h;
    ret            = pp_trans_formats(filter->format, &params->in_format,
                           &params->in_10bit, filter->force_10bit);
    if (ret < 0) {
        VPILOGE("pp_trans_formats failed ret=%d\n", ret);
        return -1;
    }

    /* upload link with pp, the frame format is vpe_format,
       it needs to decide 10bit or 8bit */
    if(frame->flag & PP_FLAG){
        uint32_t in_10bit_hantro_fmt = 0;

        if( frame->pic_info[0].picdata.pic_pixformat == 1)
            in_10bit_hantro_fmt = 1;

        if(!strcmp(params->in_format, "vpe") && in_10bit_hantro_fmt == 1)
            params->in_10bit = 1;
    }

    params->align           = DEC_ALIGN_1024B;
    params->compress_bypass = 0;
    params->cache_enable    = 1;
    params->shaper_enable   = 1;

    /* 17 buffers as default value, same to decoder and the buffer_depth is inc
     * value.*/
        // params->ext_buffers_need = 17 + filter->buffer_depth;

        /* set options to pp params */
    params->pp_enabled                = 1;
    params->ppu_cfg[0].enabled        = 1;
    params->ppu_cfg[0].tiled_e        = 1;
    params->ppu_cfg[0].out_p010       = params->in_10bit;
    params->ppu_cfg[0].align          = 10;
    params->ppu_cfg[0].shaper_enabled = 1;

    VPILOGD("[%s, %d], pp test Params summary,width %d, height %d, in_format "
            "%s, in_10bit %d, format %d\n",
            __FUNCTION__, __LINE__, params->width, params->height,
            params->in_format, params->in_10bit, filter->format);

    for (int i = 0; i < filter->low_res_num; i++) {
        if (filter->resizes[i].x == 0 && filter->resizes[i].y == 0 &&
            filter->resizes[i].cw == 0 && filter->resizes[i].ch == 0 &&
            filter->resizes[i].sw == 0 && filter->resizes[i].sh == 0) {
            continue;
        }
        VPILOGD("pp %d low_res %d get param\n", i, i);
        pp_index                          = i;
        params->ppu_cfg[pp_index].enabled = 1;
        params->ppu_cfg[pp_index].tiled_e = 1;

        /* crop params */
        if (!(filter->resizes[i].x == 0 && filter->resizes[i].y == 0 &&
              filter->resizes[i].cw == 0 && filter->resizes[i].ch == 0)) {
            if (!(params->width && params->height &&
                  filter->resizes[i].x == 0 && filter->resizes[i].y == 0 &&
                  filter->resizes[i].cw == params->width &&
                  filter->resizes[i].ch == params->height)) {
                params->ppu_cfg[pp_index].crop.enabled = 1;
                params->ppu_cfg[pp_index].crop.x       = filter->resizes[i].x;
                params->ppu_cfg[pp_index].crop.y       = filter->resizes[i].y;
                params->ppu_cfg[pp_index].crop.width   = filter->resizes[i].cw;
                params->ppu_cfg[pp_index].crop.height  = filter->resizes[i].ch;
            }
        }
        /* scale params*/
        if (!(filter->resizes[i].sw == 0 && filter->resizes[i].sh == 0)) {
            params->ppu_cfg[pp_index].scale.enabled = 1;
            params->ppu_cfg[pp_index].scale.width   = filter->resizes[i].sw;
            params->ppu_cfg[pp_index].scale.height  = filter->resizes[i].sh;
        }
        params->ppu_cfg[pp_index].out_p010       = params->in_10bit;
        params->ppu_cfg[pp_index].align          = 10;
        params->ppu_cfg[pp_index].shaper_enabled = 1;

        VPILOGD("ppu_cfg[%d]:enabled %d, tiled_e %d, out_p010 %d, align %d, "
                "shaper_enabled %d, crop(enabled %d, %d %d, %dx%d), "
                "scale(enabled %d, %dx%d)\n",
                pp_index, params->ppu_cfg[pp_index].enabled,
                params->ppu_cfg[pp_index].tiled_e,
                params->ppu_cfg[pp_index].out_p010,
                params->ppu_cfg[pp_index].align,
                params->ppu_cfg[pp_index].shaper_enabled,
                params->ppu_cfg[pp_index].crop.enabled,
                params->ppu_cfg[pp_index].crop.x,
                params->ppu_cfg[pp_index].crop.y,
                params->ppu_cfg[pp_index].crop.width,
                params->ppu_cfg[pp_index].crop.height,
                params->ppu_cfg[pp_index].scale.enabled,
                params->ppu_cfg[pp_index].scale.width,
                params->ppu_cfg[pp_index].scale.height);
    }
    return 0;
}

static int pp_set_hwframe_res(VpiPPFilter *filter)
{
    VpiFrame *frame       = filter->frame;
    PpUnitConfig *ppu_cfg = filter->pp_client->dec_cfg.ppu_config;
    VpiPPParams *params   = &filter->params;

    frame->pic_info[0].enabled = 1;
    if (ppu_cfg[0].scale.width < params->width) {
        frame->pic_info[0].width = ppu_cfg[0].scale.width;
    } else {
        frame->pic_info[0].width = params->width;
    }
    if (ppu_cfg[0].scale.height < params->height) {
        frame->pic_info[0].height = ppu_cfg[0].scale.height;
    } else {
        frame->pic_info[0].height = params->height;
    }

    frame->pic_info[0].pic_width             = ppu_cfg[0].scale.width;
    frame->pic_info[0].pic_height            = ppu_cfg[0].scale.height;
    frame->pic_info[0].picdata.is_interlaced = 0;
    frame->pic_info[0].picdata.pic_format    = 0;
    if (params->in_10bit) {
        frame->pic_info[0].picdata.pic_pixformat = 1;
        frame->pic_info[0].picdata.bit_depth_luma =
            frame->pic_info[0].picdata.bit_depth_chroma = 10;
    } else {
        frame->pic_info[0].picdata.pic_pixformat = 0;
        frame->pic_info[0].picdata.bit_depth_luma =
            frame->pic_info[0].picdata.bit_depth_chroma = 8;
    }
    frame->pic_info[0].picdata.pic_compressed_status = 2;
    frame->pic_info[1].enabled                       = 0;
    for (int i = 2; i < 5; i++) {
        if (params->ppu_cfg[i - 1].enabled == 1) {
            frame->pic_info[i].enabled = 1;
            frame->pic_info[i].width   = ppu_cfg[i - 1].scale.width;
            frame->pic_info[i].height  = ppu_cfg[i - 1].scale.height;

            frame->pic_info[i].picdata.is_interlaced = 0;
            frame->pic_info[i].picdata.pic_format    = 0;
            if (params->in_10bit) {
                frame->pic_info[i].picdata.pic_pixformat = 1;
                frame->pic_info[i].picdata.bit_depth_luma =
                    frame->pic_info[i].picdata.bit_depth_chroma = 10;
            } else {
                frame->pic_info[i].picdata.pic_pixformat = 0;
                frame->pic_info[i].picdata.bit_depth_luma =
                    frame->pic_info[i].picdata.bit_depth_chroma = 8;
            }

            frame->pic_info[i].picdata.pic_compressed_status = 2;
        }
    }

    VPILOGD("output "
            "frames:[%d(%dx%d)][%d(%dx%d)][%d(%dx%d)][%d(%dx%d)][%d(%dx%d)]\n",
            frame->pic_info[0].enabled, frame->pic_info[0].width,
            frame->pic_info[0].height, frame->pic_info[1].enabled,
            frame->pic_info[1].width, frame->pic_info[1].height,
            frame->pic_info[2].enabled, frame->pic_info[2].width,
            frame->pic_info[2].height, frame->pic_info[3].enabled,
            frame->pic_info[3].width, frame->pic_info[3].height,
            frame->pic_info[4].enabled, frame->pic_info[4].width,
            frame->pic_info[4].height);

    if (filter->vce_ds_enable) {
        if (!frame->pic_info[0].enabled || !frame->pic_info[2].enabled) {
            VPILOGE("When use 1/4 ds pass1 for vce, pp0 and pp1 should be "
                    "enabled!\n");
            return -1;
        }
        if (frame->pic_info[1].enabled || frame->pic_info[3].enabled ||
            frame->pic_info[4].enabled) {
            VPILOGE("When use 1/4 ds pass1 for vce, except pp0 and pp1 should "
                    "not be enabled!\n");
            return -1;
        }
        frame->pic_info[2].flag = 1;
    }

    frame->flag |= PP_FLAG;

    return 0;
}

static int pp_config_props(VpiPPFilter *filter, VpiPPOpition *cfg)
{
    int ret = 0;

    /* get iput from ffmpeg*/
    filter->nb_outputs       = cfg->nb_outputs;
    filter->low_res          = cfg->low_res;
    filter->force_10bit      = cfg->force_10bit;
    filter->w                = cfg->w;
    filter->h                = cfg->h;
    filter->format           = cfg->format;
    filter->frame            = (VpiFrame *)cfg->frame;
    filter->b_disable_tcache = cfg->b_disable_tcache;

    ret = pp_parse_low_res(filter);
    if (ret < 0) {
        VPILOGE("pp_parse_low_res failed=%d!\n", ret);
        return -1;
    }

    ret = pp_set_params(filter);
    if (ret < 0) {
        VPILOGE("pp_set_params failed = %d\n", ret);
        return -1;
    }
    filter->mwl = pp_trans_demuxer_init(&filter->params);
    if (!filter->mwl) {
        VPILOGE("pp_trans_demuxer_init failed = %d\n", ret);
        return -1;
    }

    filter->inst = pp_raw_parser_open(filter->format, filter->params.width,
                                      filter->params.height);
    if (!filter->inst) {
        VPILOGE("pp_raw_parser_open failed!\n");
        return -1;
    }

    filter->pp_client = pp_client_init(filter);
    if (!filter->pp_client) {
        VPILOGE("pp_client_init failed!\n");
        return -1;
    }

    ret = pp_set_hwframe_res(filter);
    if (ret < 0) {
        VPILOGE("pp_set_hwframe_res failed!\n");
        return -1;
    }

    ret = vpi_pp_input_buf_init(filter);
    if (ret < 0) {
        return -1;
    }

    pthread_mutex_init(&filter->pp_client->pp_mutex, NULL);

    return 0;
}

static int pp_picture_consumed(VpiPPFilter *filter, VpiFrame *input)
{
    PPClient *pp                  = filter->pp_client;
    struct DecPicturePpu *picture = NULL;
    int index                     = 0;
    u32 i;

#ifdef SUPPORT_TCACHE
    index = 0;
#else
    index = 1;
#endif

    if (pp == NULL) {
        VPILOGE("PP is NULL\n");
        return -1;
    }

    picture = (struct DecPicturePpu *)input->data[0];
    if (picture == NULL) {
        VPILOGE("picture is NULL\n");
        return -1;
    }

    pthread_mutex_lock(&pp->pp_mutex);
    for (i = 0; i < pp->out_buf_nums; i++) {
        if (pp->pp_out_buffer[i].bus_address ==
            picture->pictures[index].luma.bus_address) {
            VPILOGD("push buff %d...\n", i);
            FifoPush(pp->pp_out_Fifo, &pp->pp_out_buffer[i],
                     FIFO_EXCEPTION_DISABLE);
        }
    }

    free(picture);
    pthread_mutex_unlock(&pp->pp_mutex);
    return 0;
}

static void pp_print_dec_picture(PPDecPicture *dec_picture, int num)
{
    char info_string[2048];
    int i;

    sprintf(&info_string[0], "PIC %2d", num++);
    for (i = 0; i < 4; i++) {
        if (dec_picture->pictures[i].pp_enabled) {
            sprintf(&info_string[strlen(info_string)], ", %d : %d x %d", i,
                    dec_picture->pictures[i].pic_width,
                    dec_picture->pictures[i].pic_height);
        }
    }
    VPILOGD("%s\n", info_string);
}

VpiRet vpi_prc_pp_init(VpiPrcCtx *vpi_ctx, void *cfg)
{
    return VPI_SUCCESS;
}

VpiRet vpi_prc_pp_control(VpiPrcCtx *vpi_ctx, void *indata, void *outdata)
{
    VpiCtrlCmdParam *cmd = (VpiCtrlCmdParam *)indata;
    VpiPPFilter *filter  = &vpi_ctx->ppfilter;
    VpiPPOpition *cfg    = NULL;
    VpiFrame *frame      = NULL;
    int ret              = 0;

    switch (cmd->cmd) {
    case VPI_CMD_PP_CONFIG:
        cfg = (VpiPPOpition *)cmd->data;
        ret = pp_config_props(filter, cfg);
        if (ret < 0) {
            VPILOGE("VPI_CMD_PP_CONFIG failed=%d\n", ret);
        }
        break;
    case VPI_CMD_PP_CONSUME:
        frame = cmd->data;
        ret   = pp_picture_consumed(filter, frame);
        if (ret < 0) {
            VPILOGE("VPI_CMD_PP_CONSUME failed=%d\n", ret);
        }
        break;
    default:
        VPILOGE("Not supported command %d for PP\n", cmd->cmd);
        break;
    }
    return ret;
}

void dump_frame_picinfo(VpiFrame *frame)
{
    VpiPicInfo *pic = NULL;
    int i           = 0;

    for (i = 0; i < 5; i++) {
        pic = &frame->pic_info[i];
        VPILOGD("VpiPicInfo[%d]:\n", i);
        VPILOGD("\tenable=%d,flag=%d,format=%d,width=%d,height=%d,pic_w=%d,pic_"
                "h=%d,"
                "crop:enable=%d,(%d,%d,%d,%d), scale:enable=%d,(%d,%d),"
                "picdata:is_interlaced=%d,pic_stride=%d,crop_out_width=%d,"
                "crop_out_height=%d,pic_format=%d,pic_pixformat=%d,"
                "bit_depth_luma=%d,bit_depth_chroma=%d,pic_compressed_status=%"
                "d\n",
                pic->enabled, pic->flag, pic->format, pic->width, pic->height,
                pic->pic_width, pic->pic_height, pic->crop.enabled, pic->crop.w,
                pic->crop.h, pic->crop.x, pic->crop.y, pic->scale.enabled,
                pic->scale.w, pic->scale.h, pic->picdata.is_interlaced,
                pic->picdata.pic_stride, pic->picdata.crop_out_width,
                pic->picdata.crop_out_height, pic->picdata.pic_format,
                pic->picdata.pic_pixformat, pic->picdata.bit_depth_luma,
                pic->picdata.bit_depth_chroma,
                pic->picdata.pic_compressed_status);
    }
}

void dump_frame(VpiFrame *frame)
{
    VPILOGD("\nVpiFrame dump");
    VPILOGD("\ttask_id=%d,sw=%d,sh=%d,w=%d,h=%d,"
            "linesize[0-2]=%d,%d,%d,"
            "key_frame=%d,pts=%d,pkt_dts=%d,"
            "data[0-2]=[%p,%p,%p],pic_struct_size=%d,"
            "locked=%d,nb_outputs=%d,used_cnt=%d\n",
            frame->task_id, frame->src_width, frame->src_height, frame->width,
            frame->height, frame->linesize[0], frame->linesize[1],
            frame->linesize[2], frame->key_frame, frame->pts, frame->pkt_dts,
            frame->data[0], frame->data[1], frame->data[2],
            frame->pic_struct_size, frame->locked, frame->nb_outputs,
            frame->used_cnt);
    dump_frame_picinfo(frame);
    VPILOGD("VpiFrame dump finished\n\n");
}

VpiRet vpi_prc_pp_process(VpiPrcCtx *vpi_ctx, void *indata, void *outdata)
{
    VpiPPFilter *filter = &vpi_ctx->ppfilter;
    VpiFrame *input     = (VpiFrame *)indata;
    VpiFrame *output    = (VpiFrame *)outdata;
    PPClient *pp        = filter->pp_client;
    PPDecPicture dec_picture;
    struct DecPicturePpu * picPpu;
    PPInst pp_inst            = NULL;
    PPConfig *dec_cfg         = &pp->dec_cfg;
    PPContainer *pp_c         = NULL;
    static u32 decode_pic_num = 0;
    int ret                   = 0;

    if (!pp || !pp->pp_inst) {
        VPILOGE("PP was not inited\n");
        return 0;
    }

    pp_inst = pp->pp_inst;
    pp_c    = (PPContainer *)pp_inst;

    if (filter->b_disable_tcache == 0) {
        pp_c->b_disable_tcache = 0;
        /* read frame from VpeFrame to pp_in_buffer*/
        pp_send_packet(filter, &pp->pp_in_buffer, input);
        if (pp->pp_in_buffer == NULL) {
            goto err_exit;
        }
        dec_cfg->pp_in_buffer = pp->pp_in_buffer->buffer;

#ifdef SUPPORT_TCACHE
        if (pp->tcache_handle == NULL) {
            pp->tcache_handle =
                DWLGetIpHandleByOffset(pp_c->dwl, TCACHE_OFFSET_TO_VCEA);
            if (pp->tcache_handle == NULL) {
                VPILOGE("failed get tcache handle!\n");
                goto err_exit;
            }
        }
        if (pp->edma_handle == NULL) {
            pp->edma_handle = DWLGetIpHandleByOffset(pp_c->dwl,
                                                     PCIE_EDMA_REG_BASE -
                                                     PCIE_REG_START);
            if (pp->edma_handle == NULL) {
                VPILOGE("failed get edma handle!\n");
                goto err_exit;
            }
        }

        ret = pp_tcache_config(pp);
        if (ret < 0)
            goto err_exit;
#endif

#ifdef PP_EDMA_ERR_TEST
        if (pp_edma_eerror_check() != 0) {
            VPILOGE("[%s,%d]PP force edma error in function\n", __FUNCTION__,
                    __LINE__);
            goto err_exit;
        }
#endif
    } else {
        pp_c->b_disable_tcache = 1;

        picPpu = (struct DecPicturePpu *)input->data[0];
        pp->tcache_handle     = NULL;
        dec_cfg->pp_in_buffer = picPpu->pictures[0].luma;
    }

    ret = PPSetInput(pp_inst, pp->dec_cfg.pp_in_buffer);
    if (ret != PP_OK) {
        VPILOGE("encode fails for PPSetInput, ret %d\n", ret);
        goto err_exit;
    }

    pp_request_buf(pp);
    ret = PPSetOutput(pp_inst, pp->dec_cfg.pp_out_buffer);
    if (ret != PP_OK) {
        VPILOGE("encode fails for PPSetOutput\n");
        goto err_exit;
    }

    pp_dump_config(pp);

    pp->max_frames_delay = filter->frame->max_frames_delay;
    pthread_mutex_lock(&pp->pp_mutex);
    if (pp_check_buffer_number_for_trans(pp) < 0) {
        pthread_mutex_unlock(&pp->pp_mutex);
        return -1;
    }

    pthread_mutex_unlock(&pp->pp_mutex);
    ret = PPDecode(pp_inst);
    if (ret != PP_OK) {
        VPILOGE("encode fails for PPDecode failed, %d\n", ret);
        goto err_exit;
    }
    ret = PPNextPicture(pp_inst, &dec_picture);
    pp_get_next_pic(pp, &dec_picture);
    pp_print_dec_picture(&dec_picture, decode_pic_num++);
    ret = pp_output_frame(filter, output, &dec_picture);

    pp->num_of_output_pics++;
    return ret;

err_exit:
    return -1;
}

VpiRet vpi_prc_pp_close(VpiPrcCtx *ctx)
{
    VpiPPFilter *filter = &ctx->ppfilter;
    PPClient *pp_client = filter->pp_client;
    VpiRawParser *inst  = (VpiRawParser *)filter->inst;

    if (pp_client != NULL) {
        pp_buf_release(pp_client);
        PPRelease(pp_client->pp_inst);
        if (pp_client->dwl)
            DWLRelease(pp_client->dwl);
#ifdef CHECK_MEM_LEAK_TRANS
        DWLfree(pp_client);
#else
        free(pp_client);
#endif
        filter->pp_client = NULL;
    }

    pthread_mutex_destroy(&pp_client->pp_mutex);

    if (inst) {
#ifdef CHECK_MEM_LEAK_TRANS
        DWLfree(inst);
#else
        free(inst);
#endif
        filter->inst = NULL;
    }

    if (filter->mwl != NULL) {
        pp_mwl_free_linear(filter->mwl, &filter->buffers.buffer);
    }
    if (filter->mwl != NULL) {
        pp_mwl_release(filter->mwl);
        filter->mwl = NULL;
    }
    return VPI_SUCCESS;
}
