/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Verisilicon Inc.
 */

#ifndef _CB_EDMA_H_
#define _CB_EDMA_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#include "common.h"

/*
 * This struct record edma performance detail information.
 * Statistics edma performance by direction separation, two atomic variables
 * real-time size, two regular ints record last second of bandwidth.
 * @rc2ep_size: rc2ep transmission size, util is byte.
 * @ep2rc_size: ep2rc transmission size, util is byte.
 * @rc2ep_per: rc2ep edma transmission performance, util is MB/s.
 * @ep2rc_per: ep2rc edma transmission performance, util is MB/s.
 */
struct edma_perf {
	atomic64_t rc2ep_size;
	atomic64_t ep2rc_size;
	unsigned int rc2ep_per;
	unsigned int ep2rc_per;
};

/*
 * recore tcache information for edma tranx to ecache.
 * @id: tcache index, 0 or 1.
 * @fcnt: current frame count.
 * @timeout_cnt: try to check tcache timeout count.
 * @total_element_cnt: total edma link element count for one frame.
 * @each_cnt: edma link element count for each tranx to tcache.
 * @current_index: current index for edma tranx to tcache.
 * @status: 00:reserve; 01:running; 10:error; 11:done
 * @tc_cfg[2][10]: tcache config value.
 * @tc_timer: hrtimer for tcache tranx.
 * @tedma: record edma detailed information.
 * @table_buffer: whole link table.
 */
struct tcache_info {
	u32 id;
	u32 fcnt;
	u32 timeout_cnt;
	u32 total_element_cnt;
	u32 cur_element_cnt;
	u32 each_cnt;
	u32 current_index;
	u32 status;
	u32 tc_cfg[2][10];
	struct hrtimer tc_timer;
	struct edma_t *tedma;
	void *table_buffer;
	u32 chk_rc2ep_err;
	u32 retry_flag;
};

struct err_chk {
	u8 dir;
	wait_queue_head_t chk_queue;
	spinlock_t chk_lock;
	unsigned int err_status;
	struct hrtimer err_timer;
	struct edma_t *tedma;
};

/*
 * The edma_t structure describes edma module.
 * @vedma_lt: edma link table virtual address.
 * @queue_wait: Waiting for edma interruption.
 * @wait_condition_r[4]: the event to wait for, using in rc2ep.
 * @wait_condition_w[4]: the event to wait for, using in ep2rc.
 * @readback_r[4]: only is a flag, ensure link table has been write to ddr.
 * @readback_w[4]: only is a flag, ensure link table has been write to ddr.
 * @rc2ep_cs[4]: record rc2ep channel status.
 * @ep2rc_cs[4]: record ep2rc channel status.
 * @ep2rc_sem: protect get/free edma channel, rc2ep direction.
 * @rc2ep_sem: protect get/free edma channel, ep2rc direction.
 * @rc2ep_cs_lock: protect get/free edma channel, rc2ep direction.
 * @ep2rc_cs_lock: protect get/free edma channel, ep2rc direction.
 * @edma_perf: record edma performance data.
 * @perf_timer: it is a timer, calculate performance periodically.
 * @rc2ep_cfg_lock: protect setting rc2ep edma registers.
 * @ep2rc_cfg_lock: protect setting ep2rc edma registers.
 * @edma_irq_lock: used in edma interrupt handle.
 * @tdev: record struct cb_tranx_t point.
 * @tc_info[2]: record tcache info.
 */
struct edma_t {
	void __iomem *vedma_lt;
	wait_queue_head_t queue_wait;
	u8 wait_condition_r[4];
	u8 wait_condition_w[4];
	u32 tcache_link_size[2];
	u32 readback_r[4];
	u32 readback_w[4];
	u8 rc2ep_cs[4];
	u8 ep2rc_cs[4];
	struct semaphore ep2rc_sem;
	struct semaphore rc2ep_sem;
	spinlock_t rc2ep_cs_lock;
	spinlock_t ep2rc_cs_lock;
	struct edma_perf edma_perf;
	struct timer_list perf_timer;
	spinlock_t rc2ep_cfg_lock;
	spinlock_t ep2rc_cfg_lock;
	struct cb_tranx_t *tdev;
	struct tcache_info tc_info[2];

	struct err_chk rc2ep_err_chk;
	struct err_chk ep2rc_err_chk;
	int err_flag;
	int tc_err_test;
};

long edma_ioctl(struct file *filp,
		   unsigned int cmd,
		   unsigned long arg,
		   struct cb_tranx_t *tdev);
int edma_init(struct cb_tranx_t *tdev);
void edma_release(struct cb_tranx_t *tdev);
int edma_normal_rc2ep_xfer(struct trans_pcie_edma *edma_info,
				  struct cb_tranx_t *tdev);
irqreturn_t edma_isr(int irq, void *data);

#endif /* _CB_EDMA_H_ */
