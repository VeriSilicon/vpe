#ifndef SYSLOG_SINK_H
#define SYSLOG_SINK_H


#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/syscall.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GET_TID__
#define __GET_TID__
#define gettid() syscall(__NR_gettid)
#endif

#if 0
#define SYSLOG_SINK_LEV_ERROR       LOG_ERR
#define SYSLOG_SINK_LEV_INFO        LOG_WARNING
#define SYSLOG_SINK_LEV_DEBUG_M_IP  LOG_NOTICE
#define SYSLOG_SINK_LEV_DEBUG_S_IP  LOG_INFO
#define SYSLOG_SINK_LEV_DEBUG_SW    LOG_DEBUG

#define SYSLOG_SINK_LEV_ALL         LOG_DEBUG

#else
#define SYSLOG_SINK_LEV_ERROR       3
#define SYSLOG_SINK_LEV_STAT        4
#define SYSLOG_SINK_LEV_INFO        5
#define SYSLOG_SINK_LEV_DEBUG_SW    6
#define SYSLOG_SINK_LEV_DEBUG_SW_VERBOSE 7
#define SYSLOG_SINK_LEV_DEBUG_M_IP  8
#define SYSLOG_SINK_LEV_DEBUG_S_IP  9

#define SYSLOG_SINK_LEV_ALL         SYSLOG_SINK_LEV_DEBUG_S_IP

#endif


/*
 *   module_name :          "VCD" "VCE" "BIGSEA" "F1" "F2" "F3" "F4" or some others you like ..
 *   device_id :         ASIC board device id
 */
typedef struct log_info_header {
  char * module_name;
  int device_id;
} LOG_INFO_HEADER;


/*
*level:LOG_ERR-LOG_DEBUG
*char*module_name
*
* eg.FB_SYSLOG(0, pid, SYSLOG_SINK_LEV_ERROR,"VCD","in %s : %d, pid = %d SYSLOG_SINK_LEV_ERROR\n",__func__,__LINE__,pid);
* Trans[xx][pid][module_name]......
* eg. * Trans[00][2024][VCD]......
*
*kill -USR2(-USR1) pid can add/reduce syslog by changing syslog_sink_threshold
*syslog_sink_threshold default value is SYSLOG_SINK_LEV_DEBUG, which means on output SYSLOG_SINK_LEV_DEBUG level
*if add syslog, kill -USR1 pid, otherwise kill -USR2 pid
*
*
*/
#if 0
#define FB_SYSLOG(header,level,fmt,arg...)   \
do { \
  syslog(LOG_USER | level, "Trans[%02d][%ld][%s]" fmt, \
  header ? ((LOG_INFO_HEADER *)header)->device_id : -1, (long int)gettid(), \
  header ? ((LOG_INFO_HEADER *)header)->module_name : NULL, ##arg); \
} while(0);
#else
#if 0
#define FB_SYSLOG(header,level,fmt,arg...)   \
do { \
  extern int syslog_sink_threshold; \
  extern FILE * flog; \
  pid_t pid = getpid(); \
  long int tid = gettid(); \
  int device_id; \
  char * module_name; \
  if (header) { \
    device_id = ((LOG_INFO_HEADER *)header)->device_id; \
    module_name = ((LOG_INFO_HEADER *)header)->module_name; \
  } else { \
    device_id = -1; \
    module_name = NULL; \
  } \
  if (level <= syslog_sink_threshold) { \
    fprintf(flog, "Trans[%02d][%d][%ld][L%d][%s]" fmt, \
    device_id, pid, tid, level, module_name, ##arg); \
  } \
  if (level == SYSLOG_SINK_LEV_ERROR) { \
    fprintf(stderr, "Trans[%02d][%d][%ld][%s]" fmt, \
    device_id, pid, tid, module_name, ##arg); \
  } \
  if (level == SYSLOG_SINK_LEV_INFO) { \
    fprintf(stdout, "Trans[%02d][%d][%ld][%s]" fmt, \
    device_id, pid, tid, module_name, ##arg); \
  } \
} while(0);
#endif
#endif

/*
*return 0 ok, or fail
*/
int init_syslog_module(char * ident, int default_syslog_sink_threshold);
/*
*return 0 ok, or fail
*/
int close_syslog_module(void);

int change_log_level(int loglevel);

extern int syslog_sink_threshold;

void SYSLOG(const void * inst, int level, const char * fmt, ...);

#define FB_SYSLOG(inst, level, ...)                                        \
    do {                                                                   \
        if (syslog_sink_threshold >= level) {                              \
            SYSLOG(inst, level, __VA_ARGS__);                           \
        }                                                                  \
    } while (0)

void FB_SYSLOG_FLUSH();

int get_deviceId(const char * dev);

#ifdef __cplusplus
}
#endif

#endif
