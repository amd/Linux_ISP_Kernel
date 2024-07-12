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

#ifndef __LOG_H__
#define __LOG_H__

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP]"

extern u32 g_drv_log_level;
extern u32 g_fw_log_enable;

#define TRACE_LEVEL_NONE	0
#define TRACE_LEVEL_ERROR	1
#define TRACE_LEVEL_WARNING	2
#define TRACE_LEVEL_INFO	3
#define TRACE_LEVEL_DEBUG	4
#define TRACE_LEVEL_VERBOSE	5

#ifdef OUTPUT_LOG_TO_FILE
void open_fw_log_file(void);
void close_fw_log_file(void);
void isp_write_log(const char *fmt, ...);

#define ISP_PR_ERR(format, ...)	do { \
	if (g_drv_log_level >= TRACE_LEVEL_ERROR)     \
		isp_write_log("[E]%s[%s][%d]" format, \
		LOG_TAG,                             \
		__func__, __LINE__, ##__VA_ARGS__);  \
	} while (0)

#define ISP_PR_WARN(format, ...) do { \
	if (g_drv_log_level >= TRACE_LEVEL_WARNING)   \
		isp_write_log("[W]%s[%s][%d]" format, \
		LOG_TAG,                             \
		__func__, __LINE__, ##__VA_ARGS__);  \
	} while (0)

#define ISP_PR_INFO(format, ...) do { \
	if (g_drv_log_level >= TRACE_LEVEL_INFO)      \
		isp_write_log("[I]%s[%s][%d]" format, \
		LOG_TAG,                             \
		__func__, __LINE__, ##__VA_ARGS__);  \
	} while (0)

#define ISP_PR_DBG(format, ...) do { \
	if (g_drv_log_level >= TRACE_LEVEL_DEBUG)     \
		isp_write_log("[D]%s[%s][%d]" format, \
		LOG_TAG,                             \
		__func__, __LINE__, ##__VA_ARGS__);  \
	} while (0)

#define ISP_PR_VERB(format, ...) do { \
	if (g_drv_log_level >= TRACE_LEVEL_VERBOSE)   \
		isp_write_log("[V]%s[%s][%d]" format, \
		LOG_TAG,                             \
		__func__, __LINE__, ##__VA_ARGS__);  \
	} while (0)

#define ISP_PR_PC(format, ...) do { \
	if (g_drv_log_level >= TRACE_LEVEL_WARNING)   \
		isp_write_log("[P]%s[%s][%d]" format, \
		LOG_TAG,                             \
		__func__, __LINE__, ##__VA_ARGS__);  \
	} while (0)

#define ENTER() ISP_PR_DBG("%s", "Entry!")
#define EXIT()  ISP_PR_DBG("%s", "Exit!")
#define RET(X)  ISP_PR_DBG("Exit with %d!", (X))

#else
#define ENTER() {if (g_drv_log_level >= TRACE_LEVEL_DEBUG) { \
		pr_info("[D][Cam]%s[%s][%d]: Entry!\n", \
		LOG_TAG, __func__, __LINE__); } }
#define EXIT() {if (g_drv_log_level >= TRACE_LEVEL_DEBUG) { \
		pr_info("[D][Cam]%s[%s][%d]: Exit!\n", \
		LOG_TAG, __func__, __LINE__); } }
#define RET(X) {if (g_drv_log_level >= TRACE_LEVEL_DEBUG) { \
		pr_info("[D][Cam]%s[%s][%d]: Exit with %d!\n", \
		LOG_TAG, __func__, __LINE__, X); } }

/* PC: performance check */
#define ISP_PR_PC(format, ...) { if (g_drv_log_level >= \
		TRACE_LEVEL_WARNING) { pr_info("[P][Cam]%s[%s][%d]:" \
		format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#define ISP_PR_ERR(format, ...) { if (g_drv_log_level >= \
		TRACE_LEVEL_ERROR) { pr_err("[E][Cam]%s[%s][%d]:" \
		format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#define ISP_PR_WARN(format, ...) { if (g_drv_log_level >= \
		TRACE_LEVEL_WARNING) { pr_warn("[W][Cam]%s[%s][%d]:" \
		format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#define ISP_PR_INFO(format, ...) { if (g_drv_log_level >= \
		TRACE_LEVEL_INFO) { pr_info("[I][Cam]%s[%s][%d]:" \
		format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#define ISP_PR_DBG(format, ...) { if (g_drv_log_level >= \
		TRACE_LEVEL_DEBUG) { pr_info("[D][Cam]%s[%s][%d]:" \
		format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#define ISP_PR_VERB(format, ...) { if (g_drv_log_level >= \
		TRACE_LEVEL_VERBOSE) { pr_notice("[V][Cam]%s[%s][%d]:" \
		format, LOG_TAG, __func__, __LINE__, ##__VA_ARGS__); } }

#endif
#define assert(x)		\
	do {			\
		if (!(x))	\
			ISP_PR_ERR("!!!ASSERT ERROR: " #x " !!!"); \
	} while (0)
/*
 *#define return_if_assert_fail(x)	\
 *	do {				\
 *		if (!(x))		\
 *			ISP_PR_ERR("!!!ASSERT ERROR:" #x " !!!"); \
 *		return;			\
 *	} while (0)
 *
 *#define return_val_if_assert_fail(x, val)	\
 *	do {					\
 *		if (!(x))			\
 *			ISP_PR_ERR("!!!ASSERT ERROR:" #x " !!!"); \
 *		return val;			\
 *	} while (0)
 */

char *get_host_2_fw_cmd_str(u32 cmd_id);
char *isp_dbg_get_isp_status_str(u32 status);
void dbg_show_sensor_caps(void *sensor_cap);
void isp_dbg_show_map_info(void *p);
void isp_dbg_show_bufmeta_info(char *pre,
			       u32 cid,
			       void *p,
			       void *orig_buf /* struct sys_img_buf_handle* */);
char *isp_dbg_get_img_fmt_str(void *in /* enum _image_format_t * */);
char *isp_dbg_get_pvt_fmt_str(int fmt /* enum pvt_img_fmt */);
char *isp_dbg_get_out_ch_str(int ch /* enum _isp_pipe_out_ch_t */);
char *isp_dbg_get_out_fmt_str(int fmt /* enum enum _image_format_t */);
char *isp_dbg_get_cmd_str(u32 cmd);
char *isp_dbg_get_buf_type(u32 type);/* enum _buffer_type_t */
char *isp_dbg_get_resp_str(u32 resp);
char *isp_dbg_get_buf_src_str(u32 src);
char *isp_dbg_get_buf_done_str(u32 status);
void isp_dbg_show_img_prop(char *pre,
			   void *p /* struct _image_prop_t * */);
void dbg_show_drv_settings(void *setting);
char *isp_dbg_get_scene_mode_str(u32 mode);
char *isp_dbg_get_stream_str(u32 stream);
char *isp_dbg_get_para_str(u32 para /* enum para_id */);
char *isp_dbg_get_focus_state_str(u32 state);
char *isp_dbg_get_reg_name(u32 reg);

#endif /* __LOG_H__ */
