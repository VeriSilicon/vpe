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
--  Abstract  :   Picture parameter set
--
------------------------------------------------------------------------------*/

#ifndef VP9PIC_PARAMETER_SET_H
#define VP9PIC_PARAMETER_SET_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
#define SGM_CNT 4

#define SGM_BG 0
#define SGM_AROI 1
#define SGM_ROI1 2
#define SGM_ROI2 3

typedef struct sgm
{
  bool mapModified; /* Segmentation map has been modified */
  i32  idCnt[SGM_CNT];  /* Id counts because of probability */
  /* Segment ID map is stored in ASIC SW/HW mem regs->segmentMap */
} sgm;

typedef struct
{
  struct sgm sgm;   /* Segmentation data */
  i32 qp;     /* Final qp value of current macroblock */
  bool segmentEnabled;  /* Segmentation enabled */
  i32 qpSgm[SGM_CNT]; /* Qp if segments enabled (encoder set) */
  i32 levelSgm[SGM_CNT];  /* Level if segments enabled (encoder set) */
  i32 sgmQpMapping[SGM_CNT];/* Map segments: AROI, ROI1, ROI2 into IDs */
} pps;

typedef struct
{
  pps *store;   /* Picture parameter set tables */
  i32 size;   /* Size of above storage table */
  pps *pps;   /* Active picture parameter set */
  pps *prevPps;   /* Previous picture parameter set */
  i32 qpSgm[SGM_CNT]; /* Current qp and level of segmentation... */
  i32 levelSgm[SGM_CNT];  /* ...which are written to the stream */
} ppss;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
i32 PicParameterSetAlloc(ppss *ppss, i32 mbPerPicture);
void PicParameterSetFree(ppss *ppss);

#endif
