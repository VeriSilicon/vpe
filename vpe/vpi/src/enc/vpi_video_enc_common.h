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

#ifndef __ENC_COMMON__
#define __ENC_COMMON__

#include "vpi_types.h"

#define ENC_PARAMS_NAME_MAX_LEN 100 /*The max length of params in enc_params*/
#define ENC_PARAMS_VALUE_MAX_LEN 80 /*The max length of value in enc_params*/
#define ENC_PARAM_MAX_SEG_NUM 4 /*The max segments between two single quotes*/
#define ENC_PARAM_MAX_SEG_LEN 20 /*The max length of each segment*/

typedef enum {
    VPI_ENC_PARA_INT,
    VPI_ENC_PARA_FLOAT,
    VPI_ENC_PARA_STRING,
    VPI_ENC_PARA_COLON2,
} VpiParaType;

typedef struct {
    const char *name;
    int offset;
    double min;
    double max;
    VpiParaType type;
} VpiEncSetting;

char *vpi_enc_get_paraname_paravalue(char *string, char *param_name,
                                     char *param_val);
int vpi_enc_split_string(char ** tgt, int max, char * src, char * split);
int vpi_enc_parse_param(char *src, VpiEncSetting setting[], int length,
                        void *output);
int vpi_enc_set_param(char *key, char *val, VpiEncSetting setting[], int length,
                      void *output);

#endif
