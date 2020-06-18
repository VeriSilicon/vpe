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

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "base_type.h"
#include "vpi_video_enc_common.h"
#include "vpi_error.h"
#include "vpi_log.h"

/**
 * Parse the string to get the parameter name and value
 * The format of the input string is "NAMEx1=VALUEy1:NAMEx2=VALUEy2" or
 * "NAMEx1=VALUEy1:NAMEx2='VALUEy21:VALUEy22'". After parsing, the NAMEx is
 * output by param_name and the VALUEy or VALUEy1:VALUEy2 is output by param_val
 *
 * @param string The input string for parsing. The format is "NAMEx=VALUEy:",
 * "NAMEx=VALUEy" or "NAMEx="VALUEy1:VALUEy2"
 * @param param_name The output of the NAMEx in the string. strlen(param_name)
 * is 0 means no valid param_name
 * @param param_val The output of the VALUEy in the string. strlen(param_val) is
 * 0 means no valid param_val
 * @return str The end location of the string. strlen(str) is 0 means parsing to
 * the end of the string.
 */
char *vpi_enc_get_paraname_paravalue(char *string, char *param_name,
                                     char *param_val)
{
    int equal_pos = 0, colon_pos = 0, single_quote_start = 0, single_quote_end = 0, i = 0;
    char *str = string;

    for (i = 0; i < strlen(string); i++) {
        equal_pos = strspn(str, "=");
        if (equal_pos != 0)
            break;
        str++;
    }
    /*The "=" is at the end of the string. Or there is no "=" in the string.
     * Both areillegal */
    if (i >= (strlen(string) - 1)) {
        param_name[0] = '\0';
        param_val[0]  = '\0';
        return str;
    }
    equal_pos = i; /*equal_pos is the offset of the "=" at the string */
    strncpy(param_name, string, equal_pos); /*copy the name string before "=" */
    param_name[equal_pos] = '\0';

    str = string;
    for (i = 0; i < strlen(string); i++) {
        /*For NAMEx='VALUEy1:VALUEy2'*/
        if(single_quote_start == 0){
            if(strspn(str, "'"))
                single_quote_start = i; /*Get the start location of single quote*/
        }else {
            if(strspn(str, "'")){
                single_quote_end = i; /*Get the end location of single quote*/
                break;
            }
        }

        /*For the : to separate name=value pair*/
        colon_pos = strspn(str, ":");
        if (colon_pos != 0 && single_quote_start == 0){ /*no single quote in front of ':'*/
            /*The colon_pos is the offset of the ":" at the string */
            colon_pos = i;
            VPILOGD(":%s,%d,i = %d\n",__FUNCTION__,__LINE__,i);
            break;
        }
        str++;
    }
    if((colon_pos == 0) && (single_quote_start == 0)){
            VPILOGD("%s,%d,i = %d, reach to the end\n",__FUNCTION__,__LINE__,i);
            colon_pos = i;
     }

    if(single_quote_start == 0 && single_quote_end == 0){
        /* copy the value string between "=" and ":". "string+equal_pos+1" is the
           next char after "=", it is also the first char of the value."colon_pos-1"
           is the lastest char before ":",equal_pos is the the offset of the "=". So
           "colon_pos-equal_pos-1" is the length of the value */
        memcpy(param_val, string + equal_pos + 1, colon_pos - equal_pos - 1);
        param_val[colon_pos - equal_pos - 1] = '\0';
    } else {
        /* copy the value of NAMEx='VALUEy1:VALUEy2' case. single_quote_start + 1 is the location
         * of 'V' and "single_quote_end - single_quote_start - 1" is len of VALUEy1:VALUEy2.
         */
        memcpy(param_val, string + single_quote_start + 1, single_quote_end - single_quote_start - 1);
        param_val[single_quote_end - single_quote_start - 1] = '\0';
        VPILOGD("param_name:%s param_val:%s, str:%s, strlen(str):%ld\n",param_name, param_val,str,strlen(str));
        if(strlen(str) > 1)
            str++; /*This name/value pair isn't at the end of the string. str++ jump to the back of '''*/
    }
    return str;
}

/**
 * Parse the string and split it into separated segments according to
 * split_char. Such as the string is value0:value1, the split char is ':',
 * the max_segs is 2. Sperates it a to seperated value0,value1
 *
 * @param out_str The separated strings for output
 * @param max_segs The max segments number in input_str
 * @param input_str The input string for parse
 * @param split_char The char to seperate input_str
 */
int vpi_enc_split_string(char ** out_str, int max_segs, char * input_str, char * split_char)
{
    int count = 0, i = 0, non_split_char = 0;
    char * p = NULL;
    char c = '\0';
    char * currp = input_str;

    VPILOGD("The string is %s, split_char is %s \n", input_str,split_char);
    while ((c = *currp++) != '\0') {
        if ((p = strchr(split_char, c)) == NULL) {
            if (count < max_segs) {
                out_str[count][i++] = c;
            } else {
                VPILOGE("the split_char count exceeds max num\n");
                return -1;
            }
            non_split_char = 1; /*1 means non split_char char, 0 means split_char char*/
        } else {
            if (non_split_char == 1) {
                out_str[count][i] = '\0';
                count++;
                i = 0;
            }
            non_split_char = 0; /*1 means non split_char char, 0 means split_char char*/
        }
    }
    if (non_split_char == 1) {
        out_str[count][i] = '\0';
        count++;
    }

    return count;
}

static int set_value(VpiEncSetting *op, void *target, char *val)
{
    int ret      = 0;
    int vali     = 0;
    float valf   = 0;

    switch (op->type) {
    case VPI_ENC_PARA_INT:
        vali = atoi(val);
        if (vali < op->min || vali > op->max) {
            VPILOGD("value[%d] for [%s] is not valid: [%ld-%ld]\n",
                    op->name, vali, op->min, op->max);
            return VPI_ERR_VALUE;
        }
        *(int *) target = vali;
        VPILOGD("key matched: [%s]='%d'\n", op->name, vali);
        break;

    case VPI_ENC_PARA_FLOAT:
        valf = atof(val);
        if (valf < op->min || valf > op->max) {
            VPILOGD("value[%d] for [%s] is not valid: [%f-%f]\n",
                    op->name, valf, op->min, op->max);
            return VPI_ERR_VALUE;
        }
        *(float *) target = valf;
        VPILOGD("key matched: [%s]='%f'\n", op->name, valf);
        break;

    case VPI_ENC_PARA_STRING:
         *(char **) target = val;
        VPILOGD("key matched: [%s]='%s'\n", op->name, val);
        break;

    case VPI_ENC_PARA_COLON2:
    default:
        break;
    }
    return ret;
}

/*
 * This function was used to parse long string based parameters
 * into structure
 *
 * Input: string combined with "NAMEx=VALUEy:", for example below string:
 * "bitrate_window=180:intrs_pic_rate=60"
 *
 * Output: pointer to encoder setting structure, like VpiEncVp9Setting
 */
int vpi_enc_parse_param(char *src, VpiEncSetting setting[], int length,
                        void *output)
{
    int ret     = 0;
    char *p     = src;
    char name[255] = { "\0" };
    char val[255]  = { "\0" };
    int i = 0;
    VpiEncSetting *op = NULL;

    if (setting == NULL || output == NULL) {
        return -1;
    }

    while (1) {
        /* Step 1, get key pair */
        p = vpi_enc_get_paraname_paravalue(p, name, val);
        if (strlen(name) == 0 || strlen(val) == 0)
            break;

        /* Step 2, Match name */
        for (i = 0; i < length; i++) {
            if (!strcmp(setting[i].name, name)) {
                /*Matched */
                op = &setting[i];
                break;
            }
        }

        /* Step 3, get value */
        if (i == length) {
            VPILOGE("Can't find opition %s\n", name);
            return VPI_ERR_VALUE;
        }

        /* Step 4, save value*/
        ret = set_value(op, output + op->offset, val);

        if (strlen(p++) == 0) {
            break;
        }
    }
    return ret;
}

int vpi_enc_set_param(char *key, char *val, VpiEncSetting setting[], int length,
                      void *output)
{
    VpiEncSetting *op = NULL;
    int ret           = 0;
    int i             = 0;

    if (setting == NULL || output == NULL) {
        return -1;
    }

    if (strlen(key) == 0 || strlen(val) == 0)
        return 0;

    for (i = 0; i < length; i++) {
        if (!strcmp(setting[i].name, key)) {
            op = &setting[i];
            break;
        }
    }

    if (i == length) {
        VPILOGE("Can't find opition %s\n", key);
        return VPI_ERR_INVALID_PARAM;
    }

    return set_value(op, output + op->offset, val);
}
