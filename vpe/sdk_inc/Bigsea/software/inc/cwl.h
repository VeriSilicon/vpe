/* Copyright 2016 Google Inc. All Rights Reserved. */

/*------------------------------------------------------------------------------
--
--  Abstract : Codec Wrapper Layer for OS services
--
------------------------------------------------------------------------------*/

#ifndef __CWL_H__
#define __CWL_H__

#include "basetype.h"
#include "cwlthread.h"
#ifdef __cplusplus
extern "C"
{
#endif
#ifdef FB_PERFORMANCE_STATIC
#include "fb_performance.h"
#endif

#define MAX_ASIC_CORES 4


/* Return values */
enum CWLCoreStatus {
  CWL_OK = 0,
  CWL_ERROR = -1,
  CWL_HW_WAIT_TIMEOUT = 1,
  CWL_FREE = 2
};
#define CWL_OK                      0
#define CWL_ERROR                  -1

#define CWL_HW_WAIT_OK              CWL_OK
#define CWL_HW_WAIT_ERROR           CWL_ERROR
#define CWL_HW_WAIT_TIMEOUT         1

#define CWL_HW_CONFIG_NOT_SUPPORTED    0
#define CWL_HW_CONFIG_ENABLED          1

/* Flags for CWLMarkMemory direction */
#define CWL_HW_MARK_DEVICE_ONLY          0
#define CWL_HW_MARK_HOST_TO_DEVICE       (1 << 0)
#define CWL_HW_MARK_DEVICE_TO_HOST       (1 << 1)
/* Mark that the data needs to be copied only once. After the data is copied,
 * the marked region changes to CWL_HW_MARK_DEVICE_ONLY. */
#define CWL_HW_MARK_ONE_SHOT             (1 << 2)

/* Guaranteed minimum alignment of bus addresses returned from CWLMallocLinear
 * and CWLMallocRefFrm. */
#define CWL_MIN_BUS_ADDRESS_ALIGNMENT 64

/* Hardware configuration description */
    typedef struct CWLHwConfig
    {
        u32 maxEncodedWidth; /* Maximum supported width for video encoding (not JPEG) */
        u32 vp9Enabled;      /* HW supports VP9 */
        u32 max_dec_pic_width;
        u32 max_dec_pic_height;
        u32 vp9_max_bit_depth;
        u32 vp9_support;
        u32 av1_support;
        u32 h264_support;
    } CWLHwConfig_t;

/* Allocated linear memory area information */
    typedef struct CWLLinearMem
    {
        void* virtualAddress;
        size_t busAddress;
        u32 size;

#ifndef  USE_OLD_DRV
    //rc
    void*  rc_virtualAddress;
    size_t rc_busAddress;
    void * rc_kvirt; //record for free
#endif

    } CWLLinearMem_t;


  typedef struct {
    int core_usage_counts_p1[2];
    int core_usage_counts[2];
    i64 hwcycle_accp1;
    i64 hwcycle_acc;
    i64 hwcycle_acc_total;
    pthread_mutex_t hwcycle_acc_mutex;
    struct timeval last_frame_encoded_timestamp;
#ifdef FB_PERFORMANCE_STATIC
    DECLARE_PERFORMANCE_STATIC(vp9hw);
    DECLARE_PERFORMANCE_STATIC(vp9hw_p1);
    DECLARE_PERFORMANCE_STATIC(VP9_dummy_0);
    DECLARE_PERFORMANCE_STATIC(VP9_dummy_1);
    DECLARE_PERFORMANCE_STATIC(vp9hw_total);
    DECLARE_PERFORMANCE_STATIC(vp9_total);
#endif
  } VP9ENCPERF;

/* CWLInitParam is used to pass parameters when initializing the CWL */
    typedef struct CWLInitParam
    {
        u32 clientType;
		VP9ENCPERF * perf;
    	//add for new driver
#ifdef DRV_NEW_ARCH
    	int priority;
    	char * device;
    	int mem_id;
    	int cwl_index;
#endif    
	} CWLInitParam_t;

#define CWL_CLIENT_TYPE_VP9_ENC          1U



/* CWLCodecConfig is used for IP-integration encoder config. */
    typedef struct CWLCodecConfig
    {
	// Set swaps for encoder frame input data. From MSB to LSB: swap each
        // 128 bits, 64, 32, 16, 8.
        u32 input_yuv_swap;
        // Set swaps for encoder frame output (stream) data. From MSB to LSB:
        // swap each 128 bits, 64, 32, 16, 8.
        u32 output_swap;
        // Set swaps for encoder probability tables. From MSB to LSB: swap each
        // 128 bits, 64, 32, 16, 8.
        u32 prob_table_swap;
        // Set swaps for encoder context counters. From MSB to LSB: swap each
        // 128 bits, 64, 32, 16, 8.
        u32 ctx_counter_swap;
        // Set swaps for encoder statistics. From MSB to LSB: swap each 128
        // bits, 64, 32, 16, 8.
        u32 statistics_swap;
        // Set swaps for encoder foreground/background map. From MSB to LSB:
        // swap each 128 bits, 64, 32, 16, 8.
        u32 fgbg_map_swap;

        // ASIC interrupt enable.
        // This enables/disables the ASIC to generate interrupt.
        bool irq_enable;

        // AXI master read ID base. Read service type specific values are added
        // to this.
        u32 axi_read_id_base;
        // AXI master write id base. Write service type specific values are
        // added to this.
        u32 axi_write_id_base;
        // AXI maximum read burst issued by the core [1..511]
        u32 axi_read_max_burst;
        // AXI maximum write burst issued by the core [1..511]
        u32 axi_write_max_burst;

        // ASIC clock gating enable.
        // This enables/disables the ASIC to utilize clock gating.
        // If this is 'true', ASIC core clock is automatically shut down when
        // the encoder enable bit is '0' (between frames).
        bool clock_gating_enabled;

        // ASIC internal timeout period in clocks.
        // Timeout counter acts as a watchdog that asserts a timeout interrupt
        // if bus is idle for the set period while the encoder is enabled.
        // Set to zero to disable.
        u32 timeout_period;
    }CWLCodecConfig_t;

struct DECParam{
	u32 picWidth;
	u32 picHeight;
	u32 lumaAddr;
	u32 chromaAddr;
	u32 lumaTableAddr;
	u32 chromaTableAddr;
	u32 bitDepth;
	u32 tempFilter;
	u32 inputCompress;
};

/*------------------------------------------------------------------------------
    4.  Function prototypes
------------------------------------------------------------------------------*/

typedef void CWLIRQCallbackFn(void *arg, i32 core_id);


/* Read and return the HW ID register value, static implementation */
#ifdef DRV_NEW_ARCH
    u32 CWLReadAsicID(void *inst);
#else
    u32 CWLReadAsicID(void);
#endif

/* Read and return HW configuration info, static implementation */
    CWLHwConfig_t CWLReadAsicConfig(void);

/* Initialize the CWL instance
 * Returns a wrapper instance or NULL for error
 * CWLInit is called when the encoder instance is initialized */
    void *CWLInit(CWLInitParam_t * param);

/* Release the CWL instance
 * Returns CWL_OK or CWL_ERROR
 * CWLRelease is called when the encoder instance is released */
    i32 CWLRelease(void *inst, u32 encindex);

/* Read the integration-specific encoder configuration. */
    CWLCodecConfig_t CWLReadCodecConfig(void *inst);

/* Frame buffers memory */
    i32 CWLMallocRefFrm(void *inst, u32 size, CWLLinearMem_t *info);
    void CWLFreeRefFrm(void *inst, CWLLinearMem_t * info);

/* SW/HW shared memory */
    i32 CWLMallocLinear(void *inst, u32 size, u32 direction,
                        CWLLinearMem_t * info);
    void CWLFreeLinear(void *inst, CWLLinearMem_t * info);
#ifdef FPGA    
#ifndef USE_OLD_DRV
i32 CWLMallocInoutLinear(void *instance, u32 size, CWLLinearMem_t * info);
void CWLFreeInoutLinear(void *instance, CWLLinearMem_t * info);
i32 CWLMallocEpLinear(void *instance, u32 size, CWLLinearMem_t * info);
void CWLFreeEpLinear(void *instance, CWLLinearMem_t * info);
int CWLTransDataRC2EP(const void *instance, CWLLinearMem_t * buff);
int CWLTransDataEP2RC(const void *instance, CWLLinearMem_t * buff);
#endif
#endif
/* Mark shared memory directions. */
/* Marks info as requiring a copy before a run, after a run, both, or neither.
 * Marks are explicitly released with direction_flags = 0, or are implicitly
 * released during Free.
 */
    void CWLMarkMemory(void *inst, const CWLLinearMem_t *info, u32 direction);


/* Run one or more jobs on cores. Jobs must be dispatched to the computation
 * cores in the order they are provided in |job_regs|. |job_regs| is an array
 * of pointers pointing to control vector for a job and contains exactly
 * |num_jobs| entries. When job is finished the same control vector will be
 * updated to contain the control values of the hardware after it has executed
 * the job. |status| is array of values where each individual job's status will
 * be stored.
 *
 * Returns CWL_HW_WAIT_OK on success. CWL_HW_WAIT_ERROR or CWL_HW_WAIT_TIMEOUT
 * otherwise.
 */
    void CWLRun(void *instance, size_t regs_size, void *job_regs,
                size_t num_jobs, enum CWLCoreStatus *status);

    /* SW/SW shared memory handling */

#ifdef CHECK_MEM_LEAK_TRANS
    void *CWLmalloc_func(u32 n, const     char *func_name, int line);
    void *CWLcalloc_func(u32 n, u32 s, const char *func_name, int line);
    void CWLfree_func(void *p, const     char *func_name, int line);
    
#define CWLmalloc(n)      CWLmalloc_func(n, __FUNCTION__, __LINE__)
#define CWLcalloc(n, s)   CWLcalloc_func(n, s, __FUNCTION__, __LINE__)
#define CWLfree(p)        CWLfree_func(p, __FUNCTION__, __LINE__)
#else
    void * CWLmalloc(u32 n);
    void * CWLcalloc(u32 n, u32 s);
    void CWLfree(void * p);
#endif

    void *CWLmemcpy(void *d, const void *s, u32 n);
    void *CWLmemset(void *d, i32 c, u32 n);
    int CWLmemcmp(const void *s1, const void *s2, u32 n);

void CWLStartDec(void* cwl, size_t regs_size, const void* regs, void **pcore);
void CWLWaitDec(void *cwl, enum CWLCoreStatus *ret, size_t regs_size,
                void *regs, void *pcore);

void CwlSetEncindex(const void *instance,u32 encindex);
void CwlGetHwTimePerFrame(const void *instance,void *hw_perf);

#ifdef __cplusplus
}
#endif

#endif /*__CWL_H__*/
