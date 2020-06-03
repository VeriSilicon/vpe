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
