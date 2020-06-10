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

#ifndef __VPI_VIDEO_DEC_PP_H__
#define __VPI_VIDEO_DEC_PP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "vpi_types.h"
#include "vpi_video_dec.h"

void dump_ppu(PpUnitConfig *ppu_cfg);
VpiRet vpi_dec_parse_ppu_cfg(VpiDecCtx *vpi_ctx, PpUnitConfig *ppu_cfg);
void vpi_resolve_pp_overlap_ppu(PpUnitConfig *ppu_cfg,
                                struct TBPpUnitParams *pp_units_params);
VpiRet vpi_dec_parse_resize(VpiDecCtx *vpi_ctx, DecOption *dec_cfg);
void vpi_dec_disable_all_pp_shaper(struct DecConfig *config);

#ifdef __cplusplus
}
#endif

#endif
