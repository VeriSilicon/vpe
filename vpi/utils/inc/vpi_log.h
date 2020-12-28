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

#ifndef __VPI_LOG_H__
#define __VPI_LOG_H__

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VPILOGC
#define VPILOGC(...) log_print(__VA_ARGS__)
#endif

#ifndef VPILOGE
#define VPILOGE(...) VPILOG(LOG_LEVEL_ERR, __VA_ARGS__)
#endif

#ifndef VPILOGW
#define VPILOGW(...) VPILOG(LOG_LEVEL_WAR, __VA_ARGS__)
#endif

#ifndef VPILOGI
#define VPILOGI(...) VPILOG(LOG_LEVEL_INF, __VA_ARGS__)
#endif

#ifndef VPILOGD
#define VPILOGD(...) VPILOG(LOG_LEVEL_DBG, __VA_ARGS__)
#endif

#ifndef VPILOGV
#define VPILOGV(...) VPILOG(LOG_LEVEL_VER, __VA_ARGS__)
#endif

#ifndef TRACE
#define TRACE_ENTER() VPILOGV("enter")
#define TRACE_LEAVE() VPILOGV("leave")
#define TRACE() VPILOGV("trace")
#endif

typedef enum {
    LOG_LEVEL_OFF = 0, // off
    LOG_LEVEL_ERR = 3, // error
    LOG_LEVEL_WAR = 4, // warning
    LOG_LEVEL_INF = 5, // information
    LOG_LEVEL_DBG = 6, // debug
    LOG_LEVEL_VER = 7, // verbose
    LOG_LEVEL_MAX
} LogLevel;

extern int report_file_level;

#ifndef VPILOG
#define MAX_LOG_HEADER_SIZE (512)
#define VPILOG(level, ...)                                                     \
    do {                                                                       \
        char header[MAX_LOG_HEADER_SIZE] = { 0 };                              \
        if (report_file_level >= level) {                                      \
            snprintf(header, MAX_LOG_HEADER_SIZE, "%s(%d):", __FUNCTION__,     \
                    __LINE__);                                                 \
            log_write(level, header, __VA_ARGS__);                             \
        }                                                                      \
    } while (0)
#endif

void log_print(const char *, ...);

void log_write(LogLevel level, const char *, const char *, ...);

#ifdef __cplusplus
}
#endif

#endif
