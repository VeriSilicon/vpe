#ifndef __PCIE_EDMA_H__
#define __PCIE_EDMA_H__

#include "base_type.h"
#include "transcoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void * EDMA_HANDLE;

EDMA_HANDLE TRANS_EDMA_init(char * device);
void TRANS_EDMA_release(EDMA_HANDLE ehd);

int TRANS_EDMA_RC2EP_nonlink(EDMA_HANDLE ehd, u64 src_base, u64 dst_base, u32 size);
int TRANS_EDMA_EP2RC_nonlink(EDMA_HANDLE ehd, u64 src_base, u64 dst_base, u32 size);

#ifdef ENABLE_HW_HANDSHAKE
int TRANS_EDMA_RC2EP_link_config(EDMA_HANDLE ehd, u64 link_table_rc_base, void * link_table_rc_vbase, u32 element_size);
#else
int TRANS_EDMA_RC2EP_link_config(EDMA_HANDLE ehd, u64 link_table_rc_base, void * link_table_rc_vbase, u32 element_size, u32 element_num_each);
#endif
int TRANS_EDMA_RC2EP_link_enable(EDMA_HANDLE ehd, int core_id);
int TRANS_EDMA_RC2EP_link_finish(EDMA_HANDLE ehd);

int TRANS_EDMA_link_is_pending_start(EDMA_HANDLE ehd);
int TRANS_EDMA_link_is_running(EDMA_HANDLE ehd);

#ifdef __cplusplus
}
#endif

#endif


