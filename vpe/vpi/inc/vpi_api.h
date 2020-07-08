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

#ifndef __VPI_API_H__
#define __VPI_API_H__

/*
 * provides application programming interface for the application layer.
 */

#include "vpi_types.h"

/**
 * @brief free pointer
 * @param arg pointer waiting to be freed
 */
void vpi_freep(void *arg);

/**
 * @brief Create system info structure
 * @param sys_info pointer of the system info
 */

int vpi_get_sys_info_struct(VpiSysInfo **sys_info);

/**
 * @brief Create media process structure
 * @param sys_info pointer of the media process structure
 */
int vpi_get_media_proc_struct(VpiMediaProc **media_proc);

/**
 * @brief Create empty context structure and api function pointers.
 *        Use functions in VpiApi to access vpi services.
 * @param ctx pointer of the vpi context
 * @param vpi pointer of the vpi api function
 * @param plugin vpi plugin type
 */

int vpi_create(VpiCtx *ctx, VpiApi **vpi, VpiPlugin plugin);

/**
 * @brief Destroy vpi context and free both context and vpi structure
 * @param ctx The context of vpi
 */

int vpi_destroy(VpiCtx ctx);

/**
 * @brief Open vpi hardware device
 * @param device The hardware device path
 */

int vpi_open_hwdevice(const char *device);

/**
 * @brief Close vpi hardware device
 * @param fd The device handle
 */

int vpi_close_hwdevice(int fd);

#endif
