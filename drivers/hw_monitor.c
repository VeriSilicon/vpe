// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 VeriSilicon Holdings Co., Ltd.
 *
 * This is hardware monitor driver for Linux.
 * this driver for monitoring the hardware health.
 */

#include <linux/errno.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/pagemap.h>
#include <linux/firmware.h>
#include <linux/pci.h>
#include <linux/interrupt.h>

#include "common.h"
#include "hw_monitor.h"
#include "transcoder.h"

#define FW_LOAD_ADDR		0x3a40000
#define ZSP_SRAM_ADDR		0x3a40000
#define TO_HOST_OFFSET		0x0
#define TO_ZSP_OFFSET		0x200

#define START_MODE_ADDR		0x201F8

/* warm start flag: reset ZSP boot.*/
#define WARM_START_F		0x55AA55AA

#define HEART_BEAT_EVENT	1
#define ERROR_EVENT		2

#define HOST_STATUS_BUSY	1
#define HOST_STATUS_FREE	2

#define BM_OCOTP_CTRL_BUSY	0x100
#define PATCH_VERIFY_ADDR	0x57
#define CHIP_TYPE_ADDR		0xB
#define CHIP_ID_ADDR		0x9
#define CHIP_REV_ADDR		0x8
#define	VERIFY_BIT		29
#define	PATCH_BIT		31
#define OTP_CONTROL		0x0
#define OTP_READ_CONTROL	0xc
#define OTP_READ_DATA		0x10
#define READ_FUSE		0x1
#define EFUSE_BASE_ADDR		0x3b5d000

#define FIRMWARE		"transcoder_zsp_fw.bin"

/* Jump to iTCM, run firmware. */
#define GO_CMD			0x5a5a5a5a

/* get command status:successfully. */
#define DONE			0x12345678
/* Check signature error */
#define SIG_ERROR		0xDEADBEEF
/* ('T','O','E','P'):host send to EP:0x544F4550 */
#define SIGNATURE_TO_EP		0x544F4550

/* ('T','O','R','C'):EP send to host:0x544F5243 */
#define SIGNATURE_TO_RC		0x544F5243

#define	CMD_DIR_ADDR		0xfff4
#define	CMD_RSN_ADDR		0xfff8

#define PCIE_XBAR_AXI_LPI_CON_STUS	0x20478
#define SYS_CON_CSYSREQ			(1<<4)
#define SYS_CON_CACTIVE			(1<<5)
#define SYS_CON_CSYSACK			(1<<6)
#define SYS_CON_SLV_CSYSREQ		(1<<8)
#define SYS_CON_SLV_CACTIVE		(1<<9)
#define SYS_CON_SLV_CSYSACK		(1<<10)
#define RESET_STATUS	(\
			SYS_CON_CSYSREQ | SYS_CON_CACTIVE \
			| SYS_CON_CSYSACK | SYS_CON_SLV_CSYSREQ \
			| SYS_CON_SLV_CACTIVE | SYS_CON_SLV_CSYSACK \
			)

#define RELEASE_STATUS	(\
			SYS_CON_CSYSREQ | SYS_CON_CSYSACK \
			| SYS_CON_SLV_CSYSREQ | SYS_CON_SLV_CSYSACK\
			)

#define SC_RST_CON_STUS			0x150
#define ZSP_SUBSYSTEM_SOFT_RESET	0xffffff00
#define ZSP_CORE_NO_SFT_RST		(1<<6)
#define ZSP_EXTSYS_NO_SFT_RST		(1<<7)

#define ZSP_INT_EN_CON_STUS		0x20304
#define ZSP_SFT_INT_CON_STUS		0x20308
#define CHIP_SFT_INT_CON_STUS		0x2030c
#define ENABLE_CHIP_SFT_INT		(1<<1)
#define ZSP_SFT_INT			0x1

#define ZSP_IRQ_INDEX			2
#define LOAD_TIMEOUT			3
#define HEARTBEAT_TIMEOUT		3
#define WAIT_ZSP_TIME			10 /* jiffers */
#define DDR_CTRL_FREQ			700
#define DDR_PARTICLE_FREQ		2800

#define VER_LEN				4

/* bellow are event_ID for Host_Req and Threshold_Setting */
#define ERROR_STATUS_CLEAR			0
#define PUBLIC_KEY_REVOKE			1
#define THERMAL_LOW_THRESHOLD_MODIFY		2
#define THERMAL_HIGH_THRESHOLD_MODIFY		3
#define DDR_ECC_CNT_THRESHOLD_MODIFY		4
#define SRAM_ECC_CNT_THRESHOLD_MODIFY		5
#define PCIE_ERR_CNT_THRESHOLD_MODIFY		6
#define RESET_ZSP				0xff
#define PUBLIC_KEY_SIZE				32

struct ddr_bandwidth_type {
	unsigned int ddr0_timer_cnt;
	unsigned int ddr0_rd_cnt;
	unsigned int ddr0_wr_cnt;
	unsigned int ddr0_rd_dfi_cnt;
	unsigned int ddr0_wr_dfi_cnt;

	unsigned int ddr1_timer_cnt;
	unsigned int ddr1_rd_cnt;
	unsigned int ddr1_wr_cnt;
	unsigned int ddr1_rd_dfi_cnt;
	unsigned int ddr1_wr_dfi_cnt;
};

struct err_info_type {
	unsigned int thermal_low_cnt;
	unsigned int thermal_high_cnt;
	unsigned int ddr_ecc_cnt;
	unsigned int sram_ecc_cnt;
	unsigned int pcie_err_cnt;
	unsigned int pcie_uncor_fatal_cnt;
	unsigned int pcie_uncor_unfatal_cnt;
	unsigned int reserve;
};

struct threshold_info_type {
	unsigned int thermal_low;
	unsigned int thermal_high;
	unsigned int ddr_ecc;
	unsigned int sram_ecc;
	unsigned int pcie_err;
	unsigned int reserve;
};

struct temp_type {
	unsigned int temperature[4];
	unsigned int history_temp_max;
	unsigned int throttling_time_s;
	unsigned int throttling_cnt;
	unsigned int reserve;
};

struct volt_type {
	unsigned int voltage[4];
};

struct fw_version_type {
	unsigned char zsp_fw[VER_LEN];
	unsigned char pcie_phy_fw[VER_LEN];
	unsigned char rom_patch[VER_LEN];
	unsigned char ddr_1d_fw[VER_LEN];
	unsigned char ddr_2d_fw[VER_LEN];
	unsigned int reserve;
};

/* it's a share memory between driver and firmware, the memory in sram ep side.
 * firmware will update hardware information to sram, so driver can get it.
 * [firmware ---> driver]
 */
struct cb_mail_box_f2d {
	unsigned int event_ID;
	unsigned int process_status;
	unsigned long error_ID;
	struct ddr_bandwidth_type ddr_bw_info;
	struct err_info_type ce_cnt;
	struct err_info_type uce_cnt;
	struct threshold_info_type threshold;
	struct temp_type temperature;
	struct volt_type voltage;
	struct fw_version_type fw_ver;
	unsigned long uptime_s;
	unsigned int power_status;
	unsigned int reserve;
};

/* it's a share memory between driver and firmware, the memory in sram ep side.
 * driver can send information to firmware. [ driver ---> firmware ]
 */
struct cb_mail_box_d2f {
	unsigned long event_ID;
	unsigned long error_ID;
	unsigned int param[8];
};

/*
 * This struct record hardware monitor detail information.
 * @hb_timer: check heartbeat timer
 * @heartbeat_live:  heartbeat count
 * @owner_lock: protect irq handling
 * @irq: irq number
 * @tdev: record struct cb_tranx_t point.
 * @public_key: recore current active public key read from current firmware.
 */
struct hwm_t {
	struct timer_list hb_timer;
	unsigned long heartbeat_live;
	spinlock_t owner_lock;
	unsigned int irq;
	struct cb_tranx_t *tdev;
	u8 public_key[PUBLIC_KEY_SIZE];
};

char *error_info[64] = {
	"thermal_event_low",
	"thermal_event_high",
	"bus_error",
	"pll_adjust_error",
	"mem_repair_error",
	"ddr0_init_error",
	"ddr0_ecc_1bit_error",
	"ddr0_ecc_multi_bit_error",
	"ddr1_init_error",
	"ddr1_ecc_1bit_error",
	"ddr1_ecc_multi_bit_error",
	"spi_trans_error",
	"timer_init_error",
	"i2c_trans_error",
	"perf_monitor_error",
	"ths0_vcd_a_ecc_1bit_err",
	"ths0_vcd_a_ecc_2bit_err",
	"ths0_vcd_b_ecc_1bit_err",
	"ths0_vcd_b_ecc_2bit_err",
	"ths1_vcd_a_ecc_1bit_err",
	"ths1_vcd_a_ecc_2bit_err",
	"ths1_vcd_b_ecc_1bit_err",
	"ths1_vcd_b_ecc_2bit_err",
	"ths0_vce_ecc_1bit_err",
	"ths0_vce_ecc_2bit_err",
	"ths1_vce_ecc_1bit_err",
	"ths1_vce_ecc_2bit_err",
	"ths0_tch_ecc_1bit_err",
	"ths0_tch_ecc_2bit_err",
	"ths1_tch_ecc_1bit_err",
	"ths1_tch_ecc_2bit_err",
	"pcie_internal_err",
	"pcie_unsupported_req_err",
	"pcie_ecrc_err",
	"pcie_malf_tlp_err",
	"pcie_rec_overflow_err",
	"pcie_unexp_cmplt_err",
	"pcie_cmplt_abort_err",
	"pcie_cmplt_timeout_err",
	"pcie_fc_protocol_err",
	"pcie_pois_tlp_err",
	"pcie_surprise_down_err",
	"pcie_dl_protocol_err",
	"pcie_header_log_overflow",
	"pcie_corrected_int_err",
	"pcie_advisory_non_fatal_err",
	"pcie_rpl_timer_timeout",
	"pcie_replay_no_roleover",
	"pcie_bad_dllp",
	"pcie_bad_tlp",
	"pcie_rx_err",
	"abnomal_voltage",
	"thermal_event_recover",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"zsp_warm_boot"
};

/* 1:when install driver will load zsp firmware; 0:not load firmware*/
unsigned int fw = 1;
module_param(fw, uint, 0644);
MODULE_PARM_DESC(fw,
	"load zsp firmware flag,1:load; 0:not load; default is 1.");

/* check firmware heartbeat,1:check; 0:not check */
unsigned int hb = 1;
module_param(hb, uint, 0644);
MODULE_PARM_DESC(hb,
	"check firmware heartbeat,2:check; 1:not check; default is 1.");

static int send_info_to_zsp(struct hwm_t *thwm, void *data);

static void parse_err_id(unsigned long error_id, struct hwm_t *thwm)
{
	int i;

	/* 64 is error count */
	for (i = 0; i < 64; i++)
		if ((error_id >> i) & 0x1) {
			if (i == 0) {
				if (thwm->tdev->reduce_strategy == 0) {
					thwm->tdev->clock_adjust = 1;
					trans_dbg(thwm->tdev, TR_INF,
						"hwm: video pll will reduce to 75%%.\n");
				} else if (thwm->tdev->reduce_strategy == 1) {
					thwm->tdev->clock_adjust = 2;
					trans_dbg(thwm->tdev, TR_INF,
						"hwm: video pll will reduce to 50%%.\n");
				} else if (thwm->tdev->reduce_strategy == 2) {
					thwm->tdev->clock_adjust = 3;
					trans_dbg(thwm->tdev, TR_INF,
						"hwm: video pll will reduce to 25%%.\n");
				} else
					trans_dbg(thwm->tdev, TR_INF,
						"hwm: reduce_strategy:%d error.\n",
						thwm->tdev->reduce_strategy);
			} else if (i == 1) {
				thwm->tdev->clock_adjust = 4;
				trans_dbg(thwm->tdev, TR_INF,
					"hwm: video clock will switch to 25MHz.\n");
			} else if (i == 52) {
				thwm->tdev->clock_adjust = 0;
				trans_dbg(thwm->tdev, TR_INF,
					"hwm: video pll will return to full speed.\n");
			}
			trans_dbg(thwm->tdev, TR_ERR,
				  "hwm: ZSP_REP: %s.\n", error_info[i]);
		}
}

static int reset_zsp(struct hwm_t *thwm)
{
	int ret = -EFAULT;
	unsigned int val;
	unsigned int delay;
	struct cb_mail_box_d2f set_info;

	/* inform zsp, will be reset it. */
	set_info.event_ID = RESET_ZSP;
	send_info_to_zsp(thwm, &set_info);

	ccm_write(thwm->tdev, START_MODE_ADDR, WARM_START_F);

	/* 0x150   reset; 0x20478  low power */
	/* reset ...     */
	val = ccm_read(thwm->tdev, PCIE_XBAR_AXI_LPI_CON_STUS);
	/* set bit4 & bit8 = 0 */
	val &= (~(SYS_CON_CSYSREQ | SYS_CON_SLV_CSYSREQ));
	ccm_write(thwm->tdev, PCIE_XBAR_AXI_LPI_CON_STUS, val);

	delay = 1000;
	while (delay--) {
		val = ccm_read(thwm->tdev, PCIE_XBAR_AXI_LPI_CON_STUS);
		/* bit4&bit5&bit6   bit8&bit9&bit10  all is 0 */
		if ((val & RESET_STATUS) == 0)
			break;
		usleep_range(1, 2);
	}
	if (val & RESET_STATUS) {
		trans_dbg(thwm->tdev, TR_ERR,
			"hwm: check power status_1 timeout\n");
		goto out;
	}
	ccm_write(thwm->tdev, SC_RST_CON_STUS, ZSP_SUBSYSTEM_SOFT_RESET);

	/* release ... */
	val = ccm_read(thwm->tdev, PCIE_XBAR_AXI_LPI_CON_STUS);
	val |= (SYS_CON_CSYSREQ | SYS_CON_SLV_CSYSREQ);
	ccm_write(thwm->tdev, PCIE_XBAR_AXI_LPI_CON_STUS, val);

	delay = 1000;
	while (delay--) {
		val = ccm_read(thwm->tdev, PCIE_XBAR_AXI_LPI_CON_STUS);
		if ((val & RELEASE_STATUS) == RELEASE_STATUS)
			break;
		usleep_range(1, 2);
	}
	if ((val & RELEASE_STATUS) != RELEASE_STATUS) {
		trans_dbg(thwm->tdev, TR_ERR,
			"hwm: check power status_2 timeout\n");
		goto out;
	}
	ccm_write(thwm->tdev, SC_RST_CON_STUS, (ZSP_CORE_NO_SFT_RST |
		  ZSP_EXTSYS_NO_SFT_RST | ZSP_SUBSYSTEM_SOFT_RESET));
	ret = 0;

out:
	return ret;
}

static int load_firmware(struct hwm_t *thwm)
{
	unsigned long delay;
	const struct firmware *fw;
	u32 val_dir, val_st;
	int ret = -EFAULT;
	void __iomem *load = thwm->tdev->bar2_virt + FW_LOAD_ADDR;
	void __iomem *sram = thwm->tdev->bar2_virt + ZSP_SRAM_ADDR;

	if (request_firmware(&fw, FIRMWARE, &thwm->tdev->pdev->dev)) {
		trans_dbg(thwm->tdev, TR_ERR,
			"hwm: request_firmware failed.\n");
		goto out;
	}

	memcpy(thwm->public_key, fw->data+8, PUBLIC_KEY_SIZE);
	memcpy(load, fw->data, fw->size);

	/* send command "go" to ep */
	writel(SIGNATURE_TO_EP, sram + CMD_DIR_ADDR);
	writel(GO_CMD, sram + CMD_RSN_ADDR);

	/* check zsp respond */
	delay = jiffies + LOAD_TIMEOUT * HZ;
	while (time_before(jiffies, delay)) {
		val_dir = readl(sram + CMD_DIR_ADDR);
		val_st = readl(sram + CMD_RSN_ADDR);
		if ((val_dir == SIGNATURE_TO_RC) && (val_st == DONE)) {
			trans_dbg(thwm->tdev, TR_INF,
				"hwm: load firmware successfully.\n");
			ret = 0;
			goto out_release_fw;
		}
		if ((val_dir == SIGNATURE_TO_RC) && (val_st == SIG_ERROR)) {
			trans_dbg(thwm->tdev, TR_ERR, "hwm: signature check failed!\n");
			goto out_release_fw;
		}
		usleep_range(100, 200);
	}
	trans_dbg(thwm->tdev, TR_ERR, "hwm: load firmware failed\n");

out_release_fw:
	release_firmware(fw);
out:
	return ret;
}

static void trigger_zsp_interrupt(struct hwm_t *thwm)
{
	ccm_write(thwm->tdev, ZSP_INT_EN_CON_STUS, ENABLE_CHIP_SFT_INT);
	ccm_write(thwm->tdev, CHIP_SFT_INT_CON_STUS, 0x0);
	ccm_write(thwm->tdev, CHIP_SFT_INT_CON_STUS, ZSP_SFT_INT);
	ccm_write(thwm->tdev, CHIP_SFT_INT_CON_STUS, 0x0);
}

static int send_info_to_zsp(struct hwm_t *thwm, void *data)
{
	int ret = 0;
	struct cb_mail_box_d2f __iomem *host_mem;
	unsigned long delay;
	void __iomem *sram = thwm->tdev->bar2_virt + ZSP_SRAM_ADDR;

	host_mem = sram + TO_ZSP_OFFSET;
	delay = jiffies + WAIT_ZSP_TIME;

	/*
	 * when zsp receive the interrupt,
	 * need to clear the share memory with zero after handling.
	 * so if event_ID and error_ID are not 0,
	 * that means zsp has not handled the last interrupt.
	 */
	while (time_before(jiffies, delay)) {
		if ((host_mem->event_ID + host_mem->error_ID) == 0)
			break;
		usleep_range(10, 20);
	}

	if ((host_mem->event_ID + host_mem->error_ID) == 0) {
		memcpy(host_mem, data, sizeof(struct cb_mail_box_d2f));
		trigger_zsp_interrupt(thwm);
	} else {
		trans_dbg(thwm->tdev, TR_ERR,
			  "hwm: %s, but ZSP not respond.\n", __func__);
		ret = -EFAULT;
	}

	return ret;
}

static struct cb_mail_box_f2d *get_info_from_zsp(struct hwm_t *thwm)
{
	void __iomem *sram = thwm->tdev->bar2_virt + ZSP_SRAM_ADDR;

	return (sram + TO_HOST_OFFSET);
}

static ssize_t temp_sensor_c_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int i;
	unsigned int max_temp = 0;
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	for (i = 0; i < 4; i++)
		max_temp = max(fw_info->temperature.temperature[i], max_temp);

	return sprintf(buf, "%d\n", max_temp);
}

static ssize_t max_temp_c_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->temperature.history_temp_max);
}

static ssize_t throttling_time_s_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->temperature.throttling_time_s);
}

static ssize_t throttling_status_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->temperature.throttling_cnt);
}

/* read one index, index is index of word address */
u32 read_fuse_word(int index, struct cb_tranx_t *tdev)
{
	unsigned int val;
	unsigned long delay;
	void __iomem *efuse_reg = tdev->bar2_virt + EFUSE_BASE_ADDR;

	writel(index, (efuse_reg + OTP_CONTROL));
	writel(READ_FUSE, (efuse_reg + OTP_READ_CONTROL));

	delay = 1000;
	while (delay--) {
		val = readl(efuse_reg);
		/* check efuae status.*/
		if ((val & BM_OCOTP_CTRL_BUSY) == 0)
			break;
		usleep_range(1, 2);
	}
	val = readl(efuse_reg + OTP_READ_DATA);

	return val;
}

/* get this chip type : TT FF SS */
static ssize_t chip_type_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	unsigned int val;
	char chip_type[4];
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;

	/* 0x00 -> TT, 0xAA -> SS, 0x55 -> FF */
	val = read_fuse_word(CHIP_TYPE_ADDR, tdev);
	if (val == 0x00)
		strcpy(chip_type, "TT");
	else if (val == 0xAA)
		strcpy(chip_type, "SS");
	else if (val == 0x55)
		strcpy(chip_type, "FF");
	else {
		trans_dbg(tdev, TR_ERR,
			"get_chip_type failed, val:0x%x.\n", val);
		strcpy(chip_type, "N/A");
	}

	return sprintf(buf, "%s\n", chip_type);
}

/* get this chip revision : A0:0x00; A1:0x01 */
static ssize_t chip_rev_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	unsigned int val;
	char chip_type[4];
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;

	/* 0x00 -> A0, 0x01 -> A1 */
	val = read_fuse_word(CHIP_REV_ADDR, tdev);
	val = val >> 24;
	if (val == 0x00)
		strcpy(chip_type, "A0");
	else if (val == 0x01)
		strcpy(chip_type, "A1");
	else {
		trans_dbg(tdev, TR_ERR, "get_chip_type failed,val:0x%x\n", val);
		strcpy(chip_type, "N/A");
	}

	return sprintf(buf, "%s\n", chip_type);
}


/* get this chip id : lot_id, wafer_id, coordx and coordy */
static ssize_t chip_id_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	unsigned int val_1, val_2;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;

	val_1 = read_fuse_word(CHIP_ID_ADDR, tdev);
	val_2 = read_fuse_word(CHIP_ID_ADDR+1, tdev);

	/* LotID LotID LotID LotID LotID  WaferID  CoordX  CoordY */
	return sprintf(buf, "0x%x%x\n", val_2, val_1);
}

/* get chip current power status */
static ssize_t power_state_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	/* 1:full_power; 2:reduce_power; 3:lowest_power */
	return  sprintf(buf, "%d\n", fw_info->power_status);
}

/* read one public key */
void read_key_from_efuse(struct hwm_t *thwm, int start_index, u8 *key)
{
	u32 data;
	u8 i;
	u8 *tmp_key = key;

	/* every public key has 32 bytes, 8 index. */
	for (i = 0; i < 8; i++) {
		data = read_fuse_word(start_index+i, thwm->tdev);
		tmp_key[0] = (data >> 0) & 0xff;
		tmp_key[1] = (data >> 8) & 0xff;
		tmp_key[2] = (data >> 16) & 0xff;
		tmp_key[3] = (data >> 24) & 0xff;

		tmp_key += 4;
	}
}

/* show all public key which are restored in eFUSE */
static ssize_t secure_keys_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];
	int i, j, pos = 0;
	u8 key[5][32];

	for (i = 0; i < 5; i++)
		read_key_from_efuse(thwm, 88 + i * 8, key[i]);

	for (j = 0; j < 5; j++) {
		pos += sprintf(buf+pos, "Public key %d:\n", j+1);
		for (i = 0; i < PUBLIC_KEY_SIZE; i++) {
			if ((i+1)%16 == 0)
				pos += sprintf(buf+pos, "0x%02x\n", key[j][i]);
			else
				pos += sprintf(buf+pos, "0x%02x ", key[j][i]);
		}
		pos += sprintf(buf+pos, "\n");
	}
	return pos;
}


/* show public key of this zsp firmware */
static ssize_t active_key_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];
	int i, pos = 0;

	for (i = 0; i < PUBLIC_KEY_SIZE; i++) {
		if ((i+1)%16 == 0)
			pos += sprintf(buf+pos, "0x%02x\n", thwm->public_key[i]);
		else
			pos += sprintf(buf+pos, "0x%02x ", thwm->public_key[i]);
	}
	return pos;
}

static ssize_t uptime_s_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%ld\n", fw_info->uptime_s);
}

static ssize_t temp_threshold_lower_c_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf,
						size_t count)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];
	struct cb_mail_box_d2f set_info;
	int threshold;
	int ret;

	if (count == 0)
		return 0;

	ret = sscanf(buf, "%d", &threshold);
	if (ret != 1) {
		trans_dbg(tdev, TR_ERR, "core: %s ret=%d, threshold:%d\n",
			  __func__, ret, threshold);
		return -1;
	}

	if ((threshold < 0) || (threshold > 125)) {
		trans_dbg(tdev, TR_ERR,
			"core: %s, thereshold range is [0 - 125].\n", __func__);
		return -1;
	}
	trans_dbg(tdev, TR_DBG, "core: %s,threshold:%d\n", __func__, threshold);

	set_info.event_ID = THERMAL_LOW_THRESHOLD_MODIFY;
	set_info.param[0] = threshold;
	ret = send_info_to_zsp(thwm, &set_info);

	return ret ? -1 : count;
}

static ssize_t temp_threshold_lower_c_show(struct device *dev,
						  struct device_attribute *attr,
						  char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->threshold.thermal_low);
}

static ssize_t temp_threshold_upper_c_store(struct device *dev,
						  struct device_attribute *attr,
						  const char *buf,
						  size_t count)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];
	struct cb_mail_box_d2f set_info;
	int threshold;
	int ret;

	if (count == 0)
		return 0;

	ret = sscanf(buf, "%d", &threshold);
	if (ret != 1) {
		trans_dbg(tdev, TR_ERR, "core: %s ret=%d, threshold:%d\n",
			  __func__, ret, threshold);
		return -1;
	}

	if ((threshold < 0) || (threshold > 125)) {
		trans_dbg(tdev, TR_ERR,
			"core: %s, thereshold range is [0 - 125].\n", __func__);
		return -1;
	}
	trans_dbg(tdev, TR_DBG, "core: %s,threshold:%d\n", __func__, threshold);

	set_info.event_ID = THERMAL_HIGH_THRESHOLD_MODIFY;
	set_info.param[0] = threshold;
	ret = send_info_to_zsp(thwm, &set_info);

	return ret ? -1 : count;
}

static ssize_t temp_threshold_upper_c_show(struct device *dev,
						  struct device_attribute *attr,
						  char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->threshold.thermal_high);
}

static ssize_t ddr_ecc_threshold_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf,
					       size_t count)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];
	struct cb_mail_box_d2f set_info;
	int threshold;
	int ret;

	if (count == 0)
		return 0;

	ret = sscanf(buf, "%d", &threshold);
	if (ret != 1) {
		trans_dbg(tdev, TR_ERR, "core: %s ret=%d, threshold:%d\n",
			  __func__, ret, threshold);
		return -1;
	}

	trans_dbg(tdev, TR_DBG, "core: %s,threshold:%d\n", __func__, threshold);

	set_info.event_ID = DDR_ECC_CNT_THRESHOLD_MODIFY;
	set_info.param[0] = threshold;
	ret = send_info_to_zsp(thwm, &set_info);

	return ret ? -1 : count;
}

static ssize_t ddr_ecc_threshold_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->threshold.ddr_ecc);
}

static ssize_t sram_ecc_threshold_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf,
						size_t count)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];
	struct cb_mail_box_d2f set_info;
	int threshold;
	int ret;

	if (count == 0)
		return 0;

	ret = sscanf(buf, "%d", &threshold);
	if (ret != 1) {
		trans_dbg(tdev, TR_ERR, "core: %s ret=%d, threshold:%d\n",
			  __func__, ret, threshold);
		return -1;
	}

	trans_dbg(tdev, TR_DBG, "core: %s,threshold:%d\n", __func__, threshold);

	set_info.event_ID = SRAM_ECC_CNT_THRESHOLD_MODIFY;
	set_info.param[0] = threshold;
	ret = send_info_to_zsp(thwm, &set_info);

	return ret ? -1 : count;
}

static ssize_t sram_ecc_threshold_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->threshold.sram_ecc);
}

static ssize_t zsp_ver_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];
	char ver[5] = "";

	fw_info = get_info_from_zsp(thwm);
	memcpy(ver, fw_info->fw_ver.zsp_fw, 4);
	return sprintf(buf, "%s\n", ver);
}

static ssize_t pcie_phy_ver_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];
	char ver[5] = "";

	fw_info = get_info_from_zsp(thwm);
	memcpy(ver, fw_info->fw_ver.pcie_phy_fw, 4);
	return sprintf(buf, "%s\n", ver);
}

static ssize_t rompatch_ver_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];
	char ver[5] = "";

	fw_info = get_info_from_zsp(thwm);
	memcpy(ver, fw_info->fw_ver.rom_patch, 4);
	return sprintf(buf, "%s\n", ver);
}

static ssize_t ddr_ver_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];
	char ver[5] = "";

	fw_info = get_info_from_zsp(thwm);
	memcpy(ver, fw_info->fw_ver.ddr_1d_fw, 4);
	return sprintf(buf, "%s\n", ver);
}

static ssize_t ddrbw_s0_axi_r_MBps_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	unsigned long count, timer, ddr_bw;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	if (fw_info->ddr_bw_info.ddr0_timer_cnt == 0) {
		trans_dbg(thwm->tdev, TR_ERR, "hwm: ddr timer counter0:0.\n");
		ddr_bw = 0;
	} else {
		timer = fw_info->ddr_bw_info.ddr0_timer_cnt;
		count = fw_info->ddr_bw_info.ddr0_rd_cnt;
		ddr_bw = (count * DDR_CTRL_FREQ) / timer;
	}

	return sprintf(buf, "%ld\n", ddr_bw);
}

static ssize_t ddrbw_s0_axi_w_MBps_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	unsigned long count, timer, ddr_bw;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	if (fw_info->ddr_bw_info.ddr0_timer_cnt == 0) {
		trans_dbg(thwm->tdev, TR_ERR, "hwm: ddr timer counter0:0.\n");
		ddr_bw = 0;
	} else {
		timer = fw_info->ddr_bw_info.ddr0_timer_cnt;
		count = fw_info->ddr_bw_info.ddr0_wr_cnt;
		ddr_bw = (count * DDR_CTRL_FREQ) / timer;
	}

	return sprintf(buf, "%ld\n", ddr_bw);
}

static ssize_t ddrbw_s0_dfi_r_MBps_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	unsigned long count, timer, ddr_bw;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	if (fw_info->ddr_bw_info.ddr0_timer_cnt == 0) {
		trans_dbg(thwm->tdev, TR_ERR, "hwm: ddr timer counter0:0.\n");
		ddr_bw = 0;
	} else {
		timer = fw_info->ddr_bw_info.ddr0_timer_cnt;
		count = fw_info->ddr_bw_info.ddr0_rd_dfi_cnt;
		ddr_bw = (count * DDR_CTRL_FREQ * 32) / timer;
	}

	return sprintf(buf, "%ld\n", ddr_bw);
}

static ssize_t ddrbw_s0_dfi_w_MBps_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	unsigned long count, timer, ddr_bw;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	if (fw_info->ddr_bw_info.ddr0_timer_cnt == 0) {
		trans_dbg(thwm->tdev, TR_ERR, "hwm: ddr timer counter0:0.\n");
		ddr_bw = 0;
	} else {
		timer = fw_info->ddr_bw_info.ddr0_timer_cnt;
		count = fw_info->ddr_bw_info.ddr0_wr_dfi_cnt;
		ddr_bw = (count * DDR_CTRL_FREQ * 32) / timer;
	}

	return sprintf(buf, "%ld\n", ddr_bw);
}

static ssize_t ddrbw_s1_axi_r_MBps_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	unsigned long count, timer, ddr_bw;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	if (fw_info->ddr_bw_info.ddr1_timer_cnt == 0) {
		trans_dbg(thwm->tdev, TR_ERR, "hwm: ddr timer counter1:0.\n");
		ddr_bw = 0;
	} else {
		timer = fw_info->ddr_bw_info.ddr1_timer_cnt;
		count = fw_info->ddr_bw_info.ddr1_rd_cnt;
		ddr_bw = (count * DDR_CTRL_FREQ) / timer;
	}

	return sprintf(buf, "%ld\n", ddr_bw);
}

static ssize_t ddrbw_s1_axi_w_MBps_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	unsigned long count, timer, ddr_bw;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	if (fw_info->ddr_bw_info.ddr1_timer_cnt == 0) {
		trans_dbg(thwm->tdev, TR_ERR, "hwm: ddr timer counter1:0.\n");
		ddr_bw = 0;
	} else {
		timer = fw_info->ddr_bw_info.ddr1_timer_cnt;
		count = fw_info->ddr_bw_info.ddr1_wr_cnt;
		ddr_bw = (count * DDR_CTRL_FREQ) / timer;
	}

	return sprintf(buf, "%ld\n", ddr_bw);
}

static ssize_t ddrbw_s1_dfi_r_MBps_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	unsigned long count, timer, ddr_bw;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	if (fw_info->ddr_bw_info.ddr1_timer_cnt == 0) {
		trans_dbg(thwm->tdev, TR_ERR, "hwm: ddr timer counter1:0.\n");
		ddr_bw = 0;
	} else {
		timer = fw_info->ddr_bw_info.ddr1_timer_cnt;
		count = fw_info->ddr_bw_info.ddr1_rd_dfi_cnt;
		ddr_bw = (count * DDR_CTRL_FREQ * 32) / timer;
	}

	return sprintf(buf, "%ld\n", ddr_bw);
}

static ssize_t ddrbw_s1_dfi_w_MBps_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	unsigned long count, timer, ddr_bw;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	if (fw_info->ddr_bw_info.ddr1_timer_cnt == 0) {
		trans_dbg(thwm->tdev, TR_ERR, "hwm: ddr timer counter0:0.\n");
		ddr_bw = 0;
	} else {
		timer = fw_info->ddr_bw_info.ddr1_timer_cnt;
		count = fw_info->ddr_bw_info.ddr1_wr_dfi_cnt;
		ddr_bw = (count * DDR_CTRL_FREQ * 32) / timer;
	}

	return sprintf(buf, "%ld\n", ddr_bw);
}

static ssize_t dram_ecc_ce_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->ce_cnt.ddr_ecc_cnt);
}

static ssize_t sram_ecc_ce_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->ce_cnt.sram_ecc_cnt);
}

static ssize_t pcie_ce_aer_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->ce_cnt.pcie_err_cnt);
}

static ssize_t dram_ecc_uce_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->uce_cnt.ddr_ecc_cnt);
}

static ssize_t sram_ecc_uce_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->uce_cnt.sram_ecc_cnt);
}

static ssize_t pcie_uce_fatal_aer_show(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->uce_cnt.pcie_uncor_fatal_cnt);
}

static ssize_t pcie_uce_unfatal_aer_show(struct device *dev,
						 struct device_attribute *attr,
						 char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->uce_cnt.pcie_uncor_unfatal_cnt);
}

static ssize_t pvt0_mv_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->voltage.voltage[0]);
}

static ssize_t pvt1_mv_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->voltage.voltage[1]);
}

static ssize_t pvt2_mv_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->voltage.voltage[2]);
}

static ssize_t pvt3_mv_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct cb_mail_box_f2d *fw_info;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	fw_info = get_info_from_zsp(thwm);
	return sprintf(buf, "%d\n", fw_info->voltage.voltage[3]);
}

static ssize_t reduce_strategy_store(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf,
					    size_t count)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	u32 reduce;

	if (count == 0)
		return 0;

	if (sscanf(buf, "%d", &reduce) != 1) {
		trans_dbg(tdev, TR_ERR, "core: not in hex or decimal form.\n");
		return count;
	}
	if (reduce <= 2) {
		tdev->reduce_strategy = reduce;
	} else {
		trans_dbg(tdev, TR_ERR, "core: reduce:%d is invalid\n", reduce);
		return -1;
	}

	return count;
}

static ssize_t reduce_strategy_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;

	return sprintf(buf, "0:75%%; 1:50%%; 2:25%%.\ncurrent:%d\n",
		tdev->reduce_strategy);
}
static DEVICE_ATTR_RW(reduce_strategy);

static DEVICE_ATTR_RO(temp_sensor_c);
static DEVICE_ATTR_RO(max_temp_c);
static DEVICE_ATTR_RO(throttling_time_s);
static DEVICE_ATTR_RO(throttling_status);
static DEVICE_ATTR_RO(chip_type);
static DEVICE_ATTR_RO(chip_rev);
static DEVICE_ATTR_RO(chip_id);
static DEVICE_ATTR_RO(power_state);
static DEVICE_ATTR_RO(secure_keys);
static DEVICE_ATTR_RO(active_key);
static DEVICE_ATTR_RO(uptime_s);
static DEVICE_ATTR_RW(temp_threshold_lower_c);
static DEVICE_ATTR_RW(temp_threshold_upper_c);
static DEVICE_ATTR_RW(ddr_ecc_threshold);
static DEVICE_ATTR_RW(sram_ecc_threshold);
static DEVICE_ATTR_RO(zsp_ver);
static DEVICE_ATTR_RO(pcie_phy_ver);
static DEVICE_ATTR_RO(rompatch_ver);
static DEVICE_ATTR_RO(ddr_ver);
static DEVICE_ATTR_RO(ddrbw_s0_axi_r_MBps);
static DEVICE_ATTR_RO(ddrbw_s0_axi_w_MBps);
static DEVICE_ATTR_RO(ddrbw_s0_dfi_r_MBps);
static DEVICE_ATTR_RO(ddrbw_s0_dfi_w_MBps);
static DEVICE_ATTR_RO(ddrbw_s1_axi_r_MBps);
static DEVICE_ATTR_RO(ddrbw_s1_axi_w_MBps);
static DEVICE_ATTR_RO(ddrbw_s1_dfi_r_MBps);
static DEVICE_ATTR_RO(ddrbw_s1_dfi_w_MBps);
static DEVICE_ATTR_RO(dram_ecc_ce);
static DEVICE_ATTR_RO(sram_ecc_ce);
static DEVICE_ATTR_RO(pcie_ce_aer);
static DEVICE_ATTR_RO(dram_ecc_uce);
static DEVICE_ATTR_RO(sram_ecc_uce);
static DEVICE_ATTR_RO(pcie_uce_fatal_aer);
static DEVICE_ATTR_RO(pcie_uce_unfatal_aer);
static DEVICE_ATTR_RO(pvt0_mv);
static DEVICE_ATTR_RO(pvt1_mv);
static DEVICE_ATTR_RO(pvt2_mv);
static DEVICE_ATTR_RO(pvt3_mv);


static struct attribute *transzsp_sysfs_entries[] = {
	&dev_attr_temp_sensor_c.attr,
	&dev_attr_max_temp_c.attr,
	&dev_attr_throttling_time_s.attr,
	&dev_attr_throttling_status.attr,
	&dev_attr_chip_type.attr,
	&dev_attr_chip_rev.attr,
	&dev_attr_chip_id.attr,
	&dev_attr_power_state.attr,
	&dev_attr_secure_keys.attr,
	&dev_attr_active_key.attr,
	&dev_attr_uptime_s.attr,
	&dev_attr_temp_threshold_lower_c.attr,
	&dev_attr_temp_threshold_upper_c.attr,
	&dev_attr_ddr_ecc_threshold.attr,
	&dev_attr_sram_ecc_threshold.attr,
	&dev_attr_zsp_ver.attr,
	&dev_attr_pcie_phy_ver.attr,
	&dev_attr_rompatch_ver.attr,
	&dev_attr_ddr_ver.attr,
	&dev_attr_ddrbw_s0_axi_r_MBps.attr,
	&dev_attr_ddrbw_s0_axi_w_MBps.attr,
	&dev_attr_ddrbw_s0_dfi_r_MBps.attr,
	&dev_attr_ddrbw_s0_dfi_w_MBps.attr,
	&dev_attr_ddrbw_s1_axi_r_MBps.attr,
	&dev_attr_ddrbw_s1_axi_w_MBps.attr,
	&dev_attr_ddrbw_s1_dfi_r_MBps.attr,
	&dev_attr_ddrbw_s1_dfi_w_MBps.attr,
	&dev_attr_dram_ecc_ce.attr,
	&dev_attr_sram_ecc_ce.attr,
	&dev_attr_pcie_ce_aer.attr,
	&dev_attr_dram_ecc_uce.attr,
	&dev_attr_sram_ecc_uce.attr,
	&dev_attr_pcie_uce_fatal_aer.attr,
	&dev_attr_pcie_uce_unfatal_aer.attr,
	&dev_attr_pvt0_mv.attr,
	&dev_attr_pvt1_mv.attr,
	&dev_attr_pvt2_mv.attr,
	&dev_attr_pvt3_mv.attr,
	&dev_attr_reduce_strategy.attr,
	NULL
};

static struct attribute_group transzsp_attribute_group = {
	.name = NULL,
	.attrs = transzsp_sysfs_entries,
};

irqreturn_t hw_monitor_isr(int index, void *data)
{
	struct hwm_t *thwm;
	struct cb_mail_box_f2d *fw_info;
	unsigned int event_ID;
	unsigned long error_ID;
	struct cb_tranx_t *tdev = data;

	thwm = tdev->modules[TR_MODULE_HW_MONITOR];
	fw_info = get_info_from_zsp(thwm);

	if (spin_trylock(&thwm->owner_lock) == 0)
		return IRQ_NONE;

	fw_info->process_status = HOST_STATUS_BUSY;
	/* clear interrupt status*/
	ccm_write(tdev, ZSP_SFT_INT_CON_STUS, 0x0);
	event_ID = fw_info->event_ID;
	error_ID = fw_info->error_ID;
	fw_info->event_ID = 0;
	fw_info->error_ID = 0;
	fw_info->process_status = HOST_STATUS_FREE;
	if (event_ID == HEART_BEAT_EVENT) /* heart beat event, nothing to do.*/
		thwm->heartbeat_live += 1;
	else if (event_ID == ERROR_EVENT) {
		thwm->heartbeat_live += 1;
		if (error_ID != 0)
			parse_err_id(error_ID, thwm);
	}
	spin_unlock(&thwm->owner_lock);

	return IRQ_HANDLED;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))
static void heartbeat_timer_isr(unsigned long data)
{
	struct hwm_t *thwm = (struct hwm_t *)data;
#else
static void heartbeat_timer_isr(struct timer_list *t)
{
	struct hwm_t *thwm = from_timer(thwm, t, hb_timer);
#endif
	if (hb == 2) {
		if (thwm->heartbeat_live)
			thwm->heartbeat_live = 0;
		else
			trans_dbg(thwm->tdev, TR_ERR,
				"hwm: has not heartbeat more than %ds.\n",
				HEARTBEAT_TIMEOUT);
		mod_timer(&thwm->hb_timer, jiffies + HEARTBEAT_TIMEOUT * HZ);
	}
}

int hw_monitor_register_irq(struct cb_tranx_t *tdev)
{
	int ret;
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	trans_dbg(thwm->tdev, TR_DBG, "hwm: IRQ is %d!\n", thwm->irq);
	ret = request_irq(thwm->irq, unify_isr, IRQF_SHARED|IRQF_NO_THREAD, "hw_monitor", tdev);
	if (ret)
		trans_dbg(tdev, TR_ERR, "hwm: request zsp irq failed.\n");

	return ret ? -EFAULT : 0;
}

static void hw_monitor_free_irq(struct cb_tranx_t *tdev)
{
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	/* free the IRQ */
	if (thwm->irq != 0) {
		trans_dbg(thwm->tdev, TR_DBG, "hwm: free irq:%d\n", thwm->irq);
		free_irq(thwm->irq, (void *)tdev);
	}
}

int hw_monitor_init(struct cb_tranx_t *tdev)
{
	int ret = 0;
	struct hwm_t *thwm;

	thwm = kzalloc(sizeof(struct hwm_t), GFP_KERNEL);
	if (!thwm) {
		trans_dbg(tdev, TR_ERR, "hwm: alloc hwm_t failed.\n");
		goto out;
	}
	tdev->modules[TR_MODULE_HW_MONITOR] = thwm;
	thwm->tdev = tdev;

	ret = sysfs_create_group(&tdev->misc_dev->this_device->kobj,
				 &transzsp_attribute_group);
	if (ret) {
		trans_dbg(tdev, TR_ERR,
			"hwm: failed to create sysfs device attributes.\n");
		goto out_free_zsp;
	}
	spin_lock_init(&thwm->owner_lock);
	thwm->hb_timer.expires = jiffies + HEARTBEAT_TIMEOUT * HZ;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0))
	thwm->hb_timer.function = (void *)heartbeat_timer_isr;
	thwm->hb_timer.data = (unsigned long)(thwm);
	init_timer(&thwm->hb_timer);
#else
	timer_setup(&thwm->hb_timer, heartbeat_timer_isr, 0);
#endif
	add_timer(&thwm->hb_timer);

	if (fw == 1) {
		if (load_firmware(thwm))
			goto out_del_timer;
	}

	thwm->irq = tdev->msix_entries[ZSP_IRQ_INDEX].vector;
	if (thwm->irq == 0) {
		trans_dbg(tdev, TR_ERR, "hwm: zsp vector is 0.\n");
		goto out_del_timer;
	}

	if (hw_monitor_register_irq(tdev)) {
		trans_dbg(tdev, TR_ERR, "hwm: register irq failed.\n");
		goto out_del_timer;
	}

	trans_dbg(tdev, TR_INF, "hwm: submodule inserted done.\n");
	return 0;

out_del_timer:
	del_timer_sync(&thwm->hb_timer);
	sysfs_remove_group(&tdev->misc_dev->this_device->kobj,
				&transzsp_attribute_group);
out_free_zsp:
	kfree(thwm);
	trans_dbg(tdev, TR_ERR, "hwm: transzsp probe failed.\n");
out:
	return -EFAULT;
}

int hw_monitor_release(struct cb_tranx_t *tdev)
{
	struct hwm_t *thwm = tdev->modules[TR_MODULE_HW_MONITOR];

	reset_zsp(thwm);
	del_timer_sync(&thwm->hb_timer);
	sysfs_remove_group(&tdev->misc_dev->this_device->kobj,
			   &transzsp_attribute_group);
	hw_monitor_free_irq(tdev);
	kfree(thwm);
	trans_dbg(tdev, TR_DBG, "hwm: remove module done.\n");

	return 0;
}
