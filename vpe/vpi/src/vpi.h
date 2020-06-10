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

#ifndef __VPI_H__
#define __VPI_H__

#include "time.h"
#include "vpi_types.h"

typedef struct VpeVpiCtx {
    uint32_t dummy;
    VpiPlugin plugin;
    void *ctx;
} VpeVpiCtx;

typedef struct VpiHwCtx {
    char device_name[32];
    int hw_context;
    int task_id;
    int priority;
    void *sys_info;
} VpiHwCtx;

typedef struct VpiCodecCtx {
    void *vpi_dec_ctx;
    void *vpi_enc_ctx;
    void *vpi_prc_pp_ctx;
    void *vpi_prc_spliter_ctx;
    void *vpi_prc_hwdw_ctx;
    int ref_cnt;
} VpiCodecCtx;

typedef struct statistic {
    uint32_t frame_count;
    uint32_t cycle_mb_avg;
    uint32_t cycle_mb_avg_p1;
    uint32_t cycle_mb_avg_total;
    double ssim_avg;
    uint32_t bitrate_avg;
    uint32_t hw_real_time_avg;
    uint32_t hw_real_time_avg_remove_overlap;
    int total_usage;
    int core_usage_counts[4];
    struct timeval last_frame_encoded_timestamp;
} statistic;

#endif
