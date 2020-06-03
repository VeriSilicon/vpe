/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2017 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/

#ifndef CHECKSUM_H
#define CHECKSUM_H

typedef struct {
  int offset;
  unsigned int chksum;
} checksum_ctx;
void checksum_init(checksum_ctx *ctx, unsigned int chksum, int offset);
unsigned int checksum(checksum_ctx *ctx, unsigned char *data, int len);
unsigned int checksum_final(checksum_ctx *ctx);

#endif
