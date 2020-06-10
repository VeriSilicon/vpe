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
 *        Use functions in VpiApi to access vpe services.
 * @param ctx pointer of the vpe context
 * @param vpi pointer of the vpe api function
 * @param plugin vpe plugin type
 */

int vpi_create(VpiCtx *ctx, VpiApi **vpi, VpiPlugin plugin);

/**
 * @brief Destroy vpe context and free both context and vpi structure
 * @param ctx The context of vpe
 */

int vpi_destroy(VpiCtx ctx);

#endif
