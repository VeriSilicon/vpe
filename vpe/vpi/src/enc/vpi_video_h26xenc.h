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

#ifndef __VPI_VIDEO_H26XENC_H__
#define __VPI_VIDEO_H26XENC_H__
#include "vpi_video_h26xenc_cfg.h"

extern VPIH26xParamsDef h26x_enc_param_table[];

#ifndef NEXT_MULTIPLE
#define NEXT_MULTIPLE(value, n) (((value) + (n)-1) & ~((n)-1))
#endif

int vpi_h26xe_init(struct VpiH26xEncCtx *enc_ctx, H26xEncCfg *enc_cfg);
int vpi_h26xe_encode(struct VpiH26xEncCtx *enc_ctx, void *input, void *output);
int vpi_h26xe_close(struct VpiH26xEncCtx *enc_ctx);
int vpi_h26xe_ctrl(struct VpiH26xEncCtx *enc_ctx, void *vpi_ctrl_type,
                   void *vpi_value);
#endif /*__VPI_VIDEO_H26XENC_H__ */
