#ifndef __DTRC_API_H__
#define __DTRC_API_H__


#include "base_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DTRC_INPUT_WIDTH_ALIGNMENT 8
#define DTRC_INPUT_HEIGHT_ALIGNMENT 8

typedef struct {

  int interrupt_en;

  u32 id_y;
  u32 id_uv;
  u32 id_ybp;
  u32 id_uvbp;	

  //DTCTRL 0xC8
  int bp_byARADDR;
  int output_be;
  int merge_G1G2_ARID;
  u32 bp_swap;
  u32 table_swap;
  u32 tile_swap;
  u32 raster_swap;
  u32 max_burst_length;
  int is_G1;
  int bp_mode;

} DTRC_GLOBAL_CONFIG;

typedef struct {
	
	u32 SrcWidth;
	u32 SrcHeight;
	int type_main8;
	int dc_bypass;

	u64 YDataAddr;
	u64 UVDataAddr;
	u64 YTableAddr;
	u64 UVTableAddr;

	u64 YSSA;
	u64 YSEA;
	u64 UVSSA;
	u64 UVSEA;
	
	int crop_en;
	u32 crop_x;
	u32 crop_y;
	u32 crop_w;
	u32 crop_h;

} DTRC_FRAME;

typedef void * DTRC_HANDLE;

DTRC_HANDLE DTRC_init(u32 base_offset, char * device);
void DTRC_release(DTRC_HANDLE hd);

int DTRC_global_config(DTRC_HANDLE hd, DTRC_GLOBAL_CONFIG * pGlobalCfg);
int DTRC_frame_config(DTRC_HANDLE hd, DTRC_FRAME *dtrc_para);
int DTRC_enable(DTRC_HANDLE hd, int core_id);

int DTRC_sw_reset(DTRC_HANDLE hd, int core_id);
int DTRC_get_int_status(DTRC_HANDLE hd, int core_id, u32 * pStatus);
int DTRC_clear_int(DTRC_HANDLE hd, int core_id, u32 int_status);

int DTRC_dump_regs(DTRC_HANDLE hd, int core_id);

#ifdef __cplusplus
}
#endif

#endif /* __DTRC_API_H__ */

