// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Verisilicon Inc.
 */

#include <linux/pci.h>
#include <linux/pagemap.h>

#include "common.h"
#include "encoder.h"
#include "vc8000d.h"
#include "hw_monitor.h"
#include "transcoder.h"
#include "edma.h"

irqreturn_t unify_isr(int irq, void *data)
{
    u32 val;
    struct cb_tranx_t *tdev = data;

    do {
        if (tdev->hw_err_flag)
            return IRQ_NONE;

        /* read global interrupt status. */
        val = ccm_read(tdev, GLOBAL_IRQ_REG_OFF);
        if (val & ABORT_INTERRUPT) {
            tdev->hw_err_flag = HW_ERR_FLAG;
            trans_dbg(tdev, TR_ERR, "global interrupt status:0x%x\n", val);
            return IRQ_NONE;
        }

        if (val & HW_MONITOR)
            hw_monitor_isr(0, tdev);

        if (val & THS0_VCD_A)
            vcd_isr(0, tdev);
        if (val & THS0_VCD_B)
            vcd_isr(1, tdev);
        if (val & THS1_VCD_A)
            vcd_isr(2, tdev);
        if (val & THS1_VCD_B)
            vcd_isr(3, tdev);

        if (val & THS0_VCE)
            vce_isr(0, tdev);
        if (val & THS1_VCE)
            vce_isr(1, tdev);

        if (val & THS0_BIGSEA)
            bigsea_isr(0, tdev);
        if (val & THS1_BIGSEA)
            bigsea_isr(1, tdev);
    } while (0);

    return IRQ_HANDLED;
}
