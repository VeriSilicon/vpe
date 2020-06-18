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
