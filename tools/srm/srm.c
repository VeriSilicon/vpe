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
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define DEV_NAME_PREFIX "transcoder"
#define INFO_PATH_PREFIX "/sys/class/misc/transcoder"
#define DEC_UTIL "dec_util"
#define ENC_UTIL "enc_util"
#define DEC_CORE_STATUS "dec_core_status"
#define ENC_CORE_STATUS "enc_core_status"
#define MEM_USAGE "mem_info"
#define POWER_STATUS "power_state"
#define DRVER_INDEX_BALABCE 100
#define MAX_DEVICES 12
#define MEM_FACTOR_4K_HEVC_DEC 13
#define MEM_FACTOR_2K_HEVC_DEC 25
#define MEM_FACTOR_HEVC_ENC 2
#define DEFAULT_EFFICIENCY  1.0

#define RED "\033[31m"
#define GREEN "\033[32m"
#define END "\033[0m"

typedef enum {
    SRM_PERFORMANCE   = 0,
    SRM_POWER_SAVING = 1,
} SrmMode;

typedef enum {
    SRM_IDLE     = 0,
    SRM_RESERVED = 1,
} SrmCoreStatus;

typedef struct {
    int core0;
    int core1;
    int core2;
    int core3;
} SrmDecCoreStatus;

typedef struct {
    int res_seirios;
    int res_480p30;
    int res_720p30;
    int res_1080p30;
    int res_2160p30;
} SrmTotalSource;

typedef struct {
    int device_id;
    int dec_usage;
    int enc_usage;
    int mem_usage;
    int used_mem;
    int free_mem;
    int power_state;
    SrmDecCoreStatus dec_core;
    SrmDecCoreStatus enc_core;
    SrmTotalSource comp_res;
} SrmDriverStatus;

typedef struct {
    SrmDriverStatus *driver_status;
    int driver_nums;
    float efficiency;
} SrmContext;

typedef enum {
    SRM_RES_ONE_CARD,
    SRM_RES_480P,
    SRM_RES_720P,
    SRM_RES_1080P,
    SRM_RES_2160P,
} SrmResType;

int mem_required[5] = {1024, 25, 100, 200, 500}; //unit is MB

static int get_device_numbers(void)
{
    struct dirent **namelist = NULL;
    int count                = 0;
    int n;

    n = scandir("/dev/", &namelist, 0, alphasort);
    while (n--) {
        if (strncmp(namelist[n]->d_name, DEV_NAME_PREFIX,
                    strlen(DEV_NAME_PREFIX)) == 0) {
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
        printf("transcoder%2d: power=%s, decoder=%3d%, encoder=%3d%, memory "
               "used=%4dMB, memory free=%4dMB - %s\n",
               i, status->power_state ? "on" : "off", status->dec_usage,
               status->enc_usage, status->used_mem, status->free_mem, status->comp_res.res_seirios? GREEN " free  " END:RED "active" END);
    }
    printf("\033[%dA", srm->driver_nums);
}

int srm_init(SrmContext *srm)
{
    srm->driver_nums = get_device_numbers();
    if (srm->driver_nums <= 0) {
        return -1;
    }

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

#define  MIN(x,y) (x>y? y:x)
#define  MAX(x,y) (x>y? x:y)

int srm_update_resource(SrmContext *srm, SrmResType type, float efficiency)
{
    int i           = 0;
    int enc_numbers = 0;
    int mem_numbers = 0;

    srm->efficiency = efficiency;
    if (read_driver_status(srm) != 0)
        return -1;

    for (i = 0; i < srm->driver_nums; i++) {
        SrmDriverStatus *status = &srm->driver_status[i];

        // calculate total resources in different allocation mode
        status->comp_res.res_seirios = !status->used_mem;

        enc_numbers = efficiency * (96 - 96 * status->enc_usage / 100);
        mem_numbers = status->free_mem / (mem_required[SRM_RES_480P]);
        status->comp_res.res_480p30 = MIN(enc_numbers, mem_numbers);

        enc_numbers = efficiency * (36 - 36 * status->enc_usage / 100);
        mem_numbers = status->free_mem / (mem_required[SRM_RES_720P]);
        status->comp_res.res_720p30 = MIN(enc_numbers, mem_numbers);

        enc_numbers = efficiency * (16 - 16 * status->enc_usage / 100);
        mem_numbers = status->free_mem / (mem_required[SRM_RES_1080P]);
        status->comp_res.res_1080p30 = MIN(enc_numbers, mem_numbers);

        enc_numbers = efficiency * (4 - 4 * status->enc_usage / 100);
        mem_numbers = status->free_mem / (mem_required[SRM_RES_2160P]);
        status->comp_res.res_2160p30 = MIN(enc_numbers, mem_numbers);
    }
}

int srm_get_total_resource(SrmContext *srm, SrmResType type)
{
    int total = 0,i = 0;

    for (i = 0; i < srm->driver_nums; i++) {
        SrmTotalSource *driver_res = &srm->driver_status[i].comp_res;
        if (type == SRM_RES_480P) {
            total += driver_res->res_480p30;
        } else if (type == SRM_RES_720P) {
            total += driver_res->res_720p30;
        } else if (type == SRM_RES_1080P) {
            total += driver_res->res_1080p30;
        } else if (type == SRM_RES_2160P) {
            total += driver_res->res_2160p30;
        } else if (type == SRM_RES_ONE_CARD) {
            total += driver_res->res_seirios;
        }
    }
    return total;
}

//return the device id
int srm_allocate_resource(SrmContext *srm, SrmResType req_type,
                          int req_nums, SrmMode mode)
{
    int driver_nums = srm->driver_nums;
    int i           = 0;
    int total_req = 0, available[MAX_DEVICES] = { 0 };
    int delta    = 100;
    int selected = -1;

    if (mode == SRM_PERFORMANCE || req_type == SRM_RES_ONE_CARD) {
        delta = 0;
    }

    for (i = 0; i < srm->driver_nums; i++) {
        SrmTotalSource *driver_res = &srm->driver_status[i].comp_res;

        if (req_type == SRM_RES_480P) {
            available[i] = driver_res->res_480p30;
        } else if (req_type == SRM_RES_720P) {
            available[i] = driver_res->res_720p30;
        } else if (req_type == SRM_RES_1080P) {
            available[i] = driver_res->res_1080p30;
        } else if (req_type == SRM_RES_2160P) {
            available[i] = driver_res->res_2160p30;
        }else if (req_type == SRM_RES_ONE_CARD) {
            available[i] = driver_res->res_seirios;
        }

        if (req_nums <= available[i]) {
            if (mode == SRM_PERFORMANCE) {
                // find the maximum delta for BALABCE mode
                if (delta <= available[i] - req_nums) {
                    delta    = available[i] - req_nums;
                    selected = i;
                }
            } else if (mode == SRM_POWER_SAVING) {
                // find the minimal delta for BALABCE mode
                if (delta >= available[i] - req_nums) {
                    delta    = available[i] - req_nums;
                    selected = i;
                }
            }
        }
    }

    if( selected !=-1){
        return srm->driver_status[selected].device_id;
    } else {
        return -1;
    }
}

void help()
{
    printf("srm allocate [resource type] [numbers] <power mode>\n");
    printf("\nresource type: \n");
    printf("        card: allocate the resource in one full card mode\n");
    printf("        480p: allocate the resource in one 480p capbility mode\n");
    printf("        720p: allocate the resource in one 720p capbility mode\n");
    printf("        1080p:allocate the resource in one 1080p capbility mode\n");
    printf("        2160p:allocate the resource in one 2160p capbility mode\n");
    printf("        *default is card\n");
    printf("\nnumbers: the number of the resource which you want to allocate, default is 1.\n");
    printf("\npower mode: \n");
    printf("        powersaving: allocate the resource in low power mode\n");
    printf("        performance: allocate the resource in performance mode\n");
    printf("        *default is performance mode\n");
    printf("\nExample:\n");
    printf("./srmtool \n");
    printf("./srmtool allocate\n");
    printf("./srmtool allocate 1080p\n");
    printf("./srmtool allocate 1080p 2\n");
    printf("./srmtool allocate 1080p 1 powersaving\n");
}

void stop(int signo)
{
    printf("srm will exit\n");
    _exit(0);
}

/*
1. monitor mode
srmtool

2. allocate resource - one card:
srmtool allocate

3. allocate resource - part of one card :
srmtool allocate 1080p 2 performance
srmtool allocate 480p 2 powersaving

function will return the allocated device ID
*/
int main(int argc, char **argv)
{
    SrmContext srm;
    int monitor = 0;
    SrmMode mode = SRM_PERFORMANCE;
    SrmResType req_type = SRM_RES_ONE_CARD;
    int req_nums = 1;
    int device_id = -1;

    signal(SIGINT, stop);

    if(argc ==1 ){
        monitor = 1;
    }else if( !strcmp(argv[1], "help")){
        help();
        return 0;
    } else if(!strcmp(argv[1], "allocate")){
        if(argc > 2){
            if(!strcmp(argv[2], "card")){
                    req_type = SRM_RES_ONE_CARD;
            } else if(!strcmp(argv[2], "480p")){
                    req_type = SRM_RES_480P;
            } else if(!strcmp(argv[2], "720p")){
                    req_type = SRM_RES_720P;
            } else if(!strcmp(argv[2], "1080p")){
                    req_type = SRM_RES_1080P;
            } else if(!strcmp(argv[2], "2160p")){
                    req_type = SRM_RES_2160P;
            } else {
                    printf("Wrong resource type, available: card/480p/720p/1080p/2160p\n");
                    return -1;
            }
        }

        if( argc > 3 ){
            req_nums = atoi(argv[3]);
            if(req_nums==0){
                printf("Wrong number\n");
                return -1;
            }
        }

        if( argc > 4 ){
            if(!strcmp(argv[4], "performance")){
                mode = SRM_PERFORMANCE;
            } else if(!strcmp(argv[4], "powersaving")){
                mode = SRM_POWER_SAVING;
            } else {
                printf("Wrong mode, available: performance/powersaving\n");
                return -1;
            }
        }
    } else if( argc > 0 ){
        printf("Wrong parameter, do you mean 'allocate'?\n");
        return -1;
    } else {
        monitor = 1;
    }

    if (srm_init(&srm) != 0) {
        return -1;
    }

     srm_update_resource(&srm, SRM_RES_ONE_CARD, DEFAULT_EFFICIENCY);

    if( !monitor){
        device_id = srm_allocate_resource(&srm, req_type, req_nums, mode);
        if( device_id != -1){
            printf("transcoder%d\n", device_id);
        }
        srm_close(&srm);
        return device_id;
    }

    while (1) {
        srm_update_resource(&srm, SRM_RES_ONE_CARD, DEFAULT_EFFICIENCY);
        srm_dump_resource(&srm);
        sleep(1);
    }
    srm_close(&srm);
    return -1;
}