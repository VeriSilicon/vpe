/*
 * Copyright 2020 VeriSilicon, Inc.
 *
 *	The program is distributed under terms of BSD license.
 *	You can obtain the copy of the license by visiting:
 *
 *	http://www.opensource.org/licenses/bsd-license.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dectypes.h"
#include "hugepage_api.h"

#include "vpi_types.h"
#include "vpi_log.h"
#include "vpi_video_hwdwprc.h"

VpiRet vpi_prc_hwdw_init(VpiPrcCtx *vpi_ctx, void *cfg)
{
    char *device = (char *)cfg;

    vpi_ctx->edma_handle = TRANS_EDMA_init(device);
    if (vpi_ctx->edma_handle == NULL) {
        VPILOGE("hwdownload edma_handle init failed!\n");
        return VPI_ERR_OPEN_FILE;
    }

    return VPI_SUCCESS;
}

VpiRet vpi_prc_hwdw_process(VpiPrcCtx *vpi_ctx, void *indata, void *outdata)
{
    struct DecPicturePpu *pic;
    struct DecPicture *pic_info;
    VpiFrame *in_frame     = (VpiFrame *)indata;
    VpiFrame *out_frame    = (VpiFrame *)outdata;
    uint8_t pix_width      = 8;
    addr_t bus_address_lum = 0, bus_address_chroma = 0;
    uint32_t y_size, uv_size;
    uint32_t linesize[2];
    int i;
    uint64_t dst_addr;
    uint32_t uv_height;

    pic      = (struct DecPicturePpu *)in_frame->data[0];
    pic_info = &pic->pictures[vpi_ctx->pp_index];
    if (pic_info->picture_info.format == 0) {
        //tile4x4
        VPILOGE("picture_info.format is DEC_OUT_FRM_TILED_4X4, \
                 only support DEC_OUT_FRM_RASTER_SCAN !\n");
        return VPI_ERR_VALUE;
    }

    if ((pic_info->sequence_info.bit_depth_chroma == 8) &&
        (pic_info->sequence_info.bit_depth_luma == 8)) {
        pix_width = 8;
    } else {
        pix_width = 16;
    }
    linesize[0] = pic_info->pic_width * (pix_width / 8);
    linesize[1] = pic_info->pic_width * (pix_width / 8);
    y_size      = pic_info->pic_width * pic_info->pic_height * (pix_width / 8);
    uv_size = pic_info->pic_width * pic_info->pic_height * (pix_width / 8) / 2;

    if (out_frame->linesize[0] == 0 && out_frame->linesize[1] == 0) {
        // use hwdownload_vpe
        out_frame->width  = pic_info->pic_width;
        out_frame->height = pic_info->pic_height;
        if (y_size < 8 * 1024) {
            y_size = 8 * 1024;
        }
        if (uv_size < 8 * 1024) {
            uv_size = 8 * 1024;
        }
        out_frame->linesize[0] = linesize[0];
        out_frame->linesize[1] = linesize[1];
        out_frame->src_width   = y_size;
        out_frame->src_height  = uv_size;
        out_frame->data[0]     = fbtrans_get_huge_pages(y_size);
        if (out_frame->data[0] == NULL) {
            VPILOGE("No memory available for the frame buffer\n");
            return VPI_ERR_NO_HW_RSC;
        }

        out_frame->data[1] = fbtrans_get_huge_pages(uv_size);
        if (out_frame->data[1] == NULL) {
            VPILOGE("No memory available for the frame buffer\n");
            return VPI_ERR_NO_HW_RSC;
        }

        bus_address_lum    = pic_info->luma.bus_address;
        bus_address_chroma = pic_info->chroma.bus_address;
        TRANS_EDMA_EP2RC_nonlink(vpi_ctx->edma_handle, bus_address_lum,
                                 (uint64_t)out_frame->data[0], y_size);
        TRANS_EDMA_EP2RC_nonlink(vpi_ctx->edma_handle, bus_address_chroma,
                                 (uint64_t)out_frame->data[1], uv_size);
    } else {
        // use hwdownload
        if (out_frame->linesize[0] < linesize[0] ||
            out_frame->linesize[1] < linesize[1]) {
            VPILOGE("APP allocated memory is lower than Codec SDK\n");
            VPILOGE("APP linesize %d %d, SDK linesize %d %d\n",
                    out_frame->linesize[0], out_frame->linesize[1], linesize[0],
                    linesize[1]);
            return VPI_ERR_VALUE;
        }

        if (out_frame->linesize[0] == linesize[0] &&
            out_frame->linesize[1] == linesize[1]) {
            bus_address_lum    = pic_info->luma.bus_address;
            bus_address_chroma = pic_info->chroma.bus_address;
            TRANS_EDMA_EP2RC_nonlink(vpi_ctx->edma_handle, bus_address_lum,
                                     (uint64_t)out_frame->data[0], y_size);
            TRANS_EDMA_EP2RC_nonlink(vpi_ctx->edma_handle, bus_address_chroma,
                                     (uint64_t)out_frame->data[1], uv_size);
        } else {
            for (i = 0; i < pic_info->pic_height; i++) {
                bus_address_lum = pic_info->luma.bus_address + i * linesize[0];
                dst_addr =
                    (uint64_t)out_frame->data[0] + i * out_frame->linesize[0];
                TRANS_EDMA_EP2RC_nonlink(vpi_ctx->edma_handle, bus_address_lum,
                                         dst_addr, linesize[0]);
            }
            uv_height = (pic_info->pic_height + 1) / 2;
            for (i = 0; i < uv_height; i++) {
                bus_address_chroma =
                    pic_info->chroma.bus_address + i * linesize[1];
                dst_addr =
                    (uint64_t)out_frame->data[1] + i * out_frame->linesize[1];
                TRANS_EDMA_EP2RC_nonlink(vpi_ctx->edma_handle,
                                         bus_address_chroma, dst_addr,
                                         linesize[1]);
            }
        }
    }

    in_frame->used_cnt++;
    if (in_frame->used_cnt == in_frame->nb_outputs) {
        in_frame->locked = 0;
    }
    VPILOGD("linesize %d %d\n", out_frame->linesize[0], out_frame->linesize[1]);

    VPILOGD("in %s:%d, dma get luma data from EP:%p to RC:%p \n", __FUNCTION__,
            __LINE__, (void *)bus_address_lum, out_frame->data[0]);
    VPILOGD("in %s:%d, dma get chroma data from EP:%p to RC:%p \n",
            __FUNCTION__, __LINE__, (void *)bus_address_chroma,
            out_frame->data[1]);

    return VPI_SUCCESS;
}

VpiRet vpi_prc_hwdw_control(VpiPrcCtx *vpi_ctx, void *indata, void *outdata)
{
    VpiCtrlCmdParam *in_param = (VpiCtrlCmdParam *)indata;

    switch (in_param->cmd) {
    case VPI_CMD_HWDW_FREE_BUF:
        fbtrans_free_huge_pages(in_param->data, 8 * 1024);
        break;
    case VPI_CMD_HWDW_SET_INDEX: {
        vpi_ctx->pp_index = *(int *)in_param->data;
        break;
    }
    default:
        break;
    }

    return VPI_SUCCESS;
}

VpiRet vpi_prc_hwdw_close(VpiPrcCtx *vpi_ctx)
{
    if (vpi_ctx->edma_handle) {
        TRANS_EDMA_release(vpi_ctx->edma_handle);
        vpi_ctx->edma_handle = NULL;
    }

    return VPI_SUCCESS;
}
