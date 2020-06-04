/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Verisilicon Inc.
 */

#ifndef _CB_HW_MONITOR_H_
#define _CB_HW_MONITOR_H_

#include <linux/types.h>

#include "common.h"

int hw_monitor_init(struct cb_tranx_t *tdev);
int hw_monitor_release(struct cb_tranx_t *tdev);
irqreturn_t hw_monitor_isr(int irq, void *data);

#endif /* _CB_HW_MONITOR_H_ */
