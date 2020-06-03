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
--  Abstract  :   Encoder instance
--
------------------------------------------------------------------------------*/

#ifndef VP9MACROBLOCK_TOOLS_H
#define VP9MACROBLOCK_TOOLS_H

/*------------------------------------------------------------------------------
  Include headers
------------------------------------------------------------------------------*/
#include "base_type.h"
#include "vp9instance.h"

/*------------------------------------------------------------------------------
  External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
  Module defines
------------------------------------------------------------------------------*/

typedef enum
{
  /* Intra luma 16x16 or intra chroma 8x8 prediction modes */
  DC_PRED,
  V_PRED,
  H_PRED,
  TM_PRED,

  /* Common name of intra predicted mb where partition size is 4x4 */
  B_PRED,

  /* Intra 4x4 prediction modes */
  B_DC_PRED,
  B_TM_PRED,
  B_VE_PRED,
  B_HE_PRED,
  B_LD_PRED,
  B_RD_PRED,
  B_VR_PRED,
  B_VL_PRED,
  B_HD_PRED,
  B_HU_PRED,

  /* Inter prediction (partitioning) types */
  P_16x16,    /* [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15] */
  P_16x8,     /* [0,1,2,3,4,5,6,7,8][9,10,11,12,13,14,15] */
  P_8x16,     /* [0,1,4,5,8,9,12,13][2,3,6,7,10,11,14,15] */
  P_8x8,      /* [0,1,4,5][2,3,6,7][8,9,12,13][10,11,14,15] */
  P_4x4     /* Every subblock gets its own vector */
} type;

/*------------------------------------------------------------------------------
  Function prototypes
------------------------------------------------------------------------------*/
void InitQuantTables(vp9Instance_s *);

#endif
