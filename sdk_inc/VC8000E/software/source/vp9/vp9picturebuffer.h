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

#ifndef VP9PICTURE_BUFFER_H
#define VP9PICTURE_BUFFER_H

/*------------------------------------------------------------------------------
  Include headers
------------------------------------------------------------------------------*/
#include "base_type.h"
#include "vp9entropytools.h"
#include "encasiccontroller.h"

/*------------------------------------------------------------------------------
  External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
  Module defines
------------------------------------------------------------------------------*/
#define BUFFER_SIZE 3

typedef struct
{
  i32 lumWidth;   /* Width of *lum */
  i32 lumHeight;    /* Height of *lum */
  i32 chWidth;    /* Width of *cb and *cr */
  i32 chHeight;   /* Height of *cb and *cr */
  u32 lum;
  u32 cb;
} picture;

typedef struct refPic
{
  picture picture;  /* Image data */
  entropy *entropy; /* Entropy store of picture */
  i32 poc;    /* Picture order count */

  bool i_frame;   /* I frame (key frame), only intra mb */
  bool p_frame;   /* P frame, intra and inter mb */
  bool show;    /* Frame is for display (showFrame flag) */
  bool ipf;   /* Frame is immediately previous frame */
  bool arf;   /* Frame is altref frame */
  bool grf;   /* Frame is golden frame */
  bool search;    /* Frame is used for motion estimation */
  struct refPic *refPic;  /* Back reference pointer to itself */
} refPic;

typedef struct
{
  i32 size;   /* Amount of allocated reference pictures */
  picture input;    /* Input picture */
  refPic refPic[BUFFER_SIZE + 1]; /* Reference picture store */
  refPic refPicList[BUFFER_SIZE]; /* Reference picture list */
  refPic *cur_pic;  /* Pointer to picture under reconstruction */
  refPic *last_pic; /* Last picture */
} picBuffer;

/*------------------------------------------------------------------------------
  Function prototypes
------------------------------------------------------------------------------*/
i32 PictureBufferAlloc(picBuffer *picBuffer, i32 width, i32 height);
void PictureBufferFree(picBuffer *picBuffer);
void InitializePictureBuffer(picBuffer *picBuffer);
void UpdatePictureBuffer(picBuffer *picBuffer);
void PictureBufferSetRef(picBuffer *picBuffer, asicData_s *asic);

#endif
