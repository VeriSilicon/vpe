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
--  Abstract  :   VP9 stream headers
--
------------------------------------------------------------------------------*/

#ifndef VP9HEADERS_H
#define VP9HEADERS_H

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

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void VP9FrameHeader(vp9Instance_s *);
void VP9FrameHeaderFinish(vp9Instance_s *container);
void VP9FrameTag(vp9Instance_s *);
void VP9DataPartitionSizes(vp9Instance_s *);
#endif
