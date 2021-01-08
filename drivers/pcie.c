// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 VeriSilicon Holdings Co., Ltd.
 *
 * This is pcie configuration driver for Linux.
 */

#include <linux/io.h>
#include <linux/pci.h>
#include <linux/errno.h>
#include <linux/pagemap.h>
#include <linux/aer.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>

#include "common.h"
#include "pcie.h"
#include "transcoder.h"

/* ATU_CAP Registers */
#define IATU_BASE_OFF			0x8100
#define IATU_REGION_CTRL_1_OFF(i)	(IATU_BASE_OFF+0x200*(i)+0x00)
#define IATU_REGION_CTRL_2_OFF(i)	(IATU_BASE_OFF+0x200*(i)+0x04)
#define IATU_LWR_BASE_ADDR_OFF(i)	(IATU_BASE_OFF+0x200*(i)+0x08)
#define IATU_UPPER_BASE_ADDR_OFF(i)	(IATU_BASE_OFF+0x200*(i)+0x0c)
#define IATU_LIMIT_ADDR_OFF(i)		(IATU_BASE_OFF+0x200*(i)+0x10)
#define IATU_LWR_TARGET_ADDR_OFF(i)	(IATU_BASE_OFF+0x200*(i)+0x14)
#define IATU_UPPER_TARGET_ADDR_OFF(i)	(IATU_BASE_OFF+0x200*(i)+0x18)

#ifndef EMULATOR
#ifdef __x86_64__
#define ENABLE_REPORT
#endif
#endif

#define BAR_PCIE_MAPPING_SIZE		0x80000
#define REGION_ENABLE			(1<<31)

/* PCIe AXI Master View  IP controllers high 32bits address. */
#define IPS_H_ADDR			0x1

/* PCIe AXI Master View two slices DDR (low and high 32bits)address. */
#define S0_DDR_L_ADDR			0x4000000
#define S0_DDR_H_ADDR			0x0
#define S1_DDR_L_ADDR			0x84000000
#define S1_DDR_H_ADDR			0x0

/*  PCIe AXI Master View pcie,ddr and other ips controller offset */
#define PCIE_CON_BASE_L_ADDR		0x0
#define DDR0_CON_BASE_L_ADDR		0x2000000
#define DDR1_CON_BASE_L_ADDR		0x4000000
#define IPS_CON_BASE_L_ADDR		0x6000000

/* offset address base on region2: pcie - noc. */
#define DDR_CON_OFF			0x1100000
#define OTHER_IPS_CON_OFF		0x3100000
#define PCIE_MAPPING_END_OFF		0x3f00000

#define CCM_ADDR_OFF			0x400000
#define CCM_MAP_SIZE			0x30000

#ifdef __x86_64__
#define MSI_NUM				32
#else
#define MSI_NUM				30
#endif

static inline void pcie_write(struct cb_tranx_t *tdev,
				 unsigned int off,
				 unsigned int val)
{
	writel((val), ((off)+tdev->bar0_virt));
}

static inline unsigned int pcie_read(struct cb_tranx_t *tdev,
					unsigned int off)
{
	return readl((off)+(tdev->bar0_virt));
}

static ssize_t link_status_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	u32 val;
	int pos = 0;
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct pci_dev *pcie_dev = tdev->pdev;

	pci_read_config_dword(pcie_dev, 0x80, &val);

	if (((val & 0xF0000) >> 16) == 1)
		pos += sprintf(buf + pos, "Speed 2.5GT/s, ");
	else if (((val & 0xF0000) >> 16) == 2)
		pos += sprintf(buf + pos, "Speed 5GT/s, ");
	else if (((val & 0xF0000) >> 16) == 3)
		pos += sprintf(buf + pos, "Speed 8GT/s, ");
	else
		pos += sprintf(buf + pos, "Speed failed, ");

	if (((val & 0x3F00000) >> 20) == 1)
		pos += sprintf(buf + pos, "Width x1\n");
	else if (((val & 0x3F00000) >> 20) == 2)
		pos += sprintf(buf + pos, "Width x2\n");
	else if (((val & 0x3F00000) >> 20) == 4)
		pos += sprintf(buf + pos, "Width x4\n");
	else
		pos += sprintf(buf + pos, "Width Failed!\n ");

	return pos;
}

static ssize_t bus_id_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
	struct cb_tranx_t *tdev = mtdev->tdev;
	struct pci_dev *pcie_dev = tdev->pdev;

	return sprintf(buf, "%04x:%02x:%02x.%d\n",
			pci_domain_nr(pcie_dev->bus), pcie_dev->bus->number,
			PCI_SLOT(pcie_dev->devfn), PCI_FUNC(pcie_dev->devfn));
}

static DEVICE_ATTR_RO(link_status);
static DEVICE_ATTR_RO(bus_id);

static struct attribute *trans_pcie_sysfs_entries[] = {
	&dev_attr_link_status.attr,
	&dev_attr_bus_id.attr,
	NULL
};

static struct attribute_group trans_pcie_attribute_group = {
	.name = NULL,
	.attrs = trans_pcie_sysfs_entries,
};


/*
 * enable pcie, get pcie region address and size,
 * config region mapping, request msix number.
 * PCIe AXI Master View Memory Space - Offset Address Based On BAR2
 *		------------------                  ------------------
 *		|0x1_0000_0000     |                |0x0000_0000     |
 *		|      ...         |        -       |      ...       |
 *		|0x1_0110_0000-1   |                |0x0110_0000-1   |
 *		------------------                  ------------------
 *
 *		------------------                  ------------------
 *		|0x1_0200_0000     |                |0x0110_0000     |
 *		|      ...         |        -       |      ...       |
 *		|0x1_0400_0000-1   |                |0x0310_0000-1   |
 *		------------------                  ------------------
 *
 *		------------------                  ------------------
 *		|0x1_0600_0000     |                |0x0310_0000     |
 *		|      ...         |        -       |      ...       |
 *		|0x1_06E0_0000-1   |                |0x03F0_0000-1   |
 *		------------------                  ------------------
 */
int cb_pci_init(struct cb_tranx_t *tdev)
{
	int ret, i;
	resource_size_t bar4_base, bar2_base, bar0_base;
	u32 bar4_len, bar2_len;

	ret = pci_enable_device(tdev->pdev);
	if (ret) {
		trans_dbg(tdev, TR_ERR,
			"pcie: Unable to Enable PCIe device, ret:%d.\n", ret);
		goto out;
	}

	pci_set_master(tdev->pdev);
	if (pci_set_dma_mask(tdev->pdev, DMA_BIT_MASK(64))
		|| pci_set_consistent_dma_mask(tdev->pdev,
					DMA_BIT_MASK(64))) {
		trans_dbg(tdev, TR_ERR, "pcie: Set pci dma mask 64 failed.\n");
		if (pci_set_dma_mask(tdev->pdev, DMA_BIT_MASK(32))
		|| pci_set_consistent_dma_mask(tdev->pdev, DMA_BIT_MASK(32))) {
			trans_dbg(tdev, TR_ERR,
				"pcie: No suitable DMA available.\n");
			goto out_disable_pci;
		}
	}
#ifdef ENABLE_REPORT
	ret = pci_enable_pcie_error_reporting(tdev->pdev);
	if (ret) {
		trans_dbg(tdev, TR_ERR, "pcie: PCIe error reporting failed.\n");
		goto out_clear_master;
	}
#endif

	/* get every region info */
	bar0_base = pci_resource_start(tdev->pdev, 0);
	bar2_base = pci_resource_start(tdev->pdev, 2);
	bar2_len = pci_resource_len(tdev->pdev, 2);
	bar4_base = pci_resource_start(tdev->pdev, 4);
	bar4_len = pci_resource_len(tdev->pdev, 4);

	tdev->bar0_virt = ioremap_nocache(bar0_base, BAR_PCIE_MAPPING_SIZE);
	if (!tdev->bar0_virt) {
		trans_dbg(tdev, TR_ERR, "pcie: ioremap bar0 failed.\n");
		goto out_disable_err_report;
	}
	/* map hardware to bar2(region 2) */
	pcie_write(tdev, IATU_REGION_CTRL_2_OFF(0), 0x0);
	pcie_write(tdev, IATU_REGION_CTRL_1_OFF(0), 0x0);
	pcie_write(tdev, IATU_LWR_BASE_ADDR_OFF(0), QWORD_LO(bar2_base));
	pcie_write(tdev, IATU_UPPER_BASE_ADDR_OFF(0), QWORD_HI(bar2_base));
	pcie_write(tdev, IATU_LIMIT_ADDR_OFF(0),
		   (QWORD_LO(bar2_base) + DDR_CON_OFF - 1));
	pcie_write(tdev, IATU_LWR_TARGET_ADDR_OFF(0), PCIE_CON_BASE_L_ADDR);
	pcie_write(tdev, IATU_UPPER_TARGET_ADDR_OFF(0), IPS_H_ADDR);
	pcie_write(tdev, IATU_REGION_CTRL_2_OFF(0), REGION_ENABLE);

	pcie_write(tdev, IATU_REGION_CTRL_2_OFF(1), 0x0);
	pcie_write(tdev, IATU_REGION_CTRL_1_OFF(1), 0x0);
	pcie_write(tdev, IATU_LWR_BASE_ADDR_OFF(1),
		   QWORD_LO(bar2_base) + DDR_CON_OFF);
	pcie_write(tdev, IATU_UPPER_BASE_ADDR_OFF(1), QWORD_HI(bar2_base));
	pcie_write(tdev, IATU_LIMIT_ADDR_OFF(1),
		   (QWORD_LO(bar2_base) + OTHER_IPS_CON_OFF - 1));
	pcie_write(tdev, IATU_LWR_TARGET_ADDR_OFF(1), DDR0_CON_BASE_L_ADDR);
	pcie_write(tdev, IATU_UPPER_TARGET_ADDR_OFF(1), IPS_H_ADDR);
	pcie_write(tdev, IATU_REGION_CTRL_2_OFF(1), REGION_ENABLE);

	pcie_write(tdev, IATU_REGION_CTRL_2_OFF(2), 0x0);
	pcie_write(tdev, IATU_REGION_CTRL_1_OFF(2), 0x0);
	pcie_write(tdev, IATU_LWR_BASE_ADDR_OFF(2),
		   QWORD_LO(bar2_base) + OTHER_IPS_CON_OFF);
	pcie_write(tdev, IATU_UPPER_BASE_ADDR_OFF(2), QWORD_HI(bar2_base));
	pcie_write(tdev, IATU_LIMIT_ADDR_OFF(2),
		   (QWORD_LO(bar2_base) + PCIE_MAPPING_END_OFF - 1));
	pcie_write(tdev, IATU_LWR_TARGET_ADDR_OFF(2), IPS_CON_BASE_L_ADDR);
	pcie_write(tdev, IATU_UPPER_TARGET_ADDR_OFF(2), IPS_H_ADDR);
	pcie_write(tdev, IATU_REGION_CTRL_2_OFF(2), REGION_ENABLE);

	/* only map first 64MiB in every slice.
	 * map S1_ddr to bar4(region 4):
	 * ep_ddr(0x4000000  -- tdev->len_ddr/2)
	 * to rc(bar4+0 -- bar4+tdev->len_ddr/2-1)
	 */
	pcie_write(tdev, IATU_REGION_CTRL_1_OFF(6), 0x0);
	pcie_write(tdev, IATU_LWR_BASE_ADDR_OFF(6), QWORD_LO(bar4_base));
	pcie_write(tdev, IATU_UPPER_BASE_ADDR_OFF(6), QWORD_HI(bar4_base));
	pcie_write(tdev, IATU_LIMIT_ADDR_OFF(6),
		   (QWORD_LO(bar4_base) + (bar4_len >> 1)-1));
	pcie_write(tdev, IATU_LWR_TARGET_ADDR_OFF(6), S0_DDR_L_ADDR);
	pcie_write(tdev, IATU_UPPER_TARGET_ADDR_OFF(6), S0_DDR_H_ADDR);
	pcie_write(tdev, IATU_REGION_CTRL_2_OFF(6), REGION_ENABLE);

	/*
	 * map S2_ddr to bar4(region 2):
	 * ep_ddr(0x84000000  -- tdev->len_ddr/2)
	 * to rc(bar4+tdev->len_ddr/2 -- bar4+tdev->len_ddr-1)
	 */
	pcie_write(tdev, IATU_REGION_CTRL_1_OFF(7), 0x0);
	pcie_write(tdev, IATU_LWR_BASE_ADDR_OFF(7),
		   (QWORD_LO(bar4_base) + (bar4_len >> 1)));
	pcie_write(tdev, IATU_UPPER_BASE_ADDR_OFF(7), QWORD_HI(bar4_base));
	pcie_write(tdev, IATU_LIMIT_ADDR_OFF(7),
		   (QWORD_LO(bar4_base) + bar4_len - 1));
	pcie_write(tdev, IATU_LWR_TARGET_ADDR_OFF(7), S1_DDR_L_ADDR);
	pcie_write(tdev, IATU_UPPER_TARGET_ADDR_OFF(7), S1_DDR_H_ADDR);
	pcie_write(tdev, IATU_REGION_CTRL_2_OFF(7), REGION_ENABLE);

	tdev->bar2_virt = ioremap_nocache(bar2_base, bar2_len);
	if (!tdev->bar2_virt) {
		trans_dbg(tdev, TR_ERR, "pcie: failed to ioremap bar2.\n");
		goto out_unmap_pcie;
	}
	tdev->ccm = tdev->bar2_virt + CCM_ADDR_OFF;
	tdev->bar2_virt_end = tdev->bar2_virt + bar2_len;

	for (i = 0; i < MAX_MSIX_CNT; i++)
		tdev->msix_entries[i].entry = i;

	ret = pci_enable_msix_range(tdev->pdev, tdev->msix_entries,
					MIN_MSIX_CNT, MAX_MSIX_CNT);
	if (ret >= MIN_MSIX_CNT) {
		trans_dbg(tdev, TR_DBG,
			"pcie: Have requested msi irq number:%d.\n", ret);
	} else {
		pci_disable_msix(tdev->pdev);
		trans_dbg(tdev, TR_ERR,
			"pcie: allocate irq vectors failed,ret=%d.\n",
			ret);
		goto out_unmap_bar2;
	}

	ret = sysfs_create_group(&tdev->misc_dev->this_device->kobj,
				 &trans_pcie_attribute_group);
	if (ret) {
		trans_dbg(tdev, TR_ERR,
			"pcie: failed to create sysfs device attributes\n");
		goto out_unmap_bar2;
	}

	/*
	 * enable interrupt data path
	 * enable all interrupt path except edma,edma not use glue logic.
	 */
	ccm_write(tdev, INT_POL_REF_CFG, GLUE_LGIC_INTE);
	trans_dbg(tdev, TR_DBG, "pcie: INT_POL_REF_CFG:0x%x.\n",
		  ccm_read(tdev, INT_POL_REF_CFG));

	/* enable: zsp_sft(2),vcd(7.9.8.10),vce(11.12),bigsea(15.16) */
	ccm_write(tdev, MAIL_BOX_INTERRUPT_CONTROLLER, GLUE_LGIC_INTE);
	trans_dbg(tdev, TR_DBG, "pcie: MAIL_BOX_INTERRUPT_CONTROLLER:0x%x.\n",
		  ccm_read(tdev, MAIL_BOX_INTERRUPT_CONTROLLER));
	return 0;

out_unmap_bar2:
	iounmap(tdev->bar2_virt);
out_unmap_pcie:
	iounmap(tdev->bar0_virt);
out_disable_err_report:
#ifdef ENABLE_REPORT
	pci_disable_pcie_error_reporting(tdev->pdev);
#endif
out_clear_master:
	pci_clear_master(tdev->pdev);
out_disable_pci:
	pci_disable_device(tdev->pdev);
out:
	return -EFAULT;
}

static int switch_ddr_mapping(struct cb_tranx_t *tdev,
				    int ddr_id)
{
	unsigned int ddr_base_l;
	resource_size_t bar2_base;

	/* ddr0:0x0200_0000      ddr1:0x0400_0000 */
	if (ddr_id == 0)
		ddr_base_l = DDR0_CON_BASE_L_ADDR;
	else if (ddr_id == 1)
		ddr_base_l = DDR1_CON_BASE_L_ADDR;
	else
		return -EFAULT;

	bar2_base = pci_resource_start(tdev->pdev, 2);
	pcie_write(tdev, IATU_REGION_CTRL_2_OFF(1), 0x0);
	pcie_write(tdev, IATU_REGION_CTRL_1_OFF(1), 0x0);
	pcie_write(tdev, IATU_LWR_BASE_ADDR_OFF(1),
		QWORD_LO(bar2_base) + DDR_CON_OFF);
	pcie_write(tdev, IATU_UPPER_BASE_ADDR_OFF(1), QWORD_HI(bar2_base));
	pcie_write(tdev, IATU_LIMIT_ADDR_OFF(1),
		(QWORD_LO(bar2_base) + OTHER_IPS_CON_OFF - 1));
	pcie_write(tdev, IATU_LWR_TARGET_ADDR_OFF(1), ddr_base_l);
	pcie_write(tdev, IATU_UPPER_TARGET_ADDR_OFF(1), IPS_H_ADDR);
	pcie_write(tdev, IATU_REGION_CTRL_2_OFF(1), REGION_ENABLE);

	return 0;
}

long cb_pci_ioctl(struct file *filp,
		      unsigned int cmd,
		      unsigned long arg,
		      struct cb_tranx_t *tdev)
{
	long ret = 0;
	void __user *argp = (void __user *)arg;
	unsigned int ddr_id;
	struct bar_info bar_inf;

	switch (cmd) {
	case CB_TRANX_GET_BARADDR:
		if (copy_from_user(&bar_inf, argp, sizeof(bar_inf))) {
			trans_dbg(tdev, TR_ERR,
				"pcie: GET_BARADDR copy_from_user failed.\n");
			return -EFAULT;
		}

		if (bar_inf.bar_num == 0) {
			bar_inf.bar_addr = pci_resource_start(tdev->pdev, 0);
			bar_inf.bar_size = pci_resource_len(tdev->pdev, 0);
		} else if (bar_inf.bar_num == 2) {
			bar_inf.bar_addr = pci_resource_start(tdev->pdev, 2);
			bar_inf.bar_size = pci_resource_start(tdev->pdev, 2);
		} else if (bar_inf.bar_num == 4) {
			bar_inf.bar_addr = pci_resource_start(tdev->pdev, 4);
			bar_inf.bar_size = pci_resource_start(tdev->pdev, 4);
		}
		if (copy_to_user(argp, &bar_inf, sizeof(bar_inf))) {
			trans_dbg(tdev, TR_ERR,
				"pcie: GET_BARADDR copy_to_user failed.\n");
			return -EFAULT;
		}
		break;
	case CB_TRANX_SW_DDR_MAPPING:
		__get_user(ddr_id, (unsigned int *)arg);
		if (ddr_id > 1) {
			trans_dbg(tdev, TR_ERR,
				"pcie: SW_DDR_MAPPING, ddr_id:%d is error.\n",
				ddr_id);
			return -EFAULT;
		}
		trans_dbg(tdev, TR_DBG, "pcie: ddr id:%d.\n", ddr_id);
		ret = switch_ddr_mapping(tdev, ddr_id);
		break;
	default:
		trans_dbg(tdev, TR_ERR,
			"pcie: %s, cmd:0x%x is error.\n", __func__, cmd);
		ret = -EINVAL;
	}

	return ret;
}

void cb_pci_release(struct cb_tranx_t *tdev)
{
	/* disable interrupt data path */
	ccm_write(tdev, INT_POL_REF_CFG, 0x0);
	ccm_write(tdev, MAIL_BOX_INTERRUPT_CONTROLLER, 0x0);
	pci_disable_msix(tdev->pdev);

	iounmap(tdev->bar0_virt);
	iounmap(tdev->bar2_virt);
#ifdef ENABLE_REPORT
	pci_disable_pcie_error_reporting(tdev->pdev);
#endif
	pci_disable_device(tdev->pdev);
	pci_clear_master(tdev->pdev);
	sysfs_remove_group(&tdev->misc_dev->this_device->kobj,
			   &trans_pcie_attribute_group);

	trans_dbg(tdev, TR_DBG, "pcie: remove module done.\n");
}
