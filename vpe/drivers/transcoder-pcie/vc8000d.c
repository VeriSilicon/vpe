// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Verisilicon Inc.
 *
 * This is vc8000d management driver for Linux.
 * vc8000d is a video decoder, has two slices, every slice has two cores.
 * This driver provide some IOCTL commands for userspace,
 * like reserve/release a hardware core, access registers.
 * How to operate vc8000d: reserve a idle core, config registers,
 * enable the core, wait interrupt, release the core.
 *
 * reserving core supports priority, live and vod, the level of live is
 * higher than vod.
 * vc8000d has two slice(0 and 1), and every slice have two cores(a and b),
 * so vc8000d has 4 cores.
 * The abbreviation of vc8000d is vcd.
 */

#include <linux/pci.h>
#include <linux/pagemap.h>
#include <linux/miscdevice.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/delay.h>

#include "common.h"
#include "vc8000d.h"
#include "misc_ip.h"
#include "transcoder.h"
#include "edma.h"

#ifndef EMULATOR
#define VC8000D_TIMEOUT		(4*HZ) /* second */
#else
#define VC8000D_TIMEOUT		(600*HZ) /* second */
#endif

#define VCD_CLOCK_CONTROL

/* vc8000d core index */
#define S0_VCD_A		0
#define S0_VCD_B		1
#define S1_VCD_A		2
#define S1_VCD_B		3


#define VCD_SLICE0_A_OFF	0x3100000
#define VCD_SLICE0_B_OFF	0x3110000
#define VCD_SLICE1_A_OFF	0x3500000
#define VCD_SLICE1_B_OFF	0x3510000

#define VCD_WRAPPER_REG_OFF	0x8000
#define SYSTEM_REG_OFF		0x400000

/* there are five interrupt lines share a interrupt number. */
#define VCD_IRQ_CNT		5

#define S0_VCD_A_VC8000D_TOP_INT	(1<<0)
#define S0_VCD_A_SRAM_ECC_E2_INT	(1<<1)
#define S0_VCD_A_SRAM_ECC_E1_INT	(1<<2)
#define S0_VCD_A_L2RW_VCD_INT		(1<<3)
#define S0_VCD_A_DEC400_F1_INT		(1<<4)

#define S1_VCD_A_VC8000D_TOP_INT	(1<<5)
#define S1_VCD_A_SRAM_ECC_E2_INT	(1<<6)
#define S1_VCD_A_SRAM_ECC_E1_INT	(1<<7)
#define S1_VCD_A_L2RW_VCD_INT		(1<<8)
#define S1_VCD_A_DEC400_F1_INT		(1<<9)

#define S0_VCD_B_VC8000D_TOP_INT	(1<<10)
#define S0_VCD_B_SRAM_ECC_E2_INT	(1<<11)
#define S0_VCD_B_SRAM_ECC_E1_INT	(1<<12)
#define S0_VCD_B_L2RW_VCD_INT		(1<<13)
#define S0_VCD_B_DEC400_F1_INT		(1<<14)

#define S1_VCD_B_VC8000D_TOP_INT	(1<<15)
#define S1_VCD_B_SRAM_ECC_E2_INT	(1<<16)
#define S1_VCD_B_SRAM_ECC_E1_INT	(1<<17)
#define S1_VCD_B_L2RW_VCD_INT		(1<<18)
#define S1_VCD_B_DEC400_F1_INT		(1<<19)

#define WRAPPER_SFT_RSTN_CSR		0x0
#define WRAPPER_CLK_EN_CSR		0x4
#define WRAPPER_FUSE_DEC_CSR		0x8
#define WRAPPER_FUSE_PP_CSR		0xC

#define THS0_VCD_A_CLK_CON_STUS		(CLK_CON_STUS + 0xC)
#define THS0_VCD_B_CLK_CON_STUS		(CLK_CON_STUS + 0x10)
#define THS0_VCD_A_RST_CON_STUS		(RST_CON_STUS + 0x8)
#define THS0_VCD_B_RST_CON_STUS		(RST_CON_STUS + 0xc)
#define THS0_TCACH_RST_CON_STUS		(RST_CON_STUS + 0x30)

#define THS1_VCD_A_CLK_CON_STUS		(CLK_CON_STUS + 0x28)
#define THS1_VCD_B_CLK_CON_STUS		(CLK_CON_STUS + 0x2C)
#define THS1_VCD_A_RST_CON_STUS		(RST_CON_STUS + 0x20)
#define THS1_VCD_B_RST_CON_STUS		(RST_CON_STUS + 0x24)
#define THS1_TCACH_RST_CON_STUS		(RST_CON_STUS + 0x40)
#define WRAPPER_SIZE			0x1000

#define DWL_CLIENT_TYPE_H264_DEC        1
#define DWL_CLIENT_TYPE_MPEG4_DEC       2
#define DWL_CLIENT_TYPE_JPEG_DEC        3
#define DWL_CLIENT_TYPE_PP              4
#define DWL_CLIENT_TYPE_VC1_DEC         5
#define DWL_CLIENT_TYPE_MPEG2_DEC       6
#define DWL_CLIENT_TYPE_VP6_DEC         7
#define DWL_CLIENT_TYPE_AVS_DEC         8
#define DWL_CLIENT_TYPE_RV_DEC          9
#define DWL_CLIENT_TYPE_VP8_DEC         10
#define DWL_CLIENT_TYPE_VP9_DEC         11
#define DWL_CLIENT_TYPE_HEVC_DEC        12
#define DWL_CLIENT_TYPE_ST_PP           14

#define DWL_MPEG2_E      31	/* 1 bit  */
#define DWL_VC1_E        29	/* 2 bits */
#define DWL_JPEG_E       28	/* 1 bit  */
#define DWL_MPEG4_E      26	/* 2 bits */
#define DWL_H264_E       24	/* 2 bits */
#define DWL_VP6_E        23	/* 1 bit  */
#define DWL_RV_E         26	/* 2 bits */
#define DWL_VP8_E        23	/* 1 bit  */
#define DWL_VP7_E        24	/* 1 bit  */
#define DWL_WEBP_E       19	/* 1 bit  */
#define DWL_AVS_E        22	/* 1 bit  */
#define DWL_G1_PP_E      16	/* 1 bit  */
#define DWL_G2_PP_E      31	/* 1 bit  */
#define DWL_PP_E         31	/* 1 bit  */
#define DWL_HEVC_E       26	/* 3 bits */
#define DWL_VP9_E        29	/* 3 bits */

#define VCD_IRQ_STAT		1
#define VCD_IRQ_STAT_OFF	(VCD_IRQ_STAT * 4)
#define VCD_CTRL_REG3		3
#define VCD_CTRL_REG3_OFF	(VCD_CTRL_REG3 * 4)
#define VCD_VLC_CODE_LEN	21
#define VCD_VLC_CODE_LEN_OFF	(VCD_VLC_CODE_LEN * 4)
#define VCD_CFG_STAT		23
#define VCD_CFG_STAT_OFF	(VCD_CFG_STAT * 4)
#define VCD_SYNTH_CFG		50
#define VCD_SYNTH_CFG_OFF	(VCD_SYNTH_CFG * 4)
#define VCD_SYNTH_CFG_2		54
#define VCD_SYNTH_CFG_2_OFF	(VCD_SYNTH_CFG_2 * 4)
#define VCD_SYNTH_CFG_3		56
#define VCD_SYNTH_CFG_3_OFF	(VCD_SYNTH_CFG_3 * 4)
#define VCD_FUSE_CFG		57
#define VCD_FUSE_CFG_OFF	(VCD_FUSE_CFG * 4)
#define VCD_PP_SYNTH_CFG	60
#define VCD_PP_SYNTH_CFG_OFF	(VCD_PP_SYNTH_CFG * 4)
#define VCD_PP_FUSE_CFG		61
#define VCD_PP_FUSE_CFG_OFF	(VCD_PP_FUSE_CFG * 4)
#define VCD_PP_CFG_STAT		260
#define VCD_PP_CFG_STAT_OFF	(VCD_PP_CFG_STAT * 4)

#define VCD_PP_FUSE_CFG_G1	99
#define VCD_PP_FUSE_CFG_G2	99
#define VCD_DEC_ABORT		0x20
#define VCD_DEC_IRQ_DISABLE	0x10
#define VCD_DEC_IRQ		0x100

#define VCDPP_SYNTH_CFG		100
#define VCD_HW_BUILD_ID		309
#define VCD_HW_BUILD_ID_OFF	(VCD_HW_BUILD_ID * 4)

#define VCD_DEC_E		0x1
#define VCD_PP_E		0x1
#define VCD_DEC_IRQ		0x100
#define VCD_PP_IRQ		0x100

/*VC8000D total regs */
#define VCD_REGS_CNT		393
#define VCD_VC8000D_LAST_REG	(VCD_REGS_CNT-1)
#define DEC_IO_SIZE_MAX		(VCD_REGS_CNT * 4)

#define IS_G1(hw_id)		(((hw_id) == 0x6731) ? 1 : 0)
#define IS_G2(hw_id)		(((hw_id) == 0x6732) ? 1 : 0)
#define IS_VC8000D(hw_id)	(((hw_id) == 0x8001) ? 1 : 0)

#define SFT_EN_L2RW_VCD_DEC400_F1_ADB400_SLV_ARESET	(1<<4)
#define SFT_EN_L2RW_VCD_DEC400_F1_WRAPPER_PRESET	(1<<3)
#define SFT_EN_VC8000D_ARESETN_ASYNC			(1<<2)
#define SFT_EN_VC8000D_ARESET				(1<<1)
#define SFT_EN_VC8000D_PRESET				(1<<0)
#define WRAPPER_SFT_RSTN		(\
	SFT_EN_VC8000D_PRESET|SFT_EN_VC8000D_ARESET|\
	SFT_EN_VC8000D_ARESETN_ASYNC|SFT_EN_L2RW_VCD_DEC400_F1_WRAPPER_PRESET|\
	SFT_EN_L2RW_VCD_DEC400_F1_ADB400_SLV_ARESET\
	)

#define SFT_EN_L2RW_VCD_DEC400_F1_ADB400_SLV_ACLK	(1<<4)
#define SFT_EN_L2RW_VCD_DEC400_F1_WRAPPER_PCLK		(1<<3)
#define SFT_EN_VC8000D_ACLK_ASYNC			(1<<2)
#define FT_EN_VC8000D_ACLK				(1<<1)
#define SFT_EN_VC8000D_PCLK				(1<<0)
#define WRAPPER_CLK_EN		(\
	SFT_EN_VC8000D_PCLK|FT_EN_VC8000D_ACLK|\
	SFT_EN_VC8000D_ACLK_ASYNC|SFT_EN_L2RW_VCD_DEC400_F1_WRAPPER_PCLK|\
	SFT_EN_L2RW_VCD_DEC400_F1_ADB400_SLV_ACLK\
	)

#define SFT_EN_VCD_FUSE_DEC		0xFFFFFFFF
#define SFT_EN_VCD_FUSE_PP		0xFFFFFFFF

#define VC8000D_A_WRAPPER(n)		(0x8000+(0x400000*(n)))
#define VC8000D_B_WRAPPER(n)		(0x18000+(0x400000*(n)))

/* offset adress, io space size, irq index */
static const
unsigned int vcd_cores[VCD_MAX_CORES][3] = {
	{VCD_SLICE0_A_OFF, DEC_IO_SIZE_MAX, IRQ_S0_VCD_A},
	{VCD_SLICE0_B_OFF, DEC_IO_SIZE_MAX, IRQ_S0_VCD_B},
	{VCD_SLICE1_A_OFF, DEC_IO_SIZE_MAX, IRQ_S1_VCD_A},
	{VCD_SLICE1_B_OFF, DEC_IO_SIZE_MAX, IRQ_S1_VCD_B}
};

/* there are five interrupt source use a same irq of every core. */
static const
char *vc8000d_interrupt_info[4][5] = {
	{
	 "s0_vcd_a_vc8000d_top_int",
	 "s0_vcd_a_sram_ecc_e2_int",
	 "s0_vcd_a_sram_ecc_e1_int",
	 "s0_vcd_a_l2rw_vcd_int",
	 "s0_vcd_a_dec400_f1_int"},
	{
	 "s1_vcd_a_vc8000d_top_int",
	 "s1_vcd_a_sram_ecc_e2_int",
	 "s1_vcd_a_sram_ecc_e1_int",
	 "s1_vcd_a_l2rw_vcd_int",
	 "s1_vcd_a_dec400_f1_int"},
	{
	 "s0_vcd_b_vc8000d_top_int",
	 "s0_vcd_b_sram_ecc_e2_int",
	 "s0_vcd_b_sram_ecc_e1_int",
	 "s0_vcd_b_l2rw_vcd_int",
	 "s0_vcd_b_dec400_f1_int"},
	{
	 "s1_vcd_b_vc8000d_top_int",
	 "s1_vcd_b_sram_ecc_e2_int",
	 "s1_vcd_b_sram_ecc_e1_int",
	 "s1_vcd_b_l2rw_vcd_int",
	 "s1_vcd_b_dec400_f1_int"}
};

const char *core_status[5] = {
	"idle",
	"reserve",
	"enable",
	"rcv_irq",
	"chk_irq"
};

static void vcd_enable_clock(struct vc8000d_t *tvcd, int id)
{
	u32 status;

	switch (id) {
	case 0:
		status = ccm_read(tvcd->tdev, THS0_VCD_A_CLK_CON_STUS);
		status |= CLK_ENABLE;
		ccm_write(tvcd->tdev, THS0_VCD_A_CLK_CON_STUS, status);
		break;
	case 1:
		status = ccm_read(tvcd->tdev, THS0_VCD_B_CLK_CON_STUS);
		status |= CLK_ENABLE;
		ccm_write(tvcd->tdev, THS0_VCD_B_CLK_CON_STUS, status);
		break;
	case 2:
		status = ccm_read(tvcd->tdev, THS1_VCD_A_CLK_CON_STUS);
		status |= CLK_ENABLE;
		ccm_write(tvcd->tdev, THS1_VCD_A_CLK_CON_STUS, status);
		break;
	case 3:
		status = ccm_read(tvcd->tdev, THS1_VCD_B_CLK_CON_STUS);
		status |= CLK_ENABLE;
		ccm_write(tvcd->tdev, THS1_VCD_B_CLK_CON_STUS, status);
		break;
	default:
		trans_dbg(tvcd->tdev, TR_ERR,
			  "vc8000d: %s core:%d error.\n", __func__, id);
		break;
	}
}

static void vcd_disable_clock(struct vc8000d_t *tvcd, int id)
{
	u32 status;

#ifndef VCD_CLOCK_CONTROL
	return;
#endif
	switch (id) {
	case 0:
		status = ccm_read(tvcd->tdev, THS0_VCD_A_CLK_CON_STUS);
		status &= (~CLK_ENABLE);
		ccm_write(tvcd->tdev, THS0_VCD_A_CLK_CON_STUS, status);
		break;
	case 1:
		status = ccm_read(tvcd->tdev, THS0_VCD_B_CLK_CON_STUS);
		status &= (~CLK_ENABLE);
		ccm_write(tvcd->tdev, THS0_VCD_B_CLK_CON_STUS, status);
		break;
	case 2:
		status = ccm_read(tvcd->tdev, THS1_VCD_A_CLK_CON_STUS);
		status &= (~CLK_ENABLE);
		ccm_write(tvcd->tdev, THS1_VCD_A_CLK_CON_STUS, status);
		break;
	case 3:
		status = ccm_read(tvcd->tdev, THS1_VCD_B_CLK_CON_STUS);
		status &= (~CLK_ENABLE);
		ccm_write(tvcd->tdev, THS1_VCD_B_CLK_CON_STUS, status);
		break;
	default:
		trans_dbg(tvcd->tdev, TR_ERR,
			"vc8000d: %s core:%d error.\n", __func__, id);
		break;
	}
}

/*
 * pll_1, for slice_0 vc8000d,offset is 0x210 base on ccm
 * pll_2, for slice_1 vc8000d,offset is 0x220 base on ccm
 */
int adjust_vcd_pll(struct cb_tranx_t *tdev, u32 slice_id)
{
	int ret = 0;
	u32 pll_m, pll_s;

	switch (tdev->clock_adjust) {
	case 0:
		pll_m = VCD_PLL_M_NORMAL;
		pll_s = VCD_PLL_S_NORMAL;
		break;
	case 1:
		pll_m = VCD_PLL_M_75;
		pll_s = VCD_PLL_S_75;
		break;
	case 2:
		pll_m = VCD_PLL_M_50;
		pll_s = VCD_PLL_S_50;
		break;
	case 3:
		pll_m = VCD_PLL_M_25;
		pll_s = VCD_PLL_S_25;
		break;
	case 4:
		pll_m = 0;
		pll_s = 0;
		break;
	default:
		trans_dbg(tdev, TR_ERR,
			  "PLL: %s adjust:%d error, slice_id:%d.\n",
			  __func__, tdev->clock_adjust, slice_id);
		ret = -EFAULT;
	}

	/* slice_0 id is 1, slice_1 id is 2 */
	if (!ret)
		ret = adjust_video_pll(tdev, pll_m, pll_s, (slice_id+1));

	return ret;
}

/* software reset vc8000d core a */
static int vcd_a_sft_reset(struct cb_tranx_t *tdev, int core)
{
	unsigned int val;
	unsigned int round = RESET_ROUND;

	val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_CON(core));
	ccm_write(tdev, DDR1_P_CHNL_PWR_DWN_CON(core), val|0x1);

	while (round--) {
		val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_STUS(core));
		if ((val&0x3) == 0x1) {
			ccm_write(tdev, VCD_A_RST_CON_STUS(core), 0x0);
			break;
		}
		usleep_range(100, 200);
	}
	if ((val&0x3) != 0x1)
		trans_dbg(tdev, TR_ERR,
			"vc8000d: %s core:%d failed, val=0x%x.\n",
			__func__, core, val);

	return ((val&0x3) == 0x1) ? 0 : -EFAULT;
}

/* software release vc8000d core a */
static int vcd_a_sft_release(struct cb_tranx_t *tdev,
				   void __iomem *vcdbase,
				   int core)
{
	unsigned int val;
	unsigned int round = RESET_ROUND;

	ccm_write(tdev, VCD_A_RST_CON_STUS(core), 0x1);
	writel(0x1f, vcdbase+VC8000D_A_WRAPPER(core)+0x4);
	readl(vcdbase+VC8000D_A_WRAPPER(core)+0x4);

	writel(0xFFFFFFFF, vcdbase+VC8000D_A_WRAPPER(core)+0x8);
	readl(vcdbase+VC8000D_A_WRAPPER(core)+0x8);

	val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_CON(core));
	ccm_write(tdev, DDR1_P_CHNL_PWR_DWN_CON(core), val&0xfffffffe);
	while (round--) {
		val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_STUS(core));
		if ((val & 0x3) == 0x1)
			break;
		usleep_range(100, 200);
	}
	if ((val&0x3) != 0x1)
		trans_dbg(tdev, TR_ERR,
			"vc8000d: %s core:%d failed, val=0x%x.\n",
			__func__, core, val);

	return ((val&0x3) == 0x1) ? 0 : -EFAULT;
}

/* software reset vc8000d core b */
static int vcd_b_sft_reset(struct cb_tranx_t *tdev, int core)
{
	unsigned int val;
	unsigned int round = RESET_ROUND;

	val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_CON(core));
	ccm_write(tdev, DDR1_P_CHNL_PWR_DWN_CON(core), val|0x2);
	while (round--) {
		val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_STUS(core));
		if ((val&0xc) == 0x4) {
			ccm_write(tdev, DDR1_P_CHNL_PWR_DWN_STUS(core), 0x4);
			ccm_write(tdev, VCD_B_RST_CON_STUS(core), 0x0);
			break;
		}
		usleep_range(100, 200);
	}
	if ((val&0xc) != 0x4)
		trans_dbg(tdev, TR_ERR,
			"vc8000d: %s core:%d failed, val=0x%x.\n",
			__func__, core, val);

	return ((val&0xc) == 0x4) ? 0 : -EFAULT;
}

/* software release vc8000d core b */
static int vcd_b_sft_release(struct cb_tranx_t *tdev,
				   void __iomem *vcdbase,
				   int core)
{
	unsigned int val;
	unsigned int round = RESET_ROUND;

	ccm_write(tdev, VCD_B_RST_CON_STUS(core), 0x1);
	val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_CON(core));
	ccm_write(tdev, DDR1_P_CHNL_PWR_DWN_CON(core), val&0xfffffffd);

	writel(0x1f, vcdbase+VC8000D_B_WRAPPER(core)+0x4);
	readl(vcdbase+VC8000D_B_WRAPPER(core)+0x4);

	writel(0xFFFFFFFF, vcdbase+VC8000D_B_WRAPPER(core)+0x8);
	readl(vcdbase+VC8000D_B_WRAPPER(core)+0x8);

	while (round--) {
		val = ccm_read(tdev, DDR1_P_CHNL_PWR_DWN_STUS(core));
		if ((val&0xc) == 0x4) {
			ccm_write(tdev, DDR1_P_CHNL_PWR_DWN_STUS(core), 0x4);
			break;
		}
		usleep_range(100, 200);
	}
	if ((val&0xc) != 0x4)
		trans_dbg(tdev, TR_ERR,
			"vc8000d: %s core:%d failed, val=0x%x.\n",
			__func__, core, val);

	return ((val&0xc) == 0x4) ? 0 : -EFAULT;
}

/* reset vc8000d hardware: first software reset then software release */
int vc8000d_core_reset(struct cb_tranx_t *tdev, int core_id)
{
	void __iomem *vcdbase;
	int ret = -EFAULT;

	mutex_lock(&tdev->reset_lock);
	vcdbase = tdev->bar2_virt + VCD_SLICE0_A_OFF;
	switch (core_id) {
		case 0:
			if (vcd_a_sft_reset(tdev, 0))
				goto out;
			ccm_write(tdev, 0x100A4, 0xFFFFFFFF);
			if (vcd_a_sft_release(tdev, vcdbase, 0))
				goto out;
			break;
		case 1:
			if (vcd_b_sft_reset(tdev, 0))
				goto out;
			ccm_write(tdev, 0x100AC, 0xFFFFFFFF);
			if (vcd_b_sft_release(tdev, vcdbase, 0))
				goto out;
			break;
		case 2:
			if (vcd_a_sft_reset(tdev, 1))
				goto out;
			ccm_write(tdev, 0x10114, 0xFFFFFFFF);
			if (vcd_a_sft_release(tdev, vcdbase, 1))
				goto out;
			break;
		case 3:
			if (vcd_b_sft_reset(tdev, 1))
				goto out;
			ccm_write(tdev, 0x1011C, 0xFFFFFFFF);
			if (vcd_b_sft_release(tdev, vcdbase, 1))
				goto out;
			break;
		default:
			goto out;
	}
	trans_dbg(tdev, TR_DBG,
		"vc8000d: %s core %d reset done.\n", __func__, core_id);
	ret = 0;
out:
	mutex_unlock(&tdev->reset_lock);
	return ret;
}

/* check hardware ID */
static void vcd_check_id(struct vc8000d_t *tvcd)
{
	int i;
	u32 hwid;

	tvcd->cores = 0;
	for (i = 0; i < VCD_MAX_CORES; i++) {
		/* product version only */
		hwid = ((readl(tvcd->core[i].hwregs)) >> 16) & 0xFFFF;
		if (IS_VC8000D(hwid)) {
			tvcd->cores++;
			tvcd->core[i].hw_id = readl(tvcd->core[i].hwregs);
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: Supported HW found at 0x%lx,ID:0x%x\n",
				tvcd->core[i].hwbase,
				readl(tvcd->core[i].hwregs));
		} else {
			trans_dbg(tvcd->tdev, TR_ERR,
				"vc8000d: Unknown HW found at 0x%lx,ID:0x%x\n",
				tvcd->core[i].hwbase,
				readl(tvcd->core[i].hwregs));
		}
	}
}

/* clear all regisetr to 0. */
static void vcd_clear_all_regs(struct vc8000d_t *tvcd)
{
	int i, j;
	u32 status;

	for (j = 0; j < tvcd->cores; j++) {
		status = readl(tvcd->core[j].hwregs + VCD_IRQ_STAT_OFF);
		if (status & VCD_DEC_E) {
			/* abort with IRQ disabled */
			status = VCD_DEC_ABORT | VCD_DEC_IRQ_DISABLE;
			writel(status, tvcd->core[j].hwregs+VCD_IRQ_STAT_OFF);
		}

		for (i = 4; i < tvcd->core[j].iosize; i += 4)
			writel(0, tvcd->core[j].hwregs + i);
	}
}

static int core_has_format(const u32 *cfg, int id, u32 format)
{
	return (cfg[id] & (1 << format)) ? 1 : 0;
}

/* get the feature of every core */
static void read_core_config(struct vc8000d_t *tvcd)
{
	int c;
	u32 reg, tmp, mask;

	memset(tvcd->core_format, 0, sizeof(tvcd->core_format));
	for (c = 0; c < tvcd->cores; c++) {
		/* Decoder configuration */
		reg = readl(tvcd->core[c].hwregs + VCD_SYNTH_CFG * 4);

		tmp = (reg >> DWL_H264_E) & 0x3;
		if (tmp)
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has H264\n", c);
		tvcd->core_format[c] |= tmp ? 1 << DWL_CLIENT_TYPE_H264_DEC : 0;

		tmp = (reg >> DWL_JPEG_E) & 0x01;
		if (tmp)
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has JPEG\n", c);
		tvcd->core_format[c] |= tmp ? 1 << DWL_CLIENT_TYPE_JPEG_DEC : 0;

		tmp = (reg >> DWL_MPEG4_E) & 0x3;
		if (tmp)
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has MPEG4\n", c);
		tvcd->core_format[c] |= tmp ? 1<<DWL_CLIENT_TYPE_MPEG4_DEC : 0;

		tmp = (reg >> DWL_VC1_E) & 0x3;
		if (tmp)
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has VC1\n", c);
		tvcd->core_format[c] |= tmp ? 1 << DWL_CLIENT_TYPE_VC1_DEC : 0;

		tmp = (reg >> DWL_MPEG2_E) & 0x01;
		if (tmp)
			trans_dbg(tvcd->tdev, TR_DBG,
			"vc8000d: core[%d] has MPEG2\n", c);
		tvcd->core_format[c] |= tmp ? 1 << DWL_CLIENT_TYPE_MPEG2_DEC : 0;

		tmp = (reg >> DWL_VP6_E) & 0x01;
		if (tmp)
			trans_dbg(tvcd->tdev, TR_DBG,
			"vc8000d: core[%d] has VP6\n", c);
		tvcd->core_format[c] |= tmp ? 1 << DWL_CLIENT_TYPE_VP6_DEC : 0;

		reg = readl(tvcd->core[c].hwregs + VCD_SYNTH_CFG_2 * 4);

		/* VP7 and WEBP is part of VP8 */
		mask = (1 << DWL_VP8_E) | (1 << DWL_VP7_E) | (1 << DWL_WEBP_E);
		tmp = (reg & mask);
		if (tmp & (1 << DWL_VP8_E))
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has VP8\n", c);
		if (tmp & (1 << DWL_VP7_E))
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has VP7\n", c);
		if (tmp & (1 << DWL_WEBP_E))
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has WebP\n", c);
		tvcd->core_format[c] |= tmp ? 1 << DWL_CLIENT_TYPE_VP8_DEC : 0;

		tmp = (reg >> DWL_AVS_E) & 0x01;
		if (tmp)
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has AVS\n", c);
		tvcd->core_format[c] |= tmp ? 1 << DWL_CLIENT_TYPE_AVS_DEC : 0;

		tmp = (reg >> DWL_RV_E) & 0x03;
		if (tmp)
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has RV\n", c);
		tvcd->core_format[c] |= tmp ? 1 << DWL_CLIENT_TYPE_RV_DEC : 0;

		reg = readl(tvcd->core[c].hwregs + VCD_SYNTH_CFG_3 * 4);

		tmp = (reg >> DWL_HEVC_E) & 0x07;
		if (tmp)
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has HEVC\n", c);
		tvcd->core_format[c] |= tmp ? 1 << DWL_CLIENT_TYPE_HEVC_DEC : 0;

		tmp = (reg >> DWL_VP9_E) & 0x07;
		if (tmp)
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has VP9\n", c);
		tvcd->core_format[c] |= tmp ? 1 << DWL_CLIENT_TYPE_VP9_DEC : 0;

		/* Post-processor configuration */
		reg = readl(tvcd->core[c].hwregs + VCD_PP_CFG_STAT * 4);

		tmp = (reg >> DWL_PP_E) & 0x01;
		if (tmp)
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has PP\n", c);
		tvcd->core_format[c] |= tmp ? 1 << DWL_CLIENT_TYPE_PP : 0;

		if ((c == 0) || (c == 2)) {
			tvcd->core_format[c] |= 1 << DWL_CLIENT_TYPE_ST_PP;
			trans_dbg(tvcd->tdev, TR_DBG,
				"vc8000d: core[%d] has ST_PP\n", c);
		}
	}
}

/* get a core supported this format */
static int find_a_core(struct vc8000d_t *tvcd,
			   struct file *filp,
			   unsigned int format)
{
	int c;
	int id = -1;

	for (c = 0; c < tvcd->cores; c++) {
		if (core_has_format(tvcd->core_format, c, format)) {
			id = c;
			break;
		}
	}
	return id;
}

/*
 * Called by vc8000d_release_core,When has a idle core, this function will
 * query LIVE queue first,if get a LIVE element, give the core to it,
 * then delete it from LIVE list. Until LIVE list is empty, will query
 * VOD queue. If all list is empty, only return.
 */
static void vcd_kickoff_next_task(struct vc8000d_t *tvcd, int id)
{
	int find = 0;
	struct rsv_taskq *f, *t;

	if (!list_empty(&tvcd->list_live)) {
		trans_dbg(tvcd->tdev, TR_DBG, "vc8000d: list_live count %d.\n",
			tvcd->live_count);
		list_for_each_entry_safe(f, t, &tvcd->list_live, rsv_list) {
			if ((f->reserved == 0) &&
				(core_has_format(tvcd->core_format,
					id, f->format))) {
				find = 1;
				f->reserved = 1;
				tvcd->core[id].filp = f->filp;
				f->core_id = id;
				tvcd->live_count--;
				trans_dbg(tvcd->tdev, TR_DBG,
					"vc8000d: get a live element,format:0x%x,core:%d\n",
					f->format, id);
				break;
			}
		}
	}
	/* live list is empty */
	if ((find == 0) && !list_empty(&tvcd->list_vod)) {
		trans_dbg(tvcd->tdev, TR_DBG,
			"vc8000d: list_vod count %d.\n", tvcd->vod_count);
		list_for_each_entry_safe(f, t, &tvcd->list_vod, rsv_list) {
			if ((f->reserved == 0) &&
				(core_has_format(tvcd->core_format,
					id, f->format))) {
				f->reserved = 1;
				tvcd->core[id].filp = f->filp;
				f->core_id = id;
				tvcd->vod_count--;
				trans_dbg(tvcd->tdev, TR_DBG,
					"vc8000d: get a vod element,format:0x%x,core:%d\n",
					f->format, id);
				break;
			}
		}
	}
}

/*
 * Get a idle core; if there are a idle core now, return the core id;
 * else accroding the priority, add the reserve request to queue,wait
 * other application release core.
 * There are two priority: VOD and LIVE, the level of LIVE is
 * higher than VOD.So LIVE level will get a idle core first. Until LIVE
 * list is empty, VOD can get idle core.
 */
static int vc8000d_reserve_core(struct vc8000d_t *tvcd,
				       struct file *filp,
				       unsigned int format,
				       char task_priority)
{
	int id = -1;
	int i;
	int ret;
	struct rsv_taskq *new;

	if (tvcd->tdev->hw_err_flag)
		return tvcd->tdev->hw_err_flag;

	if (WARN_ON(task_priority > TASK_VOD)) {
		trans_dbg(tvcd->tdev, TR_ERR,
			"vc8000d: task_priority:%d error\n", task_priority);
		return -EINVAL;
	}

	spin_lock(&tvcd->rsv_lock);
	for (i = 0; i < tvcd->cores; i++) {
		if ((tvcd->core[i].filp == NULL) &&
		    (core_has_format(tvcd->core_format, i, format))) {
			tvcd->core[i].filp = filp;
			id = i;
			break;
		}
	}

	if (id != -1) {
		spin_unlock(&tvcd->rsv_lock);
		trans_dbg(tvcd->tdev, TR_DBG,
			"vc8000d: %s get a idle core:%d,priority:%d,filp:0x%p\n",
			__func__, id, task_priority, filp);
	} else {
		/* get a free queue element for reserving */
		for (i = 0; i < TR_MAX_LIST; i++) {
			new = tvcd->taskq[i];
			if (new->used == 0) {
				new->used = 1;
				new->reserved = 0;
				break;
			}
		}
		if (i == TR_MAX_LIST) {
			trans_dbg(tvcd->tdev, TR_ERR,
				"vc8000d: can't get valid element from taskq array\n");
			spin_unlock(&tvcd->rsv_lock);
			return -EFAULT;
		}

		INIT_LIST_HEAD(&new->rsv_list);
		new->filp = filp;
		new->format = format;

		if (task_priority == TASK_LIVE) {
			list_add_tail(&new->rsv_list, &tvcd->list_live);
			tvcd->live_count++;
		} else {
			list_add_tail(&new->rsv_list, &tvcd->list_vod);
			tvcd->vod_count++;
		}
		spin_unlock(&tvcd->rsv_lock);

		ret = wait_event_interruptible(tvcd->hw_queue, new->reserved);
		if (ret) {
			spin_lock(&tvcd->rsv_lock);
			if (new->reserved)
				tvcd->core[new->core_id].filp = NULL;
			list_del(&new->rsv_list);
			new->used = 0;
			new->reserved = 0;
			spin_unlock(&tvcd->rsv_lock);
			trans_dbg(tvcd->tdev, TR_NOTICE,
				"vc8000d: wait reserve terminated,ret:%d,filp:0x%p.\n",
				ret, filp);
			return -ERESTARTSYS;
		}

		spin_lock(&tvcd->rsv_lock);
		id = new->core_id;
		list_del(&new->rsv_list);
		new->used = 0;
		new->reserved = 0;
		spin_unlock(&tvcd->rsv_lock);

		if (id >= tvcd->cores) {
			trans_dbg(tvcd->tdev, TR_ERR,
				"vc8000d: %s, core >= tvcd->cores\n", __func__);
			return -EFAULT;
		}
	}

	if (id == 0)
		tvcd->loading[0].tv_s = ktime_get();
	else if (id == 2)
		tvcd->loading[1].tv_s = ktime_get();

	tvcd->core[id].irq_rcvd = 0;
	/* core_0 and core_1 in slice_0, core_2 and core_3 in slice_1*/
	adjust_vcd_pll(tvcd->tdev, id/2);
	vcd_enable_clock(tvcd, id);

	if (tvcd->core[id].core_status != IDLE_FLAG) {
		trans_dbg(tvcd->tdev, TR_NOTICE,
			"vc8000d: %s core_%d_status:%s error\n",
			__func__, id, core_status[tvcd->core[id].core_status]);
	}
	tvcd->core[id].core_status = RSV_FLAG;
	tvcd->core[id].rsv_cnt++;

	return id;
}

/* release a core, then weak up reserve function. */
static void vc8000d_release_core(struct vc8000d_t *tvcd,
					int id, int mode,
					struct file *filp)
{
	u32 status;
	int c;
	struct cb_tranx_t *tdev = tvcd->tdev;
	struct edma_t *tedma = tdev->modules[TR_MODULE_EDMA];

	if (tvcd->core[id].filp == NULL) {
		trans_dbg(tvcd->tdev, TR_ERR,
			"vc8000d: %s core:%d is not reserve\n", __func__, id);
		return;
	}

	if (mode == ABNORM_EXIT) {
		usleep_range(50000, 60000);
		trans_dbg(tvcd->tdev, TR_NOTICE,
			"vc8000d: %s, abnorm exit, wait core_%d 50ms for ip done, core status:%s\n",
			__func__, id, core_status[tvcd->core[id].core_status]);
		if (id == 0) {
			tedma->tc_info[0].status = 0x11; /* done flag */
			usleep_range(50000, 60000);
		} else if (id == 2) {
			tedma->tc_info[1].status = 0x11; /* done flag */
			usleep_range(50000, 60000);
		}

		vc8000d_core_reset(tvcd->tdev, id);
		if (id == 0) {
			tcache_subsys_reset(tvcd->tdev, 0);
		} else if (id == 2) {
			tcache_subsys_reset(tvcd->tdev, 1);
		}
		tvcd->tdev->hw_err_flag = 0;
	}

	if (tvcd->core[id].core_status != CHK_IRQ_FLAG) {
		trans_dbg(tvcd->tdev, TR_NOTICE,
			"vc8000d: %s, core_%d_status:%s",
			__func__, id, core_status[tvcd->core[id].core_status]);
	}
	tvcd->core[id].core_status = IDLE_FLAG;
	tvcd->core[id].idle_cnt++;

	/* make sure HW is disabled */
	status = readl(tvcd->core[id].hwregs + VCD_IRQ_STAT_OFF);
	if (status & VCD_DEC_E) {
		trans_dbg(tvcd->tdev, TR_ERR,
			"vc8000d: core:%d status is enabled, force reset; status=0x%x, reg[21]=0x%x reg[3]=0x%x, filp=%p\n",
			id, status,
			readl(tvcd->core[id].hwregs + VCD_VLC_CODE_LEN_OFF),
			readl(tvcd->core[id].hwregs + VCD_CTRL_REG3_OFF),
			filp);
		/* abort decoder */
		status |= VCD_DEC_ABORT | VCD_DEC_IRQ_DISABLE;
		writel(status, tvcd->core[id].hwregs + VCD_IRQ_STAT_OFF);
	}

	if (id == 0) {
		tvcd->loading[0].tv_e = ktime_get();
		tvcd->loading[0].time_cnt +=
			ktime_to_us(ktime_sub(tvcd->loading[0].tv_e,
				tvcd->loading[0].tv_s));
	} else if (id == 2) {
		tvcd->loading[1].tv_e = ktime_get();
		tvcd->loading[1].time_cnt +=
			ktime_to_us(ktime_sub(tvcd->loading[1].tv_e,
				tvcd->loading[1].tv_s));
	}

	tvcd->core[id].irq_rcvd = 0;
	vcd_disable_clock(tvcd, id);

	spin_lock(&tvcd->rsv_lock);
	tvcd->core[id].filp = NULL;
	for (c = 0; c < tvcd->cores; c++) {
		if (tvcd->core[c].filp == NULL)
			vcd_kickoff_next_task(tvcd, c);
	}
	spin_unlock(&tvcd->rsv_lock);

	wake_up_interruptible_all(&tvcd->hw_queue);
}

/*
 * Calculate the utilization of decoder in one second.
 * So the total used time divide one second is the utilization.
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0))
static void dec_loading_timer_isr(unsigned long data)
{
	struct vc8000d_t *tvcd = (struct vc8000d_t *)data;
#else
static void dec_loading_timer_isr(struct timer_list *t)
{
	struct vc8000d_t *tvcd = from_timer(tvcd, t, loading_timer);
#endif
	mod_timer(&tvcd->loading_timer, jiffies + LOADING_TIME*HZ);

	tvcd->loading[0].time_cnt_saved = tvcd->loading[0].time_cnt;
	tvcd->loading[1].time_cnt_saved = tvcd->loading[1].time_cnt;

	tvcd->loading[0].time_cnt = 0;
	tvcd->loading[1].time_cnt = 0;

	tvcd->loading[0].total_time = LOADING_TIME * 1000000;
	tvcd->loading[1].total_time = LOADING_TIME * 1000000;
}

/* Display encoder utilization which is the average of the two core. */
static ssize_t dec_util_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct cb_tranx_t *tdev = dev_get_drvdata(dev);
	struct vc8000d_t *tvcd = tdev->modules[TR_MODULE_VC8000D];
	unsigned long average_loading;

	average_loading = (tvcd->loading[0].time_cnt_saved +
			   tvcd->loading[1].time_cnt_saved) * 100 /
			   (tvcd->loading[0].total_time +
			   tvcd->loading[1].total_time);
	if (average_loading > 100)
		average_loading = 100;

	return sprintf(buf, "%ld%%\n", average_loading);
}

static DEVICE_ATTR_RO(dec_util);

static ssize_t dec_core_status_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct cb_tranx_t *tdev = dev_get_drvdata(dev);
	struct vc8000d_t *tvcd = tdev->modules[TR_MODULE_VC8000D];
	int i, pos = 0;

	for (i = 0; i < 4; i++) {
		pos += sprintf(buf+pos, "core:%d  status:%s\n",
			i, core_status[tvcd->core[i].core_status]);
	}

	return pos;
}

static DEVICE_ATTR_RO(dec_core_status);

static ssize_t dec_reserve_cnt_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct cb_tranx_t *tdev = dev_get_drvdata(dev);
	struct vc8000d_t *tvcd = tdev->modules[TR_MODULE_VC8000D];
	int i, pos = 0;

	for (i = 0; i < 4; i++) {
		pos += sprintf(buf+pos, "core:%d  count:%d\n",
			i, tvcd->core[i].rsv_cnt);
	}

	return pos;
}

static DEVICE_ATTR_RO(dec_reserve_cnt);

static ssize_t dec_reset_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	struct cb_tranx_t *tdev = dev_get_drvdata(dev);
	struct vc8000d_t *tvcd = tdev->modules[TR_MODULE_VC8000D];
	int id, ret;

	if (count == 0)
		return 0;

	ret = sscanf(buf, "%d", &id);
	if (ret != 1) {
		trans_dbg(tdev, TR_ERR, "vc8000d: %s ret=%d, input_val:%d\n",
			  __func__, ret, id);
		return -1;
	}

	trans_dbg(tdev, TR_ERR, "vc8000d: manual reset vc8000d:%d\n",id );
	vcd_enable_clock(tvcd, id);
	vc8000d_core_reset(tdev, id);

	return count;
}
static DEVICE_ATTR_WO(dec_reset);

static struct attribute *trans_dec_sysfs_entries[] = {
	&dev_attr_dec_util.attr,
	&dev_attr_dec_core_status.attr,
	&dev_attr_dec_reserve_cnt.attr,
	&dev_attr_dec_reset.attr,
	NULL
};

static struct attribute_group trans_dec_attribute_group = {
	.name = NULL,
	.attrs = trans_dec_sysfs_entries,
};

void vcd_close(struct cb_tranx_t *tdev, struct file *filp)
{
	int id;
	struct vc8000d_t *tvcd = tdev->modules[TR_MODULE_VC8000D];

	for (id = 0; id < VCD_MAX_CORES; id++) {
		if (tvcd->core[id].filp == filp) {
			trans_dbg(tvcd->tdev, TR_NOTICE,
				  "vc8000d: Abnormal exit, %s core:%d, filp=%p\n",
				  __func__, id, filp);
			vc8000d_release_core(tvcd, id, ABNORM_EXIT, filp);
		}
	}
}

static int check_dec_irq(struct vc8000d_t *tvcd,
			     const struct file *filp,
			     u32 id)
{
	int rdy = 0;

	spin_lock(&tvcd->chk_irq_lock);
	if (tvcd->core[id].irq_rcvd) {
		tvcd->core[id].irq_rcvd = 0;
		rdy = 1;

		if (tvcd->core[id].core_status != RCVD_IRQ_FLAG) {
			trans_dbg(tvcd->tdev, TR_NOTICE,
				"vc8000d: %s, core_%d_status:%s",
				__func__, id, core_status[tvcd->core[id].core_status]);
		}
		tvcd->core[id].chk_irq_cnt++;
		tvcd->core[id].core_status = CHK_IRQ_FLAG;
	}
	spin_unlock(&tvcd->chk_irq_lock);

	return rdy;
}

/*wait for interrupt from specified core*/
long wait_dec_ready(struct vc8000d_t *tvcd,
			const struct file *filp,
			u32 id)
{
	int ret;

	ret = wait_event_interruptible_timeout(tvcd->dec_wait_queue,
				check_dec_irq(tvcd, filp, id), VC8000D_TIMEOUT);
	if (!ret) { //timeout
		trans_dbg(tvcd->tdev, TR_ERR,
			"vc8000d: %s core:%d timeout, hw_status:0x%x\n",
			__func__, id, readl(tvcd->core[id].hwregs + 4));
		ret = -EFAULT;
	} else if (ret > 0)  //done, ok
		ret = 0;

	return ret;
}

/* Query the interrupt status of each core */
static int check_core_irq(struct vc8000d_t *tvcd,
			       const struct file *filp,
			       int *core_id)
{
	u32 format;
	int rdy = 0, id = 0;

	spin_lock(&tvcd->chk_irq_lock);
	for (id = 0; id < tvcd->cores; id++) {
		if ((tvcd->core[id].filp == filp) && (tvcd->core[id].irq_rcvd)) {
			format = readl(tvcd->core[id].hwregs + 3*4);
			if (format&0xF8000000)  /* if current decoding is not h264(0:h264), continue */
				continue;

			/* we have an IRQ for our client */
			/* reset the wait condition(s) */
			tvcd->core[id].irq_rcvd = 0;
			/* signal ready core no. for our client */
			*core_id = id;
			rdy = 1;

			if (tvcd->core[id].core_status != RCVD_IRQ_FLAG) {
				trans_dbg(tvcd->tdev, TR_ERR,
					"vc8000d: %s, core_%d_status:%s",
					__func__, id, core_status[tvcd->core[id].core_status]);
			}
			tvcd->core[id].chk_irq_cnt++;
			tvcd->core[id].core_status = CHK_IRQ_FLAG;
			break;
		}
	}
	spin_unlock(&tvcd->chk_irq_lock);

	return rdy;
}

/*  wait vcd interrupt */
static int vc8000d_wait_core_ready(struct vc8000d_t *tvcd,
					   const struct file *filp,
					   int *id)
{
	int ret;

	ret = wait_event_interruptible(tvcd->dec_wait_queue,
				       check_core_irq(tvcd, filp, id));
	return ret;
}
   
irqreturn_t vcd_isr(int index, void *data)
{
	int i;
	unsigned int handled = 0;
	u32 irq_status_dec, val, chip_int_2_stus;
	struct cb_tranx_t *tdev = data;
	struct vc8000d_t *tvcd = tdev->modules[TR_MODULE_VC8000D];
	struct video_core_info *core = &tvcd->core[index];
	void __iomem *hwregs = core->hwregs;
	irqreturn_t ret = IRQ_NONE;

	if (spin_trylock(&core->irq_lock) == 0)
		return IRQ_NONE;

	/* reconfirm vc8000d status in global interrupt register. */
	val = ccm_read(tdev, GLOBAL_IRQ_REG_OFF);
	if (((index == S0_VCD_A) && (!(val & THS0_VCD_A)))
		|| ((index == S0_VCD_B) && (!(val & THS0_VCD_B)))
		|| ((index == S1_VCD_A) && (!(val & THS1_VCD_A)))
		|| ((index == S1_VCD_B) && (!(val & THS1_VCD_B)))) {
		spin_unlock(&core->irq_lock);
		return IRQ_NONE;
	}

	if (RSV_FLAG != core->core_status) {
		vcd_enable_clock(tvcd, index);
		irq_status_dec = readl(hwregs + VCD_IRQ_STAT_OFF);
		if (irq_status_dec & VCD_DEC_IRQ) {
			/* clear dec IRQ */
			writel(irq_status_dec & (~VCD_DEC_IRQ), hwregs + VCD_IRQ_STAT_OFF);
		}
		trans_dbg(tdev, TR_ERR,
			"vc8000d: %s, core_%d_status:%s err, hw_status:0x%x\n",
			__func__, index, core_status[core->core_status], irq_status_dec);

		spin_unlock(&core->irq_lock);
		return IRQ_NONE;
	}

	irq_status_dec = readl(hwregs + VCD_IRQ_STAT_OFF);
	if ((irq_status_dec & VCD_DEC_IRQ) != VCD_DEC_IRQ) {
		spin_unlock(&core->irq_lock);
		return IRQ_NONE;
	}

	/* interrupt status register read */
	if (tvcd->core[index].filp == NULL) {
		trans_dbg(tvcd->tdev, TR_ERR,
			"vc8000d: %s index:%d, owner is null, fatal error, hw_status=0x%x\n",
			__func__, index, irq_status_dec);
	}

	if (irq_status_dec & ((1<<0)|(1<<2)|(1<<3)|(1<<5)))
		trans_dbg(tvcd->tdev, TR_ERR,
			"vc8000d: %s index:%d, hw_status=0x%x\n",
			__func__, index, irq_status_dec);

	if (irq_status_dec & VCD_DEC_IRQ) {
		/* mask vc8000d */
		val = irq_status_dec | VCD_DEC_IRQ_DISABLE;
		writel(val, hwregs + VCD_IRQ_STAT_OFF);

		/* clear dec IRQ */
		irq_status_dec &= (~VCD_DEC_IRQ);
		writel(irq_status_dec, hwregs + VCD_IRQ_STAT_OFF);

		/* unmask vc8000d */
		irq_status_dec = readl(hwregs + VCD_IRQ_STAT_OFF);
		val = irq_status_dec & (~VCD_DEC_IRQ_DISABLE);
		writel(val, hwregs + VCD_IRQ_STAT_OFF);

		core->core_status = RCVD_IRQ_FLAG;
		core->irq_cnt++;
		core->irq_rcvd = 1;

		handled++;
		ret = IRQ_HANDLED;
	}
	spin_unlock(&core->irq_lock);

	if (handled) {
		wake_up_all(&tvcd->dec_wait_queue);
	} else {
		chip_int_2_stus = ccm_read(tdev, CHIP_INT_2_STUS);
		if (chip_int_2_stus & (1 << (index * VCD_IRQ_CNT))) {
			trans_dbg(tvcd->tdev, TR_ERR,
				"vc8000d: not done interrupt! index=%d global_sta:0x%x dec_status:0x%x\n",
				index, chip_int_2_stus, irq_status_dec);
		}
		for (i = 0; i < 5; i++) {
			if (chip_int_2_stus &
				(1 << (index * VCD_IRQ_CNT + i))) {
				trans_dbg(tvcd->tdev, TR_ERR,
					  "vc8000d: receive irq is: %s.\n",
					  vc8000d_interrupt_info[index][i]);
			}
		}
	}

	return ret;
}

int vc8000d_register_irq(struct cb_tranx_t *tdev)
{
	struct vc8000d_t *tvcd;
	int i, result;
	int n;

	tvcd = tdev->modules[TR_MODULE_VC8000D];
	for (i = 0; i < VCD_MAX_CORES; i++) {
		/* register irq for each core */
		if (tvcd->core[i].irq != -1) {
			trans_dbg(tdev, TR_DBG, "vc8000d: cord:%d IRQ is %d!\n",
				  i, tvcd->core[i].irq);

			result = request_irq(tvcd->core[i].irq, unify_isr,
					IRQF_SHARED|IRQF_NO_THREAD, "vc8000d", (void *)tdev);
			if (result != 0) {
				if (result == -EINVAL) {
					trans_dbg(tdev, TR_ERR,
						"vc8000d: Bad IRQ:%d or handler\n",
						tvcd->core[i].irq);
				} else if (result == -EBUSY) {
					trans_dbg(tdev, TR_ERR,
						"vc8000d: IRQ:%d busy\n",
						tvcd->core[i].irq);
				}
				goto out_irq_free;
			}
		} else {
			trans_dbg(tdev, TR_DBG,
				  "vc8000d: cord:%d IRQ not in use!\n", i);
		}
	}
	return 0;

out_irq_free:
	for (n = 0; n < i; n++) {
		if (tvcd->core[n].irq != -1)
			free_irq(tvcd->core[n].irq, (void *)tdev);
	}

	return -EFAULT;
}

static void vc8000d_free_irq(struct cb_tranx_t *tdev)
{
	struct vc8000d_t *tvcd;
	int n;

	tvcd = tdev->modules[TR_MODULE_VC8000D];
	/* free the IRQ */
	for (n = 0; n < VCD_MAX_CORES; n++) {
		if (tvcd->core[n].irq != -1) {
			trans_dbg(tdev, TR_DBG,
				  "vc8000d: free irq tvcd->irq[%d]:%d.\n",
				  n, tvcd->core[n].irq);
			free_irq(tvcd->core[n].irq, (void *)tdev);
		}
	}
}

/* enable the clock of every core and data path */
static void init_clk_rst(struct vc8000d_t *tvcd)
{
	int i;
	void __iomem *wrapper_reg;

	/* CLK: 0x01  -- divider is 1 */
	ccm_write(tvcd->tdev, THS0_VCD_A_CLK_CON_STUS, CLK_ENABLE | DIV_1);
	ccm_write(tvcd->tdev, THS0_VCD_B_CLK_CON_STUS, CLK_ENABLE | DIV_1);
	ccm_write(tvcd->tdev, THS1_VCD_A_CLK_CON_STUS, CLK_ENABLE | DIV_1);
	ccm_write(tvcd->tdev, THS1_VCD_B_CLK_CON_STUS, CLK_ENABLE | DIV_1);

	/* RST: 0x01  -- no soft reset, default is 1. */
	ccm_write(tvcd->tdev, THS0_VCD_A_RST_CON_STUS, NO_SOFT_RESET);
	ccm_write(tvcd->tdev, THS0_VCD_B_RST_CON_STUS, NO_SOFT_RESET);
	ccm_write(tvcd->tdev, THS1_VCD_A_RST_CON_STUS, NO_SOFT_RESET);
	ccm_write(tvcd->tdev, THS1_VCD_B_RST_CON_STUS, NO_SOFT_RESET);

	for (i = 0; i < VCD_MAX_CORES; i++) {
		wrapper_reg = tvcd->tdev->bar2_virt +
			vcd_cores[i][0] + VCD_WRAPPER_REG_OFF;
		writel(WRAPPER_SFT_RSTN, wrapper_reg + WRAPPER_SFT_RSTN_CSR);
		writel(WRAPPER_CLK_EN, wrapper_reg + WRAPPER_CLK_EN_CSR);
		writel(SFT_EN_VCD_FUSE_DEC, wrapper_reg + WRAPPER_FUSE_DEC_CSR);
		writel(SFT_EN_VCD_FUSE_PP, wrapper_reg + WRAPPER_FUSE_PP_CSR);
	}
}

int vc8000d_init(struct cb_tranx_t *tdev)
{
	int i;
	int ret;
	struct rsv_taskq *buf;
	struct vc8000d_t *tvcd;

	tvcd = kzalloc(sizeof(struct vc8000d_t), GFP_KERNEL);
	if (!tvcd) {
		trans_dbg(tdev, TR_ERR, "vc8000d: kmalloc vc8000d_t failed\n");
		goto out;
	}
	tdev->modules[TR_MODULE_VC8000D] = tvcd;
	tvcd->tdev = tdev;

	buf = kzalloc(sizeof(struct rsv_taskq)*TR_MAX_LIST, GFP_KERNEL);
	if (!buf) {
		trans_dbg(tdev, TR_ERR, "vc8000d: kmalloc vcd taskq failed.\n");
		goto out_free_dev;
	}
	for (i = 0; i < TR_MAX_LIST; i++)
		tvcd->taskq[i] = buf + i;

	for (i = 0; i < VCD_MAX_CORES; i++) {
		tvcd->core[i].hwbase =
			pci_resource_start(tdev->pdev, 2) + vcd_cores[i][0];
		tvcd->core[i].hwregs = tdev->bar2_virt + vcd_cores[i][0];
		tvcd->core[i].iosize = vcd_cores[i][1];
		tvcd->core[i].irq = pci_irq_vector(tdev->pdev, vcd_cores[i][2]);
		spin_lock_init(&tvcd->core[i].irq_lock);
	}

	init_clk_rst(tvcd);
	INIT_LIST_HEAD(&tvcd->list_live);
	INIT_LIST_HEAD(&tvcd->list_vod);

	vcd_check_id(tvcd);
	if (!tvcd->cores) {
		trans_dbg(tvcd->tdev, TR_ERR, "vc8000d: can't find any core\n");
		goto out_free_taskq;
	}
	trans_dbg(tdev, TR_DBG, "vc8000d: core_count:%d\n", tvcd->cores);
	trans_dbg(tdev, TR_DBG, "vc8000d: support two slices,irq:%d %d %d %d\n",
		  tvcd->core[S0_VCD_A].irq, tvcd->core[S0_VCD_B].irq,
		  tvcd->core[S1_VCD_A].irq, tvcd->core[S1_VCD_B].irq);

	init_waitqueue_head(&tvcd->dec_wait_queue);
	init_waitqueue_head(&tvcd->hw_queue);
	spin_lock_init(&tvcd->rsv_lock);
	spin_lock_init(&tvcd->chk_irq_lock);

	/* read configuration of each core */
	read_core_config(tvcd);
	for (i = 0; i < tvcd->cores; i++) {
		tvcd->core[i].build_id =
			readl(tvcd->core[i].hwregs + VCD_HW_BUILD_ID_OFF);
		tvcd->vcd_cfg[i].core_id = i;
		tvcd->vcd_cfg[i].asic_id = readl(tvcd->core[i].hwregs);
		tvcd->vcd_cfg[i].hantrodec_synth_cfg =
			readl(tvcd->core[i].hwregs + VCD_SYNTH_CFG_OFF);
		tvcd->vcd_cfg[i].hantrodec_synth_cfg_2 =
			readl(tvcd->core[i].hwregs + VCD_SYNTH_CFG_2_OFF);
		tvcd->vcd_cfg[i].hantrodec_synth_cfg_3 =
			readl(tvcd->core[i].hwregs + VCD_SYNTH_CFG_3_OFF);
		tvcd->vcd_cfg[i].hantrodec_fuse_cfg =
			readl(tvcd->core[i].hwregs + VCD_FUSE_CFG_OFF);
		tvcd->vcd_cfg[i].hantrodecpp_synth_cfg =
			readl(tvcd->core[i].hwregs + VCD_PP_SYNTH_CFG_OFF);
		tvcd->vcd_cfg[i].hantrodecpp_cfg_stat =
			readl(tvcd->core[i].hwregs + VCD_PP_CFG_STAT_OFF);
		tvcd->vcd_cfg[i].hantrodecpp_fuse_cfg =
			readl(tvcd->core[i].hwregs + VCD_PP_FUSE_CFG_OFF);

		trans_dbg(tdev, TR_DBG, "vc8000d: core_id:0x%x asic_id:0x%x\n",
			  tvcd->vcd_cfg[i].core_id, tvcd->vcd_cfg[i].asic_id);
		trans_dbg(tdev, TR_DBG,
			"vc8000d: synth_cfg:0x%x synth_cfg_2:0x%x\n",
			tvcd->vcd_cfg[i].hantrodec_synth_cfg,
			tvcd->vcd_cfg[i].hantrodec_synth_cfg_2);
		trans_dbg(tdev, TR_DBG,
			"vc8000d: synth_cfg_3:0x%x fuse_cfg:0x%x\n",
			tvcd->vcd_cfg[i].hantrodec_synth_cfg_3,
			tvcd->vcd_cfg[i].hantrodec_fuse_cfg);
		trans_dbg(tdev, TR_DBG,
			"vc8000d: pp_synth_cfg:0x%x pp_cfg_stat:0x%x\n",
			tvcd->vcd_cfg[i].hantrodecpp_synth_cfg,
			tvcd->vcd_cfg[i].hantrodecpp_cfg_stat);
		trans_dbg(tdev, TR_DBG, "vc8000d: pp_fuse_cfg:0x%x\n",
			tvcd->vcd_cfg[i].hantrodecpp_fuse_cfg);
	}

	/* clear all registers */
	vcd_clear_all_regs(tvcd);

	for (i = 0; i < tvcd->cores; i++)
		vc8000d_core_reset(tdev, i);

	/* End of initialization will disable all cores' clock */
	for (i = 0; i < tvcd->cores; i++)
		vcd_disable_clock(tvcd, i);

	if (vc8000d_register_irq(tdev)) {
		trans_dbg(tdev, TR_ERR, "vc8000d: register irq failed.\n");
		goto out_free_taskq;
	}

	ret = sysfs_create_group(&tdev->misc_dev->this_device->kobj,
				 &trans_dec_attribute_group);
	if (ret) {
		trans_dbg(tdev, TR_ERR,
			"vc8000d: failed to create sysfs device attributes\n");
		goto out_free_irq;
	}

	tvcd->loading_timer.expires = jiffies + LOADING_TIME*HZ;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0))
	tvcd->loading_timer.function = (void *)dec_loading_timer_isr;
	tvcd->loading_timer.data = (unsigned long)(tvcd);
	init_timer(&tvcd->loading_timer);
#else
	timer_setup(&tvcd->loading_timer, dec_loading_timer_isr, 0);
#endif
	add_timer(&tvcd->loading_timer);

	trans_dbg(tdev, TR_INF, "vc8000d: module initialize done.\n");
	return 0;

out_free_irq:
	vc8000d_free_irq(tdev);
out_free_taskq:
	kfree(buf);
out_free_dev:
	kfree(tvcd);
out:
	trans_dbg(tdev, TR_ERR, "vc8000d: module initialize failed.\n");
	return -EFAULT;
}

int vc8000d_release(struct cb_tranx_t *tdev)
{
	int i;
	struct vc8000d_t *tvcd = tdev->modules[TR_MODULE_VC8000D];

	del_timer_sync(&tvcd->loading_timer);
	sysfs_remove_group(&tdev->misc_dev->this_device->kobj,
			   &trans_dec_attribute_group);
	vc8000d_free_irq(tdev);
	for (i = 0; i < tvcd->cores; i++)
		vcd_disable_clock(tvcd, i);
	kfree(tvcd);

	trans_dbg(tdev, TR_DBG, "vc8000d: remove module done.\n");
	return 0;
}

long vc8000d_ioctl(struct file *filp,
			unsigned int cmd,
			unsigned long arg,
			struct cb_tranx_t *tdev)
{
	int ret = 0;
	u32 id, hw_id, io_size;
	int core_id;
	struct core_info info;
	struct vcd_core_config cfg;
	struct vc8000d_t *tvcd;
	void __user *argp = (void __user *)arg;

	tvcd = tdev->modules[TR_MODULE_VC8000D];
	switch (cmd) {
	case CB_TRANX_VCD_IO_SIZE:
		__get_user(id, (u32 *)argp);
		if (id >= tvcd->cores)
			return -EFAULT;
		io_size = tvcd->core[id].iosize;
		__put_user(io_size, (u32 *)argp);
		break;
	case CB_TRANX_VCD_CORE_CNT:
		__put_user(tvcd->cores, (u32 *)argp);
		trans_dbg(tdev, TR_DBG,
			"vc8000d: hantrodec_data->cores:%d\n", tvcd->cores);
		break;
	case CB_TRANX_VCD_GET_HWID:
		__get_user(id, (u32 *)argp);
		if (id >= tvcd->cores)
			return -EFAULT;
		id = tvcd->core[id].hw_id;
		__put_user(id, (u32 *)argp);
		break;
	case CB_TRANX_VCD_FIND_CORE:
		trans_dbg(tdev, TR_DBG, "vc8000d: get core_id,format:%d\n", arg);
		ret = find_a_core(tvcd, filp, arg);
		break;
	case CB_TRANX_VCD_GET_BUILDID:
		__get_user(id, (u32 *)argp);
		if (id >= tvcd->cores)
			return -EFAULT;
		hw_id = tvcd->core[id].hw_id;
		if (!(IS_G1(hw_id >> 16) || IS_G2(hw_id >> 16)))
			hw_id = tvcd->core[id].build_id;
		__put_user(hw_id, (u32 *)argp);
		break;
	case CB_TRANX_VCD_GET_CFG:
		ret = copy_from_user(&cfg, argp, sizeof(cfg));
		if (ret) {
			trans_dbg(tdev, TR_ERR,
				"vc8000d: get_cfg copy_from_user failed.\n");
			return -EFAULT;
		}
		if (cfg.core_id >= tvcd->cores) {
			trans_dbg(tdev, TR_ERR,
				"vc8000d: get_cfg id:%d error\n", cfg.core_id);
			return -EFAULT;
		}
		ret = copy_to_user(argp, (void *)&tvcd->vcd_cfg[cfg.core_id],
				   sizeof(struct vcd_core_config));
		if (ret) {
			trans_dbg(tdev, TR_ERR,
				"vc8000d: get_cfg copy_to_user failed.\n");
			return -EFAULT;
		}
		break;
	case CB_TRANX_VCD_RESERVE:
		ret = copy_from_user(&info, argp, sizeof(info));
		if (ret) {
			trans_dbg(tdev, TR_ERR,
				"vc8000d: reserve copy_from_user failed\n");
			return -EFAULT;
		}
		core_id = vc8000d_reserve_core(tvcd,
				filp, info.format, info.task_priority);
		return core_id;
	case CB_TRANX_VCD_RELEASE:
		__get_user(id, (u32 *)argp);
		trans_dbg(tdev, TR_DBG, "vc8000d: Release core:%d\n", id);
		if (id >= tvcd->cores || tvcd->core[id].filp != filp) {
			trans_dbg(tdev, TR_ERR,
				"vc8000d: bogus DEC release, core:%d.\n", id);
			return -EFAULT;
		}
		vc8000d_release_core(tvcd, id, NORM_EXIT, filp);
		break;
	case CB_TRANX_VCD_WAIT_DONE:
		core_id = -1;
		ret = vc8000d_wait_core_ready(tvcd, filp, &core_id);
		__put_user(core_id, (int *)argp);
		break;
	case CB_TRANX_VCD_WAIT_CORE_DONE: /*wait for interrupt from specified core*/
		__get_user(id, (u32 *)argp);
		if (id >= tvcd->cores) {
			trans_dbg(tdev, TR_ERR,
				"vc8000d: wait core done, core:%d is error\n",
				id);
			return -EFAULT;
		}
		ret = wait_dec_ready(tvcd, filp, id);
		break;
	default:
		trans_dbg(tdev, TR_ERR,
			"vc8000d: %s, cmd:0x%x is error.\n", __func__, cmd);
		ret = -EINVAL;
	}

	return ret;
}
