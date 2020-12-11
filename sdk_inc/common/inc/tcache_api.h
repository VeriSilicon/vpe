#ifndef __TCACHE_API_H__
#define __TCACHE_API_H__

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_INPUT_PLANE 3
#define MAX_OUTPUT_PLANE 2

#define TCACHE_RAW_INPUT_ALIGNMENT 16


typedef enum {
  TCACHE_COV_BD_8,
  TCACHE_COV_BD_RESERV1,
  TCACHE_COV_BD_10,
  TCACHE_COV_BD_RESERV3,
} TCACHE_COV_BD;

typedef enum {
  TCACHE_PIX_FMT_YUV420P,
  TCACHE_PIX_FMT_NV12,
  TCACHE_PIX_FMT_NV21,
  TCACHE_PIX_FMT_YUV420P10LE,
  TCACHE_PIX_FMT_YUV420P10BE,
  TCACHE_PIX_FMT_P010LE,
  TCACHE_PIX_FMT_P010BE,
  TCACHE_PIX_FMT_YUV422P,
  TCACHE_PIX_FMT_YUV422P10LE,
  TCACHE_PIX_FMT_YUV422P10BE,
  TCACHE_PIX_FMT_YUV444P,
  TCACHE_PIX_FMT_RGB24,
  TCACHE_PIX_FMT_BGR24,
  TCACHE_PIX_FMT_ARGB,
  TCACHE_PIX_FMT_RGBA,
  TCACHE_PIX_FMT_ABGR,
  TCACHE_PIX_FMT_BGRA,
  TCACHE_PIX_FMT_DTRC_PACKED_10_NV12,
} TCACHE_PIX_FMT;

typedef struct {
	
  //write related configuration
  TCACHE_PIX_FMT		write_format;
  TCACHE_COV_BD		write_bd;

  u32			writeStartAddr[MAX_INPUT_PLANE];
  u32			writeEndAddr[MAX_INPUT_PLANE];
  u32			writeStride[MAX_INPUT_PLANE];

  //handshake related configuration
  int			hs_enable; 
  u32			hs_image_height;
  int			hs_dma_ch;
  u32			hs_go_toggle_count;
  u32			hs_ce;

  //RGB color convert related configuration
  u32			RGB_COV_A;
  u32			RGB_COV_B;
  u32			RGB_COV_C;
  u32			RGB_COV_E;
  u32			RGB_COV_F;

  //read related configuration
  TCACHE_PIX_FMT		read_format;
  u32			read_count;
  u32			read_client;

  u32			readStartAddr[MAX_OUTPUT_PLANE];
  u32			readEndAddr[MAX_OUTPUT_PLANE];
  u32			readStride[MAX_OUTPUT_PLANE];
  u32			readValidStride[MAX_OUTPUT_PLANE];

  //dtrc read related configuration
  int			dtrc_read_enable;
  int			dtrc_bitdepth;
  u32			dtrc_frame_height;
  u32			dtrc_frame_stride;
  int			dtrc_read_luma_enable;
  u32			dtrc_read_luma_AXIID;
  u32			dtrc_read_luma_start_address;
  int			dtrc_read_chroma_enable;
  u32			dtrc_read_chroma_AXIID;
  u32			dtrc_read_chroma_start_address;

} TCACHE_PARAM;

typedef void * TCACHE_HANDLE;

#define BLOCK_LINE 64
#define BLOCK_NUM 4

int tcache_isRGB(TCACHE_PIX_FMT fmt);
int tcache_get_planes(TCACHE_PIX_FMT fmt);
int tcache_get_stride(int width, TCACHE_PIX_FMT fmt, int plane, int alignment);
int tcache_get_height(int height, TCACHE_PIX_FMT fmt, int plane);
int tcache_get_stride_align(int input);
int tcache_get_block_size(int stride, TCACHE_PIX_FMT fmt, int plane);
int tcache_get_block_height(TCACHE_PIX_FMT fmt, int plane);

TCACHE_PIX_FMT tcache_get_output_format(TCACHE_PIX_FMT fmt, int target_bit_depth);
TCACHE_COV_BD tcache_get_bitdepth(TCACHE_PIX_FMT fmt);

TCACHE_HANDLE TCACHE_init(u32 base_offset, char * device);
void TCACHE_release(TCACHE_HANDLE thd);

int TCACHE_config(TCACHE_HANDLE thd, TCACHE_PARAM * pParam);
int TCACHE_enable(TCACHE_HANDLE thd, int core_id);
int TCACHE_commit(TCACHE_HANDLE thd, int core_id);

int TCACHE_dump_regs(TCACHE_HANDLE thd, int core_id);
int TCACHE_swreset(TCACHE_HANDLE thd, int core_id);
int TCACHE_status(TCACHE_HANDLE thd, int core_id, u32 * pStatus);
int TCACHE_clear_int(TCACHE_HANDLE thd, int core_id, u32 int_status);

#ifdef __cplusplus
}
#endif

#endif /* __TCACHE_API_H__ */

