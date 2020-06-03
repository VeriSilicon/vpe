/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract :  Operations on Input line buffer.
--              For test/fpga_verification/example purpose. 
--
------------------------------------------------------------------------------*/

#ifndef ENC_INPUTLINEBUFREGISTER_H
#define ENC_INPUTLINEBUFREGISTER_H

#include "base_type.h"
#include "encasiccontroller.h"

typedef u32 (*getVCEncRdMbLines)(const void *inst);
typedef i32 (*setVCEncWrMbLines)(const void *inst, u32 lines);

typedef struct
{
    u8 *buf;
    u32 busAddress;
} lineBufMem;

/* struct for input mb line buffer */
typedef struct
{
    /* src picture related pointers */
    u8 *lumSrc;
    u8 *cbSrc;
    u8 *crSrc;

    /* line buffer SRAM related pointers */
    u8 *sram; /* line buffer virtual address */
    u32 sramBusAddr; /* line buffer bus address */
    u32 sramSize; /* line buffer size in bytes */
    u32 *hwSyncReg; /* virtual address of registers in line buffer, only for fpga verification purpose */

    lineBufMem lumBuf; /*luma address in line buffer */
    lineBufMem cbBuf; /*cb address in line buffer */
    lineBufMem crBuf; /*cr address in line buffer */

    /* encoding parameters */
    u32 inputFormat; /* format of input video */
    u32 pixOnRow; /* pixels in one line */
    u32 encWidth;
    u32 encHeight;
    u32 srcHeight;
    u32 srcVerOffset;
    u32 ctbSize;

    /* parameters of line buffer mode */
    i32 wrCnt;
    u32 depth;       /* number of MB_row lines in the input line buffer */
    u32 loopBackEn;
    u32 hwHandShake;

    /*functions */
    getVCEncRdMbLines getMbLines; /* get read mb lines from encoder register */
    setVCEncWrMbLines setMbLines; /* set written mb lines to encoder register */

    /* encoder instance */
    void *inst;

    /* asic control */
    asicData_s *asic;
}inputLineBufferCfg;

/* HW Register field names */
typedef enum {
    InputlineBufWrCntr,
    InputlineBufDepth,
    InputlineBufHwHandshake,
    InputlineBufPicHeight,
    InputlineBufRdCntr,
} lineBufRegName;

#define LINE_BUF_SWREG_AMOUNT   4 /*4x 32-bit*/

static const regField_s lineBufRegisterDesc[] = {
/* HW ID register, read-only */
    {InputlineBufWrCntr       , 0x000, 0x000001ff,  0, 0, RW, "slice_wr_cntr. +slice_depth when one slice is filled into slice_fifo"},
    {InputlineBufDepth        , 0x000, 0x0003fe00,  9, 0, RW, "slice_depth. unit is MB line"},
    {InputlineBufHwHandshake  , 0x000, 0x00040000, 18, 0, RW, "slice_hw_mode_en. active high. enable bit of slice_fifo hardware mode. should be disabled before the start of next frame."},
    {InputlineBufPicHeight    , 0x000, 0x0ff80000, 19, 0, RW, "pic_height. same value of swreg14[18:10] in H1."},
    {InputlineBufRdCntr       , 0x008, 0x000001ff,  0, 0, RO, "slice_rd_cntr. read only"},
};

void VCEncInitInputLineBufSrcPtr (inputLineBufferCfg *lineBufCfg);
i32  VCEncInitInputLineBufPtr (inputLineBufferCfg *lineBufCfg);
i32  VCEncInitInputLineBuffer(inputLineBufferCfg *lineBufCfg);
u32  VCEncStartInputLineBuffer(inputLineBufferCfg *lineBufCfg);
void VCEncInputLineBufDone (void *pAppData);
u32  VCEncInputLineBufPolling ();

#endif

