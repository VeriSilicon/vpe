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

#ifndef __VPI_VIDEO_VP9DEC_H__
#define __VPI_VIDEO_VP9DEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "vpi_video_dec.h"

#ifndef NEXT_MULTIPLE
#define NEXT_MULTIPLE(value, n) (((value) + (n)-1) & ~((n)-1))
#endif

VpiRet vpi_dec_vp9_init(const void **inst, struct DecConfig config,
                        const void *dwl);
VpiRet vpi_dec_vp9_get_info(VpiDecInst inst, struct DecSequenceInfo *info);
VpiRet vpi_dec_vp9_set_info(VpiDecInst inst, struct DecConfig config,
                            struct DecSequenceInfo *Info);
enum DecRet vpi_dec_vp9_next_picture(VpiDecInst inst,
                                     struct DecPicturePpu *pic);
VpiRet vpi_dec_vp9_picture_consumed(VpiDecInst inst, struct DecPicturePpu pic);
int vpi_decode_vp9_dec_frame(VpiDecCtx *vpi_ctx, void *indata, void *outdata);
VpiRet vpi_dec_vp9_end_of_stream(VpiDecInst inst);
void vpi_dec_vp9_release(VpiDecInst inst);
void vpi_decode_vp9_picture_consume(VpiDecCtx *vpi_ctx, void *data);
VpiRet vpi_decode_vp9_init(VpiDecCtx *vpi_ctx);
VpiRet vpi_decode_vp9_control(VpiDecCtx *vpi_ctx, void *indata, void *outdata);
int vpi_decode_vp9_close(VpiDecCtx *vpi_ctx);
int vpi_decode_vp9_put_packet(VpiDecCtx *vpi_ctx, void *indata);
int vpi_decode_vp9_get_frame(VpiDecCtx *vpi_ctx, void *outdata);
int vpi_decode_vp9_dec_process(VpiDecCtx *vpi_ctx);

#ifdef USE_EXTERNAL_BUFFER
enum DecRet vpi_dec_vp9_get_buffer_info(VpiDecInst inst,
                                        struct DecBufferInfo *buf_info);
enum DecRet vpi_dec_vp9_add_buffer(VpiDecInst inst, struct DWLLinearMem *buf);
#endif

#ifdef __cplusplus
}
#endif

#endif