// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 VeriSilicon Holdings Co., Ltd.
 *
 * Support manage two slices memory;
 * the memory is in ep side, so the address is in ep pcie axi master view
 * memory space. Every slice have 2GB DDR memory,
 * but low 64MB have same address mapping with Tcache, and high 256MB
 * will be used by DDR ECC, so the valid size that codec can use is 1728MB.
 * One slice memory will be divide into 16 blocks with same size(108MB),
 * and one block contain 27684 chunks, one chunk is 4KB. Block is the
 * minimum unit for video task, every block is only used by a task, so
 * one task will take one block at one time. Next allocation, task will
 * get free memory form itself block firstly, if no, it will get a new
 * block. When a block is not used, it will be freed. One block can’t
 * shared by different tasks.
 * When allocate memory from a block, it also check the first chunk(chunk
 * index is 0), if not be used, reserve it, and record reserved chunk
 * count in the chunk 0, so consecutive chunks will be reserved for this
 * allocation; next allocation, check chunk 0, it is used, get the reserved
 * chunk size(N), then check the chunk which index is N, if not be used,
 * reserved it, or else continue to check next, until get valid chunks,
 * if not, get a new block.
 *
 * the memory in slice0 and slice1 will do a simple balance by used size,
 * when the memory size which is allocated from slice0 is greater than
 * slice1, then next time memory will be allocated from slice1; and the
 * allocation from slice0 is the other way round.
 */

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/pci.h>
#include <linux/pagemap.h>

#include "common.h"
#include "memory.h"
#include "transcoder.h"

/* aggress as follows: the first slice is slice_0, another is slice_1 */
#define MAX_TASK_NUM	16
#define MIN_TASK_ID	1
#define S0_BLOCK_CNT	47
#define S1_BLOCK_CNT	47
#define CHUNK_SIZE	0x1000

#define SLICE0_INDEX	0
#define SLICE1_INDEX	1

/* exclude tcache,first 0x200000 for edma link table.*/
unsigned long s0_start = 0x4200000;
/* exclude ecc data */
unsigned long s0_end = 0x70000000;
/* exclude tcache */
unsigned long s1_start = 0x84000000;
/* exclude ecc data */
unsigned long s1_end = 0xF0000000;

module_param(s0_start, ulong, 0644);
MODULE_PARM_DESC(s0_start, "ddr0 start address; default is 0x4200000.");

module_param(s0_end, ulong, 0644);
MODULE_PARM_DESC(s0_end, "ddr0 end address; default is 0x70000000.");

module_param(s1_start, ulong, 0644);
MODULE_PARM_DESC(s1_start, "ddr1 start address; default is 0x84000000.");

module_param(s1_end, ulong, 0644);
MODULE_PARM_DESC(s1_end, "ddr1 end address; default is 0xF0000000.");

/* 0:use slice_0 1:use_slice_1 2:balance s0 and s1 */
unsigned int method = 2;

module_param(method, uint, 0644);
MODULE_PARM_DESC(method,
	"allocate mem method,0:use s0,1:use s1,2:balance s0/s1;default is 2");

/* taskid status */
#define ID_BUSY		0x1
#define ID_FREE		0x0

/* memory chunk information */
struct hlina_chunk {
	u64 bus_add; /* this chunk start physical address */
	u32 ck_rsv; /* indicate this chunk whether be used or not */
};

/* memory block information */
struct mem_block {
	u32 index; /* block index */
	u64 start_addr; /*  the block start address; */
	int task_id; /* this block be used by task_id */
	u32 block_size; /* block size; */
	u32 chunks; /* total chunk count */
	u32 free_chunks; /* the chunks not used */
	struct hlina_chunk *chk_inf;
};

/* The memory_t structure describes memory module */
struct memory_t {
	struct cb_tranx_t *tdev;
	struct mutex mem_mutex_ep;
	struct mutex mem_mutex_rc;

	struct mem_block *mem_s0_bk; /* memory slice0 block*/
	struct mem_block *mem_s1_bk; /* memory slice1 block*/
	u32 s0_rev_size; /* slice0_reserved_size */
	u32 s1_rev_size; /* slice1_reserved_size */

	spinlock_t taskid_lock;
	int id_st[MAX_TASK_NUM + 1]; /* task id status */
	struct file *id_filp[MAX_TASK_NUM + 1]; /* save owner who use the id */
};

/* get a not used task id, maximun support 32 task, so total count of
 * task id is 32, from 1 to 32.
 */
static int get_task_id(struct memory_t *tmem, struct file *filp)
{
	int i;

	spin_lock(&tmem->taskid_lock);
	for (i = MIN_TASK_ID; i <= MAX_TASK_NUM; i++) {
		if (tmem->id_st[i] == ID_FREE) {
			tmem->id_st[i] = ID_BUSY;
			tmem->id_filp[i] = filp;
			break;
		}
	}
	spin_unlock(&tmem->taskid_lock);

	return (i <= MAX_TASK_NUM) ? i : -1;
}

/* if free a task id, will free all corresponding memory. */
static void free_task_id(struct memory_t *tmem, int id)
{
	int i, j;
	struct mem_block *sbk;
	int mem_leak_flag_0 = 0;
	int mem_leak_flag_1 = 0;

	mutex_lock(&tmem->mem_mutex_ep);

	spin_lock(&tmem->taskid_lock);
	tmem->id_st[id] = ID_FREE;
	tmem->id_filp[id] = NULL;

	for (i = 0; i < S0_BLOCK_CNT; i++) {
		if (tmem->mem_s0_bk[i].task_id == id) {
			sbk = &(tmem->mem_s0_bk[i]);
			for (j = 0; j < sbk->chunks; j++) {
				if (sbk->chk_inf[j].ck_rsv) {
					mem_leak_flag_0++;
					tmem->s0_rev_size -=
					sbk->chk_inf[j].ck_rsv * CHUNK_SIZE;
					sbk->chk_inf[j].ck_rsv = 0;
					trans_dbg(tmem->tdev, TR_DBG,
						"mem: mem leak,s0,blk:%d,id:%d,add=0x%llx\n",
						i, id, sbk->chk_inf[j].bus_add);
				}
			}
			sbk->task_id = 0;
		}
	}
	for (i = 0; i < S1_BLOCK_CNT; i++) {
		if (tmem->mem_s1_bk[i].task_id == id) {
			sbk = &(tmem->mem_s1_bk[i]);
			for (j = 0; j < sbk->chunks; j++) {
				if (sbk->chk_inf[j].ck_rsv) {
					mem_leak_flag_1++;
					tmem->s1_rev_size -=
					sbk->chk_inf[j].ck_rsv * CHUNK_SIZE;
					sbk->chk_inf[j].ck_rsv = 0;
					trans_dbg(tmem->tdev, TR_DBG,
						"mem: mem leak,s1,blk:%d,id:%d,add=0x%llx\n",
						i, id, sbk->chk_inf[j].bus_add);
				}
			}
			sbk->task_id = 0;
		}
	}
	if (mem_leak_flag_0)
		trans_dbg(tmem->tdev, TR_NOTICE, "mem: mem leak in slice_0\n");
	if (mem_leak_flag_1)
		trans_dbg(tmem->tdev, TR_NOTICE, "mem: mem leak in slice_1\n");

	spin_unlock(&tmem->taskid_lock);
	mutex_unlock(&tmem->mem_mutex_ep);
}

void cb_mem_close(struct cb_tranx_t *tdev, struct file *filp)
{
	int id;
	struct memory_t *tmem = tdev->modules[TR_MODULE_MEMORY];

	for (id = MIN_TASK_ID; id <= MAX_TASK_NUM; id++) {
		if (tmem->id_filp[id] == filp) {
			trans_dbg(tdev, TR_DBG, "%s id:%d\n", __func__, id);
			free_task_id(tmem, id);
			break;
		}
	}
}

/* Cycle through the buffers we have, give the first free one */
static int get_chunks(int slice, int block_index, unsigned long *busaddr,
			 unsigned int size, int task_id, struct memory_t *tmem)
{
	int i, j;
	int ret = 0;
	int bi = block_index;
	struct mem_block *mb;
	unsigned int skip_chunks = 0;
	/* calculate how many chunks we need;round up to chunk boundary */
	unsigned int alloc_chunks = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;

	if (slice > 1) {
		trans_dbg(tmem->tdev, TR_ERR,
			"mem: %s, slice id:%d is error\n", __func__, slice);
		return -EFAULT;
	}

	if (slice == 0)
		mb = tmem->mem_s0_bk;
	else
		mb = tmem->mem_s1_bk;

	*busaddr = 0;
	/* run through the chunk table */
	for (i = 0; i < mb[bi].chunks;) {
		skip_chunks = 0;
		/* if this chunk is available */
		if (!mb[bi].chk_inf[i].ck_rsv) {
			/* check that there is enough memory left */
			if (i + alloc_chunks > mb[bi].chunks)
				break;

			/*
			 * check that there is enough consecutive
			 * chunks available.
			 */
			for (j = i; j < i + alloc_chunks; j++) {
				if (mb[bi].chk_inf[j].ck_rsv) {
					skip_chunks = 1;
					/* skip the used chunks */
					i = j + mb[bi].chk_inf[j].ck_rsv;
					break;
				}
			}

			/* if enough free memory found */
			if (!skip_chunks) {
				*busaddr = mb[bi].chk_inf[i].bus_add;
				mb[bi].chk_inf[i].ck_rsv = alloc_chunks;
				mb[bi].free_chunks -= alloc_chunks;

				if (slice == 0)
					tmem->s0_rev_size +=
						alloc_chunks * CHUNK_SIZE;
				else
					tmem->s1_rev_size +=
						alloc_chunks * CHUNK_SIZE;

				break;
			}
		} else {
			/* skip the used chunks */
			i += mb[bi].chk_inf[i].ck_rsv;
		}
	}

	if (*busaddr == 0) {
		trans_dbg(tmem->tdev, TR_DBG,
			"mem: get chunks failed in s:%d,block:%d,size:0x%x\n",
			slice, bi, size);
		ret = -EFAULT;
	} else
		trans_dbg(tmem->tdev, TR_DBG,
			"mem: alloc ep s:%d block:%d,addr:0x%lx size:0x%x,task_id:%d\n",
			slice, bi, *busaddr, size, task_id);

	return ret;
}

/* allocate memory from a new block */
int alloc_in_new_block(int slice, unsigned long *addr,
			     unsigned int size, int task_id,
			     struct memory_t *tmem)
{
	int i, j, blk_cnt;
	int ret = 0;
	struct mem_block *mem_block;

	if (slice == SLICE0_INDEX) {
		mem_block = tmem->mem_s0_bk;
		blk_cnt = S0_BLOCK_CNT;
	} else if (slice == SLICE1_INDEX) {
		mem_block = tmem->mem_s1_bk;
		blk_cnt = S1_BLOCK_CNT;
	} else {
		trans_dbg(tmem->tdev, TR_ERR,
			"mem: %s, slice id:%d is error\n", __func__, slice);
		return -EFAULT;
	}

	*addr = 0;
	/* traversal all blocks, get a new block. */
	for (i = 0; i < blk_cnt; i++) {
		if (mem_block[i].task_id == 0) {
			ret = get_chunks(slice, i, addr, size, task_id, tmem);
			if (ret) {
				trans_dbg(tmem->tdev, TR_ERR,
					"mem: the size:0x%x is very large, more than slice:%d block:%d size, task_id=%d\n",
					size, slice, i, task_id);
				for (j = 0; j < mem_block[i].chunks; j++) {
					if (mem_block[i].chk_inf[j].ck_rsv)
						trans_dbg(tmem->tdev, TR_ERR,
							"mem: j:%d reserve_cnt:%d\n",
							j, mem_block[i].chk_inf[j].ck_rsv);
				}
			} else {
				mem_block[i].task_id = task_id;
				break;
			}
		}
	}

	/*  not has valid block, alloc failed. */
	if (i == blk_cnt) {
		trans_dbg(tmem->tdev, TR_DBG,
			"mem: %s, not has valid block in slice%d.\n",
			__func__, slice);
		return -EFAULT;
	}

	return ret;
}

/*
 * get memory from a slice.
 * @s: slice id.
 * @id: task id
 * @check: check:1, only get memory from used block
 *         check:0, if can't get valid memory in used block, allocate from
 *                  a new block.
 */
static int alloc_mem_in_slice(int s, unsigned long *addr,
					unsigned int size, int id,
					struct memory_t *tmem, int check)
{
	int i, blk_cnt;
	int ret = 0;
	struct mem_block *mem_block;

	if (s == SLICE0_INDEX) {
		mem_block = tmem->mem_s0_bk;
		blk_cnt = S0_BLOCK_CNT;
	} else if (s == SLICE1_INDEX) {
		mem_block = tmem->mem_s1_bk;
		blk_cnt = S1_BLOCK_CNT;
	} else {
		trans_dbg(tmem->tdev, TR_ERR,
			"mem: slice id:%d is error in alloc mem in slice\n", s);
		return -EFAULT;
	}
	*addr = 0;

	/* traversal all blocks,check itself block if it has valid memory */
	for (i = 0; i < blk_cnt; i++) {
		if (mem_block[i].task_id == id) {
			ret = get_chunks(s, i, addr, size, id, tmem);
			if (!ret)
				break;
		}
	}

	/* not has valid memory in itself block */
	if (i == blk_cnt) {
		if (check == 1)
			ret = -EFAULT;
		else
			/* get a new block; */
			ret = alloc_in_new_block(s, addr, size, id, tmem);
	}

	return ret;
}

/* allocate memory from its own used block */
static int alloc_in_used_block(unsigned long *busaddr,
				     unsigned int size,
				     int task_id,
				     struct memory_t *tmem)
{
	int ret;

	if (tmem->s0_rev_size <= tmem->s1_rev_size) {
		/* check slice0 if there is valid used block */
		ret = alloc_mem_in_slice(SLICE0_INDEX, busaddr, size,
					 task_id, tmem, 1);
		/* s0 not has valid used blocks for this id, check s1. */
		if (ret) {
			/* check slice1 if it has valid used block. */
			ret = alloc_mem_in_slice(SLICE1_INDEX, busaddr, size,
						 task_id, tmem, 1);
		}
	} else {
		/* check slice1 if it has valid used block */
		ret = alloc_mem_in_slice(SLICE1_INDEX, busaddr, size,
					 task_id, tmem, 1);
		/* s1 not has valid used blocks for this id, check s0. */
		if (ret) {
			/* check s0 if it has valid used block. */
			ret = alloc_mem_in_slice(SLICE0_INDEX, busaddr, size,
						 task_id, tmem, 1);
		}
	}

	return ret;
}

/*
 * allocate memory in ep side.
 * method is the allocation strategy.
 *	0: only use slice_0
 *	1: only use_slice_1
 *	2: balance s0 and s1 by used size.
 */
static int alloc_mem_ep(unsigned long *busaddr,
			    unsigned int size,
			    int task_id,
			    struct memory_t *tmem)
{
	int ret = 0;
	*busaddr = 0;

	if (WARN_ON(method > 2)) {
		trans_dbg(tmem->tdev, TR_ERR, "mem: %s, method:%d error\n",
			  __func__);
		return -EFAULT;
	}

	/* alloc memory from slice 0  */
	if (method == 0)
		ret = alloc_mem_in_slice(SLICE0_INDEX, busaddr, size, task_id,
					 tmem, 0);

	/* alloc memory from slice 1 */
	else if (method == 1)
		ret = alloc_mem_in_slice(SLICE1_INDEX, busaddr, size, task_id,
					 tmem, 0);

	/* alloc memory from slice 0 and slice 1, a simple balance */
	else {
		if (alloc_in_used_block(busaddr, size, task_id, tmem)) {
			if (tmem->s0_rev_size <= tmem->s1_rev_size) {
				/* first alloc from s0 */
				ret = alloc_in_new_block(SLICE0_INDEX,
						busaddr, size, task_id, tmem);
				if (ret)
					ret = alloc_in_new_block(SLICE1_INDEX,
								 busaddr, size,
								 task_id, tmem);
			} else {
				/* first alloc from s1 */
				ret = alloc_in_new_block(SLICE1_INDEX,
						busaddr, size, task_id, tmem);
				if (ret)
					ret = alloc_in_new_block(SLICE0_INDEX,
								 busaddr, size,
								 task_id, tmem);
			}
		}

	}

	if (!*busaddr) {
		trans_dbg(tmem->tdev, TR_DBG, "mem: alloc ep mem failed\n");
		ret = -EFAULT;
	}

	return ret;
}

/*
 * when free memory, will check the block which did this memory used
 * if this block is not used, free it.
 */
static int free_mem_ep(unsigned long busaddr,
			   unsigned int size,
			   int task_id,
			   struct memory_t *tmem)
{
	int i, ret = 0;
	int slice, bi; /* bi:block index */
	struct mem_block *mb; /* memory block */
	unsigned long tmp;
	u32 rsv_size;

	/* Find the slice it belongs to */
	if ((busaddr >= s0_start) && (busaddr < s0_end)) {
		slice = SLICE0_INDEX;
		mb = tmem->mem_s0_bk;

		/* Find the block it belongs to */
		for (i = 0; i < S0_BLOCK_CNT; i++) {
			tmp = mb[i].start_addr + mb[i].block_size;
			if ((busaddr >= mb[i].start_addr) && (busaddr < tmp)) {
				bi = i;
				break;
			}
		}
	} else if ((busaddr >= s1_start) && (busaddr < s1_end)) {
		slice = SLICE1_INDEX;
		mb = tmem->mem_s1_bk;

		for (i = 0; i < S1_BLOCK_CNT; i++) {
			tmp = mb[i].start_addr + mb[i].block_size;
			if ((busaddr >= mb[i].start_addr) && (busaddr < tmp)) {
				bi = i;
				break;
			}
		}
	} else {
		trans_dbg(tmem->tdev, TR_ERR,
			"mem: busaddr:0x%lx is invalid\n", busaddr);
		ret = -EFAULT;
		goto err;
	}

	if (mb[bi].task_id != task_id) {
		trans_dbg(tmem->tdev, TR_ERR,
			"mem: %s Wrong task_id:%d on block:%d,expected id is %d\n",
			__func__, task_id, bi, mb[bi].task_id);
		trans_dbg(tmem->tdev, TR_ERR,
			"mem: %s Owner mismatch while freeing memory!\n",
			__func__);
		ret = -EFAULT;
		goto err;
	}

	trans_dbg(tmem->tdev, TR_DBG,
		"mem: %s, busaddr:0x%lx, size:0x%x, in slice:%d block:%d.\n",
		__func__, busaddr, size, slice, bi);

	for (i = 0; i < mb[bi].chunks; i++) {
		if (mb[bi].chk_inf[i].bus_add == busaddr) {
			rsv_size = mb[bi].chk_inf[i].ck_rsv * CHUNK_SIZE;
			if (slice == SLICE0_INDEX)
				tmem->s0_rev_size -= rsv_size;
			else
				tmem->s1_rev_size -= rsv_size;

			mb[bi].free_chunks += mb[bi].chk_inf[i].ck_rsv;
			mb[bi].chk_inf[i].ck_rsv = 0;

			if (mb[bi].free_chunks == mb[bi].chunks) {
				trans_dbg(tmem->tdev, TR_DBG,
					"mem: %s block:%d is free in slice:%d\n",
					__func__, bi, slice);
				mb[bi].task_id = 0;
			}
			break;
		}
	}

	if (i == mb[bi].chunks) {
		trans_dbg(tmem->tdev, TR_ERR,
			"mem: %s can't find the chunk,busaddr:0x%lx size:0x%x\n",
			__func__, busaddr, size);
		ret = -EFAULT;
		goto err;
	}

err:
	return ret;
}

/* Compute memory utilization */
static void mem_usage(struct cb_tranx_t *tdev,
			 struct mem_used_info *info)
{
	int i;
	u32 s0_used = 0, s1_used = 0;
	struct memory_t *tmem = tdev->modules[TR_MODULE_MEMORY];

	info->s0_blk_used = 0;
	info->s1_blk_used = 0;
	for (i = 0; i < S0_BLOCK_CNT; i++) {
		if (tmem->mem_s0_bk[i].task_id) {
			info->s0_blk_used++;
			s0_used += tmem->mem_s0_bk[i].block_size;
		}
	}
	for (i = 0; i < S1_BLOCK_CNT; i++) {
		if (tmem->mem_s1_bk[i].task_id) {
			info->s1_blk_used++;
			s1_used += tmem->mem_s1_bk[i].block_size;
		}
	}

	info->s0_used = s0_used / 1024 / 1024;
	info->s0_free = (s0_end - s0_start - s0_used) / 1024 / 1024;
	info->s1_used = s1_used / 1024 / 1024;
	info->s1_free = (s1_end - s1_start - s1_used) / 1024 / 1024;
}

/* display  memory status */
static ssize_t mem_info_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int pos = 0;
	struct cb_tranx_t *tdev = dev_get_drvdata(dev);
	struct memory_t *tmem = tdev->modules[TR_MODULE_MEMORY];
	struct mem_block *s0_bk = tmem->mem_s0_bk;
	struct mem_block *s1_bk = tmem->mem_s1_bk;
	u32 max_len = 16, i, j;
	u32 cnt = S0_BLOCK_CNT/max_len, remainder = S0_BLOCK_CNT%max_len;
	struct mem_used_info info;
	u32 s0_total = (s0_end - s0_start)/1024/1024;
	u32 s1_total = (s1_end - s1_start)/1024/1024;

	mem_usage(tdev, &info);
	pos += sprintf(buf + pos, "S0:  %d MB used   %d MB free   %d.%02d%% used\n",
		info.s0_used, info.s0_free,
		(info.s0_used*10000)/(s0_total)/100, (info.s0_used*10000)/(s0_total)%100);
	pos += sprintf(buf + pos, "S1:  %d MB used   %d MB free   %d.%02d%% used\n",
		info.s1_used, info.s1_free,
		(info.s1_used*10000)/(s1_total)/100, (info.s1_used*10000)/(s1_total)%100);

	for (j = 0; j < cnt; j++) {
		pos += sprintf(buf + pos, "block:");
		for (i = 0; i < max_len; i++)
			pos += sprintf(buf + pos, "  %2d", i+j*max_len);
		pos += sprintf(buf + pos, "\n");

		pos += sprintf(buf + pos, "   S0:");
		for (i = 0; i < max_len; i++)
			pos += sprintf(buf + pos, "  %2d", s0_bk[i+j*max_len].task_id);
		pos += sprintf(buf + pos, "\n");

		pos += sprintf(buf + pos, "   S1:");
		for (i = 0; i < max_len; i++)
			pos += sprintf(buf + pos, "  %2d", s1_bk[i+j*max_len].task_id);
		pos += sprintf(buf + pos, "\n");
	}
	if (remainder) {
		pos += sprintf(buf + pos, "block:");
		for (i = 0; i < remainder; i++)
			pos += sprintf(buf + pos, "  %2d", i+j*max_len);
		pos += sprintf(buf + pos, "\n");

		pos += sprintf(buf + pos, "   S0:");
		for (i = 0; i < remainder; i++)
			pos += sprintf(buf + pos, "  %2d", s0_bk[i+j*max_len].task_id);
		pos += sprintf(buf + pos, "\n");

		pos += sprintf(buf + pos, "   S1:");
		for (i = 0; i < remainder; i++)
			pos += sprintf(buf + pos, "  %2d", s1_bk[i+j*max_len].task_id);
		pos += sprintf(buf + pos, "\n");
	}

	return pos;
}


static DEVICE_ATTR_RO(mem_info);

static struct attribute *transmem_sysfs_entries[] = {
	&dev_attr_mem_info.attr,
	NULL
};

static struct attribute_group transmem_attribute_group = {
	.name = NULL,
	.attrs = transmem_sysfs_entries,
};

int cb_mem_init(struct cb_tranx_t *tdev)
{
	int ret = -EFAULT;
	u32 blk_size, i, j;
	void *block;
	void *hlina_chunks;
	struct mem_block *mem_s0_bk;
	struct mem_block *mem_s1_bk;
	u64 tmp_addr;
	struct memory_t *tmem;

	tmem = kzalloc(sizeof(struct memory_t), GFP_KERNEL);
	if (!tmem) {
		trans_dbg(tdev, TR_ERR, "mem: alloc tmem error\n");
		goto out;
	}
	tdev->modules[TR_MODULE_MEMORY] = tmem;
	tmem->tdev = tdev;

	block = kzalloc(
		sizeof(struct mem_block) * (S0_BLOCK_CNT + S1_BLOCK_CNT),
		GFP_KERNEL);
	if (!block) {
		trans_dbg(tdev, TR_ERR, "mem: alloc block error\n");
		goto out_free_dev;
	}

	hlina_chunks = vzalloc((s0_end - s0_start + s1_end - s1_start)
				/ CHUNK_SIZE * sizeof(struct hlina_chunk));
	if (!hlina_chunks) {
		trans_dbg(tdev, TR_ERR, "mem: alloc hlina_chunks failed\n");
		goto out_free_slice;
	}

	mem_s0_bk = block;
	blk_size = ((s0_end - s0_start) / S0_BLOCK_CNT) & (~(CHUNK_SIZE - 1));
	trans_dbg(tdev, TR_DBG,
		"memory : s0_start:0x%08lx s0_size:0x%lx block_cnt:%d, block_size:0x%lx,chunks:%ld.\n",
		s0_start, s0_end - s0_start, S0_BLOCK_CNT,
		blk_size, blk_size * S0_BLOCK_CNT / CHUNK_SIZE);
	for (i = 0; i < S0_BLOCK_CNT; i++) {
		mem_s0_bk[i].index = i;
		mem_s0_bk[i].task_id = 0;
		mem_s0_bk[i].block_size = blk_size;
		mem_s0_bk[i].start_addr = s0_start + blk_size * i;
		mem_s0_bk[i].chunks = mem_s0_bk[i].block_size / CHUNK_SIZE;
		mem_s0_bk[i].free_chunks = mem_s0_bk[i].chunks;
		mem_s0_bk[i].chk_inf = hlina_chunks;
		hlina_chunks +=
			sizeof(struct hlina_chunk) * mem_s0_bk[i].chunks;
		tmp_addr = mem_s0_bk[i].start_addr;
		for (j = 0; j < mem_s0_bk[i].chunks; j++) {
			mem_s0_bk[i].chk_inf[j].bus_add = tmp_addr;
			mem_s0_bk[i].chk_inf[j].ck_rsv = 0;
			tmp_addr += CHUNK_SIZE;
		}
	}

	mem_s1_bk = block + sizeof(struct mem_block) * S0_BLOCK_CNT;
	blk_size = ((s1_end - s1_start) / S1_BLOCK_CNT) & (~(CHUNK_SIZE - 1));
	trans_dbg(tdev, TR_DBG,
		"memory : s1_start:0x%08lx s1_size:0x%lx block_cnt:%d, block_size:0x%lx,chunks:%ld.\n",
		s1_start, s1_end - s1_start, S1_BLOCK_CNT,
		blk_size, blk_size * S0_BLOCK_CNT / CHUNK_SIZE);
	for (i = 0; i < S1_BLOCK_CNT; i++) {
		mem_s1_bk[i].index = i;
		mem_s1_bk[i].task_id = 0;
		mem_s1_bk[i].block_size = blk_size;
		mem_s1_bk[i].start_addr = s1_start + blk_size * i;
		mem_s1_bk[i].chunks = mem_s1_bk[i].block_size / CHUNK_SIZE;
		mem_s1_bk[i].free_chunks = mem_s1_bk[i].chunks;
		mem_s1_bk[i].chk_inf = hlina_chunks;
		hlina_chunks +=
			sizeof(struct hlina_chunk) * mem_s1_bk[i].chunks;
		tmp_addr = mem_s1_bk[i].start_addr;
		for (j = 0; j < mem_s1_bk[i].chunks; j++) {
			mem_s1_bk[i].chk_inf[j].bus_add = tmp_addr;
			mem_s1_bk[i].chk_inf[j].ck_rsv = 0;
			tmp_addr += CHUNK_SIZE;
		}
	}

	tmem->mem_s0_bk = mem_s0_bk;
	tmem->mem_s1_bk = mem_s1_bk;
	tmem->s0_rev_size = 0;
	tmem->s1_rev_size = 0;

	mutex_init(&tmem->mem_mutex_ep);
	mutex_init(&tmem->mem_mutex_rc);
	spin_lock_init(&tmem->taskid_lock);

	ret = sysfs_create_group(&tdev->misc_dev->this_device->kobj,
				 &transmem_attribute_group);
	if (ret) {
		trans_dbg(tdev, TR_ERR, "mem: failed to create sysfs entry\n");
		goto out_free_chunk;
	}

	trans_dbg(tdev, TR_INF, "mem: module initialize done.\n");
	return 0;

out_free_chunk:
	vfree(hlina_chunks);
out_free_slice:
	kfree(block);
out_free_dev:
	kfree(tmem);
out:
	trans_dbg(tdev, TR_ERR, "mem: module initialize failed.\n");
	return ret;
}

int cb_mem_release(struct cb_tranx_t *tdev)
{
	struct memory_t *tmem = tdev->modules[TR_MODULE_MEMORY];

	mutex_destroy(&tmem->mem_mutex_ep);
	mutex_destroy(&tmem->mem_mutex_rc);

	vfree(tmem->mem_s0_bk[0].chk_inf);
	kfree(tmem->mem_s0_bk);

	sysfs_remove_group(&tdev->misc_dev->this_device->kobj,
			   &transmem_attribute_group);

	kfree(tmem);
	trans_dbg(tdev, TR_DBG, "mem: remove module done.\n");

	return 0;
}

static int check_task_id(struct memory_t *tmem, int task_id)
{
	if ((task_id > MAX_TASK_NUM) || (task_id < MIN_TASK_ID)
	    || (tmem->id_st[task_id] == ID_FREE)) {
		return -EFAULT;
	} else
		return 0;
}
long cb_mem_ioctl(struct file *filp,
		      unsigned int cmd,
		      unsigned long arg,
		      struct cb_tranx_t *tdev)
{
	int ret = 0;
	int id;
	struct mem_used_info info;
	struct mem_info memp;
	unsigned long addr;
	void __user *argp = (void __user *)arg;
	struct memory_t *tmem = tdev->modules[TR_MODULE_MEMORY];

	switch (cmd) {
	case CB_TRANX_MEM_GET_UTIL:
		mem_usage(tdev, &info);
		if (__copy_to_user(argp, &info, sizeof(info))) {
			trans_dbg(tdev, TR_ERR,
				"mem: get mem usage - copy_to_user failed.\n");
			ret = -EFAULT;
		}
		break;
	case CB_TRANX_MEM_ALLOC:
		if (copy_from_user(&memp, argp, sizeof(memp))) {
			trans_dbg(tdev, TR_ERR,
				"mem: get mem - copy_from_user failed.\n");
			return -EFAULT;
		}
		if (memp.mem_location) {
			trans_dbg(tdev, TR_ERR,
				"mem: get mem - mem_location error.\n");
			return -EFAULT;
		}
		if (check_task_id(tmem, memp.task_id)) {
			trans_dbg(tdev, TR_ERR,
				"mem: get mem, id:%d is error\n", memp.task_id);
			return -EFAULT;
		}

		if (mutex_lock_interruptible(&tmem->mem_mutex_ep))
			return -ERESTARTSYS;
		ret = alloc_mem_ep(&addr, memp.size, memp.task_id, tmem);
		mutex_unlock(&tmem->mem_mutex_ep);

		if (ret) {
			trans_dbg(tdev, TR_ERR, "mem: alloc memory failed.\n");
			return -EFAULT;
		}
		memp.phy_addr = addr;
		if (__copy_to_user(argp, &memp, sizeof(memp))) {
			trans_dbg(tdev, TR_ERR,
				"mem: get mem, copy_to_user failed, then free it\n");
			free_mem_ep(addr, memp.size, memp.task_id, tmem);
			return -EFAULT;
		}

		break;
	case CB_TRANX_MEM_FREE:
		if (copy_from_user(&memp, argp, sizeof(memp))) {
			trans_dbg(tdev, TR_ERR,
				"mem: free mem - copy_from_user failed.\n");
			return -EFAULT;
		}
		if (memp.mem_location) {
			trans_dbg(tdev, TR_ERR,
				"mem: free mem - mem_location error.\n");
			return -EFAULT;
		}
		if (check_task_id(tmem, memp.task_id)) {
			trans_dbg(tdev, TR_ERR,
				"mem: free mem,id:%d is error\n", memp.task_id);
			return -EFAULT;
		}

		if (mutex_lock_interruptible(&tmem->mem_mutex_ep))
			return -ERESTARTSYS;
		ret = free_mem_ep(memp.phy_addr, memp.size, memp.task_id, tmem);
		mutex_unlock(&tmem->mem_mutex_ep);

		if (ret) {
			trans_dbg(tdev, TR_ERR, "mem: free memory failed\n");
			return -EFAULT;
		}
		break;
	case CB_TRANX_MEM_GET_TASKID:
		id = get_task_id(tmem, filp);
		if (id == -1) {
			trans_dbg(tdev, TR_ERR, "mem: can't get valid id\n");
			return -EFAULT;
		}
		__put_user(id, (int *)argp);
		break;
	case CB_TRANX_MEM_FREE_TASKID:
		__get_user(id, (int *)argp);
		if (check_task_id(tmem, id)) {
			trans_dbg(tdev, TR_ERR,
				"mem: free id, id:%d is error\n", id);
			return -EFAULT;
		}
		free_task_id(tmem, id);
		break;
	default:
		trans_dbg(tdev, TR_ERR,
			  "mem: %s, cmd:0x%x is error.\n", __func__, cmd);
		ret = -EINVAL;
	}

	return ret;
}
