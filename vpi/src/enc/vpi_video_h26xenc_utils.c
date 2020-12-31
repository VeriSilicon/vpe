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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <io.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#endif
#include "error.h"
#include "encasiccontroller.h"
#include "instance.h"
#include "vpi_video_h26xenc.h"
#include "vpi.h"
#include "hugepage_api.h"
#ifdef INTERNAL_TEST
#include "sw_test_id.h"
#endif
#ifdef TEST_DATA
#include "enctrace.h"
#endif
#include "tools.h"

#ifdef SUPPORT_TCACHE
#include "dtrc_api.h"
#endif

#include "vpi_log.h"
#include "vpi_video_h26xenc_options.h"
#include "vpi_video_h26xenc_utils.h"

#define MAX_LINE_LENGTH_BLOCK 512 * 8
#define ENC_TB_INFO_PRINT(fmt, ...)                                            \
    do {                                                                       \
    } while (0)
#define ENC_TB_ERROR_PRINT(fmt, ...)                                           \
    do {                                                                       \
    } while (0)
#define ENC_TB_DEBUG_PRINT(fmt, ...)                                           \
    do {                                                                       \
    } while (0)
#define ENC_TB_DEBUGV_PRINT(fmt, ...)                                          \
    do {                                                                       \
    } while (0)
/*Type POC QPoffset  QPfactor  num_ref_pics ref_pics  used_by_cur*/
char *RpsDefaultGOPSize1[] = {
    "Frame1:  P    1   0        0.578     0      1        -1         1",
    NULL,
};

char *RpsDefaultH264GOPSize1[] = {
    "Frame1:  P    1   0        0.4     0      1        -1         1",
    NULL,
};

char *RpsDefaultGOPSize2[] = {
    "Frame1:  P    2   0        0.6     0      1        -2         1",
    "Frame2:  B    1   0        0.68    0      2        -1 1       1 1",
    NULL,
};

char *RpsDefaultGOPSize3[] = {
    "Frame1:  P    3   0        0.5     0      1        -3         1   ",
    "Frame2:  B    1   0        0.5     0      2        -1 2       1 1 ",
    "Frame3:  B    2   0        0.68    0      2        -1 1       1 1 ",
    NULL,
};

char *RpsDefaultGOPSize4[] = {
    "Frame1:  P    4   0        0.5      0     1       -4         1 ",
    "Frame2:  B    2   0        0.3536   0     2       -2 2       1 1",
    "Frame3:  B    1   0        0.5      0     3       -1 1 3     1 1 0",
    "Frame4:  B    3   0        0.5      0     2       -1 1       1 1 ",
    NULL,
};

char *RpsDefaultGOPSize5[] = {
    "Frame1:  P    5   0        0.442    0     1       -5         1 ",
    "Frame2:  B    2   0        0.3536   0     2       -2 3       1 1",
    "Frame3:  B    1   0        0.68     0     3       -1 1 4     1 1 0",
    "Frame4:  B    3   0        0.3536   0     2       -1 2       1 1 ",
    "Frame5:  B    4   0        0.68     0     2       -1 1       1 1 ",
    NULL,
};

char *RpsDefaultGOPSize6[] = {
    "Frame1:  P    6   0        0.442    0     1       -6         1 ",
    "Frame2:  B    3   0        0.3536   0     2       -3 3       1 1",
    "Frame3:  B    1   0        0.3536   0     3       -1 2 5     1 1 0",
    "Frame4:  B    2   0        0.68     0     3       -1 1 4     1 1 0",
    "Frame5:  B    4   0        0.3536   0     2       -1 2       1 1 ",
    "Frame6:  B    5   0        0.68     0     2       -1 1       1 1 ",
    NULL,
};

char *RpsDefaultGOPSize7[] = {
    "Frame1:  P    7   0        0.442    0     1       -7         1 ",
    "Frame2:  B    3   0        0.3536   0     2       -3 4       1 1",
    "Frame3:  B    1   0        0.3536   0     3       -1 2 6     1 1 0",
    "Frame4:  B    2   0        0.68     0     3       -1 1 5     1 1 0",
    "Frame5:  B    5   0        0.3536   0     2       -2 2       1 1 ",
    "Frame6:  B    4   0        0.68     0     3       -1 1 3     1 1 0",
    "Frame7:  B    6   0        0.68     0     2       -1 1       1 1 ",
    NULL,
};

char *RpsDefaultGOPSize8[] = {
    "Frame1:  P    8   0        0.442    0  1           -8        1 ",
    "Frame2:  B    4   0        0.3536   0  2           -4 4      1 1 ",
    "Frame3:  B    2   0        0.3536   0  3           -2 2 6    1 1 0 ",
    "Frame4:  B    1   0        0.68     0  4           -1 1 3 7  1 1 0 0",
    "Frame5:  B    3   0        0.68     0  3           -1 1 5    1 1 0",
    "Frame6:  B    6   0        0.3536   0  2           -2 2      1 1",
    "Frame7:  B    5   0        0.68     0  3           -1 1 3    1 1 0",
    "Frame8:  B    7   0        0.68     0  2           -1 1      1 1",
    NULL,
};

char *RpsDefaultInterlaceGOPSize1[] = {
    "Frame1:  P    1   0        0.8       0   2           -1 -2     0 1",
    NULL,
};

char *RpsLowdelayDefaultGOPSize1[] = {
    "Frame1:  B    1   0        0.65      0     2       -1 -2         1 1",
    NULL,
};

char *RpsLowdelayDefaultGOPSize2[] = {
    "Frame1:  B    1   0        0.4624    0     2       -1 -3         1 1",
    "Frame2:  B    2   0        0.578     0     2       -1 -2         1 1",
    NULL,
};

char *RpsLowdelayDefaultGOPSize3[] = {
    "Frame1:  B    1   0        0.4624    0     2       -1 -4         1 1",
    "Frame2:  B    2   0        0.4624    0     2       -1 -2         1 1",
    "Frame3:  B    3   0        0.578     0     2       -1 -3         1 1",
    NULL,
};

char *RpsLowdelayDefaultGOPSize4[] = {
    "Frame1:  B    1   0        0.4624    0     2       -1 -5         1 1",
    "Frame2:  B    2   0        0.4624    0     2       -1 -2         1 1",
    "Frame3:  B    3   0        0.4624    0     2       -1 -3         1 1",
    "Frame4:  B    4   0        0.578     0     2       -1 -4         1 1",
    NULL,
};

char *RpsPass2GOPSize4[] = {
    "Frame1:  B    4   0        0.5      0     2       -4 -8      1 1",
    "Frame2:  B    2   0        0.3536   0     2       -2 2       1 1",
    "Frame3:  B    1   0        0.5      0     3       -1 1 3     1 1 0",
    "Frame4:  B    3   0        0.5      0     3       -1 -3 1    1 0 1",
    NULL,
};

char *RpsPass2GOPSize8[] = {
    "Frame1:  B    8   0        0.442    0  2           -8 -16    1 1",
    "Frame2:  B    4   0        0.3536   0  2           -4 4      1 1",
    "Frame3:  B    2   0        0.3536   0  3           -2 2 6    1 1 0",
    "Frame4:  B    1   0        0.68     0  4           -1 1 3 7  1 1 0 0",
    "Frame5:  B    3   0        0.68     0  4           -1 -3 1 5 1 0 1 0",
    "Frame6:  B    6   0        0.3536   0  3           -2 -6 2   1 0 1",
    "Frame7:  B    5   0        0.68     0  4           -1 -5 1 3 1 0 1 0",
    "Frame8:  B    7   0        0.68     0  3           -1 -7 1   1 0 1",
    NULL,
};

char *RpsPass2GOPSize2[] = {
    "Frame1:  B    2   0        0.6     0      2        -2 -4      1 1",
    "Frame2:  B    1   0        0.68    0      2        -1 1       1 1",
    NULL,
};

FILE *open_file(char *name, char *mode)
{
    FILE *fp;

    if (!(fp = fopen(name, mode))) {
        /*Error(4, ERR, name, ", ", SYSERR);*/
    }

    return fp;
}

void h26xenc_buf_list_add(H26xEncBufLink **head, H26xEncBufLink *list)
{
    H26xEncBufLink *temp;

    if(NULL == *head) {
        *head = list;
        (*head)->next = NULL;
    } else {
        temp = *head;
        while(temp) {
            if(NULL == temp->next) {
                temp->next = list;
                list->next = NULL;
                return;
            }
            temp = temp->next;
        }
    }
}

H26xEncBufLink* h26xenc_buf_list_delete(H26xEncBufLink *head)
{
    if (NULL == head || NULL == head->next) {
        return NULL;
    }

    return head->next;
}

/**
 *WriteStrm
 *Write encoded stream to file
 *
 *@Param: fout    - file to write
 *@Param: strbuf  - data to be written
 *@Param: size    - amount of data to write
 *@Param: endian  - data endianess, big or little
 */
void h26x_enc_write_strm(FILE *fout, u32 *strmbuf, u32 size, u32 endian)
{
#ifdef NO_OUTPUT_WRITE
    return;
#endif

    if (!fout || !strmbuf || !size)
        return;

    /* Swap the stream endianess before writing to file if needed */
    if (endian == 1) {
        u32 i = 0, words = (size + 3) / 4;

        while (words) {
            u32 val = strmbuf[i];
            u32 tmp = 0;

            tmp |= (val & 0xFF) << 24;
            tmp |= (val & 0xFF00) << 8;
            tmp |= (val & 0xFF0000) >> 8;
            tmp |= (val & 0xFF000000) >> 24;
            strmbuf[i] = tmp;
            words--;
            i++;
        }
    }
    ENC_TB_DEBUGV_PRINT("Write out stream size %d \n", size);
    /* Write the stream to file */
    fwrite(strmbuf, 1, size, fout);
}

void get_stream_bufs(VCEncStrmBufs *bufs, VPIH26xEncCfg *tb,
                     VPIH26xEncOptions *options, bool encoding)
{
    i32 i;
    for (i = 0; i < MAX_OUT_BUF_NUM; i++) {
#ifdef USE_OLD_DRV
        bufs->buf[i] =
            tb->outbuf_mem[i] ? (u8 *)tb->outbuf_mem[i]->virtualAddress : NULL;
#else
        bufs->buf[i] = tb->outbuf_mem[i] ?
                           (u8 *)tb->outbuf_mem[i]->rc_virtualAddress :
                           NULL;
#endif
        bufs->bufLen[i] = tb->outbuf_mem[i] ? tb->outbuf_mem[i]->size : 0;
    }

#ifdef INTERNAL_TEST
    if (encoding && (options->test_id == TID_INT) && options->streamBufChain &&
        (options->parallel_core_num <= 1))
        HevcStreamBufferLimitTest(NULL, bufs);
#endif
}

static void write_flags_2_memory(char flag, u8 *memory, u16 column, u16 row,
                                 u16 blockunit, u16 width, u16 ctb_size,
                                 u32 ctb_per_row, u32 ctb_per_column)
{
    u32 blks_per_ctb      = ctb_size / 8;
    u32 blks_per_unit     = 1 << (3 - blockunit);
    u32 ctb_row_number    = row * blks_per_unit / blks_per_ctb;
    u32 ctb_column_number = column * blks_per_unit / blks_per_ctb;
    u32 ctb_row_stride    = ctb_per_row * blks_per_ctb * blks_per_ctb;
    u32 xoffset           = (column * blks_per_unit) % blks_per_ctb;
    u32 yoffset           = (row * blks_per_unit) % blks_per_ctb;
    u32 stride            = blks_per_ctb;
    u32 columns, rows, r, c;

    rows = columns = blks_per_unit;
    if (blks_per_ctb < blks_per_unit) {
        rows = MIN(rows, ctb_per_column * blks_per_ctb - row * blks_per_unit);
        columns =
            MIN(columns, ctb_per_row * blks_per_ctb - column * blks_per_unit);
        rows /= blks_per_ctb;
        columns *= blks_per_ctb;
        stride = ctb_row_stride;
    }
    /* ctb addr --> blk addr*/
    memory += ctb_row_number * ctb_row_stride +
              ctb_column_number * (blks_per_ctb * blks_per_ctb);
    memory += yoffset * stride + xoffset;
    for (r = 0; r < rows; r++) {
        u8 *dst = memory + r * stride;
        u8 val;
        for (c = 0; c < columns; c++) {
            val    = *dst;
            *dst++ = (val & 0x7f) | (flag << 7);
        }
    }
}

static void write_flags_row_data_2_memory(char *rowStartAddr, u8 *memory,
                                          u16 width, u16 row_number,
                                          u16 blockunit, u16 ctb_size,
                                          u32 ctb_per_row, u32 ctb_per_column)
{
    int i;
    i = 0;
    while (i < width) {
        write_flags_2_memory(*rowStartAddr, memory, i, row_number, blockunit,
                             width, ctb_size, ctb_per_row, ctb_per_column);
        rowStartAddr++;
        i++;
    }
}

static void write_qp_value_2_memory(char qpDelta, u8 *memory, u16 column,
                                    u16 row, u16 blockunit, u16 width,
                                    u16 ctb_size, u32 ctb_per_row,
                                    u32 ctb_per_column)
{
    u32 blks_per_ctb      = ctb_size / 8;
    u32 blks_per_unit     = 1 << (3 - blockunit);
    u32 ctb_row_number    = row * blks_per_unit / blks_per_ctb;
    u32 ctb_column_number = column * blks_per_unit / blks_per_ctb;
    u32 ctb_row_stride    = ctb_per_row * blks_per_ctb * blks_per_ctb;
    u32 xoffset           = (column * blks_per_unit) % blks_per_ctb;
    u32 yoffset           = (row * blks_per_unit) % blks_per_ctb;
    u32 stride            = blks_per_ctb;
    u32 columns, rows, r, c;

    rows = columns = blks_per_unit;
    if (blks_per_ctb < blks_per_unit) {
        rows = MIN(rows, ctb_per_column * blks_per_ctb - row * blks_per_unit);
        columns =
            MIN(columns, ctb_per_row * blks_per_ctb - column * blks_per_unit);
        rows /= blks_per_ctb;
        columns *= blks_per_ctb;
        stride = ctb_row_stride;
    }
    /* ctb addr --> blk addr*/
    memory += ctb_row_number * ctb_row_stride +
              ctb_column_number * (blks_per_ctb * blks_per_ctb);
    memory += yoffset * stride + xoffset;
    for (r = 0; r < rows; r++) {
        u8 *dst = memory + r * stride;
        for (c = 0; c < columns; c++)
            *dst++ = qpDelta;
    }
}

static void write_qp_delta_data_2_memory(char qpDelta, u8 *memory, u16 column,
                                         u16 row, u16 blockunit, u16 width,
                                         u16 ctb_size, u32 ctb_per_row,
                                         u32 ctb_per_column)
{
    u8 twoBlockDataCombined;
    int r, c;
    u32 blks_per_ctb   = ctb_size / 8;
    u32 blks_per_unit  = (1 << (3 - blockunit));
    u32 ctb_row_number = row * blks_per_unit / blks_per_ctb;
    u32 ctb_row_stride = ctb_per_row * (blks_per_ctb * blks_per_ctb) / 2;
    u32 ctb_row_offset = (blks_per_ctb < blks_per_unit ?
                              0 :
                              row - (row / (blks_per_ctb / blks_per_unit)) *
                                        (blks_per_ctb / blks_per_unit)) *
                         blks_per_unit;
    u32 internal_ctb_stride = blks_per_ctb / 2;
    u32 ctb_column_number;
    u8 *rowMemoryStartPtr = memory + ctb_row_number * ctb_row_stride;
    u8 *ctbMemoryStartPtr, *curMemoryStartPtr;
    u32 columns, rows;
    u32 xoffset;

    {
        ctb_column_number = column * blks_per_unit / blks_per_ctb;
        ctbMemoryStartPtr =
            rowMemoryStartPtr +
            ctb_column_number * (blks_per_ctb * blks_per_ctb) / 2;
        switch (blockunit) {
        case 0:
            twoBlockDataCombined =
                (-qpDelta & 0X0F) | (((-qpDelta & 0X0F)) << 4);
            rows    = 8;
            columns = 4;
            xoffset = 0;
            break;
        case 1:
            twoBlockDataCombined =
                (-qpDelta & 0X0F) | (((-qpDelta & 0X0F)) << 4);
            rows    = 4;
            columns = 2;
            xoffset = column % ((ctb_size + 31) / 32);
            xoffset = xoffset << 1;
            break;
        case 2:
            twoBlockDataCombined =
                (-qpDelta & 0X0F) | (((-qpDelta & 0X0F)) << 4);
            rows    = 2;
            columns = 1;
            xoffset = column % ((ctb_size + 15) / 16);
            break;
        case 3:
            xoffset           = column >> 1;
            xoffset           = xoffset % ((ctb_size + 15) / 16);
            curMemoryStartPtr = ctbMemoryStartPtr +
                                ctb_row_offset * internal_ctb_stride + xoffset;
            twoBlockDataCombined = *curMemoryStartPtr;
            if (column % 2) {
                twoBlockDataCombined =
                    (twoBlockDataCombined & 0x0f) | (((-qpDelta & 0X0F)) << 4);
            } else {
                twoBlockDataCombined =
                    (twoBlockDataCombined & 0xf0) | (-qpDelta & 0X0F);
            }
            rows    = 1;
            columns = 1;
            break;
        default:
            rows                 = 0;
            twoBlockDataCombined = 0;
            columns              = 0;
            xoffset              = 0;
            break;
        }
        u32 stride = internal_ctb_stride;
        if (blks_per_ctb < blks_per_unit) {
            rows =
                MIN(rows, ctb_per_column * blks_per_ctb - row * blks_per_unit);
            columns =
                MIN(columns,
                    (ctb_per_row * blks_per_ctb - column * blks_per_unit) / 2);
            rows /= blks_per_ctb;
            columns *= blks_per_ctb;
            stride = ctb_row_stride;
        }
        for (r = 0; r < rows; r++) {
            curMemoryStartPtr =
                ctbMemoryStartPtr + (ctb_row_offset + r) * stride + xoffset;
            for (c = 0; c < columns; c++) {
                *curMemoryStartPtr++ = twoBlockDataCombined;
            }
        }
    }
}

static void write_qp_delta_row_data_2_memory(char *qp_delta_row_start_addr,
                                             u8 *memory, u16 width,
                                             u16 row_number, u16 blockunit,
                                             u16 ctb_size, u32 ctb_per_row,
                                             u32 ctb_per_column,
                                             i32 roi_map_version)
{
    i32 i = 0;
    while (i < width) {
        if (roi_map_version >= 1)
            write_qp_value_2_memory(*qp_delta_row_start_addr, memory, i,
                                    row_number, blockunit, width, ctb_size,
                                    ctb_per_row, ctb_per_column);
        else
            write_qp_delta_data_2_memory(*qp_delta_row_start_addr, memory, i,
                                         row_number, blockunit, width, ctb_size,
                                         ctb_per_row, ctb_per_column);

        qp_delta_row_start_addr++;
        i++;
    }
}

static i32 copy_qp_delta_2_memory(VPIH26xEncOptions *options, VCEncInst enc,
                                  VPIH26xEncCfg *vpi_h26xe_cfg,
                                  i32 roi_map_version)
{
#define MAX_LINE_LENGTH_BLOCK 512 * 8

    i32 ret = OK;
    u32 ctb_per_row =
        ((options->width + options->max_cu_size - 1) / (options->max_cu_size));
    u32 ctb_per_column =
        ((options->height + options->max_cu_size - 1) / (options->max_cu_size));
    FILE *roi_map_file = vpi_h26xe_cfg->roi_map_file;
#ifdef USE_OLD_DRV
    u8 *memory = (u8 *)vpi_h26xe_cfg->roi_map_delta_qp_mem->virtualAddress;
#else
    u8 *memory = (u8 *)vpi_h26xe_cfg->roi_map_delta_qp_mem->rc_virtualAddress;
#endif
    u16 block_unit;
    switch (options->roi_map_delta_qp_block_unit) {
    case 0:
        block_unit = 64;
        break;
    case 1:
        block_unit = 32;
        break;
    case 2:
        block_unit = 16;
        break;
    case 3:
        block_unit = 8;
        break;
    default:
        block_unit = 64;
        break;
    }
    u16 width = (((options->width + options->max_cu_size - 1) &
                  (~(options->max_cu_size - 1))) +
                 block_unit - 1) /
                block_unit;
    u16 height = (((options->height + options->max_cu_size - 1) &
                   (~(options->max_cu_size - 1))) +
                  block_unit - 1) /
                 block_unit;
    u16 blockunit = options->roi_map_delta_qp_block_unit;
    u16 ctb_size  = options->max_cu_size;
    char rowbuffer[1024];
    i32 line_idx = 0, i;
    i32 qpdelta, qptype, qpdelta_num = 0;
    char *rowbufferptr;
#ifdef CHECK_MEM_LEAK_TRANS
    char *ach_parser_buffer =
        (char *)EWLmalloc(sizeof(char) * MAX_LINE_LENGTH_BLOCK);
#else
    char *ach_parser_buffer =
        (char *)malloc(sizeof(char) * MAX_LINE_LENGTH_BLOCK);
#endif

    if (ach_parser_buffer == NULL) {
        ENC_TB_ERROR_PRINT("Qp delta config Error: fail to alloc buffer!\n");
        ret = NOK;
        goto copyEnd;
    }

    if (roi_map_file == NULL) {
        /*printf("Qp delta config: Error, Can Not Open File %s\n", fname );*/
        ret = NOK;
        goto copyEnd;
    }

    while (line_idx < height) {
        ach_parser_buffer[0] = '\0';
        /* Read one line*/
        char *line = fgets((char *)ach_parser_buffer, MAX_LINE_LENGTH_BLOCK,
                           roi_map_file);
        if (feof(roi_map_file)) {
            fseek(roi_map_file, 0L, SEEK_SET);
            if (!line)
                continue;
        }

        if (!line)
            break;
        /*handle line end*/
        char *s = strpbrk(line, "#\n");
        if (s)
            *s = '\0';

        if (line) {
            i            = 0;
            rowbufferptr = rowbuffer;
            memset(rowbufferptr, 0, 1024);
            while (i < width) {
                /* read data from file*/
                if (roi_map_version) {
                    /*format: a%d: absolute QP; %d: QP delta*/
                    qptype = 0;
                    if (*line == 'a') {
                        qptype = 1;
                        line++;
                    }
                    sscanf(line, "%d", &qpdelta);
                    if (qptype) {
                        qpdelta = CLIP3(0, 51, qpdelta);
                    } else {
                        qpdelta = CLIP3(-31, 32, qpdelta);
                        qpdelta = -qpdelta;
                    }
                    /* setup the map value */
                    qpdelta &= 0x3f;
                    if (roi_map_version == 1) {
                        qpdelta = (qpdelta << 1) | qptype;
                    } else if (roi_map_version == 2) {
                        qpdelta |= (qptype ? 0 : 0x40);
                    }
                } else {
                    sscanf(line, "%d", &qpdelta);
                    qpdelta = CLIP3(-15, 0, qpdelta);
                }

                /* get qpdelta*/
                *(rowbufferptr++) = (char)qpdelta;
                i++;
                qpdelta_num++;

                /* find next qpdelta*/
                line = strchr(line, ',');
                if (line) {
                    while ((*line == ',') || (*line == ' '))
                        line++;
                    if (*line == '\0')
                        break;
                } else
                    break;
            }
            write_qp_delta_row_data_2_memory(rowbuffer, memory, width, line_idx,
                                             blockunit, ctb_size, ctb_per_row,
                                             ctb_per_column, roi_map_version);
            line_idx++;
        }
    }
    /*fclose(f_in);*/
    if (qpdelta_num != width * height) {
        ENC_TB_ERROR_PRINT("QP delta Config: Error, Parsing File Failed\n");
        ret = NOK;
    }

copyEnd:
#ifndef USE_OLD_DRV
    if (EWLTransDataRC2EP(vpi_h26xe_cfg->ewl,
                          vpi_h26xe_cfg->roi_map_delta_qp_mem,
                          vpi_h26xe_cfg->roi_map_delta_qp_mem,
                          vpi_h26xe_cfg->roi_map_delta_qp_mem->size))
        ret = NOK;
#endif
    if (ach_parser_buffer) {
#ifdef CHECK_MEM_LEAK_TRANS
        EWLfree(ach_parser_buffer);
#else
        free(ach_parser_buffer);
#endif
    }
    return ret;
}

static int copy_flags_map_2_memory(VPIH26xEncOptions *options, VCEncInst enc,
                                   VPIH26xEncCfg *vpi_h26xe_cfg)
{
    u32 ctb_per_row =
        ((options->width + options->max_cu_size - 1) / (options->max_cu_size));
    u32 ctb_per_column =
        ((options->height + options->max_cu_size - 1) / (options->max_cu_size));
#ifdef USE_OLD_DRV
    u8 *memory = (u8 *)vpi_h26xe_cfg->roi_map_delta_qp_mem->virtualAddress;
#else
    u8 *memory = (u8 *)vpi_h26xe_cfg->roi_map_delta_qp_mem->rc_virtualAddress;
#endif
    u16 ctb_size = options->max_cu_size;
    u16 blockunit;
    char ach_parser_buffer[MAX_LINE_LENGTH_BLOCK];
    char rowbuffer[MAX_LINE_LENGTH_BLOCK];
    int line_idx = 0, i;
    int flag, flag_num = 0;
    char *rowbufferptr;
    u16 width, height, block_unit_size;
    i32 roi_map_version =
        ((struct vcenc_instance *)enc)->asic.regs.asicCfg.roiMapVersion;
    FILE *mapFile = NULL;

    if (roi_map_version == 3)
        roi_map_version = options->roi_qp_delta_ver;

    if (roi_map_version == 1) {
        mapFile   = vpi_h26xe_cfg->ipcm_map_file;
        blockunit = (ctb_size == 64 ? 0 : 2);
    } else if (roi_map_version == 2) {
        i32 blockUnitMax = IS_H264(options->codec_format) ? 2 : 1;
        mapFile          = vpi_h26xe_cfg->skip_map_file;
        blockunit        = options->skip_map_block_unit;
        if (blockunit > blockUnitMax) {
            ENC_TB_DEBUG_PRINT(
                "SKIP Map Config: Error, Block size too small, changed to %dx%d\n",
                8 << (3 - blockUnitMax), 8 << (3 - blockUnitMax));
            blockunit = blockUnitMax;
        }
    }

    if (mapFile == NULL) {
        /*printf("Qp delta config: Error, Can Not Open File %s\n", fname );*/
        return -1;
    }

    block_unit_size = 8 << (3 - blockunit);
    width           = (((options->width + options->max_cu_size - 1) &
              (~(options->max_cu_size - 1))) +
             block_unit_size - 1) /
            block_unit_size;
    height = (((options->height + options->max_cu_size - 1) &
               (~(options->max_cu_size - 1))) +
              block_unit_size - 1) /
             block_unit_size;

    if (!vpi_h26xe_cfg->roi_map_file) {
        i32 block_size = ((options->width + options->max_cu_size - 1) &
                          (~(options->max_cu_size - 1))) *
                         ((options->height + options->max_cu_size - 1) &
                          (~(options->max_cu_size - 1))) /
                         (8 * 8 * 2);
        if (roi_map_version)
            block_size *= 2;
#ifdef USE_OLD_DRV
        memset(vpi_h26xe_cfg->roi_map_delta_qp_mem->virtualAddress, 0,
               block_size);
#else
        memset(vpi_h26xe_cfg->roi_map_delta_qp_mem->rc_virtualAddress, 0,
               block_size);
        if (EWLTransDataRC2EP(vpi_h26xe_cfg->ewl,
                              vpi_h26xe_cfg->roi_map_delta_qp_mem,
                              vpi_h26xe_cfg->roi_map_delta_qp_mem,
                              vpi_h26xe_cfg->roi_map_delta_qp_mem->size))
            return -1;
#endif
    }

    while (line_idx < height) {
        ach_parser_buffer[0] = '\0';
        /* Read one line*/
        char *line =
            fgets((char *)ach_parser_buffer, MAX_LINE_LENGTH_BLOCK, mapFile);
        if (feof(mapFile)) {
            fseek(mapFile, 0L, SEEK_SET);
            if (!line)
                continue;
        }

        if (!line)
            break;
        /*handle line end*/
        char *s = strpbrk(line, "#\n");
        if (s)
            *s = '\0';

        if (line) {
            i            = 0;
            rowbufferptr = rowbuffer;
            memset(rowbufferptr, 0, MAX_LINE_LENGTH_BLOCK);

            while (i < width) {
                sscanf(line, "%d", &flag);
                if (flag < 0 || flag > 1) {
                    ENC_TB_ERROR_PRINT("Invalid IPCM/SKIP map entry.\n");
                    return -1;
                }
                *(rowbufferptr++) = (char)flag;
                i++;
                flag_num++;
                line = strchr(line, ',');
                if (line) {
                    while (*line == ',')
                        line++;
                    if (*line == '\0')
                        break;
                } else
                    break;
            }
            write_flags_row_data_2_memory(rowbuffer, memory, width, line_idx,
                                          blockunit, ctb_size, ctb_per_row,
                                          ctb_per_column);
            line_idx++;
        }
    }

    if (flag_num != width * height) {
        ENC_TB_ERROR_PRINT(
            "IPCM/SKIP Map Config: Error, Parsing File Failed\n");
        return -1;
    }

    return 0;
}

float get_pixel_width_inbyte(VCEncPictureType type)
{
    switch (type) {
    case VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR:
        return 1.25;
    case VCENC_YUV420_10BIT_PACKED_Y0L2:
        return 2;
    case VCENC_YUV420_PLANAR_10BIT_I010:
    case VCENC_YUV420_PLANAR_10BIT_P010:
        return 2;
    case VCENC_YUV420_PLANAR:
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU:
    case VCENC_YUV422_INTERLEAVED_YUYV:
    case VCENC_YUV422_INTERLEAVED_UYVY:
    case VCENC_RGB565:
    case VCENC_BGR565:
    case VCENC_RGB555:
    case VCENC_BGR555:
    case VCENC_RGB444:
    case VCENC_BGR444:
    case VCENC_RGB888:
    case VCENC_BGR888:
    case VCENC_RGB101010:
    case VCENC_BGR101010:
    case VCENC_YUV420_SEMIPLANAR_101010:
        return 1;
    default:
        return 1;
    }
}

void get_aligned_pic_size_byformat(i32 type, u32 width, u32 height,
                                   u32 alignment, u32 *luma_Size,
                                   u32 *p_chroma_size, u32 *p_picture_size)
{
    u32 luma_stride = 0, chroma_stride = 0;
    u32 luma_size = 0, chroma_size = 0, picture_size = 0;

    VCEncGetAlignedStride(width, type, &luma_stride, &chroma_stride, alignment);
    switch (type) {
    case VCENC_YUV420_PLANAR:
        luma_size   = luma_stride * height;
        chroma_size = chroma_stride * height / 2 * 2;
        break;
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU:
        luma_size   = luma_stride * height;
        chroma_size = chroma_stride * height / 2;
        break;
    case VCENC_YUV422_INTERLEAVED_YUYV:
    case VCENC_YUV422_INTERLEAVED_UYVY:
    case VCENC_RGB565:
    case VCENC_BGR565:
    case VCENC_RGB555:
    case VCENC_BGR555:
    case VCENC_RGB444:
    case VCENC_BGR444:
    case VCENC_RGB888:
    case VCENC_BGR888:
    case VCENC_RGB101010:
    case VCENC_BGR101010:
        luma_size   = luma_stride * height;
        chroma_size = 0;
        break;
    case VCENC_YUV420_PLANAR_10BIT_I010:
        luma_size   = luma_stride * height;
        chroma_size = chroma_stride * height / 2 * 2;
        break;
    case VCENC_YUV420_PLANAR_10BIT_P010:

        luma_size   = luma_stride * height;
        chroma_size = chroma_stride * height / 2;
        break;
    case VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR:
        luma_size   = luma_stride * 10 / 8 * height;
        chroma_size = chroma_stride * 10 / 8 * height / 2 * 2;
        break;
    case VCENC_YUV420_10BIT_PACKED_Y0L2:
        luma_size   = luma_stride * 2 * 2 * height / 2;
        chroma_size = 0;
        break;
    case VCENC_YUV420_PLANAR_8BIT_DAHUA_HEVC:
        luma_size   = luma_stride * ((height + 32 - 1) & (~(32 - 1)));
        chroma_size = luma_size / 2;
        break;
    case VCENC_YUV420_PLANAR_8BIT_DAHUA_H264:
        luma_size   = luma_stride * height * 2 * 12 / 8;
        chroma_size = 0;
        break;
    case VCENC_YUV420_SEMIPLANAR_8BIT_FB:
    case VCENC_YUV420_SEMIPLANAR_VU_8BIT_FB:
        luma_size   = luma_stride * ((height + 3) / 4);
        chroma_size = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_PLANAR_10BIT_P010_FB:
        luma_size   = luma_stride * ((height + 3) / 4);
        chroma_size = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_SEMIPLANAR_101010:
        luma_size   = luma_stride * height;
        chroma_size = chroma_stride * height / 2;
        break;
    case VCENC_YUV420_8BIT_TILE_64_4:
    case VCENC_YUV420_UV_8BIT_TILE_64_4:
        luma_size   = luma_stride * ((height + 3) / 4);
        chroma_size = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_10BIT_TILE_32_4:
        luma_size   = luma_stride * ((height + 3) / 4);
        chroma_size = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_10BIT_TILE_48_4:
    case VCENC_YUV420_VU_10BIT_TILE_48_4:
        luma_size   = luma_stride * ((height + 3) / 4);
        chroma_size = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_8BIT_TILE_128_2:
    case VCENC_YUV420_UV_8BIT_TILE_128_2:
        luma_size   = luma_stride * ((height + 1) / 2);
        chroma_size = chroma_stride * (((height / 2) + 1) / 2);
        break;
    case VCENC_YUV420_10BIT_TILE_96_2:
    case VCENC_YUV420_VU_10BIT_TILE_96_2:
        luma_size   = luma_stride * ((height + 1) / 2);
        chroma_size = chroma_stride * (((height / 2) + 1) / 2);
        break;

        /* fb format */
    case INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_YUV420P:
    case INPUT_FORMAT_PP_YUV420_SEMIPLANNAR:
    case INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_VU:
        luma_stride   = STRIDE(width, alignment);
        chroma_stride = STRIDE(width, alignment);

        luma_size   = luma_stride * height;
        chroma_size = chroma_stride * height / 2;
        break;

    case INPUT_FORMAT_PP_YUV420_PLANAR_10BIT_P010:
#ifdef SUPPORT_TCACHE
    case INPUT_FORMAT_YUV420_SEMIPLANAR_10BIT_P010BE:
#endif
        luma_stride   = STRIDE(width * 2, alignment);
        chroma_stride = STRIDE(width * 2, alignment);

        luma_size   = luma_stride * height;
        chroma_size = chroma_stride * height / 2;
        break;

#ifdef SUPPORT_DEC400
    case INPUT_FORMAT_YUV420_SEMIPLANAR_8BIT_COMPRESSED_FB:
    case INPUT_FORMAT_YUV420_SEMIPLANAR_VU_8BIT_COMPRESSED_FB:
        luma_stride   = STRIDE(width * 4, alignment);
        chroma_stride = STRIDE(width * 4, alignment);

        luma_size   = luma_stride * ((height + 3) / 4);
        chroma_size = chroma_stride * (((height / 2) + 3) / 4);
        break;

    case INPUT_FORMAT_YUV420_PLANAR_10BIT_P010_COMPRESSED_FB:
        luma_stride   = STRIDE(width * 4 * 2, alignment);
        chroma_stride = STRIDE(width * 4 * 2, alignment);

        luma_size   = luma_stride * ((height + 3) / 4);
        chroma_size = chroma_stride * (((height / 2) + 3) / 4);
        break;
#endif

#ifdef SUPPORT_TCACHE

    case INPUT_FORMAT_RFC_8BIT_COMPRESSED_FB:
        luma_stride = STRIDE(width, DTRC_INPUT_WIDTH_ALIGNMENT);
        luma_size   = luma_stride * STRIDE(height, DTRC_INPUT_HEIGHT_ALIGNMENT);
        /*luma_size = luma_stride * ((height+3)/4);*/
        chroma_size = luma_size / 2;
        break;
    case INPUT_FORMAT_RFC_10BIT_COMPRESSED_FB:
        luma_stride = STRIDE(width, DTRC_INPUT_WIDTH_ALIGNMENT);
        luma_size =
            luma_stride * STRIDE(height, DTRC_INPUT_HEIGHT_ALIGNMENT) * 10 / 8;
        chroma_size = luma_size / 2;
        break;
    case INPUT_FORMAT_YUV420_PLANAR_10BIT_P010LE:
    case INPUT_FORMAT_YUV420_PLANAR_10BIT_P010BE:
        luma_stride   = STRIDE(width * 2, alignment);
        chroma_stride = STRIDE((width / 2) * 2, alignment);
        luma_size     = luma_stride * height;
        chroma_size   = chroma_stride * (height / 2) * 2;
        break;
    case INPUT_FORMAT_ARGB_FB:
    case INPUT_FORMAT_ABGR_FB:
    case INPUT_FORMAT_RGBA_FB:
    case INPUT_FORMAT_BGRA_FB:
        luma_stride = STRIDE(width * 4, alignment);
        luma_size   = luma_stride * height;
        chroma_size = 0;
        break;
    case INPUT_FORMAT_RGB24_FB:
    case INPUT_FORMAT_BGR24_FB:
        luma_stride = STRIDE(width * 3, alignment);
        luma_size   = luma_stride * height;
        chroma_size = 0;
        break;
    case INPUT_FORMAT_YUV422P:
        luma_stride   = STRIDE(width, alignment);
        chroma_stride = STRIDE(width / 2, alignment);
        luma_size     = luma_stride * height;
        chroma_size   = chroma_stride * height * 2;
        break;
    case INPUT_FORMAT_YUV422P10LE:
    case INPUT_FORMAT_YUV422P10BE:
        luma_stride   = STRIDE(width * 2, alignment);
        chroma_stride = STRIDE((width / 2) * 2, alignment);
        luma_size     = luma_stride * height;
        chroma_size   = chroma_stride * height * 2;
        break;
    case INPUT_FORMAT_YUV444P:
        luma_stride = STRIDE(width * 2, alignment);
        luma_size   = luma_stride * height;
        chroma_size = luma_size * 2;
        break;

#endif

    default:
        VPILOGD("not support this format\n");
        chroma_size = luma_size = 0;
        break;
    }

    picture_size = luma_size + chroma_size;
    if (luma_Size != NULL)
        *luma_Size = luma_size;
    if (p_chroma_size != NULL)
        *p_chroma_size = chroma_size;
    if (p_picture_size != NULL)
        *p_picture_size = picture_size;
}

void change_to_customized_format(VPIH26xEncOptions *options,
                                 VCEncPreProcessingCfg *pre_proc_cfg)
{
    if ((options->format_customized_type == 0) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        if (IS_HEVC(options->codec_format))
            pre_proc_cfg->inputType = VCENC_YUV420_PLANAR_8BIT_DAHUA_HEVC;
        else
            pre_proc_cfg->inputType = VCENC_YUV420_PLANAR_8BIT_DAHUA_H264;
        pre_proc_cfg->origWidth =
            ((pre_proc_cfg->origWidth + 16 - 1) & (~(16 - 1)));
    }

    if ((options->format_customized_type == 1) &&
        ((options->input_format == VCENC_YUV420_SEMIPLANAR) ||
         (options->input_format == VCENC_YUV420_SEMIPLANAR_VU) ||
         (options->input_format == VCENC_YUV420_PLANAR_10BIT_P010))) {
        if (options->input_format == VCENC_YUV420_SEMIPLANAR)
            pre_proc_cfg->inputType = VCENC_YUV420_SEMIPLANAR_8BIT_FB;
        else if (options->input_format == VCENC_YUV420_SEMIPLANAR_VU)
            pre_proc_cfg->inputType = VCENC_YUV420_SEMIPLANAR_VU_8BIT_FB;
        else
            pre_proc_cfg->inputType = VCENC_YUV420_PLANAR_10BIT_P010_FB;
    }

    if ((options->format_customized_type == 2) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        pre_proc_cfg->inputType = VCENC_YUV420_SEMIPLANAR_101010;
        pre_proc_cfg->xOffset   = pre_proc_cfg->xOffset / 6 * 6;
    }

    if ((options->format_customized_type == 3) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        pre_proc_cfg->inputType = VCENC_YUV420_8BIT_TILE_64_4;
        pre_proc_cfg->xOffset   = pre_proc_cfg->xOffset / 64 * 64;
        pre_proc_cfg->yOffset   = pre_proc_cfg->yOffset / 4 * 4;
    }

    if ((options->format_customized_type == 4) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        pre_proc_cfg->inputType = VCENC_YUV420_UV_8BIT_TILE_64_4;
        pre_proc_cfg->xOffset   = pre_proc_cfg->xOffset / 64 * 64;
        pre_proc_cfg->yOffset   = pre_proc_cfg->yOffset / 4 * 4;
    }

    if ((options->format_customized_type == 5) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        pre_proc_cfg->inputType = VCENC_YUV420_10BIT_TILE_32_4;
        pre_proc_cfg->xOffset   = pre_proc_cfg->xOffset / 32 * 32;
        pre_proc_cfg->yOffset   = pre_proc_cfg->yOffset / 4 * 4;
    }

    if ((options->format_customized_type == 6) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        pre_proc_cfg->inputType = VCENC_YUV420_10BIT_TILE_48_4;
        pre_proc_cfg->xOffset   = pre_proc_cfg->xOffset / 48 * 48;
        pre_proc_cfg->yOffset   = pre_proc_cfg->yOffset / 4 * 4;
    }

    if ((options->format_customized_type == 7) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        pre_proc_cfg->inputType = VCENC_YUV420_VU_10BIT_TILE_48_4;
        pre_proc_cfg->xOffset   = pre_proc_cfg->xOffset / 48 * 48;
        pre_proc_cfg->yOffset   = pre_proc_cfg->yOffset / 4 * 4;
    }

    if ((options->format_customized_type == 8) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        pre_proc_cfg->inputType = VCENC_YUV420_8BIT_TILE_128_2;
        pre_proc_cfg->xOffset   = pre_proc_cfg->xOffset / 128 * 128;
    }

    if ((options->format_customized_type == 9) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        pre_proc_cfg->inputType = VCENC_YUV420_UV_8BIT_TILE_128_2;
        pre_proc_cfg->xOffset   = pre_proc_cfg->xOffset / 128 * 128;
    }

    if ((options->format_customized_type == 10) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        pre_proc_cfg->inputType = VCENC_YUV420_10BIT_TILE_96_2;
        pre_proc_cfg->xOffset   = pre_proc_cfg->xOffset / 96 * 96;
    }

    if ((options->format_customized_type == 11) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        pre_proc_cfg->inputType = VCENC_YUV420_VU_10BIT_TILE_96_2;
        pre_proc_cfg->xOffset   = pre_proc_cfg->xOffset / 96 * 96;
    }
}

void change_cml_customized_format(VPIH26xEncOptions *options)
{
    if ((options->format_customized_type == 0) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        if (IS_H264(options->codec_format)) {
            if (options->ver_offset_src != DEFAULT)
                options->ver_offset_src = options->ver_offset_src & (~(16 - 1));
            if (options->hor_offset_src != DEFAULT)
                options->hor_offset_src = options->hor_offset_src & (~(16 - 1));
        } else {
            if (options->ver_offset_src != DEFAULT)
                options->ver_offset_src = options->ver_offset_src & (~(32 - 1));
            if (options->hor_offset_src != DEFAULT)
                options->hor_offset_src = options->hor_offset_src & (~(32 - 1));
        }
        options->width         = options->width & (~(16 - 1));
        options->height        = options->height & (~(16 - 1));
        options->scaled_width  = options->scaled_width & (~(16 - 1));
        options->scaled_height = options->scaled_height & (~(16 - 1));
    }

    if ((options->format_customized_type == 1) &&
        ((options->input_format == VCENC_YUV420_SEMIPLANAR) ||
         (options->input_format == VCENC_YUV420_SEMIPLANAR_VU) ||
         (options->input_format == VCENC_YUV420_PLANAR_10BIT_P010))) {
        options->ver_offset_src = 0;
        options->hor_offset_src = 0;
        if (options->test_id == 16)
            options->test_id = 0;

        options->rotation = 0;
    } else if (options->format_customized_type == 1) {
        options->format_customized_type = -1;
    }

    if (((options->format_customized_type >= 2) &&
         (options->format_customized_type <= 11)) &&
        (options->input_format == VCENC_YUV420_PLANAR)) {
        if (options->test_id == 16)
            options->test_id = 0;
        options->rotation = 0;
    } else
        options->format_customized_type = -1;
}

static void trans_yuv_to_fbformat(VPIH26xEncCfg *tb,
                                  VPIH26xEncOptions *options, i32 *ret)
{
    u8 *transform_buf;
    u32 x, y;
    VCEncIn *p_enc_in = &(tb->enc_in);
#ifdef USE_OLD_DRV
    transform_buf = (u8 *)tb->transform_mem->virtualAddress;
#else
    transform_buf = (u8 *)tb->transform_mem->rc_virtualAddress;
#endif
    u32 alignment      = (tb->input_alignment == 0 ? 1 : tb->input_alignment);
    u32 byte_per_compt = 0;

    if (options->input_format == VCENC_YUV420_SEMIPLANAR ||
        options->input_format == VCENC_YUV420_SEMIPLANAR_VU)
        byte_per_compt = 1;
    else if (options->input_format == VCENC_YUV420_PLANAR_10BIT_P010)
        byte_per_compt = 2;

    VPILOGD("transform YUV to FB format\n");

    if ((options->input_format == VCENC_YUV420_SEMIPLANAR) ||
        (options->input_format == VCENC_YUV420_SEMIPLANAR_VU) ||
        (options->input_format == VCENC_YUV420_PLANAR_10BIT_P010)) {
        u32 stride =
            (options->lum_width_src * 4 * byte_per_compt + alignment - 1) &
            (~(alignment - 1));

        /*luma*/
        for (x = 0; x < options->lum_width_src / 4; x++) {
            for (y = 0; y < options->lum_height_src; y++)
                memcpy(transform_buf + y % 4 * 4 * byte_per_compt +
                           stride * (y / 4) + x * 16 * byte_per_compt,
                       tb->lum +
                           y * ((options->lum_width_src + 15) & (~15)) *
                               byte_per_compt +
                           x * 4 * byte_per_compt,
                       4 * byte_per_compt);
        }

        transform_buf += stride * options->lum_height_src / 4;

        /*chroma*/
        for (x = 0; x < options->lum_width_src / 4; x++) {
            for (y = 0; y < ((options->lum_height_src / 2) + 3) / 4 * 4; y++)
                memcpy(transform_buf + y % 4 * 4 * byte_per_compt +
                           stride * (y / 4) + x * 16 * byte_per_compt,
                       tb->cb +
                           y * ((options->lum_width_src + 15) & (~15)) *
                               byte_per_compt +
                           x * 4 * byte_per_compt,
                       4 * byte_per_compt);
        }
    }

    {
        u32 size_lum =
            ((options->lum_width_src * 4 * byte_per_compt + alignment - 1) &
             (~(alignment - 1))) *
            options->lum_height_src / 4;

        p_enc_in->busLuma = tb->transform_mem->busAddress;

        p_enc_in->busChromaU = p_enc_in->busLuma + (u32)size_lum;
    }
    *ret = OK;
#ifndef USE_OLD_DRV
    if (EWLTransDataRC2EP(tb->ewl, tb->transform_mem, tb->transform_mem,
                          tb->transform_mem->size))
        *ret = NOK;
#endif
}

i32 read_config_files(VPIH26xEncCfg *vpi_h26xe_cfg,
                      VPIH26xEncOptions *options)
{
    i32 i;

    if ((options->roi_map_delta_qp_enable) &&
        (options->roi_map_delta_qp_file != NULL)) {
        vpi_h26xe_cfg->roi_map_file =
            fopen(options->roi_map_delta_qp_file, "r");
        if (vpi_h26xe_cfg->roi_map_file == NULL) {
            ENC_TB_DEBUG_PRINT(
                "ROI map Qp delta config: Error, Can Not Open File %s\n",
                options->roi_map_delta_qp_file);
            /*return -1;*/
        }
    }

    if (options->roi_map_delta_qp_enable &&
        options->roi_map_delta_qp_bin_file) {
        vpi_h26xe_cfg->roi_map_bin_file =
            fopen(options->roi_map_delta_qp_bin_file, "rb");
        if (vpi_h26xe_cfg->roi_map_bin_file == NULL) {
            ENC_TB_DEBUG_PRINT(
                "ROI map Qp delta config: Error, Can Not Open File %s\n",
                options->roi_map_delta_qp_bin_file);
        }
    }

    if ((options->ipcm_map_enable) && (options->ipcm_map_file != NULL)) {
        vpi_h26xe_cfg->ipcm_map_file = fopen(options->ipcm_map_file, "r");
        if (vpi_h26xe_cfg->ipcm_map_file == NULL) {
            ENC_TB_DEBUG_PRINT("IPCM map config: Error, Can Not Open File %s\n",
                               options->ipcm_map_file);
            /*return -1;*/
        }
    }

    for (i = 0; i < 2; i++) {
        if (options->gmv_file_name[i]) {
            vpi_h26xe_cfg->gmv_file[i] = fopen(options->gmv_file_name[i], "r");
            if (!vpi_h26xe_cfg->gmv_file[i]) {
                ENC_TB_DEBUG_PRINT("GMV config: Error, Can Not Open File %s\n",
                                   options->gmv_file_name[i]);
            }
        }
    }

    if (options->skip_map_enable && options->skip_map_file &&
        (vpi_h26xe_cfg->skip_map_file == NULL)) {
        vpi_h26xe_cfg->skip_map_file = fopen(options->skip_map_file, "r");
        if (vpi_h26xe_cfg->skip_map_file == NULL) {
            ENC_TB_DEBUG_PRINT("SKIP map config: Error, Can Not Open File %s\n",
                               options->skip_map_file);
        }
    }

    if (options->roi_map_info_bin_file &&
        (vpi_h26xe_cfg->roi_map_info_bin_file == NULL)) {
        vpi_h26xe_cfg->roi_map_info_bin_file =
            fopen(options->roi_map_info_bin_file, "rb");
        if (vpi_h26xe_cfg->roi_map_info_bin_file == NULL) {
            VPILOGE("ROI map config: Error, Can Not Open File %s\n",
                   options->roi_map_info_bin_file);
        }
    }

    if (options->roimap_cu_ctrl_info_bin_file &&
        (vpi_h26xe_cfg->roimap_cu_ctrl_info_bin_file == NULL)) {
        vpi_h26xe_cfg->roimap_cu_ctrl_info_bin_file =
            fopen(options->roimap_cu_ctrl_info_bin_file, "rb");
        if (vpi_h26xe_cfg->roimap_cu_ctrl_info_bin_file == NULL) {
            VPILOGE("ROI map cu ctrl config: Error, Can Not Open File %s\n",
                   options->roimap_cu_ctrl_info_bin_file);
        }
    }

    if (options->roimap_cu_ctrl_index_bin_file &&
        (vpi_h26xe_cfg->roimap_cu_ctrl_index_bin_file == NULL)) {
        vpi_h26xe_cfg->roimap_cu_ctrl_index_bin_file =
            fopen(options->roimap_cu_ctrl_index_bin_file, "rb");
        if (vpi_h26xe_cfg->roimap_cu_ctrl_index_bin_file == NULL) {
            VPILOGE("ROI map cu ctrl index config: Error, Can Not Open File %s\n",
                   options->roimap_cu_ctrl_index_bin_file);
        }
    }

    return 0;
}

static i32 parse_gmv_file(FILE *f_in, i16 *hor, i16 *ver, i32 list)
{
    char ach_parser_buffer[4096];
    char *buf = &(ach_parser_buffer[0]);

    if (!f_in)
        return -1;
    if (feof(f_in))
        return -1;

    /* Read one line*/
    buf[0]     = '\0';
    char *line = fgets(buf, 4096, f_in);
    if (!line)
        return -1;
    /*handle line end*/
    char *s = strpbrk(line, "#\n");
    if (s)
        *s = '\0';

    line = nextIntToken(line, hor);
    if (!line)
        return -1;

    line = nextIntToken(line, ver);
    if (!line)
        return -1;

#ifdef SEARCH_RANGE_ROW_OFFSET_TEST
    extern char gmvConfigLine[2][4096];
    memcpy(&(gmvConfigLine[list][0]), &(ach_parser_buffer[0]), 4096);
#endif

    return 0;
}

i32 read_gmv(VPIH26xEncCfg *tb, VCEncIn *p_enc_in,
             VPIH26xEncOptions *options)
{
    i16 mvx, mvy, i;

    for (i = 0; i < 2; i++) {
        if (tb->gmv_file[i]) {
            if (parse_gmv_file(tb->gmv_file[i], &mvx, &mvy, i) == 0) {
                p_enc_in->gmv[i][0] = mvx;
                p_enc_in->gmv[i][1] = mvy;
            }
        } else {
            p_enc_in->gmv[i][0] = options->gmv[i][0];
            p_enc_in->gmv[i][1] = options->gmv[i][1];
        }
    }

    return 0;
}

static i32 get_smart_opt(char *line, char *opt, i32 *val)
{
    char *p = strstr(line, opt);
    if (!p)
        return NOK;

    p = strchr(line, '=');
    if (!p)
        return NOK;

    p++;
    while (*p == ' ')
        p++;
    if (*p == '\0')
        return NOK;

    sscanf(p, "%d", val);
    return OK;
}

i32 parsing_smart_config(char *fname, VPIH26xEncOptions *options)
{
#define MAX_LINE_LENGTH 1024
    char ach_parser_buffer[MAX_LINE_LENGTH];
    FILE *f_in = fopen(fname, "r");
    if (f_in == NULL) {
        ENC_TB_ERROR_PRINT("Smart Config: Error, Can Not Open File %s\n",
                           fname);
        return -1;
    }

    while (1) {
        if (feof(f_in))
            break;

        ach_parser_buffer[0] = '\0';
        /*Read one line*/
        char *line = fgets((char *)ach_parser_buffer, MAX_LINE_LENGTH, f_in);
        if (!line)
            break;
        /*handle line end*/
        char *s = strpbrk(line, "#\n");
        if (s)
            *s = '\0';

        if (get_smart_opt(line, "smart_Mode_Enable",
                          &options->smart_mode_enable) == OK)
            continue;

        if (get_smart_opt(line, "smart_H264_Qp", &options->smart_h264_qp) == OK)
            continue;
        if (get_smart_opt(line, "smart_H264_Luma_DC_Th",
                          &options->smart_h264_lum_dc_th) == OK)
            continue;
        if (get_smart_opt(line, "smart_H264_Cb_DC_Th",
                          &options->smart_h264_cb_dc_th) == OK)
            continue;
        if (get_smart_opt(line, "smart_H264_Cr_DC_Th",
                          &options->smart_h264_cr_dc_th) == OK)
            continue;

        if (get_smart_opt(line, "smart_Hevc_Luma_Qp",
                          &options->smart_hevc_lum_qp) == OK)
            continue;
        if (get_smart_opt(line, "smart_Hevc_Chroma_Qp",
                          &options->smart_hevc_chr_qp) == OK)
            continue;

        if (get_smart_opt(line, "smart_Hevc_CU8_Luma_DC_Th",
                          &(options->smart_hevc_lum_dc_th[0])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Hevc_CU16_Luma_DC_Th",
                          &(options->smart_hevc_lum_dc_th[1])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Hevc_CU32_Luma_DC_Th",
                          &(options->smart_hevc_lum_dc_th[2])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Hevc_CU8_Chroma_DC_Th",
                          &(options->smart_hevc_chr_dc_th[0])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Hevc_CU16_Chroma_DC_Th",
                          &(options->smart_hevc_chr_dc_th[1])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Hevc_CU32_Chroma_DC_Th",
                          &(options->smart_hevc_chr_dc_th[2])) == OK)
            continue;

        if (get_smart_opt(line, "smart_Hevc_CU8_Luma_AC_Num_Th",
                          &(options->smart_hevc_lum_ac_num_th[0])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Hevc_CU16_Luma_AC_Num_Th",
                          &(options->smart_hevc_lum_ac_num_th[1])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Hevc_CU32_Luma_AC_Num_Th",
                          &(options->smart_hevc_lum_ac_num_th[2])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Hevc_CU8_Chroma_AC_Num_Th",
                          &(options->smart_hevc_chr_ac_num_th[0])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Hevc_CU16_Chroma_AC_Num_Th",
                          &(options->smart_hevc_chr_ac_num_th[1])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Hevc_CU32_Chroma_AC_Num_Th",
                          &(options->smart_hevc_chr_ac_num_th[2])) == OK)
            continue;

        if (get_smart_opt(line, "smart_Mean_Th0",
                          &(options->smart_mean_th[0])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Mean_Th1",
                          &(options->smart_mean_th[1])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Mean_Th2",
                          &(options->smart_mean_th[2])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Mean_Th3",
                          &(options->smart_mean_th[3])) == OK)
            continue;
        if (get_smart_opt(line, "smart_Pixel_Num_Cnt_Th",
                          &options->smart_pix_num_cnt_th) == OK)
            continue;
    }

    fclose(f_in);

    ENC_TB_INFO_PRINT("======== Smart Algorithm Configuration ========\n");
    ENC_TB_INFO_PRINT("smart_Mode_Enable = %d\n", options->smart_mode_enable);

    ENC_TB_INFO_PRINT("smart_H264_Qp = %d\n", options->smart_h264_qp);
    ENC_TB_INFO_PRINT("smart_H264_Luma_DC_Th = %d\n",
                      options->smart_h264_lum_dc_th);
    ENC_TB_INFO_PRINT("smart_H264_Cb_DC_Th = %d\n",
                      options->smart_h264_cb_dc_th);
    ENC_TB_INFO_PRINT("smart_H264_Cr_DC_Th = %d\n",
                      options->smart_h264_cr_dc_th);

    ENC_TB_INFO_PRINT("smart_Hevc_Luma_Qp = %d\n", options->smart_hevc_lum_qp);
    ENC_TB_INFO_PRINT("smart_Hevc_Chroma_Qp = %d\n",
                      options->smart_hevc_chr_qp);

    ENC_TB_INFO_PRINT("smart_Hevc_CU8_Luma_DC_Th = %d\n",
                      (options->smart_hevc_lum_dc_th[0]));
    ENC_TB_INFO_PRINT("smart_Hevc_CU16_Luma_DC_Th = %d\n",
                      (options->smart_hevc_lum_dc_th[1]));
    ENC_TB_INFO_PRINT("smart_Hevc_CU32_Luma_DC_Th = %d\n",
                      (options->smart_hevc_lum_dc_th[2]));
    ENC_TB_INFO_PRINT("smart_Hevc_CU8_Chroma_DC_Th = %d\n",
                      (options->smart_hevc_chr_dc_th[0]));
    ENC_TB_INFO_PRINT("smart_Hevc_CU16_Chroma_DC_Th = %d\n",
                      (options->smart_hevc_chr_dc_th[1]));
    ENC_TB_INFO_PRINT("smart_Hevc_CU32_Chroma_DC_Th = %d\n",
                      (options->smart_hevc_chr_dc_th[2]));

    ENC_TB_INFO_PRINT("smart_Hevc_CU8_Luma_AC_Num_Th = %d\n",
                      (options->smart_hevc_lum_ac_num_th[0]));
    ENC_TB_INFO_PRINT("smart_Hevc_CU16_Luma_AC_Num_Th = %d\n",
                      (options->smart_hevc_lum_ac_num_th[1]));
    ENC_TB_INFO_PRINT("smart_Hevc_CU32_Luma_AC_Num_Th = %d\n",
                      (options->smart_hevc_lum_ac_num_th[2]));
    ENC_TB_INFO_PRINT("smart_Hevc_CU8_Chroma_AC_Num_Th = %d\n",
                      (options->smart_hevc_chr_ac_num_th[0]));
    ENC_TB_INFO_PRINT("smart_Hevc_CU16_Chroma_AC_Num_Th = %d\n",
                      (options->smart_hevc_chr_ac_num_th[1]));
    ENC_TB_INFO_PRINT("smart_Hevc_CU32_Chroma_AC_Num_Th = %d\n",
                      (options->smart_hevc_chr_ac_num_th[2]));

    ENC_TB_INFO_PRINT("smart_Mean_Th0 = %d\n", (options->smart_mean_th[0]));
    ENC_TB_INFO_PRINT("smart_Mean_Th1 = %d\n", (options->smart_mean_th[1]));
    ENC_TB_INFO_PRINT("smart_Mean_Th2 = %d\n", (options->smart_mean_th[2]));
    ENC_TB_INFO_PRINT("smart_Mean_Th3 = %d\n", (options->smart_mean_th[3]));
    ENC_TB_INFO_PRINT("smart_Pixel_Num_Cnt_Th = %d\n",
                      options->smart_pix_num_cnt_th);
    ENC_TB_INFO_PRINT("===============================================\n");

    return 0;
}

static i32 file_read(FILE *file, u8 *data, u64 seek, size_t size)
{
    if ((file == NULL) || (data == NULL))
        return NOK;

    fseeko(file, seek, SEEK_SET);
    if (fread(data, sizeof(u8), size, file) < size) {
        if (!feof(file)) {
            /*Error(2, ERR, SYSERR);*/
        }
        return NOK;
    }

    return OK;
}

/**
 *h26x_enc_next_picture
 *h26x_enc_next_picture calculates next input picture depending input and output
 *frame rates.
 */
u64 h26x_enc_next_picture(VPIH26xEncCfg *tb, int picture_cnt)
{
    u64 numer, denom;

    numer = (u64)tb->input_rate_numer * (u64)tb->output_rate_denom;
    denom = (u64)tb->input_rate_denom * (u64)tb->output_rate_numer;

    return numer * (picture_cnt / (1 << tb->interlaced_frame)) / denom;
}

i32 read_picture(VPIH26xEncCfg *vpi_h26xe_cfg, u32 inputFormat,
                 u32 src_img_size, u32 src_width, u32 src_height)
{
    i32 num;
    u64 seek;
    i32 w;
    i32 h;
    u8 *lum;
    u8 *cb;
    u8 *cr;
    i32 i;
    u32 luma_stride;
    u32 chroma_stride;
    u32 alignment;
    u32 src_img_size_ds = vpi_h26xe_cfg->src_img_size_ds;
    u32 src_width_ds    = src_width / 2;
    u32 src_height_ds   = src_height / 2;
    u8 *lum_ds;
    u8 *cb_ds;
    u8 *cr_ds;
    u64 seek_ds;
    u32 luma_stride_ds;
    u32 chroma_stride_ds;

    num = h26x_enc_next_picture(vpi_h26xe_cfg, vpi_h26xe_cfg->enc_in.picture_cnt) +
          vpi_h26xe_cfg->first_pic;

    if (num > vpi_h26xe_cfg->last_pic)
        return NOK;
    ENC_TB_DEBUGV_PRINT("src_img_size = %d, num = %d\n", src_img_size, num);
    seek      = ((u64)num) * ((u64)src_img_size);
    seek_ds   = ((u64)num) * ((u64)src_img_size_ds);
    lum       = vpi_h26xe_cfg->lum;
    cb        = vpi_h26xe_cfg->cb;
    cr        = vpi_h26xe_cfg->cr;
    lum_ds    = vpi_h26xe_cfg->lum_ds;
    cb_ds     = vpi_h26xe_cfg->cb_ds;
    cr_ds     = vpi_h26xe_cfg->cr_ds;
    alignment = (vpi_h26xe_cfg->format_customized_type != -1 ?
                     0 :
                     vpi_h26xe_cfg->input_alignment);
    VCEncGetAlignedStride(src_width, inputFormat, &luma_stride, &chroma_stride,
                          alignment);
    VCEncGetAlignedStride(src_width_ds, inputFormat, &luma_stride_ds,
                          &chroma_stride_ds, alignment);

    if (vpi_h26xe_cfg->format_customized_type == 0 &&
        inputFormat == VCENC_YUV420_PLANAR)
        memset(lum, 0, luma_stride * src_height * 3 / 2);

    switch (inputFormat) {
    case VCENC_YUV420_PLANAR: {
        /* Lum */
        for (i = 0; i < src_height; i++) {
            if (file_read(vpi_h26xe_cfg->in, lum, seek, src_width))
                return NOK;
            seek += src_width;
            lum += luma_stride;
        }
        if (vpi_h26xe_cfg->in_ds) {
            for (i = 0; i < src_height_ds; i++) {
                if (file_read(vpi_h26xe_cfg->in_ds, lum_ds, seek_ds,
                              src_width_ds))
                    return NOK;
                seek_ds += src_width_ds;
                lum_ds += luma_stride_ds;
            }
        }

        w = ((src_width + 1) >> 1);
        h = ((src_height + 1) >> 1);
        /* Cb */
        for (i = 0; i < h; i++) {
            if (file_read(vpi_h26xe_cfg->in, cb, seek, w))
                return NOK;
            seek += w;
            cb += chroma_stride;
        }

        /* Cr */
        for (i = 0; i < h; i++) {
            if (file_read(vpi_h26xe_cfg->in, cr, seek, w))
                return NOK;

            seek += w;
            cr += chroma_stride;
        }
        if (vpi_h26xe_cfg->in_ds) {
            w = ((src_width_ds + 1) >> 1);
            h = ((src_height_ds + 1) >> 1);
            for (i = 0; i < h; i++) {
                if (file_read(vpi_h26xe_cfg->in_ds, cb_ds, seek_ds, w))
                    return NOK;
                seek_ds += w;
                cb_ds += chroma_stride_ds;
            }

            /* Cr */
            for (i = 0; i < h; i++) {
                if (file_read(vpi_h26xe_cfg->in_ds, cr_ds, seek_ds, w))
                    return NOK;

                seek_ds += w;
                cr_ds += chroma_stride_ds;
            }
        }
        break;
    }
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU: {
        /* Lum */
        ENC_TB_DEBUGV_PRINT("%d, %d, %d\n", src_width, src_height, luma_stride);
        for (i = 0; i < src_height; i++) {
            if (file_read(vpi_h26xe_cfg->in, lum, seek, src_width))
                return NOK;
            seek += src_width;
            lum += luma_stride;
        }

        /* CbCr */
        ENC_TB_DEBUGV_PRINT("%d, %d, %d\n", src_width, src_height / 2,
                            chroma_stride);
        for (i = 0; i < (src_height / 2); i++) {
            if (file_read(vpi_h26xe_cfg->in, cb, seek, src_width))
                return NOK;
            seek += src_width;
            cb += chroma_stride;
        }
        break;
    }
    case VCENC_YUV422_INTERLEAVED_YUYV:
    case VCENC_YUV422_INTERLEAVED_UYVY:
    case VCENC_RGB565:
    case VCENC_BGR565:
    case VCENC_RGB555:
    case VCENC_BGR555:
    case VCENC_RGB444:
    case VCENC_BGR444: {
        for (i = 0; i < src_height; i++) {
            if (file_read(vpi_h26xe_cfg->in, lum, seek, src_width * 2))
                return NOK;
            seek += src_width * 2;
            lum += luma_stride;
        }
        break;
    }
    case VCENC_RGB888:
    case VCENC_BGR888:
    case VCENC_RGB101010:
    case VCENC_BGR101010: {
        for (i = 0; i < src_height; i++) {
            if (file_read(vpi_h26xe_cfg->in, lum, seek, src_width * 4))
                return NOK;
            seek += src_width * 4;
            lum += luma_stride;
        }
        break;
    }
    case VCENC_YUV420_PLANAR_10BIT_I010: {
        /* Lum */
        for (i = 0; i < src_height; i++) {
            if (file_read(vpi_h26xe_cfg->in, lum, seek, src_width * 2))
                return NOK;
            seek += src_width * 2;
            lum += luma_stride;
        }

        w = ((src_width + 1) >> 1);
        h = ((src_height + 1) >> 1);
        /* Cb */
        for (i = 0; i < h; i++) {
            if (file_read(vpi_h26xe_cfg->in, cb, seek, w * 2))
                return NOK;
            seek += w * 2;
            cb += chroma_stride;
        }

        /* Cr */
        for (i = 0; i < h; i++) {
            if (file_read(vpi_h26xe_cfg->in, cr, seek, w * 2))
                return NOK;
            seek += w * 2;
            cr += chroma_stride;
        }
        break;
    }
    case VCENC_YUV420_PLANAR_10BIT_P010: {
        /* Lum */

        ENC_TB_DEBUGV_PRINT("%d, %d, %d\n", src_width * 2, src_height,
                            luma_stride);

        for (i = 0; i < src_height; i++) {
            if (file_read(vpi_h26xe_cfg->in, lum, seek, src_width * 2))
                return NOK;
            seek += src_width * 2;
            lum += luma_stride;
        }

        h = ((src_height + 1) >> 1);
        /* CbCr */

        ENC_TB_DEBUGV_PRINT("%d, %d, %d\n", src_width * 2, h, chroma_stride);

        for (i = 0; i < h; i++) {
            if (file_read(vpi_h26xe_cfg->in, cb, seek, src_width * 2))
                return NOK;
            seek += src_width * 2;
            cb += chroma_stride;
        }
        break;
    }
    case VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR: {
        /* Lum */
        for (i = 0; i < src_height; i++) {
            if (file_read(vpi_h26xe_cfg->in, lum, seek, src_width * 10 / 8))
                return NOK;
            seek += src_width * 10 / 8;
            lum += luma_stride * 10 / 8;
        }
        w = ((src_width + 1) >> 1);
        h = ((src_height + 1) >> 1);
        /* Cb */
        for (i = 0; i < h; i++) {
            if (file_read(vpi_h26xe_cfg->in, cb, seek, w * 10 / 8))
                return NOK;
            seek += w * 10 / 8;
            cb += chroma_stride * 10 / 8;
        }

        /* Cr */
        for (i = 0; i < h; i++) {
            if (file_read(vpi_h26xe_cfg->in, cr, seek, w * 10 / 8))
                return NOK;

            seek += w * 10 / 8;
            cr += chroma_stride * 10 / 8;
        }
        break;
    }
    case VCENC_YUV420_10BIT_PACKED_Y0L2: {
        for (i = 0; i < src_height / 2; i++) {
            if (file_read(vpi_h26xe_cfg->in, lum, seek, src_width * 2 * 2))
                return NOK;
            seek += src_width * 2 * 2;
            lum += luma_stride * 2 * 2;
        }
        break;
    }
    case VCENC_YUV420_PLANAR_8BIT_DAHUA_HEVC:
    case VCENC_YUV420_PLANAR_8BIT_DAHUA_H264: {
        if (file_read(vpi_h26xe_cfg->in, lum, seek, src_img_size))
            return NOK;
        break;
    }
    case VCENC_YUV420_SEMIPLANAR_8BIT_FB:
    case VCENC_YUV420_SEMIPLANAR_VU_8BIT_FB:
    case VCENC_YUV420_PLANAR_10BIT_P010_FB: {
        ENC_TB_DEBUGV_PRINT("read size = %d\n", src_img_size);
        if (file_read(vpi_h26xe_cfg->in, lum, seek, src_img_size))
            return NOK;
        break;
    }
    case VCENC_YUV420_SEMIPLANAR_101010:
    case VCENC_YUV420_8BIT_TILE_64_4:
    case VCENC_YUV420_UV_8BIT_TILE_64_4:
    case VCENC_YUV420_10BIT_TILE_32_4:
    case VCENC_YUV420_10BIT_TILE_48_4:
    case VCENC_YUV420_VU_10BIT_TILE_48_4:
    case VCENC_YUV420_8BIT_TILE_128_2:
    case VCENC_YUV420_UV_8BIT_TILE_128_2:
    case VCENC_YUV420_10BIT_TILE_96_2:
    case VCENC_YUV420_VU_10BIT_TILE_96_2: {
        u32 tile_stride = 0, tile_h = 0;
        if (inputFormat == VCENC_YUV420_SEMIPLANAR_101010) {
            tile_stride = (src_width + 2) / 3 * 4;
            tile_h      = 1;
        } else if (inputFormat == VCENC_YUV420_8BIT_TILE_64_4 ||
                   inputFormat == VCENC_YUV420_UV_8BIT_TILE_64_4) {
            tile_stride = STRIDE(src_width, 64) * 4;
            tile_h      = 4;
        } else if (inputFormat == VCENC_YUV420_10BIT_TILE_32_4) {
            tile_stride = STRIDE(src_width, 32) * 2 * 4;
            tile_h      = 4;
        } else if (inputFormat == VCENC_YUV420_10BIT_TILE_48_4 ||
                   inputFormat == VCENC_YUV420_VU_10BIT_TILE_48_4) {
            tile_stride = (src_width + 47) / 48 * 48 / 3 * 4 * 4;
            tile_h      = 4;
        } else if (inputFormat == VCENC_YUV420_8BIT_TILE_128_2 ||
                   inputFormat == VCENC_YUV420_UV_8BIT_TILE_128_2) {
            tile_stride = STRIDE(src_width, 128) * 2;
            tile_h      = 2;
        } else if (inputFormat == VCENC_YUV420_10BIT_TILE_96_2 ||
                   inputFormat == VCENC_YUV420_VU_10BIT_TILE_96_2) {
            tile_stride = (src_width + 95) / 96 * 96 / 3 * 4 * 2;
            tile_h      = 2;
        }

        /* Lum */
        for (i = 0; i < STRIDE(src_height, tile_h) / tile_h; i++) {
            if (file_read(vpi_h26xe_cfg->in, lum, seek, tile_stride))
                return NOK;
            seek += tile_stride;
            lum += luma_stride;
        }
        /* Cb */
        for (i = 0; i < STRIDE(src_height / 2, tile_h) / tile_h; i++) {
            if (file_read(vpi_h26xe_cfg->in, cb, seek, tile_stride))
                return NOK;
            seek += tile_stride;
            cb += chroma_stride;
        }

        break;
    }
#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
    case INPUT_FORMAT_YUV420_SEMIPLANAR_8BIT_COMPRESSED_FB:
    case INPUT_FORMAT_YUV420_SEMIPLANAR_VU_8BIT_COMPRESSED_FB:
    case INPUT_FORMAT_YUV420_PLANAR_10BIT_P010_COMPRESSED_FB:
    case INPUT_FORMAT_RFC_8BIT_COMPRESSED_FB:
    case INPUT_FORMAT_RFC_10BIT_COMPRESSED_FB: {
        if (file_read(vpi_h26xe_cfg->in, lum, seek, src_img_size))
            return NOK;
        break;
    }

    case INPUT_FORMAT_ARGB_FB:
    case INPUT_FORMAT_ABGR_FB:
    case INPUT_FORMAT_RGBA_FB:
    case INPUT_FORMAT_BGRA_FB:
    case INPUT_FORMAT_RGB24_FB:
    case INPUT_FORMAT_BGR24_FB:
    case INPUT_FORMAT_YUV420_PLANAR_10BIT_P010LE:
    case INPUT_FORMAT_YUV420_PLANAR_10BIT_P010BE:
    case INPUT_FORMAT_YUV420_SEMIPLANAR_10BIT_P010BE:
    case INPUT_FORMAT_YUV422P:
    case INPUT_FORMAT_YUV422P10LE:
    case INPUT_FORMAT_YUV422P10BE:
    case INPUT_FORMAT_YUV444P: {
        /* first plane */
        if (inputFormat < INPUT_FORMAT_RGB24_FB)
            w = src_width * 4;
        else if (inputFormat < INPUT_FORMAT_YUV420_PLANAR_10BIT_P010LE)
            w = src_width * 3;
        else if (inputFormat == INPUT_FORMAT_YUV422P ||
                 inputFormat == INPUT_FORMAT_YUV444P)
            w = src_width;
        else
            w = src_width * 2;
        /*stride = STRIDE(w, alignment);*/
        h = src_height;

        ENC_TB_DEBUGV_PRINT("w = %d, h = %d, stride = %d\n", w, h, luma_stride);

        for (i = 0; i < h; i++) {
            if (file_read(vpi_h26xe_cfg->in, lum, seek, w))
                return NOK;
            seek += w;
            lum += luma_stride;
        }

        if (inputFormat > INPUT_FORMAT_BGR24_FB) {
            /* second plane */
            if (inputFormat != INPUT_FORMAT_YUV420_SEMIPLANAR_10BIT_P010BE &&
                inputFormat != INPUT_FORMAT_YUV444P) {
                w = ((w + 1) >> 1);
                /*stride = STRIDE(w, alignment);*/
            }
            if (inputFormat < INPUT_FORMAT_YUV422P) {
                h = ((h + 1) >> 1);
            }

            ENC_TB_DEBUGV_PRINT("w = %d, h = %d, stride = %d\n", w, h,
                                chroma_stride);

            for (i = 0; i < h; i++) {
                if (file_read(vpi_h26xe_cfg->in, cb, seek, w))
                    return NOK;
                seek += w;
                cb += chroma_stride;
            }

            if (inputFormat != INPUT_FORMAT_YUV420_SEMIPLANAR_10BIT_P010BE) {
                /* third plane */
                ENC_TB_DEBUGV_PRINT("w = %d, h = %d, stride = %d\n", w, h,
                                    chroma_stride);

                for (i = 0; i < h; i++) {
                    if (file_read(vpi_h26xe_cfg->in, cr, seek, w))
                        return NOK;
                    seek += w;
                    cr += chroma_stride;
                }
            }
        }
    }
#endif
    case INPUT_FORMAT_PP_YUV420_SEMIPLANNAR:
    case INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_VU:
    case INPUT_FORMAT_PP_YUV420_PLANAR_10BIT_P010:
        ENC_TB_DEBUGV_PRINT("read size = %d\n", src_img_size);
        if (file_read(vpi_h26xe_cfg->in, lum, seek, src_img_size))
            return NOK;
        break;
    default:
        break;
    }

    return OK;
}

/**
 *get_resolution
 *Parse image resolution from file name
 */
u32 get_resolution(char *filename, i32 *p_width, i32 *p_height)
{
    i32 i;
    u32 w, h;
    i32 len           = strlen(filename);
    i32 filenameBegin = 0;

    /* Find last '/' in the file name, it marks the beginning of file name */
    for (i = len - 1; i; --i)
        if (filename[i] == '/') {
            filenameBegin = i + 1;
            break;
        }

    /* If '/' found, it separates trailing path from file name */
    for (i = filenameBegin; i <= len - 3; ++i) {
        if ((strncmp(filename + i, "subqcif", 7) == 0) ||
            (strncmp(filename + i, "sqcif", 5) == 0)) {
            *p_width  = 128;
            *p_height = 96;
            ENC_TB_INFO_PRINT(
                "Detected resolution SubQCIF (128x96) from file name.\n");
            return 0;
        }
        if (strncmp(filename + i, "qcif", 4) == 0) {
            *p_width  = 176;
            *p_height = 144;
            ENC_TB_INFO_PRINT(
                "Detected resolution QCIF (176x144) from file name.\n");
            return 0;
        }
        if (strncmp(filename + i, "4cif", 4) == 0) {
            *p_width  = 704;
            *p_height = 576;
            ENC_TB_INFO_PRINT(
                "Detected resolution 4CIF (704x576) from file name.\n");
            return 0;
        }
        if (strncmp(filename + i, "cif", 3) == 0) {
            *p_width  = 352;
            *p_height = 288;
            ENC_TB_INFO_PRINT(
                "Detected resolution CIF (352x288) from file name.\n");
            return 0;
        }
        if (strncmp(filename + i, "qqvga", 5) == 0) {
            *p_width  = 160;
            *p_height = 120;
            ENC_TB_INFO_PRINT(
                "Detected resolution QQVGA (160x120) from file name.\n");
            return 0;
        }
        if (strncmp(filename + i, "qvga", 4) == 0) {
            *p_width  = 320;
            *p_height = 240;
            ENC_TB_INFO_PRINT(
                "Detected resolution QVGA (320x240) from file name.\n");
            return 0;
        }
        if (strncmp(filename + i, "vga", 3) == 0) {
            *p_width  = 640;
            *p_height = 480;
            ENC_TB_INFO_PRINT(
                "Detected resolution VGA (640x480) from file name.\n");
            return 0;
        }
        if (strncmp(filename + i, "720p", 4) == 0) {
            *p_width  = 1280;
            *p_height = 720;
            ENC_TB_INFO_PRINT(
                "Detected resolution 720p (1280x720) from file name.\n");
            return 0;
        }
        if (strncmp(filename + i, "1080p", 5) == 0) {
            *p_width  = 1920;
            *p_height = 1080;
            ENC_TB_INFO_PRINT(
                "Detected resolution 1080p (1920x1080) from file name.\n");
            return 0;
        }
        if (filename[i] == 'x') {
            if (sscanf(filename + i - 4, "%ux%u", &w, &h) == 2) {
                *p_width  = w;
                *p_height = h;
                ENC_TB_INFO_PRINT("Detected resolution %dx%d from file name.\n",
                                  w, h);
                return 0;
            } else if (sscanf(filename + i - 3, "%ux%u", &w, &h) == 2) {
                *p_width  = w;
                *p_height = h;
                ENC_TB_INFO_PRINT("Detected resolution %dx%d from file name.\n",
                                  w, h);
                return 0;
            } else if (sscanf(filename + i - 2, "%ux%u", &w, &h) == 2) {
                *p_width  = w;
                *p_height = h;
                ENC_TB_INFO_PRINT("Detected resolution %dx%d from file name.\n",
                                  w, h);
                return 0;
            }
        }
        if (filename[i] == 'w') {
            if (sscanf(filename + i, "w%uh%u", &w, &h) == 2) {
                *p_width  = w;
                *p_height = h;
                ENC_TB_INFO_PRINT("Detected resolution %dx%d from file name.\n",
                                  w, h);
                return 0;
            }
        }
    }

    return 1; /* Error - no resolution found */
}

static char *next_token(char *str)
{
    char *p = strchr(str, ' ');
    if (p) {
        while (*p == ' ')
            p++;
        if (*p == '\0')
            p = NULL;
    }
    return p;
}

static int parse_gop_config_string(char *line, VCEncGopConfig *gop_cfg,
                                   int frame_idx, int gop_size)
{
    if (!line)
        return -1;

    /*format: FrameN Type POC QPoffset QPfactor  num_ref_pics ref_pics  used_by_cur*/
    int frameN, poc, num_ref_pics, i;
    char type;
    VCEncGopPicConfig *cfg         = NULL;
    VCEncGopPicSpecialConfig *scfg = NULL;

    /*frame idx*/
    sscanf(line, "Frame%d", &frameN);
    if ((frameN != (frame_idx + 1)) && (frameN != 0))
        return -1;

    if (frameN > gop_size)
        return 0;

    if (0 == frameN) {
        /*format: FrameN Type  QPoffset  QPfactor   TemporalId  num_ref_pics   ref_pics  used_by_cur  LTR    Offset   Interval*/
        scfg = &(gop_cfg->pGopPicSpecialCfg[gop_cfg->special_size++]);

        /*frame type*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%c", &type);
        if (type == 'I' || type == 'i')
            scfg->codingType = VCENC_INTRA_FRAME;
        else if (type == 'P' || type == 'p')
            scfg->codingType = VCENC_PREDICTED_FRAME;
        else if (type == 'B' || type == 'b')
            scfg->codingType = VCENC_BIDIR_PREDICTED_FRAME;
        else
            scfg->codingType = FRAME_TYPE_RESERVED;

        /*qp offset*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%d", &(scfg->QpOffset));

        /*qp factor*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%lf", &(scfg->QpFactor));
        scfg->QpFactor = sqrt(scfg->QpFactor);

        /*temporalId factor*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%d", &(scfg->temporalId));

        /*num_ref_pics*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%d", &num_ref_pics);
        if (num_ref_pics > VCENC_MAX_REF_FRAMES) { /* NUMREFPICS_RESERVED -1 */
            ENC_TB_ERROR_PRINT(
                "GOP Config: Error, num_ref_pic can not be more than %d \n",
                VCENC_MAX_REF_FRAMES);
            return -1;
        }
        scfg->numRefPics = num_ref_pics;

        if ((scfg->codingType == VCENC_INTRA_FRAME) && (0 == num_ref_pics))
            num_ref_pics = 1;
        /*ref_pics*/
        for (i = 0; i < num_ref_pics; i++) {
            line = next_token(line);
            if (!line)
                return -1;
            if ((strncmp(line, "L", 1) == 0) || (strncmp(line, "l", 1) == 0)) {
                sscanf(line, "%c%d", &type, &(scfg->refPics[i].ref_pic));
                scfg->refPics[i].ref_pic =
                    LONG_TERM_REF_ID2DELTAPOC(scfg->refPics[i].ref_pic - 1);
            } else {
                sscanf(line, "%d", &(scfg->refPics[i].ref_pic));
            }
        }
        if (i < num_ref_pics)
            return -1;

        /*used_by_cur*/
        for (i = 0; i < num_ref_pics; i++) {
            line = next_token(line);
            if (!line)
                return -1;
            sscanf(line, "%u", &(scfg->refPics[i].used_by_cur));
        }
        if (i < num_ref_pics)
            return -1;

        /* LTR*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%d", &scfg->i32Ltr);
        if (VCENC_MAX_LT_REF_FRAMES < scfg->i32Ltr)
            return -1;

        /* Offset*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%d", &scfg->i32Offset);

        /* Interval*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%d", &scfg->i32Interval);

        if (0 != scfg->i32Ltr) {
            gop_cfg->u32LTR_idx[gop_cfg->ltrcnt] =
                LONG_TERM_REF_ID2DELTAPOC(scfg->i32Ltr - 1);
            gop_cfg->ltrcnt++;
            if (VCENC_MAX_LT_REF_FRAMES < gop_cfg->ltrcnt)
                return -1;
        }
        /* short_change*/
        scfg->i32short_change = 0;
        if (0 == scfg->i32Ltr) {
            /* not long-term ref */
            scfg->i32short_change = 1;
            for (i = 0; i < num_ref_pics; i++) {
                if (IS_LONG_TERM_REF_DELTAPOC(scfg->refPics[i].ref_pic) &&
                    (0 != scfg->refPics[i].used_by_cur)) {
                    scfg->i32short_change = 0;
                    break;
                }
            }
        }
    } else {
        /*format: FrameN Type  POC  QPoffset    QPfactor   TemporalId  num_ref_pics  ref_pics  used_by_cur*/
        cfg = &(gop_cfg->pGopPicCfg[gop_cfg->size++]);

        /*frame type*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%c", &type);
        if (type == 'P' || type == 'p')
            cfg->codingType = VCENC_PREDICTED_FRAME;
        else if (type == 'B' || type == 'b')
            cfg->codingType = VCENC_BIDIR_PREDICTED_FRAME;
        else
            return -1;

        /*poc*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%d", &poc);
        if (poc < 1 || poc > gop_size)
            return -1;
        cfg->poc = poc;

        /*qp offset*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%d", &(cfg->QpOffset));

        /*qp factor*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%lf", &(cfg->QpFactor));
        /* sqrt(QpFactor) is used in calculating lambda*/
        cfg->QpFactor = sqrt(cfg->QpFactor);

        /*temporalId factor*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%d", &(cfg->temporalId));

        /*num_ref_pics*/
        line = next_token(line);
        if (!line)
            return -1;
        sscanf(line, "%d", &num_ref_pics);
        if (num_ref_pics < 0 || num_ref_pics > VCENC_MAX_REF_FRAMES) {
            ENC_TB_ERROR_PRINT(
                "GOP Config: Error, num_ref_pic can not be more than %d \n",
                VCENC_MAX_REF_FRAMES);
            return -1;
        }
        /*ref_pics*/
        for (i = 0; i < num_ref_pics; i++) {
            line = next_token(line);
            if (!line)
                return -1;
            if ((strncmp(line, "L", 1) == 0) || (strncmp(line, "l", 1) == 0)) {
                sscanf(line, "%c%d", &type, &(cfg->refPics[i].ref_pic));
                cfg->refPics[i].ref_pic =
                    LONG_TERM_REF_ID2DELTAPOC(cfg->refPics[i].ref_pic - 1);
            } else {
                sscanf(line, "%d", &(cfg->refPics[i].ref_pic));
            }
        }
        if (i < num_ref_pics)
            return -1;

        /*used_by_cur*/
        for (i = 0; i < num_ref_pics; i++) {
            line = next_token(line);
            if (!line)
                return -1;
            sscanf(line, "%u", &(cfg->refPics[i].used_by_cur));
        }
        if (i < num_ref_pics)
            return -1;

        cfg->numRefPics = num_ref_pics;
    }

    return 0;
}

static int parse_gop_config_file(int gop_size, char *fname,
                                 VCEncGopConfig *gop_cfg)
{
#define MAX_LINE_LENGTH 1024
    int frame_idx = 0, line_idx = 0, add_tmp;
    char ach_parser_buffer[MAX_LINE_LENGTH];
    FILE *f_in = fopen(fname, "r");
    if (f_in == NULL) {
        ENC_TB_ERROR_PRINT("GOP Config: Error, Can Not Open File %s\n", fname);
        return -1;
    }

    while (0 == feof(f_in)) {
        if (feof(f_in))
            break;
        line_idx++;
        ach_parser_buffer[0] = '\0';
        /*Read one line*/
        char *line = fgets((char *)ach_parser_buffer, MAX_LINE_LENGTH, f_in);
        if (!line)
            break;
        /*handle line end*/
        char *s = strpbrk(line, "#\n");
        if (s)
            *s = '\0';

        add_tmp = 1;
        line    = strstr(line, "Frame");
        if (line) {
            if (0 == strncmp(line, "Frame0", 6))
                add_tmp = 0;

            if (parse_gop_config_string(line, gop_cfg, frame_idx, gop_size) <
                0) {
                ENC_TB_ERROR_PRINT("Invalid gop configure!\n");
                return -1;
            }

            frame_idx += add_tmp;
        }
    }

    fclose(f_in);
    if (frame_idx != gop_size) {
        ENC_TB_ERROR_PRINT("GOP Config: Error, Parsing File %s Failed at Line %d\n",
                           fname, line_idx);
        return -1;
    }
    return 0;
}

static int read_gop_config(char *fname, char **config, VCEncGopConfig *gop_cfg,
                           int gop_size, u8 *gop_cfg_offset)
{
    int ret = -1;

    if (gop_cfg->size >= MAX_GOP_PIC_CONFIG_NUM)
        return -1;

    if (gop_cfg_offset)
        gop_cfg_offset[gop_size] = gop_cfg->size;
    if (fname) {
        ret = parse_gop_config_file(gop_size, fname, gop_cfg);
    } else if (config) {
        int id = 0;
        while (config[id]) {
            parse_gop_config_string(config[id], gop_cfg, id, gop_size);
            id++;
        }
        ret = 0;
    }
    return ret;
}

int init_gop_configs(int gop_size, VPIH26xEncOptions *options,
                     VCEncGopConfig *gop_cfg, u8 *gop_cfg_offset, bool bPass2)
{
    int i, pre_load_num;
    char *fname = options->gop_cfg;
    char **default_configs[8] = {
        options->
            gop_lowdelay ? RpsLowdelayDefaultGOPSize1
            : (IS_H264(options->codec_format) ? RpsDefaultH264GOPSize1 :
               RpsDefaultGOPSize1),
        options->
            gop_lowdelay ? RpsLowdelayDefaultGOPSize2 : RpsDefaultGOPSize2,
        options->
            gop_lowdelay ? RpsLowdelayDefaultGOPSize3 : RpsDefaultGOPSize3,
        options->
            gop_lowdelay ? RpsLowdelayDefaultGOPSize4 : RpsDefaultGOPSize4,
        RpsDefaultGOPSize5,
        RpsDefaultGOPSize6,
        RpsDefaultGOPSize7,
        RpsDefaultGOPSize8
    };
    if (gop_size < 0 || gop_size > MAX_GOP_SIZE) {
        ENC_TB_ERROR_PRINT("GOP Config: Error, Invalid GOP Size\n");
        return -1;
    }
    if (bPass2) {
        default_configs[1] = RpsPass2GOPSize2;
        default_configs[3] = RpsPass2GOPSize4;
        default_configs[7] = RpsPass2GOPSize8;
    }
    /*Handle Interlace*/
    if (options->interlaced_frame && gop_size == 1) {
        default_configs[0] = RpsDefaultInterlaceGOPSize1;
    }
    /* GOP size in rps array for gop_size=N
       N<=4:      GOP1, ..., GOPN
       4<N<=8:   GOP1, GOP2, GOP3, GOP4, GOPN
       N > 8:       GOP1, GOPN
       Adaptive:  GOP1, GOP2, GOP3, GOP4, GOP6, GOP8
    */
    if (gop_size > 8)
        pre_load_num = 1;
    else if (gop_size >= 4 || gop_size == 0)
        pre_load_num = 4;
    else
        pre_load_num = gop_size;

    gop_cfg->special_size = 0;
    gop_cfg->ltrcnt       = 0;

    for (i = 1; i <= pre_load_num; i++) {
        if (read_gop_config(gop_size == i ? fname : NULL,
                            default_configs[i - 1], gop_cfg, i, gop_cfg_offset))
            return -1;
    }

    if (gop_size == 0) {
        /*gop6*/
        if (read_gop_config(NULL, default_configs[5], gop_cfg, 6,
                            gop_cfg_offset))
            return -1;
        /*gop8*/
        if (read_gop_config(NULL, default_configs[7], gop_cfg, 8,
                            gop_cfg_offset))
            return -1;
    } else if (gop_size > 4) {
        /*gop_size*/
        if (read_gop_config(fname, default_configs[gop_size - 1], gop_cfg,
                            gop_size, gop_cfg_offset))
            return -1;
    }

    if ((DEFAULT != options->ltr_interval) && (gop_cfg->special_size == 0)) {
        if (options->gop_size != 1) {
            ENC_TB_ERROR_PRINT(
                "GOP Config: Error, when using --LTR configure option, the gopsize alse should be set to 1!\n");
            return -1;
        }
        gop_cfg->pGopPicSpecialCfg[0].poc         = 0;
        gop_cfg->pGopPicSpecialCfg[0].QpOffset    = options->long_term_qp_delta;
        gop_cfg->pGopPicSpecialCfg[0].QpFactor    = QPFACTOR_RESERVED;
        gop_cfg->pGopPicSpecialCfg[0].temporalId  = TEMPORALID_RESERVED;
        gop_cfg->pGopPicSpecialCfg[0].codingType  = FRAME_TYPE_RESERVED;
        gop_cfg->pGopPicSpecialCfg[0].numRefPics  = NUMREFPICS_RESERVED;
        gop_cfg->pGopPicSpecialCfg[0].i32Ltr      = 1;
        gop_cfg->pGopPicSpecialCfg[0].i32Offset   = 0;
        gop_cfg->pGopPicSpecialCfg[0].i32Interval = options->ltr_interval;
        gop_cfg->pGopPicSpecialCfg[0].i32short_change = 0;
        gop_cfg->u32LTR_idx[0] = LONG_TERM_REF_ID2DELTAPOC(0);

        gop_cfg->pGopPicSpecialCfg[1].poc                = 0;
        gop_cfg->pGopPicSpecialCfg[1].QpOffset           = QPOFFSET_RESERVED;
        gop_cfg->pGopPicSpecialCfg[1].QpFactor           = QPFACTOR_RESERVED;
        gop_cfg->pGopPicSpecialCfg[1].temporalId         = TEMPORALID_RESERVED;
        gop_cfg->pGopPicSpecialCfg[1].codingType         = FRAME_TYPE_RESERVED;
        gop_cfg->pGopPicSpecialCfg[1].numRefPics         = 2;
        gop_cfg->pGopPicSpecialCfg[1].refPics[0].ref_pic = -1;
        gop_cfg->pGopPicSpecialCfg[1].refPics[0].used_by_cur = 1;
        gop_cfg->pGopPicSpecialCfg[1].refPics[1].ref_pic =
            LONG_TERM_REF_ID2DELTAPOC(0);
        gop_cfg->pGopPicSpecialCfg[1].refPics[1].used_by_cur = 1;
        gop_cfg->pGopPicSpecialCfg[1].i32Ltr                 = 0;
        gop_cfg->pGopPicSpecialCfg[1].i32Offset = options->long_term_gap_offset;
        gop_cfg->pGopPicSpecialCfg[1].i32Interval     = options->long_term_gap;
        gop_cfg->pGopPicSpecialCfg[1].i32short_change = 0;

        gop_cfg->special_size = 2;
        gop_cfg->ltrcnt       = 1;
    }

    if (0)
        for (i = 0;
             i < (gop_size == 0 ? gop_cfg->size : gop_cfg_offset[gop_size]);
             i++) {
            /* when use long-term, change P to B in default configs (used for last gop)*/
            VCEncGopPicConfig *cfg = &(gop_cfg->pGopPicCfg[i]);
            if (cfg->codingType == VCENC_PREDICTED_FRAME)
                cfg->codingType = VCENC_BIDIR_PREDICTED_FRAME;
        }
    /*Compatible with old bframe_qp_delta setting*/
    if (options->bframe_qp_delta >= 0 && fname == NULL) {
        for (i = 0; i < gop_cfg->size; i++) {
            VCEncGopPicConfig *cfg = &(gop_cfg->pGopPicCfg[i]);
            if (cfg->codingType == VCENC_BIDIR_PREDICTED_FRAME)
                cfg->QpOffset = options->bframe_qp_delta;
        }
    }
    /*lowDelay auto detection*/
    VCEncGopPicConfig *cfg_start =
        &(gop_cfg->pGopPicCfg[gop_cfg_offset[gop_size]]);
    if (gop_size == 1) {
        options->gop_lowdelay = 1;
    } else if ((gop_size > 1) && (options->gop_lowdelay == 0)) {
        options->gop_lowdelay = 1;
        for (i = 1; i < gop_size; i++) {
            if (cfg_start[i].poc < cfg_start[i - 1].poc) {
                options->gop_lowdelay = 0;
                break;
            }
        }
    }
#ifdef INTERNAL_TEST
    if ((options->test_id == TID_POC && gop_size == 1) &&
        !IS_H264(options->codec_format)) {
        VCEncGopPicConfig *cfg = &(gop_cfg->pGopPicCfg[0]);
        if (cfg->numRefPics == 2)
            cfg->refPics[1].ref_pic = -(options->intraPicRate - 1);
    }
#endif
    {
        i32 i32LtrPoc[VCENC_MAX_LT_REF_FRAMES];

        for (i = 0; i < VCENC_MAX_LT_REF_FRAMES; i++)
            i32LtrPoc[i] = -1;
        for (i = 0; i < gop_cfg->special_size; i++) {
            if (gop_cfg->pGopPicSpecialCfg[i].i32Ltr >
                VCENC_MAX_LT_REF_FRAMES) {
                ENC_TB_ERROR_PRINT(
                    "GOP Config: Error, Invalid long-term index\n");
                return -1;
            }
            if (gop_cfg->pGopPicSpecialCfg[i].i32Ltr > 0)
                i32LtrPoc[i] = gop_cfg->pGopPicSpecialCfg[i].i32Ltr - 1;
        }

        for (i = 0; i < gop_cfg->ltrcnt; i++) {
            if ((0 != i32LtrPoc[0]) || (-1 == i32LtrPoc[i]) ||
                ((i > 0) && i32LtrPoc[i] != (i32LtrPoc[i - 1] + 1))) {
                ENC_TB_ERROR_PRINT(
                    "GOP Config: Error, Invalid long-term index\n");
                return -1;
            }
        }
    }

    /*For lowDelay, Handle the first few frames that miss reference frame*/
    if (1) {
        int n_gop;
        int idx         = 0;
        int maxErrFrame = 0;
        VCEncGopPicConfig *cfg;

        /* Find the max frame number that will miss its reference frame defined in rps*/
        while ((idx - maxErrFrame) < gop_size) {
            n_gop = (idx / gop_size) * gop_size;
            cfg   = &(cfg_start[idx % gop_size]);

            for (i = 0; i < cfg->numRefPics; i++) {
                /*POC of this reference frame*/
                int refPoc = cfg->refPics[i].ref_pic + cfg->poc + n_gop;
                if (refPoc < 0) {
                    maxErrFrame = idx + 1;
                }
            }
            idx++;
        }

        /* Try to config a new rps for each "error" frame by modifying its original rps*/
        for (idx = 0; idx < maxErrFrame; idx++) {
            int j, iRef, nRefsUsedByCur, nPoc;
            VCEncGopPicConfig *cfgCopy;

            if (gop_cfg->size >= MAX_GOP_PIC_CONFIG_NUM)
                break;

            /* Add to array end*/
            cfg     = &(gop_cfg->pGopPicCfg[gop_cfg->size]);
            cfgCopy = &(cfg_start[idx % gop_size]);
            memcpy(cfg, cfgCopy, sizeof(VCEncGopPicConfig));
            gop_cfg->size++;

            /* Copy reference pictures*/
            nRefsUsedByCur = iRef = 0;
            nPoc = cfgCopy->poc + ((idx / gop_size) * gop_size);
            for (i = 0; i < cfgCopy->numRefPics; i++) {
                int new_ref     = 1;
                int used_by_cur = cfgCopy->refPics[i].used_by_cur;
                int ref_pic     = cfgCopy->refPics[i].ref_pic;
                /*Clip the reference POC*/
                if ((cfgCopy->refPics[i].ref_pic + nPoc) < 0)
                    ref_pic = 0 - (nPoc);

                /* Check if already have this reference*/
                for (j = 0; j < iRef; j++) {
                    if (cfg->refPics[j].ref_pic == ref_pic) {
                        new_ref = 0;
                        if (used_by_cur)
                            cfg->refPics[j].used_by_cur = used_by_cur;
                        break;
                    }
                }

                /*Copy this reference*/
                if (new_ref) {
                    cfg->refPics[iRef].ref_pic     = ref_pic;
                    cfg->refPics[iRef].used_by_cur = used_by_cur;
                    iRef++;
                }
            }
            cfg->numRefPics = iRef;
            /*If only one reference frame, set P type.*/
            for (i = 0; i < cfg->numRefPics; i++) {
                if (cfg->refPics[i].used_by_cur)
                    nRefsUsedByCur++;
            }
            if (nRefsUsedByCur == 1)
                cfg->codingType = VCENC_PREDICTED_FRAME;
        }
    }
    return 0;
}

/**
 *read_userdata
 *Read user data from file and pass to encoder
 *
 *@Params: name - name of file in which user data is located
 *@Returns:NULL - when user data reading failed. pointer - allocated buffer containing user data
 */
u8 *read_userdata(VCEncInst encoder, char *name)
{
    FILE *file          = NULL;
    i32 byteCnt;
    u8 *data;
    int ret = 0;

    if (name == NULL)
        return NULL;

    if (strcmp("0", name) == 0)
        return NULL;

    /* Get user data length from file */
    file = fopen(name, "rb");
    if (file == NULL) {
        ENC_TB_ERROR_PRINT("Unable to open User Data file: %s\n", name);
        return NULL;
    }
    fseeko(file, 0L, SEEK_END);
    byteCnt = ftell(file);
    rewind(file);

    /* Minimum size of user data */
    if (byteCnt < 16)
        byteCnt = 16;

    /* Maximum size of user data */
    if (byteCnt > 2048)
        byteCnt = 2048;

    /* Allocate memory for user data */
#ifdef CHECK_MEM_LEAK_TRANS
    if ((data = (u8 *)EWLmalloc(sizeof(u8) * byteCnt)) == NULL) {
#else
    if ((data = (u8 *)malloc(sizeof(u8) * byteCnt)) == NULL) {
#endif
        fclose(file);
        ENC_TB_ERROR_PRINT("Unable to alloc User Data memory\n");
        return NULL;
    }

    /* Read user data from file */
    ret = fread(data, sizeof(u8), byteCnt, file);
    fclose(file);

    ENC_TB_INFO_PRINT("User data: %d bytes [%d %d %d %d ...]\n", byteCnt,
                      data[0], data[1], data[2], data[3]);

    /* Pass the data buffer to encoder
     * The encoder reads the buffer during following VCEncStrmEncode() calls.
     * User data writing must be disabled (with VCEncSetSeiUserData(enc, 0, 0)) */
    VCEncSetSeiUserData(encoder, data, byteCnt);

    return data;
}

i32 get_next_gop_size(VPIH26xEncCfg *tb, VCEncIn *p_enc_in,
                      VCEncInst encoder, i32 *p_next_gop_size, AdapGopCtr *agop)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)encoder;
    if (vcenc_instance->lookaheadDepth) {
        i32 updGop =
            getPass1UpdatedGopSize(vcenc_instance->lookahead.priv_inst);
        if (updGop)
            *p_next_gop_size = updGop;
    } else if (p_enc_in->codingType != VCENC_INTRA_FRAME)
        adaptive_gop_decision(tb, p_enc_in, encoder, p_next_gop_size, agop);

    return *p_next_gop_size;
}

i32 adaptive_gop_decision(VPIH26xEncCfg *tb, VCEncIn *p_enc_in,
                          VCEncInst encoder, i32 *p_next_gop_size,
                          AdapGopCtr *agop)
{
    i32 next_gop_size = -1;

    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)encoder;
    unsigned int ui_intra_cu8_num = vcenc_instance->asic.regs.intraCu8Num;
    unsigned int ui_skip_cu8_num  = vcenc_instance->asic.regs.skipCu8Num;
    unsigned int ui_PB_frame_cost = vcenc_instance->asic.regs.PBFrame4NRdCost;
    double dIntraVsInterskip =
        (double)ui_intra_cu8_num / (double)((tb->width / 8) * (tb->height / 8));
    double dSkipVsInterskip =
        (double)ui_skip_cu8_num / (double)((tb->width / 8) * (tb->height / 8));

    agop->gop_frm_num++;
    agop->sum_intra_vs_interskip += dIntraVsInterskip;
    agop->sum_skip_vs_interskip += dSkipVsInterskip;
    agop->sum_cost_P +=
        (p_enc_in->codingType == VCENC_PREDICTED_FRAME) ? ui_PB_frame_cost : 0;
    agop->sum_cost_B += (p_enc_in->codingType == VCENC_BIDIR_PREDICTED_FRAME) ?
                            ui_PB_frame_cost :
                            0;
    agop->sum_intra_vs_interskipP +=
        (p_enc_in->codingType == VCENC_PREDICTED_FRAME) ? dIntraVsInterskip : 0;
    agop->sum_intra_vs_interskipB +=
        (p_enc_in->codingType == VCENC_BIDIR_PREDICTED_FRAME) ?
            dIntraVsInterskip :
            0;

    if (p_enc_in->gopPicIdx ==
        p_enc_in->gopSize -
            1) /*last frame of the current gop. decide the gopsize of next gop.*/
    {
        dIntraVsInterskip = agop->sum_intra_vs_interskip / agop->gop_frm_num;
        dSkipVsInterskip  = agop->sum_skip_vs_interskip / agop->gop_frm_num;
        agop->sum_cost_B  = (agop->gop_frm_num > 1) ?
                               (agop->sum_cost_B / (agop->gop_frm_num - 1)) :
                               0xFFFFFFF;
        agop->sum_intra_vs_interskipB =
            (agop->gop_frm_num > 1) ?
                (agop->sum_intra_vs_interskipB / (agop->gop_frm_num - 1)) :
                0xFFFFFFF;
        /*Enabled adaptive GOP size for large resolution*/
        if (((tb->width * tb->height) >= (1280 * 720)) ||
            ((MAX_ADAPTIVE_GOP_SIZE > 3) &&
             ((tb->width * tb->height) >= (416 * 240)))) {
            if ((((double)agop->sum_cost_P / (double)agop->sum_cost_B) < 1.1) &&
                (dSkipVsInterskip >= 0.95)) {
                agop->last_gopsize = next_gop_size = 1;
            } else if (((double)agop->sum_cost_P / (double)agop->sum_cost_B) >
                       5) {
                next_gop_size = agop->last_gopsize;
            } else {
                if (((agop->sum_intra_vs_interskipP > 0.40) &&
                     (agop->sum_intra_vs_interskipP < 0.70) &&
                     (agop->sum_intra_vs_interskipB < 0.10))) {
                    agop->last_gopsize++;
                    if (agop->last_gopsize == 5 || agop->last_gopsize == 7) {
                        agop->last_gopsize++;
                    }
                    agop->last_gopsize =
                        MIN(agop->last_gopsize, MAX_ADAPTIVE_GOP_SIZE);
                    next_gop_size = agop->last_gopsize;
                } else if (dIntraVsInterskip >= 0.30) {
                    agop->last_gopsize = next_gop_size = 1; /*No B*/
                } else if (dIntraVsInterskip >= 0.20) {
                    agop->last_gopsize = next_gop_size = 2; /*One B*/
                } else if (dIntraVsInterskip >= 0.10) {
                    agop->last_gopsize--;
                    if (agop->last_gopsize == 5 || agop->last_gopsize == 7) {
                        agop->last_gopsize--;
                    }
                    agop->last_gopsize = MAX(agop->last_gopsize, 3);
                    next_gop_size      = agop->last_gopsize;
                } else {
                    agop->last_gopsize++;
                    if (agop->last_gopsize == 5 || agop->last_gopsize == 7) {
                        agop->last_gopsize++;
                    }
                    agop->last_gopsize =
                        MIN(agop->last_gopsize, MAX_ADAPTIVE_GOP_SIZE);
                    next_gop_size = agop->last_gopsize;
                }
            }
        } else {
            next_gop_size = 3;
        }
        agop->gop_frm_num             = 0;
        agop->sum_intra_vs_interskip  = 0;
        agop->sum_skip_vs_interskip   = 0;
        agop->sum_cost_P              = 0;
        agop->sum_cost_B              = 0;
        agop->sum_intra_vs_interskipP = 0;
        agop->sum_intra_vs_interskipB = 0;

        next_gop_size = MIN(next_gop_size, MAX_ADAPTIVE_GOP_SIZE);
    }

    if (next_gop_size != -1)
        *p_next_gop_size = next_gop_size;

    return next_gop_size;
}

#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
#include "fb_ips.h"
#include "dec400_f2_api.h"
#include "tcache_api.h"
#include "trans_edma_api.h"
#include "dtrc_api.h"
#include "l2cache_api.h"

#define VCE_HEIGHT_ALIGNMENT 64
#define VCE_INPUT_ALIGNMENT 32
#endif

i32 change_format_for_FB(VPIH26xEncCfg *vpi_h26xe_cfg,
                         VPIH26xEncOptions *options,
                         VCEncPreProcessingCfg *pre_proc_cfg)
{
    switch (options->input_format) {
#ifdef SUPPORT_DEC400
        /*for dec400*/
    case INPUT_FORMAT_YUV420_SEMIPLANAR_8BIT_COMPRESSED_FB:
        pre_proc_cfg->inputType       = VCENC_YUV420_SEMIPLANAR_8BIT_FB;
        pre_proc_cfg->input_alignment = 1024;
        break;
    case INPUT_FORMAT_YUV420_SEMIPLANAR_VU_8BIT_COMPRESSED_FB:
        pre_proc_cfg->inputType       = VCENC_YUV420_SEMIPLANAR_VU_8BIT_FB;
        pre_proc_cfg->input_alignment = 1024;
        break;
    case INPUT_FORMAT_YUV420_PLANAR_10BIT_P010_COMPRESSED_FB:
        pre_proc_cfg->inputType       = VCENC_YUV420_PLANAR_10BIT_P010_FB;
        pre_proc_cfg->input_alignment = 1024;
        break;
#endif
#ifdef SUPPORT_TCACHE
        /*for tcache*/
    case VCENC_YUV420_PLANAR:
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU:
    case INPUT_FORMAT_YUV422P:
    case INPUT_FORMAT_YUV444P:
    case INPUT_FORMAT_RFC_8BIT_COMPRESSED_FB:
        pre_proc_cfg->inputType       = VCENC_YUV420_SEMIPLANAR;
        pre_proc_cfg->input_alignment = 32;
        break;
    case VCENC_YUV420_PLANAR_10BIT_P010: /*this is semiplaner P010LE */
    case INPUT_FORMAT_YUV420_SEMIPLANAR_10BIT_P010BE:
    case INPUT_FORMAT_YUV420_PLANAR_10BIT_P010BE:
    case INPUT_FORMAT_YUV420_PLANAR_10BIT_P010LE:
    case INPUT_FORMAT_YUV422P10LE:
    case INPUT_FORMAT_YUV422P10BE:
    case INPUT_FORMAT_RFC_10BIT_COMPRESSED_FB:
        pre_proc_cfg->inputType       = VCENC_YUV420_PLANAR_10BIT_P010;
        pre_proc_cfg->input_alignment = 32;
        break;
    case INPUT_FORMAT_ARGB_FB:
    case INPUT_FORMAT_ABGR_FB:
    case INPUT_FORMAT_BGRA_FB:
    case INPUT_FORMAT_RGBA_FB:
    case INPUT_FORMAT_BGR24_FB:
    case INPUT_FORMAT_RGB24_FB:
        pre_proc_cfg->inputType = (options->bit_depth_luma == 10) ?
                                      VCENC_YUV420_PLANAR_10BIT_P010 :
                                      VCENC_YUV420_SEMIPLANAR;
        pre_proc_cfg->input_alignment = 32;
        break;
#else
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_PLANAR_10BIT_P010:
        /*don't change*/
        break;
#endif
    case INPUT_FORMAT_PP_YUV420_SEMIPLANNAR:
        pre_proc_cfg->inputType = VCENC_YUV420_SEMIPLANAR;
        break;
    case INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_VU:
        pre_proc_cfg->inputType = VCENC_YUV420_SEMIPLANAR_VU;
        /*pre_proc_cfg->input_alignment = 32;*/ /*depend on the options option*/
        break;
    case INPUT_FORMAT_PP_YUV420_SEMIPLANNAR_YUV420P:
        pre_proc_cfg->inputType       = VCENC_YUV420_PLANAR;
        pre_proc_cfg->input_alignment = 32;
        break;
    case INPUT_FORMAT_PP_YUV420_PLANAR_10BIT_P010:
        pre_proc_cfg->inputType       = VCENC_YUV420_PLANAR_10BIT_P010;
        pre_proc_cfg->input_alignment = 32;
        break;
    default:
        break;
    }
    ENC_TB_DEBUGV_PRINT("::::: format %d -> %d, input alignment change to %d\n",
                        options->input_format, pre_proc_cfg->inputType,
                        pre_proc_cfg->input_alignment);
    return 0;
}

#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
i32 read_table(VPIH26xEncCfg *tb, u32 lum_tbl_size, u32 ch_tbl_size)
{
    i32 num;

#if 1
    num = h26x_enc_next_picture(tb, tb->enc_in.picture_cnt) + tb->first_pic;
#else
    num = tb->picture_enc_cnt;
#endif

    if (num > tb->last_pic)
        return NOK;

    if (file_read(tb->ints, tb->tslu, num * (lum_tbl_size + ch_tbl_size),
                  lum_tbl_size))
        return NOK;
    if (file_read(tb->ints, tb->tsch,
                  num * (lum_tbl_size + ch_tbl_size) + lum_tbl_size,
                  ch_tbl_size))
        return NOK;

    return OK;
}

static u32 get_bits_per_pixel(i32 input_type)
{
    switch (input_type) {
#ifdef SUPPORT_TCACHE
    case INPUT_FORMAT_ARGB_FB:
    case INPUT_FORMAT_ABGR_FB:
    case INPUT_FORMAT_RGBA_FB:
    case INPUT_FORMAT_BGRA_FB:
        return 32;
    case INPUT_FORMAT_RGB24_FB:
    case INPUT_FORMAT_BGR24_FB:
        return 24;
    case INPUT_FORMAT_YUV420_PLANAR_10BIT_P010LE:
    case INPUT_FORMAT_YUV420_PLANAR_10BIT_P010BE:
    case INPUT_FORMAT_YUV420_SEMIPLANAR_10BIT_P010BE:
        return 24;
    case INPUT_FORMAT_YUV422P:
        return 16;
    case INPUT_FORMAT_YUV422P10LE:
    case INPUT_FORMAT_YUV422P10BE:
        return 32;
    case INPUT_FORMAT_YUV444P:
        return 24;
#endif
    default:
        return 0;
    }
}
#endif

u32 setup_input_buffer(VPIH26xEncCfg *vpi_h26xe_cfg,
                       VPIH26xEncOptions *options, VCEncIn *p_enc_in)
{
    u32 src_img_size;

    ENC_TB_DEBUG_PRINT("\n");

#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
    if (options->input_format >=
            INPUT_FORMAT_YUV420_SEMIPLANAR_8BIT_COMPRESSED_FB &&
        options->input_format <= INPUT_FORMAT_RFC_10BIT_COMPRESSED_FB) {
        vpi_h26xe_cfg->tslu =
            (u8 *)vpi_h26xe_cfg->ts_lumamem->rc_virtualAddress;
        vpi_h26xe_cfg->tsch =
            (u8 *)vpi_h26xe_cfg->ts_chromamem->rc_virtualAddress;
    }
#endif
    if ((vpi_h26xe_cfg->input_alignment == 0 ||
         options->format_customized_type != -1) &&
        0 != VCEncGetBitsPerPixel(options->input_format)) {
        u32 size_lum = 0;
        u32 size_ch  = 0;

        get_aligned_pic_size_byformat(options->input_format,
                                      options->lum_width_src,
                                      options->lum_height_src, 0, &size_lum,
                                      &size_ch, NULL);

        p_enc_in->busLuma = vpi_h26xe_cfg->picture_mem->busAddress;
#ifdef USE_OLD_DRV
        vpi_h26xe_cfg->lum = (u8 *)vpi_h26xe_cfg->picture_mem->virtualAddress;
#else
        vpi_h26xe_cfg->lum =
            (u8 *)vpi_h26xe_cfg->picture_mem->rc_virtualAddress;
#endif
        p_enc_in->busChromaU = p_enc_in->busLuma + size_lum;
        vpi_h26xe_cfg->cb    = vpi_h26xe_cfg->lum + (u32)size_lum;
        p_enc_in->busChromaV = p_enc_in->busChromaU + (u32)size_ch / 2;
        vpi_h26xe_cfg->cr    = vpi_h26xe_cfg->cb + (u32)size_ch / 2;
        src_img_size = options->lum_width_src * options->lum_height_src *
                       VCEncGetBitsPerPixel(options->input_format) / 8;
    } else {
        p_enc_in->busLuma = vpi_h26xe_cfg->picture_mem->busAddress;
#ifdef USE_OLD_DRV
        vpi_h26xe_cfg->lum = (u8 *)vpi_h26xe_cfg->picture_mem->virtualAddress;
#else
        vpi_h26xe_cfg->lum =
            (u8 *)vpi_h26xe_cfg->picture_mem->rc_virtualAddress;
#endif

        p_enc_in->busChromaU = p_enc_in->busLuma + vpi_h26xe_cfg->luma_size;
        vpi_h26xe_cfg->cb    = vpi_h26xe_cfg->lum + vpi_h26xe_cfg->luma_size;

        p_enc_in->busChromaV =
            p_enc_in->busChromaU + vpi_h26xe_cfg->chroma_size / 2;
        vpi_h26xe_cfg->cr = vpi_h26xe_cfg->cb + vpi_h26xe_cfg->chroma_size / 2;
        if (0 != VCEncGetBitsPerPixel(options->input_format))
            src_img_size = options->lum_width_src * options->lum_height_src *
                           VCEncGetBitsPerPixel(options->input_format) / 8;
        else {
            if (options->input_format >= VCENC_YUV420_SEMIPLANAR_101010 &&
                options->input_format <= VCENC_YUV420_VU_10BIT_TILE_96_2)
                get_aligned_pic_size_byformat(options->input_format,
                                              options->lum_width_src,
                                              options->lum_height_src, 0, NULL,
                                              NULL, &src_img_size);
            else
#ifdef SUPPORT_TCACHE
                if (0 != get_bits_per_pixel(options->input_format))
                src_img_size = options->lum_width_src *
                               options->lum_height_src *
                               get_bits_per_pixel(options->input_format) / 8;
            else
#endif
                src_img_size =
                    vpi_h26xe_cfg->luma_size + vpi_h26xe_cfg->chroma_size;
        }
    }

    ENC_TB_DEBUG_PRINT("src_img_size = %d\n", src_img_size);

    if (options->lookahead_depth && options->cutree_blkratio) {
        u32 size_lum = 0;
        u32 size_ch  = 0;

        u32 alignment = (options->format_customized_type != -1 ?
                             0 :
                             vpi_h26xe_cfg->input_alignment);
        get_aligned_pic_size_byformat(options->input_format_ds,
                                      options->lum_widthsrc_ds,
                                      options->lum_heightsrc_ds, alignment,
                                      &size_lum, &size_ch, NULL);

        /* save full resolution yuv for 2nd pass */
        p_enc_in->busLumaOrig    = p_enc_in->busLuma;
        p_enc_in->busChromaUOrig = p_enc_in->busChromaU;
        p_enc_in->busChromaVOrig = p_enc_in->busChromaV;
        /* setup down-sampled yuv for 1nd pass */
        p_enc_in->busLuma = vpi_h26xe_cfg->picture_dsmem->busAddress;
#ifdef USE_OLD_DRV
        vpi_h26xe_cfg->lum_ds =
            (u8 *)vpi_h26xe_cfg->picture_dsmem->virtualAddress;
#else
        vpi_h26xe_cfg->lum_ds =
            (u8 *)vpi_h26xe_cfg->picture_dsmem->rc_virtualAddress;
#endif
        p_enc_in->busChromaU = p_enc_in->busLuma + size_lum;
        vpi_h26xe_cfg->cb_ds = vpi_h26xe_cfg->lum_ds + (u32)size_lum;
        p_enc_in->busChromaV = p_enc_in->busChromaU + (u32)size_ch / 2;
        vpi_h26xe_cfg->cr_ds = vpi_h26xe_cfg->cb_ds + (u32)size_ch / 2;
        if (0 != VCEncGetBitsPerPixel(options->input_format_ds))
            vpi_h26xe_cfg->src_img_size_ds =
                options->lum_widthsrc_ds * options->lum_heightsrc_ds *
                VCEncGetBitsPerPixel(options->input_format_ds) / 8;
        else {
            if (options->input_format_ds >= VCENC_YUV420_SEMIPLANAR_101010 &&
                options->input_format_ds <= VCENC_YUV420_VU_10BIT_TILE_96_2)
                get_aligned_pic_size_byformat(options->input_format_ds,
                                              options->lum_widthsrc_ds,
                                              options->lum_heightsrc_ds, 0,
                                              NULL, NULL,
                                              &vpi_h26xe_cfg->src_img_size_ds);
            else
#ifdef SUPPORT_TCACHE
                if (0 != get_bits_per_pixel(options->input_format_ds))
                vpi_h26xe_cfg->src_img_size_ds =
                    options->lum_widthsrc_ds * options->lum_heightsrc_ds *
                    get_bits_per_pixel(options->input_format_ds) / 8;
            else
#endif
                vpi_h26xe_cfg->src_img_size_ds =
                    vpi_h26xe_cfg->luma_size_ds + vpi_h26xe_cfg->chroma_size_ds;
        }

        ENC_TB_DEBUG_PRINT("vpi_h26xe_cfg->src_img_size_ds = %d\n",
                           vpi_h26xe_cfg->src_img_size_ds);

    } else {
        p_enc_in->busLumaOrig          = p_enc_in->busChromaUOrig =
            p_enc_in->busChromaVOrig   = (ptr_t)NULL;
        vpi_h26xe_cfg->src_img_size_ds = 0;
    }

    return src_img_size;
}

void setup_output_buffer(VCEncInst inst, VpiEncOutData *out_buffer,
                         VCEncIn *p_enc_in)
{
    p_enc_in->busOutBuf[0]  = out_buffer->outbuf_mem->busAddress;
    p_enc_in->outBufSize[0] = out_buffer->outbuf_mem->size;
    VCEncSetOutBusAddr(inst, out_buffer->outbuf_mem);
#ifdef USE_OLD_DRV
    p_enc_in->pOutBuf[0] = out_buffer->outbuf_mem->virtualAddress;
#else
    p_enc_in->pOutBuf[0] = out_buffer->outbuf_mem->rc_virtualAddress;
#endif
    VPILOGD("p_enc_in->busOutBuf[0] %x\n", (uint32_t)p_enc_in->busOutBuf[0]);
    VPILOGD("p_enc_in->pOutBuf[0] %p\n", p_enc_in->pOutBuf[0]);
}

static i32 setup_roi_map_ver3(VPIH26xEncCfg *tb,
                              VPIH26xEncOptions *options, VCEncInst encoder)
{
    u8 *memory;
    u8 u8RoiData[24];

    i32 blkSize;
    i32 num;
    i32 size;
    i32 ret = NOK;
    i32 block_num, i, j, m, n, block_width, block_height, mb_width,
        last_mb_height;
    i32 ctb_block_num, mb_block_num, block_cu8_rnum, block_cu8_cnum,
        last_block_cu8_cnum, block_cu8_num, read_size;
    u64 block_base_addr;
    u32 ctb_num, blkInCtb, blkRowInCtb, blkColumInCtb, cu8NumInCtb;

    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)encoder;

    /* V3 ROI*/
    if ((tb->roi_map_file) && (vcenc_instance->RoiQpDelta_ver < 3)) {
        /*RoiQpDelta_ver == 0,1,2*/
        if (copy_qp_delta_2_memory(options, encoder, tb,
                                   vcenc_instance->RoiQpDelta_ver))
            return NOK;
    } else if ((vcenc_instance->roiMapEnable) &&
               (tb->roi_map_info_bin_file != NULL)) {
        /*RoiQpDelta_ver == 1,2,3*/
#ifdef USE_OLD_DRV
        memory = (u8 *)tb->roi_map_delta_qp_mem->virtualAddress;
#else
        memory = (u8 *)tb->roi_map_delta_qp_mem->rc_virtualAddress;
#endif

        /* need fill into buffer of 8x8 block_size*/
        blkSize = 64 >> (options->roi_map_delta_qp_block_unit & 3);
        num     = h26x_enc_next_picture(tb, tb->enc_in.picture_cnt) + tb->first_pic;
        ret     = NOK;

        if (num <= tb->last_pic) {
            switch (options->roi_map_delta_qp_block_unit) {
            case 0:
                block_num = 64;
                break;
            case 1:
                block_num = 16;
                break;
            case 2:
                block_num = 4;
                break;
            case 3:
                block_num = 1;
                break;
            default:
                block_num = 64;
                break;
            }

            block_width = (((options->width + options->max_cu_size - 1) &
                            (~(options->max_cu_size - 1))) +
                           blkSize - 1) /
                          blkSize;
            mb_width = ((options->width + options->max_cu_size - 1) &
                        (~(options->max_cu_size - 1))) /
                       8;
            block_height = (((options->height + options->max_cu_size - 1) &
                             (~(options->max_cu_size - 1))) +
                            blkSize - 1) /
                           blkSize;
            size            = block_width * block_height;
            block_base_addr = ((u64)num) * ((u64)size);
            block_cu8_cnum  = blkSize / 8;
            last_block_cu8_cnum =
                (((options->width + options->max_cu_size - 1) &
                  (~(options->max_cu_size - 1))) -
                 (block_width - 1) * blkSize) /
                8;
            last_mb_height = (((options->height + options->max_cu_size - 1) &
                               (~(options->max_cu_size - 1))) -
                              (block_height - 1) * blkSize) /
                             16;

            if (IS_H264(options->codec_format)) {
                /*h264*/
                ctb_block_num  = blkSize / options->max_cu_size;
                block_cu8_rnum = block_cu8_cnum / ctb_block_num;
                cu8NumInCtb    = block_cu8_rnum * block_cu8_cnum;
                for (i = 0; i < size; i++) {
                    ret = file_read(tb->roi_map_info_bin_file, u8RoiData,
                                    (block_base_addr + i), 1);
                    if (ret == NOK)
                        break;

                    ctb_num  = i / block_width;
                    blkInCtb = i % block_width;

                    if (blkInCtb == (block_width - 1))
                        block_cu8_num = last_block_cu8_cnum;
                    else
                        block_cu8_num = block_cu8_cnum;

                    if (ctb_num == (block_height - 1))
                        mb_block_num = last_mb_height;
                    else
                        mb_block_num = ctb_block_num;

                    for (j = 0; j < mb_block_num; j++) {
                        for (m = 0; m < block_cu8_rnum; m++) {
                            for (n = 0; n < block_cu8_num; n++) {
                                memory[(ctb_num * ctb_block_num + j) *
                                           mb_width * block_cu8_rnum +
                                       blkInCtb * cu8NumInCtb +
                                       m * block_cu8_num + n] = u8RoiData[0];
                            }
                        }
                    }
                }
            } else {
                /* hevc*/
                ctb_block_num = options->max_cu_size / blkSize;
                cu8NumInCtb   = ctb_block_num * ctb_block_num * block_cu8_cnum *
                              block_cu8_cnum;

                for (i = 0; i < size; i++) {
                    ret = file_read(tb->roi_map_info_bin_file, u8RoiData,
                                    (block_base_addr + i), 1);
                    if (ret == NOK)
                        break;

                    ctb_num       = i / (ctb_block_num * ctb_block_num);
                    blkInCtb      = i % (ctb_block_num * ctb_block_num);
                    blkRowInCtb   = blkInCtb / ctb_block_num;
                    blkColumInCtb = blkInCtb % ctb_block_num;

                    for (m = 0; m < block_cu8_cnum; m++) {
                        for (n = 0; n < block_cu8_cnum; n++) {
                            memory[ctb_num * cu8NumInCtb +
                                   (blkRowInCtb * block_cu8_cnum + m) *
                                       ctb_block_num * block_cu8_cnum +
                                   blkColumInCtb * block_cu8_cnum + n] =
                                u8RoiData[0];
                        }
                    }
                }
            }
        }

        if (ret == NOK) {
#ifdef USE_OLD_DRV
            memset((u8 *)tb->roi_map_delta_qp_mem->virtualAddress, 0,
                   tb->roi_map_delta_qp_mem->size);
#else
            memset((u8 *)tb->roi_map_delta_qp_mem->rc_virtualAddress, 0,
                   tb->roi_map_delta_qp_mem->size);
#endif
        }
    }
    /*V3~V7 ROI index*/
    if ((vcenc_instance->RoimapCuCtrl_index_enable) &&
        (tb->roimap_cu_ctrl_index_bin_file != NULL)) {
#ifdef USE_OLD_DRV
        memory = (u8 *)tb->roimap_cu_ctrl_indexmem->virtualAddress;
#else
        memory = (u8 *)tb->roimap_cu_ctrl_indexmem->rc_virtualAddress;
#endif
        blkSize = IS_H264(options->codec_format) ? 16 : 64;
        num     = h26x_enc_next_picture(tb, tb->enc_in.picture_cnt) + tb->first_pic;
        size    = (((options->width + blkSize - 1) / blkSize) *
                    ((options->height + blkSize - 1) / blkSize) +
                7) >>
               3;
        ret = NOK;
        if (num <= tb->last_pic) {
            for (i = 0; i < size; i += read_size) {
                if ((size - i) > 1024)
                    read_size = 1024;
                else
                    read_size = size - i;

                ret = file_read(tb->roimap_cu_ctrl_index_bin_file,
                                (u8 *)(memory + i),
                                (((u64)num) * ((u64)size) + i), read_size);
                if (ret == NOK)
                    break;
            }
        }

        if (ret == NOK) {
#ifdef USE_OLD_DRV
            memset((u8 *)tb->roimap_cu_ctrl_indexmem->virtualAddress, 0,
                   tb->roimap_cu_ctrl_indexmem->size);
#else
            memset((u8 *)tb->roimap_cu_ctrl_indexmem->rc_virtualAddress, 0,
                   tb->roimap_cu_ctrl_indexmem->size);
#endif
        }
    }
    /*V3~V7 ROI*/
    if ((vcenc_instance->RoimapCuCtrl_enable) &&
        (tb->roimap_cu_ctrl_info_bin_file != NULL)) {
#ifdef USE_OLD_DRV
        memory = (u8 *)tb->roimap_cu_ctrl_infomem->virtualAddress;
#else
        memory = (u8 *)tb->roimap_cu_ctrl_infomem->rc_virtualAddress;
#endif

        blkSize = 8;
        num     = h26x_enc_next_picture(tb, tb->enc_in.picture_cnt) + tb->first_pic;

        block_width = (((options->width + options->max_cu_size - 1) &
                        (~(options->max_cu_size - 1))) +
                       blkSize - 1) /
                      blkSize;
        block_height = (((options->height + options->max_cu_size - 1) &
                         (~(options->max_cu_size - 1))) +
                        blkSize - 1) /
                       blkSize;
        size = block_width * block_height;
        ret  = NOK;
        switch (vcenc_instance->RoimapCuCtrl_ver) {
        case 3:
            block_num = 1;
            break;
        case 4:
            block_num = 2;
            break;
        case 5:
            block_num = 6;
            break;
        case 6:
            block_num = 12;
            break;
        default:
            block_num = 14;
            break;
        }

        size = size * block_num;
        if (num <= tb->last_pic) {
            for (i = 0; i < size; i += read_size) {
                if ((size - i) > 1024)
                    read_size = 1024;
                else
                    read_size = size - i;

                ret = file_read(tb->roimap_cu_ctrl_info_bin_file,
                                (u8 *)(memory + i),
                                (((u64)num) * ((u64)size) + i), read_size);
                if (ret == NOK)
                    break;
            }
        }

        if (ret == NOK) {
#ifdef USE_OLD_DRV
            memset((u8 *)tb->roimap_cu_ctrl_infomem->virtualAddress, 0,
                   tb->roimap_cu_ctrl_infomem->size);
#else
            memset((u8 *)tb->roimap_cu_ctrl_infomem->rc_virtualAddress, 0,
                   tb->roimap_cu_ctrl_infomem->size);
#endif
        }
    }

    return OK;
}

i32 setup_roi_map_buffer(VPIH26xEncCfg *tb, VPIH26xEncOptions *options,
                         VCEncIn *p_enc_in, VCEncInst encoder)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)encoder;
    p_enc_in->roiMapDeltaQpAddr = tb->roi_map_delta_qp_mem->busAddress;

    /* copy config data to memory. allocate delta qp map memory. */
    if ((options->lookahead_depth) && tb->roi_map_bin_file) {
#ifdef USE_OLD_DRV
        p_enc_in->pRoiMapDelta = (i8 *)tb->roi_map_delta_qp_mem->virtualAddress;
#else
        p_enc_in->pRoiMapDelta =
            (i8 *)tb->roi_map_delta_qp_mem->rc_virtualAddress;
#endif
        p_enc_in->roiMapDeltaSize = tb->roi_map_delta_qp_mem->size;
        i32 blkSize = 64 >> (options->roi_map_delta_qp_block_unit & 3);
        i32 num  = h26x_enc_next_picture(tb, tb->enc_in.picture_cnt) + tb->first_pic;
        i32 size = ((options->width + blkSize - 1) / blkSize) *
                   ((options->height + blkSize - 1) / blkSize);
        i32 ret = NOK;

        if (num <= tb->last_pic)
            ret = file_read(tb->roi_map_bin_file, (u8 *)p_enc_in->pRoiMapDelta,
                            ((u64)num) * ((u64)size), size);

        if (ret != OK)
            memset(p_enc_in->pRoiMapDelta, 0, size);
    }

    if (vcenc_instance->asic.regs.asicCfg.roiMapVersion == 3) {
        p_enc_in->RoimapCuCtrlAddr = tb->roimap_cu_ctrl_infomem->busAddress;
        p_enc_in->RoimapCuCtrlIndexAddr =
            tb->roimap_cu_ctrl_indexmem->busAddress;
        if (setup_roi_map_ver3(tb, options, encoder))
            return NOK;
    } else if (tb->roi_map_file) {
        if (copy_qp_delta_2_memory(options, encoder, tb,
                                   vcenc_instance->asic.regs.asicCfg
                                       .roiMapVersion))
            return NOK;
    }

    if (tb->ipcm_map_file || tb->skip_map_file) {
        if (copy_flags_map_2_memory(options, encoder, tb))
            return NOK;
    }

    return OK;
}

FILE *format_customized_yuv(VPIH26xEncCfg *tb,
                            VPIH26xEncOptions *options, i32 *ret)
{
    *ret = OK;
    if ((options->format_customized_type == 1) &&
        ((options->input_format == VCENC_YUV420_SEMIPLANAR) ||
         (options->input_format == VCENC_YUV420_SEMIPLANAR_VU) ||
         (options->input_format == VCENC_YUV420_PLANAR_10BIT_P010))) {
        trans_yuv_to_fbformat(tb, options, ret);
    }

    return NULL;
}

void get_free_iobuffer(VPIH26xEncCfg *tb)
{
    i32 i_buf;

    /*find output buffer of multi-cores*/
    tb->picture_mem =
        &(tb->picture_mem_factory[tb->picture_enc_cnt % tb->buffer_cnt]);
    tb->picture_dsmem =
        &(tb->picture_dsmem_factory[tb->picture_enc_cnt % tb->buffer_cnt]);

    /*find output buffer of multi-cores*/
    for (i_buf = 0; i_buf < tb->stream_buf_num; i_buf++)
        tb->outbuf_mem[i_buf] =
            &(tb->outbuf_mem_factory[tb->picture_enc_cnt %
                                     tb->parallel_core_num][i_buf]);

    /*find ROI Map buffer of multi-cores*/
    tb->roi_map_delta_qp_mem = &(
        tb->roi_map_delta_qpmem_factory[tb->picture_enc_cnt % tb->buffer_cnt]);

    /*find transform buffer of multi-cores*/
    tb->transform_mem =
        &(tb->transform_mem_factory[tb->picture_enc_cnt % tb->buffer_cnt]);
    tb->roimap_cu_ctrl_infomem =
        &(tb->roimap_cu_ctrl_infomem_factory[tb->picture_enc_cnt %
                                             tb->buffer_cnt]);
    tb->roimap_cu_ctrl_indexmem =
        &(tb->roimap_cu_ctrl_indexmem_factory[tb->picture_enc_cnt %
                                              tb->buffer_cnt]);

#if defined(SUPPORT_DEC400) || defined(SUPPORT_TCACHE)
    tb->ts_lumamem =
        &(tb->ts_lumamem_factory[tb->picture_enc_cnt % tb->buffer_cnt]);
    tb->ts_chromamem =
        &(tb->ts_chromamem_factory[tb->picture_enc_cnt % tb->buffer_cnt]);
#endif
}

void init_slice_ctl(VPIH26xEncCfg *tb, VPIH26xEncOptions *options)
{
    int i;
    for (i = 0; i < MAX_CORE_NUM; i++) {
        tb->slice_ctl_factory[i].multislice_encoding =
            (options->slice_size != 0 &&
             (((options->height + 63) / 64) > options->slice_size)) ?
                1 :
                0;
        tb->slice_ctl_factory[i].output_byte_stream =
            options->byte_stream ? 1 : 0;
        tb->slice_ctl_factory[i].out_stream_file = tb->out;
        tb->slice_ctl_factory[i].stream_pos      = 0;
    }
}

void init_stream_segment_crl(VPIH26xEncCfg *tb, VPIH26xEncOptions *options)
{
    tb->stream_seg_ctl.stream_rd_counter = 0;
    tb->stream_seg_ctl.stream_multi_seg_en =
        options->stream_multi_segment_mode != 0;
#ifdef USE_OLD_DRV
    tb->stream_seg_ctl.stream_base =
        (u8 *)tb->outbuf_mem_factory[0][0].virtualAddress;
#else
    tb->stream_seg_ctl.stream_base =
        (u8 *)tb->outbuf_mem_factory[0][0].rc_virtualAddress;
#endif

    if (tb->stream_seg_ctl.stream_multi_seg_en) {
        tb->stream_seg_ctl.segment_size = tb->outbuf_mem_factory[0][0].size /
                                          options->stream_multi_segment_amount;
        tb->stream_seg_ctl.segment_size =
            ((tb->stream_seg_ctl.segment_size + 16 - 1) &
             (~(16 - 1))); /*segment size must be aligned to 16byte*/
        tb->stream_seg_ctl.segment_amount =
            options->stream_multi_segment_amount;
    }
    tb->stream_seg_ctl.start_code_done    = 0;
    tb->stream_seg_ctl.output_byte_stream = tb->byte_stream;
    tb->stream_seg_ctl.out_stream_file    = tb->out;
}

void setup_slice_ctl(VPIH26xEncCfg *tb)
{
    /*find transform buffer of multi-cores*/
    tb->slice_ctl =
        &(tb->slice_ctl_factory[tb->picture_enc_cnt % tb->parallel_core_num]);
    tb->slice_ctl_out = &(tb->slice_ctl_factory[(tb->picture_enc_cnt + 1) %
                                                tb->parallel_core_num]);
}

/* Helper function to calculate time diffs.*/
unsigned int uTimeDiff(struct timeval end, struct timeval start)
{
    return (end.tv_sec - start.tv_sec) * 1000000 +
           (end.tv_usec - start.tv_usec);
}

void h26x_enc_outbuf_init(VpiH26xEncCtx *enc_ctx)
{
    VPIH26xEncCfg *cfg = (VPIH26xEncCfg *)&enc_ctx->vpi_h26xe_cfg;
    EWLLinearMem_t *mem;
    int i;

    for (i = 0; i < MAX_OUT_BUF_NUM; i++) {
        memset(&enc_ctx->enc_pkt[i], 0, sizeof(VpiEncOutData));
        enc_ctx->enc_pkt[i].outbuf_mem = &(cfg->outbuf_mem_factory[cfg->picture_enc_cnt %
                                                 cfg->parallel_core_num][i]);
    }

    for (i = 0; i < enc_ctx->outstrm_num; i++) {
        memset(&enc_ctx->outstream_pkt[i], 0, sizeof(VpiEncOutData));
        mem = malloc(sizeof(EWLLinearMem_t));
        mem->rc_busAddress = (ptr_t)enc_ctx->outstream_mem[i];
        mem->size = DEFAULT_OUT_STRM_BUF_SIZE;
        enc_ctx->outstream_pkt[i].outbuf_mem = mem;
    }
}

void h26x_enc_outbuf_uninit(VpiH26xEncCtx *enc_ctx)
{
    int i;

    for (i = 0; i < enc_ctx->outstrm_num; i++) {
        if (enc_ctx->outstream_pkt[i].outbuf_mem)
            free(enc_ctx->outstream_pkt[i].outbuf_mem);
    }
}

int h26x_enc_get_empty_stream_buffer(VpiH26xEncCtx *ctx)
{
    int i;
    EWLLinearMem_t *mem;

    for (i = 0; i < ctx->outstrm_num; i++) {
        if (ctx->stream_buf_list[i]->used == 0) {
            VPILOGD("get i %d\n", i);
            return i;
        }
    }
    VPILOGD("current ctx->outstrm_num %d\n", ctx->outstrm_num);
    if (ctx->outstrm_num < MAX_OUTPUT_FIFO_DEPTH) {
        i = ctx->outstrm_num;
        ctx->outstream_mem[i] =
            fbtrans_get_huge_pages(DEFAULT_OUT_STRM_BUF_SIZE);
        if (ctx->outstream_mem[i] == NULL) {
            VPILOGE("Failed to allocate output buffer size!\n");
            return -1;
        }
        memset(&ctx->outstream_pkt[i], 0, sizeof(VpiEncOutData));
        mem = malloc(sizeof(EWLLinearMem_t));
        mem->rc_busAddress = (ptr_t)ctx->outstream_mem[i];
        mem->size = DEFAULT_OUT_STRM_BUF_SIZE;
        ctx->outstream_pkt[i].outbuf_mem = mem;
        ctx->outstrm_num++;
        return i;
    } else {
        return -1;
    }
}

int h26x_enc_get_pic_buffer(VpiH26xEncCtx *ctx, void *outdata)
{
    VpiFrame **frame;
    VpiEncH26xPic *trans_pic = NULL;
    int status, i;
    int input_cnt;

    frame = (VpiFrame **)outdata;
    for (i = 0; i < MAX_WAIT_DEPTH; i++) {
        pthread_mutex_lock(&ctx->pic_wait_list[i].pic_mutex);
        if (ctx->pic_wait_list[i].state == 0) {
            trans_pic = &ctx->pic_wait_list[i];
            pthread_mutex_unlock(&trans_pic->pic_mutex);
            break;
        } else {
            pthread_mutex_unlock(&ctx->pic_wait_list[i].pic_mutex);
        }
    }
    if (i == MAX_WAIT_DEPTH) {
        *frame = NULL;
        return -1;
    }
    if (trans_pic->pic == NULL) {
        trans_pic->pic = malloc(sizeof(VpiFrame));
    }
    *frame = trans_pic->pic;
    return 0;
}

int h26x_enc_get_frame_packet(VpiH26xEncCtx *ctx, void *outdata)
{
    int stream_size;
    int ret;
    VpiEncOutData *out_buf = NULL;
    H26xEncBufLink *buf    = NULL;

    pthread_mutex_lock(&ctx->h26xe_thd_mutex);
    while (1) {
        buf = ctx->stream_buf_head;
        if (buf) {
            out_buf = (VpiEncOutData *)buf->item;
            if (out_buf->end_data == HANTRO_TRUE) {
                pthread_mutex_unlock(&ctx->h26xe_thd_mutex);
                return 1;
            }
            stream_size = out_buf->resend_header ? out_buf->stream_size :
                                (out_buf->stream_size + out_buf->header_size);
            *(int *)outdata = stream_size;
            pthread_mutex_unlock(&ctx->h26xe_thd_mutex);
            return 0;
        } else if (buf == NULL) {
            if (ctx->encode_end == 1) {
                pthread_mutex_unlock(&ctx->h26xe_thd_mutex);
                return 1;
            } else {
                if (ctx->eos_received == 0) {
                    if (ctx->got_frame) {
                        ctx->waiting_for_pkt = 1;
                        pthread_cond_wait(&ctx->h26xe_thd_cond,
                                          &ctx->h26xe_thd_mutex);
                        continue;
                    } else {
                        pthread_mutex_unlock(&ctx->h26xe_thd_mutex);
                        return -1;
                    }
                } else {
                    ctx->waiting_for_pkt = 1;
                    pthread_cond_wait(&ctx->h26xe_thd_cond,
                                      &ctx->h26xe_thd_mutex);
                    continue;
                }
            }
        }
    }
    pthread_mutex_unlock(&ctx->h26xe_thd_mutex);
    return 0;
}

void h26x_enc_buf_list_add(H26xEncBufLink **head, H26xEncBufLink *list)
{
    H26xEncBufLink *temp;

    if(NULL == *head) {
        *head = list;
        (*head)->next = NULL;
    } else {
        temp = *head;
        while(temp) {
            if(NULL == temp->next) {
                temp->next = list;
                list->next = NULL;
                return;
            }
            temp = temp->next;
        }
    }
}

H26xEncBufLink* h26x_enc_buf_list_delete(H26xEncBufLink *head)
{
    if (NULL == head || NULL == head->next) {
        return NULL;
    }

    return head->next;
}

int h26x_enc_get_used_pic_mem(VpiH26xEncCtx *ctx, void *mem)
{
    VpiBufRef **ref;

    pthread_mutex_lock(&ctx->h26xe_thd_mutex);
    ref = (VpiBufRef **)mem;

    if (ctx->rls_pic_head) {
        *ref = (VpiBufRef *)ctx->rls_pic_head->item;
        ctx->rls_pic_head->used = 0;
        ctx->rls_pic_head =
            h26x_enc_buf_list_delete(ctx->rls_pic_head);
    } else {
        *ref = NULL;
    }

    pthread_mutex_unlock(&ctx->h26xe_thd_mutex);
    return 0;
}

void h26x_enc_consume_pic(VpiH26xEncCtx *ctx, int consume_poc)
{
    VpiEncH26xPic * trans_pic = NULL;
    VpiFrame *in_vpi_frame    = NULL;
    int i;

    //find the need_poc
    VPILOGD("%x consume pic poc %d\n", ctx, consume_poc);
    for (i = 0; i < MAX_WAIT_DEPTH; i++) {
        pthread_mutex_lock(&ctx->pic_wait_list[i].pic_mutex);
        if (ctx->pic_wait_list[i].state == 1) {
            trans_pic = &ctx->pic_wait_list[i];
            if (trans_pic->poc == consume_poc) {
                pthread_mutex_unlock(&trans_pic->pic_mutex);
                break;
            } else {
                pthread_mutex_unlock(&trans_pic->pic_mutex);
            }
        } else {
            pthread_mutex_unlock(&ctx->pic_wait_list[i].pic_mutex);
        }
    }
    if (i == MAX_WAIT_DEPTH) {
        return;
    }

    pthread_mutex_lock(&trans_pic->pic_mutex);
    trans_pic->poc               = -1;
    trans_pic->state             = 0;
    trans_pic->used              = 0;
    trans_pic->in_pass_one_queue = 0;

    in_vpi_frame = (VpiFrame *)trans_pic->pic->vpi_opaque;
    for (i = 0; i < MAX_WAIT_DEPTH; i++) {
        if (ctx->rls_pic_list[i]->used == 0) {
            ctx->rls_pic_list[i]->item = trans_pic->pic->opaque;
            ctx->rls_pic_list[i]->used = 1;
            break;
        }
    }
    if (i == MAX_WAIT_DEPTH) {
        pthread_mutex_unlock(&trans_pic->pic_mutex);
        return;
    }
    pthread_mutex_unlock(&trans_pic->pic_mutex);

    pthread_mutex_lock(&in_vpi_frame->frame_mutex);
    in_vpi_frame->used_cnt++;
    pthread_mutex_unlock(&in_vpi_frame->frame_mutex);

    if (ctx->force_idr) {
        pthread_mutex_lock(&ctx->h26xe_thd_mutex);
        ctx->inject_frm_cnt--;
        pthread_mutex_unlock(&ctx->h26xe_thd_mutex);
    }

    pthread_mutex_lock(&ctx->h26xe_thd_mutex);

    h26x_enc_buf_list_add(&ctx->rls_pic_head, ctx->rls_pic_list[i]);
    pthread_mutex_unlock(&ctx->h26xe_thd_mutex);

}

int h26x_enc_get_extradata_size(VpiH26xEncCtx *ctx, void *outdata)
{
    u32 *size = (u32 *)outdata;

    *size = ctx->header_size;
    return 0;
}

int h26x_enc_get_extradata(VpiH26xEncCtx *ctx, void *data)
{
    u8 *header_data = (u8 *)data;

    memcpy(header_data, ctx->header_data, ctx->header_size);
    return 0;
}