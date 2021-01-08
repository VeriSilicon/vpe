// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 VeriSilicon Holdings Co., Ltd.
 *
 * This is encoder management driver for Linux.
 * The driver provide unify reserve/release function for bigsea and vc8000e.
 * Bigsea has two slice_0 and slice_1, and vc8000e also has two slices,
 * But at the same time, bigsea and vc8000e only one can work at a slice.
 * so they use same functions to reserve/release core.
 * We can think of this way, there are two encoder cores, slice_0 and slice_1.
 * each core support bigsea and vc8000e.
 * When reserve a core, if there are a idle core, return the core id;
 * else accroding the priority, add the request to corresponding queue.
 */

#include <linux/pci.h>
#include <linux/pagemap.h>
#include <linux/time.h>
#include <linux/miscdevice.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include "common.h"
#include "encoder.h"
#include "misc_ip.h"
#include "transcoder.h"

#define S0_ENC			0
#define S1_ENC			1
#define ENC_MAX_CORES		2

/*
 * This struct record one core detail information.
 * @core_id: core index, 0 or 1.
 * @is_reserved: if this core is used(reserved), is_reserved is 1.
 */
struct enc_core_info {
	u32 core_id;
	u32 is_reserved;
	u32 core_status;
};

/*
 * This struce record encoder detail information.
 * @cores: total core count.
 * @codec: record two cores information.
 * @enc_lock: a spin lock, protect encoder reserve and release.
 * @wait_queue: when release a core,will weak up reserve function.
 * @live_list: LIVE priority task queue,LIVE task will add to this queue.
 * @vod_list: VOD priority task queue,VOD task will add to this queue.
 * @live_count: the element count in live queue,for debug.
 * @vod_count: the element count in vod queue,for debug.
 * @loading[ENC_MAX_CORES]: calculate encoder utilization.
 * @loading_lock: protect get encoder utilization.
 * @loading_timer: calculate encoder loading when get timer interrupt.
 * @enc_clk[ENC_MAX_CORES]: record current encoder clock.
 * @taskq[TR_MAX_LIST]: save element of reserved core list.
 * @tdev: record struct cb_tranx_t point.
 */
struct encoder_t {
	int cores;
	struct enc_core_info codec[ENC_MAX_CORES];
	spinlock_t enc_lock;
	wait_queue_head_t wait_queue;
	struct list_head live_list;
	struct list_head vod_list;
	u32 live_count;
	u32 vod_count;
	struct loading_info loading[ENC_MAX_CORES];
	struct timer_list loading_timer;
	u32 enc_clk[ENC_MAX_CORES];
	struct rsv_taskq *taskq[TR_MAX_LIST];
	struct cb_tranx_t *tdev;
};

extern const char *core_status[5];

/*
 * Called by release_encoder,When has a idle core, this function will
 * query LIVE queue first,if get a LIVE element, give the core to it,
 * then delete it from LIVE list. Until LIVE list is empty, will query
 * VOD queue. If all list is empty, only return.
 */
static void enc_kickoff_next_task(struct encoder_t *tenc, u32 core)
{
	int find = 0;
	struct rsv_taskq *f, *t;

	if (!list_empty(&tenc->live_list)) {
		trans_dbg(tenc->tdev, TR_DBG,
			"encoder: list_live count %d.\n", tenc->live_count);

		list_for_each_entry_safe(f, t, &tenc->live_list, rsv_list) {
			if (f->reserved == 0) {
				find = 1;
				f->reserved = 1;
				f->core_id = core;
				tenc->codec[core].is_reserved = 1;
				tenc->live_count--;
				trans_dbg(tenc->tdev, TR_DBG,
					"encoder: get valid element.\n");
				break;
			}
		}
	}

	/* live list is empty */
	if ((find == 0) && !list_empty(&tenc->vod_list)) {
		trans_dbg(tenc->tdev, TR_DBG,
			"encoder: list_vod count %d.\n", tenc->vod_count);
		list_for_each_entry_safe(f, t, &tenc->vod_list, rsv_list) {
			if (f->reserved == 0) {
				f->reserved = 1;
				f->core_id = core;
				tenc->codec[core].is_reserved = 1;
				tenc->vod_count--;
				trans_dbg(tenc->tdev, TR_DBG,
					"encoder: get valid element.\n");
				break;
			}
		}
	}
}

/*
 * Get a idle core, if there are a idle core now, return the core id;
 * else accroding the priority, add the reserve request to queue,wait
 * other application release core.
 * There are two priority: VOD and LIVE, the level of LIVE is
 * higher than VOD.So LIVE level will get a idle core first. Until LIVE
 * list is empty, VOD can get idle core.
 */
int reserve_encoder(struct cb_tranx_t *tdev, u32 *core, u32 task_priority)
{
	int i;
	int success = 0;
	struct encoder_t *tenc;
	struct rsv_taskq *new;

	tenc = tdev->modules[TR_MODULE_ENCODER];
	if (tenc->tdev->hw_err_flag)
		return tenc->tdev->hw_err_flag;

	if (task_priority > TASK_VOD) {
		trans_dbg(tdev, TR_ERR,
			  "encoder: task_priority:%d error\n", task_priority);
		return -EINVAL;
	}

	spin_lock(&tenc->enc_lock);

	for (i = 0; i < tenc->cores; i++) {
		if (!tenc->codec[i].is_reserved) {
			tenc->codec[i].is_reserved = 1;
			*core = i;
			success = 1;
			tenc->codec[i].core_status = RSV_FLAG;
			break;
		}
	}

	/* get a idle core. */
	if (success == 1) {
		spin_unlock(&tenc->enc_lock);
		trans_dbg(tdev, TR_DBG,
			  "encoder: get a idle core:%d, priority:0x%x.\n",
			  *core, task_priority);
	} else {
		/* get a free queue element for reserving */
		for (i = 0; i < TR_MAX_LIST; i++) {
			new = tenc->taskq[i];
			if (new->used == 0) {
				new->used = 1;
				new->reserved = 0;
				break;
			}
		}
		if (i == TR_MAX_LIST) {
			trans_dbg(tdev, TR_ERR,
				"encoder: can't get valid element from taskq queue\n");
			spin_unlock(&tenc->enc_lock);
			return -EFAULT;
		}

		INIT_LIST_HEAD(&new->rsv_list);

		if (task_priority == TASK_LIVE) {
			list_add_tail(&new->rsv_list, &tenc->live_list);
			tenc->live_count++;
		} else {
			list_add_tail(&new->rsv_list, &tenc->vod_list);
			tenc->vod_count++;
		}
		spin_unlock(&tenc->enc_lock);

		if (wait_event_interruptible(tenc->wait_queue, new->reserved)) {
			spin_lock(&tenc->enc_lock);
			if (new->reserved)
				tenc->codec[new->core_id].is_reserved = 0;
			new->used = 0;
			list_del(&new->rsv_list);
			spin_unlock(&tenc->enc_lock);
			trans_dbg(tdev, TR_NOTICE,
				  "encoder: reserve wait terminated.\n");
			return -ERESTARTSYS;
		}

		spin_lock(&tenc->enc_lock);
		*core = new->core_id;
		tenc->codec[*core].core_status = RSV_FLAG;
		new->used = 0;
		list_del(&new->rsv_list);
		spin_unlock(&tenc->enc_lock);

		trans_dbg(tdev, TR_DBG, "encoder: get core:%d, priority:0x%x\n",
			  *core, task_priority);
	}

	if (*core == 0)
		tenc->loading[0].tv_s = ktime_get();
	else
		tenc->loading[1].tv_s = ktime_get();

	return 0;
}

/* release a core, then weak up reserve function. */
int release_encoder(struct cb_tranx_t *tdev, u32 core)
{
	struct encoder_t *tenc;

	tenc = tdev->modules[TR_MODULE_ENCODER];
	if (core >= tenc->cores) {
		trans_dbg(tdev, TR_ERR,
			  "encoder: release core_id:%d is error.\n", core);
		return -EFAULT;
	}
	trans_dbg(tdev, TR_DBG, "encoder: %s core:%d\n", __func__, core);

	if (tenc->codec[core].is_reserved == 0) {
		trans_dbg(tdev, TR_ERR,
			  "encoder: %s core:%d already in release status.\n",
			  core);
		return -EFAULT;
	}

	if (core == 0) {
		tenc->loading[0].tv_e = ktime_get();
		tenc->loading[0].time_cnt +=
			ktime_to_us(ktime_sub(tenc->loading[0].tv_e,
				tenc->loading[0].tv_s));
	} else {
		tenc->loading[1].tv_e = ktime_get();
		tenc->loading[1].time_cnt +=
			ktime_to_us(ktime_sub(tenc->loading[1].tv_e,
				tenc->loading[1].tv_s));
	}

	spin_lock(&tenc->enc_lock);
	tenc->codec[core].is_reserved = 0;
	enc_kickoff_next_task(tenc, core);
	tenc->codec[core].core_status = IDLE_FLAG;
	spin_unlock(&tenc->enc_lock);

	wake_up_interruptible_all(&tenc->wait_queue);

	return 0;
}

/*
 * Calculate the utilization of encoder in one second.
 * It is encoder used time, from reserve a idle
 * core to release it, add up the time period in one second.
 * So the total used time divide one second is the utilization.
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))
static void enc_loading_timer_isr(unsigned long data)
{
	struct encoder_t *tenc = (struct encoder_t *)data;
#else
static void enc_loading_timer_isr(struct timer_list *t)
{
	struct encoder_t *tenc = from_timer(tenc, t, loading_timer);
#endif
	mod_timer(&tenc->loading_timer, jiffies + LOADING_TIME*HZ);

	tenc->loading[0].time_cnt_saved = tenc->loading[0].time_cnt;
	tenc->loading[1].time_cnt_saved = tenc->loading[1].time_cnt;

	tenc->loading[0].time_cnt = 0;
	tenc->loading[1].time_cnt = 0;

	tenc->loading[0].total_time = LOADING_TIME * 1000000;
	tenc->loading[1].total_time = LOADING_TIME * 1000000;
}

/* Display encoder utilization which is the average of the two core. */
static ssize_t enc_util_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct encoder_t *tenc = tdev->modules[TR_MODULE_ENCODER];
	unsigned long average_loading;

	average_loading = (tenc->loading[0].time_cnt_saved +
			tenc->loading[1].time_cnt_saved) * 100 /
			(tenc->loading[0].total_time +
			tenc->loading[1].total_time);
	if (average_loading > 100)
		average_loading = 100;

	return sprintf(buf, "%ld%%\n", average_loading);
}

static DEVICE_ATTR_RO(enc_util);

static ssize_t enc_core_status_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct encoder_t *tenc = tdev->modules[TR_MODULE_ENCODER];
	int i, pos = 0;

	for (i = 0; i < 2; i++) {
		pos += sprintf(buf+pos, "core:%d  status:%s\n",
			i, core_status[tenc->codec[i].core_status]);
	}

	return pos;
}

static DEVICE_ATTR_RO(enc_core_status);

static ssize_t enc_reserve_cnt_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct vc8000e_t *tvce = tdev->modules[TR_MODULE_VC8000E];
	struct bigsea_t *tbigsea = tdev->modules[TR_MODULE_BIGSEA];
	int i, pos = 0;

	for (i = 0; i < 2; i++) {
		pos += sprintf(buf+pos, "core:%d  count:%d\n",
			i, tvce->core[i].rsv_cnt + tbigsea->core[i].rsv_cnt);
	}

	return pos;
}

static DEVICE_ATTR_RO(enc_reserve_cnt);

/*
 * reset bigsea and vce, they use some same data path, so reset it
 * at the same time.
 */
static int enc_sft_reset(struct cb_tranx_t *tdev, int core)
{
	unsigned int val;
	unsigned int round = RESET_ROUND;
	int done = 0;

	val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_CON(core));
	ccm_write(tdev, DDR1_P_CHNL_PWR_DWN_CON(core), val|0x3c);

	/* timeout time is more than 1s */
	while (round--) {
		val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_STUS(core));
		if ((val&0x550) == 0x550) {
			done = 1;
			break;
		}
		usleep_range(100, 200);
	}

	if (!done) {
		trans_dbg(tdev, TR_ERR,
			  "encoder: %s core:%d failed.\n", __func__, core);
		return -EFAULT;
	}
	ccm_write(tdev, DDR1_P_CHNL_PWR_DWN_STUS(core), 0x550);
	ccm_write(tdev, BIGSEA_A_RST_CON_STUS(core), 0x0);
	ccm_write(tdev, VCE_A_RST_CON_STUS(core), 0x0);

	return 0;
}

/*
 * arter reset, hardware need to release, reset and release are a couple.
 * If not release, hardware can not work.
 */
static int enc_sft_release(struct cb_tranx_t *tdev, int core)
{
	unsigned int val;
	unsigned int round = RESET_ROUND;
	int done = 0;

	ccm_write(tdev, BIGSEA_A_RST_CON_STUS(core), 0x1);
	ccm_write(tdev, VCE_A_RST_CON_STUS(core), 0x1);
	val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_CON(core));
	ccm_write(tdev, DDR1_P_CHNL_PWR_DWN_CON(core), val&0xffffffc3);

	/* timeout time is more than 100ms */
	while (round--) {
		val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_STUS(core));
		if ((val&0x550) == 0x550) {
			done = 1;
			break;
		}
		usleep_range(100, 200);
	}

	if (!done) {
		trans_dbg(tdev, TR_ERR,
			  "encoder: %s core:%d failed.\n", __func__, core);
		return -EFAULT;
	}
	ccm_write(tdev, DDR1_P_CHNL_PWR_DWN_STUS(core), 0x550);

	return 0;
}

/*
 * reset encoder, before reset make sure the clock is on, so will enable
 * them again.
 */
int enc_reset_core(struct cb_tranx_t *tdev, int core_id)
{
	int ret = -EFAULT;
	unsigned int reg = core_id ? 0x10124 : 0x100B4;

	mutex_lock(&tdev->reset_lock);
	bigsea_enable_clock(tdev->modules[TR_MODULE_BIGSEA], core_id);
	vce_enable_clock(tdev->modules[TR_MODULE_VC8000E], 1 << core_id);

	if (enc_sft_reset(tdev, core_id))
		goto out;
	/*
	 * Each bit written with high will generate related ECC
	 * interrupt clear operation, and each bit will be automatically
	 * cleared when related ECC interrupt cleared.
	 */
	ccm_write(tdev, reg, 0xFFFFFFFF);
	if (enc_sft_release(tdev, core_id))
		goto out;
	trans_dbg(tdev, TR_NOTICE,
		"encoder: %s reset core %d done.\n", __func__, core_id);
	ret = 0;
out:
	mutex_unlock(&tdev->reset_lock);
	return ret;
}


/*
 * pll0, for slice_0 vce and bigsea,offset is 0x200 base on ccm
 * pll0_mirror, for slice_1 vce and bigsea,offset is 0x280 base on ccm
 *
 * @tyep: 0 is bigse, 1 is vce;
 * @slice_id: slice_0 is 0; slice_1 is 1;
 */
int adjust_enc_pll(struct cb_tranx_t *tdev, u32 slice_id, int type)
{
	int ret = 0;
	u32 pll_m, pll_s;
	u32 freq = 0;
	struct encoder_t *tenc = tdev->modules[TR_MODULE_ENCODER];

	if (type == TR_PLL_BIGSEA) {
		switch (tdev->clock_adjust) {
		case 0:
			pll_m = BIGSEA_PLL_M;
			pll_s = BIGSEA_PLL_S;
			freq = BIGSEA_PLL;
			break;
		case 1:
			pll_m = ENC_PLL_M_75;
			pll_s = ENC_PLL_S_75;
			freq = ENC_PLL_75;
			break;
		case 2:
			pll_m = ENC_PLL_M_50;
			pll_s = ENC_PLL_S_50;
			freq = ENC_PLL_50;
			break;
		case 3:
			pll_m = ENC_PLL_M_25;
			pll_s = ENC_PLL_S_25;
			freq = ENC_PLL_25;
			break;
		case 4:
			pll_m = 0;
			pll_s = 0;
			freq = ENC_PLL_BYPASS;
			break;
		default:
			trans_dbg(tdev, TR_ERR,
				"PLL: %s adjust slice:%d to %d error,bigsea\n",
				__func__, slice_id, tdev->clock_adjust);
			ret = -EFAULT;
			break;
		}
	} else if (type == TR_PLL_VC8000E) {
		switch (tdev->clock_adjust) {
		case 0:
			pll_m = ENC_PLL_M_NORMAL;
			pll_s = ENC_PLL_S_NORMAL;
			freq = ENC_PLL_NORMAL;
			break;
		case 1:
			pll_m = ENC_PLL_M_75;
			pll_s = ENC_PLL_S_75;
			freq = ENC_PLL_75;
			break;
		case 2:
			pll_m = ENC_PLL_M_50;
			pll_s = ENC_PLL_S_50;
			freq = ENC_PLL_50;
			break;
		case 3:
			pll_m = ENC_PLL_M_25;
			pll_s = ENC_PLL_S_25;
			freq = ENC_PLL_25;
			break;
		case 4:
			pll_m = 0;
			pll_s = 0;
			freq = ENC_PLL_BYPASS;
			break;
		default:
			trans_dbg(tdev, TR_ERR,
				"PLL: %s adjust slice:%d to %d error,vc8000e\n",
				__func__, slice_id, tdev->clock_adjust);
			ret = -EFAULT;
			break;
		}
	} else {
		trans_dbg(tdev, TR_ERR,
			"PLL: %s slice_id:%d, adjust:%d, type:%d error.\n",
			__func__, slice_id, tdev->clock_adjust, type);
		ret = -EFAULT;
	}
	tenc->enc_clk[slice_id] = ((type&0xffff)<<16) | (freq&0xffff);

	/* slice_0 id is 0, slice_1 id is 8 */
	if (!ret)
		ret = adjust_video_pll(tdev, pll_m, pll_s, slice_id*8);

	return ret;
}

/* Display encoder current clock in slice0. */
static ssize_t enc_s0_clk_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	int pos = 0;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct encoder_t *tenc = tdev->modules[TR_MODULE_ENCODER];

	pos += sprintf(buf + pos, "%s  %dMHz\n",
		((tenc->enc_clk[0]&0xffff0000) == 0)?"bigsea ":"vc8000e",
		tenc->enc_clk[0]&0xffff);

	return pos;
}

/* Display encoder current clock in slice1. */
static ssize_t enc_s1_clk_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	int pos = 0;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct encoder_t *tenc = tdev->modules[TR_MODULE_ENCODER];

	pos += sprintf(buf + pos, "%s  %dMHz\n",
		((tenc->enc_clk[1]&0xffff0000) == 0)?"bigsea ":"vc8000e",
		tenc->enc_clk[1]&0xffff);

	return pos;
}

static ssize_t enc_reset_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	int id, ret;

	if (count == 0)
		return 0;

	ret = sscanf(buf, "%d", &id);
	if (ret != 1) {
		trans_dbg(tdev, TR_ERR, "encoder: %s ret=%d, input_val:%d\n",
			  __func__, ret, id);
		return -1;
	}

	trans_dbg(tdev, TR_ERR, "encoder: manual reset encoder:%d\n", id);
	enc_reset_core(tdev, id);

	return count;
}
static DEVICE_ATTR_WO(enc_reset);

static ssize_t tcache_reset_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	int id, ret;

	if (count == 0)
		return 0;

	ret = sscanf(buf, "%d", &id);
	if (ret != 1) {
		trans_dbg(tdev, TR_ERR, "tcache: %s ret=%d, input_val:%d\n",
			  __func__, ret, id);
		return -1;
	}

	trans_dbg(tdev, TR_ERR, "tcache: manual reset tcache:%d\n", id);
	tcache_subsys_reset(tdev, id);

	return count;
}
static DEVICE_ATTR_WO(tcache_reset);

static DEVICE_ATTR_RO(enc_s0_clk);
static DEVICE_ATTR_RO(enc_s1_clk);

static struct attribute *trans_enc_sysfs_entries[] = {
	&dev_attr_enc_util.attr,
	&dev_attr_enc_s0_clk.attr,
	&dev_attr_enc_s1_clk.attr,
	&dev_attr_enc_core_status.attr,
	&dev_attr_enc_reserve_cnt.attr,
	&dev_attr_enc_reset.attr,
	&dev_attr_tcache_reset.attr,
	NULL
};

static struct attribute_group trans_enc_attribute_group = {
	.name = NULL,
	.attrs = trans_enc_sysfs_entries,
};

int encoder_init(struct cb_tranx_t *tdev)
{
	struct encoder_t *tenc;
	struct rsv_taskq *buf;
	int ret, i;

	tenc = kzalloc(sizeof(struct encoder_t), GFP_KERNEL);
	if (!tenc) {
		trans_dbg(tdev, TR_ERR,
			  "encoder: kmalloc encoder struct failed.\n");
		return -EFAULT;
	}
	tdev->modules[TR_MODULE_ENCODER] = tenc;
	tenc->tdev = tdev;
	tenc->enc_clk[S0_ENC] = (TR_PLL_VC8000E<<16) | ENC_PLL_NORMAL;
	tenc->enc_clk[S1_ENC] = (TR_PLL_VC8000E<<16) | ENC_PLL_NORMAL;

	buf = kzalloc(sizeof(struct rsv_taskq)*TR_MAX_LIST, GFP_KERNEL);
	if (!buf) {
		trans_dbg(tdev, TR_ERR,
			  "encoder: kmalloc encoder taskq failed.\n");
		goto free_dev;
	}
	for (i = 0; i < TR_MAX_LIST; i++)
		tenc->taskq[i] = buf + i;

	tenc->codec[S0_ENC].core_id = S0_ENC;
	tenc->codec[S1_ENC].core_id = S1_ENC;
	tenc->cores = 2;
	trans_dbg(tdev, TR_DBG, "encoder: support slice0 and slice1.\n");

	INIT_LIST_HEAD(&tenc->live_list);
	INIT_LIST_HEAD(&tenc->vod_list);

	init_waitqueue_head(&tenc->wait_queue);
	spin_lock_init(&tenc->enc_lock);

	ret = sysfs_create_group(&tdev->misc_dev->this_device->kobj,
				&trans_enc_attribute_group);
	if (ret) {
		trans_dbg(tdev, TR_ERR,
			 "encoder: failed to create sysfs device attributes\n");
		goto free_taskq;
	}

	tenc->loading_timer.expires = jiffies + LOADING_TIME*HZ;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))
	tenc->loading_timer.function = (void *)enc_loading_timer_isr;
	tenc->loading_timer.data = (unsigned long)(tenc);
	init_timer(&tenc->loading_timer);
#else
	timer_setup(&tenc->loading_timer, enc_loading_timer_isr, 0);
#endif
	add_timer(&tenc->loading_timer);

	trans_dbg(tdev, TR_INF, "encoder: module initialize done.\n");

	return 0;

free_taskq:
	kfree(buf);
free_dev:
	kfree(tenc);
	trans_dbg(tdev, TR_ERR, "encoder: module initialize filed.\n");
	return -EFAULT;
}

int encoder_release(struct cb_tranx_t *tdev)
{
	struct encoder_t *tenc = tdev->modules[TR_MODULE_ENCODER];

	del_timer_sync(&tenc->loading_timer);
	sysfs_remove_group(&tdev->misc_dev->this_device->kobj,
			&trans_enc_attribute_group);

	kfree(tenc->taskq[0]);
	kfree(tenc);

	trans_dbg(tdev, TR_DBG, "encoder: remove module done.\n");
	return 0;
}
