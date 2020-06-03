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
--------------------------------------------------------------------------------*/

#ifndef VP9PUT_BITS_H
#define VP9PUT_BITS_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "base_type.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef struct
{
  u8 *data;   /* Pointer to next byte of data buffer */
  u8 *pData;    /* Pointer to beginning of data buffer */
  i32 size;   /* Size of *data in bytes */
  i32 byteCnt;    /* Data buffer stream byte count */

  i32 range;    /* Bool encoder range [128, 255] */
  i32 bottom;   /* Bool encoder left endpoint */
  i32 bitsLeft;   /* Bool encoder bits left before flush bottom */
} vp9buffer;

typedef struct
{
  i32 value;    /* Bits describe the bool tree  */
  i32 number;   /* Number, valid bit count in above tree */
  i32 index[9];   /* Probability table index */
} tree;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
i32 VP9SetBuffer(vp9buffer *, u8 *, i32);
i32 VP9BufferOverflow(vp9buffer *);
i32 VP9BufferGap(vp9buffer *buffer, i32 gap);
void VP9PutByte(vp9buffer *buffer, i32 byte);
void VP9PutLit(vp9buffer *, i32, i32);
void VP9PutBool(vp9buffer *buffer, i32 prob, i32 boolValue);
void VP9PutTree(vp9buffer *buffer, tree const  *tree, i32 *prob);
void VP9FlushBuffer(vp9buffer *buffer);

#endif
