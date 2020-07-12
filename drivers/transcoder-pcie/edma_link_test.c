#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <hugetlbfs.h>
#include "transcoder.h"

typedef unsigned char				u8;
typedef unsigned short				u16;
typedef unsigned int				u32;

#define QWORD_HI(value)		((value>>32)&0xFFFFFFFF)
#define QWORD_LO(value)		(value&0xFFFFFFFF)

#define ENABLE_HUGEPAGE
#define EACH_ELEMENT_SIZE	0x16000

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
/* macro to get greater of two values */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int create_link_table(struct dma_link_table *link_table, unsigned long rc_addr, unsigned long ep_addr, int size)
{
	int i = 0;
	int j;
	
	while (size)
	{
		link_table[i].control = 0x1;
		link_table[i].dst_high = QWORD_HI(ep_addr);
		link_table[i].dst_low = QWORD_LO(ep_addr);
		link_table[i].sar_high = QWORD_HI(rc_addr);
		link_table[i].sar_low = QWORD_LO(rc_addr);
		link_table[i].size = MIN(EACH_ELEMENT_SIZE, size);
		ep_addr += link_table[i].size;
		rc_addr += link_table[i].size;
		size -= link_table[i].size;
		i++;
	}

#if 0
	for (j = 0; j <= i; j++)
		printf("i=%3d control=0x%x sar_h=0x%x sar_l=0x%x dst_h=0x%x dst_l=0x%x size=0x%x \n",
		j, link_table[j].control, link_table[j].sar_high, link_table[j].sar_low, link_table[j].dst_high, link_table[j].dst_low, link_table[j].size);
#endif
	return i;
}

int main(int argc, char **argv)
{
	int task_id;
	struct mem_info ep_region;

	int fd = -1;
	int i = 0,flag, ret, status;
	unsigned char *virtual_src = MAP_FAILED;
	unsigned char *virtual_dst = MAP_FAILED;
	struct dma_link_table *link_table = MAP_FAILED;

	unsigned long tmp_addr,ep_addr;
	int size,round,save_round;
	struct trans_pcie_edma edma_trans;
	int enable_hugepage = 0, slice, element_size;
	
	unsigned char val=0x12;
	
	if(argc <7)
	{
		printf("%s dev_node r/w size round enable_hugepage slice [val];  w:rc2ep, r:ep2rc, e:rw ; enable_hugepage 1:enable, 0:disable.\n",argv[0]);
		return -1;
	}

	fd = open(argv[1], O_RDWR);
	if(fd == -1)
	{
		printf("Failed to open dev: %s\n", argv[1]);
		return -1;
	}

	if(strcmp(argv[2],"w") == 0)
	{
		flag = 0x1; //rc2ep
	}
	else if(strcmp(argv[2],"r") == 0)
	{
		flag = 0x0; //ep2rc
	}
	else if(strcmp(argv[2],"e") == 0)
	{
		flag = 0x3; //rc2ep - ep2rc
	}
	else
	{
		printf("diretion:%s is error\n", argv[2]);
		return -1;
	}

	
	link_table = (struct dma_link_table *)malloc(0x20000);
	if (!link_table)
	{
		perror("Err mmap link_table:");
		goto out_0;
	}
	memset(link_table, 0x0, 0x10000);
	
	size = strtoul(argv[3], 0, 0);
	if (ioctl(fd, CB_TRANX_MEM_GET_TASKID, &task_id) < 0) //get mem task id
	{
		printf("ioctl get task id failed.\n");
		return -1;
	}
//	printf("direct:%s task_id=%d size:0x%x flag=0x%x.\n",argv[2],task_id,size,flag);
	
	ep_region.mem_location = EP_SIDE;
	ep_region.size = size; 
	ep_region.task_id = task_id;
	if (ioctl(fd, CB_TRANX_MEM_ALLOC, &ep_region) < 0) //get ep mem
	{
		printf("ioctl get ep mem failed.\n");
		return -1;
	}
	ep_addr = ep_region.phy_addr;
//	printf("EP address=0x%lx \n",ep_region.phy_addr);

	round = strtoul(argv[4], 0, 0);

	if(strcmp(argv[5],"1") == 0)
	{
		enable_hugepage = 0x1;
	}

	status = slice = strtoul(argv[6], 0, 0);

	if (argc ==8)
		val =  strtoul(argv[7], 0, 0);

	if(enable_hugepage)	
		virtual_src = get_huge_pages(size, GHP_DEFAULT);
	else
		virtual_src = malloc(size);
	if (!virtual_src)
	{
		perror("Err mmap virtual_src:");
		goto out_0;
	}

	if(enable_hugepage)	
		virtual_dst = get_huge_pages(size, GHP_DEFAULT);
	else
		virtual_dst = malloc(size);
	if (!virtual_dst)
	{
		perror("Err mmap virtual_dst: ");
		goto out_0;
	}

	memset(virtual_src,val,size);
	memset(virtual_dst,0x0,size);
	
	if(size < 0)
		goto	out_0;

//	printf("round=%d size=0x%x virtual_src=%p virtual_dst=%p ep_addr=0x%x slice=%d. \n",
//		round,size, virtual_src, virtual_dst, ep_addr, slice);
	save_round = round;

	if (ioctl(fd, CB_TRANX_REQUEST_TC, &slice) < 0)
	{
		printf("tcache %d request failed\n", slice);
		goto	out_0;
	}

//	while (round--)
	{
		if ((flag == 0x1) || (flag == 0x3))
		{
			element_size = create_link_table(link_table, ( unsigned long)virtual_src, ep_addr, size);
			edma_trans.slice = slice;
			edma_trans.direct = RC2EP;
			edma_trans.element_size = element_size;
			edma_trans.rc_ult = (__u64)link_table;
			ret = ioctl(fd, CB_TRANX_EDMA_TRANX_TCACHE, &edma_trans);
			if (ret < 0) {
				printf("edma link transmit failed!\n");
				goto out_0;
			}
			printf("round=%d direct:%s size=0x%x virtual_src=%p virtual_dst=%p ep_addr=0x%08x val=0x%02x slice=%d. \n",
				round, argv[2], size, virtual_src, virtual_dst, ep_addr, val, slice);
		}
		
		if ((flag == 0x0) || (flag == 0x3))
		{
			tmp_addr = ( unsigned long)virtual_dst;
			edma_trans.sar_low = QWORD_LO(ep_addr);
			edma_trans.sar_high = QWORD_HI(ep_addr);
			edma_trans.dar_low = QWORD_LO(tmp_addr);
			edma_trans.dar_high = QWORD_HI(tmp_addr);
			edma_trans.direct = EP2RC;
			edma_trans.size = size;
			edma_trans.interrupt_enable = INTE_EN;//POLL_EN;

			if (ioctl(fd, CB_TRANX_EDMA_TRANX, &edma_trans) < 0) //transmit via edma
			{
				printf("ioctl EP2RC failed\n");
				goto out_0;
			}
			printf("round=%d virtual_dst=%p val_tg=0x%x, ep_addr=0x%lx. \n",
				save_round-round,virtual_dst, *(unsigned int*)virtual_dst, ep_addr);
		}
		if (flag == 0x3)
		{
			for(i=0;i<size;i++)
			{
				if(*(virtual_dst+i) != *(virtual_src+i))
				{
					printf("compare failed, i=0x%x \n",i);
					break;
				}
			}
			if(i==size)
				printf("round:%d compare successfully, size=0x%x, val_tg=0x%08x\n",
				save_round-round, size, *(u32*)(virtual_dst+size/4));
		}
	}
	
out_0:
	if (ioctl(fd, CB_TRANX_EDMA_STATUS, &status))
		printf("edma link get status failed for slice %d\n", slice);
	
	if (ioctl(fd, CB_TRANX_RELEASE_TC, &slice) < 0) {
		printf("tcache %d release failed\n", slice);
	}

	ioctl(fd, CB_TRANX_MEM_FREE, &ep_region);
	ioctl(fd, CB_TRANX_MEM_FREE_TASKID, &task_id);

	if(enable_hugepage)	
		free_huge_pages(virtual_src);
	else
		free(virtual_src);
	
	if(enable_hugepage)	
		free_huge_pages(virtual_dst);
	else
		free(virtual_dst);

	free(link_table);
	close(fd);

	return 0;
}
