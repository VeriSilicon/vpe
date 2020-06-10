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

#ifndef __VPI_VIDEO_DEC_PICTURE_CONSUME_H__
#define __VPI_VIDEO_DEC_PICTURE_CONSUME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "vpi_video_dec.h"

void init_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx);
uint32_t find_dec_pic_wait_consume_index(VpiDecCtx *vpi_ctx, uint8_t *data);
uint32_t find_dec_pic_wait_consume_empty_index(VpiDecCtx *vpi_ctx);
void add_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx, void *data);
void del_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx, uint8_t *data);
void free_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx);

#ifdef __cplusplus
}
#endif

#endif