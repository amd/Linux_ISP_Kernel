/*
 * Copyright 2024 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef PARAM_TYPES_PUB_H
#define PARAM_TYPES_PUB_H

#include "base_types_pub.h"

#define INTERNAL_MEMORY_POOL_SIZE (200 * 1024 * 1024)

/*
 * @def MAX_OUTPUT_MCSC
 * The max output port of mcsc sub-Ip
 * output0 - Preview
 * output1 - video
 * output2 - still
 */
#define MAX_OUTPUT_MCSC                 (3)

/* Command Structure */

/*
 * @struct cmd_t
 * The host command is 64 bytes each.
 * The format of the command is defined as following
 */
struct cmd_t {
	uint32 cmd_seq_num;
	uint32 cmd_id;
	uint32 cmd_param[12];
	uint16 cmd_stream_id;
	uint8 cmd_silent_resp;
	uint8 reserved;
#ifdef CONFIG_ENABLE_CMD_RESP_256_BYTE
	uint8  reserved_1[192];
#endif
	uint32 cmd_check_sum;
};

/*
 * @enum cmd_param_package_direction_t
 * The direction of param package in command
 */
enum cmd_param_package_direction_t {
	CMD_PARAM_PACKAGE_DIRECTION_INVALID     = 0, /* Invalid param */
	CMD_PARAM_PACKAGE_DIRECTION_GET         = 1, /* Host get data from FW */
	CMD_PARAM_PACKAGE_DIRECTION_SET         = 2, /* Host set data to FW */
	CMD_PARAM_PACKAGE_DIRECTION_BIDIRECTION = 3, /* Host and FW access data both */
	CMD_PARAM_PACKAGE_DIRECTION_MAX         = 4  /* Invalid param */
};

/*
 * @struct  _cmd_param_package_t
 * The Definition of command parameter package structure
 */
struct cmd_param_package_t {
	uint32 package_addr_lo;    /* The low 32 bit address of the package address. */
	uint32 package_addr_hi;    /* The high 32 bit address of the package address. */
	uint32 package_size;      /* The total package size in bytes. */
	uint32 package_check_sum; /* The byte sum of the package. */
};

/* Response Structure */

/*
 * @struct  _resp_t
 * The Definition of command response structure.
 * The struct resp_t should be totally 64 bytes.
 */
struct resp_t {
	uint32 resp_seq_num;
	uint32 resp_id;
	uint32 resp_param[12];

	uint8  reserved[4];
#ifdef CONFIG_ENABLE_CMD_RESP_256_BYTE
	uint8  reserved_1[192]; /* reserved_1 for 256 byte align use */
#endif
	uint32 resp_check_sum;
};

/*
 * @struct  _resp_param_package_t
 * The Definition of command response param package structure
 */
struct resp_param_package_t {
	uint32 package_addr_lo;   /* The low 32 bit address of the package address. */
	uint32 package_addr_hi;   /* The high 32 bit address of the package address. */
	uint32 package_size;     /* The total package size in bytes. */
	uint32 package_check_sum; /* The byte sum of the package. */
};

/*
 * @enum cmd_chan_id_t
 * The enum definition of command channel ID
 */
enum cmd_chan_id_t {
	CMD_CHAN_ID_INVALID     = -1,   /* Invalid ID */
	CMD_CHAN_ID_STREAM_1    =  0,   /* Stream1 channel ID */
	CMD_CHAN_ID_STREAM_2    =  1,   /* Stream2 channel ID */
	CMD_CHAN_ID_STREAM_3    =  2,   /* Stream3 channel ID */
	CMD_CHAN_ID_ASYNC       =  3,   /* Async channel ID */
	CMD_CHAN_ID_MAX         =  4    /* Max value if command channel ID */
};

/*
 * @enum resp_chan_id_t
 * The enum definition of response channel ID
 */
enum resp_chan_id_t {
	RESP_CHAN_ID_INVALID    = -1,   /* Invalid ID */
	RESP_CHAN_ID_STREAM_1   =  0,   /* Stream1 channel ID */
	RESP_CHAN_ID_STREAM_2   =  1,   /* Stream2 channel ID */
	RESP_CHAN_ID_STREAM_3   =  2,   /* Stream3 channel ID */
	RESP_CHAN_ID_GLOBAL     =  3,   /* global channel ID */
	RESP_CHAN_ID_MAX        =  4    /* Max value if command channel ID */
};

/*
 * @struct window_t
 *The struct definition of window structure
 */
struct window_t {
	uint32 h_offset;         /* The offset of window horizontal direction */
	uint32 v_offset;         /* The offset of window vertical direction */
	uint32 h_size;           /* The size of window horizontal direction */
	uint32 v_size;           /* The size of window vertical direction */
};

/*
 * @struct point_t
 * The struct definition of a point structure
 */
struct point_t {
	uint32 x; /* The x coordinate of the point */
	uint32 y; /* The y coordinate of the point */
};

/*
 * @enum stream_id_t
 * The enum definition of stream Id
 */
enum stream_id_t {
	STREAM_ID_INVALID = -1, /* STREAM_ID_INVALID. */
	STREAM_ID_1 = 0,        /* STREAM_ID_1. */
	STREAM_ID_2 = 1,        /* STREAM_ID_2. */
	STREAM_ID_3 = 2,        /* STREAM_ID_3. */
	STREAM_ID_MAXIMUM       /* STREAM_ID_MAXIMUM. */
};

/*
 * @enum sensor_shutter_type_t
 * Sensor shutter type
 */
enum sensor_shutter_type_t {
	SENSOR_SHUTTER_TYPE_GLOBAL,     /* Gloable shutter */
	SENSOR_SHUTTER_TYPE_ROLLING,    /* Rolling shutter */
	SENSOR_SHUTTER_TYPE_MAX         /* Max */
};

/*
 * @struct pd_output_type_t
 * Sensor pd output prop
 */
enum pd_output_type_t {
	PD_OUTPUT_INVALID = 0,        /* pd output invalid */
	PD_OUTPUT_PIXEL   = 1,        /* pd output pixel */
	PD_OUTPUT_DATA    = 2,        /* pd output data */
	PD_OUTPUT_MAX,                /* pd output max */
};

/*
 * @enum cfapattern_t
 * The enum definition of Sensor Color filter array pattern
 */
enum cfa_pattern_t {
	CFA_PATTERN_INVALID      = 0,   /* CFA_PATTERN_INVALID */
	CFA_PATTERN_RGGB         = 1,   /* CFA_PATTERN_RGGB */
	CFA_PATTERN_GRBG         = 2,   /* CFA_PATTERN_GRBG */
	CFA_PATTERN_GBRG         = 3,   /* CFA_PATTERN_GBRG */
	CFA_PATTERN_BGGR         = 4,   /* CFA_PATTERN_BGGR */
	CFA_PATTERN_PURE_IR     =  5,   /* CFA_PATTERN_PURE_IR */
	CFA_PATTERN_RIGB         = 6,   /* CFA_PATTERN_RIGB */
	CFA_PATTERN_RGIB         = 7,   /* CFA_PATTERN_RGIB */
	CFA_PATTERN_IRBG         = 8,   /* CFA_PATTERN_IRBG */
	CFA_PATTERN_GRBI         = 9,   /* CFA_PATTERN_GRBI */
	CFA_PATTERN_IBRG         = 10,  /* CFA_PATTERN_IBRG */
	CFA_PATTERN_GBRI         = 11,  /* CFA_PATTERN_GBRI */
	CFA_PATTERN_BIGR         = 12,  /* CFA_PATTERN_BIGR */
	CFA_PATTERN_BGIR         = 13,  /* CFA_PATTERN_BGIR */
	CFA_PATTERN_BGRGGIGI     = 14,  /* CFA_PATTERN_BGRGGIGI */
	CFA_PATTERN_RGBGGIGI     = 15,  /* CFA_PATTERN_RGBGGIGI */
	CFA_PATTERN_MAX                 /* CFA_PATTERN_MAX */
};

/*
 * @enum image_format_t
 * The enum definition of image format
 */
enum image_format_t {
	IMAGE_FORMAT_INVALID,               /* invalid */
	IMAGE_FORMAT_NV12,                  /* 4:2:0,semi-planar, 8-bit */
	IMAGE_FORMAT_NV21,                  /* 4:2:0,semi-planar, 8-bit */
	IMAGE_FORMAT_I420,                  /* 4:2:0,planar, 8-bit */
	IMAGE_FORMAT_YV12,                  /* 4:2:0,planar, 8-bit */
	IMAGE_FORMAT_YUV422PLANAR,          /* 4:2:2,planar, 8-bit */
	IMAGE_FORMAT_YUV422SEMIPLANAR,      /* semi-planar, 4:2:2,8-bit */
	IMAGE_FORMAT_YUV422INTERLEAVED,     /* interleave, 4:2:2, 8-bit */
	IMAGE_FORMAT_P010,                  /* semi-planar, 4:2:0, 10-bit */
	IMAGE_FORMAT_Y210,                  /* interleave, 4:2:2, 10-bit */
	IMAGE_FORMAT_L8,                    /* Only Y 8-bit */
	IMAGE_FORMAT_RGBBAYER8,             /* RGB bayer 8-bit */
	IMAGE_FORMAT_RGBBAYER10,            /* RGB bayer 10-bit */
	IMAGE_FORMAT_RGBBAYER12,            /* RGB bayer 12-bit */
	IMAGE_FORMAT_RGBBAYER14,            /* RGB bayer 14-bit */
	IMAGE_FORMAT_RGBBAYER16,            /* RGB bayer 16-bit */
	IMAGE_FORMAT_RGBBAYER20,            /* RGB bayer 20-bit */
	IMAGE_FORMAT_RGBIR8,                /* RGBIR 8-bit */
	IMAGE_FORMAT_RGBIR10,               /* RGBIR 10-bit */
	IMAGE_FORMAT_RGBIR12,               /* RGBIR 12-bit */
	IMAGE_FORMAT_Y210BF,                /* interleave, 4:2:2, 10-bit bubble free */
	IMAGE_FORMAT_RGB888,                /* RGB 888 */
	IMAGE_FORMAT_BAYER12,               /* Bayer 12-bit */
	IMAGE_FORMAT_RAWDATA,               /* Raw unformatted data */
	IMAGE_FORMAT_MAX                    /* Max value of enum image_format_t */
};

/*
 * @enum mipi_virtual_channel_t
 * The enum definition of Mipi pipe HW virtual channel
 */
enum mipi_virtual_channel_t {
	MIPI_VIRTUAL_CHANNEL_0 = 0x0, /* virtual channel 0 */
	MIPI_VIRTUAL_CHANNEL_1 = 0x1, /* virtual channel 1 */
	MIPI_VIRTUAL_CHANNEL_2 = 0x2, /* virtual channel 2 */
	MIPI_VIRTUAL_CHANNEL_3 = 0x3, /* virtual channel 3 */
	MIPI_VIRTUAL_CHANNEL_MAX      /* virtual channel max */
};

/*
 * @enum mipi_data_type_t
 * The enum definition of Mipi received data type
 */
enum mipi_data_type_t {
	MIPI_DATA_TYPE_FSC              = 0x00, /* frame start code */
	MIPI_DATA_TYPE_FEC              = 0x01, /* frame end code */
	MIPI_DATA_TYPE_LSC              = 0x02, /* line start code */
	MIPI_DATA_TYPE_LEC              = 0x03, /* line end code */

	/* 0x04 .. 0x07 reserved */

	MIPI_DATA_TYPE_GSPC1            = 0x08, /* generic short packet code 1 */
	MIPI_DATA_TYPE_GSPC2            = 0x09, /* generic short packet code 2 */
	MIPI_DATA_TYPE_GSPC3            = 0x0A, /* generic short packet code 3 */
	MIPI_DATA_TYPE_GSPC4            = 0x0B, /* generic short packet code 4 */
	MIPI_DATA_TYPE_GSPC5            = 0x0C, /* generic short packet code 5 */
	MIPI_DATA_TYPE_GSPC6            = 0x0D, /* generic short packet code 6 */
	MIPI_DATA_TYPE_GSPC7            = 0x0E, /* generic short packet code 7 */
	MIPI_DATA_TYPE_GSPC8            = 0x0F, /* generic short packet code 8 */

	MIPI_DATA_TYPE_NULL             = 0x10, /* null */
	MIPI_DATA_TYPE_BLANKING         = 0x11, /* blanking data */
	MIPI_DATA_TYPE_EMBEDDED         = 0x12, /* embedded 8-bit non image data */

	/* 0x13 .. 0x17 reserved */

	MIPI_DATA_TYPE_YUV420_8         = 0x18, /* YUV 420 8-Bit */
	MIPI_DATA_TYPE_YUV420_10        = 0x19, /* YUV 420 10-Bit */
	MIPI_DATA_TYPE_LEGACY_YUV420_8  = 0x1A, /* YUV 420 8-Bit */
	/* 0x1B reserved */
	MIPI_DATA_TYPE_YUV420_8_CSPS    = 0x1C, /* YUV 420 8-Bit (chroma shifted pixel sampling) */
	MIPI_DATA_TYPE_YUV420_10_CSPS   = 0x1D, /* YUV 420 10-Bit (chroma shifted pixel sampling) */
	MIPI_DATA_TYPE_YUV422_8         = 0x1E, /* YUV 422 8-Bit */
	MIPI_DATA_TYPE_YUV422_10        = 0x1F, /* YUV 422 10-Bit */

	MIPI_DATA_TYPE_RGB444           = 0x20, /* RGB444 */
	MIPI_DATA_TYPE_RGB555           = 0x21, /* RGB555 */
	MIPI_DATA_TYPE_RGB565           = 0x22, /* RGB565 */
	MIPI_DATA_TYPE_RGB666           = 0x23, /* RGB666 */
	MIPI_DATA_TYPE_RGB888           = 0x24, /* RGB888 */

	/* 0x25 .. 0x27 reserved */

	MIPI_DATA_TYPE_RAW_6            = 0x28, /* RAW6 */
	MIPI_DATA_TYPE_RAW_7            = 0x29, /* RAW7 */
	MIPI_DATA_TYPE_RAW_8            = 0x2A, /* RAW8 */
	MIPI_DATA_TYPE_RAW_10           = 0x2B, /* RAW10 */
	MIPI_DATA_TYPE_RAW_12           = 0x2C, /* RAW12 */
	MIPI_DATA_TYPE_RAW_14           = 0x2D, /* RAW14 */
	MIPI_DATA_TYPE_RAW_16           = 0x2E, /* RAW16 */

	/* 0x2E .. 0x2F reserved */

	MIPI_DATA_TYPE_USER_1           = 0x30, /* user defined 1 */
	MIPI_DATA_TYPE_USER_2           = 0x31, /* user defined 2 */
	MIPI_DATA_TYPE_USER_3           = 0x32, /* user defined 3 */
	MIPI_DATA_TYPE_USER_4           = 0x33, /* user defined 4 */
	MIPI_DATA_TYPE_USER_5           = 0x34, /* user defined 5 */
	MIPI_DATA_TYPE_USER_6           = 0x35, /* user defined 6 */
	MIPI_DATA_TYPE_USER_7           = 0x36, /* user defined 7 */
	MIPI_DATA_TYPE_USER_8           = 0x37, /* user defined 8 */
	MIPI_DATA_TYPE_MAX
};

/*
 * j@enum mipi_comp_scheme_t
 * The enum definition of Mipi compact scheme type
 */
enum mipi_comp_scheme_t {
	MIPI_COMP_SCHEME_NONE    = 0,   /* NONE */
	MIPI_COMP_SCHEME_12_8_12 = 1,   /* 12_8_12 */
	MIPI_COMP_SCHEME_12_7_12 = 2,   /* 12_7_12 */
	MIPI_COMP_SCHEME_12_6_12 = 3,   /* 12_6_12 */
	MIPI_COMP_SCHEME_10_8_10 = 4,   /* 10_8_10 */
	MIPI_COMP_SCHEME_10_7_10 = 5,   /* 10_7_10 */
	MIPI_COMP_SCHEME_10_6_10 = 6,   /* 10_6_10 */
	MIPI_COMP_SCHEME_MAX            /* Max */
};

/*
 * @enum mipi_pred_block_t
 * The enum definition of Mipi Predictor block type
 */
enum mipi_pred_block_t {
	MIPI_PRED_BLOCK_INVALID = 0,   /* invalid */
	MIPI_PRED_BLOCK_1       = 1,   /* Predictor1 (simple algorithm) */
	MIPI_PRED_BLOCK_2       = 2,   /* Predictor2 (more complex algorithm) */
	MIPI_PRED_BLOCK_MAX            /* Max value of MIPI_PRED_BLOCK_MAX */
};

/*
 * @enum mipi_pipe_input_t
 * The enum definition of Mipi Form0 input type
 */
enum mipi_pipe_input_t {
	MIPI_PIPE_INPUT_MIPI		= 0, /* input data from mipi csi */
	MIPI_PIPE_INPUT_PARALLEL	= 1, /* input data from parallel intf */
	MIPI_PIPE_INPUT_CREST		= 2, /* input data from crest module */
	MIPI_PIPE_INPUT_MAX,
};

/*
 * @struct mipi_intf_prop_t
 * The definition of Sensor mipi interface property structure
 */
struct mipi_intf_prop_t {
	uint8 num_lanes;                     /* the lane numbers */
	enum mipi_virtual_channel_t virt_channel; /* the virtual channel number */
	enum mipi_data_type_t data_type;          /* the sensor output data type by package */
	enum mipi_comp_scheme_t comp_scheme;        /* the compress scheme */
	enum mipi_pred_block_t pred_block;          /* the Predictor */
};

/*
 * @enum parallel_data_type_t
 * The definition of Sensor parallel interface data type
 */
enum parallel_data_type_t {
	PARALLEL_DATA_TYPE_INVALID       = 0,/* PARALLEL_DATA_TYPE_INVALID */
	PARALLEL_DATA_TYPE_RAW8          = 1,/* PARALLEL_DATA_TYPE_RAW8 */
	PARALLEL_DATA_TYPE_RAW10         = 2,/* PARALLEL_DATA_TYPE_RAW10 */
	PARALLEL_DATA_TYPE_RAW12         = 3,/* PARALLEL_DATA_TYPE_RAW12 */
	PARALLEL_DATA_TYPE_YUV420_8BIT   = 4,/* PARALLEL_DATA_TYPE_YUV420_8BIT */
	PARALLEL_DATA_TYPE_YUV420_10BIT  = 5,/* PARALLEL_DATA_TYPE_YUV420_10BIT */
	PARALLEL_DATA_TYPE_YUV422_8BIT   = 6,/* PARALLEL_DATA_TYPE_YUV422_8BIT */
	PARALLEL_DATA_TYPE_YUV422_10BIT  = 7,/* PARALLEL_DATA_TYPE_YUV422_10BIT */
	PARALLEL_DATA_TYPE_MAX               /* PARALLEL_DATA_TYPE_MAX */
};

/*
 * @enum polarity_t
 * The definition of different POLARITY type:high or low
 */
enum polarity_t {
	POLARITY_INVALID      = 0,/* POLARITY_INVALID */
	POLARITY_HIGH         = 1,/* POLARITY_HIGH */
	POLARITY_LOW          = 2,/* POLARITY_LOW */
	POLARITY_MAX              /* POLARITY_MAX */
};

/*
 * @enum sample_edge_t
 * The definition of different valid edge type:negative or positive
 */
enum sample_edge_t {
	SAMPLE_EDGE_INVALID       = 0,/* SAMPLE_EDGE_INVALID */
	SAMPLE_EDGE_NEG           = 1,/* SAMPLE_EDGE_NEG */
	SAMPLE_EDGE_POS           = 2,/* SAMPLE_EDGE_POS */
	SAMPLE_EDGE_MAX               /* SAMPLE_EDGE_MAX */
};

/*
 * @struct parallel_intf_prop_t
 * The definition of Sensor parallel interface prop structure
 */
struct parallel_intf_prop_t {
	enum parallel_data_type_t
	data_type;      /* data_type please refer to enum parallel_data_type_t */
	enum polarity_t h_pol;                  /* h_pol please refer to enum polarity_t */
	enum polarity_t v_pol;                  /* v_pol please refer to enum polarity_t */
	enum sample_edge_t edge;                /* edge please refer to enum sample_edge_t */
};

/*
 * @struct sensor_emb_prop_t
 * Sensor emb prop
 */
struct sensor_emb_prop_t {
	enum mipi_virtual_channel_t  virt_channel;       /* Virtual channel */
	enum mipi_data_type_t        data_type;          /* Mipi data type */
	struct window_t              emb_data_window;          /* Emb data window */
	uint32                expo_start_byte_offset;    /* exposure start pos */
	uint32                expo_needed_bytes;        /* exposure needed bytes */
};

/*
 * @struct mipi_form_pd_data_config_t
 * Sensor PD data property
 */
struct mipi_form_pd_data_config_t {
	enum mipi_virtual_channel_t  virt_channel;    /* vitural channel */
	enum mipi_data_type_t        data_type;       /* datatype */
	struct window_t              pd_data_window;   /* pd data window */

};

/*
 * @struct sensor_pd_prop_t
 * Sensor PD property
 */
struct sensor_pd_prop_t {
	struct mipi_form_pd_data_config_t    pd_data_config;   /* PD data config */
};

/*
 * @struct mipi_pipe_path_cfg_t
 * Mipi pipe cfg info
 */
struct mipi_pipe_path_cfg_t {
	bool_t
	b_enable;    /* If disabled, the RAW image only can be from host */
	enum sensor_id          sensor_id;   /* Sensor Id */
};

/*
 * @enum isp_pipe_out_ch_t
 * The output channel type
 */
enum isp_pipe_out_ch_t {
	ISP_PIPE_OUT_CH_PREVIEW = 0,        /* Preview */
	ISP_PIPE_OUT_CH_VIDEO,              /* Video */
	ISP_PIPE_OUT_CH_STILL,              /* Still */
	ISP_PIPE_OUT_CH_RAW,                /* Raw */
	ISP_PIPE_OUT_CH_MIPI_RAW,           /* Mipi Raw */
	ISP_PIPE_OUT_CH_MIPI_HDR_RAW,       /* Mipi Raw for DoLHDR short exposure */
	ISP_PIPE_OUT_CH_MIPI_TMP,           /* Mipi Raw tmp */
	ISP_PIPE_OUT_CH_MIPI_HDR_RAW_TMP,   /* Mipi HDR Shor Exposure Raw */
	ISP_PIPE_OUT_CH_CSTAT_DS_PREVIEW,   /* Cstat downscaler */
	ISP_PIPE_OUT_CH_LME_MV0,
	ISP_PIPE_OUT_CH_LME_MV1,
	ISP_PIPE_OUT_CH_LME_WDMA,
	ISP_PIPE_OUT_CH_LME_SAD,
	ISP_PIPE_OUT_CH_BYRP_TAPOUT,        /* Byrp tapout */
	ISP_PIPE_OUT_CH_RGBP_TAPOUT,        /* Rgbp tapout */
	ISP_PIPE_OUT_CH_MCFP_TAPOUT,        /* Mcfp tapout */
	ISP_PIPE_OUT_CH_YUVP_TAPOUT,        /* Yuvp tapout */
	ISP_PIPE_OUT_CH_MCSC_TAPOUT,        /* Mcsc tapout */
	ISP_PIPE_OUT_CH_CSTAT_CDS,          /* Cstat CDS */
	ISP_PIPE_OUT_CH_CSTAT_FDPIG,        /* Cstat FDPIG */
	ISP_PIPE_OUT_CH_MAX,                /* Max */
};

/*
 * @enum isp_pipe_in_ch_t
 * The input channel type
 */
enum isp_pipe_in_ch_t {
	ISP_PIPE_IN_CH_BYRP_RDMA0,
	ISP_PIPE_IN_CH_BYRP_RDMA1,
	ISP_PIPE_IN_CH_BYRP_HDR_RDMA,
	ISP_PIPE_IN_CH_LME_RDMA,
	ISP_PIPE_IN_CH_LME_PREV_RDMA,
	ISP_PIPE_IN_CH_YUVP_INPUT_SEG,
	ISP_PIPE_IN_CH_MAX,                /* Max */
};

/*
 * @struct isp_pipe_path_cfg_t
 * Isp pipe path cfg info
 * A combination value from enum isp_pipe_id
 */
struct isp_pipe_path_cfg_t {
	uint32  isp_pipe_id;                  /* pipe ids for pipeline construction */
};

/*
 * @struct StreamPathCfg_t
 * Stream path cfg info
 */
struct stream_cfg_t {
	struct mipi_pipe_path_cfg_t mipi_pipe_path_cfg;  /* Isp mipi path */
	struct isp_pipe_path_cfg_t  isp_pipe_path_cfg;   /* Isp pipe path */
	bool_t            b_enable_tnr;       /* enable TNR */
	uint32 rta_frames_per_proc;	/* number of frame rta per-processing, */
					/* set to 0 to use fw default value */
};

enum isp_yuv_range_t {
	ISP_YUV_RANGE_FULL = 0,     /* YUV value range in 0~255 */
	ISP_YUV_RANGE_NARROW = 1,   /* YUV value range in 16~235 */
	ISP_YUV_RANGE_MAX
};

/*
 * @struct image_prop_t
 *Image property
 */
struct image_prop_t {
	enum image_format_t image_format;  /* Image format */
	uint32 width;               /* Width */
	uint32 height;              /* Height */
	uint32 luma_pitch;           /* Luma pitch */
	uint32 chroma_pitch;         /* Chrom pitch */
	enum isp_yuv_range_t yuv_range;     /* YUV value range */
};

/*
 * @page RawPktFmt
 * @verbatim
 * Suppose the image pixel is in the sequence of:
 *             A B C D E F
 *             G H I J K L
 *             M N O P Q R
 *             ...
 * The following RawPktFmt define the raw picture output format.
 * For each format, different raw pixel width will have different memory
 * filling format. The raw pixel width is set by the SesorProp_t.
 *
 * RAW_PKT_FMT_0:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT   |  0   0   0   0   0   0   0   0   A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT  |  A1  A0  0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2
 *    12-BIT  |  A3  A2  A1  A0  0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4
 *    14-BIT  |  A5  A4  A3  A2  A1  A0  0   0   A13 A12 A11 A10 A9  A8  A7  A6
 *
 * RAW_PKT_FMT_1:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT      B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     A1  A0  0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2
 *    12-BIT     A3  A2  A1  A0  0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4
 *    14-BIT     A5  A4  A3  A2  A1  A0  0   0   A13 A12 A11 A10 A9  A8  A7  A6
 *
 * RAW_PKT_FMT_2:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT      0   0   0   0   0   0   0   0   A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    12-BIT     0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    14-BIT     0   0   A13 A12 A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *
 * RAW_PKT_FMT_3:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT      B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    12-BIT     0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    14-BIT     0   0   A13 A12 A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *
 * RAW_PKT_FMT_4:
 *    (1) 8-BIT:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *               B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    (2) 10-BIT:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *               B5  B4  B3  B2  B1  B0  A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *               D1  D0  C9  C8  C7  C6  C5  C4  C3  C2  C1  C0  B9  B8  B7  B6
 *               E7  E6  E5  E4  E3  E2  E1  E0  D9  D8  D7  D6  D5  D4  D3  D2
 *               G3  G2  G1  G0  F9  F8  F7  F6  F5  F4  F3  F2  F1  F0  E9  E8
 *               H9  H8  H7  H6  H5  H4  H3  H2  H1  H0  G9  G8  G7  G6  G5  G4
 *    (3) 12-BIT:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *               B3  B2  B1  B0  A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *               C7  C6  C5  C4  C3  C2  C1  C0  B11 B10 B9  B8  B7  B6  B5  B4
 *               D11 D10 D9  D8  D7  D6  D5  D4  D3  D2  D1  D0  C11 C10 C9  C8
 *    (4) 14-BIT:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *               B1  B0  A13 A12 A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *               C3  C2  C1  C0  B13 B12 B11 B10 B9  B8  B7  B6  B5  B4  B3  B2
 *               D5  D4  D3  D2  D1  D0  C13 C12 C11 C10 C9  C8  C7  C6  C5  C4
 *               E7  E6  E5  E4  E3  E2  E1  E0  D13 D12 D11 D10 D9  D8  D7  D6
 *               F9  F8  F7  F6  F5  F4  F3  F2  F1  F0  E13 E12 E11 E10 E9  E8
 *               G11 G10 G9  G8  G7  G6  G5  G4  G3  G2  G1  G0  F13 F12 F11 F10
 *               H13 H12 H11 H10 H9  H8  H7  H6  H5  H4  H3  H2  H1  H0  G13 G12
 *
 * RAW_PKT_FMT_5:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT      B7  B6  B5  B4  B3  B2  B1  B0  A7  A6  A5  A4  A3  A2  A1  A0
 *    10-BIT     0   0   0   0   0   0   A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *    12-BIT     0   0   0   0   A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0
 *
 * RAW_PKT_FMT_6:
 *    --------+----------------------------------------------------------------
 *    Bit-Pos |  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *    --------+----------------------------------------------------------------
 *    8-BIT      A7  A6  A5  A4  A3  A2  A1  A0  B7  B6  B5  B4  B3  B2  B1  B0
 *    10-BIT     A9  A8  A7  A6  A5  A4  A3  A2  A1  A0  0   0   0   0   0   0
 *    12-BIT     A11 A10 A9  A8  A7  A6  A5  A4  A3  A2  A1  A0  0   0   0   0
 * @endverbatim
 */

/*
 * @enum raw_pkt_fmt_t
 * Raw package format
 */
enum raw_pkt_fmt_t {
	RAW_PKT_FMT_0,  /* Default(ISP1P1 legacy format) */
	RAW_PKT_FMT_1,  /* ISP1P1 legacy format and bubble-free for 8-bit raw pixel */
	RAW_PKT_FMT_2,  /* Android RAW16 format */
	RAW_PKT_FMT_3,  /* Android RAW16 format and bubble-free for 8-bit raw pixel */
	RAW_PKT_FMT_4,  /* ISP2.0 bubble-free format */
	RAW_PKT_FMT_5,  /* RGB-IR format for GPU process */
	RAW_PKT_FMT_6,  /* RGB-IR format for GPU process with data swapped */
	RAW_PKT_FMT_MAX /* Max */
};

/*
 * @enum buffer_type_t
 * Buffer type
 */
enum buffer_type_t {
	BUFFER_TYPE_INVALID,             /* Enum value Invalid */

	BUFFER_TYPE_RAW,                 /* Enum value BUFFER_TYPE_RAW_ZSL, */
	BUFFER_TYPE_MIPI_RAW,            /* Enum value BUFFER_TYPE_MIPI_RAW, */
	BUFFER_TYPE_RAW_TEMP,            /* Enum value BUFFER_TYPE_RAW_TEMP */
	BUFFER_TYPE_MIPI_RAW_SHORT_EXPO, /* Enum value BUFFER_TYPE_MIPI_RAW_SHORT_EXPO(DoLHDR) */
	BUFFER_TYPE_EMB_DATA,            /* Enum value BUFFER_TYPE_EMB_DATA */
	BUFFER_TYPE_PD_DATA,             /* Enum value PD for stg1 or stg2 */

	BUFFER_TYPE_STILL,               /* Enum value BUFFER_TYPE_STILL */
	BUFFER_TYPE_PREVIEW,             /* Enum value BUFFER_TYPE_PREVIEW */
	BUFFER_TYPE_VIDEO,               /* Enum value BUFFER_TYPE_VIDEO */

	BUFFER_TYPE_META_INFO,           /* Enum value BUFFER_TYPE_META_INFO */
	BUFFER_TYPE_FRAME_INFO,          /* Enum value BUFFER_TYPE_FRAME_INFO */

	BUFFER_TYPE_TNR_REF,             /* Enum value BUFFER_TYPE_TNR_REF */
	BUFFER_TYPE_META_DATA,           /* Enum value BUFFER_TYPE_META_DATA */
	BUFFER_TYPE_SETFILE_DATA,        /* Enum value BUFFER_TYPE_SETFILE_DATA */
	BUFFER_TYPE_MEM_POOL,            /* Enum value BUFFER_TYPE_MEM_POOL */
	BUFFER_TYPE_CSTAT_DS,            /* Enum value BUFFER_TYPE_CSTAT_DS */

	/* Lme buffer types for DIAG loopback test */
	BUFFER_TYPE_LME_RDMA,
	BUFFER_TYPE_LME_PREV_RDMA,
	BUFFER_TYPE_LME_WDMA,
	BUFFER_TYPE_LME_MV0,
	BUFFER_TYPE_LME_MV1,
	BUFFER_TYPE_LME_SAD,

	BUFFER_TYPE_BYRP_TAPOUT,         /* Enum value BUFFER_TYPE_BYRP_TAPOUT */
	BUFFER_TYPE_RGBP_TAPOUT,         /* Enum value BUFFER_TYPE_RGBP_TAPOUT */
	BUFFER_TYPE_MCFP_TAPOUT,         /* Enum value BUFFER_TYPE_MCFP_TAPOUT */
	BUFFER_TYPE_YUVP_TAPOUT,         /* Enum value BUFFER_TYPE_YUVP_TAPOUT */
	BUFFER_TYPE_MCSC_TAPOUT,         /* Enum value BUFFER_TYPE_MCSC_TAPOUT */
	BUFFER_TYPE_CSTAT_CDS,           /* Enum value BUFFER_TYPE_CSTAT_CDS */
	BUFFER_TYPE_CSTAT_FDPIG,         /* Enum value BUFFER_TYPE_CSTAT_FDPIG */

	BUFFER_TYPE_YUVP_INPUT_SEG,      /* Enum value BUFFER_TYPE_YUVP_INPUT_SEG */
	BUFFER_TYPE_CTL_META_DATA,       /* Enum value BUFFER_TYPE_CTL_META_DATA */
	BUFFER_TYPE_EMUL_DATA,           /* Enum value BUFFER_TYPE_EMUL_DATA */
	BUFFER_TYPE_CSTAT_DRC,           /* Enum value BUFFER_TYPE_CSTAT_DRC */
	BUFFER_TYPE_MAX                  /* Enum value  Max */
};

/*
 * @enum addr_space_type_t
 * Address space type
 */
enum addr_space_type_t {
	ADDR_SPACE_TYPE_GUEST_VA        = 0,    /* Enum value ADDR_SPACE_TYPE_GUEST_VA */
	ADDR_SPACE_TYPE_GUEST_PA        = 1,    /* Enum value ADDR_SPACE_TYPE_GUEST_PA */
	ADDR_SPACE_TYPE_SYSTEM_PA       = 2,    /* Enum value ADDR_SPACE_TYPE_SYSTEM_PA */
	ADDR_SPACE_TYPE_FRAME_BUFFER_PA = 3,    /* Enum value ADDR_SPACE_TYPE_FRAME_BUFFER_PA */
	ADDR_SPACE_TYPE_GPU_VA          = 4,    /* Enum value ADDR_SPACE_TYPE_GPU_VA */
	ADDR_SPACE_TYPE_MAX             = 5     /* Enum value max */
};

/*
 * @struct buffer_t
 * The definition of struct buffer_t
 */
struct buffer_t {
	/* A check num for debug usage, host need to */
	/* set the buf_tags to different number */
	uint32 buf_tags;
	union {
		uint32 value;       /* The member of vmid_space union; Vmid[31:16], Space[15:0] */
		struct {
		uint32 space : 16;  /* The member of vmid_space union; Vmid[31:16], Space[15:0] */
		uint32 vmid  : 16;  /* The member of vmid_space union; Vmid[31:16], Space[15:0] */
		} bit;
	} vmid_space;
	uint32 buf_base_a_lo;          /* Low address of buffer A */
	uint32 buf_base_a_hi;          /* High address of buffer A */
	uint32 buf_size_a;            /* Buffer size of buffer A */

	uint32 buf_base_b_lo;          /* Low address of buffer B */
	uint32 buf_base_b_hi;          /* High address of buffer B */
	uint32 buf_size_b;            /* Buffer size of buffer B */

	uint32 buf_base_c_lo;          /* Low address of buffer C */
	uint32 buf_base_c_hi;          /* High address of buffer C */
	uint32 buf_size_c;            /* Buffer size of buffer C */
};

/*
 * @struct output_buf_t
 * Output buffer
 */
struct output_buf_t {
	bool_t      enabled;   /* enabled */
	struct buffer_t    buffer;   /* buffer */
	struct image_prop_t image_prop;   /* image_prop */
};

/*
 * @enum irillu_status_t enum irillu_status_t
 */
/*
 * @enum  _irillu_status_t
 * @brief  The status of IR illuminator
 */
enum irillu_status_t {
	IR_ILLU_STATUS_UNKNOWN, /* enum value: IR_ILLU_STATUS_UNKNOWN */
	IR_ILLU_STATUS_ON, /* enum value: IR_ILLU_STATUS_ON */
	IR_ILLU_STATUS_OFF, /* enum value: IR_ILLU_STATUS_OFF */
	IR_ILLU_STATUS_MAX, /* enum value: IR_ILLU_STATUS_MAX */
};

/*
 * @struct irmeta_info_t struct irmeta_info_t
 */
/*
 * @struct irmeta_info_t
 * @brief  The IR MetaInfo for IR illuminator status
 */
struct irmeta_info_t {
	enum irillu_status_t ir_illu_status; /* IR illuminator status */
};

/* AAA */
/* ------- */
#define MAX_REGIONS               16

/* ISP firmware supported AE ROI region num */
#define MAX_AE_ROI_REGION_NUM     1

/* ISP firmware supported AWB ROI region num */
#define MAX_AWB_ROI_REGION_NUM    0

/* ISP firmware supported AF ROI region num */
#define MAX_AF_ROI_REGION_NUM     0

/*
 * _roi_type_mask_t
 */
enum roi_type_mask_t {
	ROI_TYPE_MASK_AE = 0x1, /* AE ROI */
	ROI_TYPE_MASK_AWB = 0x2,/* AWB ROI */
	ROI_TYPE_MASK_AF = 0x4, /* AF ROI */
	ROI_TYPE_MASK_MAX
};

/*
 * @enum roi_mode_mask_t
 * ROI modes
 */
enum roi_mode_mask_t {
	ROI_MODE_MASK_TOUCH = 0x1, /* Using touch ROI */
	ROI_MODE_MASK_FACE = 0x2   /* Using face ROI */
};

/*
 * @brief   Defines and area using the top left and bottom right corners
 */
struct isp_area_t {
	struct point_t top_left;       /* !< top left corner */
	struct point_t bottom_right;   /* !< bottom right corner */
};

/*
 * @brief Defines the touch area with weight
 */
struct isp_touch_area_t {
	struct isp_area_t points; /* Touch region's top left and bottom right points */
	uint32    touch_weight; /* touch area's weight */
};

/*
 * @brief Face detection land marks
 */
struct isp_fd_landmarks_t {
	struct point_t eye_left;
	struct point_t eye_right;
	struct point_t nose;
	struct point_t mouse_left;
	struct point_t mouse_right;
};

/*
 * @brief Face detection all face info
 */
struct isp_fd_face_info_t {
	uint32 face_id;     /* The ID of this face */
	uint32 score;      /* The score of this face, larger than 0 for valid face */
	struct isp_area_t face_area;   /* The face region info */
	struct isp_fd_landmarks_t marks; /* The face landmarks info */
};

/*
 * @brief Face detection info
 */
struct isp_fd_info_t {
	uint32 is_enabled;                   /* Set to 0 to disable this face detection info */
	uint32 frame_count;                  /* Frame count of this face detection info from */
	uint32 is_marks_enabled;              /* Set to 0 to disable the five marks on the faces */
	uint32 face_num;                     /* Number of faces */
	struct isp_fd_face_info_t face[MAX_REGIONS];  /* Face detection info */
};

/*
 * @brief Touch ROI info
 */
struct isp_touch_info_t {
	uint32          touch_num;                   /* Touch region numbers */
	struct isp_touch_area_t  touch_area[MAX_REGIONS];/* Touch regions */
};

/*
 * @enum buffer_status_t enum buffer_status_t
 */
/*
 * @enum  _buffer_status_t
 * @brief  The enumeration about BufferStatus
 */
enum buffer_status_t {
	BUFFER_STATUS_INVALID,  /* The buffer is INVALID */
	BUFFER_STATUS_SKIPPED,  /* The buffer is not filled with image data */
	BUFFER_STATUS_EXIST,    /* The buffer is exist and waiting for filled */
	BUFFER_STATUS_DONE,     /* The buffer is filled with image data */
	BUFFER_STATUS_LACK,     /* The buffer is unavailable */
	BUFFER_STATUS_DIRTY,    /* The buffer is dirty, probably caused by LMI leakage */
	BUFFER_STATUS_MAX       /* The buffer STATUS_MAX */
};

/*
 * @enum buffer_source_t enum buffer_source_t
 */
/*
 * @enum  _buffer_source_t
 * @brief  The enumeration about BufferStatus
 */
enum buffer_source_t {
	BUFFER_SOURCE_INVALID, /* BUFFER_SOURCE_INVALID */
	BUFFER_SOURCE_CMD_CAPTURE,  /* The buffer is from a capture command */
	BUFFER_SOURCE_STREAM, /* The buffer is from the stream buffer queue */
	BUFFER_SOURCE_TEMP, /* BUFFER_SOURCE_TEMP */
	BUFFER_SOURCE_MAX /* BUFFER_SOURCE_MAX */
};

/*
 * @struct mipi_crc struct mipi_crc
 */
/*
 * @struct mipi_crc
 * @brief  The Meta info crc
 */
struct mipi_crc {
	uint32 crc[8]; /* crc */
};

/*
 * @struct ch_crop_win_based_on_acq_t
 * @brief  The ch_crop_win_based_on_acq_t
 */
struct ch_crop_win_based_on_acq_t {
	struct window_t window;/* based on Acq window */
};

/*
 * @struct buffer_meta_info_t struct buffer_meta_info_t
 */
/*
 * @struct buffer_meta_info_t
 * @brief  The Meta info crc
 */
struct buffer_meta_info_t {
	bool_t              enabled; /* enabled flag */
	enum buffer_status_t      status; /* BufferStatus */
	struct error_code         err; /* err code */
	enum buffer_source_t      source; /* BufferSource */
	struct image_prop_t         image_prop; /* image_prop */
	struct buffer_t            buffer; /* buffer */
	struct mipi_crc             wdma_crc; /* wdma_crc */
	struct ch_crop_win_based_on_acq_t crop_win_acq; /* crop_win_acq */
};

struct byrp_crc {
	uint32 rdma_crc; /* rdma input crc */
	uint32 wdma_crc; /* wdma output crc */
};

struct mcsc_crc {
	/* wdma 1P/2P crc for
	 * output0 - Preview
	 * output1 - video
	 * output2 - still
	 */
	uint32 wdma1_pcrc[MAX_OUTPUT_MCSC];
	uint32 wdma2_pcrc[MAX_OUTPUT_MCSC];
};

struct gdc_crc {
	uint32 rdma_ycrc;    /* rdma crc of input Y plane. */
	uint32 rdma_uv_crc;   /* rdma crc of input UV plane. */
	uint32 wdma1_pcrc;   /* wdma crc of output Y plane. */
	uint32 wdma2_pcrc;   /* wdma crc of output UV plane. */
};

struct lme_crc {
	/* only WDMA related RTL logic found, for RDMA only SEED is configured. */
	uint32 sps_mv_out_crc; /* wdma sub pixel search motion vector crc */
	uint32 sad_out_crc;   /* wdma sad crc */
	uint32 mbmv_out_crc;  /* wdma mbmv crc */

};

struct rgbp_crc {
	uint32 rdma_rep_rgb_even_crc;   /* rdma input crc */
	uint32 wdma_ycrc;            /* wdma y plane crc */
	uint32 wdma_uv_crc;           /* wdma UV plane crc */
};

struct yuvp_crc {
	uint32 rdma_ycrc;    /* rdma crc of input Y plane. */
	uint32 rdma_uv_crc;   /* rdma crc of input UV plane. */
	uint32 rdma_seg_crc;  /* rdma crc of segmentation. */
	uint32 rdma_drc_crc;  /* rdma crc of Drc */
	uint32 rdma_drc1_crc; /* rdma crc of Drc1 */
	uint32 wdma_ycrc;    /* wdma crc of output Y plane. */
	uint32 wdma_uv_crc;   /* wdma crc of output UV plane. */
};

struct mcfp_crc {
	uint32 rdma_curr_ycrc; /* rdma crc of curr input Y plane. */
	uint32 rdma_curr_uv_crc;/* rdma crc of curr input UV plane. */
	uint32 rdma_prev_ycrc; /* rdma crc of prev input Y plane. */
	uint32 rdma_prev_uv_crc;/* rdma crc of prev input UV plane. */
	uint32 wdma_curr_ycrc; /* wdma crc of curr output Y plane. */
	uint32 wdma_curr_uv_crc;/* wdma crc of curr output Uv plane. */
	uint32 wdma_prev_ycrc; /* wdma crc of prev output Y plane. */
	uint32 wdma_prev_uv_crc;/* wdma crc of prev output UV plane. */
};

struct cstat_crc {
	uint32 rdma_byr_in_crc;    /* rdma crc of input bayer. */
	uint32 wdma_rgb_hist_crc;  /* wdma crc of rgb histogram */
	uint32 wdma_thstat_pre;   /* wdma crc of TH stat Pre */
	uint32 wdma_thstat_awb;   /* wdma crc of TH stat Awb */
	uint32 wdma_thstat_ae;    /* wdma crc of TH stat Ae */
	uint32 wdma_drc_grid;     /* wdma crc of Drc grid */
	uint32 wdma_lme_ds0;      /* wdma crc of lme down scaler0 */
	uint32 wdma_lme_ds1;      /* wdma crc of lme down scaler1 */
	uint32 wdma_fdpig;       /* wdma crc of FD pre img generator */
	uint32 wdma_cds0;        /* wdma crc of scene detect scaler */
};

struct pdp_crc {
	uint32 rdma_afcrc;       /* rdma crc of AF */
	uint32 wdma_stat_crc;     /* wdma crc of stat */
};

/*
 * @struct usr_ctrlmeta_info_t struct usr_ctrlmeta_info_t
 */
struct usr_ctrlmeta_info_t {
	uint32 brightness;      /* The brightness value. */
	uint32 contrast;        /* The contrast value */
	uint32 saturation;      /* The saturation value */
	uint32 hue;             /* The hue value */
};

/*
 * @struct guid_t
 * The Definition of SecureBIO secure buffer GUID structure
 */
struct secure_buf_guid_t {
	uint32 guid_data1;        /* GUID1 */
	uint16 guid_data2;        /* GUID2 */
	uint16 guid_data3;        /* GUID3 */
	uint8  guid_data4[8];     /* GUID4 */
};

/*
 * @struct meta_info_secure_t
 * @brief  The meta_info_secure_t
 */

struct meta_info_secure_t {
	bool_t                   b_is_secure; /* is secure frame */
	struct secure_buf_guid_t guid; /* guid of the frame */
};

/*
 * @struct meta_info_t
 * @brief  The MetaInfo
 */
struct meta_info_t {
	uint32                    poc;            /* frame id */
	uint32                    fc_id;          /* frame ctl id */
	uint32                    time_stamp_lo;  /* time_stamp_lo */
	uint32                    time_stamp_hi;  /* time_stamp_hi */
	struct buffer_meta_info_t preview;        /* preview BufferMetaInfo */
	struct buffer_meta_info_t video;          /* video BufferMetaInfo */
	struct buffer_meta_info_t still;	  /* yuv zsl BufferMetaInfo */
	struct buffer_meta_info_t full_still;	  /* full_still zsl BufferMetaInfo; */
	struct buffer_meta_info_t raw;            /* x86 raw */
	struct buffer_meta_info_t raw_mipi;       /* raw mipi */
	struct buffer_meta_info_t raw_mipi_short_expo; /* DolHDR short exposure raw mipi */
	struct buffer_meta_info_t metadata;       /* Host Camera Metadata */
	struct buffer_meta_info_t lme_mv0;        /* Lme Mv0 */
	struct buffer_meta_info_t lme_mv1;        /* Lme Mv1 */
	struct buffer_meta_info_t lme_wdma;       /* Lme Wdma */
	struct buffer_meta_info_t lme_sad;        /* Lme Sad */
	struct buffer_meta_info_t cstatds;        /* Cstat Downscaler */
	enum raw_pkt_fmt_t raw_pkt_fmt;      /* The raw buffer packet format if the raw is exist */
	struct byrp_crc    byrp_crc;        /* byrp crc */
	struct mcsc_crc    mcsc_crc;        /* mcsc crc */
	struct gdc_crc     gdc_crc;         /* gdc crc */
	struct lme_crc     lme_crc;         /* lme crc */
	struct rgbp_crc    rgbp_crc;        /* rgbp crc */
	struct yuvp_crc    yuvp_crc;        /* yuvp crc */
	struct mcfp_crc    mcfp_crc;        /* mcfp crc */
	struct cstat_crc   cstat_crc;       /* cstat crc */
	struct pdp_crc     pdp_crc;         /* pdp crc */
	struct mipi_crc    mipi_crc;        /* mipi crc */
	bool_t is_still_cfm;	/* is_still_cfm, flag to indicate */
				/* if the image in preview buffer is still confirmation image, */
				/* The value is only valid for response of capture still */
	struct irmeta_info_t     i_rmeta;         /* i_rmetadata */
	struct usr_ctrlmeta_info_t ctrls;          /* user ctrls */
	struct buffer_meta_info_t byrp_tap_out;     /* Byrp tapout BufferMetaInfo */
	struct buffer_meta_info_t rgbp_tap_out;     /* Rgbp tapout BufferMetaInfo */
	struct buffer_meta_info_t mcfp_tap_out;     /* mcfp tapout BufferMetaInfo */
	struct buffer_meta_info_t yuvp_tap_out;     /* yuvp tapout BufferMetaInfo */
	struct buffer_meta_info_t yuvp_tap_in_seg_conf; /* yuvp tapin SingleBufferMetaInfo */
	struct buffer_meta_info_t mcsc_tap_out;     /* mcsc tapout BufferMetaInfo */
	struct buffer_meta_info_t cds;            /* Cstat cds BufferMetaInfo */
	struct buffer_meta_info_t fdpig;          /* Cstat fdpig BufferMetaInfo */
	struct meta_info_secure_t secure_meta; /* secure meta */
};

#endif
