// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Verisilicon Inc.
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

#include "transcoder.h"

int main(int argc, char **argv)
{
    int fd;

    unsigned long ep_addr, rc_addr;
    unsigned long src, src_h, src_l, dst, dst_h, dst_l, size, i;
    int round;
    struct trans_pcie_edma edma_trans;
    struct timeval time, time_1;
    unsigned long time_cnt = 0, diff = 0;
    float bw = 0;

    if (argc < 7) {
        printf("%s dev_node rc2ep/ep2rc ep_addr rc_addr size round \n",
               argv[0]);
        exit(1);
    }

    size  = strtoul(argv[5], 0, 0);
    round = strtoul(argv[6], 0, 0);

    fd = open(argv[1], O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        printf("failed to open: %s \n", argv[1]);
        return -1;
    }

    if (strcmp(argv[2], "rc2ep") == 0) {
        src = strtoul(argv[4], 0, 0);
        dst = strtoul(argv[3], 0, 0);
        printf("rc2ep - RC_addr:0x%lx EP_addr=0x%lx size=0x%x\n", src, dst,
               size);
    }
    if (strcmp(argv[2], "ep2rc") == 0) {
        dst = strtoul(argv[4], 0, 0);
        src = strtoul(argv[3], 0, 0);
        printf("ep2rc - RC_addr:0x%lx EP_addr=0x%lx size=0x%x\n", dst, src,
               size);
    }

    src_l = src & 0xffffffff;
    src_h = src >> 32;
    dst_l = dst & 0xffffffff;
    dst_h = dst >> 32;

    if (strcmp(argv[2], "rc2ep") == 0) // rc to ep
    {
        edma_trans.direct           = RC2EP;
        edma_trans.interrupt_enable = INTE_EN;
        edma_trans.size             = size;
        edma_trans.sar_high         = src_h;
        edma_trans.sar_low          = src_l;
        edma_trans.dar_high         = 0x0;
        edma_trans.dar_low          = dst;
    }
    if (strcmp(argv[2], "ep2rc") == 0) // ep to rc
    {
        edma_trans.direct           = EP2RC;
        edma_trans.interrupt_enable = INTE_EN;
        edma_trans.size             = size;
        edma_trans.sar_high         = 0x0;
        edma_trans.sar_low          = src;
        edma_trans.dar_high         = dst_h;
        edma_trans.dar_low          = dst_l;
    }

    for (i = 0; i < round; i++) {
        gettimeofday(&time, NULL);
        if (ioctl(fd, CB_TRANX_EDMA_PHY_TRANX, &edma_trans) < 0) // enable edma
                                                                 // transmit
            printf("edma tranx failed.\n");
        gettimeofday(&time_1, NULL);
        diff = 1000000 * (time_1.tv_sec - time.tv_sec) + time_1.tv_usec -
               time.tv_usec;
        time_cnt += diff;
        printf(" round %d/%d time=%ldus.\n", i + 1, round, diff);
    }

    size = size / 1024 / 1024;
    bw   = round * size * 1000 * 1000 / time_cnt;
    printf("total_time=%ld us, avg_bw= %f MB/s\n", time_cnt, bw);

    close(fd);
    return 0;
}
