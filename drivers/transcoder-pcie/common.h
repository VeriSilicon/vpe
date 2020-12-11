/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020 VeriSilicon Holdings Co., Ltd.
 */

#ifndef _CB_COMMON_H_
#define _CB_COMMON_H_

#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>

#include "transcoder.h"

/* submodule index */
enum TRANS_MODULE_INDEX {
	TR_MODULE_PCIE = 0,
	TR_MODULE_EDMA,
	TR_MODULE_VC8000D,
	TR_MODULE_VC8000E,
	TR_MODULE_BIGSEA,
	TR_MODULE_MEMORY,
	TR_MODULE_ENCODER,
	TR_MODULE_HW_MONITOR,
	TR_MODULE_MISC_IP,
	TR_MODULE_MAX,
};

/* interrupt number index */
#define IRQ_EDMA		0
#define IRQ_ZSP_SFT		2
#define IRQ_S0_VCD_A		7
#define IRQ_S1_VCD_A		8
#define IRQ_S0_VCD_B		9
#define IRQ_S1_VCD_B		10
#define IRQ_S0_VCE		11
#define IRQ_S1_VCE		12
#define IRQ_S0_BIGSEA		15
#define IRQ_S1_BIGSEA		16

/* global interrupt flag. */
#define EDMA			(1<<0)
#define HW_MONITOR		(1<<2)
#define THS0_VCD_A		(1<<7)
#define THS0_VCD_B		(1<<9)
#define THS1_VCD_A		(1<<8)
#define THS1_VCD_B		(1<<10)
#define THS0_VCE		(1<<11)
#define THS1_VCE		(1<<12)
#define THS0_BIGSEA		(1<<15)
#define THS1_BIGSEA		(1<<16)
#define GLUE_LGIC_INTE		(\
	HW_MONITOR|THS0_VCD_A|THS0_VCD_B|THS1_VCD_A|\
	THS1_VCD_B|THS0_VCE|THS1_VCE|THS0_BIGSEA|THS1_BIGSEA\
	)

/* 0xFFFE6078 */
#define ABORT_INTERRUPT		(~(GLUE_LGIC_INTE|EDMA))

#define CLK_ENABLE		0x1
#define DIV_1			(0x0<<4)
#define DIV_2			(0x1<<4)
#define DIV_4			(0x2<<4)
#define DIV_8			(0x3<<4)
#define NO_SOFT_RESET		0x1

#define CHIP_INT_2_STUS			0x20314
#define MAIL_BOX_INTERRUPT_CONTROLLER	0x20300

/* pcie glue logic registers */
#define INT_POL_REF_CFG			0x200ac
#define LINK_REQ_RST_NOT_SYNC		0x2006c
#define GLOBAL_IRQ_REG_OFF		0x200a8
#define GLUE_LOGIC_INT_SEL		(1<<24) /* msi */

#define QWORD_HI(v)			(((v)>>32)&0xFFFFFFFF)
#define QWORD_LO(v)			((v)&0xFFFFFFFF)
#define RESET_TIMEOUT			(1*HZ)
#define RESET_ROUND			10000

#define LOW_POWER_CTRL_BASE		0x20400
#define SYS_RST_CTRL_BASE		0x100
#define VCE_A_RST_CON_STUS(n)		(SYS_RST_CTRL_BASE+0x00+0x18*(n))
#define VCD_A_RST_CON_STUS(n)		(SYS_RST_CTRL_BASE+0x08+0x18*(n))
#define VCD_B_RST_CON_STUS(n)		(SYS_RST_CTRL_BASE+0x0c+0x18*(n))
#define BIGSEA_A_RST_CON_STUS(n)	(SYS_RST_CTRL_BASE+0x10+0x18*(n))

#define DDR1_P_CHNL_PWR_DWN_CON(n)	(LOW_POWER_CTRL_BASE+0x10+0x8*(n))
#define DDR1_P_CHNL_PWR_DWN_STUS(n)	(LOW_POWER_CTRL_BASE+0x14+0x8*(n))
#define PWR_ACTION_CON_STUS(n)		(LOW_POWER_CTRL_BASE+0x20+0x4*(n))
#define PWR_STUS_STUS(n)		(LOW_POWER_CTRL_BASE+0x28+0x4*(n))

/* hardware error flag, fatal error. */
#define HW_ERR_FLAG			0xDEADDEAD

#define PLL_BASE_OFF(off)		(0x200 + (off) * 0x10)
#define PLL_STATUS_BASE_OFF(off)	(0x204 + (off) * 0x10)

#define TR_PLL_S_SHIFT		17
#define TR_PLL_M_SHIFT		7
#define TR_PLL_P_SHIFT		1
#define TR_PLL_BYPASS_SHIFT	22
#define TR_PLL_RESETB_SHIFT	0
#define TR_PLL_LOCK_SHIFT	31

#define TR_PLL_S_MASK		(0x7 << 17)
#define TR_PLL_M_MASK		(0x3FF << 7)
#define TR_PLL_P_MASK		(0x3F << 1)

#define TR_PLL_LOCK_MASK	(0x1 << 31)
#define TR_PLL_RESETB_MASK	(0x1 << 0)
#define TR_PLL_BYPASS_MASK	(0x1 << 22)

#define CLK_CON_STUS		0x0
#define RST_CON_STUS		0x100

#define IDLE_FLAG		0x0
#define RSV_FLAG		0x1
#define EN_FLAG			0x2
#define RCVD_IRQ_FLAG		0x3
#define CHK_IRQ_FLAG		0x4

#define TCACHE_BASE		0x3200000
#define TCACHE_REGS_OFF(x)	(TCACHE_BASE+0x400000*(x))

extern const char *core_status[5];

#define ABNORM_EXIT		0x1
#define NORM_EXIT		0x0

/*
 * element count of reserved list, fbtrans max support 32 tasks,
 * one task max reserve 4 cores, add marge 32
 */
#define TR_MAX_LIST	160

struct tcache_status {
	struct semaphore sem;
	struct file *filp;
};

/* The cb_tranx_t structure describes transcoder devices */
struct cb_tranx_t {
	const char *dev_name; /* device name */
	int node_index; /* only a index for device name: /dev/transcoderN */
	struct miscdevice *misc_dev; /* misc device info */
	struct pci_dev *pdev; /* pci device info */
	void __iomem *bar0_virt; /* pci bar0 virtual address */
	void __iomem *bar2_virt; /* pci bar2 virtual address */
	void __iomem *bar2_virt_end; /* temp */
	void *modules[TR_MODULE_MAX]; /* submodule private data */
	struct tcache_status tcache[2]; /* protect tcache access for user app */
	unsigned int hw_err_flag; /* hardware error flag */
	void __iomem *ccm; /* clock controller base virtual address */
	int print_level; /* log level */
	u32 clock_adjust; /* adjust video pll frequency */
	u32 reduce_strategy; /* reduce strategy when the temperature exceeds the threshold */
	struct mutex reset_lock;
};

struct cb_misc_tdev {
	struct miscdevice misc;
	struct cb_tranx_t *tdev;
};

/* get codec utilization in one second */
#define LOADING_TIME		1

/* Used to record the usage of chip */
struct loading_info {
	unsigned long time_cnt; /* unit microsecond */
	unsigned long time_cnt_saved; /* unit microsecond */
	ktime_t tv_s;
	ktime_t tv_e;
	unsigned long total_time; /* unit microsecond */
};

/*
 * Record details for each video core
 * @hwbase: core physical address
 * @iosize: core registers space size
 * @hwregs: core virtual address, it's mapped hwbase
 * @irq: the core irq number, allocate by pcie msi
 * @hw_id: core hardware ID read from register
 * @irq_rcvd: receive interrupt flag
 * @irq_status: record irq status register value.
 * @build_id: hardware build id
 * @filp: application file point, who use this core
 */
struct video_core_info {
	unsigned long hwbase;
	unsigned int iosize;
	void __iomem *hwregs;
	int irq;
	u32 hw_id;
	u32 irq_rcvd;
	u32 irq_status;
	unsigned int build_id;
	u32 core_status;
	spinlock_t irq_lock;

	u32 rsv_cnt;
	u32 irq_cnt;
	u32 chk_irq_cnt;
	u32 idle_cnt;
	struct file *filp;
};

/*
 * Record details for reserve queue
 * @used: mark this element is used
 * @filp: application file point, who use this core
 * @format: request core format
 * @reserved: mark this list element has get a valid core
 * @core_id: core id
 * @vcd_list: traversal reserve queue by vcd_list.
 */
struct rsv_taskq {
	int used;
	struct file *filp;
	unsigned int format;
	int reserved;
	int core_id;
	struct list_head rsv_list;
};

irqreturn_t unify_isr(int irq, void *data);

/* TR_ERR: error; TR_INF:info; TR_DBG:debug */
enum TRANS_DEBUG_LEVEL {
	TR_ERR    = 0x0,
	TR_INF    = 0x1,
	TR_NOTICE = 0X2,
	TR_DBG    = 0x3,
};

void __trans_dbg(void *dev, int level, const char *fmt, ...);

#define trans_dbg(adapter, level, fmt, ...) \
	__trans_dbg(adapter, level, fmt, ##__VA_ARGS__) \


static inline void ccm_write(struct cb_tranx_t *tdev,
				unsigned int off,
				unsigned int val)
{
	writel((val), ((off)+tdev->ccm));
}

static inline unsigned int ccm_read(struct cb_tranx_t *tdev,
				       unsigned int off)
{
	return readl((off)+(tdev->ccm));
}

static inline void tcache_write(struct cb_tranx_t *tdev, u32 id,
				    u32 off, u32 val)
{
	writel(val, (tdev->bar2_virt + TCACHE_REGS_OFF(id) + off*4));
}

static inline unsigned int tcache_read(struct cb_tranx_t *tdev,
					   u32 id, u32 off)
{
	return readl(tdev->bar2_virt + TCACHE_REGS_OFF(id) + off*4);
}

#endif /* _CB_COMMON_H_ */
