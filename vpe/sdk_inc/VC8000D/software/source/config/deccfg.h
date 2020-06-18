/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Holdings Co., Ltd. All rights reserved        --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#ifndef __DEC_X170_CFG_H__
#define __DEC_X170_CFG_H__

/* predefined values of HW system parameters. DO NOT ALTER! */
#define DEC_X170_LITTLE_ENDIAN 1
#define DEC_X170_BIG_ENDIAN 0

#define DEC_X170_BUS_BURST_LENGTH_UNDEFINED 0
#define DEC_X170_BUS_BURST_LENGTH_4 4
#define DEC_X170_BUS_BURST_LENGTH_8 8
#define DEC_X170_BUS_BURST_LENGTH_16 16

#define DEC_X170_ASIC_SERVICE_PRIORITY_DEFAULT 0
#define DEC_X170_ASIC_SERVICE_PRIORITY_WR_1 1
#define DEC_X170_ASIC_SERVICE_PRIORITY_WR_2 2
#define DEC_X170_ASIC_SERVICE_PRIORITY_RD_1 3
#define DEC_X170_ASIC_SERVICE_PRIORITY_RD_2 4

#define DEC_X170_OUTPUT_FORMAT_RASTER_SCAN 0
#define DEC_X170_OUTPUT_FORMAT_TILED 1

#define DEC_X170_BUS_WIDTH_32 0
#define DEC_X170_BUS_WIDTH_64 1
#define DEC_X170_BUS_WIDTH_128 2
#define DEC_X170_BUS_WIDTH_256 3

#define HANTRODEC_STREAM_SWAP_8 1
#define HANTRODEC_STREAM_SWAP_16 2
#define HANTRODEC_STREAM_SWAP_32 4
#define HANTRODEC_STREAM_SWAP_64 8

#define HANTRODEC_DOUBLE_BUFFER_ENABLE 0

#define DEC_X170_APF_DISABLE 0

#define DEC_X170_CLOCK_GATING 0
#define DEC_X170_CLOCK_GATING_RUNTIME 0

/* end of predefined values */

/* now what we use */

#ifndef DEC_X170_USING_IRQ
/* Control IRQ generation by decoder hardware */
#define DEC_X170_USING_IRQ 1
#endif

#ifndef DEC_X170_ASIC_SERVICE_PRIORITY
/* hardware intgernal prioriy scheme. better left unchanged */
#define DEC_X170_ASIC_SERVICE_PRIORITY DEC_X170_ASIC_SERVICE_PRIORITY_DEFAULT
#endif

#ifndef DEC_X170_SCMD_DISABLE
/* AXI single command multiple data disable not set */
#define DEC_X170_SCMD_DISABLE 0
#endif

#ifndef DEC_X170_APF_DISABLE
/* Advanced prefetch disable flag. If disable flag is set, product shall
 * operate akin to 9190 and earlier products. */
#define DEC_X170_APF_DISABLE 0
#endif

/* Enable advanced prefetch single PU mode (will disable "normal" APF) */
#define DEC_X170_APF_SINGLE_PU_MODE 0

#ifndef DEC_X170_BUS_BURST_LENGTH
/* how long are the hardware data bursts; better left unchanged    */
#define DEC_X170_BUS_BURST_LENGTH DEC_X170_BUS_BURST_LENGTH_16
#endif

#ifndef DEC_X170_INPUT_STREAM_ENDIAN
/* this should match the system endianess, so that Decoder reads      */
/* the input stream in the right order                                */
#define DEC_X170_INPUT_STREAM_ENDIAN DEC_X170_LITTLE_ENDIAN
#endif

#ifndef DEC_X170_OUTPUT_PICTURE_ENDIAN
/* this should match the system endianess, so that Decoder writes     */
/* the output pixel data in the right order                           */
#define DEC_X170_OUTPUT_PICTURE_ENDIAN DEC_X170_LITTLE_ENDIAN
#endif

#ifndef DEC_X170_LATENCY_COMPENSATION
/* compensation for bus latency; values up to 63 */
#define DEC_X170_LATENCY_COMPENSATION 0
#endif

#ifndef DEC_X170_INTERNAL_CLOCK_GATING
/* clock is gated from decoder between images and for disabled codecs */
#define DEC_X170_INTERNAL_CLOCK_GATING DEC_X170_CLOCK_GATING
#endif
#ifndef DEC_X170_INTERNAL_CLOCK_GATING_RUNTIME
/* clock is gated from decoder structures that are not used */
#define DEC_X170_INTERNAL_CLOCK_GATING_RUNTIME DEC_X170_CLOCK_GATING_RUNTIME
#endif
#ifndef DEC_X170_OUTPUT_FORMAT
/* Decoder output picture format in external memory: Raster-scan or     */
/*macroblock tiled i.e. macroblock data written in consecutive addresses */
#define DEC_X170_OUTPUT_FORMAT DEC_X170_OUTPUT_FORMAT_RASTER_SCAN
#endif

#ifndef DEC_X170_DATA_DISCARD_ENABLE
#define DEC_X170_DATA_DISCARD_ENABLE 0
#endif

/* Decoder output data swap for 32bit words*/
#ifndef DEC_X170_OUTPUT_SWAP_32_ENABLE
#define DEC_X170_OUTPUT_SWAP_32_ENABLE 1
#endif

/* Decoder input data swap(excluding stream data) for 32bit words*/
#ifndef DEC_X170_INPUT_DATA_SWAP_32_ENABLE
#define DEC_X170_INPUT_DATA_SWAP_32_ENABLE 1
#endif

/* Decoder input stream swap for 32bit words */
#ifndef DEC_X170_INPUT_STREAM_SWAP_32_ENABLE
#define DEC_X170_INPUT_STREAM_SWAP_32_ENABLE 1
#endif

/* Decoder input data endian. Do not modify this! */
#ifndef DEC_X170_INPUT_DATA_ENDIAN
#define DEC_X170_INPUT_DATA_ENDIAN DEC_X170_BIG_ENDIAN
#endif

/* AXI bus read and write ID values used by HW. 0 - 255 */
#ifndef DEC_X170_AXI_ID_R
#define DEC_X170_AXI_ID_R 16
#endif

#ifndef DEC_X170_AXI_ID_W
#define DEC_X170_AXI_ID_W 0
#endif

/* Enable/Disable AXI bus read and write ID service specific IDs.
   Set these to 1 to enable service specific, incremental read/write IDs. For
   example, if DEC_X170_AXI_ID_R_E is set, all read services will get unique ID
   within the range
   DEC_X170_AXI_ID_R...(DEC_X170_AXI_ID_R+NUM_OF_READ_SERVICES). */
#ifndef DEC_X170_AXI_ID_R_E
#define DEC_X170_AXI_ID_R_E                    0
#endif

#ifndef DEC_X170_AXI_ID_W_E
#define DEC_X170_AXI_ID_W_E                    0
#endif

/* Decoder service merge disable */
#ifndef DEC_X170_SERVICE_MERGE_DISABLE
#define DEC_X170_SERVICE_MERGE_DISABLE 0
#endif

#ifndef DEC_X170_BUS_WIDTH
#define DEC_X170_BUS_WIDTH DEC_X170_BUS_WIDTH_128
#endif

#ifndef HANTRODEC_STREAM_SWAP
#if 0
#define HANTRODEC_STREAM_SWAP                           \
  (HANTRODEC_STREAM_SWAP_8 | HANTRODEC_STREAM_SWAP_16 | \
   HANTRODEC_STREAM_SWAP_32)
#else
/* Match our documents, set defualt SWAP to no-swap */
#define HANTRODEC_STREAM_SWAP 0
#endif
#endif

#ifndef HANTRODEC_MAX_BURST
#define HANTRODEC_MAX_BURST 64
#endif

#ifndef HANTRODEC_INTERNAL_DOUBLE_REF_BUFFER
#define HANTRODEC_INTERNAL_DOUBLE_REF_BUFFER HANTRODEC_DOUBLE_BUFFER_ENABLE
#endif

#ifndef HANTRODEC_TIMEOUT_OVERRIDE
#ifdef ENABLE_HW_HANDSHAKE
#define HANTRODEC_TIMEOUT_OVERRIDE 0x500000
#else
#define HANTRODEC_TIMEOUT_OVERRIDE 0x7FFFFFFF //0x500000
#endif
#endif

/* Check validity of values */

/* data discard and tiled mode can not be on simultaneously */
#if (DEC_X170_DATA_DISCARD_ENABLE && \
     (DEC_X170_OUTPUT_FORMAT == DEC_X170_OUTPUT_FORMAT_TILED))
#error \
"Bad value specified: DEC_X170_DATA_DISCARD_ENABLE && (DEC_X170_OUTPUT_FORMAT == DEC_X170_OUTPUT_FORMAT_TILED)"
#endif

#if (DEC_X170_OUTPUT_PICTURE_ENDIAN > 1)
#error "Bad value specified for DEC_X170_OUTPUT_PICTURE_ENDIAN"
#endif

#if (DEC_X170_OUTPUT_FORMAT > 1)
#error "Bad value specified for DEC_X170_OUTPUT_FORMAT"
#endif

#if (DEC_X170_BUS_BURST_LENGTH > 31)
#error "Bad value specified for DEC_X170_AMBA_BURST_LENGTH"
#endif

#if (DEC_X170_ASIC_SERVICE_PRIORITY > 4)
#error "Bad value specified for DEC_X170_ASIC_SERVICE_PRIORITY"
#endif

#if (DEC_X170_LATENCY_COMPENSATION > 63)
#error "Bad value specified for DEC_X170_LATENCY_COMPENSATION"
#endif

#if (DEC_X170_OUTPUT_SWAP_32_ENABLE > 1)
#error "Bad value specified for DEC_X170_OUTPUT_SWAP_32_ENABLE"
#endif

#if (DEC_X170_INPUT_DATA_SWAP_32_ENABLE > 1)
#error "Bad value specified for DEC_X170_INPUT_DATA_SWAP_32_ENABLE"
#endif

#if (DEC_X170_INPUT_STREAM_SWAP_32_ENABLE > 1)
#error "Bad value specified for DEC_X170_INPUT_STREAM_SWAP_32_ENABLE"
#endif

#if (DEC_X170_OUTPUT_SWAP_32_ENABLE > 1)
#error "Bad value specified for DEC_X170_INPUT_DATA_ENDIAN"
#endif

#if (DEC_X170_DATA_DISCARD_ENABLE > 1)
#error "Bad value specified for DEC_X170_DATA_DISCARD_ENABLE"
#endif

#if (DEC_X170_SERVICE_MERGE_DISABLE > 1)
#error "Bad value specified for DEC_X170_SERVICE_MERGE_DISABLE"
#endif

#if ((DEC_X170_INTERNAL_CLOCK_GATING == 0) && \
     DEC_X170_INTERNAL_CLOCK_GATING_RUNTIME)
#error \
"DEC_X170_INTERNAL_CLOCK_GATING_RUNTIME can only be enabled when DEC_X170_INTERNAL_CLOCK_GATING enabled"
#endif

/* Common defines for the decoder */
#define DEC_MAX_PPU_COUNT 4

/* Number registers for the decoder */
#if (DEC_MAX_PPU_COUNT == 1)
#define DEC_X170_REGISTERS 342
#define PP_UNIFIED_REGS 22
#elif (DEC_MAX_PPU_COUNT == 2)
#define DEC_X170_REGISTERS 359
#define PP_UNIFIED_REGS (22+17)
#elif (DEC_MAX_PPU_COUNT == 3)
#define DEC_X170_REGISTERS 376
#define PP_UNIFIED_REGS (22+17*2)
#elif (DEC_MAX_PPU_COUNT == 4)
#define DEC_X170_REGISTERS 393
#define PP_UNIFIED_REGS (22+17*3)
#endif

/* Todo: should be removed in final version. */
#define TOTAL_X170_ORIGIN_REGS      119
/* Expanded registers for 64-bit MSB. */
#define TOTAL_X170_EXPAND_REGS      36
/* Expanded registers for unified PP. */
#define TOTAL_X170_EXPAND_PP_REGS   9
/* Starting register for unified PP. */
#define PP_START_REGS               (TOTAL_X170_ORIGIN_REGS + TOTAL_X170_EXPAND_REGS)
/*starting register for error conceal */
#define ERROR_CONCEAL_START_REGS    315
#define TOTAL_ERROR_CONCEAL_REGS    2
/* Starting register of unified PP. */
#define PP_START_UNIFIED_REGS       320
#define TOTAL_X170_REGISTERS        (PP_START_UNIFIED_REGS + PP_UNIFIED_REGS)
#define DEC_X170_EXPAND_REGS        27

/* Max amount of stream */
#define DEC_X170_MAX_STREAM         ((1 << 24) - 1)   /* G1 */
#define DEC_X170_MAX_STREAM_G2      ((1 << 30) - 1) /* G2 */
#define DEC_X170_MAX_STREAM_VC8000D ((1 << 30) - 1) /* VC8000D */


/* Timeout value for the DWLWaitHwReady() call. */
/* Set to -1 for an unspecified value */
#ifndef DEC_X170_TIMEOUT_LENGTH
#define DEC_X170_TIMEOUT_LENGTH -1
#endif

/* Enable HW internal watchdog timeout IRQ */
#define DEC_X170_HW_TIMEOUT_INT_ENA 1

/* Memory wait states for reference buffer */
#define DEC_X170_REFBU_WIDTH 64
#define DEC_X170_REFBU_LATENCY 20
#define DEC_X170_REFBU_NONSEQ 8
#define DEC_X170_REFBU_SEQ 1

/* Check validity of the stream addresses */

#define X170_CHECK_BUS_ADDRESS(d) ((d) < 64 ? 1 : 0)
#define X170_CHECK_VIRTUAL_ADDRESS(d) (((void*)(d) < (void*)64) ? 1 : 0)
#define X170_CHECK_BUS_ADDRESS_AGLINED(d) (((d) < 64 || ((d) & (DEC_X170_BUS_BURST_LENGTH - 1))) ? 1 : 0)

/* Max number of macroblock rows to memset/memcpy in partial freeze error
 * concealment */
#define DEC_X170_MAX_EC_COPY_ROWS 8

/* Default amount of clock cycles before triggering timeout interrupt */
#define DEC_X170_TIMEOUT_CYCLES 0xFFFFFF

/* Default PP output swap */
#define DEC_X170_PP_OUTPUT_SWAP 0

/* Default linear buffer alignment */
#define DEC_X170_BUS_ADDR_ALIGNMENT 2048

/* PP standalone input reading block size */
#define HANTRODEC_PP_IN_BLK_SIZE_64X16 0
#define HANTRODEC_PP_IN_BLK_SIZE_64X64 1

#endif /* __DEC_X170_CFG_H__ */
