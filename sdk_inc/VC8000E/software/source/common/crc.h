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

#ifndef CRC_H
#define CRC_H

/* Polynomial for CRC32 */
#define QUOTIENT 0x04c11db7

typedef struct {
  unsigned int crctab[256];
  unsigned int crc;
} crc32_ctx;
void crc32_init(crc32_ctx *ctx, unsigned int init_crc);
unsigned int crc32(crc32_ctx *ctx, unsigned char *data, int len);
unsigned int crc32_final(crc32_ctx *ctx);

#endif
