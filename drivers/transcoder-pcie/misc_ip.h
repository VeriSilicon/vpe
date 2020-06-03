/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Verisilicon Inc.
 */

#ifndef _CB_MISC_IP_H_
#define _CB_MISC_IP_H_

#include <linux/types.h>
#include <linux/ioctl.h>
#include "common.h"

int misc_ip_init(struct cb_tranx_t *tdev);
int misc_ip_release(struct cb_tranx_t *tdev);
long misc_ip_ioctl(struct file *filp, unsigned int cmd, unsigned long arg,
                   struct cb_tranx_t *tdev);
int dtrc_reset(struct cb_tranx_t *data, u32 id);
int tcache_init(struct cb_tranx_t *tdev);
int tcache_reset(struct cb_tranx_t *tdev);
int enable_all_pll(struct cb_tranx_t *tdev);
int adjust_video_pll(struct cb_tranx_t *tdev, u32 pll_m, u32 pll_s, u32 pll_id);

#endif /* _CB_MISC_IP_H_ */
