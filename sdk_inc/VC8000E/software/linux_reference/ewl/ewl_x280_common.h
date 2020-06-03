/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------
--
--  Abstract : H2 Encoder Wrapper Layer for OS services
--
------------------------------------------------------------------------------*/
#ifndef __EWL_X280_COMMON_H__
#define __EWL_X280_COMMON_H__

#include <stdio.h>
#include <signal.h>
#include <semaphore.h>
#include <queue.h>

#if defined(SUPPORT_FBIPS) || !defined(USE_OLD_DRV)
#include "fb_ips.h"
#endif
#ifndef USE_OLD_DRV
#include "trans_edma_api.h"
#include "dec400_f2_api.h"
#include "l2cache_api.h"
#include "dtrc_api.h"
#include "tcache_api.h"
#endif
#include "fb_performance.h"

#ifdef ewl_trace_file
extern FILE *fEwl;
#endif

/* Macro for debug printing */
#undef PTRACE
#ifdef FB_SYSLOG_ENABLE
#include "syslog_sink.h"
#define PTRACE(fmt, ...) FB_SYSLOG(ewl,SYSLOG_SINK_LEV_DEBUG_SW,"%s(%d): " fmt,\
                                    __FUNCTION__, __LINE__, ## __VA_ARGS__)
#define ERROR_PRINT(fmt, ...) FB_SYSLOG(ewl,SYSLOG_SINK_LEV_ERROR,"%s(%d): " fmt,\
                                    __FUNCTION__, __LINE__, ## __VA_ARGS__)
#define REG_DEBUG(fmt, ...) FB_SYSLOG(ewl,SYSLOG_SINK_LEV_DEBUG_M_IP,"%s(%d): " fmt,\
                                    __FUNCTION__, __LINE__, ## __VA_ARGS__)
#else

#ifdef TRACE_EWL
#   include <stdio.h>
#ifdef ewl_trace_file
#   define PTRACE(...) \
  { if(fEwl == NULL) \
        fEwl = fopen("ewl.trc", "w"); \
    if (fEwl) {fprintf(fEwl,"%s:%d:",__FILE__,__LINE__);fprintf(fEwl,__VA_ARGS__);} \
  }
#else
#   define PTRACE(...) {printf("%s:%d:",__FILE__,__LINE__);printf(__VA_ARGS__);}
#endif
#else
#   define PTRACE(...)  /* no trace */
#endif

#endif

#ifndef DRV_NEW_ARCH
/* the encoder device driver nod */
#ifndef MEMALLOC_MODULE_PATH
#define MEMALLOC_MODULE_PATH        "/tmp/dev/memalloc"
#endif

#ifndef ENC_MODULE_PATH
#define ENC_MODULE_PATH             "/tmp/dev/transcoder_codec"
#endif

#ifndef SDRAM_LM_BASE
#define SDRAM_LM_BASE               0x00000000
#endif
#endif

typedef struct
{
 i32 core_id; //physical core id
 u32 regSize;             /* IO mem size */
#ifndef NEW_IOCTL_ENABLE
 volatile u32 *pRegBase;  /* IO mem base */
#endif
}regMapping;

typedef struct {
  struct node *next;
  int core_id;
} EWLWorker;
#define FIRST_CORE(inst) (((EWLWorker *)(inst->workers.tail))->core_id)
#define LAST_CORE(inst)  (((EWLWorker *)(inst->workers.head))->core_id)
#if 0
typedef struct ip_info {
  ptr_t ip_base;
  u32 ip_regsize;
  void * ip_vbase;
  void * ip_handle;
} ip_info_t;
#endif
/* EWL internal information for Linux */
typedef struct
{
#ifdef FB_SYSLOG_ENABLE
  LOG_INFO_HEADER log_header;
  char module_name[10];
#endif
    int pass;
    u32 clientType;
#ifdef DRV_NEW_ARCH
    int fd_trans;
    int mem_id;
    int priority;
    char * device;
#else
#ifdef USE_OLD_DRV
    int fd_mem;              /* /dev/mem */
#endif
    int fd_enc;              /* /dev/hx280 */
    int fd_memalloc;         /* /dev/memalloc */
#endif
    regMapping reg; //register for reserved cores
    regMapping *reg_all_cores;
    u32 performance;
    struct queue freelist;
    struct queue workers;

    /* loopback line buffer in on-chip SRAM*/
    u32 lineBufSramBase;  /* bus addr */
    volatile u32 *pLineBufSram; /* virtual addr */
    u32 lineBufSramSize;

#if 0//def SUPPORT_FBIPS
	int fd_fb;
	int ip_nums;
	ip_info_t * ips;
#endif
#ifndef USE_OLD_DRV
  EDMA_HANDLE edma_handle;
  F2_DEC_HANDLE f2dec_handle;
  L2CACHE_HANDLE vcel2r_handle;
  DTRC_HANDLE dtrc_handle;
  L2CACHE_HANDLE dtrcl2r_handle;
  TCACHE_HANDLE tcache_handle;
#endif

  ENCPERF * perf;

#ifdef VCE_MEM_ERR_TEST
  i32 *vce_memory_err_shadow;
  i32 *vce_memory_err_cnt;
#endif
#ifdef VCE_EDMA_ERR_TEST
  i32 *vce_edma_err_shadow;
  i32 *vce_edma_err_cnt;
#endif
} hx280ewl_t;


#endif /* __EWLX280_COMMON_H__ */
