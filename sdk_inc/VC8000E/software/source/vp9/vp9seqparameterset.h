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
--  Abstract  :   Sequence parameter set
--
------------------------------------------------------------------------------*/

#ifndef VP9SEQ_PARAMETER_SET_H
#define VP9SEQ_PARAMETER_SET_H

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
  i32 picWidthInMbs;
  i32 picHeightInMbs;
  i32 picWidthInPixel;
  i32 picHeightInPixel;
  i32 horizontalScaling;
  i32 verticalScaling;
  i32 colorType;
  i32 clampType;
  i32 dctPartitions;  /* Dct data partitions 0=1, 1=2, 2=4, 3=8 */
  i32 partitionCnt; /* Abbreviation:  2+(1<<prm->dctPartitions) */
  i32 profile;
  i32 filterType;
  i32 filterLevel;
  i32 filterSharpness;
  i32 quarterPixelMv;
  i32 splitMv;
  i32 singBias[3];  /* SingBias: 0 = ipf, 1 = grf, 2 = arf */

  i32 autoFilterLevel;
  i32 autoFilterSharpness;
  bool filterDeltaEnable;
  i32 modeDelta[4];
  i32 oldModeDelta[4];
  i32 refDelta[4];
  i32 oldRefDelta[4];

  i32 refreshEntropy;
  i32 qpDelta[5];          /* Quant deltas for Ydc, Y2dc, Y2ac, UVdc, UVac */
  i32 autoQpDelta[5];      /* Automatic setting based on QP */
} sps;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

#endif
