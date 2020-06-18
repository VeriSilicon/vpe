/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Holdings Co., Ltd. All rights reserved        --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
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

#ifndef SW_UTIL_H_
#define SW_UTIL_H_

#include <stdio.h>
#include "basetype.h"
#include "dwl.h"

#define HANTRO_OK 0
#define HANTRO_NOK 1

struct strmInfo {
	u32 low_latency;
	u32 send_len;
	addr_t strm_bus_addr;
	addr_t strm_bus_start_addr;
	u8* strm_vir_addr;
	u8* strm_vir_start_addr;
	u32 last_flag;
};

#define HANTRO_FALSE (0U)
#define HANTRO_TRUE (1U)

#define MEMORY_ALLOCATION_ERROR 0xFFFF
#define PARAM_SET_ERROR 0xFFF0

/* value to be returned by GetBits if stream buffer is empty */
#define END_OF_STREAM 0xFFFFFFFFU

/* macro to get smaller of two values */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/* macro to get greater of two values */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/* macro to get absolute value */
#define ABS(a) (((a) < 0) ? -(a) : (a))

/* macro to clip a value z, so that x <= z =< y */
#define CLIP3(x, y, z) (((z) < (x)) ? (x) : (((z) > (y)) ? (y) : (z)))

/* macro to clip a value z, so that 0 <= z =< 255 */
#define CLIP1(z) (((z) < 0) ? 0 : (((z) > 255) ? 255 : (z)))

/* macro to allocate memory */
#define ALLOCATE(ptr, count, type) \
  { ptr = DWLmalloc((count) * sizeof(type)); }

/* macro to free allocated memory */
#define FREE(ptr)      \
  {                    \
    if (ptr != NULL) { \
      DWLfree(ptr);    \
      ptr = NULL;      \
    }                  \
  }

/* macro to report syntax error */
#define SW_CHECK_SYNTAX_ELEMENT(rv, elem)                    \
  if (rv != HANTRO_OK) {                                     \
    fprintf(stderr, "error in syntax element '%s'\n", elem); \
    return rv;                                               \
  }

#define SW_CHECK_END_OF_STREAM(rv, elem)                             \
  if (rv == END_OF_STREAM) {                                         \
    fprintf(stderr, "end of stream in syntax element '%s'\n", elem); \
    return rv;                                                       \
  }

/* round to next multiple of n */
#define NEXT_MULTIPLE(value, n) (((value) + (n) - 1) & ~((n) - 1))
#define ALIGN(a) (1 << (a))

#define MAJOR_VERSION(hw_id) (((hw_id) & 0xF000) >> 12)
#define MINOR_VERSION(hw_id) (((hw_id) & 0x0FF0) >> 4)

/* legacy release based on product number */
#define IS_G1_LEGACY(hw_id) (((hw_id) >> 16) == 0x6731)
#define IS_G2_LEGACY(hw_id) (((hw_id) >> 16) == 0x6732)
#define IS_LEGACY(hw_id) ((((hw_id) >> 16) == 0x6731) || (((hw_id) >> 16) == 0x6732))

u32 SwCountLeadingZeros(u32 value, u32 length);
u32 SwNumBits(u32 value);
u8* SwTurnAround(const u8 * strm, const u8* buf, u8* tmp_buf, u32 buf_size, u32 num_bits);

#endif /* #ifdef SW_UTIL_H_ */
