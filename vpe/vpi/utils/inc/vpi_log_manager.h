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

#ifndef __VPI_LOG_MANAGER_H__
#define __VPI_LOG_MANAGER_H__

#include "vpi_log.h"

#ifdef __cplusplus
extern "C" {
#endif

int log_open(char *);

void log_close();

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VPI_LOG_MANAGER_H__ */
