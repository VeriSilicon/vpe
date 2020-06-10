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

#ifndef __VPI_VIDEO_DEC_BUFFER_H__
#define __VPI_VIDEO_DEC_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "vpi_video_dec.h"

void vpi_dec_buf_list_add(BufLink **head, BufLink *list);
BufLink* vpi_dec_buf_list_delete(BufLink *head);
int vpi_send_packet_to_decode_buffer(VpiDecCtx *vpi_ctx, VpiPacket *vpi_packet,
                                     struct DWLLinearMem stream_buffer);
int vpi_dec_get_stream_buffer_index(VpiDecCtx *vpi_ctx, int status);
void vpi_dec_release_ext_buffers(VpiDecCtx *vpi_ctx);
int vpi_dec_check_buffer_number_for_trans(VpiDecCtx *vpi_ctx);
VpiRet vpi_dec_output_frame(VpiDecCtx *vpi_ctx, VpiFrame *vpi_frame,
                            struct DecPicturePpu *decoded_pic);

#ifdef __cplusplus
}
#endif

#endif
