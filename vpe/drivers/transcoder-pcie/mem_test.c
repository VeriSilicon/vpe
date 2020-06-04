/* SPDX-License-Identifier: GPL-2.0-or-later */
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

#include "transcoder.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define EP_COUNT 11
#define RC_COUNT 5

int main(int argc, char **argv)
{
    int task_id;
    int memdev_fd = -1;
    int i = 0, n = 0, ret;
    int first, second;
    struct mem_info ep_region[80], rc_region[80];
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

    // get mem from ep(board)
    first  = mem_cnt / 2;
    second = mem_cnt - first;

    for (i = 1; i <= first; i++) {
        ep_region[i].size    = 1024 * 1024 * i;
        ep_region[i].task_id = task_id;
        rc_region[i].size    = 1024 * 1024 * i;
        rc_region[i].task_id = task_id;
        n                    = i;
    }
    for (i = 1; i <= second; i++) {
        ep_region[n + i].size    = 1024 * 1024 * i;
        ep_region[n + i].task_id = task_id;
        rc_region[n + i].size    = 1024 * 1024 * i;
        rc_region[n + i].task_id = task_id;
    }
#if 1
    for (i = 1; i <= mem_cnt; i++) {
        ep_region[i].mem_location = EP_SIDE;
        ret = ioctl(memdev_fd, CB_TRANX_MEM_ALLOC, &ep_region[i]); // get
        if (ret) {
            printf("ioctl get ep mem failed.\n");
            for (i = 1; i <= n; i++) {
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
#endif

#if 0
	for(i=1;i<=mem_cnt;i++)
	{
		rc_region[i].mem_location = RC_SIDE;
		if (ioctl(memdev_fd, CB_TRANX_MEM_ALLOC, &rc_region[i]) < 0) // get
		{
			printf("ioctl get rc cma mem failed.\n");
			for(i=1;i<=n;i++)
			{
				ioctl(memdev_fd, CB_TRANX_MEM_FREE, &rc_region[i]); // free
			}
			goto end;
		}
		n = i;
		printf("RC: %d phy_addr=0x%lx size=0x%x rc_kvirt=%p \n", i,rc_region[i].phy_addr,rc_region[i].size,rc_region[i].rc_kvirt);
	}

	for(i=1;i<=mem_cnt;i++)
	{
		ioctl(memdev_fd, CB_TRANX_MEM_FREE, &rc_region[i]);
	}
#endif

end:
    close(memdev_fd);

    return 0;
}
