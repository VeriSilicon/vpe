/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2018 Verisilicon Inc.
 */

#ifndef __TRANSCODER_H__
#define __TRANSCODER_H__

#include <linux/types.h>
#include <linux/ioctl.h>

enum TRANS_EDMA_DIR {
	RC2EP = 0,
	EP2RC = 1,
};

enum TRANS_EDMA_INT {
	POLL_EN = 0,
	INTE_EN = 1,
};

enum TRANS_LT_STATUS {
	LT_UNDONE = 0,
	LT_DONE = 1,
};

enum TRANS_SLICE_ID {
	SLICE_0 = 0,
	SLICE_1 = 1,
};

enum TRANS_TASK_PRIORITY {
	TASK_LIVE = 0,
	TASK_VOD = 1,
};

enum TRANS_MEM_LOCATION {
	EP_SIDE = 0,
	RC_SIDE = 1,
};

enum MISC_IP_ID {
	VC8000D_ID = 0,
	VC8000E_ID,
	BIGSEA_ID,
	L2CACH_VCD_ID,
	L2CACH_VCE_ID,
	F1_ID,
	F2_ID,
	F3_ID,
	F4_TCACH_ID,
	F4_DTRC_ID,
	F4_L2CACHE_ID,
};

/* pcie bar region description */
struct bar_info {
	__u32 bar_num;
	__u64 bar_addr;
	__u32 bar_size;
};

/* edma info description */
struct trans_pcie_edma {
	__u32 size; /* transfer size */
	__u32 sar_low; /* source address low 32 bits */
	__u32 sar_high; /* source address high 32 bits */
	__u32 dar_low; /* destination address low 32 bits */
	__u32 dar_high; /* destination address high 32 bits */
	__u32 direct; /* 1:EP to RC;  0:RC to EP*/
	__u32 interrupt_enable;
	__u32 element_size;
	__u32 slice;
	__u64 rc_ult;
	__u32 element_num_each;
};

/* edma link table info */
struct dma_link_table {
	__u32 control; /* control configuration */
	__u32 size; /* transfer size */
	__u32 sar_low; /* source address low 32 bits */
	__u32 sar_high; /* source address high 32 bits */
	__u32 dst_low; /* destination address low 32 bits */
	__u32 dst_high; /* destination address high 32 bits */
};

struct reg_desc {
	__u32 id; /* register index, starting from 0 */
	__u32 val; /* value */
};

struct core_desc {
	__u32 id; /* core index */
	__u32 *regs; /* core register virtual address */
	__u32 size; /* size of register space */
	__u32 reg_id; /* register index */
};

struct ip_desc {
	__u8 ip_id;
	struct core_desc core;
};

/* vc8000d configuration info */
struct vcd_core_config {
	__u32 core_id; /* core index */
	__u32 asic_id; /* asic ID */
	__u32 hantrodec_synth_cfg; /* index 50 configuration */
	__u32 hantrodec_synth_cfg_2; /* index 54 configuration */
	__u32 hantrodec_synth_cfg_3; /* index 56 configuration */
	__u32 hantrodec_fuse_cfg; /* index 57 configuration */
	__u32 hantrodecpp_synth_cfg; /* index 60 configuration */
	__u32 hantrodecpp_cfg_stat; /* index 260 configuration */
	__u32 hantrodecpp_fuse_cfg; /* index 61 configuration */
};

/* vc8000e configuration info */
struct vce_core_config {
	__u32 core_id; /* core index */
	__u32 vce_cfg_1; /* index 80 configuration */
	__u32 vce_cfg_2; /* index 214 configuration */
	__u32 vce_cfg_3; /* index 226 configuration */
};

/* reserve core parameter */
struct core_info {
	__u32 format; /* reserved core info */
	__u32 task_priority; /* reserve priority */
};

struct mem_info {
	__s32 task_id; /* task id */
	__u8 mem_location; /* needed memory location */
	__u64 phy_addr; /* physics address */
	__u32 size;
	void *rc_kvirt;
};

/* ep memory used information */
struct mem_used_info {
	__u32 s0_used;
	__u32 s0_free;
	__u32 s0_blk_used;
	__u32 s1_used;
	__u32 s1_free;
	__u32 s1_blk_used;
};

/* pcie submodule ioctl commands */
#define IOCTL_CMD_PCIE_MINNR          0x00
#define IOCTL_CMD_PCIE_MAXNR          0x01
#define CB_TRANX_GET_BARADDR         _IOWR('k', 0x00, struct bar_info *)
#define CB_TRANX_SW_DDR_MAPPING      _IOWR('k', 0x01, __u32 *)

/* edma submodule ioctl commands */
#define IOCTL_CMD_EDMA_MINNR          0x02
#define IOCTL_CMD_EDMA_MAXNR          0x04
#define CB_TRANX_EDMA_STATUS          _IOWR('k', 0x02, __u32 *)
#define CB_TRANX_EDMA_TRANX           _IOWR('k', 0x03, struct trans_pcie_edma *)
#define CB_TRANX_EDMA_TRANX_TCACHE    _IOWR('k', 0x04, struct trans_pcie_edma *)

/* memory submodule ioctl commands */
#define IOCTL_CMD_MEM_MINNR           0x05
#define IOCTL_CMD_MEM_MAXNR           0x09
#define CB_TRANX_MEM_GET_UTIL         _IOWR('k', 0x05, struct mem_used_info *)
#define CB_TRANX_MEM_ALLOC            _IOWR('k', 0x06, struct mem_info *)
#define CB_TRANX_MEM_FREE             _IOWR('k', 0x07, struct mem_info *)
#define CB_TRANX_MEM_GET_TASKID       _IOWR('k', 0x08, __s32 *)
#define CB_TRANX_MEM_FREE_TASKID      _IOWR('k', 0x09, __s32 *)

/* vc8000d submodule ioctl commands */
#define IOCTL_CMD_VCD_MINNR           0x0a
#define IOCTL_CMD_VCD_MAXNR           0x13
#define CB_TRANX_VCD_IO_SIZE          _IOR('k', 0x0a, __u32 *)
#define CB_TRANX_VCD_CORE_CNT         _IOR('k', 0x0b, __u32 *)
#define CB_TRANX_VCD_GET_HWID         _IOR('k', 0x0c, __u32 *)
#define CB_TRANX_VCD_FIND_CORE        _IOR('k', 0x0d, __u32 *)
#define CB_TRANX_VCD_GET_BUILDID      _IOR('k', 0x0e, __u32 *)
#define CB_TRANX_VCD_GET_CFG          _IOR('k', 0x0f, struct vcd_core_config *)
#define CB_TRANX_VCD_RESERVE          _IOWR('k', 0x10, struct core_info *)
#define CB_TRANX_VCD_RELEASE          _IOWR('k', 0x11, __u32 *)
#define CB_TRANX_VCD_WAIT_DONE        _IOR('k', 0x12, __s32 *)
#define CB_TRANX_VCD_WAIT_CORE_DONE   _IOR('k', 0x13, __u32 *)

/* vc8000e submodule ioctl commands */
#define IOCTL_CMD_VCE_MINNR           0x14
#define IOCTL_CMD_VCE_MAXNR           0x1a
#define CB_TRANX_VCE_IO_SIZE          _IOR('k', 0x14, __u32 *)
#define CB_TRANX_VCE_CORE_CNT         _IOR('k', 0x15, __u32 *)
#define CB_TRANX_VCE_GET_HWID         _IOR('k', 0x16, __u32 *)
#define CB_TRANX_VCE_GET_CFG          _IOR('k', 0x17, struct vce_core_config *)
#define CB_TRANX_VCE_RESERVE          _IOWR('k', 0x18, struct core_info *)
#define CB_TRANX_VCE_RELEASE          _IOWR('k', 0x19, __u32 *)
#define CB_TRANX_VCE_WAIT_DONE        _IOR('k', 0x1a, __u32 *)

/* bigsea submodule ioctl commands */
#define IOCTL_CMD_BIGSEA_MINNR        0x1b
#define IOCTL_CMD_BIGSEA_MAXNR        0x20
#define CB_TRANX_BIGSEA_IO_SIZE       _IOR('k', 0x1b, __u32 *)
#define CB_TRANX_BIGSEA_CORE_CNT      _IOR('k', 0x1c, __u32 *)
#define CB_TRANX_BIGSEA_GET_HWID      _IOR('k', 0x1d, __u32 *)
#define CB_TRANX_BIGSEA_RESERVE       _IOWR('k', 0x1e, struct core_info *)
#define CB_TRANX_BIGSEA_RELEASE       _IOWR('k', 0x1f, __u32 *)
#define CB_TRANX_BIGSEA_WAIT_DONE     _IOR('k', 0x20, struct core_desc *)

/* misc submodule ioctl commands */
#define IOCTL_CMD_MISC_IP_MINNR       0x21
#define IOCTL_CMD_MISC_IP_MAXNR       0x24
#define CB_TRANX_RD_REG               _IOWR('k', 0x21, struct ip_desc *)
#define CB_TRANX_WR_REG               _IOWR('k', 0x22, struct ip_desc *)
#define CB_TRANX_PULL_REGS            _IOWR('k', 0x23, struct ip_desc *)
#define CB_TRANX_PUSH_REGS            _IOWR('k', 0x24, struct ip_desc *)

/* request/release tcache */
#define CB_TRANX_REQUEST_TC           _IOWR('k', 0x25, __u32 *)
#define CB_TRANX_RELEASE_TC           _IOWR('k', 0x26, __u32 *)

#define CB_TRANX_EDMA_PHY_TRANX       _IOWR('k', 0x27, struct trans_pcie_edma *)


#define TRANS_MAXNR	0x27
#endif  /*  __TRANSCODER_H__*/
