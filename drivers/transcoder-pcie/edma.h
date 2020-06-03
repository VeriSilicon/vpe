/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Verisilicon Inc.
 */

#ifndef _CB_EDMA_H_
#define _CB_EDMA_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#include "common.h"

long edma_ioctl(struct file *filp, unsigned int cmd, unsigned long arg,
                struct cb_tranx_t *tdev);
int edma_init(struct cb_tranx_t *tdev);
void edma_release(struct cb_tranx_t *tdev);
int edma_normal_rc2ep_xfer(struct trans_pcie_edma *edma_info,
                           struct cb_tranx_t *tdev);
irqreturn_t edma_isr(int irq, void *data);

#endif /* _CB_EDMA_H_ */
