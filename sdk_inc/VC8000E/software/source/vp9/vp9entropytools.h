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
--  Abstract  :   Entropy coding
--
------------------------------------------------------------------------------*/

#ifndef VP9ENTROPY_TOOLS_H
#define VP9ENTROPY_TOOLS_H

/*------------------------------------------------------------------------------
  Include headers
------------------------------------------------------------------------------*/
#include "base_type.h"

/*------------------------------------------------------------------------------
  External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
  Module defines
------------------------------------------------------------------------------*/
typedef struct
{
  i32 skipFalseProb;
  i32 intraProb;
  i32 lastProb;
  i32 gfProb;
  i32 kfYmodeProb[4];
  i32 YmodeProb[4];
  i32 kfUVmodeProb[3];
  i32 UVmodeProb[3];
  i32 kfBmodeProb[10][10][9];
  i32 BmodeProb[9];
  i32 coeffProb[4][8][3][11];
  i32 oldCoeffProb[4][8][3][11];
  i32 mvRefProb[4];
  i32 mvProb[2][19];
  i32 oldMvProb[2][19];
  i32 subMvPartProb[3]; /* TODO use pointer directly to subMvPartProb */
  i32 subMvRefProb[5][3]; /* TODO use pointer directly to subMvRefProb */
  i32 defaultCoeffProbFlag;   /* Flag for coeffProb == defaultCoeffProb */
  i32 updateCoeffProbFlag;    /* Flag for coeffProb != oldCoeffProb */
  i32 segmentProb[3];
} entropy;

#endif
