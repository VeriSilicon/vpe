// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Verisilicon Inc.
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
#include <hugetlbfs.h>
#include "transcoder.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define QWORD_HI(value) ((value >> 32) & 0xFFFFFFFF)
#define QWORD_LO(value) (value & 0xFFFFFFFF)

#define ENABLE_HUGEPAGE

int main(int argc, char **argv)
{
    int task_id;
    struct mem_info ep_region;

    int fd                     = -1;
    int i                      = 0, flag;
    unsigned char *virtual_src = MAP_FAILED;
    unsigned char *virtual_dst = MAP_FAILED;
    unsigned long tmp_addr, ep_addr;
    int size, round, save_round;
    struct trans_pcie_edma edma_trans;
    int enable_hugepage = 0;

    unsigned char val = 0x12;

    if (argc < 6) {
        printf("%s dev_node r/w size round enable_hugepage [val];  w:rc2ep, "
               "r:ep2rc, e:rw ; enable_hugepage 1:enable, 0:disable.\n",
               argv[0]);
        return -1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        printf("Failed to open dev: %s\n", argv[1]);
        return -1;
    }

    if (strcmp(argv[2], "r") == 0) {
        flag = 0x1;
    } else if (strcmp(argv[2], "w") == 0) {
        flag = 0x0;
    } else if (strcmp(argv[2], "e") == 0) {
        flag = 0x3;
    } else {
        printf("diretion:%s is error\n", argv[2]);
        return -1;
    }

    size = strtoul(argv[3], 0, 0);
    if (ioctl(fd, CB_TRANX_MEM_GET_TASKID, &task_id) < 0) {
        printf("ioctl get task id failed.\n");
        return -1;
    }
    printf("direct:%s task_id=%d size:0x%x flag=0x%x.\n", argv[2], task_id,
           size, flag);

    ep_region.mem_location = EP_SIDE;
    ep_region.size         = size;
    ep_region.task_id      = task_id;
    if (ioctl(fd, CB_TRANX_MEM_ALLOC, &ep_region) < 0) {
        printf("ioctl get ep mem failed.\n");
        return -1;
    }
    ep_addr = ep_region.phy_addr;
    printf("EP address=0x%lx \n", ep_region.phy_addr);

    round = strtoul(argv[4], 0, 0);

    if (strcmp(argv[5], "1") == 0) {
        enable_hugepage = 0x1;
    }

    if (argc == 7)
        val = strtoul(argv[6], 0, 0);

    if (enable_hugepage)
        virtual_src = get_huge_pages(size, GHP_DEFAULT);
    else
        virtual_src = malloc(size);
    if (!virtual_src) {
        perror("Err mmap virtual_src:");
        goto out_0;
    }

    if (enable_hugepage)
        virtual_dst = get_huge_pages(size, GHP_DEFAULT);
    else
        virtual_dst = malloc(size);
    if (!virtual_dst) {
        perror("Err mmap virtual_dst: ");
        goto out_0;
    }

    memset(virtual_src, val, size);
    if (size < 0)
        goto out_0;

    printf("round=%d size=0x%x virtual_src=%p virtual_dst=%p ep_addr=0x%x. \n",
           round, size, virtual_src, virtual_dst, ep_addr);
    save_round = round;
    while (round--) {
        if ((flag == 0x0) || (flag == 0x3)) {
            tmp_addr                    = (unsigned long)virtual_src;
            edma_trans.dar_low          = QWORD_LO(ep_addr);
            edma_trans.dar_high         = QWORD_HI(ep_addr);
            edma_trans.sar_low          = QWORD_LO(tmp_addr);
            edma_trans.sar_high         = QWORD_HI(tmp_addr);
            edma_trans.direct           = RC2EP;
            edma_trans.size             = size;
            edma_trans.interrupt_enable = INTE_EN;

            if (ioctl(fd, CB_TRANX_EDMA_TRANX, &edma_trans) < 0) {
                printf("ioctl RC2EP failed\n");
                goto out_0;
            }
            printf("round=%d virtual_src=%p val_src=0x%x, ep_addr=0x%lx. \n",
                   save_round - round, virtual_src,
                   *(unsigned int *)virtual_src, ep_addr);
        }

        if ((flag == 0x1) || (flag == 0x3)) {
            tmp_addr                    = (unsigned long)virtual_dst;
            edma_trans.sar_low          = QWORD_LO(ep_addr);
            edma_trans.sar_high         = QWORD_HI(ep_addr);
            edma_trans.dar_low          = QWORD_LO(tmp_addr);
            edma_trans.dar_high         = QWORD_HI(tmp_addr);
            edma_trans.direct           = EP2RC;
            edma_trans.size             = size;
            edma_trans.interrupt_enable = INTE_EN; // POLL_EN;

            if (ioctl(fd, CB_TRANX_EDMA_TRANX, &edma_trans) < 0) // transmit via
                                                                 // edma
            {
                printf("ioctl EP2RC failed\n");
                goto out_0;
            }
            printf("round=%d virtual_dst=%p val_tg=0x%x, ep_addr=0x%lx. \n",
                   save_round - round, virtual_dst,
                   *(unsigned int *)virtual_dst, ep_addr);
        }
        if (flag == 0x3) {
            for (i = 0; i < size; i++) {
                if (*(virtual_dst + i) != *(virtual_src + i)) {
                    printf("compare failed, i=0x%x \n", i);
                    break;
                }
            }
            if (i == size)
                printf("round:%d compare successfully, size=0x%x, "
                       "val_tg=0x%08x\n",
                       save_round - round, size,
                       *(u32 *)(virtual_dst + size / 4));
        }
    }
out_0:

    ioctl(fd, CB_TRANX_MEM_FREE, &ep_region);
    ioctl(fd, CB_TRANX_MEM_FREE_TASKID, &task_id);

    if (enable_hugepage)
        free_huge_pages(virtual_src);
    else
        free(virtual_src);

    if (enable_hugepage)
        free_huge_pages(virtual_dst);
    else
        free(virtual_dst);

    close(fd);

    return 0;
}
