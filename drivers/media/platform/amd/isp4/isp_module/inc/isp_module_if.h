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

#ifndef ISP_MODULE_INTERFACE_H
#define ISP_MODULE_INTERFACE_H

#include "isp_fw_if/base_types_pub.h"
#include "isp_fw_if/global_param_types_pub.h"
#include "isp_fw_if/param_types_pub.h"

#define ISP_MODULE_IF_VERSION_1 0x0100 /* 1.0 */

/*
 * Set this flag to open camera in HDR mode,
 * otherwise camera will be opened in normal mode
 */
#define SYS_IMG_BUF_MAX_PLANES 3
#define OPEN_CAMERA_FLAG_HDR 0x00000001
#define MAX_ISP_ROI_NUM               16

enum camera_port_id {
	/* camera port0 for both integrate and discrete ISP */
	CAMERA_PORT_0,
	/* camera port1 for both integrate and discrete ISP,
	 * it means front camera for discrete ISP
	 */
	CAMERA_PORT_1,
	/* camera port2 only for integrate ISP */
	CAMERA_PORT_2,

	CAMERA_PORT_MAX
};

enum pvt_img_fmt {
	PVT_IMG_FMT_INVALID = -1,
	PVT_IMG_FMT_YV12,
	PVT_IMG_FMT_I420,
	PVT_IMG_FMT_NV21,
	PVT_IMG_FMT_P010,
	PVT_IMG_FMT_NV12,
	PVT_IMG_FMT_YUV422P,
	PVT_IMG_FMT_YUV422_SEMIPLANAR,
	PVT_IMG_FMT_YUV422_INTERLEAVED,
	PVT_IMG_FMT_L8,
	PVT_IMG_FMT_BAYER_RAW,
	PVT_IMG_FMT_RGB888,
	PVT_IMG_FMT_MAX
};

/* return value of isp module functions */
enum imf_ret_value {
	IMF_RET_SUCCESS = 0,
	IMF_RET_FAIL = -1,
	IMF_RET_INVALID_PARAMETER = -2,
	IMF_RET_NOT_SUPPORT = -3
};

enum stream_id {
	STREAM_ID_PREVIEW,
	STREAM_ID_VIDEO,
	STREAM_ID_ZSL,
	STREAM_ID_NUM = STREAM_ID_ZSL
};

enum isp_3a_type {
	ISP_3A_TYPE_AF  = 0x1,
	ISP_3A_TYPE_AE  = 0x2,
	ISP_3A_TYPE_AWB = 0x4,
};

enum cb_evt_id {
	CB_EVT_ID_FRAME_DONE, /* parameter is frame_done_cb_para */
	CB_EVT_ID_CMD_DONE,
	CB_EVT_ID_PRIVACY,
};

enum para_id {
	PARA_ID_DATA_FORMAT,
	PARA_ID_DATA_RES_FPS_PITCH,
	PARA_ID_MAX_PARA_COUNT
};

enum camera_type {
	CAMERA_TYPE_RGB_BAYER = 0,
	CAMERA_TYPE_RGBIR = 1,
	CAMERA_TYPE_IR = 2,
	CAMERA_TYPE_MEM
};

enum buf_done_status {
	/* It means no corresponding image buf in callback */
	BUF_DONE_STATUS_ABSENT,
	BUF_DONE_STATUS_SUCCESS,
	BUF_DONE_STATUS_FAILED
};

enum isp_roi_kind {
	ISP_ROI_KIND_TOUCH = 0x1,
	ISP_ROI_KIND_FACE  = 0x2,
};

struct isp_point {
	u32 x; /* The x coordinate of the point */
	u32 y; /* The y coordinate of the point */
};

struct isp_area {
	struct isp_point top_left;       /* top left corner */
	struct isp_point bottom_right;   /* bottom right corner */
};

struct isp_touch_area {
	/* Touch region's top left and bottom right points */
	struct isp_area points;
	/* touch area's weight */
	u32    weight;
};

struct isp_touch_roi_info {
	u32          num;                   /* Touch region numbers */
	struct isp_touch_area  area[MAX_ISP_ROI_NUM];/* Touch regions */
};

struct isp_face_marks {
	struct isp_point eye_left;
	struct isp_point eye_right;
	struct isp_point nose;
	struct isp_point mouse_left;
	struct isp_point mouse_right;
};

struct isp_fd_face_info {
	/* The ID of this face */
	u32              face_id;
	/* The score of this face, larger than 0 for valid face */
	u32 score;
	struct isp_area           face_area;   /* The face region info */
	/* The face landmarks info from AMD face detection library */
	struct isp_face_marks  marks;
};

struct isp_face_roi_info {
	/* Set to 0 to disable this face detection info */
	u32 is_enabled;
	/* Frame count of this face detection info from */
	u32 frame_count;
	/* Set to 0 to disable the five marks on the faces */
	u32 is_marks_enabled;
	/* Number of faces */
	u32 num;
	/* Face detection info */
	struct isp_fd_face_info face[MAX_ISP_ROI_NUM];
};

struct isp_roi_info {
	/* See isp_roi_kind, selecting touch mode or face mode or both modes */
	u32 kind;
	struct isp_touch_roi_info touch_info; /* Rouch ROI data */
	struct isp_face_roi_info fd_info;       /* Face detection data */
};

struct sys_img_buf_info {
	struct {
		void *sys_addr;
		u64 mc_addr;
		u32 len;
	} planes[SYS_IMG_BUF_MAX_PLANES];
};

struct take_one_pic_para {
	enum pvt_img_fmt fmt;
	s32 width;
	s32 height;
	s32 luma_pitch;
	s32 chroma_pitch;
};

struct buf_done_info {
	enum buf_done_status status;
	struct sys_img_buf_info buf;
};

/* call back parameter for CB_EVT_ID_FRAME_DONE */
struct frame_done_cb_para {
	s32 poc;
	s32 cam_id;
	s64 time_stamp;
	struct buf_done_info preview;
	struct buf_done_info video;
	struct buf_done_info zsl;
	struct meta_info_t meta_info;
};

/* call back parameter for CB_EVT_ID_CMD_DONE */
struct cmd_done_cb_para {
	s32 cam_id;
	s32 cmd_id;
	s32 cmd_status;
	s32 cmd_seqnum;
	s32 cmd_payload;
};

typedef s32 (*func_isp_module_cb)(void *context,
				  enum cb_evt_id event_id,
				  void *even_para);

struct pvt_img_res_fps_pitch {
	s32 width;
	s32 height;
	s32 fps;
	s32 luma_pitch;
	s32 chroma_pitch;
};

struct isp_module_if {
	/* the interface size; */
	s16 size;
	/*
	 * the interface version,
	 * its value will be (version_high<<16) | version_low,
	 * so the current version 1.0 will be (1<<16)|0
	 */
	s16 version;

	/*
	 * the context of function call,
	 * it should be the first parameter of all function
	 * call in this interface.
	 */
	void *context;

	/* set fw binary. */
	enum imf_ret_value (*set_fw_bin)(void *context,
					 void *fw_data,
					 s32 fw_len);

	/* set calibration data binary. */
	enum imf_ret_value (*set_calib_bin)(void *context,
					    enum camera_port_id cam_id,
					    void *calib_data, s32 len,
					    void *calib_data_default,
					    s32 len_default);

	/*
	 * open a camera including sensor, VCM and flashlight as whole.
	 * @param cam_id indicate which camera to open:
	 *    CAMERA_PORT_0
	 *    CAMERA_PORT_1
	 *    CAMERA_PORT_2
	 * @param res_fps_id
	 *    index got from get_camera_res_fps,
	 *    please refer to get_camera_res_fps, valid for
	 *    CAMERA_PORT_0, CAMERA_PORT_1 and CAMERA_PORT_2,
	 * @param flag
	 *     Ored OPEN_CAMERA_FLAG_*, to indicate the open options,
	 *     valid for CAMERA_PORT_0, CAMERA_PORT_1 and
	 *     CAMERA_PORT_2,
	 */
	enum imf_ret_value (*open_camera)(void *context,
					  enum camera_port_id cam_id,
					  u32 res_fps_id,
					  u32 flag);

	/*
	 * Close a camera including sensor, VCM and flashlight as whole.
	 * @param cam_id
	 *     indicate which camera to open:
	 *     0: rear camera for both integrate and discrete ISP
	 *     1: front left camera for both integrate and discrete ISP,
	 *        it means front camera for discrete ISP
	 *     2: front right camera only for integrate ISP
	 * @param res_fps_id
	 *     index got from get_camera_res_fps,
	 *     please refer to get_camera_res_fps
	 */
	enum imf_ret_value (*close_camera)(void *context,
					   enum camera_port_id cam_id);

	/*
	 * set stream buffer from OS to ISP FW/HW.
	 * return 0 for success others for fail.
	 * @param cam_id
	 *     indicate which camera this buffer is for,
	 *     its available values are:
	 *     0: rear camera for both integrate and discrete ISP
	 *     1: front left camera for both integrate and discrete ISP,
	 *        it means front camera for discrete ISP
	 *     2: front right camera only for integrate ISP
	 * @param stream_id
	 *     stream id of the buffer
	 * @param buf_hdl
	 *     it contains the buffer information,
	 *     please refer to section 6 for the detailed structure definition.
	 */
	enum imf_ret_value (*set_stream_buf)(void *context,
					     enum camera_port_id cam_id,
					     enum stream_id stream_id,
					     struct sys_img_buf_info *buf);

	/*
	 * set parameter for stream.
	 * return 0 for success others for fail.
	 * refer to section 4 for all available parameters.
	 * @param cam_id
	 *     indicate which camera this parameter is for,
	 *     its available values are:
	 *     0: rear camera for both integrate and discrete ISP
	 *     1: front left camera for both integrate and discrete ISP,
	 *        it means front camera for discrete ISP
	 *     2: front right camera only for integrate ISP
	 * @param stream_id
	 *     stream id of the parameter
	 * @param para_type
	 *     it indicates what parameter to set.
	 * @param para_value
	 *     it indicates the parameter value to set.
	 *     different parameter type will have
	 *     different parameter value definition.
	 */
	enum imf_ret_value (*set_stream_para)(void *context,
					      enum camera_port_id cam_id,
					      enum stream_id stream_id,
					      enum para_id para_type,
					      void *para_value);

	/* start stream for cam_id, return 0 for success others for fail */
	enum imf_ret_value (*start_stream)(void *context,
					   enum camera_port_id cam_id,
					   enum stream_id stream_id);

	/* stop stream for cam_id, return 0 for success others for fail */
	enum imf_ret_value (*stop_stream)(void *context,
					  enum camera_port_id cam_id,
					  enum stream_id stream_id);

	void (*reg_notify_cb)(void *context,
			      enum camera_port_id cam_id,
			      func_isp_module_cb cb,
			      void *cb_context);

	/*
	 * unregister callback functions for different events,
	 * event_id is same as that in reg_notify_cb_imp,
	 * more details please refer to section 5.
	 */
	void (*unreg_notify_cb)(void *context, enum camera_port_id cam_id);

	/*
	 * set roi. return 0 for success others for fail.
	 * @param cam_id
	 *     indicate which sensor
	 * @param cam_id
	 *     indicate which sensor
	 * @param type
	 *     ROI type, ORed value of enum isp_3a_type
	 * @param roi
	 *     indicate the detailed roi info
	 */
	enum imf_ret_value (*set_roi)(void *context,
				      enum camera_port_id cam_id,
				      u32 type,
				      struct isp_roi_info *roi);
};

struct isp_module_if *ispm_get_interface(void);
/* enum imf_ret_value ispm_interface_init(struct sw_isp_if *intf); */
/* enum imf_ret_value ispm_interface_uninit(void); */

#ifdef __cplusplus
}
#endif

#endif
