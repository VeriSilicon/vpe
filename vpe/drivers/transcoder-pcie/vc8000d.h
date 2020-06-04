/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Verisilicon Inc.
 */

#ifndef _CB_VC8000D_H_
#define _CB_VC8000D_H_

#include <linux/types.h>
#include <linux/ioctl.h>
#include "common.h"

#define VCD_PLL_M_NORMAL 520 /* 650MHz */
#define VCD_PLL_S_NORMAL 2 /* 650MHz */
#define VCD_PLL_M_75 390 /* 487.5MHz */
#define VCD_PLL_S_75 2 /* 487.5MHz */
#define VCD_PLL_M_50 520 /* 325MHz */
#define VCD_PLL_S_50 3 /* 325MHz */
#define VCD_PLL_M_25 520 /* 162.5MHz */
#define VCD_PLL_S_25 4 /* 162.5MHz */
#define VCD_MAX_CORES 4

/*
 * This struct record vc8000d detail information.
 * @cores: core count
 * @core[VCD_MAX_CORES]: cores information
 * @core_format[VCD_MAX_CORES]: format of core supported
 * @vcd_cfg[VCD_MAX_CORES]: vcd core config
 * @rsv_lock: protect reserve and release
 * @dec_wait_queue: when receive irq, weak up waited task
 * @hw_queue: when release a core, weak reserved task
 * @list_live: LIVE priority task queue,LIVE task will add to this queue
 * @list_vod: VOD priority task queue,VOD task will add to this queue
 * @taskq[TR_MAX_LIST]: save list element of reserved core
 * @live_count: the count of element in list_live
 * @vod_count: the count of element in list_vod
 * @loading[2]: calculate decoder utilization, only statistics s0_a and s1_a.
 * @loading_lock: protect get decoder utilization.
 * @loading_timer: calculate decoder loading when get timer interrupt.
 * @tdev: record transcoder devices description
 */
struct vc8000d_t {
    int cores;
    struct video_core_info core[VCD_MAX_CORES];
    u32 core_format[VCD_MAX_CORES];
    struct vcd_core_config vcd_cfg[VCD_MAX_CORES];
    spinlock_t rsv_lock;
    spinlock_t chk_irq_lock;
    wait_queue_head_t dec_wait_queue;
    wait_queue_head_t hw_queue;
    struct list_head list_live;
    struct list_head list_vod;
    struct rsv_taskq *taskq[TR_MAX_LIST];
    int live_count;
    int vod_count;
    struct loading_info loading[2];
    struct timer_list loading_timer;
    struct cb_tranx_t *tdev;
};

int vc8000d_init(struct cb_tranx_t *tdev);
int vc8000d_release(struct cb_tranx_t *tdev);
long vc8000d_ioctl(struct file *filp, unsigned int cmd, unsigned long arg,
                   struct cb_tranx_t *tdev);
int adjust_vcd_pll(struct cb_tranx_t *tdev, u32 slice_id);
void vcd_close(struct cb_tranx_t *tdev, struct file *filp);
int vc8000d_reset(struct cb_tranx_t *tdev);
irqreturn_t vcd_isr(int irq, void *data);

#endif /* _CB_VC8000D_H_ */
