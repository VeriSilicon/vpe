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

#ifndef __VPI_VIDEO_HWDWPRC_H__
#define __VPI_VIDEO_HWDWPRC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "vpi_video_prc.h"

VpiRet vpi_prc_hwdw_init(VpiPrcCtx *vpi_ctx, void *cfg);
VpiRet vpi_prc_hwdw_process(VpiPrcCtx *vpi_ctx, void *indata, void *outdata);
VpiRet vpi_prc_hwdw_control(VpiPrcCtx *vpi_ctx, void *indata, void *outdata);
VpiRet vpi_prc_hwdw_close(VpiPrcCtx *vpi_ctx);

#ifdef __cplusplus
}
#endif

#endif