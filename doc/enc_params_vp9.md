Verisilicon platform VP9 encoder enc_params user manual

The enc_params parameter fully follow FFmpeg rule, for example:
-enc_params "ref_frame_scheme=4:lag_in_frames=18:passes=2:bitrate_window=60:effort=0:intra_pic_rate=60"
|params name        |Type        |Min     |Max      |Description      |
|:------------------|:-----------|:-------|:--------|:----------------|
|intra_pic_rate		|int		 |0	      |65535	|Intra picture rate in frames. |
|bitrate_window		|int		 |0	      |300		|Bitrate window length in frames.|
|qp_hdr				|int		 |0	      |255		|Initial QP used for the first frame. |
|qp_min				|int		 |0	      |255		|Minimum frame header QP.|
|qp_max				|int		 |0	      |255		|Maximum frame header QP.|
|fixed_intra_qp		|int		 |0	      |255		|Fixed Intra QP 0 = disabled.|
|picRc				|int		 |0	      |1		|0=OFF, 1=ON Picture rate control enable. |
|mcomp_filter_type	|int		 |0	      |4		|Interpolation filter mode. |
|effort				|int		 |0	      |5		|Encoder effort level 0=fastest 5=best quality|
|ref_frame_scheme	|int		 |0	      |5		|Reference frame update scheme. Values TBD.|
|filte_level		|int		 |0	      |64		|Filter strength level for deblocking|
|filter_sharpness	|int		 |0	      |8		|0..8 8=auto Filter sharpness for deblocking.|
|lag_in_frames		|int		 |0	      |25		|Number of frames to lag. Up to 25. |
|passes				|int		 |0	      |2		|Number of passes (1/2). |

