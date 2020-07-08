Verisilicon platform H26X encoder enc_params user manual

The enc_params parameter fully follow FFmpeg rule, for example:
-enc_params "ref_frame_scheme=4:lag_in_frames=18:passes=2:bitrate_window=60:effort=0:intra_pic_rate=60"
|params name              |Type|  Min|    Max|  Description                     |
|:------------------------|:---|:----|:------|:---------------------------------|
|force8bit|                 INT|    0|      1|  force to output 8bit stream.    |
|intra_pic_rate|            INT|    0|      -|  Intra picture rate in frames. (default [0])Forces every Nth frame to be encoded as intra frame.|
|bitrate_window|            INT| -255|    300|  Bitrate window length in frames. (default [intra_pic_rate]).|
|intra_qp_delta|            INT| -255|    127|  Intra QP delta. (default [26], -1..51).[Bigsea] Intra QP delta. (default [0], -127..127).|
|qp_hdr|                    INT|   -1|    255|  Initial QP used for the first frame. (default [-1], -1..51).[Bigsea] Initial QP used for the first frame. (default [-1 or 50], -1..255).|
|qp_min|                    INT|    0|    255|  Minimum frame header QP for any slices. (default [0], 0..51).[Bigsea] Minimum frame header QP. (default [10], 0..255).|
|qp_max|                    INT|    0|    255|  Maximum frame header QP for any slices. (default [51], 0..51).[Bigsea] Maximum frame header QP. (default [255], 0..255)|
|fixed_intra_qp|            INT|    0|    255|  Fixed Intra QP,0|= disabled. (default [0], 0..51).[Bigsea] Fixed Intra QP,0|= disabled. (default [0], 0..255).|
|pic_skip|                  INT|    0|      1|  Enable picture skip rate control.|
|bitdepth|                  INT|    0|      1|  Bitdepth. 0=8-bit, 1=10-bit. [default: keep the same as input].|
|tier|                      INT|    0|      1|  [VC8000E/HEVC] encoder only (default [0], 0..1)0 -main tier1 -high tier|
|byte_stream|               INT|    0|      1|  stream type (default [1], 0..1)0 - NAL units. Nal sizes returned in <nal_sizes.txt>1 - byte stream according to Hevc Standard Annex B.|
|video_range|               INT|    0|      1|  [VC8000E/HEVC] Video signal sample range value in Hevc stream. (default [0], 0..1)0 - Y range in [16..235] Cb,Cr in [16..240]1 - Y,Cb,Cr range in [0..255]|
|sei|                       INT|    -|      -|  enable SEI messages.|
|disable_cabac|             INT|    0|      1|  disable cabac, use cavlc.|
|slice_size|                INT|    0|      -|  slice size in number of CTU rows. (default [0], 0..height/ctu_size)0 - to encode each picture in one slice1..height/ctu_size - to each slice with N CTU row|
|tol_moving_bitrate|        INT|    0|   2000|  percent tolerance over target bitrate of moving bit rate (default [2000], 0..2000%%).|
| bit_var_range_I|          INT|    10| 10000|  percent variations over average bits per frame for I frame. (default [10000], 10..10000%%).|
| bit_var_range_P|          INT|    10| 10000|  percent variations over average bits per frame for P frame. (default [10000], 10..10000%%).|
| bit_var_range_B|          INT|    10| 10000|  percent variations over average bits per frame for B frame. (default [10000], 10..10000%%).|
|enable_pic_rc|             INT|  -255|     1|  enable picture rate control. Calculates new target QP for every frame.|
|ctb_rc|                    INT|    0|      3|  CTB QP adjustment mode for Rate Control and Subjective Quality. (default [0], 0..3).0 = No CTB QP adjustment (best PSNR).1 = CTB QP adjustment for Subjective Quality only.2 = CTB QP adjustment for Rate Control only(suggest, best bitrate).3 = CTB QP adjustment for both Subjective Quality and Rate Control.|
|tol_ctb_rc_inter|          FLOAT|  -|      -|  Tolerance of Ctb Rate Control for INT|ER frames. (float point number). (default [0.0])Ctb Rc will try to limit INT|ER frame bits within the range of:\t[targetPicSize/(1+tol_ctb_rc_inter), targetPicSize*(1+tol_ctb_rc_inter)].|
|tol_ctb_rc_intra|          FLOAT|  -|      -|  Tolerance of Ctb Rate Control for INT|RA frames. (float point number). (default [-1.0])|
|ctb_row_qp_step|           INT|    -|  -|      The maximum accumulated QP adjustment step per CTB Row allowed by Ctb Rate Control.Default value is [4] for H264 and [16] for HEVC.QP_step_per_CTB = (ctbRowQpStep / Ctb_per_Row) and limited by maximum = 4.|
|pic_qp_deelta_range|       INT|    -|      -|  Min:Max. Qp_Delta Range in Picture RC.Min: -1..-10 Minimum Qp_Delta in Picture RC. [-2]Max: 1..10 Maximum Qp_Delta in Picture RC. [3]|
|hrd_conformance|           INT|    0|      1|  enable HRD conformance. Uses standard defined model to limit bitrate variance.|
|cpb_size|                  INT|    -|      -|  HRD Coded Picture Buffer size in bits. default [1000000], suggest 2xbitrate.|
|gop_size|                  INT|    0|      8|  GOP Size. (default [0], 0..8).0 for adaptive GOP size; 1~7 for fixed GOP size.|
|gop_lowdelay|              INT|    0|      1|  Enable default lowDelay GOP configuration if --gopConfig not specified, only valid for GOP size <= 4.|
|qp_min_I|                  INT|    0|     51|  minimum frame header QP overriding qp_min for I slices. (default [0], 0..51).|
|qp_max_I|                  INT|    0|     51|  maximum frame header QP overriding qp_max for I slices. (default [51], 0..51).|
|bframe_qp_delta|           INT|    -1|    51|  BFrame QP Delta. (default [-1], -1..51).|
|chroma_qp_offset|          INT|    -12|   12|  Chroma QP offset. (default [0], -12..12).|
|vbr|                       INT|    0|      1|  enable variable Bit Rate Control by qp_min.|
|user_data|                 STR|    -|       |  SEI User data file name. File is read and inserted as SEI message before first frame.|
|intra_area|                INT|    -|      -|  left : top : right : bottom. CTB coordinatesspecifying rectangular area of CTBs to force encoding in intra mode.|
|ipcm1_area|                INT|    -|      -|  left : top : right : bottom. CTB coordinatesspecifying rectangular area of CTBs to force encoding in IPCM mode.|
|ipcm2_area|                INT|    -|      -|  left : top : right : bottom. CTB coordinatesspecifying rectangular area of CTBs to force encoding in IPCM mode.|
|enable_const_chroma|       INT|    0|      1|  enable setting chroma a constant pixel value.|
|const_cb|                  INT|    0|   1023|  The constant pixel value for Cb.(for 8bit default [128], 0..255, for 10bit default [512], 0..1023).|
|const_cr|                  INT|    0|   1023|  The constant pixel value for Cr.(for 8bit default [128], 0..255, for 10bit default [512], 0..1023).|
|rdo_level|                 INT|    1|     3 |  [VC8000E/HEVC] programable HW RDO Level (default [1], 1..3).|
|disable_ssim|              INT|    0|  1    |  Disable SSIM Calculation.|
|disable_vui_timing_info|   INT|    0|  1    |  Disable Write VUI timing info in SPS.|
|lookahead_depth|           INT|    0|  40   |  Number of frames to lookahead. Up to 40. [0]|

