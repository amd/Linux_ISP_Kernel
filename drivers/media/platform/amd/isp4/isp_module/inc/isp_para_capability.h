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

#ifndef _ISP_PROPERTY_DEF_H_
#define _ISP_PROPERTY_DEF_H_

/* camera property value */

/*property for AMP
 *drivers/stream/ksproperty-videoprocamp-brightness
 */
#define BRIGHTNESS_MIN_VALUE		-128
#define BRIGHTNESS_MAX_VALUE		127
#define BRIGHTNESS_STEP_VALUE		1
#define BRIGHTNESS_DEF_VALUE		0
#define BRIGHTNESS_MIN_VALUE_IN_FW	-128
#define BRIGHTNESS_MAX_VALUE_IN_FW	127
#define BRIGHTNESS_DEF_VALUE_IN_FW	0

#define CONTRAST_MIN_VALUE		-128
#define CONTRAST_MAX_VALUE		127
#define CONTRAST_STEP_VALUE		1
#define CONTRAST_DEF_VALUE		0
#define CONTRAST_MIN_VALUE_IN_FW	0
#define CONTRAST_MAX_VALUE_IN_FW	255
#define CONTRAST_DEF_VALUE_IN_FW	128

#define HUE_MIN_VALUE			-128
#define HUE_MAX_VALUE			127
#define HUE_STEP_VALUE			1
#define HUE_DEF_VALUE			0
#define HUE_MIN_VALUE_IN_FW		-128
#define HUE_MAX_VALUE_IN_FW		127
#define HUE_DEF_VALUE_IN_FW		0

#define SHARPNESS_MIN_VALUE		0
#define SHARPNESS_MAX_VALUE		255
#define SHARPNESS_STEP_VALUE		1
#define SHARPNESS_DEF_VALUE		128

#define SATURATION_MIN_VALUE		-128
#define SATURATION_MAX_VALUE		127
#define SATURATION_STEP_VALUE		1
#define SATURATION_DEF_VALUE		0
#define SATURATION_MIN_VALUE_IN_FW	0
#define SATURATION_MAX_VALUE_IN_FW	255
#define SATURATION_DEF_VALUE_IN_FW	128

#define BACKLIGHT_COMPENSATION_MIN_VALUE	0
#define BACKLIGHT_COMPENSATION_MAX_VALUE	1
#define BACKLIGHT_COMPENSATION_STEP_VALUE	1
#define BACKLIGHT_COMPENSATION_DEF_VALUE	1

#define WHITEBALANCE_MIN_VALUE		0
#define WHITEBALANCE_MAX_VALUE		50000
#define WHITEBALANCE_STEP_VALUE		1
#define WHITEBALANCE_DEF_VALUE		100

#define COLOR_ENABLE_MIN_VALUE		0
#define COLOR_ENABLE_MAX_VALUE		1
#define COLOR_ENABLE_STEP_VALUE		1
#define COLOR_ENABLE_DEF_VALUE		1

#define GAIN_MIN_VALUE			0
#define GAIN_MAX_VALUE			4
#define GAIN_STEP_VALUE			1
#define GAIN_DEF_VALUE			1

#define GAMA_MIN_VALUE			100
#define GAMA_MAX_VALUE			500
#define GAMA_STEP_VALUE			1
#define GAMA_DEF_VALUE			220

#define ANTI_FLICKER_MIN_VALUE		0
#define ANTI_FLICKER_MAX_VALUE		2
#define ANTI_FLICKER_STEP_VALUE		1
/* 0 - disable, 1 - 50HZ, 2 - 60HZ */
#define ANTI_FLICKER_DEF_VALUE		0

/* Digital Multiplier, aka Digital Zoom */
#define DIGITAL_MULTIPLIER_STEP_VALUE		1
#define DIGITAL_MULTIPLIER_MIN_VALUE		100
#define DIGITAL_MULTIPLIER_MAX_VALUE		400
#define DIGITAL_MULTIPLIER_LIMIT_STEP_VALUE	1
#define DIGITAL_MULTIPLIER_LIMIT_MIN_VALUE	100
#define DIGITAL_MULTIPLIER_LIMIT_MAX_VALUE	400

/* property for camera control */
/* according to ms's definition,This value is expressed in log base 2 seconds, */
/* so -7 = 1/128s, -6 = 1/64s, 0 = 1s, 1 = 2s */
#define EXPOSURE_MIN_VALUE		-10
#define EXPOSURE_MAX_VALUE		1
#define EXPOSURE_STEP_VALUE		1
#define EXPOSURE_DEF_VALUE		-5	/* 31.25 ms. */
#define EXPOSURE_RELATIVE_DEF_VALUE	0
#define EXPOSURE_FLAG_DEF_VALUE		KSPROPERTY_CAMERACONTROL_FLAGS_AUTO

#define MAX_EXPOSURE_TIME (10000000ULL * 60)	/* 1 min */
#define MIN_EXPOSURE_TIME 10000	/* 1 ms */

#define EVCOMPENSATION_MIN_VALUE	-2
#define EVCOMPENSATION_MAX_VALUE	2
#define EVCOMPENSATION_STEP_VALUE	1
#define EVCOMPENSATION_DEF_VALUE	0

#define FOCUS_MIN_VALUE			0
#define FOCUS_MAX_VALUE			1023
#define FOCUS_HYPER_VALUE		1000
#define FOCUS_MACRO_FLOOR		0
#define FOCUS_MACRO_CEIL		64
#define FOCUS_HYPER_FLOOR		1000
#define FOCUS_HYPER_CEIL		FOCUS_MAX_VALUE

#define FOCUS_STEP_VALUE		1
#define FOCUS_DEF_VALUE			64

#define ZOOM_MIN_VALUE			100	/* 10 */
#define ZOOM_MAX_VALUE			400	/* 600 */
#define ZOOM_STEP_VALUE			10	/* 600 */
#define ZOOM_DEF_VALUE			100	/* 600 */

#define FOCAL_LEN_MIN_VALUE		1	/* 10 */
#define FOCAL_LEN_MAX_VALUE		65536	/* 600 */
#define FOCAL_LEN_STEP_VALUE		10	/* 600 */
#define FOCAL_LEN_DEF_VALUE		1	/* 600 */

#define ZOOM_MAX_H_OFF			32
#define ZOOM_MIN_H_OFF			(-ZOOM_MAX_H_OFF)
#define ZOOM_H_OFF_STEP_VALUE		1
#define ZOOM_H_OFF_DEF_VALUE		0

#define ZOOM_MAX_V_OFF			32
#define ZOOM_MIN_V_OFF			(-ZOOM_MAX_V_OFF)
#define ZOOM_V_OFF_STEP_VALUE		1
#define ZOOM_V_OFF_DEF_VALUE		0

#define MAX_PHOTO_SEQUENCE_FPS			30
#define MAX_PHOTO_SEQUENCE_HOSTORY_FRAME_COUNT	10
#define MAX_PHOTO_SEQUENCE_FRAME_RATE		MAX_PHOTO_SEQUENCE_FPS

#define MAX_AF_ROI_NUM			3
#define MAX_AE_ROI_NUM			1
#define MAX_AWB_ROI_NUM			1

#define MAX_ITIME_FOR_VFR		66666	/* 15fps */

#endif /* _ISP_PROPERTY_DEF_H_ */
