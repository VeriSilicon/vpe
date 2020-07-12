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
--  Description :  Encode picture
--
------------------------------------------------------------------------------*/

#ifndef __VP9_CODE_FRAME_H__
#define __VP9_CODE_FRAME_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "vp9instance.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

typedef enum
{
  VP9ENCODE_OK = 0,
  VP9ENCODE_TIMEOUT = 1,
  VP9ENCODE_DATA_ERROR = 2,
  VP9ENCODE_HW_ERROR = 3,
  VP9ENCODE_SYSTEM_ERROR = 4,
  VP9ENCODE_HW_RESET = 5
} vp9EncodeFrame_e;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void VP9SetFrameParams(vp9Instance_s *inst);
vp9EncodeFrame_e VP9CodeFrame(vp9Instance_s *inst);
vp9EncodeFrame_e VP9CodeFrameMultiPass(vp9Instance_s *inst);
//void VP9InitPenalties(vp9Instance_s * inst);
u32 ProcessStatistics(vp9Instance_s *inst, i32 *boost);

#endif
