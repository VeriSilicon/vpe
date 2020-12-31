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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>

#define DEV_NAME_PREFIX "transcoder"
#define INFO_PATH_PREFIX "/sys/class/misc/transcoder"
#define DEC_UTIL "dec_util"
#define ENC_UTIL "enc_util"
#define DEC_CORE_STATUS "dec_core_status"
#define ENC_CORE_STATUS "enc_core_status"
#define MEM_USAGE "mem_info"
#define POWER_STATUS "power_state"

typedef enum SrmMode {
    SRM_BALANCE   = 0,
    SRM_LOW_POWER = 1,
} SrmMode;

typedef enum {
    SRM_IDLE     = 0,
    SRM_RESERVED = 1,
} SrmCoreStatus;

typedef struct SrmDecCoreStatus {
    int core0;
    int core1;
    int core2;
    int core3;
} SrmDecCoreStatus;

typedef struct DriverStatus {
    int device_id;
    int dec_usage;
    int enc_usage;
    int mem_usage;
    int used_mem;
    int free_mem;
    int power_state;
    SrmDecCoreStatus dec_core;
    SrmDecCoreStatus enc_core;
} SrmDriverStatus;

typedef struct SrmContext {
    SrmDriverStatus *driver_status;
    int driver_nums;
} SrmContext;

static char *mode_name(SrmMode mode)
{
    if (mode == SRM_LOW_POWER)
        return "Low Power Mode";
    if (mode == SRM_BALANCE)
        return "Balance Mode";
    return NULL;
}

static int get_device_numbers(void)
{
    struct dirent **namelist = NULL;
    int count                = 0;
    int n;

    n = scandir("/dev/", &namelist, 0, alphasort);
    while (n--) {
        if (strncmp(namelist[n]->d_name, DEV_NAME_PREFIX,
                    strlen(DEV_NAME_PREFIX)) == 0) {
            printf("Scanned device '%s'\n", namelist[n]->d_name);
            count++;
        }
        free(namelist[n]);
    }

    free(namelist);
    return count;
}

static int get_power_state(int device_id, SrmDriverStatus *status)
{
    char file[255];
    FILE *fp;

    sprintf(file, "%s%d/%s", INFO_PATH_PREFIX, device_id, POWER_STATUS);
    fp = fopen(file, "r");
    if (fp == NULL) {
        printf("get_power_state can't open file %s\n", file);
        return -1;
    }
    fscanf(fp, "%d", &status->power_state);
    fclose(fp);

    return 0;
}

static int get_dec_core_status(int device_id, SrmDriverStatus *status)
{
    char file[255];
    char s0[255];
    char s1[255];
    FILE *fp;

    sprintf(file, "%s%d/%s", INFO_PATH_PREFIX, device_id, DEC_CORE_STATUS);
    fp = fopen(file, "r");
    if (fp == NULL) {
        printf("get_dec_core_status can't open file %s\n", file);
        return -1;
    }

    fscanf(fp, "%s  %s", s0, s1);
    if (strstr(s0, "core:0")) {
        if (strstr(s1, "idle"))
            status->dec_core.core0 = SRM_IDLE;
        else
            status->dec_core.core0 = SRM_RESERVED;
    }

    fscanf(fp, "%s  %s", s0, s1);
    if (strstr(s0, "core:1")) {
        if (strstr(s1, "idle"))
            status->dec_core.core1 = SRM_IDLE;
        else
            status->dec_core.core1 = SRM_RESERVED;
    }

    fscanf(fp, "%s  %s", s0, s1);
    if (strstr(s0, "core:2")) {
        if (strstr(s1, "idle"))
            status->dec_core.core2 = SRM_IDLE;
        else
            status->dec_core.core2 = SRM_RESERVED;
    }

    fscanf(fp, "%s  %s", s0, s1);
    if (strstr(s0, "core:3")) {
        if (strstr(s1, "idle"))
            status->dec_core.core3 = SRM_IDLE;
        else
            status->dec_core.core3 = SRM_RESERVED;
    }

    fclose(fp);

    return 0;
}

static int get_enc_core_status(int device_id, SrmDriverStatus *status)
{
    char file[255];
    char s0[255];
    char s1[255];
    FILE *fp;

    sprintf(file, "%s%d/%s", INFO_PATH_PREFIX, device_id, ENC_CORE_STATUS);
    fp = fopen(file, "r");
    if (fp == NULL) {
        printf("get_dec_core_status can't open file %s\n", file);
        return -1;
    }

    fscanf(fp, "%s  %s", s0, s1);
    if (strstr(s0, "core:0")) {
        if (strstr(s1, "idle"))
            status->enc_core.core0 = SRM_IDLE;
        else
            status->enc_core.core0 = SRM_RESERVED;
    }

    fscanf(fp, "%s  %s", s0, s1);
    if (strstr(s0, "core:1")) {
        if (strstr(s1, "idle"))
            status->enc_core.core1 = SRM_IDLE;
        else
            status->enc_core.core1 = SRM_RESERVED;
    }

    fclose(fp);

    return 0;
}

static int get_dec_usage(int device_id, SrmDriverStatus *status)
{
    char file[255];
    FILE *fp;

    sprintf(file, "%s%d/%s", INFO_PATH_PREFIX, device_id, DEC_UTIL);
    fp = fopen(file, "r");
    if (fp == NULL) {
        printf("get_dec_core_status can't open file %s\n", file);
        return -1;
    }
    fscanf(fp, "%d", &status->dec_usage);
    fclose(fp);

    return 0;
}

static int get_enc_usage(int device_id, SrmDriverStatus *status)
{
    char file[255];
    FILE *fp;

    sprintf(file, "%s%d/%s", INFO_PATH_PREFIX, device_id, ENC_UTIL);
    fp = fopen(file, "r");
    if (fp == NULL) {
        printf("get_dec_core_status can't open file %s\n", file);
        return -1;
    }
    fscanf(fp, "%d", &status->enc_usage);
    fclose(fp);

    return 0;
}

static int get_mem_usage(int device_id, SrmDriverStatus *status)
{
    char file[255];
    char s0[255];
    char s1[255];
    FILE *fp;
    int used_s0 = 0, used_s1 = 0, free_s0 = 0, free_s1 = 0;

    sprintf(file, "%s%d/%s", INFO_PATH_PREFIX, device_id, MEM_USAGE);
    fp = fopen(file, "r");
    if (fp == NULL) {
        printf("get_dec_core_status can't open file %s\n", file);
        return -1;
    }
    fscanf(fp, "%s%d%*s%*s%d%*c%*s%*s%*d%*c%*s%*s", s0, &used_s0, &free_s0);
    fscanf(fp, "%s%d%*s%*s%d%*c%*s%*s%*d%*c%*s%*s", s1, &used_s1, &free_s1);

    if (strncmp(s0, "S0:", 3) == 0 && strncmp(s1, "S1:", 3) == 0) {
        status->free_mem = free_s0 + free_s1;
        status->used_mem = used_s0 + used_s1;
    } else {
        printf("Memory usage file %s format is wrong, s0=%s, free_s0=%d\n",
               file, s0, free_s0);
        return -1;
    }
    fclose(fp);

    return 0;
}

static int read_driver_status(SrmContext *srm)
{
    int i   = 0;
    int ret = 0;
    int num = srm->driver_nums;

    for (i = 0; i < num; i++) {
        srm->driver_status[i].device_id = i;
        ret = get_power_state(i, &srm->driver_status[i]);
        if (ret != 0) {
            printf("get_power_state %d failed\n", i);
            return -1;
        }
        ret = get_dec_core_status(i, &srm->driver_status[i]);
        if (ret != 0) {
            printf("get_dec_core_status %d failed\n", i);
            return -1;
        }
        ret = get_enc_core_status(i, &srm->driver_status[i]);
        if (ret != 0) {
            printf("get_enc_core_status %d failed\n", i);
            return -1;
        }
        ret = get_dec_usage(i, &srm->driver_status[i]);
        if (ret != 0) {
            printf("get_dec_usage %d failed\n", i);
            return -1;
        }
        ret = get_enc_usage(i, &srm->driver_status[i]);
        if (ret != 0) {
            printf("get_enc_usage %d failed\n", i);
            return -1;
        }
        ret = get_mem_usage(i, &srm->driver_status[i]);
        if (ret != 0) {
            printf("get_mem_usage %d failed\n", i);
            return -1;
        }
    }
    return 0;
}

void srm_dump_resource(SrmContext *srm)
{
    int i                   = 0;
    SrmDriverStatus *status = NULL;

    for (i = 0; i < srm->driver_nums; i++) {
        status = &srm->driver_status[i];
        printf("\rtranscoder%2d: power=%s, decoder=%2d%, encoder=%2d%, memory used=%4dMB, memory free=%4dMB",
               status->device_id, status->power_state?"on":"off", status->dec_usage,
               status->enc_usage, status->used_mem, status->free_mem);
    }
}

int srm_init(SrmContext *srm)
{
    srm->driver_nums = get_device_numbers();
    if (srm->driver_nums <= 0) {
        printf("No transcoder device was found!\n");
        return -1;
    }
    printf("srm found %d devices\n", srm->driver_nums);

    srm->driver_status = malloc(sizeof(SrmDriverStatus) * srm->driver_nums);
    if (!srm->driver_status) {
        printf("Malloc driver_status failed!\n");
        return -1;
    }
    return 0;
}

void srm_close(SrmContext *srm)
{
    free(srm->driver_status);
}

int main()
{
    SrmContext srm;

    if(srm_init(&srm)!=0){
        return -1;
    }

    while(1){
        if(read_driver_status(&srm) !=0)
            return -1;
        srm_dump_resource(&srm);
        usleep(100000);
    }
    srm_close(&srm);
}