/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Verisilicon Inc.
 */

#ifndef _CB_PCIE_H_
#define _CB_PCIE_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#include "common.h"

int cb_pci_init(struct cb_tranx_t *tdev);
long cb_pci_ioctl(struct file *filp,
		      unsigned int cmd,
		      unsigned long arg,
		      struct cb_tranx_t *tdev);
void cb_pci_release(struct cb_tranx_t *tdev);

#endif /* _CB_PCIE_H_ */
