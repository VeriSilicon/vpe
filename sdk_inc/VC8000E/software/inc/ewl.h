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

#ifndef __EWL_H__
#define __EWL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>
#include "base_type.h"
#ifdef FB_PERFORMANCE_STATIC
#include "fb_performance.h"
#endif

/* HW ID check. */
#define HW_ID_PRODUCT_MASK  0xFFFF0000
#define HW_ID_MAJOR_NUMBER_MASK  0x0000FF00
#define HW_ID_MINOR_NUMBER_MASK  0x000000FF
#define HW_ID_PRODUCT(x) (((x &  HW_ID_PRODUCT_MASK) >> 16))
#define HW_ID_MAJOR_NUMBER(x) (((x &  HW_ID_MAJOR_NUMBER_MASK) >> 8))
#define HW_ID_MINOR_NUMBER(x) ((x &  HW_ID_MINOR_NUMBER_MASK))

#define HW_ID_PRODUCT_H2       0x4832
#define HW_ID_PRODUCT_VC8000E  0x8000

#define HWIF_REG_CFG1  (80)
#define HWIF_REG_CFG2  (214)
#define HWIF_REG_CFG3  (226)
#define HWIF_REG_CFG4  (287)

#define CORE_INFO_MODE_OFFSET       31
#define CORE_INFO_AMOUNT_OFFSET     28

#define MAX_SUPPORT_CORE_NUM        4
/* HW status register bits */
#define ASIC_STATUS_SEGMENT_READY       0x1000
#define ASIC_STATUS_FUSE_ERROR          0x200
#define ASIC_STATUS_SLICE_READY         0x100
#define ASIC_STATUS_LINE_BUFFER_DONE    0x080  /* low latency */
#define ASIC_STATUS_HW_TIMEOUT          0x040
#define ASIC_STATUS_BUFF_FULL           0x020
#define ASIC_STATUS_HW_RESET            0x010
#define ASIC_STATUS_ERROR               0x008
#define ASIC_STATUS_FRAME_READY         0x004
#define ASIC_IRQ_LINE                   0x001

#define ASIC_STATUS_ALL       (ASIC_STATUS_SEGMENT_READY |\
                               ASIC_STATUS_FUSE_ERROR |\
                               ASIC_STATUS_SLICE_READY |\
                               ASIC_STATUS_LINE_BUFFER_DONE |\
                               ASIC_STATUS_HW_TIMEOUT |\
                               ASIC_STATUS_BUFF_FULL |\
                               ASIC_STATUS_HW_RESET |\
                               ASIC_STATUS_ERROR |\
                               ASIC_STATUS_FRAME_READY)

  /* Return values */
#define EWL_OK                      0
#define EWL_ERROR                  -1

#define EWL_HW_WAIT_OK              EWL_OK
#define EWL_HW_WAIT_ERROR           EWL_ERROR
#define EWL_HW_WAIT_TIMEOUT         1

  /* HW configuration values */
#define EWL_HW_BUS_TYPE_UNKNOWN     0
#define EWL_HW_BUS_TYPE_AHB         1
#define EWL_HW_BUS_TYPE_OCP         2
#define EWL_HW_BUS_TYPE_AXI         3
#define EWL_HW_BUS_TYPE_PCI         4

#define EWL_HW_BUS_WIDTH_UNKNOWN     0
#define EWL_HW_BUS_WIDTH_32BITS      1
#define EWL_HW_BUS_WIDTH_64BITS      2
#define EWL_HW_BUS_WIDTH_128BITS     3

#define EWL_HW_SYNTHESIS_LANGUAGE_UNKNOWN     0
#define EWL_HW_SYNTHESIS_LANGUAGE_VHDL        1
#define EWL_HW_SYNTHESIS_LANGUAGE_VERILOG     2

#define EWL_HW_CONFIG_NOT_SUPPORTED    0
#define EWL_HW_CONFIG_ENABLED          1

  /* Hardware configuration description */
//swreg80 bit             name                                      function

//    bit 30                 sw_enc_HWScalingSupport         Down-scaling supported by HW. 0=not supported. 1=supported
//    bit 29                 sw_enc_HWBFrameSupport         HW bframe support. 0=not support bframe. 1=support bframe
//    bit 28                 sw_enc_HWRgbSupport              RGB to YUV conversion supported by HW. 0=not supported. 1=supported
//    bit 27                 sw_enc_HWHevcSupport             HEVC encoding supported by HW. 0=not supported. 1=supported
//    bit 26                 sw_enc_HWVp9Support              VP9 encoding supported by HW. 0=not supported. 1=supported
//    bit 25                 sw_enc_HWDenoiseSupport        Denoise supported by HW, 0=not supported. 1=supported
//    bit [23:21]           sw_enc_HWBus                         Bus connection of HW. 1=AHB. 2=OCP. 3=AXI. 4=PCI. 5=AXIAHB. 6=AXIAPB.
//    bit 20                 sw_enc_HWCavlcSupport            CAVLC supported by HW, 0=not supported. 1=supported
//    bit 19                 sw_enc_HWLineBufSupport         LineBuffer input mode supported by HW, 0=not supported. 1=supported
//    bit 18                 sw_enc_HWProgRdoSupport       Prog Rdo supported by HW, 0=not supported. 1=supported
//    bit 17                 sw_enc_HWRFCSupport             Reference frame compression supported by HW, 0=not supported. 1=supported
//    bit 16                 sw_enc_HWTU32Support            TU32 supported by HW, 0=not supported. 1=supported
//    bit 15                 sw_enc_HWJPEGSupport           JPEG encoder supported by HW, 0=not supported. 1=supported
//    bit [14:13]          sw_enc_HWBusWidth                 Bus width of HW. 0=32b. 1=64b. 2=128b
//    bit [12:00]          sw_enc_HWMaxVideoWidth         Maximum video width supported by HW (pixels)
  typedef struct EWLHwConfig
  {
    u32 h264Enabled;     /*bit 31 *//* HW supports H264 */
    u32 scalingEnabled;  /*bit 30 *//* HW supports down-scaling */
    u32 bFrameEnabled; /*bit 29 *//* HW b frame enabled */
    u32 rgbEnabled;      /*bit 28 *//* HW supports RGB input */
    u32 hevcEnabled;     /*bit 27 *//* HW supports HEVC */
    u32 vp9Enabled;      /*bit 26 *//* HW supports VP9 */
    u32 deNoiseEnabled;      /*bit 25 *//* HW supports DeNoise */
    u32 main10Enabled;      /*bit 24 *//* HW supports Main10 */
    u32 busType;      /*bit 23:21 *//* HW bus type in use  1=AHB. 2=OCP. 3=AXI. 4=PCI. 5=AXIAHB. 6=AXIAPB.*/
    u32 cavlcEnable; /*bit 20 *//* HW support H264 CAVLC */
    u32 lineBufEnable; /*bit 19 *//* HW support line buffer input mode */
    u32 progRdoEnable; /*bit 18 *//* HW support prog rdo for HEVC */
    u32 rfcEnable;         /*bit 17 *//* HW support reference frame compression */
    u32 tu32Enable;      /*bit 16 *//* HW support TU32 transform for HEVC */
    u32 jpegEnabled;     /* bit 15   */ /* HW support jpeg encoder */
    u32 busWidth;           /*bit 14:13 *//* bus width  */
    u32 maxEncodedWidthHEVC; /*bit 12:0 *//* Maximum supported width for video encoding (HEVC mode on V62 and later) */

    /* second part fuse in reg 214 */
    u32 ljpegSupport;        /*bit 31 */ /* HW supports Loss JPEG or not */
    u32 roiAbsQpSupport; /*bit 30 */ /* HW supports ROI Absolute QP or not */
    u32 intraTU32Enable;  /*bit 29 */ /* HW supports IntraTU32 or not */
    u32 roiMapVersion;    /*bit 28:26 */ /* roi map buffer format version */
    u32 maxEncodedWidthH264; /*bit 25:13 *//* Maximum supported width for video encoding (H264 mode) */
    u32 maxEncodedWidthJPEG; /*bit 12:0 *//* Maximum supported width for video encoding (JPEG mode) */

    /* third part fuse in reg 226 */
    u32 ssimSupport;        /*bit 31 */ /* HW supports SSIM calculation or not */
    u32 P010RefSupport;        /*bit 30 */ /* HW supports P010 tile-raster format for reference frame buffer or not */
    u32 cuInforVersion;     /*bit 29:27*/ /* Version of the output CU information format. */
    u32 meVertSearchRangeHEVC;  /*bit 26:21*/ /*ME vertical search range in 8 pixel unit for HEVC*/
    u32 meVertSearchRangeH264;   /*bit 20:15*/ /*ME vertical search range in 8 pixel unit for H264*/
    u32 ctbRcVersion;          /*bit 14:12 */ /* CTB RC Version */
    u32 jpeg422Support;        /*bit 11 */ /* HW supports JPEG422 coding or not */
    u32 gmvSupport;            /*bit 10*/ /*Global MV supported by HW. 0=not supported. 1=supported*/
    u32 ROI8Support;           /*bit 9 */ /*8 ROIs supported by HW. 0=not supported. 1=supported*/

    /* multi-pass features*/
    u32 meHorSearchRangeBframe;/* bit 8:7 */ /* ME4N horizontal search range in 64 pixel unit for BFrame (0: 64 pixel)*/
    u32 RDOQSupport;           /* bit 6 */ /* HW support RDOQ or not */
    u32 bMultiPassSupport;     /* bit 5 */ /* HW support multipass: 0=not supported. 1=supoorted.*/
    u32 inLoopDSRatio;         /* bit 4*/  /* HW support in-loop down scale: 0=1:1, 1=1:2.*/
    u32 streamBufferChain;     /* bit 3 */ /* Stream Buffer Chain supported by HW. 0=not supported. 1=supported */
    u32 streamMultiSegment;    /* bit 2 */ /* Stream multi-segment supported by HW. 0=not supported. 1=supported */
    u32 IframeOnly;  /* bit 1 */ /* Only I frame supported by HW. 0=I/P/B supported. 1=I only supported */
    u32 dynamicMaxTuSize;         /* bit 0 */ /* Dynamic max TU size change per frame supported by HW. 0=not supported. 1=supported */

    /* fourth part fuse in reg 287 */
    u32 videoHeightExt;    /* bit 31*/ /* Maximum allowed video height extended from 8192 to 8640. 0=Not. 1=Yes. */
    u32 cscExtendSupport;  /* bit 30*/ /* RGB to YUV conversion extension. 0=not supported. 1=supported*/
    u32 scaled420Support;  /* bit 29*/ /* out-loop scaler output YUV420SP. 0=not supported. 1=supported*/
  } EWLHwConfig_t;


  /* Allocated linear memory area information */
  typedef struct EWLLinearMem
  {
    //with new mem driver, these are for ep, and virtual address not used
    u32 *virtualAddress;
    ptr_t busAddress;
    u32 size;
    u32 *allocVirtualAddr;
    ptr_t allocBusAddr;
#ifdef USE_OLD_DRV
#ifdef SUPPORT_TCACHE
    int in_host;
#endif
#else
    //rc
    u32 * rc_virtualAddress;
    ptr_t rc_busAddress;
    void * rc_kvirt; //record for free
    u32 * rc_allocVirtualAddr;
    ptr_t rc_allocBusAddr;
#endif
  } EWLLinearMem_t;

  /* EWLInitParam is used to pass parameters when initializing the EWL */

  typedef struct {
    int core_usage_counts_p1[2];
    int core_usage_counts[2];
    i64 hwcycle_accp1;
    i64 hwcycle_acc;
    i64 hwcycle_acc_total;
    pthread_mutex_t hwcycle_acc_mutex;
    struct timeval last_frame_encoded_timestamp;
#ifdef FB_PERFORMANCE_STATIC
    DECLARE_PERFORMANCE_STATIC(vcehw);
    DECLARE_PERFORMANCE_STATIC(vcehwp1);
    DECLARE_PERFORMANCE_STATIC(vce_dummy_0);
    DECLARE_PERFORMANCE_STATIC(vce_dummy_1);
    DECLARE_PERFORMANCE_STATIC(CU_ANAL);
    DECLARE_PERFORMANCE_STATIC(vcehw_total);
    DECLARE_PERFORMANCE_STATIC(vce_total);
#endif
  } ENCPERF;

  typedef struct EWLInitParam
  {
    u32 clientType;

    int ewl_index;
    int pass;
    ENCPERF * perf;

    //add for new driver
#ifdef DRV_NEW_ARCH
    int priority;
    char * device;
    int mem_id;
#endif

#ifdef VCE_MEM_ERR_TEST
    i32 *vce_memory_err_shadow;
    i32 *vce_memory_err_cnt;
#endif
#ifdef VCE_EDMA_ERR_TEST
    i32 *vce_edma_err_shadow;
    i32 *vce_edma_err_cnt;
#endif

  } EWLInitParam_t;

#define EWL_CLIENT_TYPE_H264_ENC         0U
#define EWL_CLIENT_TYPE_HEVC_ENC         1U
#define EWL_CLIENT_TYPE_VP9_ENC          2U
#define EWL_CLIENT_TYPE_JPEG_ENC         3U
#define EWL_CLIENT_TYPE_CUTREE           4U

  extern u32 (*pollInputLineBufTestFunc)(void);

  /*------------------------------------------------------------------------------
      4.  Function prototypes
  ------------------------------------------------------------------------------*/

  /* Read and return the HW ID register value, static implementation */
#ifdef DRV_NEW_ARCH
  u32 EWLReadAsicID(const void *inst, u32 core_id);

  u32 EWLGetSliceNum(const void *inst);

  EWLHwConfig_t EWLReadAsicConfig(const void *inst, u32 core_id);
#else
  u32 EWLReadAsicID(u32 core_id);

  u32 EWLGetSliceNum(void);

  EWLHwConfig_t EWLReadAsicConfig(u32 core_id);
#endif

  u32 EWLGetCoreNum(void);

  /* Read and return HW configuration info, static implementation */

  /* Initialize the EWL instance
   * Returns a wrapper instance or NULL for error
   * EWLInit is called when the encoder instance is initialized */
  const void *EWLInit(EWLInitParam_t *param);

  /* Release the EWL instance
   * Returns EWL_OK or EWL_ERROR
   * EWLRelease is called when the encoder instance is released */
  i32 EWLRelease(const void *inst);

  /* Reserve the HW resource for one codec instance
   * EWLReserveHw is called when beginning a frame encoding
   * The function may block until the resource is available.
   * Returns EWL_OK if the resource was successfully reserved for this instance
   * or EWL_ERROR if unable to reserve the resource. */
  i32 EWLReserveHw(const void *inst, u32 *core_info);

  /* Release the HW resource
   * EWLReleaseHw is called when the HW has finished the frame encoding.
   * The codec SW will continue the frame encoding but the HW can
   * be used by another codec.*/
  void EWLReleaseHw(const void *inst);

  u32 EWLGetPerformance(const void *inst);

  /* Frame buffers memory */
  i32 EWLMallocRefFrm(const void *instance, u32 size, u32 alignment,EWLLinearMem_t * info);
  void EWLFreeRefFrm(const void *inst, EWLLinearMem_t *info);

  /* SW/HW shared memory */
  i32 EWLMallocLinear(const void *instance, u32 size, u32 alignment,EWLLinearMem_t * info);
  void EWLFreeLinear(const void *inst, EWLLinearMem_t *info);

  /* D-Cache coherence *//* Not in use currently */
  void EWLDCacheRangeFlush(const void *instance, EWLLinearMem_t *info);
  void EWLDCacheRangeRefresh(const void *instance, EWLLinearMem_t *info);

  /* Write value to a HW register
   * All registers are written at once at the beginning of frame encoding
   * Offset is relative to the the HW ID register (#0) in bytes
   * Enable indicates when the HW is enabled. If shadow registers are used then
   * they must be flushed to the HW registers when enable is '1' before
   * writing the register that enables the HW */
  void EWLWriteReg(const void *inst, u32 offset, u32 val);
  /* Write back value to a HW register on callback/frame done (for multicore) */
  void EWLWriteBackReg(const void *inst, u32 offset, u32 val);

  /* Read and return the value of a HW register
   * The status register is read after every macroblock encoding by SW
   * The other registers which may be updated by the HW are read after
   * BUFFER_FULL or FRAME_READY interrupt
   * Offset is relative to the the HW ID register (#0) in bytes */
  u32 EWLReadReg(const void *inst, u32 offset);

#ifdef NEW_IOCTL_ENABLE
int EWLPushReg(const void *inst, u32 * pReg, u32 start_offset, u32 size);
int EWLPullReg(const void *inst, u32 * pReg, u32 start_offset, u32 size);
void EWLDumpReg(const void *inst);
#endif

  /* Writing all registers in one call *//*Not in use currently */
  void EWLWriteRegAll(const void *inst, const u32 *table, u32 size);
  /* Reading all registers in one call *//*Not in use currently */
  void EWLReadRegAll(const void *inst, u32 *table, u32 size);

  /* HW enable/disable. This will write <val> to register <offset> and by */
  /* this enablig/disabling the hardware. */
  void EWLEnableHW(const void *inst, u32 offset, u32 val);
  void EWLDisableHW(const void *inst, u32 offset, u32 val);

  /* Synchronize SW with HW
   * Returns EWL_HW_WAIT_OK, EWL_HW_WAIT_ERROR or EWL_HW_WAIT_TIMEOUT
   * EWLWaitHwRdy is called after enabling the HW to wait for IRQ from HW.
   * If slicesReady pointer is given, at input it should contain the number
   * of slicesReady received. The function will return when the HW has finished
   * encoding next slice. Upon return the slicesReady pointer will contain
   * the number of slices that are ready and available in the HW output buffer.
   */

  i32 EWLWaitHwRdy(const void *inst, u32 *slicesReady,u32 totalsliceNumber,u32* status_register);

  /* SW/SW shared memory handling */
#ifdef CHECK_MEM_LEAK_TRANS
  void *EWLmalloc_func(u32 n, const     char *func_name, int line);
  void *EWLcalloc_func(u32 n, u32 s, const char *func_name, int line);
  void EWLfree_func(void *p,    const char *func_name, int line);

#define EWLmalloc(n)      EWLmalloc_func(n, __FUNCTION__, __LINE__)
#define EWLcalloc(n, s)   EWLcalloc_func(n, s, __FUNCTION__, __LINE__)
#define EWLfree(p)        EWLfree_func(p, __FUNCTION__, __LINE__)
#else
  void *EWLmalloc(u32 n);
  void EWLfree(void *p);
  void *EWLcalloc(u32 n, u32 s);
#endif

  void *EWLmemcpy(void *d, const void *s, u32 n);
  void *EWLmemset(void *d, i32 c, u32 n);
  int EWLmemcmp(const void *s1, const void *s2, u32 n);

  /* Get the address/size of on-chip sram used for input line buffer. */
  i32 EWLGetLineBufSram (const void *instance, EWLLinearMem_t *info);

  /* allocate loopback line buffer in memory, mainly used when there is no on-chip sram */
  i32 EWLMallocLoopbackLineBuf (const void *instance, u32 size, EWLLinearMem_t *info);

  /* Tracing PSNR profile result */
  void EWLTraceProfile(const void *inst);

#if defined(SUPPORT_FBIPS) || !defined(USE_OLD_DRV)
void * EWLGetIpHandleByOffset(const void *instance, u32 offset);
void * EWLGetIpHandle(const void *instance, ptr_t base);
void * EWLCheckIpHandle(const void *instance, ptr_t base);
i32 EWLMallocHostLinear(const void *instance, u32 size, u32 alignment,EWLLinearMem_t * info);
#ifdef USE_OLD_DRV
void EWLFreeHostLinear(const void *instance, EWLLinearMem_t * info);
#endif
#endif

#ifndef USE_OLD_DRV
i32 EWLMallocInoutLinear(const void *instance, u32 size, u32 alignment,EWLLinearMem_t * info);
int EWLTransDataRC2EP(const void *instance, EWLLinearMem_t * src, EWLLinearMem_t * dest, u32 size);
int EWLTransDataEP2RC(const void *instance, EWLLinearMem_t * src, EWLLinearMem_t * dest, u32 size);
#endif

i64 EWLGetHwPerformance(const void *instance);

#ifdef __cplusplus
}
#endif
#endif /*__EWL_H__*/
