/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Holdings Co., Ltd. All rights reserved        --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#ifndef FB_PERFORMANCE_H
#define FB_PERFORMANCE_H

#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/syscall.h>

#include "base_type.h"
#ifdef FB_SYSLOG_ENABLE
#include "syslog_sink.h"
#endif

#ifndef __GET_TID__
#define __GET_TID__
#define gettid() syscall(__NR_gettid)
#endif

#ifdef FB_SYSLOG_ENABLE
#define STATDEBUG(inst,fmt, ...) { \
        FB_SYSLOG(inst,SYSLOG_SINK_LEV_DEBUG_SW,"%s(%d): " fmt,\
                  __FUNCTION__, __LINE__, ## __VA_ARGS__);}
#define STATERROR(inst,fmt, ...) { \
        FB_SYSLOG(inst,SYSLOG_SINK_LEV_ERROR,"%s(%d): " fmt,\
                  __FUNCTION__, __LINE__, ## __VA_ARGS__);}
#define STATSTAT(inst,fmt, ...) { \
        FB_SYSLOG(inst,SYSLOG_SINK_LEV_STAT, fmt, ## __VA_ARGS__);}
#else
#define STATDEBUG(inst,fmt, ...) fprintf(stdout, "%s(%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#define STATERROR(inst,fmt, ...) fprintf(stderr, "%s(%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#define STATSTAT(inst,fmt, ...) fprintf(stdout, "%s(%d): " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#endif


#ifdef FB_PERFORMANCE_STATIC
#define RECORD_MAX 10
#define DECLARE_PERFORMANCE_STATIC(name) \
  u32 name##_count; \
  i64 name##_total; \
  i32 name##_val[RECORD_MAX]; \
  long int name##_tid; \
  struct timeval name##_start; \
  struct timeval name##_end; \
  int name##need_continue; \
  int name##need_end; \
  pthread_mutex_t name##_mutex;
#define PERFORMANCE_STATIC_INIT(log,inst,name) { \
  pthread_mutex_init(&inst->name##_mutex, NULL); \
  inst->name##_count = 0; \
  inst->name##_total = 0; \
  inst->name##need_continue = 0; \
  inst->name##need_end = 0; \
}
#define PERFORMANCE_STATIC_START(log,inst,name) { \
  pthread_mutex_lock(&inst->name##_mutex); \
  if (!inst->name##need_continue) { \
  	inst->name##_tid = gettid(); \
  	gettimeofday(&inst->name##_start, NULL); \
  	if (0) {STATSTAT(log,#name" %d start(%lu)\n",inst->name##_count,inst->name##_start.tv_sec*1000000+inst->name##_start.tv_usec);} \
  	inst->name##need_continue = 1; \
  	inst->name##need_end = 1; \
  } else { \
  	if (0) {STATDEBUG(log,#name" continue\n");fflush(stdout);} \
  } \
  pthread_mutex_unlock(&inst->name##_mutex); \
}
#define PERFORMANCE_STATIC_END(log,inst,name) { \
  pthread_mutex_lock(&inst->name##_mutex); \
  gettimeofday(&inst->name##_end, NULL); \
  inst->name##need_continue = 0; \
  if (inst->name##need_end) { \
  	if (0) {STATSTAT(log,#name" %d end(%lu)\n",inst->name##_count,inst->name##_end.tv_sec*1000000+inst->name##_end.tv_usec);} \
  	i32 value = (inst->name##_end.tv_sec-inst->name##_start.tv_sec)*1000000 \
  	+(inst->name##_end.tv_usec-inst->name##_start.tv_usec); \
  	if (inst->name##_count < RECORD_MAX) \
  		inst->name##_val[inst->name##_count] = value; \
  	inst->name##_count++; \
  	inst->name##_total+= value; \
  	inst->name##need_end = 0; \
  } else { \
    if (0) {STATDEBUG(log,#name" %d status error\n",inst->name##_count);fflush(stdout);} \
  } \
  pthread_mutex_unlock(&inst->name##_mutex); \
}
#define PERFORMANCE_STATIC_REPORT(log,inst,name) { \
  pthread_mutex_destroy(&inst->name##_mutex); \
  STATSTAT(log,#name" count: %d, total: %ld, %ld pertime\n", \
			inst->name##_count, inst->name##_total, \
			inst->name##_count ? inst->name##_total/inst->name##_count : 0); \
}
#define PERFORMANCE_STATIC_VERBOSE(log,inst,name) { \
  int i; \
  if (0) { \
    for (i = 0; i < ((inst->name##_count < RECORD_MAX)? inst->name##_count : RECORD_MAX); i++) { \
    	STATSTAT(log,#name"[%03d] = %d\n", i, inst->name##_val[i]); \
    } \
  } \
}
#define PERFORMANCE_STATIC_GET_START(log,inst,name) \
  (((u64)(inst->name##_start.tv_sec) * 1000000) + inst->name##_start.tv_usec)
#define PERFORMANCE_STATIC_GET_TOTAL(log,inst,name) \
  inst->name##_total
#define PERFORMANCE_STATIC_OVERLAP_END(log,inst,name0,name1,name2,name3,name) { \
  if (0) {STATSTAT(log,#name0" %d\n", inst->name0##need_end);} \
  if (0) {STATSTAT(log,#name1" %d\n", inst->name1##need_end);} \
  if (0) {STATSTAT(log,#name2" %d\n", inst->name2##need_end);} \
  if (0) {STATSTAT(log,#name3" %d\n", inst->name3##need_end);} \
  if (!(inst->name0##need_end || inst->name1##need_end || inst->name2##need_end || inst->name3##need_end)) { \
    if (0) {STATSTAT(log,"to end name\n");} \
    PERFORMANCE_STATIC_END(log,inst,name); \
  } \
}
#else
#define DECLARE_PERFORMANCE_STATIC(name)
#define PERFORMANCE_STATIC_INIT(log,inst,name)
#define PERFORMANCE_STATIC_START(log,inst,name)
#define PERFORMANCE_STATIC_END(log,inst,name)
#define PERFORMANCE_STATIC_REPORT(log,inst,name)
#define PERFORMANCE_STATIC_VERBOSE(log,inst,name)
#define PERFORMANCE_STATIC_GET_START(log,inst,name) 0
#define PERFORMANCE_STATIC_GET_TOTAL(log,inst,name) 0
#define PERFORMANCE_STATIC_OVERLAP_END(log,inst,name0,name1,name2,name3,name)
#endif


#endif /* TB_SW_PERFORMANCE_H */

