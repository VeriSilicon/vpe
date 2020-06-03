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
--  Abstract : Encoder initialization and setup
--
------------------------------------------------------------------------------*/

#ifndef __VP9_INIT_H__
#define __VP9_INIT_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "vp9encapi.h"
#include "vp9instance.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

bool_e VP9CheckCfg(const VP9EncConfig *pEncCfg);
i32 VP9GetAllowedWidth(i32 width, VP9EncPictureType inputType);

VP9EncRet VP9Init(const VP9EncConfig *pEncCfg, vp9Instance_s **instAddr);

void VP9Shutdown(vp9Instance_s *data);

#endif
