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
#ifndef GLOBAL_PARAM_TYPES_PUB
#define GLOBAL_PARAM_TYPES_PUB

/*
 * @enum sensor_id
 * The enum definition of sensor Id
 */
enum sensor_id {
	SENSOR_ID_INVALID   = -1, /* Invalid sensor id */
	SENSOR_ID_ON_MIPI0  = 0,  /* Sensor id for ISP input from MIPI port 0 */
	SENSOR_ID_ON_MIPI1  = 1,  /* Sensor id for ISP input from MIPI port 1 */
	SENSOR_ID_ON_MIPI2  = 2,  /* Sensor id for ISP input from MIPI port 2 */
	SENSOR_ID_RDMA      = 3,  /* Sensor id for ISP input from RDMA */
	SENSOR_ID_CREST     = 4,  /* Sensor id for ISP input from CREST */
	SENSOR_ID_MAX             /* Maximum sensor id for ISP input */
};

/*
 * @enum sensor_i2c_device_id
 * The enum definition of I2C device Id
 */
enum sensor_i2c_device_id {
	I2C_DEVICE_ID_INVALID = -1, /* I2C_DEVICE_ID_INVALID. */
	I2C_DEVICE_ID_A  = 0,       /* I2C_DEVICE_ID_A. */
	I2C_DEVICE_ID_B  = 1,       /* I2C_DEVICE_ID_B. */
	I2C_DEVICE_ID_C  = 2,       /* I2C_DEVICE_ID_C. */
	I2C_DEVICE_ID_MAX           /* I2C_DEVICE_ID_MAX. */
};

/*
 * @struct error_code
 * The struct definition of fw error code
 */
struct error_code {
	uint32 code1;  /* See ERROR_CODE1_XXX for reference */
	uint32 code2;  /* See ERROR_CODE2_XXX for reference */
	uint32 code3;  /* See ERROR_CODE3_XXX for reference */
	uint32 code4;  /* See ERROR_CODE4_XXX for reference */
	uint32 code5;  /* See ERROR_CODE5_XXX for reference */
};

/*
 * @enum isp_log_level
 * The enum definition of different FW log level
 */
enum isp_log_level {
	ISP_LOG_LEVEL_DEBUG  = 0,   /* The FW will output all debug and above level log */
	ISP_LOG_LEVEL_INFO   = 1,   /* The FW will output all info and above level log */
	ISP_LOG_LEVEL_WARN   = 2,   /* The FW will output all warning and above level log */
	ISP_LOG_LEVEL_ERROR  = 3,   /* The FW will output all error and above level log */
	ISP_LOG_LEVEL_MAX           /* The FW will output none level log */
};

enum isp_rta_log_level {
	ISP_RTA_LOG_LEVEL_DEFAULT   = 0, /* Default RTA log level, will be redirect to info level */
	ISP_RTA_LOG_LEVEL_ERROR     = 1, /* RTA output error log */
	ISP_RTA_LOG_LEVEL_WARN      = 2, /* RTA output error, warn level log */
	ISP_RTA_LOG_LEVEL_INFO      = 3, /* RTA output error, warn, info level log */
	ISP_RTA_LOG_LEVEL_DEBUG0    = 4, /* RTA output error, warn, info, debug0 log */
	ISP_RTA_LOG_LEVEL_DEBUG1    = 5, /* RTA output error, warn, info, debug1 log */
	ISP_RTA_LOG_LEVEL_DEBUG2    = 6, /* RTA output error, warn, info, debug2 log */
	ISP_RTA_LOG_LEVEL_DEBUG3    = 7, /* RTA output error, warn, info, debug3 log */
	ISP_RTA_LOG_LEVEL_DEBUG4    = 8, /* RTA output error, warn, info, debug4 log */
	ISP_RTA_LOG_LEVEL_VERBOSE   = 9, /* RTA output error, warn, info, debug0, verbose log */
	ISP_RTA_LOG_LEVEL_MAX
};

/*
 * @enum sensor_intf_type
 * The enum definition of different sensor interface type
 */
enum sensor_intf_type {
	SENSOR_INTF_TYPE_MIPI      = 0, /* The MIPI Csi2 sensor interface */
	SENSOR_INTF_TYPE_PARALLEL  = 1, /* The Parallel sensor interface */
	SENSOR_INTF_TYPE_RDMA      = 2, /* There is no sensor, just get data from RDMA */
	SENSOR_INTF_TYPE_MAX            /* The max value of enum sensor_intf_type */
};

/*
 * @enum error_level
 * The enum definition of error level
 */
enum error_level {
	ERROR_LEVEL_INVALID, /* invalid level */
	ERROR_LEVEL_FATAL,   /* The error has caused the stream stopped */
	ERROR_LEVEL_WARN,    /* Firmware has automatically restarted the stream */
	ERROR_LEVEL_NOTICE,  /* Should take notice of this error which may lead some other error */
	ERROR_LEVEL_MAX      /* max value of enum error_level */
};

#endif
