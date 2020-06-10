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

#ifndef __VPI_LOG_H__
#define __VPI_LOG_H__

#include <stdio.h>

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
    LOG_LEVEL_OFF, // off
    LOG_LEVEL_ERR, // error
    LOG_LEVEL_WAR, // warning
    LOG_LEVEL_INF, // information
    LOG_LEVEL_DBG, // debug
    LOG_LEVEL_VER, // verbose
    LOG_LEVEL_MAX
} LogLevel;

#ifndef VPILOG
#define MAX_LOG_HEADER_SIZE (512)
#define VPILOG(level, ...)                                                     \
    do {                                                                       \
        char header[MAX_LOG_HEADER_SIZE] = { 0 };                              \
        snprintf(header, MAX_LOG_HEADER_SIZE, "%s(%d):", __FUNCTION__,         \
                 __LINE__);                                                    \
        log_write(level, header, __VA_ARGS__);                                 \
    } while (0)
#endif

void log_print(const char *, ...);

void log_write(LogLevel level, const char *, const char *, ...);

#ifdef __cplusplus
}
#endif

#endif
