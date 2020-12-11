/*
 * Copyright (c) 2020, VeriSilicon Holdings Co., Ltd. All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __VPI_H__
#define __VPI_H__

#include "time.h"
#include "vpi_types.h"

/**
  * Chromaticity coordinates of the source primaries.
  * These values match the ones defined by ISO/IEC 23001-8_2013 § 7.1.
  */
enum VPIColorPrimaries {
    VPICOL_PRI_RESERVED0   = 0,
    VPICOL_PRI_BT709       = 1,  ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
    VPICOL_PRI_UNSPECIFIED = 2,
    VPICOL_PRI_RESERVED    = 3,
    VPICOL_PRI_BT470M      = 4,  ///< also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)

    VPICOL_PRI_BT470BG     = 5,  ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
    VPICOL_PRI_SMPTE170M   = 6,  ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
    VPICOL_PRI_SMPTE240M   = 7,  ///< functionally identical to above
    VPICOL_PRI_FILM        = 8,  ///< colour filters using Illuminant C
    VPICOL_PRI_BT2020      = 9,  ///< ITU-R BT2020
    VPICOL_PRI_SMPTE428    = 10, ///< SMPTE ST 428-1 (CIE 1931 XYZ)
    VPICOL_PRI_SMPTEST428_1 = VPICOL_PRI_SMPTE428,
    VPICOL_PRI_SMPTE431    = 11, ///< SMPTE ST 431-2 (2011) / DCI P3
    VPICOL_PRI_SMPTE432    = 12, ///< SMPTE ST 432-1 (2010) / P3 D65 / Display P3
    VPICOL_PRI_JEDEC_P22   = 22, ///< JEDEC P22 phosphors
    VPICOL_PRI_NB                ///< Not part of ABI
};

/**
 * Color Transfer Characteristic.
 * These values match the ones defined by ISO/IEC 23001-8_2013 § 7.2.
 */
enum VPIColorTransferCharacteristic {
    VPICOL_TRC_RESERVED0    = 0,
    VPICOL_TRC_BT709        = 1,  ///< also ITU-R BT1361
    VPICOL_TRC_UNSPECIFIED  = 2,
    VPICOL_TRC_RESERVED     = 3,
    VPICOL_TRC_GAMMA22      = 4,  ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
    VPICOL_TRC_GAMMA28      = 5,  ///< also ITU-R BT470BG
    VPICOL_TRC_SMPTE170M    = 6,  ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
    VPICOL_TRC_SMPTE240M    = 7,
    VPICOL_TRC_LINEAR       = 8,  ///< "Linear transfer characteristics"
    VPICOL_TRC_LOG          = 9,  ///< "Logarithmic transfer characteristic (100:1 range)"
    VPICOL_TRC_LOG_SQRT     = 10, ///< "Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
    VPICOL_TRC_IEC61966_2_4 = 11, ///< IEC 61966-2-4
    VPICOL_TRC_BT1361_ECG   = 12, ///< ITU-R BT1361 Extended Colour Gamut
    VPICOL_TRC_IEC61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
    VPICOL_TRC_BT2020_10    = 14, ///< ITU-R BT2020 for 10-bit system
    VPICOL_TRC_BT2020_12    = 15, ///< ITU-R BT2020 for 12-bit system
    VPICOL_TRC_SMPTE2084    = 16, ///< SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems
    VPICOL_TRC_SMPTEST2084  = VPICOL_TRC_SMPTE2084,
    VPICOL_TRC_SMPTE428     = 17, ///< SMPTE ST 428-1
    VPICOL_TRC_SMPTEST428_1 = VPICOL_TRC_SMPTE428,
    VPICOL_TRC_ARIB_STD_B67 = 18, ///< ARIB STD-B67, known as "Hybrid log-gamma"
    VPICOL_TRC_NB                 ///< Not part of ABI
};

/**
 * YUV colorspace type.
 * These values match the ones defined by ISO/IEC 23001-8_2013 § 7.3.
 */
enum VPIColorSpace {
    VPICOL_SPC_RGB         = 0,  ///< order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB)
    VPICOL_SPC_BT709       = 1,  ///< also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
    VPICOL_SPC_UNSPECIFIED = 2,
    VPICOL_SPC_RESERVED    = 3,
    VPICOL_SPC_FCC         = 4,  ///< FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
    VPICOL_SPC_BT470BG     = 5,  ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601
    VPICOL_SPC_SMPTE170M   = 6,  ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
    VPICOL_SPC_SMPTE240M   = 7,  ///< functionally identical to above
    VPICOL_SPC_YCGCO       = 8,  ///< Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
    VPICOL_SPC_YCOCG       = VPICOL_SPC_YCGCO,
    VPICOL_SPC_BT2020_NCL  = 9,  ///< ITU-R BT2020 non-constant luminance system
    VPICOL_SPC_BT2020_CL   = 10, ///< ITU-R BT2020 constant luminance system
    VPICOL_SPC_SMPTE2085   = 11, ///< SMPTE 2085, Y'D'zD'x
    VPICOL_SPC_CHROMA_DERIVED_NCL = 12, ///< Chromaticity-derived non-constant luminance system
    VPICOL_SPC_CHROMA_DERIVED_CL = 13, ///< Chromaticity-derived constant luminance system
    VPICOL_SPC_ICTCP       = 14, ///< ITU-R BT.2100-0, ICtCp
    VPICOL_SPC_NB                ///< Not part of ABI
};

typedef struct VpiBufRef VpiBufRef;

typedef struct VpeVpiCtx {
    uint32_t dummy;
    VpiPlugin plugin;
    void *ctx;
    int fd;
} VpeVpiCtx;

typedef struct VpiHwCtx {
    char *device_name;
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
    int fd;
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

typedef struct VpiDevCtx {
    char device_name[32];
    int fd;
} VpiDevCtx;
#endif
