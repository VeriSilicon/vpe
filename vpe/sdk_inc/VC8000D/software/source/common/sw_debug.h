/*------------------------------------------------------------------------------
--Copyright (c) 2015-2017, VeriSilicon Holdings Co., Ltd. All rights reserved --
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

#ifndef SW_DEBUG_H_
#define SW_DEBUG_H_

/* macro for assertion, used only when _ASSERT_USED is defined */
#ifdef _ASSERT_USED
#ifndef ASSERT
#include <assert.h>
#define ASSERT(expr) assert(expr)
#endif
#else
#define ASSERT(expr)
#endif

/* macros for range checking used only when _RANGE_CHECK is defined */
#ifdef _RANGE_CHECK

#include <stdio.h>

/* macro for range checking an single value */
#define RANGE_CHECK(value, min_bound, max_bound)                   \
  {                                                                \
    if ((value) < (min_bound) || (value) > (max_bound))            \
      fprintf(stderr, "Warning: Value exceeds given limit(s)!\n"); \
  }

/* macro for range checking an array of values */
#define RANGE_CHECK_ARRAY(array, min_bound, max_bound, length)               \
  {                                                                          \
    i32 i;                                                                   \
    for (i = 0; i < (length); i++)                                           \
      if ((array)[i] < (min_bound) || (array)[i] > (max_bound))              \
        fprintf(stderr, "Warning: Value [%d] exceeds given limit(s)!\n", i); \
  }

#else /* _RANGE_CHECK */

#define RANGE_CHECK_ARRAY(array, min_bound, max_bound, length)
#define RANGE_CHECK(value, min_bound, max_bound)

#endif /* _RANGE_CHECK */

/* macro for debug printing, used only when _DEBUG_PRINT is defined */
#ifdef _SW_DEBUG_PRINT
#include <stdio.h>
#define DEBUG_PRINT(args) printf args
#else
#define DEBUG_PRINT(args)
#endif

/* macro for error printing, used only when _ERROR_PRINT is defined */
#ifdef _ERROR_PRINT
#include <stdio.h>
#define ERROR_PRINT(msg) fprintf(stderr, "ERROR: %s\n", msg)
#else
#define ERROR_PRINT(msg)
#endif

#endif /* SW_DEBUG_H_ */
