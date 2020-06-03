#ifndef __L2CACHE_API_H__
#define __L2CACHE_API_H__

#include "base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

  int sw_cache_e;
  int sw_cache_irq;
  int sw_abort_e;
  int sw_reorder_e;
  u32 sw_rmaxburst;

  int sw_cache_all_e;
  int sw_cache_excpt_wr_e;
  int sw_axi_rd_id_e;
  u32 base_id;
  u32 sw_timeout_cycles;

  u32 sw_cache_except_list_address;

  int bus_timeout_int_mask;
  int abort_int_mask;

} L2R_GLOBAL_CFG;

typedef struct {

  int valid;
  u32 start_address;
  u32 end_address;

} L2R_STREAM_CONFIG;

typedef void * L2CACHE_HANDLE;


L2CACHE_HANDLE L2CACHE_init(u32 base_offset, char * device);
void L2CACHE_release(L2CACHE_HANDLE hd);

int L2R_global_config(L2CACHE_HANDLE hd, L2R_GLOBAL_CFG * pGlobalCfg);
int L2R_stream_config(L2CACHE_HANDLE hd, L2R_STREAM_CONFIG * pCfg);

int L2R_Enable_Cache(L2CACHE_HANDLE hd, int core_id);
int L2R_Disable_Cache(L2CACHE_HANDLE hd, int core_id);

int L2R_dump_regs(L2CACHE_HANDLE hd, int core_id);

#ifdef __cplusplus
}
#endif

#endif /* __L2CACHE_API_H__ */

