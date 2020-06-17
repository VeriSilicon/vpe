/*
 * Copyright (c) 2020, VeriSilicon Inc. All rights reserved
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "vpi_error.h"
#include "vpi_log.h"
#include "vpi_log_manager.h"

static FILE *report_file;
static int report_file_level = LOG_LEVEL_DBG;

#define MAX_LOG_BUF_SIZE (4096)

void log_print(const char *p_format, ...)
{
    char buf[MAX_LOG_BUF_SIZE] = { 0 };
    va_list ap;

    if (!p_format)
        return;

    va_start(ap, p_format);
    vsnprintf(buf, MAX_LOG_BUF_SIZE, p_format, ap);
    va_end(ap);
    printf("%s", buf);
}

void log_write(LogLevel level, const char *p_header, const char *p_format, ...)
{
    va_list ap;
    char buf[MAX_LOG_BUF_SIZE] = { 0 };
    if (p_header) {
        strncpy(buf, p_header, MAX_LOG_BUF_SIZE);
    }

    if (strlen(buf) == MAX_LOG_BUF_SIZE) {
        return;
    }

    va_start(ap, p_format);
    vsnprintf(&buf[strlen(buf)], MAX_LOG_BUF_SIZE - strlen(buf), p_format, ap);
    va_end(ap);

    if (report_file_level >= level) {
        fputs(buf, report_file);
        fflush(report_file);
    }
}

int log_open(char *file_name)
{
    if (NULL == file_name) {
        return -1;
    }

    report_file = fopen(file_name, "w");
    if (!report_file) {
        printf("Failed to open report \"%s\"\n", file_name);
        return -1;
    }
    return 0;
}

void log_close()
{
    fclose(report_file);
}
