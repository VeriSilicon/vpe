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
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <hugetlbfs.h>

#include "transcoder.h"

#define ENABLE_HUGEPAG

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define MAP_SIZE 0x1000
#define MAP_MASK (MAP_SIZE - 1)

#define TRANS_SIZE 0x800000 // 8M for each trans
#define TRANS_EP_S0_ADDR 0x4200000 // 8M for each trans
#define TRANS_EP_S1_ADDR 0x84000000 // 8M for each trans
#define TRANS_PATTERN 0x55

struct t_param {
    u32 test_type;
    char *dev_node;
    u32 edma_mode;
    u32 size;
    u32 ep_addr;
    unsigned long ep_end_addr;
    u32 compare;
    u32 slice;
    u32 dump_ddr;
    int poll_intr;
    u32 loop_times;
    char *input_file;
};

struct info_rc {
    unsigned long pa;
    unsigned char *va;
    struct mem_info rc_region;
};

struct t_info {
    struct info_rc rc[2];
    struct t_param tp;
    int fd_dev;
};

static void usage()
{
    printf("usage: ./pcie_ddr_memtest <d:device> [r | w | t] [i] [E] [n] [s] "
           "[S] [I] [filename]\n");
    printf("\t-d: device node; e.g. /dev/transcoder0\n");
    printf("\t-r: read data from ep ddr\n");
    printf("\t-w: write data to ep ddr\n");
    printf("\t-t: do trans test, default test\n");
    printf("\t-s: size for each edma trans, default 8m\n");
    printf("\t-E: ep end address, default s0:0x70000000 s1:0xF000000\n");
    printf("\t-S: select slice, default 0\n");
    printf("\t-i: input file.\n");
    printf("\t-I: use interrupt, default polling\n");
    printf("\t-n: loops\n");
    printf("\texample:\n");
    printf("\t\t./pcie_ddr_memtest -d /dev/transcoder0 -I\n");
    printf("\t\t./pcie_ddr_memtest -d /dev/transcoder0 -I -S <0 or 1>\n");
    printf("\t\t./pcie_ddr_memtest -d /dev/transcoder0 -I -s <size> -n "
           "<loop>\n");
}

void get_time(char *s)
{
    struct timeval timev;
    time_t timep;
    struct tm *p;
    gettimeofday(&timev, NULL);
    time(&timep);
    p = localtime(&timep);
    printf("[memtest] %s time: %ldus\n", s, timev.tv_usec);
    printf("[%s] %d-%d-%d %d:%d:%d\n", s, (p->tm_year + 1900), (1 + p->tm_mon),
           p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
}

static int compare_data(struct info_rc *rc, u32 size, u32 ep_addr,
                        unsigned long ep_end_addr)
{
    u32 i;

    for (i = 0; i < size; i++) {
        if (*(rc[0].va + i) != *(rc[1].va + i)) {
            printf("[memtest]\033[31m compare fail at 0x%08x !!!\033[0m\n",
                   ep_addr + i);
            goto exit;
        }
    }
    printf("[memtest]\033[32m compare pass start addr: 0x%08x, end addr: 0x%lx "
           "!!!\033[0m\n",
           ep_addr, ep_end_addr);
exit:
    return 0;
}

static int do_trans(struct trans_pcie_edma *edma_trans, int fd)
{
    // enable edma transmit
#ifndef ENABLE_HUGEPAG
    if (ioctl(fd, IOCTL_CMD_EDMA_TRANSMIT, edma_trans) < 0) {
        printf("[memtest] edma trans timeout\n");
        return -1;
    }
#else
    if (ioctl(fd, CB_TRANX_EDMA_TRANX, edma_trans) < 0) {
        printf("[memtest] edma trans timeout\n");
        return -1;
    }
#endif

    return 0;
}

static int malloc_rc(struct info_rc *rc, u32 size, int fd)
{
    u32 map_size;
    struct mem_info *rc_region = &rc->rc_region;

    if (size == 0) {
        printf("error, invalid size !!!\n");
        return -1;
    }
#ifndef ENABLE_HUGEPAG
    rc_region->mem_location = RC_SIDE;
    rc_region->size         = size;

    if (ioctl(fd, CB_TRANX_MEM_ALLOC, rc_region) < 0) {
        printf("error, ioctl get rc cma mem failed.\n");
        return -1;
    }

    map_size = size + MAP_MASK;
    map_size &= ~MAP_MASK;

    rc->pa = rc_region->phy_addr;
    rc->va = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                  rc_region->phy_addr & ~MAP_MASK);
#else
    rc->va = get_huge_pages(size, GHP_DEFAULT);
    rc->pa = rc_region->phy_addr = rc->va;
#endif
    if (rc->va == NULL) {
        printf("[memtest] mmap failed for cma mem region");
        return -1;
    }

    return 0;
}

static int do_trans_test(struct t_info *ti)
{
    int ret;
    unsigned long i;
    FILE *fp;
    int fd      = ti->fd_dev;
    int size    = ti->tp.size;
    u32 ep_addr = ti->tp.ep_addr;
    unsigned long ep_end_addr;
    u32 loops = ti->tp.loop_times;
    u32 count = 0;
    struct trans_pcie_edma edma_trans_wr, edma_trans_rd;

    if (loops)
        loops--;

    if (malloc_rc(&ti->rc[0], size, fd) < 0)
        return -1;
    if (malloc_rc(&ti->rc[1], size, fd) < 0)
        return -1;

    if (ti->tp.input_file == NULL) {
        // fill random data
        srand((u32)time(NULL));
        for (i = 0; i < size; i++) {
            *(ti->rc[0].va + (u32)i) = rand() % 256;
        }
    } else {
        fp = fopen(ti->tp.input_file, "rb");
        fread(ti->rc[0].va, 1, size, fp);
        fclose(fp);
        printf("[memtest] Test data from %s\n", ti->tp.input_file);
    }

    memcpy(ti->rc[1].va, ti->rc[0].va, size);

    if (ti->tp.ep_end_addr == 0) {
        if (ti->tp.slice == 0)
            ep_end_addr = 0x70000000 - size;
        else
            ep_end_addr = 0xF0000000 - size;
    } else
        ep_end_addr = ti->tp.ep_end_addr - size;

    do {
        printf("[memtest] trans %d begin, ddr_addr_start: 0x%x, size: 0x%x, "
               "ddr_end_addr: 0x%lx.\n",
               count++, ep_addr, size, size + ep_end_addr);

        for (i = ep_addr; i <= ep_end_addr; i += size) {
            edma_trans_wr.direct           = RC2EP;
            edma_trans_wr.interrupt_enable = ti->tp.poll_intr;
            edma_trans_wr.size             = size;
            edma_trans_wr.sar_high         = ti->rc[0].pa >> 32;
            edma_trans_wr.sar_low          = ti->rc[0].pa & 0xffffffff;
            edma_trans_wr.dar_high         = 0x0;
            edma_trans_wr.dar_low          = (u32)i;

            edma_trans_rd.direct           = EP2RC;
            edma_trans_rd.interrupt_enable = ti->tp.poll_intr;
            edma_trans_rd.size             = size;
            edma_trans_rd.sar_high         = 0x0;
            edma_trans_rd.sar_low          = (u32)i;
            edma_trans_rd.dar_high         = ti->rc[0].pa >> 32;
            edma_trans_rd.dar_low          = ti->rc[0].pa & 0xffffffff;

            if (do_trans(&edma_trans_wr, fd) < 0) {
                printf("write failed 0x%08x !!!\n", (u32)i);
                return -1;
            }
            if (do_trans(&edma_trans_rd, fd) < 0) {
                printf("read failed 0x%08x !!!\n", (u32)i);
                return -1;
            }
        }

        compare_data(ti->rc, size, ep_addr, size + ep_end_addr);

    } while (loops--);

    printf("[memtest] done ...\n");
}

static int do_ep_read(struct t_info *ti)
{
    int ret;
    FILE *fp;
    int fd   = ti->fd_dev;
    int size = ti->tp.size;
    struct trans_pcie_edma edma_trans;

    if (malloc_rc(&ti->rc[0], size, fd) < 0)
        return -1;

    if (ti->tp.input_file == NULL) {
        printf("error, invalid filename !!!\n");
        return -1;
    }

    edma_trans.direct           = EP2RC;
    edma_trans.interrupt_enable = ti->tp.poll_intr;
    edma_trans.size             = size;
    edma_trans.sar_high         = 0x0;
    edma_trans.sar_low          = ti->tp.ep_addr;
    edma_trans.dar_high         = ti->rc[0].pa >> 32;
    edma_trans.dar_low          = ti->rc[0].pa & 0xffffffff;

    printf("[memtest] ep addr: 0x%08x, pa: 0x%lx, size: 0x%08x\n",
           ti->tp.ep_addr, ti->rc[0].pa, size);

    if (do_trans(&edma_trans, fd) < 0) {
        printf("read failed !!!\n");
        return -1;
    }

    fp = fopen(ti->tp.input_file, "wb");
    fwrite(ti->rc[0].va, 1, size, fp);
    fclose(fp);
}

static int do_ep_write(struct t_info *ti)
{
    int ret;
    FILE *fp;
    int fd   = ti->fd_dev;
    int size = ti->tp.size;
    struct trans_pcie_edma edma_trans;

    if (malloc_rc(&ti->rc[0], size, fd) < 0)
        return -1;

    if (ti->tp.input_file == NULL) {
        printf("error, invalid filename !!!\n");
        return -1;
    }

    // read in the data
    fp = fopen(ti->tp.input_file, "rb");
    fread(ti->rc[0].va, 1, size, fp);
    fclose(fp);

    fp = fopen("tttt.bin", "wb");
    fwrite(ti->rc[0].va, 1, size, fp);
    fclose(fp);

    edma_trans.direct           = RC2EP;
    edma_trans.interrupt_enable = ti->tp.poll_intr;
    edma_trans.size             = size;
    edma_trans.sar_high         = ti->rc[0].pa >> 32;
    edma_trans.sar_low          = ti->rc[0].pa & 0xffffffff;
    edma_trans.dar_high         = 0x0;
    edma_trans.dar_low          = ti->tp.ep_addr;

    printf("[memtest] ep addr: 0x%08x, pa: 0x%lx, size: 0x%08x\n",
           ti->tp.ep_addr, ti->rc[0].pa, size);

    if (do_trans(&edma_trans, fd) < 0) {
        printf("write failed !!!\n");
        return -1;
    }
}

static int parse_param(int argc, char **argv, struct t_param *tp)
{
    int ret = 0;
    int c;

    // set default trans size and ep addr
    tp->size    = TRANS_SIZE;
    tp->ep_addr = TRANS_EP_S0_ADDR;

    while ((c = getopt(argc, argv, "d:w:r:ts:S:E:i:In:h")) != -1) {
        switch (c) {
        case 'h':
            usage();
            return -1;
            break;
        case 'r':
            tp->test_type = 1;
            tp->ep_addr   = strtol(optarg, NULL, 0);
            break;
        case 'w':
            tp->test_type = 2;
            tp->ep_addr   = strtol(optarg, NULL, 0);
            break;
        case 't':
            tp->test_type = 0;
            break;
        case 'd':
            tp->dev_node = optarg;
            break;
        case 's':
            tp->size = strtol(optarg, NULL, 0);
            break;
        case 'E':
            tp->ep_end_addr = strtol(optarg, NULL, 0);
            break;
        case 'i':
            tp->input_file = optarg;
            break;
        case 'S':
            tp->slice = atoi(optarg);
            if (tp->slice)
                tp->ep_addr = TRANS_EP_S1_ADDR;
            break;
        case 'I':
            tp->poll_intr = 1;
            break;
        case 'n':
            tp->loop_times = atoi(optarg);
            break;
        default:
            break;
        }
    }

    if (optind < argc)
        tp->input_file = argv[optind];

    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    u32 test_type;
    struct t_info *trans_info;

    if (argc < 2) {
        usage();
        return 0;
    }
    trans_info = (struct t_info *)calloc(1, sizeof(struct t_info));

    if (parse_param(argc, argv, &trans_info->tp) < 0)
        return -1;

    trans_info->fd_dev =
        open(trans_info->tp.dev_node, O_RDWR, S_IRUSR | S_IWUSR);
    if (trans_info->fd_dev == -1) {
        printf("failed to open: %s \n", trans_info->tp.dev_node);
        return -1;
    }
    printf("[memtest] testing device node is %s. \n", trans_info->tp.dev_node);

    if (trans_info->tp.test_type == 0) {
        do_trans_test(trans_info);
    } else if (trans_info->tp.test_type == 1) {
        do_ep_read(trans_info);
    } else {
        do_ep_write(trans_info);
    }

exit:
#ifndef ENABLE_HUGEPAG
    if (trans_info->rc[0].va) {
        munmap(trans_info->rc[0].va, trans_info->tp.size);
        ioctl(trans_info->fd_dev, CB_TRANX_MEM_FREE,
              &trans_info->rc[0].rc_region);
    }
    if (trans_info->rc[1].va) {
        munmap(trans_info->rc[1].va, trans_info->tp.size);
        ioctl(trans_info->fd_dev, CB_TRANX_MEM_FREE,
              &trans_info->rc[1].rc_region);
    }
#else
    if (trans_info->rc[0].va)
        free_huge_pages(trans_info->rc[0].va);
    if (trans_info->rc[1].va)
        free_huge_pages(trans_info->rc[1].va);
#endif
    close(trans_info->fd_dev);

    free(trans_info);

    return 0;
}
