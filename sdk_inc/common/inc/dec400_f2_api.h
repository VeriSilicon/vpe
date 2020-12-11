#ifndef	__DEC400_F2_API__H__
#define	__DEC400_F2_API__H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_READ_STREAM		6
#define MAX_WRITE_STREAM	2

typedef enum {
  F2_COMP_FMT_YUV_ONLY = 5,
  F2_COMP_FMT_UV_MIX,
} F2_COMP_FMT;

typedef enum {
  F2_COMP_ALIGN_256B = 4,
  F2_COMP_ALIGN_512B,
} F2_ALIGN_MODE;

typedef enum {
  F2_TILE_MODE_64x4  = 0x7,
  F2_TILE_MODE_128x4 = 0x18,
  F2_TILE_MODE_256x4 = 0x19,
  F2_TILE_MODE_128x8 = 0x1E,
} F2_TILE_MODE;

typedef enum {
  F2_BIT_DEPTH_8,
  F2_BIT_DEPTH_10,
  F2_BIT_DEPTH_12,
  F2_BIT_DEPTH_16,
  F2_BIT_DEPTH_14,
} F2_BIT_DEPTH;

typedef enum {
  F2_STREAM_MODE_REFY0 = 1,
  F2_STREAM_MODE_REFUV0,
  F2_STREAM_MODE_REFY1,
  F2_STREAM_MODE_REFUV1,
  F2_STREAM_MODE_INPUTY,
  F2_STREAM_MODE_INPUTUV,
} F2_STREAM_MODE;

#define F2_STREAM_MODE_REFY  F2_STREAM_MODE_REFY0
#define F2_STREAM_MODE_REFUV F2_STREAM_MODE_REFUV0

typedef enum {
  F2_BUTT_JOINT_VC8000E = 0,
  F2_BUTT_JOINT_VP9,
} F2_BUTT_JOINT;

typedef struct {

  F2_BUTT_JOINT	ButtJoint;

  int read_miss_policy;
  int write_miss_policy;
  int read_OT_cnt;
  int write_OT_cnt;

} F2_DEC_GLOBAL_CFG;

typedef struct {

  int				compression_enable;
  F2_COMP_FMT		compression_format;
  F2_ALIGN_MODE	compression_align_mode1;
  F2_TILE_MODE	tile_mode;
  F2_STREAM_MODE	stream_mode;
  F2_BIT_DEPTH	bit_depth;

  u32				buf_base;
  u32				cache_base;
  u32				buf_end;

} F2_DEC_STREAM_CFG;

#define READ_STREAM  0
#define WRITE_STREAM 1

typedef void * F2_DEC_HANDLE;

F2_DEC_HANDLE F2_DEC_init(char * device);
void F2_DEC_release(F2_DEC_HANDLE hd);

int F2_DEC_global_config(F2_DEC_HANDLE hd, F2_DEC_GLOBAL_CFG * pGlbCfg);
int F2_DEC_stream_config(F2_DEC_HANDLE hd, int rw, F2_DEC_STREAM_CFG * pStreamCfg);

int F2_DEC_enable(F2_DEC_HANDLE hd, int core_id);
int F2_DEC_flush(F2_DEC_HANDLE hd, int core_id);
int F2_DEC_DisableAllStreams(F2_DEC_HANDLE hd, int core_id);

int F2_DEC_dump_regs(F2_DEC_HANDLE hd, int core_id);

#ifdef __cplusplus
}
#endif

#endif /* __DEC400_F2_API__H__ */

