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
#ifndef SENSOR_PROPERTIES_PUB_H
#define SENSOR_PROPERTIES_PUB_H

/* The Sensor property parameter version number keep in sync with the host driver */
#define SENSORPROP_VER  3

/*
 * @brief Possible Sensor Aperture Options
 *
 * @details This enum lists the possible apertures might be used.
 * But the actual supported aperture depends on the sensor.
 */
enum sensor_aperture_t {
	F1_0    = 100U,
	F1_4    = 140U,
	F1_5    = 150U,
	F1_7    = 170U,
	F1_8    = 180U,
	F1_9    = 190U,
	F2_0    = 200U,
	F2_1    = 210U,
	F2_2    = 220U,
	F2_4    = 240U,
	F2_45   = 245U,
	F2_6    = 260U,
	F2_7    = 270U,
	F2_8    = 280U,
	F4_0    = 400U,
	F5_6    = 560U,
	F8_0    = 800U,
	F11_0   = 1100U,
	F16_0   = 1600U,
	F22_0   = 2200U,
	F32_0   = 3200U,
	APERTURE_MAX,
};

/*
 * @brief Extended sensor mode
 *
 */
enum sensor_mode_ext_t {
	EX_NONE = 0U,       /* No extend mode */
	EX_3DHDR,           /* 3D HDR control mode */
	EX_AEB,             /* Automatic Exposure Bracketing (AEB) mode */
	EX_12BIT,           /* 12 bits input bitwidth mode */
	EX_MODE_MAX
};

/*
 * @brief Sensor position index enum
 *
 * @details There may have multiple sensors in the rear or front side.
 *          Enumerate all the positions for sensors.
 *
 * @todo Resolve conflicts of enum SensorPosition in IspCommonDefinitions.hpp
 * which contains similar meaning.
 */
enum sensor_position_t {
	ISP_SENSOR_POSITION_REAR_1 = 0U,
	ISP_SENSOR_POSITION_FRONT_1,
	ISP_SENSOR_POSITION_REAR_2,
	ISP_SENSOR_POSITION_FRONT_2,
	ISP_SENSOR_POSITION_REAR_3,
	ISP_SENSOR_POSITION_FRONT_3,
	ISP_SENSOR_POSITION_REAR_4,
	ISP_SENSOR_POSITION_FRONT_4,
	ISP_SENSOR_POSITION_INDEX_MAX
};

/*
 * @brief Exposure gain count
 *
 * @details Three set of exposure time and again/dgain
 *          values could be stored in the order of long, short, middle.
 *          The exposure gain count number is used to select number of exposure data
 */
enum exposure_gain_count_t {
	ISP_EXPOSURE_GAIN_COUNT_INVALID = 0,
	ISP_EXPOSURE_GAIN_COUNT_1,      /* Using long exposure */
	ISP_EXPOSURE_GAIN_COUNT_2,      /* Using long and short exposure */
	ISP_EXPOSURE_GAIN_COUNT_3,      /* Using long, short and middle exposure */
	ISP_EXPOSURE_GAIN_COUNT_END
};

/*
 * @brief Sensor exposure gain type
 */
enum exposure_gain_type_t {
	ISP_EXPOSURE_GAIN_LONG = 0,
	ISP_EXPOSURE_GAIN_SHORT,
	ISP_EXPOSURE_GAIN_MIDDLE,
	ISP_EXPOSURE_GAIN_MAX
};

/*
 * @brief Sensor HDR mode
 */
enum sensor_hdr_mode_t {
	ISP_SENSOR_HDR_MODE_SINGLE = 0,/* Single exposure mode */
	ISP_SENSOR_HDR_MODE_2HDR,      /* 2 exposure HDR mode */
	ISP_SENSOR_HDR_MODE_3HDR,      /* 3 exposure HDR mode */
	ISP_SENSOR_HDR_MODE_2AEB,      /* 2 AEB(Automatic Exposure Bracketing) exposure HDR mode */
	ISP_SENSOR_HDR_MODE_2STHDR,    /* 2 exposure staggered HDR mode */
};

/*
 * @enum sensor_ae_prop_type_t
 * Sensor prop type for Ae
 */
enum sensor_ae_prop_type_t {
	SENSOR_AE_PROP_TYPE_INVALID =  0,   /* Invalid */
	SENSOR_AE_PROP_TYPE_SONY    =  1,   /* Analog gain formula: */
	/* gain = weight1 / (wiehgt2 - param) */
	SENSOR_AE_PROP_TYPE_OV      =  2,   /* Analog gain formula: */
	/* gain = (param / weight1) << shift */
	SENSOR_AE_PROP_TYPE_SCRIPT  =  3,   /* AE use script to adjust expo/gain settings */
	SENSOR_AE_PROP_TYPE_MAX             /* Max */
};

/*
 * @struct sensor_ae_gain_formula_t
 * Gain formula for Ae
 */
struct sensor_ae_gain_formula_t {
	uint32 weight1;     /* constant a */
	uint32 weight2;     /* constant b */
	uint32 min_shift;    /* minimum S */
	uint32 max_shift;    /* maximum S */
	uint32 min_param;    /* minimum X */
	uint32 max_param;    /* maximum X */
};

/*
 * @struct sensor_ae_prop_t
 * Sensor Ae prop
 */
struct sensor_ae_prop_t {
	/* Sensor property related */
	enum sensor_ae_prop_type_t
	type;        /* Sensor property for Analog gain calculation */
	uint32 min_expo_line;             /* minimum exposure line */
	uint32 max_expo_line;             /* maximum exposure line */
	uint32 expo_line_alpha;           /* exposure line alpha for correct frame rate */
	uint32 min_analog_gain;           /* minimum analog gain, 1000-based fixed point */
	uint32 max_analog_gain;           /* maximum analog gain, 1000-based fixed point */
	uint32 min_digital_gain;          /* Minimum digital gain times x1000 */
	uint32 max_digital_gain;          /* Maximum digital gain times x1000 */
	bool_t shared_again;             /* HDR LE/SE share same analog gain */
	bool_t use_dgain;                /* Sensor is using digital gain or not */
	struct	sensor_ae_gain_formula_t formula;  /* formula for Ae gain */

	/* Sensor profile related */
	uint32 time_of_line;              /* time of line in nanosecond precise */
	uint32 frame_length_lines;        /* frame length as exposure line per sensor profile */
	uint32 line_length_pixels;        /* Line length in number of pixel clock ticks */
	uint32 expo_offset;              /* extra exposure time in nanosecond */
	/* when calculating time of line. */
	/* TOL * line + offset = real exposure time. */
	uint64 rollingshutterskew;      /* Rolling shutter skew time in nano seconds */

	/* Sensor calib related */
	uint32 base_iso;                 /* how many ISO is equal to 1.x gain */
	uint32 init_itime[ISP_EXPOSURE_GAIN_MAX];	/* Initial integration time, */
	/* 1000-based fixed point */
	uint32 init_analog_gain[ISP_EXPOSURE_GAIN_MAX];	/* Initial analog gain, */
	/* 1000-based fixed point */
	uint32 init_digital_gain[ISP_EXPOSURE_GAIN_MAX];/* Initial digital gain times x1000 */
};

/*
 * @struct sensor_m2_mcalib_prop_t
 * Sensor M2M calibration prop
 */
struct sensor_m2_mcalib_prop_t {
	uint32 m2m_en;                     /* M2M calibration enable */
	uint32 m2m_calib_width;             /* M2M calibration width */
	uint32 m2m_calib_height;            /* M2M calibration height */
	uint32 cur_sensor_offset_x;          /* x internal offset of the current profile */
	uint32 cur_sensor_offset_y;          /* y internal offset of the current profile */
};

/*
 * Sensor Types that will be supported
 */
enum sensor_type {
	SENSOR_TYPE_STANDARD_RGB = 0,
	SENSOR_TYPE_RGBIR = 1,
	SENSOR_TYPE_IR = 2,
	SENSOR_TYPE_QUAD = 3,
	SENSOR_TYPE_TETRA = 4,
	SENSOR_TYPE_NONA = 5,
	SENSOR_TYPE_TOF = 6,
	SENSOR_TYPE_MAX,
};

/*
 * DOL_HDR mode configured in sensor
 */
enum dol_hdr_mode {
	DOL_INVALID = 0,
	DOL_2_FRAMES = 1,
	DOL_3_FRAMES = 2,
	DOL_MAX,
};

/*
 * @brief The sensor properties
 *
 * @details These parameters are static information after sensor is stream on.
 * This structure keeps the information read from sensor driver.
 * Some of the parameters needs to be calculated to fit 3A algorithms' usage.
 */
struct sensor_prop_t {
	uint32
	version;           /* The sensor property interface version */
	enum sensor_intf_type    intf_type;          /* Intf type */
	union {
		struct mipi_intf_prop_t     mipi;     /* Mipi intf prop */
		struct parallel_intf_prop_t parallel; /* Parallel intf prop */
	} intf_prop;
	enum cfa_pattern_t        cfa_pattern;        /* CFA pattern */
	enum sensor_shutter_type_t sensor_shutter_type; /* Shutter pattern */
	bool_t              has_embedded_data;   /* Has embedded data */
	enum mipi_virtual_channel_t emb_virt_channel; /* Embedded data MIPI virtual channel */
	enum mipi_data_type_t      emb_data_type;     /* Embedded data MIPI data type */

	uint32 emb_win_offset_h;     /* Embedded data window horizontal offset */
	uint32 emb_win_offset_v;     /* Embedded data window vertical offset */
	uint32 emb_win_size_h;       /* Embedded data window horizontal size */
	uint32 emb_win_size_v;       /* Embedded data window vertical size */

	uint32
	emb_expo_start_offset;/* Embedded data exposure start offset in bytes */
	uint32
	emb_expo_bytes;      /* Embedded data exposure needed bytes */
	/* Distinguish pre and post for embedded data */
	uint32              itime_delay_frames;  /* Itime delay frames */
	uint32              gain_delay_frames;   /* Gain delay frames */
	bool_t              is_pdaf_sensor;      /* Is pdaf sensor? */
	enum pd_output_type_t      pd_output_type;      /* Pd output type */
	struct sensor_ae_prop_t ae;                /* Ae prop */

	uint32
	max_frame_rate;     /* Maximum framerate, multiplied by 1000 */
	enum sensor_type    sensor_type;        /* Sensor type */
	uint32              calibrated_width;   /* Sensor calibrated pixel size */
	uint32              calibrated_height;  /* Sensor calibrated pixel size */
	uint32              cur_width;          /* Frame current width */
	uint32              cur_height;         /* Frame current height */
	uint32              cropX;             /* Crop image offset X */
	uint32              cropY;             /* Crop image offset Y */

	bool_t
	hdr_ctrl_by_again;    /* HDR ration controlled by analog gain or not */
	enum sensor_aperture_t
	aperture_num;       /* The initial aperture number of the lens */
	enum sensor_id          sensor_id;          /* Physical sensor ID */
	enum sensor_position_t    sensor_position;    /* Sensor position mapping */
	enum sensor_mode_ext_t     ex_mode;            /* Sensor extended mode */

	bool_t
	wdr_enable;         /* Wide Dynamic Range flag which will affect the HDR exposure flag */
	enum dol_hdr_mode          hdr_mode;           /* HDR mode */
	enum mipi_virtual_channel_t hdr_virt_channel;   /* HDR virtual channel */
	uint32              peri_state;         /* Sensor peripheral available status */
	struct sensor_m2_mcalib_prop_t m2m_prop; /* Sensor m2m calbration property */

	/* The time in microseconds when the sensor outputs useful image data */
	/* auto calculation on zero value(not suggested) */
	uint32              vvalid_time;
};

#endif
