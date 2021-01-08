// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 VeriSilicon Holdings Co., Ltd.
 *
 * This is misc ip management driver for Linux.
 * There are eight types of small ip, we call them misc ip.they are: L2CACH_VCD,
 * L2CACH_VCE, DEC400_VCD, DEC400_VCE, TCACHE, DTRC, DEC, L2CACHE_DTRC_TCACH
 * and PLL_CONTROLLER. This file provide register operation and initialization,
 * like read/write a register or pull/push a batch of registers, because the
 * codec IPs(vc8000d, vc8000e, bigsea) have the similar interfaces, they use
 * unified register operation APIs.
 */

#include <linux/pci.h>
#include <linux/pagemap.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/vmalloc.h>

#include "common.h"
#include "misc_ip.h"
#include "edma.h"
#include "encoder.h"
#include "transcoder.h"
#include "vc8000d.h"

#define CHECK_ADDR
#define CHECK_CORE_STATUS

#define TC_CONFIG_OFF		0x1300
#define TC_WR_CONFIG0_OFF	0x1301
#define TC_WR_START_ADDR_Y_OFF	0x1302
#define TC_WR_END_ADDR_Y_OFF	0x1303
#define TC_WR_STRIDE_Y_OFF	0x1304
#define TC_WR_START_ADDR_U_OFF	0x1305
#define TC_WR_END_ADDR_U_OFF	0x1306
#define TC_WR_STRIDE_U_OFF	0x1307

#define TC_CONFIG_COMMIT_OFF	0x131d
#define TC_INTERRUPT_VCE_OFF	0x131e
#define TC_SW_RESET_OFF		0x131f

#define TCACHE_MEM_OFF(x)	(0x80000000*(x))
#define TCACHE_MAP_SIZE		0x500000

/* dtrc offset address base on BAR2 */
#define DTRC_OFF		0x3210000
/*
 * DTRC_BASE(x) is dtrc offset address.
 * there are two dtrc,0x400000 is their address offset, so x is 0 or 1.
 */
#define DTRC_BASE(x)		(DTRC_OFF+(x)*0x400000)
#define	DTRC_SIZE		0x1000
#define FDINTR			0xc4
#define DTCTRL			0xc8
#define BLK_AXI_LPI_CON_STUS		(LOW_POWER_CTRL_BASE+0x64)
#define THS_TCACH_RST_CON_STUS(n)	(SYS_RST_CTRL_BASE+0x30+0x10*(n))

#define VC8000D_CORES		4
#define VC8000E_CORES		2
#define BIGSEA_CORES		2
#define L2CACH_VCD_CORES	4
#define L2CACH_VCE_CORES	2
#define F1_CORES		4
#define F2_CORES		2
#define F3_CORES		2
#define F4_TCACH_CORES		2
#define F4_DTRC_CORES		2
#define F4_L2CACHE_CORES	2
#define MISC_IP_TOTAL_CORES	(\
	VC8000D_CORES + VC8000E_CORES + BIGSEA_CORES +\
	L2CACH_VCD_CORES + L2CACH_VCE_CORES + F1_CORES +\
	F2_CORES + F3_CORES + F4_TCACH_CORES +\
	F4_DTRC_CORES + F4_L2CACHE_CORES\
	)

#define VC8000D_IOSIZE		0x10000
#define VC8000E_IOSIZE		0x10000
#define BIGSEA_IOSIZE		0x10000
#define L2CACH_VCD_IOSIZE	0x10000
#define L2CACH_VCE_IOSIZE	0x10000
#define F1_IOSIZE		0x10000
#define F2_IOSIZE		0x10000
#define F3_IOSIZE		0x10000
#define F4_TCACH_IOSIZE		0x10000
#define F4_DTRC_IOSIZE		0x10000
#define F4_L2CACHE_IOSIZE	0x10000
#define MISC_IP_TOTAL_IOSIZE	(\
	VC8000D_CORES*VC8000D_IOSIZE +\
	VC8000E_CORES*VC8000E_IOSIZE +\
	BIGSEA_CORES*BIGSEA_IOSIZE +\
	L2CACH_VCD_CORES*L2CACH_VCD_IOSIZE +\
	L2CACH_VCE_CORES*L2CACH_VCE_IOSIZE +\
	F1_CORES*F1_IOSIZE +\
	F2_CORES*F2_IOSIZE +\
	F3_CORES*F3_IOSIZE +\
	F4_TCACH_CORES*F4_TCACH_IOSIZE +\
	F4_DTRC_CORES*F4_DTRC_IOSIZE +\
	F4_L2CACHE_CORES*F4_L2CACHE_IOSIZE\
)

#define S0_VC8000D_A_OFF		0x3100000
#define S0_VC8000D_B_OFF		0x3110000
#define S0_VC8000E_OFF			0x3120000
#define S0_BIGSEA_OFF			0x3130000
#define S0_L2CACH_VCD_A_OFF		0x3160000	//L2CACH_VCD
#define S0_DEC400_VCD_A_OFF		0x3170000	//F1
#define S0_L2CACH_VCD_B_OFF		0x3180000
#define S0_DEC400_VCD_B_OFF		0x3190000
#define S0_DEC400_VCE_A_OFF		0x31B0000	//F2
#define S0_L2CACHR_VCE_A_OFF		0x31C0000	//L2CACH_VCE
#define S0_DEC400_VCE_B_OFF		0x31E0000
#define S0_L2CACHR_VCE_B_OFF		0x31F0000
#define S0_TCACH_OFF			0x3200000	//F4_TCACH
#define S0_DTRC_TCACH_OFF		0x3210000	//F4_DTRC
#define S0_L2CACHE_DTRC_TCACH_OFF	0x3220000	//F4_L2CACHE
#define S0_DEC_PCIE_OFF			0x3230000	//F3

#define S1_VC8000D_A_OFF		0x3500000
#define S1_VC8000D_B_OFF		0x3510000
#define S1_VC8000E_OFF			0x3520000
#define S1_BIGSEA_OFF			0x3530000
#define S1_L2CACH_VCD_A_OFF		0x3560000
#define S1_DEC400_VCD_A_OFF		0x3570000
#define S1_L2CACH_VCD_B_OFF		0x3580000
#define S1_DEC400_VCD_B_OFF		0x3590000
#define S1_DEC400_VCE_A_OFF		0x35B0000
#define S1_L2CACHR_VCE_A_OFF		0x35C0000
#define S1_DEC400_VCE_B_OFF		0x35E0000
#define S1_L2CACHR_VCE_B_OFF		0x35F0000
#define S1_TCACH_OFF			0x3600000
#define S1_DTRC_TCACH_OFF		0x3610000
#define S1_L2CACHE_DTRC_TCACH_OFF	0x3620000
#define S1_DEC_PCIE_OFF			0x3630000

#define PLL_CON_OFF			0x200

#define BIGSEA_HW_STA			2


static const
u32 vc8000d[VC8000D_CORES][2] = {
	{S0_VC8000D_A_OFF, VC8000D_IOSIZE},
	{S0_VC8000D_B_OFF, VC8000D_IOSIZE},
	{S1_VC8000D_A_OFF, VC8000D_IOSIZE},
	{S1_VC8000D_B_OFF, VC8000D_IOSIZE},
};
static const
u32 vc8000e[VC8000E_CORES][2] = {
	{S0_VC8000E_OFF, VC8000E_IOSIZE},
	{S1_VC8000E_OFF, VC8000E_IOSIZE},
};
static const
u32 bigsea[BIGSEA_CORES][2] = {
	{S0_BIGSEA_OFF, BIGSEA_IOSIZE},
	{S1_BIGSEA_OFF, BIGSEA_IOSIZE},
};
static const
u32 l2cach_vcd[L2CACH_VCD_CORES][2] = {
	{S0_L2CACH_VCD_A_OFF, L2CACH_VCD_IOSIZE},
	{S0_L2CACH_VCD_B_OFF, L2CACH_VCD_IOSIZE},
	{S1_L2CACH_VCD_A_OFF, L2CACH_VCD_IOSIZE},
	{S1_L2CACH_VCD_B_OFF, L2CACH_VCD_IOSIZE},
};
static const
u32 l2cach_vce[L2CACH_VCE_CORES][2] = {
	{S0_L2CACHR_VCE_A_OFF, L2CACH_VCE_IOSIZE},
	{S1_L2CACHR_VCE_A_OFF, L2CACH_VCE_IOSIZE},
};
static const
u32 f1[F1_CORES][2] = {
	{S0_DEC400_VCD_A_OFF, F1_IOSIZE},
	{S0_DEC400_VCD_B_OFF, F1_IOSIZE},
	{S1_DEC400_VCD_A_OFF, F1_IOSIZE},
	{S1_DEC400_VCD_B_OFF, F1_IOSIZE},
};
static const
u32 f2[F2_CORES][2] = {
	{S0_DEC400_VCE_A_OFF, F2_IOSIZE},
	{S1_DEC400_VCE_A_OFF, F2_IOSIZE},
};
static const
u32 f3[F3_CORES][2] = {
	{S0_DEC_PCIE_OFF, F3_IOSIZE},
	{S1_DEC_PCIE_OFF, F3_IOSIZE},
};
static const
u32 f4_tcach[F4_TCACH_CORES][2] = {
	{S0_TCACH_OFF, F4_TCACH_IOSIZE},
	{S1_TCACH_OFF, F4_TCACH_IOSIZE},
};
static const
u32 f4_dtrc[F4_DTRC_CORES][2] = {
	{S0_DTRC_TCACH_OFF, F4_DTRC_IOSIZE},
	{S1_DTRC_TCACH_OFF, F4_DTRC_IOSIZE},
};
static const
u32 f4_l2cache[F4_L2CACHE_CORES][2] = {
	{S0_L2CACHE_DTRC_TCACH_OFF, F4_L2CACHE_IOSIZE},
	{S1_L2CACHE_DTRC_TCACH_OFF, F4_L2CACHE_IOSIZE},
};

struct ip_info {
	u32 base_off;
	void __iomem *hwregs;
	u32 iosize;
	u32 *shadow;
};

struct misc_ip {
	struct ip_info vc8000d[VC8000D_CORES];
	struct ip_info vc8000e[VC8000E_CORES];
	struct ip_info bigsea[BIGSEA_CORES];
	struct ip_info l2cach_vcd[L2CACH_VCD_CORES];
	struct ip_info l2cach_vce[L2CACH_VCE_CORES];
	struct ip_info f1[F1_CORES];
	struct ip_info f2[F2_CORES];
	struct ip_info f3[F3_CORES];
	struct ip_info f4_tcach[F4_TCACH_CORES];
	struct ip_info f4_dtrc[F4_DTRC_CORES];
	struct ip_info f4_l2cache[F4_L2CACHE_CORES];
	struct cb_tranx_t *tdev;
};

static inline void check_bar2_addr(struct cb_tranx_t *tdev,
					void __iomem *addr,
					int ip_id)
{
	if ((addr < tdev->bar2_virt) || (addr > tdev->bar2_virt_end))
		trans_dbg(tdev, TR_ERR,
			"%s ip_id=%d bar2_start=0x%p bar2_end=0x%p addr=0x%p\n",
			__func__, ip_id, tdev->bar2_virt,
			tdev->bar2_virt_end, addr, ip_id);
}

int tcache_subsys_reset(struct cb_tranx_t *tdev, int slice)
{
	unsigned int val;
	unsigned int round = RESET_ROUND;
	unsigned int mask;
	unsigned int reg;
	int i;

	mutex_lock(&tdev->reset_lock);
	mask = slice ? 0x80000000 : 0x40000000;
	while (round--) {
		val = ccm_read(tdev, 0x20464);
		if (val & mask)
			break;
		usleep_range(100, 200);
	}
	if (round <= 0) {
		mutex_unlock(&tdev->reset_lock);
		trans_dbg(tdev, TR_ERR, "tcache: %s slice:%d reset failed.\n",
			  __func__, slice);
		return -EFAULT;
	}
	reg = slice ? 0x140 : 0x130;
	val = ccm_read(tdev, reg);
	val &= (~0x01);
	ccm_write(tdev, reg, val);
	usleep_range(100, 200);
	ccm_write(tdev, reg, val | 0x01);
	mutex_unlock(&tdev->reset_lock);

	/* clear interrupt status */
	for (i = 0; i < 10; i++)
		ccm_write(tdev, 0x10000+0xc4+i*8+slice*0x70, 0xFFFFFFFF);

	trans_dbg(tdev, TR_NOTICE,
		"tcache: %s slice:%d reset successfully.\n", __func__, slice);

	return 0;
}


static void tcache_config(struct cb_tranx_t *tdev, u32 id)
{
	tcache_write(tdev, id, TC_CONFIG_OFF, 0x0);
	tcache_write(tdev, id, TC_WR_CONFIG0_OFF, 0x5);
	tcache_write(tdev, id, TC_WR_START_ADDR_Y_OFF, 0x0);
	tcache_write(tdev, id, TC_WR_END_ADDR_Y_OFF, 0x80000);
	tcache_write(tdev, id, TC_WR_STRIDE_Y_OFF, 0x2000);

	tcache_write(tdev, id, TC_WR_START_ADDR_U_OFF, 0x1000000);
	tcache_write(tdev, id, TC_WR_END_ADDR_U_OFF, 0x1040000);
	tcache_write(tdev, id, TC_WR_STRIDE_U_OFF, 0x2000);
	tcache_write(tdev, id, TC_CONFIG_COMMIT_OFF, 0x1);
}

/* tcache internal reset */
static int tcache_swreset(struct cb_tranx_t *tdev, u32 id)
{
	int ret = -EFAULT;
	unsigned long delay;

	tcache_write(tdev, id, TC_SW_RESET_OFF, 0x1);
	delay = 1000;
	while (delay--) {
		if (tcache_read(tdev, id, TC_INTERRUPT_VCE_OFF) & (1<<31)) {
			ret = 0;
			break;
		}
		usleep_range(10, 15);
	}
	trans_dbg(tdev, TR_DBG, "tcache: %s slice:%d %s\n",
		  __func__, id, ret ? "failed" : "succeeded");

	return ret;
}

/* write data to tcache, and tcache only support edma access */
static int tcache_buf_init(struct cb_tranx_t *tdev,
				dma_addr_t src_add, dma_addr_t dst_add,
				u32 size, u32 id)
{
	struct trans_pcie_edma edma_info;
	int ret = 0;

	dst_add += TCACHE_MEM_OFF(id);
	edma_info.size = size;
	edma_info.dar_high = 0;
	edma_info.dar_low = QWORD_LO(dst_add);
	edma_info.sar_high = QWORD_HI(src_add);
	edma_info.sar_low = QWORD_LO(src_add);
	edma_info.direct = RC2EP;
	edma_info.interrupt_enable = INTE_EN;

	ret = edma_normal_rc2ep_xfer(&edma_info, tdev);
	if (ret)
		trans_dbg(tdev, TR_ERR,
			"tcache: %s failed,size=0x%x slice=%d dst_addr:0x%x.\n",
			__func__, size, id, dst_add);
	return ret;
}

/*
 * tcache is a specific memory space in chip, it support ecc verify,
 * so before using, driver will initialize all space on two tcache.
 */
int tcache_init(struct cb_tranx_t *tdev)
{
	void *kvir_dma;
	dma_addr_t bus;
	/* only edma can access tcache memory, and max size is 0x80000,
	 * so tcache need a dma buffer to initialize it.
	 */
	int dma_size = 0x80000;
	int ret = -EFAULT;
	int i;

	kvir_dma = dma_alloc_coherent(&tdev->pdev->dev,
				dma_size, &bus, GFP_KERNEL);
	if (!kvir_dma) {
		trans_dbg(tdev, TR_ERR,
			"tcache: dma_alloc_coherent failed.\n");
		goto out;
	}

	/* there are two tcache in a chip, we need initialize it one by one */
	for (i = 0; i < 2; i++) {
		tcache_config(tdev, i);
		if (tcache_buf_init(tdev, bus, 0x0000000, 0x80000, i))
			goto out_free_dmabuf;
		if (tcache_buf_init(tdev, bus, 0x1000000, 0x40000, i))
			goto out_free_dmabuf;
		if (tcache_buf_init(tdev, bus, 0x0000000, 0x80000, i))
			goto out_free_dmabuf;
		if (tcache_buf_init(tdev, bus, 0x1000000, 0x40000, i))
			goto out_free_dmabuf;
		if (tcache_buf_init(tdev, bus, 0x0000000, 0x80000, i))
			goto out_free_dmabuf;
		if (tcache_buf_init(tdev, bus, 0x1000000, 0x40000, i))
			goto out_free_dmabuf;
		if (tcache_buf_init(tdev, bus, 0x0000000, 0x80000, i))
			goto out_free_dmabuf;
		if (tcache_buf_init(tdev, bus, 0x1000000, 0x40000, i))
			goto out_free_dmabuf;
	}
	if (tcache_swreset(tdev, 0))
		goto out_free_dmabuf;
	if (tcache_swreset(tdev, 1))
		goto out_free_dmabuf;
	ret = 0;

out_free_dmabuf:
	dma_free_coherent(&tdev->pdev->dev, dma_size, kvir_dma, bus);
out:
	return ret;
}

static void check_core_status(struct cb_tranx_t *tdev, int ip_id,
					int core_id, char *fun_name, int reg_id)
{
	struct vc8000d_t *tvcd = tdev->modules[TR_MODULE_VC8000D];
	struct vc8000e_t *tvce = tdev->modules[TR_MODULE_VC8000E];
	struct bigsea_t *tbigsea = tdev->modules[TR_MODULE_BIGSEA];
	struct video_core_info *video_core;

#ifndef CHECK_CORE_STATUS
	return;
#endif
	if (ip_id == VC8000D_ID) {
	    video_core = &tvcd->core[core_id];
	    if (video_core->core_status == IDLE_FLAG) {
		    trans_dbg(tdev, TR_ERR, "misc_ip: %s-%s vc8000d core_id=%d reg_id:%d core_status=%d\n",
			__func__, fun_name, core_id, reg_id, video_core->core_status);
	    }
	}
	if (ip_id == VC8000E_ID) {
	    video_core = &tvce->core[core_id];
	    if (video_core->core_status == IDLE_FLAG) {
		    trans_dbg(tdev, TR_ERR, "misc_ip: %s-%s vc8000e core_id=%d reg_id:%d core_status=%d\n",
			__func__, fun_name, core_id, reg_id, video_core->core_status);
	    }
	}
	if (ip_id == BIGSEA_ID) {
	    video_core = &tbigsea->core[core_id];
	    if (video_core->core_status == IDLE_FLAG) {
		    trans_dbg(tdev, TR_ERR, "misc_ip: %s-%s bigsea core_id=%d reg_id:%d core_status=%d\n",
			__func__, fun_name, core_id, reg_id, video_core->core_status);
	    }
	}
}

/* write one register. */
static int write_one_reg(struct ip_info *ip, struct core_desc core,
			     struct cb_tranx_t *tdev, int ip_id)
{
	int ret, id;
	u32 val;

	if (tdev->hw_err_flag)
		return tdev->hw_err_flag;

	check_core_status(tdev, ip_id, core.id, "write_one_reg", core.reg_id);
	id = core.reg_id;
	ret = copy_from_user(&val, core.regs, 4);
	if (ret)
		trans_dbg(tdev, TR_ERR, "misc_ip: %s failed.\n", __func__);
	else {
		check_bar2_addr(tdev, ip->hwregs + id * 4, ip_id);
		writel(val, ip->hwregs + id * 4);
	}

	return ret ? -EFAULT : 0;
}

/* read one register. */
static int read_one_reg(struct ip_info *ip, struct core_desc core,
				struct cb_tranx_t *tdev, int ip_id)
{
	int ret, id;
	u32 val;

	if (tdev->hw_err_flag)
		return tdev->hw_err_flag;

	check_core_status(tdev, ip_id, core.id, "read_one_reg", core.reg_id);
	id = core.reg_id;
	check_bar2_addr(tdev, ip->hwregs + id * 4, ip_id);
	val = readl(ip->hwregs + id * 4);

	if ((ip_id == BIGSEA_ID) && (id == BIGSEA_HW_STA))
		check_bigsea_hwerr(tdev, val, core.id);

	/* put registers to user space */
	ret = copy_to_user(core.regs, &val, 4);
	if (ret)
		trans_dbg(tdev, TR_ERR, "misc_ip: %s failed.\n", __func__);

	return ret ? -EFAULT : 0;
}

/*
 * get value from userspace, then batch write registers.
 * start id is core->reg_id, size is core->size.
 */
static int regs_batch_write(struct ip_info *ip, struct core_desc core,
				 struct cb_tranx_t *tdev, int ip_id)
{
	int ret, i, regs_cnt;
	struct reg_desc *regs_info = (struct reg_desc *)ip->shadow;

	if (tdev->hw_err_flag)
		return tdev->hw_err_flag;

	check_core_status(tdev, ip_id, core.id, "regs_batch_write", core.reg_id);
	regs_cnt = core.size / sizeof(struct reg_desc);
	ret = copy_from_user(regs_info, core.regs, core.size);
	if (ret)
		trans_dbg(tdev, TR_ERR, "misc_ip: %s failed.\n", __func__);
	else {
		for (i = 0; i < regs_cnt; i++) {
			check_bar2_addr(tdev, ip->hwregs + regs_info[i].id * 4, ip_id);
			writel(regs_info[i].val, ip->hwregs+regs_info[i].id*4);
		}
	}

	return ret ? -EFAULT : 0;
}

/*
 * batch read registers, then copy them to userspace.
 * start id is core->reg_id, size is core->size.
 */
static int regs_batch_read(struct ip_info *ip, struct core_desc core,
				struct cb_tranx_t *tdev, int ip_id)
{
	int ret, i, regs_cnt;
	struct reg_desc *regs_info = (struct reg_desc *)ip->shadow;

	if (tdev->hw_err_flag)
		return tdev->hw_err_flag;

	check_core_status(tdev, ip_id, core.id, "regs_batch_read", core.reg_id);
	ret = copy_from_user(regs_info, core.regs, core.size);
	if (ret) {
		trans_dbg(tdev, TR_ERR, "misc_ip: %s failed.\n", __func__);
		return -EFAULT;
	}
	regs_cnt = core.size / sizeof(struct reg_desc);
	for (i = 0; i < regs_cnt; i++) {
		check_bar2_addr(tdev, ip->hwregs + regs_info[i].id * 4, ip_id);
		regs_info[i].val = readl(ip->hwregs + regs_info[i].id * 4);
		if ((ip_id == BIGSEA_ID) && (regs_info[i].id == BIGSEA_HW_STA))
			check_bigsea_hwerr(tdev, regs_info[BIGSEA_HW_STA].val, core.id);
	}
	ret = copy_to_user(core.regs, regs_info, core.size);
	if (ret)
		trans_dbg(tdev, TR_ERR, "misc_ip: %s failed.\n", __func__);

	return ret ? -EFAULT : 0;
}

static struct ip_info *get_spcecific_ip(struct misc_ip *ip_dev,
					     struct ip_desc ipdesc)
{
	struct ip_info *ipinfo = NULL;
	u32 core_id = ipdesc.core.id;

	switch (ipdesc.ip_id) {
	case VC8000D_ID:
		if (core_id > VC8000D_CORES)
			trans_dbg(ip_dev->tdev, TR_ERR,
				"misc_ip: %s ip_id:%d, core id:%d error.\n",
				__func__, ipdesc.ip_id, core_id);
		else
			ipinfo = &ip_dev->vc8000d[core_id];
		break;
	case VC8000E_ID:
		if (core_id > VC8000E_CORES)
			trans_dbg(ip_dev->tdev, TR_ERR,
				"misc_ip: %s ip_id:%d, core id:%d error.\n",
				__func__, ipdesc.ip_id, core_id);
		else
			ipinfo = &ip_dev->vc8000e[core_id];
		break;
	case BIGSEA_ID:
		if (core_id > BIGSEA_CORES)
			trans_dbg(ip_dev->tdev, TR_ERR,
				"misc_ip: %s ip_id:%d, core id:%d error.\n",
				__func__, ipdesc.ip_id, core_id);
		else
			ipinfo = &ip_dev->bigsea[core_id];
		break;
	case L2CACH_VCD_ID:
		if (core_id > L2CACH_VCD_CORES)
			trans_dbg(ip_dev->tdev, TR_ERR,
				"misc_ip: %s ip_id:%d, core id:%d error.\n",
				__func__, ipdesc.ip_id, core_id);
		else
			ipinfo = &ip_dev->l2cach_vcd[core_id];
		break;
	case L2CACH_VCE_ID:
		if (core_id > L2CACH_VCE_CORES)
			trans_dbg(ip_dev->tdev, TR_ERR,
				"misc_ip: %s ip_id:%d, core id:%d error.\n",
				__func__, ipdesc.ip_id, core_id);
		else
			ipinfo = &ip_dev->l2cach_vce[core_id];
		break;
	case F1_ID:
		if (core_id > F1_CORES)
			trans_dbg(ip_dev->tdev, TR_ERR,
				"misc_ip: %s ip_id:%d, core id:%d error.\n",
				__func__, ipdesc.ip_id, core_id);
		else
			ipinfo = &ip_dev->f1[core_id];
		break;
	case F2_ID:
		if (core_id > F2_CORES)
			trans_dbg(ip_dev->tdev, TR_ERR,
				"misc_ip: %s ip_id:%d, core id:%d error.\n",
				__func__, ipdesc.ip_id, core_id);
		else
			ipinfo = &ip_dev->f2[core_id];
		break;
	case F3_ID:
		if (core_id > F3_CORES)
			trans_dbg(ip_dev->tdev, TR_ERR,
				"misc_ip: %s ip_id:%d, core id:%d error.\n",
				__func__, ipdesc.ip_id, core_id);
		else
			ipinfo = &ip_dev->f3[core_id];
		break;
	case F4_TCACH_ID:
		if (core_id > F4_TCACH_CORES)
			trans_dbg(ip_dev->tdev, TR_ERR,
				"misc_ip: %s ip_id:%d, core id:%d error.\n",
				__func__, ipdesc.ip_id, core_id);
		else
			ipinfo = &ip_dev->f4_tcach[core_id];
		break;
	case F4_DTRC_ID:
		if (core_id > F4_DTRC_CORES)
			trans_dbg(ip_dev->tdev, TR_ERR,
				"misc_ip: %s ip_id:%d, core id:%d error.\n",
				__func__, ipdesc.ip_id, core_id);
		else
			ipinfo = &ip_dev->f4_dtrc[core_id];
		break;
	case F4_L2CACHE_ID:
		if (core_id > F4_L2CACHE_CORES)
			trans_dbg(ip_dev->tdev, TR_ERR,
				"misc_ip: %s ip_id:%d, core id:%d error.\n",
				__func__, ipdesc.ip_id, core_id);
		else
			ipinfo = &ip_dev->f4_l2cache[core_id];
		break;
	default:
		trans_dbg(ip_dev->tdev, TR_ERR, "misc_ip: %s, ip_id:%d error\n",
			__func__, ipdesc.ip_id);
	}

	return ipinfo;
}

int misc_ip_init(struct cb_tranx_t *tdev)
{
	int i;
	struct misc_ip *tmisc;
	void *tmp_mem;

	tmisc = kzalloc(sizeof(struct misc_ip), GFP_KERNEL);
	if (!tmisc) {
		trans_dbg(tdev, TR_ERR,
			"misc_ip: %s kmalloc bigseacodec failed\n", __func__);
		goto out;
	}
	tdev->modules[TR_MODULE_MISC_IP] = tmisc;
	tmisc->tdev = tdev;

	tmp_mem = vzalloc(MISC_IP_TOTAL_IOSIZE);
	if (!tmp_mem) {
		trans_dbg(tdev, TR_ERR, "misc_ip: malloc regs mem failed.\n");
		goto out_free_dev;
	}

	for (i = 0; i < VC8000D_CORES; i++) {
		tmisc->vc8000d[i].base_off = vc8000d[i][0];
		tmisc->vc8000d[i].hwregs = tdev->bar2_virt + vc8000d[i][0];
		tmisc->vc8000d[i].iosize = vc8000d[i][1];
		tmisc->vc8000d[i].shadow = tmp_mem;
		tmp_mem += tmisc->vc8000d[i].iosize;
	}

	for (i = 0; i < VC8000E_CORES; i++) {
		tmisc->vc8000e[i].base_off = vc8000e[i][0];
		tmisc->vc8000e[i].hwregs = tdev->bar2_virt + vc8000e[i][0];
		tmisc->vc8000e[i].iosize = vc8000e[i][1];
		tmisc->vc8000e[i].shadow = tmp_mem;
		tmp_mem += tmisc->vc8000e[i].iosize;
	}

	for (i = 0; i < BIGSEA_CORES; i++) {
		tmisc->bigsea[i].base_off = bigsea[i][0];
		tmisc->bigsea[i].hwregs = tdev->bar2_virt + bigsea[i][0];
		tmisc->bigsea[i].iosize = bigsea[i][1];
		tmisc->bigsea[i].shadow = tmp_mem;
		tmp_mem += tmisc->bigsea[i].iosize;
	}

	for (i = 0; i < L2CACH_VCD_CORES; i++) {
		tmisc->l2cach_vcd[i].base_off = l2cach_vcd[i][0];
		tmisc->l2cach_vcd[i].hwregs = tdev->bar2_virt + l2cach_vcd[i][0];
		tmisc->l2cach_vcd[i].iosize = l2cach_vcd[i][1];
		tmisc->l2cach_vcd[i].shadow = tmp_mem;
		tmp_mem += tmisc->l2cach_vcd[i].iosize;
	}

	for (i = 0; i < L2CACH_VCE_CORES; i++) {
		tmisc->l2cach_vce[i].base_off = l2cach_vce[i][0];
		tmisc->l2cach_vce[i].hwregs = tdev->bar2_virt + l2cach_vce[i][0];
		tmisc->l2cach_vce[i].iosize = l2cach_vce[i][1];
		tmisc->l2cach_vce[i].shadow = tmp_mem;
		tmp_mem += tmisc->l2cach_vce[i].iosize;
	}

	for (i = 0; i < F1_CORES; i++) {
		tmisc->f1[i].base_off = f1[i][0];
		tmisc->f1[i].hwregs = tdev->bar2_virt + f1[i][0];
		tmisc->f1[i].iosize = f1[i][1];
		tmisc->f1[i].shadow = tmp_mem;
		tmp_mem += tmisc->f1[i].iosize;
	}

	for (i = 0; i < F2_CORES; i++) {
		tmisc->f2[i].base_off = f2[i][0];
		tmisc->f2[i].hwregs = tdev->bar2_virt + f2[i][0];
		tmisc->f2[i].iosize = f2[i][1];
		tmisc->f2[i].shadow = tmp_mem;
		tmp_mem += tmisc->f2[i].iosize;
	}

	for (i = 0; i < F3_CORES; i++) {
		tmisc->f3[i].base_off = f3[i][0];
		tmisc->f3[i].hwregs = tdev->bar2_virt + f3[i][0];
		tmisc->f3[i].iosize = f3[i][1];
		tmisc->f3[i].shadow = tmp_mem;
		tmp_mem += tmisc->f3[i].iosize;
	}

	for (i = 0; i < F4_TCACH_CORES; i++) {
		tmisc->f4_tcach[i].base_off = f4_tcach[i][0];
		tmisc->f4_tcach[i].hwregs = tdev->bar2_virt + f4_tcach[i][0];
		tmisc->f4_tcach[i].iosize = f4_tcach[i][1];
		tmisc->f4_tcach[i].shadow = tmp_mem;
		tmp_mem += tmisc->f4_tcach[i].iosize;
	}

	for (i = 0; i < F4_DTRC_CORES; i++) {
		tmisc->f4_dtrc[i].base_off = f4_dtrc[i][0];
		tmisc->f4_dtrc[i].hwregs = tdev->bar2_virt + f4_dtrc[i][0];
		tmisc->f4_dtrc[i].iosize = f4_dtrc[i][1];
		tmisc->f4_dtrc[i].shadow = tmp_mem;
		tmp_mem += tmisc->f4_dtrc[i].iosize;
	}

	for (i = 0; i < F4_L2CACHE_CORES; i++) {
		tmisc->f4_l2cache[i].base_off = f4_l2cache[i][0];
		tmisc->f4_l2cache[i].hwregs = tdev->bar2_virt + f4_l2cache[i][0];
		tmisc->f4_l2cache[i].iosize = f4_l2cache[i][1];
		tmisc->f4_l2cache[i].shadow = tmp_mem;
		tmp_mem += tmisc->f4_l2cache[i].iosize;
	}
	trans_dbg(tdev, TR_INF, "misc_ip: module initialize done.\n");
	return 0;

out_free_dev:
	kfree(tmisc);
out:
	trans_dbg(tmisc->tdev, TR_ERR, "misc_ip: module initialize failed.\n");
	return -EFAULT;
}

int misc_ip_release(struct cb_tranx_t *tdev)
{
	struct misc_ip *tmisc = tdev->modules[TR_MODULE_MISC_IP];

	vfree(tmisc->vc8000d[0].shadow);
	kfree(tmisc);
	trans_dbg(tdev, TR_DBG, "misp_ip: remove module done.\n");
	return 0;
}

long misc_ip_ioctl(struct file *filp,
			unsigned int cmd,
			unsigned long arg,
			struct cb_tranx_t *tdev)
{
	int ret = 0;
	struct ip_desc ipdesc;
	struct ip_info *ipinfo;
	void __user *argp = (void __user *)arg;
	struct misc_ip *tmisc = tdev->modules[TR_MODULE_MISC_IP];

	if (copy_from_user(&ipdesc, argp, sizeof(struct ip_desc))) {
		trans_dbg(tdev, TR_ERR,
			"misc_ip: %s copy from user failed\n", __func__);
		return -EFAULT;
	}
	ipinfo = get_spcecific_ip(tmisc, ipdesc);
	if (!ipinfo)
		return -EFAULT;

	switch (cmd) {
	case CB_TRANX_RD_REG:
		ret = read_one_reg(ipinfo, ipdesc.core, tdev, ipdesc.ip_id);
		break;
	case CB_TRANX_WR_REG:
		ret = write_one_reg(ipinfo, ipdesc.core, tdev, ipdesc.ip_id);
		break;
	case CB_TRANX_PULL_REGS:
		ret = regs_batch_read(ipinfo, ipdesc.core, tdev, ipdesc.ip_id);
		break;
	case CB_TRANX_PUSH_REGS:
		ret = regs_batch_write(ipinfo, ipdesc.core, tdev, ipdesc.ip_id);
		break;
	default:
		trans_dbg(tdev, TR_ERR,
			"misc_ip: %s, cmd:0x%x error.\n", __func__, cmd);
		ret = -EINVAL;
	}

	return ret;
}

/*
 * check pll locked and switch to high frequency.
 * the offset is base on ccm(offset: 0x0040_0000)
 * PLL0:		THS0 VCE A/BIGSEA A	0x0000_0200
 * PLL0_MIRROR:		THS1 VCE A/BIGSEA A	0x0000_0280
 * PLL1:		THS0 VCD A/B		0x0000_0210
 * PLL2:		THS1 VCD A/B		0x0000_0220
 * PLL5:		DDR0			0x0000_0250
 * PLL6:		DDR1			0x0000_0260
 * PLL7:		THS0 DEC_F2		0x0000_0270
 * PLL8:		THS1 DEC_F2		0x0000_0290
 */
int enable_all_pll(struct cb_tranx_t *tdev)
{
	u32 i, val;

	/* PLL_0,PLL_1,PLL_2,PLL_7,PLL0_mirror,PLL_8 */
	for (i = 0; i <= 9; i++) {
		/* bootrom has enabled pll_3 and pll_4,
		 * and other code will enable pll_5 and pll_6
		 */
		if ((i >= 3) && (i <= 6))
			continue;

		val = ccm_read(tdev, PLL_CON_OFF + i*0x10 + 0x4);
		if ((val&0x80000000) == 0x80000000) {
			val = ccm_read(tdev, PLL_CON_OFF + i*0x10);
			val &= 0xFFBFFFFF;	// bit22: switch
			ccm_write(tdev, PLL_CON_OFF + i*0x10, val);
		} else {
			trans_dbg(tdev, TR_ERR,
				"core: enable pll_%d failed.\n", i);
			return -EFAULT;
		}
	}

	/* PLL_5 and PLL_6 */
	for (i = 5; i <= 6; i++) {
		val = ccm_read(tdev, PLL_CON_OFF + i*0x10 + 0x8);
		if ((val&0x80000000) == 0x80000000) {
			val = ccm_read(tdev, PLL_CON_OFF + i*0x10);
			val &= 0xFFFFFFFD; // bit1: switch
			ccm_write(tdev, PLL_CON_OFF + i*0x10, val);
		} else {
			trans_dbg(tdev, TR_ERR,
				"core: enable pll_%d failed.\n", i);
			return -EFAULT;
		}
	}

	return 0;
}

/*
 * adjust video pll flow:
 *    1.enable clock to bypass mode, now clock is base 25MHz.
 *    2.clear pll reset flag, clear the bit0 to 0.
 *    3.config pll clock.
 *    4.enable pll reset flag, set the bit0 to 1.
 *    5.wait pll is locked.
 *    6.disable bypass mode, switch clock to pll.
 */
int adjust_video_pll(struct cb_tranx_t *tdev,
			  u32 pll_m, u32 pll_s, u32 pll_id)
{
	u32 val, dat;
	unsigned int round = 1000;
	int done = 0;

	if ((pll_m == 0) && (pll_s == 0)) {
		/* Enable bypass mode, switch clock to base 25MHz */
		val = ccm_read(tdev, PLL_BASE_OFF(pll_id));
		val |= TR_PLL_BYPASS_MASK;
		ccm_write(tdev, PLL_BASE_OFF(pll_id), val);
		return 0;
	}

	val = ccm_read(tdev, PLL_BASE_OFF(pll_id));
	dat = val & (TR_PLL_M_MASK|TR_PLL_BYPASS_MASK);
	val &= (TR_PLL_S_MASK|TR_PLL_BYPASS_MASK);
	/* if the pll setting is same, direct return */
	if (((dat >> TR_PLL_M_SHIFT) == pll_m)
	     && ((val >> TR_PLL_S_SHIFT) == pll_s))
		return 0;

	/* 1.Switch clock to base 25MHz, enable bypass mode */
	val = ccm_read(tdev, PLL_BASE_OFF(pll_id));
	val |= TR_PLL_BYPASS_MASK;
	ccm_write(tdev, PLL_BASE_OFF(pll_id), val);

	/* 2.Configure bit 0 with "0",clear RESETB */
	val = ccm_read(tdev, PLL_BASE_OFF(pll_id));
	val &= (~TR_PLL_RESETB_MASK);
	ccm_write(tdev, PLL_BASE_OFF(pll_id), val);

	/* 3.0 Configure bit 16 ~ bit 7 with decimal value pll_m */
	val = ccm_read(tdev, PLL_BASE_OFF(pll_id));
	val &= (~TR_PLL_M_MASK);
	val |= (pll_m<<TR_PLL_M_SHIFT);

	/* 3.1 Configure bit 19 ~ bit 17 with decimal value 2 */
	val &= (~TR_PLL_S_MASK);
	val |= (pll_s<<TR_PLL_S_SHIFT);
	ccm_write(tdev, PLL_BASE_OFF(pll_id), val);

	/* 4.Configure bit 0 with "1", enable RESETB */
	val = ccm_read(tdev, PLL_BASE_OFF(pll_id));
	val |= TR_PLL_RESETB_MASK;
	ccm_write(tdev, PLL_BASE_OFF(pll_id), val);

	/* 5,wait pll is locked, timeout is > 1ms */
	while (round--) {
		val = ccm_read(tdev, PLL_STATUS_BASE_OFF(pll_id));
		if ((val&TR_PLL_LOCK_MASK) == TR_PLL_LOCK_MASK) {
			done = 1;
			break;
		}
		usleep_range(5, 10);
	}
	if (done) {
		/* 6.Switch clock to high frequency,disable bypass mode */
		val = ccm_read(tdev, PLL_BASE_OFF(pll_id));
		val &= (~TR_PLL_BYPASS_MASK);
		ccm_write(tdev, PLL_BASE_OFF(pll_id), val);
	} else {
		val = ccm_read(tdev, PLL_BASE_OFF(pll_id));
		val |= TR_PLL_BYPASS_MASK;
		ccm_write(tdev, PLL_BASE_OFF(pll_id), val);

		trans_dbg(tdev, TR_ERR,
			"PLL: %s failed,m:%d s:%d pll:%d, enter bypass mode.\n",
			__func__, pll_m, pll_s, pll_id);
	}

	return done ? 0 : -EFAULT;
}

