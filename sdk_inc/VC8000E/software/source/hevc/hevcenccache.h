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

#ifndef HEVC_ENC_CACHE_H
#define HEVC_ENC_CACHE_H

#include "base_type.h"
#include "sw_picture.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /*------------------------------------------------------------------------------
      Function prototypes
  ------------------------------------------------------------------------------*/
  VCEncRet EnableCache(struct vcenc_instance *vcenc_instance, asicData_s *asic,struct sw_picture *pic);
  void DisableCache(struct vcenc_instance *vcenc_instance);

#ifdef __cplusplus
}
#endif

#endif
