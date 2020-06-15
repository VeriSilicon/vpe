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

#ifndef __VPI_VIDEO_H26XENC_CFG_H__
#define __VPI_VIDEO_H26XENC_CFG_H__

#ifdef HEVCDECAPI_H
#undef HANTRO_FALSE
#undef HANTRO_TRUE
#endif

#ifdef SW_UTIL_H_
#undef ABS
#undef MAX
#undef MIN
#undef CLIP3
#endif

#include "encinputlinebuffer.h"
#include "hevcencapi.h"
#ifdef FB_SYSLOG_ENABLE
#include "syslog_sink.h"
#endif
#include "vpi_types.h"
#include "vpi_video_h26xenc_options.h"

#define MAX_FIFO_DEPTH 16
#define MAX_WAIT_DEPTH 78 /*34*/
#define MAX_ENC_NUM 4

#define DEFAULT_VALUE -255

#define DEFAULT -255
#define MAX_CUTREE_DEPTH 64
#define MAX_DELAY_NUM (MAX_CORE_NUM + MAX_CUTREE_DEPTH)

typedef struct {
    u32 stream_pos;
    u32 multislice_encoding;
    u32 output_byte_stream;
    FILE *out_stream_file;
} SliceCtl;

typedef struct {
    u32 stream_rd_counter;
    u32 stream_multi_seg_en;
    u8 *stream_base;
    u32 segment_size;
    u32 segment_amount;
    FILE *out_stream_file;
    u8 start_code_done;
    i32 output_byte_stream;
} SegmentCtl;

struct VPIH26xEncCfg {
#ifdef FB_SYSLOG_ENABLE
    LOG_INFO_HEADER log_header;
#endif
    ENCPERF *perf;
    char *input;
    char *half_ds_input;
    char *output;
    char *test_data_files;
    FILE *in;
    FILE *in_ds;
    FILE *out;
    FILE *fmv;
    i32 width;
    i32 height;
    i32 output_rate_numer; /* Output frame rate numerator */
    i32 output_rate_denom; /* Output frame rate denominator */
    i32 input_rate_numer; /* Input frame rate numerator */
    i32 input_rate_denom; /* Input frame rate denominator */
    i32 first_pic;
    i32 last_pic;
    i32 picture_cnt;
    i32 input_pic_cnt;
    i32 picture_enc_cnt;
    i32 idr_interval;
    i32 last_idr_picture_cnt;
    i32 byte_stream;
    u8 *lum;
    u8 *cb;
    u8 *cr;
    u8 *lum_ds;
    u8 *cb_ds;
    u8 *cr_ds;
    u32 src_img_size_ds;
    i32 interlaced_frame;
    u32 validencoded_framenumber;
    u32 input_alignment;
    u32 ref_alignment;
    u32 ref_ch_alignment;
    i32 format_customized_type;
    u32 luma_size;
    u32 chroma_size;
    u32 luma_size_ds;
    u32 chroma_size_ds;
    u32 transformed_size;
    VCEncRateCtrl rc;

    char **argv;
    i32 argc;
    /* Moved from global space */
    FILE *yuv_file;
    FILE *roi_map_file;
    FILE *roi_map_bin_file;
    FILE *ipcm_map_file;
    FILE *skip_map_file;
    FILE *roi_map_info_bin_file;
    FILE *roimap_cu_ctrl_info_bin_file;
    FILE *roimap_cu_ctrl_index_bin_file;

    /* SW/HW shared memories for input/output buffers */
    EWLLinearMem_t *picture_mem;
    EWLLinearMem_t *picture_dsmem;
    EWLLinearMem_t *outbuf_mem[MAX_STRM_BUF_NUM];
    EWLLinearMem_t *roi_map_delta_qp_mem;
    EWLLinearMem_t *transform_mem;
    EWLLinearMem_t *roimap_cu_ctrl_infomem;
    EWLLinearMem_t *roimap_cu_ctrl_indexmem;

    EWLLinearMem_t picture_mem_factory[MAX_DELAY_NUM];
    EWLLinearMem_t picture_dsmem_factory[MAX_DELAY_NUM];
    EWLLinearMem_t outbuf_mem_factory[MAX_CORE_NUM]
                                     [MAX_STRM_BUF_NUM]; /* [coreIdx][bufIdx] */
    EWLLinearMem_t roi_map_delta_qpmem_factory[MAX_DELAY_NUM];
    EWLLinearMem_t transform_mem_factory[MAX_DELAY_NUM];
    EWLLinearMem_t roimap_cu_ctrl_infomem_factory[MAX_DELAY_NUM];
    EWLLinearMem_t roimap_cu_ctrl_indexmem_factory[MAX_DELAY_NUM];

    EWLLinearMem_t scaled_picture_mem;
    float sum_square_of_error;
    float average_square_of_error;
    i32 max_error_over_target;
    i32 max_error_under_target;
    long number_square_of_error;

    u32 gop_size;
    i32 next_gop_size;
    VCEncIn enc_in;

    inputLineBufferCfg input_ctb_linebuf;

    FILE *gmv_file[2];

    u32 parallel_core_num;
    SliceCtl slice_ctl_factory[MAX_DELAY_NUM];
    SliceCtl *slice_ctl;
    SliceCtl *slice_ctl_out;

    const void *ewl;
    const void *two_pass_ewl;

    int enc_index;
    double ssim_acc;
    i64 hwcycle_acc;

#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
    char *inputts;
    FILE *ints;
    u8 *tslu;
    u8 *tsch;
    u32 tbl_luma_size;
    u32 tbl_chroma_size;
    u32 tbl_luma_size_ds;
    u32 tbl_chroma_size_ds;
    EWLLinearMem_t *ts_lumamem;
    EWLLinearMem_t *ts_chromamem;
    EWLLinearMem_t ts_lumamem_factory[MAX_DELAY_NUM];
    EWLLinearMem_t ts_chromamem_factory[MAX_DELAY_NUM];
    void *dec400f2_handle;
#endif

#ifdef DRV_NEW_ARCH
    int priority;
    char *device;
    int mem_id;
#endif
    i32 stream_buf_num;
    u32 frame_delay;
    u32 buffer_cnt;
    SegmentCtl stream_seg_ctl;

    struct timeval time_frame_start;
    struct timeval time_frame_end;
};

#define MOVING_AVERAGE_FRAMES 120

typedef struct {
    i32 frame[MOVING_AVERAGE_FRAMES];
    i32 length;
    i32 count;
    i32 pos;
    i32 frame_rate_numer;
    i32 frame_rate_denom;
} MaS;

typedef struct {
    int gop_frm_num;
    double sum_intra_vs_interskip;
    double sum_skip_vs_interskip;
    double sum_intra_vs_interskipP;
    double sum_intra_vs_interskipB;
    int sum_cost_P;
    int sum_cost_B;
    int last_gopsize;
} AdapGopCtr;

typedef struct VpiH26xEncInAddr {
    ptr_t bus_luma;
    ptr_t bus_chroma;
    ptr_t bus_luma_table;
    ptr_t bus_chroma_table;

    ptr_t bus_luma_ds;
    ptr_t bus_chroma_ds;
    ptr_t bus_luma_table_ds;
    ptr_t bus_chroma_table_ds;
} VpiH26xEncInAddr;

typedef struct VpiH26xEncCtx {
    VPIH26xEncOptions options; /*The first item of VpiH26xEncCtx structure*/
    VCEncInst hantro_encoder;
    int stream_size;
    u32 frame_cnt_total;
    u32 frame_cnt_output;
    int next_poc;
    VCEncPictureCodingType next_coding_type;
    bool encoder_is_start;
    bool adaptive_gop;
    AdapGopCtr agop;
    MaS ma;
    u64 total_bits;
    i32 next_gop_size;
    u8 *p_user_data;
    VpiFlushState flush_state;
    bool trans_flush_pic;
    VCEncIn enc_in_bk;
    int picture_cnt_bk;
    bool encoder_is_end;
    int no_input_pict;
    int find_pict;
    int pp_index;
    VCEncOut enc_out;
    VCEncGopPicConfig gop_pic_cfg[MAX_GOP_PIC_CONFIG_NUM];
    VCEncGopPicConfig gop_pic_cfg_pass2[MAX_GOP_PIC_CONFIG_NUM];
    VCEncGopPicSpecialConfig gop_pic_special_cfg[MAX_GOP_SPIC_CONFIG_NUM];
    struct VPIH26xEncCfg vpi_h26xe_cfg;
    VPIH26xParamsDef *h26x_enc_param_table;
} VpiH26xEncCtx;

enum {
    VPI_DEC_OUT_RFC,
    VPI_DEC_OUT_PP0,
    VPI_DEC_OUT_PP1,
    VPI_DEC_OUT_PP2,
    VPI_DEC_OUT_PP3,
};

int h26x_enc_check_area(VCEncPictureArea *area, VPIH26xEncOptions *options);
void h26x_enc_stream_segment_ready(void *cb_data);
int h26x_enc_init_input_line_buffer(inputLineBufferCfg *line_buf_cfg,
                                    VPIH26xEncOptions *options, VCEncIn *encIn,
                                    VCEncInst inst,
                                    struct VPIH26xEncCfg *vpi_h26xe_cfg);
void h26x_enc_init_pic_config(VCEncIn *p_enc_in);
int h26x_enc_set_options(struct VpiH26xEncCtx *vpi_h26xe_ctx,
                         VpiH26xEncCfg *h26x_enc_cfg);
int h26x_enc_get_deviceId(char *dev);
int h26x_enc_alloc_res(VPIH26xEncOptions *cmdl, VCEncInst enc,
                       struct VPIH26xEncCfg *vpi_h26xe_cfg);
void h26x_enc_free_res(VCEncInst enc, struct VPIH26xEncCfg *vpi_h26xe_cfg);
int h26x_enc_update_statistic(struct VpiH26xEncCtx *enc_ctx, int *streamSize);
void h26x_enc_report(struct VpiH26xEncCtx *enc_ctx);
#endif /*__VPI_VIDEO_H26XENC_CFG_H__ */
