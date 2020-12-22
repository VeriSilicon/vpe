// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 VeriSilicon Holdings Co., Ltd.
 *
 * This is bigsea management driver for Linux.
 * Bigsea is a video encoder, has two cores, only support vp9 format.
 * This driver provide some IOCTL commands for userspace,
 * like reserve/release a hardware core, access registers.
 * At the same time, bigsea and vc8000e only one can work, so they use
 * same functions to reserve/release core.
 * How to operate Bigsea: reserve a idle core, config registers,
 * enable the core, wait interrupt, release the core.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>

#include "transcoder.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define EP_COUNT 11
#define RC_COUNT 5
#define MAX_SIZE 36 // block max size is ~36MB

int main(int argc, char **argv)
{
    int task_id;
    int memdev_fd = -1;
    int i = 0, n = 0, j = 0, ret;
    int first, second;
    struct mem_info ep_region[200];
    int mem_cnt = 5;

    if (argc < 2) {
        printf("command line:  %s node count.\n", argv[0]);
        return -1;
    }
    memdev_fd = open(argv[1], O_RDWR);
    if (memdev_fd == -1) {
        printf("Failed to open dev: %s\n", argv[1]);
        goto end;
    } else
        printf("open /dev/trans_mem succeed.\n");
    if (argc < 3) {
        printf("not input mem_cnt,so use default count=%d \n", mem_cnt);
    } else
        mem_cnt = strtoul(argv[2], 0, 0);

    ioctl(memdev_fd, CB_TRANX_MEM_GET_TASKID, &task_id);
    printf("get task id = %d. \n", task_id);

    for (j = 0; j < mem_cnt / MAX_SIZE; j++)
        for (i = 1; i <= MAX_SIZE; i++) {
            ep_region[n].size    = 1024 * 1024 * i;
            ep_region[n].task_id = task_id;
            n++;
        }
    if (n < mem_cnt)
        for (i = 1; i <= MAX_SIZE; i++) {
            ep_region[n].size    = 1024 * 1024 * i;
            ep_region[n].task_id = task_id;
            n++;
        }

    for (i = 1; i <= mem_cnt; i++) {
        ep_region[i].mem_location = EP_SIDE;
        ret = ioctl(memdev_fd, CB_TRANX_MEM_ALLOC, &ep_region[i]); // get
        if (ret) {
            printf("ioctl get ep mem failed.\n");
            for (i = 0; i <= n; i++) {
                ioctl(memdev_fd, CB_TRANX_MEM_FREE, &ep_region[i]); // free
            }
            goto end;
        }
        n = i;
        printf("EP: alloc %d phy_addr=0x%lx size=0x%x.\n", i,
               ep_region[i].phy_addr, ep_region[i].size);
        usleep(10000);
    }

    getchar();

    printf("\n");

    for (i = 1; i <= mem_cnt; i++) {
        ioctl(memdev_fd, CB_TRANX_MEM_FREE, &ep_region[i]);
        printf("EP: free %d phy_addr=0x%lx size=0x%x.\n", i,
               ep_region[i].phy_addr, ep_region[i].size);
    }
    printf("\n");

    ioctl(memdev_fd, CB_TRANX_MEM_FREE_TASKID, &task_id);

end:
    close(memdev_fd);

    return 0;
}
