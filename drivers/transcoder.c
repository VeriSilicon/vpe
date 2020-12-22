// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 VeriSilicon Holdings Co., Ltd.
 *  Author: Fengyin Wu <Fengyin.Wu@verisilicon.com>
 *  Author: Huaidong Mo <Huaidong.Mo@verisilicon.com>
 */

#include <linux/errno.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/pagemap.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/aer.h>

#include "common.h"
#include "encoder.h"
#include "vc8000d.h"
#include "pcie.h"
#include "edma.h"
#include "memory.h"
#include "misc_ip.h"
#include "hw_monitor.h"
#include "transcoder.h"

#define description_string "transcoder driver"
#define version_string "2.50"

#define FB_VENDOR_ID 0x1d9b
#define FB_DEVICE_ID 0xface
#define VSI_VENDOR_ID 0x1eb1
#define VSI_DEVICE_ID 0x1001
#define YX_VENDOR_ID 0x1ebd
#define YX_DEVICE_ID 0x0101

#define DEVICE_CNT 12

struct tdev_private {
    void *data;
    int minor;
    int node_num;
};

static struct tdev_private transcoder_dev[DEVICE_CNT];

unsigned int level = 1;
module_param(level, uint, 0644);
MODULE_PARM_DESC(level, "print level: 3:DBG; 2:NOTICE; 1:INF; 0:ERR; default "
                        "is 2.");

static inline void show_version(void)
{
    pr_info("%s version %s\n", description_string, version_string);
}

static struct cb_tranx_t *get_trans_dev(int minor)
{
    int i;

    for (i = 0; i < DEVICE_CNT; i++) {
        if (minor == transcoder_dev[i].minor)
            break;
    }

    return (i < DEVICE_CNT) ? transcoder_dev[i].data : NULL;
}

long trans_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret            = 0;
    struct inode *inode = file_inode(filp);
    struct cb_tranx_t *tdev;
    unsigned int cid;
    unsigned int val;

    tdev = get_trans_dev(iminor(inode));
    if (!tdev) {
        trans_dbg(tdev, TR_ERR, "core: get_trans_dev failed.\n");
        return -EFAULT;
    }

    if (_IOC_NR(cmd) > TRANS_MAXNR)
        return -ENOTTY;

    cid = _IOC_NR(cmd);
    if ((cid >= IOCTL_CMD_PCIE_MINNR) && (cid <= IOCTL_CMD_PCIE_MAXNR))
        ret = cb_pci_ioctl(filp, cmd, arg, tdev);
    else if ((cid >= IOCTL_CMD_EDMA_MINNR) && (cid <= IOCTL_CMD_EDMA_MAXNR))
        ret = edma_ioctl(filp, cmd, arg, tdev);
    else if (cid == _IOC_NR(CB_TRANX_EDMA_PHY_TRANX))
        ret = edma_ioctl(filp, cmd, arg, tdev);
    else if (cid >= IOCTL_CMD_MEM_MINNR && cid <= IOCTL_CMD_MEM_MAXNR)
        ret = cb_mem_ioctl(filp, cmd, arg, tdev);
    else if (cid >= IOCTL_CMD_VCD_MINNR && cid <= IOCTL_CMD_VCD_MAXNR)
        ret = vc8000d_ioctl(filp, cmd, arg, tdev);
    else if (cid >= IOCTL_CMD_VCE_MINNR && cid <= IOCTL_CMD_VCE_MAXNR)
        ret = vc8000e_ioctl(filp, cmd, arg, tdev);
    else if (cid >= IOCTL_CMD_BIGSEA_MINNR && cid <= IOCTL_CMD_BIGSEA_MAXNR)
        ret = bigsea_ioctl(filp, cmd, arg, tdev);
    else if (cid == _IOC_NR(CB_TRANX_REQUEST_TC)) {
        __get_user(val, (u32 *)arg);
        if (val > 1) {
            trans_dbg(tdev, TR_ERR, "core: request tcache:%d error.\n", val);
            return -EFAULT;
        }

        trans_dbg(tdev, TR_DBG, "core: request tcache:%d.\n", val);
        if (down_interruptible(&tdev->tcache[val].sem))
            return -ERESTARTSYS;
        if (tdev->tcache[val].filp != NULL)
            trans_dbg(tdev, TR_ERR,
                      "core: warning: the filp of tcache_%d should be NULL\n",
                      val);
        tdev->tcache[val].filp = filp;
    } else if (cid == _IOC_NR(CB_TRANX_RELEASE_TC)) {
        __get_user(val, (u32 *)arg);
        if (val > 1) {
            trans_dbg(tdev, TR_ERR, "core: release tcache:%d error.\n", val);
            return -EFAULT;
        }
        trans_dbg(tdev, TR_DBG, "core: release tcache:%d\n", val);
        tdev->tcache[val].filp = NULL;
        up(&tdev->tcache[val].sem);
    } else if (cid >= IOCTL_CMD_MISC_IP_MINNR && cid <= IOCTL_CMD_MISC_IP_MAXNR)
        ret = misc_ip_ioctl(filp, cmd, arg, tdev);
    else
        trans_dbg(tdev, TR_ERR, "core: ioctl cmd:%d is error\n", cid);

    return ret;
}

static int trans_close(struct inode *inode, struct file *filp)
{
    int i;
    struct cb_tranx_t *tdev = get_trans_dev(iminor(inode));

    if (WARN_ON(!tdev))
        return -EFAULT;

    cb_mem_close(tdev, filp);
    bigsea_close(tdev, filp);
    vce_close(tdev, filp);
    vcd_close(tdev, filp);

    for (i = 0; i < 2; i++) {
        if (tdev->tcache[i].filp == filp) {
            trans_dbg(tdev, TR_NOTICE, "core: tcache_%d abnormal release\n", i);
            tdev->tcache[i].filp = NULL;
            up(&tdev->tcache[i].sem);
        }
    }
    return 0;
}

static int trans_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int trans_mmap(struct file *file, struct vm_area_struct *vma)
{
    int ret                 = 0;
    struct inode *inode     = file_inode(file);
    unsigned long size      = vma->vm_end - vma->vm_start;
    struct cb_tranx_t *tdev = get_trans_dev(iminor(inode));

    if (!tdev) {
        trans_dbg(tdev, TR_ERR, "core: get_trans_dev failed.\n");
        return -EFAULT;
    }

    vma->vm_flags &= ~VM_IO;
    vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size,
                        vma->vm_page_prot) < 0) {
        trans_dbg(tdev, TR_ERR, "core: remap_pfn_range failed.\n");
        trans_dbg(tdev, TR_ERR,
                  "core: start:0x%lx end:0x%lx size:0x%lx paddr:0x%lx\n",
                  vma->vm_start, vma->vm_end, size,
                  vma->vm_pgoff << PAGE_SHIFT);
        ret = -ENOMEM;
    }

    return ret;
}

static const struct file_operations trans_char_fops = {
    .owner          = THIS_MODULE,
    .open           = trans_open,
    .release        = trans_close,
    .unlocked_ioctl = trans_ioctl,
    .compat_ioctl   = trans_ioctl,
    .mmap           = trans_mmap,
};

static int modules_init(struct cb_tranx_t *data)
{
    struct cb_tranx_t *tdev = data;

    /* initialize pcie module first.*/
    if (cb_pci_init(tdev)) {
        trans_dbg(tdev, TR_ERR, "core: initialize pci failed.\n");
        goto out;
    }

    if (enable_all_pll(tdev)) {
        trans_dbg(tdev, TR_ERR, "core: enable pll failed.\n");
        goto out;
    }

    if (encoder_init(tdev)) {
        trans_dbg(tdev, TR_ERR, "core: initialize encoder failed.\n");
        goto out_release_pci;
    }

    /* initialize hw_monitor module */
    if (hw_monitor_init(tdev)) {
        trans_dbg(tdev, TR_ERR, "core: initialize hw_monitor failed\n");
        goto out_release_enc;
    }

    /* initialize edma module */
    if (edma_init(tdev)) {
        trans_dbg(tdev, TR_ERR, "core: initialize edma failed.\n");
        goto out_release_hwm;
    }

    /* initialize memory module */
    if (cb_mem_init(tdev)) {
        trans_dbg(tdev, TR_ERR, "core: initialize memory failed.\n");
        goto out_release_edma;
    }

    /* initialize vc8000d module */
    if (vc8000d_init(tdev)) {
        trans_dbg(tdev, TR_ERR, "core: initialize vc8000d failed.\n");
        goto out_release_mem;
    }

    /* initialize vc8000e module */
    if (vc8000e_init(tdev)) {
        trans_dbg(tdev, TR_ERR, "core: initialize vc8000e failed.\n");
        goto out_release_vcd;
    }

    /* initialize vc8000e module */
    if (bigsea_init(tdev)) {
        trans_dbg(tdev, TR_ERR, "core: initialize bigsea failed.\n");
        goto out_release_vce;
    }

    /* initialize misc ip module */
    if (misc_ip_init(tdev)) {
        trans_dbg(tdev, TR_ERR, "core: initialize misc_ip failed.\n");
        goto out_release_bigsea;
    }

    return 0;

out_release_bigsea:
    bigsea_release(tdev);
out_release_vce:
    vc8000e_release(tdev);
out_release_vcd:
    vc8000d_release(tdev);
out_release_mem:
    cb_mem_release(tdev);
out_release_edma:
    edma_release(tdev);
out_release_hwm:
    hw_monitor_release(tdev);
out_release_enc:
    encoder_release(tdev);
out_release_pci:
    cb_pci_release(tdev);
out:
    return -EFAULT;
}

static void modules_release(struct cb_tranx_t *data)
{
    struct cb_tranx_t *tdev = data;

    misc_ip_release(tdev);
    hw_monitor_release(tdev);
    bigsea_release(tdev);
    vc8000e_release(tdev);
    vc8000d_release(tdev);
    encoder_release(tdev);
    edma_release(tdev);
    cb_mem_release(tdev);
    cb_pci_release(tdev);
}

static ssize_t drv_log_store(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count)
{
    u32 level;
    struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
    struct cb_tranx_t *tdev    = mtdev->tdev;

    if (count == 0)
        return 0;

    if (sscanf(buf, "%d", &level) != 1) {
        trans_dbg(tdev, TR_ERR, "core: not in hex or decimal form.\n");
        return -1;
    }
    if (level <= TR_DBG)
        tdev->print_level = level;
    else {
        trans_dbg(tdev, TR_ERR, "core: level:%d is invalid.\n", level);
        return -1;
    }

    return count;
}

static ssize_t drv_log_show(struct device *dev, struct device_attribute *attr,
                            char *buf)
{
    struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
    struct cb_tranx_t *tdev    = mtdev->tdev;

    return sprintf(buf, "%d\n", tdev->print_level);
}
static ssize_t drv_rev_show(struct device *dev, struct device_attribute *attr,
                            char *buf)
{
    return sprintf(buf, "%s\n", version_string);
}

static ssize_t hw_err_store(struct device *dev, struct device_attribute *attr,
                            const char *buf, size_t count)
{
    struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
    struct cb_tranx_t *tdev    = mtdev->tdev;
    int flag, ret;

    if (count == 0)
        return 0;

    ret = sscanf(buf, "%d", &flag);
    if (ret != 1) {
        trans_dbg(tdev, TR_ERR, "edma: %s ret=%d, input_val:%d\n", __func__,
                  ret, flag);
        return -1;
    }
    tdev->hw_err_flag = flag;

    return count;
}

static ssize_t hw_err_show(struct device *dev, struct device_attribute *attr,
                           char *buf)
{
    struct cb_misc_tdev *mtdev = dev_get_drvdata(dev);
    struct cb_tranx_t *tdev    = mtdev->tdev;

    return sprintf(buf, "0x%x\n", tdev->hw_err_flag);
}

static DEVICE_ATTR_RW(hw_err);
static DEVICE_ATTR_RW(drv_log);
static DEVICE_ATTR_RO(drv_rev);

static struct attribute *trans_sysfs_entries[] = {
    &dev_attr_drv_log.attr, &dev_attr_drv_rev.attr, &dev_attr_hw_err.attr, NULL
};

static struct attribute_group trans_attribute_group = {
    .name  = NULL,
    .attrs = trans_sysfs_entries,
};

static int register_tranx_dev(struct pci_dev *pdev, struct cb_tranx_t *tdev)
{
    int ret = 0;
    int node;
    struct cb_misc_tdev *mtdev        = NULL;
    struct miscdevice *trans_misc_dev = NULL;

    for (node = 0; node < DEVICE_CNT; node++)
        if (transcoder_dev[node].node_num == -1)
            break;
    if (node >= DEVICE_CNT) {
        trans_dbg(tdev, TR_ERR, "core: node:%d error, max val should be 11.\n",
                  node);
        return -EFAULT;
    }

    tdev->dev_name = kasprintf(GFP_KERNEL, "transcoder%d", node);
    mtdev          = kzalloc(sizeof(struct miscdevice), GFP_KERNEL);
    if (!mtdev) {
        trans_dbg(tdev, TR_ERR, "core: kzalloc mtdev failed\n");
        goto out;
    }

    trans_misc_dev        = &mtdev->misc;
    trans_misc_dev->minor = MISC_DYNAMIC_MINOR;
    trans_misc_dev->fops  = &trans_char_fops;
    trans_misc_dev->name  = tdev->dev_name;
    trans_misc_dev->mode  = 0666;
    ret                   = misc_register(trans_misc_dev);
    if (ret) {
        trans_dbg(tdev, TR_ERR, "core: misc_register trans_misc_dev failed.\n");
        goto out_free_misc;
    }
    tdev->misc_dev = trans_misc_dev;
    pci_set_drvdata(pdev, tdev);
    mtdev->tdev = tdev;

    ret = sysfs_create_group(&trans_misc_dev->this_device->kobj,
                             &trans_attribute_group);
    if (ret) {
        trans_dbg(tdev, TR_ERR,
                  "core: failed to create sysfs device attributes\n");
        goto out_dereg_misc;
    }

    transcoder_dev[node].data     = tdev;
    transcoder_dev[node].minor    = trans_misc_dev->minor;
    tdev->node_index              = node;
    transcoder_dev[node].node_num = node;

    trans_dbg(tdev, TR_DBG,
              "core: register transcoder successfully: name:%s, minor:%d\n",
              trans_misc_dev->name, trans_misc_dev->minor);

    return 0;

out_dereg_misc:
    misc_deregister(trans_misc_dev);
out_free_misc:
    kfree(mtdev);
out:
    return -EFAULT;
}

static void clean_node(struct cb_tranx_t *tdev)
{
    transcoder_dev[tdev->node_index].node_num = -1;
    transcoder_dev[tdev->node_index].minor    = -1;
}

static int trans_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int i;
    u32 val32;
    int ret = 0;
    void *modules;
    struct cb_tranx_t *tdev;

    tdev = kzalloc(sizeof(struct cb_tranx_t), GFP_KERNEL);
    if (!tdev) {
        dev_info(&pdev->dev, "core: alloc cb_tranx_t failed\n");
        goto out;
    }

    /* default level is info */
    tdev->print_level = level;
    tdev->pdev        = pdev;

    /* pci+memory+edma+vc8000d+vc8000e+bigsea+encoder+hw_monitor */
    modules = kzalloc(sizeof(void *) * TR_MODULE_MAX, GFP_KERNEL);
    if (!modules) {
        dev_info(&pdev->dev, "core: allocate modules failed.\n");
        goto out_free_dev;
    }

    for (i = 0; i < TR_MODULE_MAX; i++)
        tdev->modules[i] = modules + i;

    ret = register_tranx_dev(pdev, tdev);
    if (ret) {
        trans_dbg(tdev, TR_ERR, "core: register_tranx_dev failed.\n");
        goto out_free_module;
    }

    mutex_init(&tdev->reset_lock);
    ret = modules_init(tdev);
    if (ret) {
        trans_dbg(tdev, TR_ERR, "core: modules_init failed\n");
        goto out_free_misc;
    }

    sema_init(&tdev->tcache[0].sem, 1);
    sema_init(&tdev->tcache[1].sem, 1);
    tdev->hw_err_flag = 0;

    val32 = ccm_read(tdev, LINK_REQ_RST_NOT_SYNC);
    val32 &= (~GLUE_LOGIC_INT_SEL); /* enable msi-x forward*/
    ccm_write(tdev, LINK_REQ_RST_NOT_SYNC, val32);
    trans_dbg(tdev, TR_DBG, "core: LINK_REQ_RST_NOT_SYNC:0x%x\n",
              ccm_read(tdev, LINK_REQ_RST_NOT_SYNC));

    if (tcache_init(tdev))
        goto out_release_module;

    trans_dbg(tdev, TR_INF, "transcoder inserted successfully.\n");

    return 0;

out_release_module:
    modules_release(tdev);
out_free_misc:
    misc_deregister(tdev->misc_dev);
    kfree(tdev->misc_dev);
    clean_node(tdev);
out_free_module:
    kfree(modules);
out_free_dev:
    kfree(tdev);
out:
    return -EFAULT;
}

static void trans_remove(struct pci_dev *pdev)
{
    u32 val32;
    struct cb_tranx_t *tdev           = pci_get_drvdata(pdev);
    struct miscdevice *trans_misc_dev = tdev->misc_dev;

    trans_dbg(tdev, TR_DBG,
              "core: trans remove. tdev:0x%p trans_misc_dev:0x%p %d\n", tdev,
              trans_misc_dev, trans_misc_dev->minor);

    val32 = ccm_read(tdev, LINK_REQ_RST_NOT_SYNC);
    if (val32 & GLUE_LOGIC_INT_SEL) {
        val32 &= ~GLUE_LOGIC_INT_SEL;
        ccm_write(tdev, LINK_REQ_RST_NOT_SYNC, val32);
    }
    trans_dbg(tdev, TR_DBG, "core: LINK_REQ_RST_NOT_SYNC:0x%x\n",
              ccm_read(tdev, LINK_REQ_RST_NOT_SYNC));

    modules_release(tdev);
    sysfs_remove_group(&trans_misc_dev->this_device->kobj,
                       &trans_attribute_group);
    misc_deregister(trans_misc_dev);
    clean_node(tdev);
    mutex_destroy(&tdev->reset_lock);
    kfree(tdev->modules[0]);
    kfree(trans_misc_dev);
    kfree(tdev);
}

#ifdef CONFIG_PM
static int trans_suspend(struct pci_dev *pdev, pm_message_t mesg)
{
    return 0;
}

static int trans_resume(struct pci_dev *pdev)
{
    return 0;
}
#endif

static struct pci_device_id trans_pcie_table[] = {
    { FB_VENDOR_ID, FB_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
    { VSI_VENDOR_ID, VSI_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
    { YX_VENDOR_ID, YX_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
    {
        0,
    }
};

MODULE_DEVICE_TABLE(pci, trans_pcie_table);

static struct pci_driver trans_pcie_driver = {
    .name     = "transcoder",
    .id_table = trans_pcie_table,
    .probe    = trans_probe,
    .remove   = trans_remove,
#ifdef CONFIG_PM
    .suspend = trans_suspend,
    .resume  = trans_resume,
#endif
};

static int __init trans_pcie_init(void)
{
    int i;

    show_version();
    for (i = 0; i < DEVICE_CNT; i++)
        transcoder_dev[i].node_num = -1;
    return pci_register_driver(&trans_pcie_driver);
}

static void __exit trans_pcie_cleanup(void)
{
    pci_unregister_driver(&trans_pcie_driver);
}

module_init(trans_pcie_init);
module_exit(trans_pcie_cleanup);

MODULE_AUTHOR("Fengyin Wu <Fengyin.Wu@verisilicon.com>");
MODULE_AUTHOR("Huaidong Mo <Huaidong.Mo@verisilicon.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(description_string);
MODULE_VERSION(version_string);
