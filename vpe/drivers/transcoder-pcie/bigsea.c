// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Verisilicon Inc.
 *
 * This is bigsea management driver for Linux.
 * Bigsea is a video encoder, has two cores, only support vp9 format.
 * This driver provide some IOCTL commands for userspace,
 * like reserve/release a hardware core, access registers.
 * At the same time, bigsea and vc8000e only one can work, so they use
 * same functions to reserve/release core.
 * How to operate Bigsea: reserve a idle core, config registers,
 * enable the core, wait interrupt, release the core.
 */

#include <linux/pci.h>
#include <linux/pagemap.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/delay.h>

#include "common.h"
#include "encoder.h"
#include "transcoder.h"

/*
 * BIGSEA_WAIT_TIMEOUT is encode one frame timeout time, 2 seconds is enough.
 */
#ifndef EMULATOR
#define BIGSEA_WAIT_TIMEOUT (2 * HZ)
#else
#define BIGSEA_WAIT_TIMEOUT (600 * HZ)
#endif

#define BIGSEA_CLOCK_CONTROL

/*  bigsea core index */
#define S0_BIGSEA 0
#define S1_BIGSEA 1

#define BIGSEA0_REG_OFF 0x3130000
#define BIGSEA1_REG_OFF 0x3530000

#define BIGSEA_IRQ_ST 2
#define BIGSEA_IRQ_STAT_OFF (BIGSEA_IRQ_ST * 4)
#define BIGSEA_ENABLE 0x01
#define BIGSEA_ABORT 0x00
#define BIGSEA_IRQ_DISABLE 0x00
#define BIGSEA_IRQ 0x40
#define BIGSEA_TIMEOUT 0x01
#define BIGSEA_BUSERROR 0x02

#define BIGSEA_SW_IRQ_EN (1 << 11)

#define S0_BIGSEA_CLK (CLK_CON_STUS + 0x14)
#define S1_BIGSEA_CLK (CLK_CON_STUS + 0x30)

#define S0_BIGSEA_RST (RST_CON_STUS + 0x10)
#define S1_BIGSEA_RST (RST_CON_STUS + 0x28)

/*bigsea total registers count */
#define BIGSEA_REGS 212
#define BIGSEA_CODEC_LAST_REG (BIGSEA_REGS - 1)
#define BIGSEA_IO_SIZE_MAX (BIGSEA_REGS * 4)

#define CODEC_IO_SIZE_0 (BIGSEA_REGS * 4)
#define CODEC_IO_SIZE_1 (BIGSEA_REGS * 4)

/* bigsea hardware ID */
#define BIGSEA_HW_ID 0xB16C

extern const char *core_status[5];

/* offset adress, io space size, irq index */
static const unsigned int bigsea_cores[BIGSEA_MAX_CORES][3] = {
    { BIGSEA0_REG_OFF, CODEC_IO_SIZE_0, IRQ_S0_BIGSEA },
    { BIGSEA1_REG_OFF, CODEC_IO_SIZE_1, IRQ_S1_BIGSEA }
};

void bigsea_enable_clock(void *data, int id)
{
    u32 st;
    struct bigsea_t *tbigsea = data;

    switch (id) {
    case 0:
        st = ccm_read(tbigsea->tdev, S0_BIGSEA_CLK);
        st |= CLK_ENABLE;
        ccm_write(tbigsea->tdev, S0_BIGSEA_CLK, st);
        break;
    case 1:
        st = ccm_read(tbigsea->tdev, S1_BIGSEA_CLK);
        st |= CLK_ENABLE;
        ccm_write(tbigsea->tdev, S1_BIGSEA_CLK, st);
        break;
    default:
        trans_dbg(tbigsea->tdev, TR_ERR, "bigsea: %s core id:%d error.\n",
                  __func__, id);
        break;
    }
}

void bigsea_disable_clock(struct bigsea_t *tbigsea, int id)
{
    u32 st;

#ifndef BIGSEA_CLOCK_CONTROL
    return;
#endif
    switch (id) {
    case 0:
        st = ccm_read(tbigsea->tdev, S0_BIGSEA_CLK);
        st &= (~CLK_ENABLE);
        ccm_write(tbigsea->tdev, S0_BIGSEA_CLK, st);
        break;
    case 1:
        st = ccm_read(tbigsea->tdev, S1_BIGSEA_CLK);
        st &= (~CLK_ENABLE);
        ccm_write(tbigsea->tdev, S1_BIGSEA_CLK, st);
        break;
    default:
        trans_dbg(tbigsea->tdev, TR_ERR, "bigsea: %s core id:%d error.\n",
                  __func__, id);
        break;
    }
}

void check_bigsea_hwerr(struct cb_tranx_t *tdev, u32 status)
{
    u32 irq_stats;

    irq_stats = (status >> 2) & 0x1EF;
    if (irq_stats & BIGSEA_TIMEOUT) {
        tdev->hw_err_flag = HW_ERR_FLAG;
        trans_dbg(tdev, TR_ERR, "bigsea: %s, hw timeout.\n", __func__);
    } else if (irq_stats & BIGSEA_BUSERROR) {
        tdev->hw_err_flag = HW_ERR_FLAG;
        trans_dbg(tdev, TR_ERR, "bigsea: %s, hw bus error\n", __func__);
    }
}

/* check whether the core receive irq. */
static int bigsea_check_irq(struct bigsea_t *tbigsea, int id)
{
    int rdy = 0;

    if (tbigsea->core[id].irq_rcvd == 1) {
        tbigsea->core[id].irq_rcvd = 0;
        rdy                        = 1;

        if (tbigsea->core[id].core_status != RCVD_IRQ_FLAG) {
            trans_dbg(tbigsea->tdev, TR_ERR,
                      "bigsea: %s core_%d_status:%s error\n", __func__, id,
                      core_status[tbigsea->core[id].core_status]);
        }

        tbigsea->core[id].core_status = CHK_IRQ_FLAG;
        tbigsea->core[id].chk_irq_cnt++;
    }

    return rdy;
}

static int bigsea_reserve_core(struct cb_tranx_t *tdev, struct core_info info,
                               struct file *filp)
{
    int ret;
    u32 id;
    struct bigsea_t *tbigsea = tdev->modules[TR_MODULE_BIGSEA];

    ret = reserve_encoder(tdev, &id, info.task_priority);
    trans_dbg(tdev, TR_DBG, "bigsea: reserve core:%d,priority:%d\n", id,
              info.task_priority);

    if (!ret) {
        //		adjust_enc_pll(tdev, id, TR_PLL_BIGSEA);
        bigsea_enable_clock(tbigsea, id);
        tbigsea->core[id].filp = filp;

        tbigsea->core[id].core_status = RSV_FLAG;
        tbigsea->core[id].rsv_cnt++;
        tbigsea->core[id].irq_rcvd = 0;
        ret                        = id;
    }

    return ret;
}

static int bigsea_release_core(struct cb_tranx_t *tdev, u32 id, int mode,
                               struct file *filp)
{
    u32 status;
    struct bigsea_t *tbigsea = tdev->modules[TR_MODULE_BIGSEA];

    if (tbigsea->core[id].filp == NULL) {
        trans_dbg(tdev, TR_ERR,
                  "bigsea: %s core:%d is not reserve, status=%d\n", __func__,
                  id, tbigsea->core[id].core_status);
        return release_encoder(tdev, id);
    }

    if (mode == ABNORM_EXIT) {
        usleep_range(50000, 60000);
        trans_dbg(tbigsea->tdev, TR_ERR,
                  "bigsea: %s, abnorm exit, wait core_%d 50ms for ip done, "
                  "core status:%d\n",
                  __func__, id, tbigsea->core[id].core_status);
    }

    if (tbigsea->core[id].core_status != CHK_IRQ_FLAG) {
        trans_dbg(tbigsea->tdev, TR_ERR, "bigsea: %s core_%d_status:%s error\n",
                  __func__, id, core_status[tbigsea->core[id].core_status]);
    }

    status = readl(tbigsea->core[id].hwregs + BIGSEA_IRQ_STAT_OFF);
    /* make sure HW is disabled */
    if (status & BIGSEA_ENABLE) {
        trans_dbg(tdev, TR_ERR,
                  "bigsea: %s, core:%d is enabled, force "
                  "reset,hw_status=0x%x\n",
                  __func__, id, status);

        /* abort codec */
        status |= BIGSEA_ABORT | BIGSEA_IRQ_DISABLE;
        writel(status, tbigsea->core[id].hwregs + BIGSEA_IRQ_STAT_OFF);
    }

    tbigsea->core[id].core_status = IDLE_FLAG;
    tbigsea->core[id].idle_cnt++;
    tbigsea->core[id].irq_rcvd = 0;

    bigsea_disable_clock(tbigsea, id);
    tbigsea->core[id].filp = NULL;
    return release_encoder(tdev, id);
}

void bigsea_close(struct cb_tranx_t *tdev, struct file *filp)
{
    int id;
    struct bigsea_t *tbigsea = tdev->modules[TR_MODULE_BIGSEA];

    for (id = 0; id < BIGSEA_MAX_CORES; id++) {
        if (tbigsea->core[id].filp == filp) {
            trans_dbg(tdev, TR_ERR,
                      "bigsea: Abnormal exit, %s core:%d, filp=%p\n\n",
                      __func__, id, filp);
            //			bigsea_release_core(tdev, id, ABNORM_EXIT,
            //filp);
        }
    }
}

/* wait core irq, BIGSEA_WAIT_TIMEOUT is timeout time. */
static int bigsea_wait_ready(struct bigsea_t *tbigsea, struct core_desc *core,
                             struct file *filp)
{
    u32 id = core->id;
    int ret;
    struct video_core_info *core_info = &tbigsea->core[id];

    ret = wait_event_interruptible_timeout(tbigsea->codec_wait_queue,
                                           bigsea_check_irq(tbigsea, id),
                                           BIGSEA_WAIT_TIMEOUT);
    if (ret == 0) {
        trans_dbg(tbigsea->tdev, TR_ERR,
                  "bigsea: %s core:%d timeout, hw_status=0x%x\n", __func__, id,
                  readl(core_info->hwregs + BIGSEA_IRQ_STAT_OFF));
        tbigsea->tdev->hw_err_flag = HW_ERR_FLAG;
        ret                        = -EFAULT;
    } else if (ret < 0) {
        /*
         * according application requirement, if wait is interrupted by
         * signal, it still return 0.
         */
        if (ret == -ERESTARTSYS)
            ret = 0;
    } else
        ret = 0;

    return ret;
}

/*  check hardware id. */
static void bigsea_check_id(struct bigsea_t *tbigsea)
{
    int i;
    unsigned int hwid;

    tbigsea->cores = 0;
    for (i = 0; i < BIGSEA_MAX_CORES; i++) {
        if (tbigsea->core[i].hwregs != 0) {
            /* to check there is a core indeed. */
            hwid = readl(tbigsea->core[i].hwregs);
            /* product version only */
            hwid = hwid & 0xFFFF;

            if (hwid == BIGSEA_HW_ID) {
                tbigsea->cores++;
                tbigsea->core[i].hw_id = hwid;
            } else {
                trans_dbg(tbigsea->tdev, TR_ERR,
                          "bigsea: Unknown HW found at 0x%lx,id=0x%x\n",
                          tbigsea->core[i].hwbase, hwid);
                continue;
            }

            trans_dbg(tbigsea->tdev, TR_DBG,
                      "bigsea: Supported HW found at 0x%lx,ID:0x%x\n",
                      tbigsea->core[i].hwbase, hwid);
        }
    }
}

/* bigsea interrupt handling function. */
irqreturn_t bigsea_isr(int index, void *data)
{
    unsigned int handled = 0;
    u32 irq_status_codec, val;
    irqreturn_t ret              = IRQ_NONE;
    struct cb_tranx_t *tdev      = data;
    struct bigsea_t *tbigsea     = tdev->modules[TR_MODULE_BIGSEA];
    struct video_core_info *core = &tbigsea->core[index];

    if (spin_trylock(&core->irq_lock) == 0)
        return IRQ_NONE;

    /* reconfirm bigsea status in global interrupt register. */
    val = ccm_read(tdev, GLOBAL_IRQ_REG_OFF);
    if (((index == S0_BIGSEA) && (!(val & THS0_BIGSEA))) ||
        ((index == S1_BIGSEA) && (!(val & THS1_BIGSEA)))) {
        spin_unlock(&core->irq_lock);
        return IRQ_NONE;
    }

    if (core->core_status != RSV_FLAG) {
        trans_dbg(tdev, TR_ERR, "bigsea: %s, core_%d_status:0x%x err\n",
                  __func__, index, core->core_status);
        spin_unlock(&core->irq_lock);
        return IRQ_NONE;
    }

    irq_status_codec = readl(core->hwregs + BIGSEA_IRQ_STAT_OFF);
    if (irq_status_codec & BIGSEA_IRQ) {
        val = irq_status_codec & (~BIGSEA_IRQ);
        val = (val >> 2) & 0x1EF;
        /* check whether the core has error status.*/
        if (val & BIGSEA_TIMEOUT) {
            tbigsea->tdev->hw_err_flag = HW_ERR_FLAG;
            trans_dbg(tdev, TR_ERR, "bigsea: %s, core_%d hw timeout, hw=0x%x\n",
                      __func__, index, irq_status_codec);
        } else if (val & BIGSEA_BUSERROR) {
            tbigsea->tdev->hw_err_flag = HW_ERR_FLAG;
            trans_dbg(tdev, TR_ERR,
                      "bigsea: %s, core_%d hw_bus error, hw=0x%x\n", __func__,
                      index, irq_status_codec);
        }

        /* clear codec IRQ */
        irq_status_codec &= (~BIGSEA_IRQ);
        irq_status_codec &= (~BIGSEA_SW_IRQ_EN);
        writel(irq_status_codec, core->hwregs + BIGSEA_IRQ_STAT_OFF);
        core->irq_cnt++;
        core->core_status = RCVD_IRQ_FLAG;
        core->irq_rcvd    = 1;

        handled++;
        ret = IRQ_HANDLED;
    }
    spin_unlock(&core->irq_lock);

    if (handled)
        wake_up_all(&tbigsea->codec_wait_queue);

    return ret;
}

/* clear all regisetr to 0. */
static void bigsea_clear_all_regs(struct bigsea_t *tbigsea)
{
    int i, j;
    u32 st;

    for (j = 0; j < tbigsea->cores; j++) {
        st = readl(tbigsea->core[j].hwregs + BIGSEA_IRQ_STAT_OFF);
        if (st & BIGSEA_ENABLE) {
            /* abort with IRQ disabled */
            st = BIGSEA_ABORT | BIGSEA_IRQ_DISABLE;
            writel(st, tbigsea->core[j].hwregs + BIGSEA_IRQ_STAT_OFF);
        }

        for (i = 8; i < tbigsea->core[j].iosize; i += 4)
            writel(0, tbigsea->core[j].hwregs + i);
    }
}

static int bigsea_register_irq(struct cb_tranx_t *tdev)
{
    int i, n = 0;
    int ret;
    struct bigsea_t *tbigsea;

    tbigsea = tdev->modules[TR_MODULE_BIGSEA];

    /* register irq for each core */
    for (i = 0; i < tbigsea->cores; i++) {
        if (tbigsea->core[i].irq != -1) {
            trans_dbg(tdev, TR_DBG, "bigsea: %s cord_id:%d IRQ is %d!\n",
                      __func__, i, tbigsea->core[i].irq);
            ret = request_irq(tbigsea->core[i].irq, unify_isr,
                              IRQF_SHARED | IRQF_NO_THREAD, "bigsea",
                              (void *)tdev);
            if (ret != 0) {
                if (ret == -EINVAL)
                    trans_dbg(tdev, TR_ERR, "bigsea: Bad IRQ:%d or handler\n",
                              tbigsea->core[i].irq);
                else if (ret == -EBUSY)
                    trans_dbg(tdev, TR_ERR, "bigsea: IRQ:%d busy\n",
                              tbigsea->core[i].irq);
                goto out_irq_free;
            }
            n++;
        } else
            trans_dbg(tdev, TR_ERR, "bigsea: irq:%d not in use!\n",
                      tbigsea->core[i].irq);
    }

    return 0;

out_irq_free:
    for (i = 0; i < n; i++) {
        if (tbigsea->core[i].irq != -1)
            free_irq(tbigsea->core[i].irq, (void *)tdev);
    }

    return -1;
}

int bigsea_init(struct cb_tranx_t *tdev)
{
    int i;
    struct bigsea_t *tbigsea;

    tbigsea = kzalloc(sizeof(struct bigsea_t), GFP_KERNEL);
    if (!tbigsea) {
        trans_dbg(tdev, TR_ERR, "bigsea: %s kmalloc bigsea dev failed\n",
                  __func__);
        goto out;
    }
    tdev->modules[TR_MODULE_BIGSEA] = tbigsea;
    tbigsea->tdev                   = tdev;

    /* enable bigsea clock */
    ccm_write(tdev, S0_BIGSEA_CLK, CLK_ENABLE | DIV_1);
    ccm_write(tdev, S1_BIGSEA_CLK, CLK_ENABLE | DIV_1);

    for (i = 0; i < BIGSEA_MAX_CORES; i++) {
        tbigsea->core[i].hwbase =
            pci_resource_start(tdev->pdev, 2) + bigsea_cores[i][0];
        tbigsea->core[i].hwregs = tdev->bar2_virt + bigsea_cores[i][0];
        tbigsea->core[i].iosize = bigsea_cores[i][1];
        tbigsea->core[i].irq = pci_irq_vector(tdev->pdev, bigsea_cores[i][2]);
        spin_lock_init(&tbigsea->core[i].irq_lock);
    }

    bigsea_check_id(tbigsea);
    if (!tbigsea->cores) {
        trans_dbg(tdev, TR_ERR, "bigsea: not valid core\n");
        goto out_free_dev;
    }

    trans_dbg(tdev, TR_DBG, "bigsea: support two slices, irq:%d %d\n",
              tbigsea->core[S0_BIGSEA].irq, tbigsea->core[S1_BIGSEA].irq);

    init_waitqueue_head(&tbigsea->codec_wait_queue);
    /* clear all regs */
    bigsea_clear_all_regs(tbigsea);

    /*
     * End of initialization will disable all cores' clock,
     * when reserve a idle core,will enable the corresponding core's clock
     */
    for (i = 0; i < tbigsea->cores; i++)
        bigsea_disable_clock(tbigsea, i);

    if (bigsea_register_irq(tdev)) {
        trans_dbg(tdev, TR_ERR, "bigsea: register irq failed.\n");
        goto out_free_dev;
    }

    trans_dbg(tdev, TR_INF, "bigsea: module initialize done.\n");
    return 0;

out_free_dev:
    kfree(tbigsea);
out:
    trans_dbg(tbigsea->tdev, TR_ERR, "bigsea: module initialize failed.\n");
    return -EFAULT;
}

static void bigsea_free_irq(struct cb_tranx_t *tdev)
{
    int i;
    struct bigsea_t *tbigsea = tdev->modules[TR_MODULE_BIGSEA];

    /* free the IRQ */
    for (i = 0; i < tbigsea->cores; i++) {
        if (tbigsea->core[i].irq != -1) {
            trans_dbg(tbigsea->tdev, TR_DBG,
                      "bigsea: free irq:%d of core:%d.\n", tbigsea->core[i].irq,
                      i);
            free_irq(tbigsea->core[i].irq, (void *)tdev);
        }
    }
}

int bigsea_release(struct cb_tranx_t *tdev)
{
    int i;
    struct bigsea_t *tbigsea = tdev->modules[TR_MODULE_BIGSEA];

    bigsea_free_irq(tdev);
    /* disable clock */
    for (i = 0; i < tbigsea->cores; i++)
        bigsea_disable_clock(tbigsea, i);
    kfree(tbigsea);

    trans_dbg(tdev, TR_DBG, "bigsea: remove module done.\n");
    return 0;
}

long bigsea_ioctl(struct file *filp, unsigned int cmd, unsigned long arg,
                  struct cb_tranx_t *tdev)
{
    int ret = 0;
    u32 id, io_size;
    struct core_info info;
    struct core_desc core;
    void __user *argp        = (void __user *)arg;
    struct bigsea_t *tbigsea = tdev->modules[TR_MODULE_BIGSEA];

    switch (cmd) {
    case CB_TRANX_BIGSEA_IO_SIZE:
        __get_user(id, (u32 *)argp);
        if (id >= tbigsea->cores)
            return -EFAULT;
        io_size = tbigsea->core[id].iosize;
        __put_user(io_size, (u32 *)argp);
        break;
    case CB_TRANX_BIGSEA_CORE_CNT:
        __put_user(tbigsea->cores, (unsigned int *)argp);
        trans_dbg(tdev, TR_DBG, "bigsea: core count:%d\n", tbigsea->cores);
        break;
    case CB_TRANX_BIGSEA_GET_HWID:
        __get_user(id, (u32 *)argp);
        if (id >= tbigsea->cores)
            return -EFAULT;
        if (tdev->hw_err_flag)
            return tdev->hw_err_flag;
        id = tbigsea->core[id].hw_id;
        __put_user(id, (u32 *)argp);
        break;
    case CB_TRANX_BIGSEA_RESERVE:
        if (copy_from_user(&info, argp, sizeof(struct core_info))) {
            trans_dbg(tdev, TR_ERR, "bigsea: reserve copy from user failed\n");
            return -EFAULT;
        }
        ret = bigsea_reserve_core(tdev, info, filp);
        break;
    case CB_TRANX_BIGSEA_RELEASE:
        __get_user(id, (u32 *)argp);
        trans_dbg(tdev, TR_DBG, "bigsea: release core:%d\n", id);
        if (id >= tbigsea->cores) {
            trans_dbg(tdev, TR_ERR, "bigsea: release core id:%d error.\n", id);
            return -EFAULT;
        }
        ret = bigsea_release_core(tdev, id, NORM_EXIT, filp);
        break;
    case CB_TRANX_BIGSEA_WAIT_DONE:
        if (copy_from_user(&core, argp, sizeof(struct core_desc))) {
            trans_dbg(tdev, TR_ERR, "bigsea: wait copy from user failed\n");
            return -EFAULT;
        }
        ret = bigsea_wait_ready(tbigsea, &core, filp);
        break;
    default:
        trans_dbg(tdev, TR_ERR, "bigsea: %s, cmd:0x%x is error.\n", __func__,
                  cmd);
        ret = -EINVAL;
    }

    return ret;
}
