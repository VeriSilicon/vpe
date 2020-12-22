# Copyright (C) 2020 VeriSilicon Holdings Co., Ltd.

ifeq ($(obj),)
obj = .
endif

ifeq ($(EMULATOR),y)
EXTRA_CFLAGS += -DEMULATOR
endif

ifeq ($(HW_HANDSHAKE),y)
EXTRA_CFLAGS += -DENABLE_HANDSHAKE
endif

-include ../config.mk

transcoder_pcie-objs += transcoder.o pcie.o edma.o vc8000e.o bigsea.o vc8000d.o
transcoder_pcie-objs += memory.o interrupt.o debug_trace.o hw_monitor.o
transcoder_pcie-objs += encoder.o misc_ip.o

obj-m += transcoder_pcie.o

KERN_DIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KERN_DIR) M=`pwd` modules

test:
	$(CC) -o edma_test_hugepage edma_test_hugepage.c -lhugetlbfs
	$(CC) -o edma_link_test edma_link_test.c -lhugetlbfs
	$(CC) -o mem_test mem_test.c
	$(CC) -o edma_phyaddr_test edma_phyaddr_test.c
	$(CC) -o pcie_bw_test pcie_bw_test.c -lhugetlbfs
	$(CC) -o pcie_ddr_memtest pcie_ddr_memtest.c -lhugetlbfs

.PHONY: clean
clean:
	make -C $(KERN_DIR) M=`pwd` clean
	rm -rf edma_test_hugepage mem_test edma_link_test edma_phyaddr_test pcie_bw_test pcie_ddr_memtest
