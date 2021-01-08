/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020 VeriSilicon Holdings Co., Ltd.
 */

#ifndef _CB_ENCODER_H_
#define _CB_ENCODER_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#include "common.h"

#ifdef BIGSEA_OVER_CLK
#define BIGSEA_PLL_M		544 /* 680MHz */
#define BIGSEA_PLL_S		2   /* 680MHz */
#define BIGSEA_PLL		680 /* 680MHz */
#else
#define BIGSEA_PLL_M		520 /* 650MHz */
#define BIGSEA_PLL_S		2   /* 650MHz */
#define BIGSEA_PLL		650 /* 650MHz */
#endif

#define ENC_PLL_M_NORMAL	464 /* 580MHz */
#define ENC_PLL_S_NORMAL	2   /* 580MHz */
#define ENC_PLL_NORMAL		580 /* 580MHz */

#define ENC_PLL_M_75		348 /* 435MHz */
#define ENC_PLL_S_75		2   /* 435MHz */
#define ENC_PLL_75		435 /* 435MHz */

#define ENC_PLL_M_50		464 /* 290MHz */
#define ENC_PLL_S_50		3   /* 290MHz */
#define ENC_PLL_50		290 /* 290MHz */

#define ENC_PLL_M_25		464 /* 145MHz */
#define ENC_PLL_S_25		4   /* 145MHz */
#define ENC_PLL_25		145 /* 145MHz */

#define ENC_PLL_BYPASS		25  /* 25MHz */

#define TR_PLL_BIGSEA		0
#define TR_PLL_VC8000E		1

#define VCE_MAX_CORES		2
#define BIGSEA_MAX_CORES	2

/*
 * This struct record vc8000e detail information.
 * @cores: core count.
 * @core: record two cores information.
 * @vce_cfg: record vce configuation information.
 * @irq_lock: it's a spin lock, for interrupt handling function.
 * @enc_wait_queue: when receive irq, weak up wait function.
 * @tdev: record struct cb_tranx_t point.
 */
struct vc8000e_t {
	int cores;
	struct video_core_info core[VCE_MAX_CORES];
	struct vce_core_config vce_cfg[VCE_MAX_CORES];
	wait_queue_head_t enc_wait_queue;
	struct cb_tranx_t *tdev;
};

/*
 * This struct record bigsea detail information.
 * @cores: core count.
 * @core: record two cores information.
 * @irq_lock: it's a spin lock, for interrupt handling function.
 * @codec_wait_queue: when receive irq, weak up bigsea wait function.
 * @tdev: record struct cb_tranx_t point.
 */
struct bigsea_t {
	unsigned int cores;
	struct video_core_info core[BIGSEA_MAX_CORES];
	wait_queue_head_t  codec_wait_queue;
	struct cb_tranx_t *tdev;
};

/* encoder contain two IPs: vc8000e and bigsea */
/* encoder common APIs */
int encoder_init(struct cb_tranx_t *tdev);
int encoder_release(struct cb_tranx_t *tdev);
int release_encoder(struct cb_tranx_t *tdev, u32 core);
int reserve_encoder(struct cb_tranx_t *tdev, u32 *core, u32 task_priority);
int adjust_enc_pll(struct cb_tranx_t *tdev, u32 core_id, int type);
int enc_reset_core(struct cb_tranx_t *tdev, int core_id);

/* vc8000e APIs */
long vc8000e_ioctl(struct file *filp,
		       unsigned int cmd,
		       unsigned long arg,
		       struct cb_tranx_t *tdev);
int vc8000e_release(struct cb_tranx_t *tdev);
int vc8000e_init(struct cb_tranx_t *tdev);
void vce_close(struct cb_tranx_t *tdev, struct file *filp);
void vce_enable_clock(void *d, u32 core);
irqreturn_t vce_isr(int irq, void *data);

/* bigsea APIs */
irqreturn_t bigsea_isr(int irq, void *data);
int bigsea_init(struct cb_tranx_t *tdev);
int bigsea_release(struct cb_tranx_t *tdev);
long bigsea_ioctl(struct file *filp,
		      unsigned int cmd,
		      unsigned long arg,
		      struct cb_tranx_t *tdev);
void bigsea_close(struct cb_tranx_t *tdev, struct file *filp);
void bigsea_enable_clock(void *data, int id);
void check_bigsea_hwerr(struct cb_tranx_t *tdev, u32 status, int id);

#endif /* _CB_ENCODER_H_ */
