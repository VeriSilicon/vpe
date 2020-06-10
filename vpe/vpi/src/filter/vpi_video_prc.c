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

#include "vpi_log.h"
#include "vpi_video_prc.h"
#include "vpi_video_hwdwprc.h"
#include "vpi_video_pp.h"

VpiRet vpi_vprc_init(VpiPrcCtx *vpi_ctx, void *prc_cfg)
{
    VpiRet ret = VPI_SUCCESS;

    switch (vpi_ctx->filter_type) {
    case FILTER_PP:
        ret = vpi_prc_pp_init(vpi_ctx, prc_cfg);
        break;
    case FILTER_SPLITER:
        break;
    case FILTER_HW_DOWNLOADER:
        ret = vpi_prc_hwdw_init(vpi_ctx, prc_cfg);
        break;
    default:
        VPILOGW("Unknown/Not supported format %d", vpi_ctx->filter_type);
        ret = VPI_ERR_VALUE;
    }

    return ret;
}

VpiRet vpi_vprc_process(VpiPrcCtx *vpi_ctx, void *indata, void *outdata)
{
    VpiRet ret = VPI_SUCCESS;

    switch (vpi_ctx->filter_type) {
    case FILTER_PP:
        ret = vpi_prc_pp_process(vpi_ctx, indata, outdata);
        break;
    case FILTER_SPLITER:
        break;
    case FILTER_HW_DOWNLOADER:
        ret = vpi_prc_hwdw_process(vpi_ctx, indata, outdata);
        break;
    default:
        VPILOGW("Unknown/Not supported format %d", vpi_ctx->filter_type);
        break;
    }
    return ret;
}

VpiRet vpi_vprc_control(VpiPrcCtx *vpi_ctx, void *indata, void *outdata)
{
    VpiRet ret = VPI_SUCCESS;

    switch (vpi_ctx->filter_type) {
    case FILTER_PP:
        vpi_prc_pp_control(vpi_ctx, indata, outdata);
        break;
    case FILTER_SPLITER:
        break;
    case FILTER_HW_DOWNLOADER:
        vpi_prc_hwdw_control(vpi_ctx, indata, outdata);
        break;
    default:
        VPILOGW("Unknown/Not supported format %d", vpi_ctx->filter_type);
        ret = VPI_ERR_VALUE;
    }

    return ret;
}

int vpi_vprc_close(VpiPrcCtx *vpi_ctx)
{
    int ret = VPI_SUCCESS;

    switch (vpi_ctx->filter_type) {
    case FILTER_PP:
        ret = vpi_prc_pp_close(vpi_ctx);
        break;
    case FILTER_SPLITER:
        break;
    case FILTER_HW_DOWNLOADER:
        ret = vpi_prc_hwdw_close(vpi_ctx);
        break;
    default:
        VPILOGW("Unknown/Not supported format %d", vpi_ctx->filter_type);
        ret = VPI_ERR_VALUE;
    }

    return ret;
}
