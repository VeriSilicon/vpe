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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <hugetlbfs.h>

#include "transcoder.h"

#define ENABLE_HUGEPAG
#define DIRECTION_RC2EP 0
#define DIRECTION_EP2RC 1

static int usage(char **argv)
{
    printf("usage:\n");
    printf("\t%s device_node transfer_direction test_count size_MB\n", argv[0]);
    printf("\tdevice_node       : /dev/transcoderN\n");
    printf("\ttransfer_direction: rc2ep | ep2rc\n");
    printf("\ttest_count        : transfer times\n");
    printf("\tsize              : transfer data size in MB(1~64)\n");
    printf("example:\n");
    printf("\t%s /dev/transcoder0 rc2ep 1 32 ---> rc2ep transfer 1 times data, "
           "each data size is 32MB with /dev/transcoder0\n",
           argv[0]);
    printf("\t%s /dev/transcoder1 ep2rc 2 64 ---> ep2rc transfer 2 times data, "
           "each data size is 64MB with /dev/transcoder1\n",
           argv[0]);
}

int main(int argc, char **argv)
{
    int ret = -1;
    int fd;
    int task_id;
    int direction          = -1; // 0: rc2ep 1:ep2rc
    unsigned int poll_intr = 1; // interrupt mode
    struct mem_info rc_region, ep_region;
    unsigned long time_cnt = 0, diff = 0;
    unsigned long src, src_h, src_l, dst, dst_h, dst_l, size, i;
    struct trans_pcie_edma edma_trans;
    struct timeval time, time_1;
    unsigned int count = 0;
    float bw           = 0;

    do {
        if (argc != 5) {
            ret = -2;
            break;
        }
        if (strncmp(argv[2], "rc2ep", 5) == 0) {
            direction = DIRECTION_RC2EP;
        } else if (strncmp(argv[2], "ep2rc", 5) == 0) {
            direction = DIRECTION_EP2RC;
        } else {
            printf("[ERROR] please input correct transfer direction param, "
                   "invalid param: %s\n",
                   argv[2]);
            ret = -3;
            break;
        }
        count = strtoul(argv[3], 0, 0);
        if (count <= 0) {
            printf("[ERROR] please input correct count number param, invalid "
                   "param: %s\n",
                   argv[3]);
            ret = -4;
            break;
        }
        size = strtoul(argv[4], 0, 0);
        if (size <= 0) {
            printf("[ERROR] please input correct data size param, invalid "
                   "param: %s\n",
                   argv[4]);
            ret = -5;
            break;
        }
        if (size > 64)
            size = 64;
        if (size <= 0)
            size = 32;
        size = size * 1024 * 1024;
        fd   = open(argv[1], O_RDWR, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            printf("[ERROR] failed to open: %s, please check the device node "
                   "is exist...\n",
                   argv[1]);
            break;
        }
        printf("device node is %s \n", argv[1]);
        ret = 0;
    } while (0);
    if (ret != 0) {
        usage(argv);
        exit(-1);
    }

    do {
#ifndef ENABLE_HUGEPAG
        rc_region.mem_location = RC_SIDE;
        rc_region.size         = size;
        ret = ioctl(fd, CB_TRANX_MEM_ALLOC, &rc_region); // get
        if (ret < 0) {
            printf("ioctl get host mem failed.\n");
            ret = -1;
            break;
        }
        printf("RC address=0x%lx \n", rc_region.phy_addr);
#else
        rc_region.phy_addr = (unsigned long)get_huge_pages(size, GHP_DEFAULT);
        if (rc_region.phy_addr == 0) {
            printf("get_huge_pages failed.\n");
            ret = -1;
            break;
        }
#endif
        ret = ioctl(fd, CB_TRANX_MEM_GET_TASKID, &task_id); // get
        if (ret < 0) {
            printf("ioctl get task id failed.\n");
            ret = -2;
            break;
        }
        ep_region.mem_location = EP_SIDE;
        ep_region.size         = size;
        ep_region.task_id      = task_id;
        ret                    = ioctl(fd, CB_TRANX_MEM_ALLOC, &ep_region);
        if (ret < 0) { // get
            printf("ioctl get ep mem failed.\n");
            ret = -3;
            break;
        }
        printf("EP address=0x%lx \n", ep_region.phy_addr);

        if (DIRECTION_RC2EP == direction) {
            src   = rc_region.phy_addr;
            dst   = ep_region.phy_addr; // can fix a special addr.
            src_l = src & 0xffffffff;
            src_h = src >> 32;
            dst_l = dst & 0xffffffff;
            dst_h = dst >> 32;

            edma_trans.direct = RC2EP;
            edma_trans.interrupt_enable = poll_intr; // INTE_EN;  POLL_EN
            edma_trans.size             = size;
            edma_trans.sar_high = src_h;
            edma_trans.sar_low  = src_l;
            edma_trans.dar_high = 0x0;
            edma_trans.dar_low  = dst;

        } else {
            dst   = rc_region.phy_addr;
            src   = ep_region.phy_addr; // can fix a special addr.
            src_l = src & 0xffffffff;
            src_h = src >> 32;
            dst_l = dst & 0xffffffff;
            dst_h = dst >> 32;

            edma_trans.direct = EP2RC;
            edma_trans.interrupt_enable = poll_intr; // INTE_EN;  POLL_EN
            edma_trans.size             = size;
            edma_trans.sar_high = 0x0;
            edma_trans.sar_low  = src;
            edma_trans.dar_high = dst_h;
            edma_trans.dar_low  = dst_l;
        }
        printf("normal edma trans size=0x%x, count=%d..\n", size, count);
        for (i = 0; i < count; i++) {
            gettimeofday(&time, NULL);
            if (ret = ioctl(fd, CB_TRANX_EDMA_TRANX, &edma_trans))
            {
                printf("edma tranx failed,ret=0x%x", ret);
                goto out;
            }
            gettimeofday(&time_1, NULL);
            diff = 1000000 * (time_1.tv_sec - time.tv_sec) + time_1.tv_usec -
                   time.tv_usec;
            time_cnt += diff;
            printf(" round %d/%d time=%ldus.\n", i + 1, count, diff);
        }

        size = size / 1024 / 1024;
        bw   = count * size * 1000 * 1000 / time_cnt;
        printf("total_time=%ld us, avg_bw= %f MB/s\n", time_cnt, bw);
    } while (0);

out:
#ifndef ENABLE_HUGEPAG
    ioctl(fd, CB_TRANX_MEM_FREE, &rc_region);
#else
    free_huge_pages((void *)rc_region.phy_addr);
#endif
    ioctl(fd, CB_TRANX_MEM_FREE, &ep_region);
    ioctl(fd, CB_TRANX_MEM_FREE_TASKID, &task_id);

    close(fd);

    return 0;
}
