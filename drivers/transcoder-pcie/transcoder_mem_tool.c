/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Verisilicon Inc.
 */

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

#define FATAL                                                              \
    do {                                                                   \
        fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, \
                __FILE__, errno, strerror(errno));                         \
        exit(1);                                                           \
    } while (0)

#define MAP_SIZE 0x10000
#define MAP_MASK (MAP_SIZE - 1)

int main(int argc, char **argv)
{
    int fd;
    void *map_base, *virt_addr;
    unsigned short *virt_addr16;
    unsigned int *virt_addr32;

    unsigned int read_result, writeval;
    unsigned long target;
    unsigned int len;
    int access_type = 'w';
    unsigned int size, i;

    if (argc < 3) {
        fprintf(stderr,
                "\nUsage:\t%s dev_node { address } [ type [ data ] ]\n"
                "\tdev_node : transcoder device node, like: /dev/transcoder0\n"
                "\taddress : memory address to act upon\n"
                "\ttype    : access operation type : [b]yte, [h]alfword, "
                "[w]ord\n"
                "\tdata    : data to be written\n\n",
                argv[0]);
        exit(1);
    }

    target = strtoul(argv[2], 0, 0);
    if (argc == 4) {
        len  = strtoul(argv[3], 0, 0) - target;
        size = (len > MAP_SIZE) ? len : MAP_SIZE;
        size += MAP_MASK;
        size &= ~MAP_MASK;
    } else
        size = MAP_SIZE;

    if (argc > 4)
        access_type = tolower(argv[3][0]);

    if ((fd = open(argv[1], O_RDWR | O_SYNC)) == -1) {
        printf("open %s failed \n", argv[1]);
        return -1;
    }
    fflush(stdout);

    /* Map one page */
    map_base = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                    target & ~MAP_MASK);
    if (map_base == (void *)-1)
        FATAL;
    fflush(stdout);

    virt_addr = map_base + (target & MAP_MASK);

    if (argc == 3) {
        switch (access_type) {
        case 'b':
            read_result = *((unsigned char *)virt_addr);
            break;
        case 'h':
            read_result = *((unsigned short *)virt_addr);
            break;
        case 'w':
            read_result = *((unsigned int *)virt_addr);
            break;
        default:
            fprintf(stderr, "Illegal data type '%c'.\n", access_type);
            exit(2);
        }
        printf("        0x%X \n", read_result);
        fflush(stdout);
    }

    if (argc == 4) {
        virt_addr32 = (unsigned int *)virt_addr;
        for (i = 0; i <= len / 4; i++) {
            printf("0x%08X -- 0x%08X  \n", target + i * 4,
                   *((unsigned int *)virt_addr32++));
        }

        fflush(stdout);
    }

    if (argc == 5) {
        writeval = strtoul(argv[4], 0, 0);
        switch (access_type) {
        case 'b':
            *((unsigned char *)virt_addr) = writeval;
            break;
        case 'h':
            *((unsigned short *)virt_addr) = writeval;
            break;
        case 'w':
            *((unsigned int *)virt_addr) = writeval;
            break;
        }
        printf(" 0x%lX  Written 0x%X;\n", target, writeval);
        fflush(stdout);
    }

    if (munmap(map_base, size) == -1)
        FATAL;
    close(fd);
    return 0;
}
