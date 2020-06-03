#ifndef __TRANS_REG_RW_H__
#define __TRANS_REG_RW_H__

#include "base_type.h"
#include "transcoder.h"


#ifdef __cplusplus
    extern "C" {
#endif

typedef struct reg_desc REG_DESC_t;

#define REG_WRITE_DES2BUF(buf, offset, regidx, value) ({ \
            int __off = (offset);        \
            (buf)[__off].id = (regidx);    \
            (buf)[__off].val = (value);   \
        })
int trans_write_reg(int fd, int core_id, u8 ip_id, u32 reg_idx, int val);
u32 trans_read_reg(int fd, int core_id, u8 ip_id, u32 reg_idx, u32 *val);
int trans_write_reg_batch(int fd, int core_id, u8 ip_id, REG_DESC_t *reg_desc, int reg_desc_size);
u32 trans_read_reg_batch(int fd, int core_id, u8 ip_id, REG_DESC_t *reg_desc, int reg_desc_size);

#ifdef __cplusplus
    }
#endif

#endif


