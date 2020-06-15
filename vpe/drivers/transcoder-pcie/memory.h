/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Verisilicon Inc.
 */

#ifndef _CB_MEMORY_H_
#define _CB_MEMORY_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#include "common.h"

int cb_mem_init(struct cb_tranx_t *tdev);
int cb_mem_release(struct cb_tranx_t *tdev);
long cb_mem_ioctl(struct file *filp,
		      unsigned int cmd,
		      unsigned long arg,
		      struct cb_tranx_t *tdev);
void cb_mem_close(struct cb_tranx_t *tdev, struct file *filp);

#endif /* _CB_MEMORY_H_ */
