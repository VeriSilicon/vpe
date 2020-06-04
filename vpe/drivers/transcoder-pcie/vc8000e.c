// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Verisilicon Inc.
 *
 * This is vc8000e management driver for Linux.
 * vc8000e is a video encoder, has two cores, support hevc/h264 format.
 * This driver provide some IOCTL commands for userspace,
 * like reserve/release a hardware core, access registers.
 * At the same time, bigsea and vc8000e only one can work, so they use
 * same functions to reserve/release core.
 * How to operate vc8000e: reserve a idle core, config registers,
 * enable the core, wait interrupt, release the core.
 *
 * The abbreviation of vc8000e is vce.
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
 * VC8000E_WAIT_TIMEOUT is encode one frame timeout time, 2 seconds is enough.
 */
#ifndef EMULATOR
#define VC8000E_TIMEOUT (2 * HZ) /* second */
#else
#define VC8000E_TIMEOUT (600 * HZ) /* second */
#endif

#define VCE_CLOCK_CONTROL

/* vc8000e core index */
#define S0_VCE 0
#define S1_VCE 1

#define CORE_0_IO_ADDR_OFF 0x3120000
#define CORE_1_IO_ADDR_OFF 0x3520000

#define THS0_VCE_CLK_CON (CLK_CON_STUS + 0x4)
#define THS1_VCE_CLK_CON (CLK_CON_STUS + 0x20)

/* hw ID high 16 bits */
#define ENC_HW_ID1 0x4832
#define ENC_HW_ID2 0x8000

/* VC8000E total regs */
#define VCE_REGS_CNT 394
#define VCE_LAST_REG (VCE_REGS_CNT - 1)

#define CORE_0_IO_SIZE (VCE_REGS_CNT * 4)
#define CORE_1_IO_SIZE (VCE_REGS_CNT * 4)

#define INTERRUPT_REGISTER 1
#define CONTRO_REGISTER_1 5
#define HW_SYNTHESIS_CONFIG 80
#define HW_SYNTHESIS_CONFIG_2 214
#define HW_SYNTHESIS_CONFIG_3 226

#define SW_ENC_IRQ 0x1
#define SW_ENC_IRQ_DIS (1 << 1)

extern const char *core_status[5];

/* offset adress, io space size, irq index */
static const unsigned int vce_cores[VCE_MAX_CORES][3] = {
    { CORE_0_IO_ADDR_OFF, CORE_0_IO_SIZE, IRQ_S0_VCE },
    { CORE_1_IO_ADDR_OFF, CORE_1_IO_SIZE, IRQ_S1_VCE }
};

void vce_enable_clock(void *d, u32 id)
{
    u32 status;
    struct vc8000e_t *tvce = d;

    id = (id & 0x3) - 1;
    switch (id) {
    case 0:
        status = ccm_read(tvce->tdev, THS0_VCE_CLK_CON);
        status |= CLK_ENABLE;
        ccm_write(tvce->tdev, THS0_VCE_CLK_CON, status);
        break;
    case 1:
        status = ccm_read(tvce->tdev, THS1_VCE_CLK_CON);
        status |= CLK_ENABLE;
        ccm_write(tvce->tdev, THS1_VCE_CLK_CON, status);
        break;
    default:
        trans_dbg(tvce->tdev, TR_ERR, "vc8000e: %s core id:%d error.\n",
                  __func__, id);
        break;
    }
}

void vce_disable_clock(void *d, u32 id)
{
    u32 status;
    struct vc8000e_t *tvce = d;

#ifndef VCE_CLOCK_CONTROL
    return;
#endif
    id = (id & 0x3) - 1;
    switch (id) {
    case 0:
        status = ccm_read(tvce->tdev, THS0_VCE_CLK_CON);
        status &= (~CLK_ENABLE);
        ccm_write(tvce->tdev, THS0_VCE_CLK_CON, status);
        break;
    case 1:
        status = ccm_read(tvce->tdev, THS1_VCE_CLK_CON);
        status &= (~CLK_ENABLE);
        ccm_write(tvce->tdev, THS1_VCE_CLK_CON, status);
        break;
    default:
        trans_dbg(tvce->tdev, TR_ERR, "vc8000e: %s core id:%d error.\n",
                  __func__, id);
        break;
    }
}

/* check hardware id. */
static void vce_check_id(struct vc8000e_t *tvce)
{
    u32 hwid, found_hw = 0;
    int i;

    tvce->cores = 0;
    for (i = 0; i < VCE_MAX_CORES; i++) {
        /*read hwid and check validness and store it */
        hwid = readl(tvce->core[i].hwregs);

        /* check vc8000e HW ID */
        if ((((hwid >> 16) & 0xFFFF) != ENC_HW_ID1) &&
            (((hwid >> 16) & 0xFFFF) != ENC_HW_ID2)) {
            trans_dbg(tvce->tdev, TR_DBG, "vc8000e: HW not found at 0x%lx\n",
                      tvce->core[i].hwbase);
            continue;
        }

        tvce->core[i].hw_id = hwid;
        found_hw            = 1;
        tvce->cores++;

        trans_dbg(tvce->tdev, TR_DBG,
                  "vc8000e: Supported HW found at 0x%lx,ID:0x%x.\n",
                  tvce->core[i].hwbase, hwid);
    }

    if (!found_hw)
        trans_dbg(tvce->tdev, TR_ERR, "vc8000e: NO ANY HW found!!\n");
}

/* clear all regisetr to 0. */
static void vce_clear_all_regs(struct vc8000e_t *tvce)
{
    int i, n;

    for (n = 0; n < tvce->cores; n++) {
        writel(0, tvce->core[n].hwregs + CONTRO_REGISTER_1 * 4);
        for (i = 4; i < tvce->core[n].iosize; i += 4)
            writel(0, tvce->core[n].hwregs + i);
    }
}

/* check whether the core receive irq. */
static int vce_check_irq(struct vc8000e_t *tvce, u32 *core_id, u32 *irq_status)
{
    int rdy = 0;
    u32 id;

    id = (*core_id & 0x3) - 1;
    if (tvce->core[id].irq_rcvd) {
        tvce->core[id].irq_rcvd = 0;
        rdy                     = 1;
        *core_id                = id;
        *irq_status             = tvce->core[id].irq_status;

        if (tvce->core[id].core_status != RCVD_IRQ_FLAG) {
            trans_dbg(tvce->tdev, TR_ERR,
                      "vc8000e: %s core_%d_status:%s error\n", __func__, id,
                      core_status[tvce->core[id].core_status]);
        }

        tvce->core[id].core_status = CHK_IRQ_FLAG;
        tvce->core[id].chk_irq_cnt++;
    }

    return rdy;
}

static unsigned int vce_wait_ready(struct vc8000e_t *tvce, u32 *core_id,
                                   u32 *irq_status)
{
    int ret;
    u32 id                       = (*core_id & 0x3) - 1;
    struct video_core_info *core = &tvce->core[id];

    ret = wait_event_interruptible_timeout(tvce->enc_wait_queue,
                                           vce_check_irq(tvce, core_id,
                                                         irq_status),
                                           VC8000E_TIMEOUT);
    if (!ret) {
        trans_dbg(tvce->tdev, TR_ERR,
                  "vc8000e: %s core:%d timeout, hw_status:0x%x.\n", __func__,
                  id, readl(core->hwregs + INTERRUPT_REGISTER * 4));
        ret = -EFAULT;
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

static int vce_reserve_core(struct cb_tranx_t *tdev, struct core_info info,
                            void __user *argp, struct file *filp)
{
    int ret = 0;
    u32 id;
    struct vc8000e_t *tvce;

    tvce = tdev->modules[TR_MODULE_VC8000E];
    ret  = reserve_encoder(tdev, &id, info.task_priority);
    trans_dbg(tdev, TR_DBG, "vc8000e: reserve core:%d priority:%d\n", id,
              info.task_priority);
    if (ret == 0) {
        info.format &= (~3);
        info.format |= (1 << id);
        ret = copy_to_user(argp, &info, sizeof(info));
        if (ret) {
            release_encoder(tdev, id);
            trans_dbg(tdev, TR_ERR, "vc8000e: reserve copy_to_user failed\n");
            ret = -EFAULT;
        } else {
            tvce->core[id].irq_status = 0;
            tvce->core[id].filp       = filp;
            tvce->core[id].irq_rcvd   = 0;
            //			adjust_enc_pll(tdev, id, TR_PLL_VC8000E);
            vce_enable_clock(tvce, info.format);
            tvce->core[id].core_status = RSV_FLAG;
            tvce->core[id].rsv_cnt++;
        }
    } else {
        trans_dbg(tdev, TR_ERR, "vc8000e: reserve_enc failed, ret:%d.\n", ret);
    }

    return ret;
}

static int vce_release_core(struct cb_tranx_t *tdev, u32 core_id, int mode,
                            struct file *filp)
{
    int ret = 0;
    u32 id;
    struct vc8000e_t *tvce;

    tvce = tdev->modules[TR_MODULE_VC8000E];
    id   = (core_id & 0x3) - 1;
    if (id >= tvce->cores) {
        trans_dbg(tdev, TR_ERR, "vc8000e: release id:%d is error.\n", id);
        return -EFAULT;
    }

    if (tvce->core[id].filp == NULL) {
        trans_dbg(tdev, TR_ERR,
                  "vc8000e: %s core:%d is not reserve, status=%d\n", __func__,
                  id, tvce->core[id].core_status);
        return release_encoder(tdev, id);
    }

    if (mode == ABNORM_EXIT) {
        usleep_range(50000, 60000);
        trans_dbg(tvce->tdev, TR_ERR,
                  "vc8000e: %s, abnorm exit, wait core_%d 50ms for ip done, "
                  "core status:%d\n",
                  __func__, id, tvce->core[id].core_status);
    }

    if (tvce->core[id].core_status != CHK_IRQ_FLAG) {
        trans_dbg(tvce->tdev, TR_ERR, "vc8000e: %s core_%d_status:%s error\n",
                  __func__, id, core_status[tvce->core[id].core_status]);
    }

    tvce->core[id].core_status = IDLE_FLAG;
    tvce->core[id].idle_cnt++;
    tvce->core[id].irq_rcvd = 0;
    vce_disable_clock(tvce, core_id);
    tvce->core[id].irq_status = 0;
    tvce->core[id].filp       = NULL;
    ret                       = release_encoder(tdev, id);
    trans_dbg(tdev, TR_DBG, "vc8000e: release core:%d\n", id);

    return ret;
}

void vce_close(struct cb_tranx_t *tdev, struct file *filp)
{
    int id;
    struct vc8000e_t *tvce = tdev->modules[TR_MODULE_VC8000E];

    for (id = 0; id < VCE_MAX_CORES; id++) {
        if (tvce->core[id].filp == filp) {
            trans_dbg(tdev, TR_ERR,
                      "vc8000e: Abnormal exit, %s core:%d, filp=%p\n", __func__,
                      id, filp);
            //			vce_release_core(tdev, 1<<id, ABNORM_EXIT,
            //filp);
        }
    }
}

/* vc8000e interrupt handling function. */
irqreturn_t vce_isr(int index, void *data)
{
    unsigned int handled = 0;
    u32 irq_status, val;
    u32 hwId, majorId, wClr;
    irqreturn_t ret = IRQ_NONE;

    struct cb_tranx_t *tdev      = data;
    struct vc8000e_t *tvce       = tdev->modules[TR_MODULE_VC8000E];
    struct video_core_info *core = &tvce->core[index];

    if (spin_trylock(&core->irq_lock) == 0)
        return IRQ_NONE;

    /* reconfirm vc8000e status in global interrupt register. */
    val = ccm_read(tdev, GLOBAL_IRQ_REG_OFF);
    if (((index == S0_VCE) && (!(val & THS0_VCE))) ||
        ((index == S1_VCE) && (!(val & THS1_VCE)))) {
        spin_unlock(&core->irq_lock);
        return IRQ_NONE;
    }

    if (RSV_FLAG != core->core_status) {
        trans_dbg(tdev, TR_ERR, "vc8000e: %s, core_%d_status:0x%x err\n",
                  __func__, index, core->core_status);
        spin_unlock(&core->irq_lock);
        return IRQ_NONE;
    }

    irq_status = readl(core->hwregs + INTERRUPT_REGISTER * 4);
    if (irq_status & SW_ENC_IRQ) {
        /* mask vce */
        val = irq_status | SW_ENC_IRQ_DIS;
        writel(val, core->hwregs + INTERRUPT_REGISTER * 4);

        /*
         * clear all IRQ bits.
         * hwId >= 0x80006100 means IRQ is cleared by writting 1.
         */
        hwId    = readl(core->hwregs);
        majorId = (hwId & 0x0000FF00) >> 8;
        wClr    = (majorId >= 0x61) ? irq_status : (irq_status & (~0x1FD));
        writel(wClr, core->hwregs + INTERRUPT_REGISTER * 4);
        core->irq_status = irq_status & (~SW_ENC_IRQ);
        /* unmask vce */
        val = readl(core->hwregs + INTERRUPT_REGISTER * 4);
        val &= (~SW_ENC_IRQ_DIS);
        writel(val, core->hwregs + INTERRUPT_REGISTER * 4);
        core->core_status = RCVD_IRQ_FLAG;
        core->irq_cnt++;
        core->irq_rcvd = 1;

        handled++;
        ret = IRQ_HANDLED;
    }
    spin_unlock(&core->irq_lock);

    if (handled)
        wake_up_all(&tvce->enc_wait_queue);

    return ret;
}

int vc8000e_register_irq(struct cb_tranx_t *tdev)
{
    int i, n;
    int ret;
    struct vc8000e_t *tvce;

    tvce = tdev->modules[TR_MODULE_VC8000E];
    /* register irq for each core */
    for (i = 0; i < VCE_MAX_CORES; i++) {
        if (tvce->core[i].irq != -1) {
            trans_dbg(tvce->tdev, TR_DBG, "vc8000e: %s core:%d IRQ is %d!\n",
                      __func__, i, tvce->core[i].irq);
            ret = request_irq(tvce->core[i].irq, unify_isr,
                              IRQF_SHARED | IRQF_NO_THREAD, "vc8000e",
                              (void *)tdev);
            if (ret == -EINVAL) {
                trans_dbg(tdev, TR_ERR,
                          "vc8000e: bad irq:%d number or handler\n",
                          tvce->core[i].irq);
                goto out_irq_free;
            } else if (ret == -EBUSY) {
                trans_dbg(tdev, TR_ERR,
                          "vc8000e: irq:%d busy, change your cfg\n",
                          tvce->core[i].irq);
                goto out_irq_free;
            } else {
                trans_dbg(tdev, TR_DBG, "vc8000e: request irq ok\n");
            }
        } else
            trans_dbg(tdev, TR_DBG, "vc8000e: core:%d irq is -1\n", i);
    }

    return 0;

out_irq_free:
    for (n = 0; n < i; n++) {
        if (tvce->core[n].irq != -1)
            free_irq(tvce->core[n].irq, (void *)tdev);
        vce_enable_clock(tvce, (1 << n));
        /* disable HW */
        writel(0, tvce->core[n].hwregs + INTERRUPT_REGISTER * 4);
        /* clear vce IRQ */
        writel(0, tvce->core[n].hwregs + CONTRO_REGISTER_1 * 4);
        vce_disable_clock(tvce, (1 << n));
    }

    return -1;
}

int vc8000e_init(struct cb_tranx_t *tdev)
{
    int i;
    struct vc8000e_t *tvce;

    tvce = kzalloc(sizeof(struct vc8000e_t), GFP_KERNEL);
    if (!tvce) {
        trans_dbg(tdev, TR_ERR, "vc8000e: alloc vc8000e_t failed.\n");
        goto out;
    }
    tdev->modules[TR_MODULE_VC8000E] = tvce;
    tvce->tdev                       = tdev;

    /* enable clock CLK: 0x01    -- divider is 1*/
    ccm_write(tdev, THS0_VCE_CLK_CON, CLK_ENABLE | DIV_1);
    ccm_write(tdev, THS1_VCE_CLK_CON, CLK_ENABLE | DIV_1);

    for (i = 0; i < VCE_MAX_CORES; i++) {
        tvce->core[i].hwbase =
            pci_resource_start(tdev->pdev, 2) + vce_cores[i][0];
        tvce->core[i].hwregs = tdev->bar2_virt + vce_cores[i][0];
        tvce->core[i].iosize = vce_cores[i][1];
        tvce->core[i].irq    = pci_irq_vector(tdev->pdev, vce_cores[i][2]);
        spin_lock_init(&tvce->core[i].irq_lock);
    }

    vce_check_id(tvce);
    if (!tvce->cores) {
        trans_dbg(tdev, TR_ERR, "vc8000e: core count is 0, init failed.\n");
        goto out_free_dev;
    }
    trans_dbg(tdev, TR_DBG, "vc8000e: core count is %d\n", tvce->cores);
    trans_dbg(tdev, TR_DBG, "vc8000e: support two slices, irq:%d %d.\n",
              tvce->core[S0_VCE].irq, tvce->core[S1_VCE].irq);

    /* reset hardware */
    vce_clear_all_regs(tvce);

    init_waitqueue_head(&tvce->enc_wait_queue);

    for (i = 0; i < VCE_MAX_CORES; i++) {
        tvce->vce_cfg[i].core_id = i;
        tvce->vce_cfg[i].vce_cfg_1 =
            readl(tvce->core[i].hwregs + HW_SYNTHESIS_CONFIG * 4);
        tvce->vce_cfg[i].vce_cfg_2 =
            readl(tvce->core[i].hwregs + HW_SYNTHESIS_CONFIG_2 * 4);
        tvce->vce_cfg[i].vce_cfg_3 =
            readl(tvce->core[i].hwregs + HW_SYNTHESIS_CONFIG_3 * 4);
        trans_dbg(tdev, TR_DBG,
                  "vc8000e:core_id=%d cfg_1=0x%x cfg_2=0x%x cfg_3=0x%x\n",
                  tvce->vce_cfg[i].core_id, tvce->vce_cfg[i].vce_cfg_1,
                  tvce->vce_cfg[i].vce_cfg_2, tvce->vce_cfg[i].vce_cfg_3);
    }

    /*
     * End of initialization will disable all cores' clock,
     * when reserve a idle core,will enable the corresponding core's clock
     */
    vce_disable_clock(tvce, 1 << S0_VCE);
    vce_disable_clock(tvce, 1 << S1_VCE);

    if (vc8000e_register_irq(tdev)) {
        trans_dbg(tdev, TR_ERR, "vc8000e: register irq failed.\n");
        goto out_free_dev;
    }

    trans_dbg(tdev, TR_INF, "vc8000e: module initialize done.\n");
    return 0;

out_free_dev:
    kfree(tvce);
out:
    trans_dbg(tdev, TR_ERR, "vc8000e: module initialize failed.\n");
    return -EFAULT;
}

static void vc8000e_free_irq(struct cb_tranx_t *tdev)
{
    int i = 0;
    struct vc8000e_t *tvce;

    tvce = tdev->modules[TR_MODULE_VC8000E];
    vce_enable_clock(tvce, 1 << S0_VCE); /* bit0 is 1 */
    vce_enable_clock(tvce, 1 << S1_VCE); /* bit1 is 1 */

    for (i = 0; i < tvce->cores; i++) {
        /* free the encoder IRQ */
        if (tvce->core[i].irq != -1) {
            trans_dbg(tdev, TR_DBG, "vc8000e: free irq,id:%d,irq:%d\n", i,
                      tvce->core[i].irq);
            free_irq(tvce->core[i].irq, (void *)tdev);
        }

        /* disable HW */
        writel(0, tvce->core[i].hwregs + INTERRUPT_REGISTER * 4);
        /* clear vce IRQ */
        writel(0, tvce->core[i].hwregs + CONTRO_REGISTER_1 * 4);
    }
}

int vc8000e_release(struct cb_tranx_t *tdev)
{
    struct vc8000e_t *tvce;

    tvce = tdev->modules[TR_MODULE_VC8000E];
    vc8000e_free_irq(tdev);
    vce_disable_clock(tvce, 1 << S0_VCE);
    vce_disable_clock(tvce, 1 << S1_VCE);
    kfree(tvce);
    trans_dbg(tdev, TR_DBG, "vc8000e: remove module done.\n");
    return 0;
}

long vc8000e_ioctl(struct file *filp, unsigned int cmd, unsigned long arg,
                   struct cb_tranx_t *tdev)
{
    int ret = 0;
    u32 id, val, irq_status;
    struct vc8000e_t *tvce;
    struct vce_core_config cfg;
    struct core_info info;
    void __user *argp = (void __user *)arg;

    tvce = tdev->modules[TR_MODULE_VC8000E];
    switch (cmd) {
    case CB_TRANX_VCE_IO_SIZE:
        __get_user(id, (u32 *)argp);
        if (id >= tvce->cores)
            return -EFAULT;
        val = tvce->core[id].iosize;
        __put_user(val, (u32 *)argp);
        break;
    case CB_TRANX_VCE_CORE_CNT:
        __put_user(tvce->cores, (u32 *)argp);
        break;
    case CB_TRANX_VCE_GET_HWID:
        __get_user(id, (u32 *)argp);
        if (id >= tvce->cores)
            return -EFAULT;

        id = tvce->core[id].hw_id;
        __put_user(id, (u32 *)argp);
        break;
    case CB_TRANX_VCE_GET_CFG:
        ret = copy_from_user(&cfg, argp, sizeof(cfg));
        if (ret) {
            trans_dbg(tdev, TR_ERR,
                      "vc8000e: get_cfg copy_from_user failed.\n");
            return -EFAULT;
        }

        if (cfg.core_id >= tvce->cores) {
            trans_dbg(tdev, TR_ERR, "vc8000e: get_cfg core_id:%d error.\n",
                      cfg.core_id);
            return -EFAULT;
        }

        ret = copy_to_user(argp, (void *)&tvce->vce_cfg[cfg.core_id],
                           sizeof(struct vce_core_config));
        if (ret) {
            trans_dbg(tdev, TR_ERR, "vc8000e: get_cfg copy_to_user failed.\n");
            return -EFAULT;
        }
        break;
    case CB_TRANX_VCE_RESERVE:
        if (copy_from_user(&info, argp, sizeof(struct core_info))) {
            trans_dbg(tdev, TR_ERR,
                      "vc8000e: reserve copy_from_user failed.\n");
            return -EFAULT;
        }
        return vce_reserve_core(tdev, info, argp, filp);
    case CB_TRANX_VCE_RELEASE:
        __get_user(val, (u32 *)argp);
        trans_dbg(tdev, TR_DBG, "vc8000e: release vce core info:%d.\n", val);
        return vce_release_core(tdev, val, NORM_EXIT, filp);
    case CB_TRANX_VCE_WAIT_DONE:
        __get_user(val, (u32 *)argp);
        id = (val & 0x3) - 1;
        if (id >= tvce->cores) {
            trans_dbg(tdev, TR_ERR, "vc8000e: core_wait core:%d error.\n", id);
            return -EFAULT;
        }

        ret = vce_wait_ready(tvce, &val, &irq_status);
        if (ret == 0) {
            __put_user(irq_status, (u32 *)argp);
            ret = val;
        } else {
            __put_user(0, (u32 *)argp);
            ret = -EFAULT;
        }
        break;
    default:
        trans_dbg(tdev, TR_ERR, "vc8000e: %s, cmd:0x%x is error.\n", __func__,
                  cmd);
        ret = -EINVAL;
    }

    return ret;
}
