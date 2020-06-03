/*
 * Copyright 2019 VeriSilicon, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdio.h>

#include "vpi.h"
#include "vpi_log.h"
#include "vpi_video_dec_info.h"

#ifndef BUILD_CMODEL
int DWLGetCoreStatistic(const void *instance, int *total_usage,
                        int core_usage[4]);
#endif

void vpi_dec_performance_report(VpiDecCtx *vpi_ctx)
{
    int i;
    char info_string[2048];

#ifndef BUILD_CMODEL
    struct statistic dec_statistic = { 0 };
    dec_statistic.frame_count      = vpi_ctx->pic_display_number;
    if (vpi_ctx->pic_display_number) {
        dec_statistic.cycle_mb_avg =
            vpi_ctx->cycle_count / vpi_ctx->pic_display_number;
        DWLGetCoreStatistic(vpi_ctx->dwl_inst, &dec_statistic.total_usage,
                            dec_statistic.core_usage_counts);
#ifdef FB_PERFORMANCE_STATIC
        dec_statistic.hw_real_time_avg =
            DWLGetHwPerformance(vpi_ctx->dwl_inst) / dec_statistic.frame_count;
        dec_statistic.hw_real_time_avg_remove_overlap =
            DWLGetHwPerformanceRemoveOverlap(vpi_ctx->dwl_inst) /
            dec_statistic.frame_count;
#endif
    }
#endif

    sprintf(&info_string[0], "DEC: %dframes, %dCycles/MB, %dus/frame, %.2ffps",
            dec_statistic.frame_count, dec_statistic.cycle_mb_avg,
            dec_statistic.hw_real_time_avg,
            (dec_statistic.hw_real_time_avg == 0) ?
                0.0 :
                1000000.0 / ((double)dec_statistic.hw_real_time_avg));

    if (dec_statistic.hw_real_time_avg -
            dec_statistic.hw_real_time_avg_remove_overlap >
        10) {
        sprintf(&info_string[strlen(info_string)],
                ", remove overlap : %d us/frame, %.2f fps",
                dec_statistic.hw_real_time_avg_remove_overlap,
                (dec_statistic.hw_real_time_avg_remove_overlap == 0) ?
                    0.0 :
                    1000000.0 /
                        ((double)dec_statistic.hw_real_time_avg_remove_overlap));
    }

    VPILOGI("%s\n", info_string);
    VPILOGI(":::DEC Multi-core usage statistics:\n");

    if (dec_statistic.total_usage == 0) {
        dec_statistic.total_usage = 1;
    }
    for (i = 0; i < 4; i++) {
        VPILOGI("\tSlice[%d] Core[%d] used %6d times (%2d%%)\n", i / 2, i % 2,
                dec_statistic.core_usage_counts[i],
                (dec_statistic.core_usage_counts[i] * 100) /
                    dec_statistic.total_usage);
    }

#ifdef FB_SYSLOG_ENABLE
    VPI_DEC_INFO_PRINT("%s\n", info_string);
    VPI_DEC_INFO_PRINT(":::DEC Multi-core usage statistics:\n");
    for (i = 0; i < 4; i++) {
        VPI_DEC_INFO_PRINT("\tSlice[%d] Core[%d] used %6d times (%2d%%)\n",
                           i / 2, i % 2, dec_statistic.core_usage_counts[i],
                           (dec_statistic.core_usage_counts[i] * 100) /
                               dec_statistic.total_usage);
    }

#endif
}

void vpi_report_dec_pic_info(VpiDecCtx *vpi_ctx, struct DecPicturePpu *picture)
{
    char info_string[2048];
    static char *pic_types[] = { "        IDR", "Non-IDR (P)", "Non-IDR (B)" };

    sprintf(&info_string[0], "PIC %2d/%2d, type %s, ",
            vpi_ctx->pic_display_number,
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
    VPI_DEC_INFO_PRINT("%s\n", info_string);
}

const char *dec_ret_string(int retval)
{
    switch (retval) {
    case DEC_OK:
        return "Success";
    case DEC_STRM_PROCESSED:
        return "Stream Processed";
    case DEC_PIC_RDY:
        return "Picture Ready";
    case DEC_PIC_DECODED:
        return "Picture Decoded";
    case DEC_HDRS_RDY:
        return "Header Ready";
    case DEC_ADVANCED_TOOLS:
        return "Advanced Tools";
    case DEC_PENDING_FLUSH:
        return "Pending Flush";
    case DEC_NONREF_PIC_SKIPPED:
        return "Nonref Picture Shipped";
    case DEC_END_OF_STREAM:
        return "End Of Stream";
    case DEC_WAITING_FOR_BUFFER:
        return "Waitting For Buffer";
    case DEC_ABORTED:
        return "Dec Aborted";
    case DEC_FLUSHED:
        return "Dec Flushed";
    case DEC_BUF_EMPTY:
        return "Buffer Empty";
    case DEC_STREAM_ERROR_DEDECTED:
        return "Stream Error Detected";
    case DEC_RESOLUTION_CHANGE:
        return "Resolution Change";
    case DEC_PARAM_ERROR:
        return "Param Error";
    case DEC_STRM_ERROR:
        return "Stream Error";
    case DEC_NOT_INITIALIZED:
        return "Not Initialized";
    case DEC_MEMFAIL:
        return "Memory Fail";
    case DEC_INITFAIL:
        return "Init Fail";
    case DEC_HDRS_NOT_RDY:
        return "Header Not Ready";
    case DEC_STREAM_NOT_SUPPORTED:
        return "Stream Not Supported";
    case DEC_EXT_BUFFER_REJECTED:
        return "Ext Buffer Rejected";
    case DEC_INFOPARAM_ERROR:
        return "Info Param Error";
    case DEC_NO_DECODING_BUFFER:
        return "No Decoding Buffer";
    case DEC_HW_RESERVED:
        return "HW Reserved";
    case DEC_HW_TIMEOUT:
        return "HW Timeout";
    case DEC_HW_BUS_ERROR:
        return "HW Bus Error";
    case DEC_SYSTEM_ERROR:
        return "System Error";
    case DEC_DWL_ERROR:
        return "DWL Error";
    case DEC_FATAL_SYSTEM_ERROR:
        return "Fatal System Error";
    case DEC_FORMAT_NOT_SUPPORTED:
        return "Format Not Supported";
    default:
        return "Unknown Error";
    }
}

void print_decode_return(int retval)
{
    static int prev_retval = 0xFFFFFF;

    if (prev_retval != retval || (prev_retval != DEC_NO_DECODING_BUFFER &&
                                  prev_retval != DEC_PENDING_FLUSH)) {
        VPILOGD("TB: DecDecode returned: ");
    }
    switch (retval) {
    case DEC_NO_DECODING_BUFFER:
        /* There may be too much DEC_NO_DECODING_BUFFER.
                Only print for the 1st time. */
        if (prev_retval != DEC_NO_DECODING_BUFFER) {
            VPILOGD("%s\n", dec_ret_string(retval));
        }
        break;
    case DEC_PENDING_FLUSH:
        if (prev_retval != DEC_PENDING_FLUSH) {
            VPILOGD("%s\n", dec_ret_string(retval));
        }
        break;
    default:
        VPILOGD("%s\n", dec_ret_string(retval));
        break;
    }
    prev_retval = retval;
}
