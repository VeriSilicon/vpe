// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Verisilicon Inc.
 */

#include <linux/pci.h>
#include <linux/pagemap.h>

#include "common.h"
#include "transcoder.h"

void __trans_dbg(void *dev, int level, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;
	struct cb_tranx_t *tdev = dev;
	struct pci_dev *pdev = tdev->pdev;

	BUG_ON(!tdev);
	if (level > tdev->print_level)
		return;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	printk(KERN_ERR "%s %04x:%02x:%02x.%d %pV",
		tdev->dev_name,
		pci_domain_nr(pdev->bus),
		pdev->bus->number,
		PCI_SLOT(pdev->devfn),
		PCI_FUNC(pdev->devfn), &vaf);

	va_end(args);
}

