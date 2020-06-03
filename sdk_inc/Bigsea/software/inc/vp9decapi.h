/* Copyright 2015 Google Inc. All Rights Reserved. */
#ifndef VP9DECAPI_H
#define VP9DECAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "basetype.h"
#include "decapicommon.h"
#include "dectypes.h"

/*------------------------------------------------------------------------------
    API type definitions
------------------------------------------------------------------------------*/

/* Decoder instance */
typedef const void *Vp9DecInst;

enum DecBitDepth {
  DEC_BITDEPTH_INITIAL = 0, /* Match the bit depth of the first frame over
                               the whole sequence. */
  DEC_BITDEPTH_NATIVE = 1, /* Match the bit depth of the input stream. */
  DEC_BITDEPTH_8 = 8,      /* If over 8 bit deep input, downsample to 8. */
  DEC_BITDEPTH_10 = 10 /* If over 10 bit deep input, downsample to 10. */
};
enum DecBitPacking {
  DEC_BITPACKING_LSB = 0, /* Pack the pixel into the LSBs of each sample. */
  DEC_BITPACKING_MSB
};

/* Post-processor features. */
struct Vp9PpConfig {
  enum DecBitDepth bit_depth;
  enum DecBitPacking bit_packing;
};

/* Input structure */
struct Vp9DecInput {
  u8 *stream;             /* Pointer to the input */
  size_t stream_bus_address; /* DMA bus address of the input stream */
  u32 data_len;           /* Number of bytes to be decoded */
  u32 pic_id;             /* Identifier for the picture to be decoded */
};

/* Output structure */
struct Vp9DecOutput {
  u8 *strm_curr_pos; /* Pointer to stream position where decoding ended */
  u32 strm_curr_bus_address; /* DMA bus address location where the decoding
                                ended */
  u32 data_left; /* how many bytes left undecoded */
};

/* stream info filled by Vp9DecGetInfo */
struct Vp9DecInfo {
  u32 vp_version;
  u32 vp_profile;
  u32 bit_depth;
  u32 coded_width;   /* coded width */
  u32 coded_height;  /* coded height */
  u32 frame_width;   /* pixels width of the frame as stored in memory */
  u32 frame_height;  /* pixel height of the frame as stored in memory */
  u32 scaled_width;  /* scaled width of the displayed video */
  u32 scaled_height; /* scaled height of the displayed video */
  u32 dpb_mode;      /* DPB mode; frame, or field interlaced */
  enum DecPictureFormat output_format; /* format of the output picture */
  u32 pic_buff_size; /* number of picture buffers allocated&used by decoder */
  u32 multi_buff_pp_size; /* number of picture buffers needed in
                             decoder+postprocessor multibuffer mode */
};

typedef struct DecSwHwBuild Vp9DecBuild;

enum Vp9ColorSpace {
  VP9_YCbCr_BT601 = 0,
  VP9_CUSTOM,
  VP9_RGB = 7
};

enum Vp9ColorRange {
  VP9_STUDIO_SWING = 0,
  VP9_FULL_SWING = 1,
};

/* Output structure for Vp9DecNextPicture */
struct Vp9DecPicture {
  u32 coded_width;  /* coded width of the picture */
  u32 coded_height; /* coded height of the picture */
  u32 frame_width;  /* pixels width of the frame as stored in memory */
  u32 frame_height; /* pixel height of the frame as stored in memory */
  void *output_luma_base;   /* Pointer to the picture */
  size_t output_luma_bus_address;   /* Bus address of the luminance component */
  void *output_chroma_base; /* Pointer to the picture */
  size_t output_chroma_bus_address; /* Bus address of the chrominance component */
  unsigned char fc_modes[2][8]; /* Frame compression modes */
  u32 fc_num_tiles_col;         /* Number of column tiles, needed to parse compression info */
  u32 pic_id;                    /* Identifier of the Frame to be displayed */
  u32 is_intra_frame;            /* Indicates if Frame is an Intra Frame */
  u32 is_golden_frame; /* Indicates if Frame is a Golden reference Frame */
  u32 nbr_of_err_mbs;  /* Number of concealed MB's in the frame  */
  u32 num_slice_rows;
  u32 cycles_per_mb;   /* Avarage cycle count per macroblock */
  enum DecPictureFormat output_format;
  u32 bits_per_sample;
  enum Vp9ColorSpace color_space;
  enum Vp9ColorRange color_range;
};

/*------------------------------------------------------------------------------
    Prototypes of Decoder API functions
------------------------------------------------------------------------------*/

Vp9DecBuild Vp9DecGetBuild(void);

enum DecRet Vp9DecInit(Vp9DecInst *dec_inst, u32 use_video_freeze_concealment,
                       u32 num_frame_buffers, enum DecDpbFlags dpb_flags,
                       enum DecPictureFormat output_format,
                       u32 disable_late_hw_sync,
                       u32 use_reference_compression,
                       struct Vp9PpConfig pp_cfg, 
                       bool av1baseline);

void Vp9DecRelease(Vp9DecInst dec_inst);

enum DecRet Vp9DecDecode(Vp9DecInst dec_inst, const struct Vp9DecInput *input,
                         struct Vp9DecOutput *output);

enum DecRet Vp9DecNextPicture(Vp9DecInst dec_inst,
                              struct Vp9DecPicture *output);

enum DecRet Vp9DecPictureConsumed(Vp9DecInst dec_inst,
                                  const struct Vp9DecPicture *picture);

enum DecRet Vp9DecEndOfStream(Vp9DecInst dec_inst);

enum DecRet Vp9DecGetInfo(Vp9DecInst dec_inst, struct Vp9DecInfo *dec_info);

#ifdef __cplusplus
}
#endif

#endif /* VP9DECAPI_H */
