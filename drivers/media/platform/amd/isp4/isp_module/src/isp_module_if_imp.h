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

#ifndef ISP_MODULE_IF_IMP_H
#define ISP_MODULE_IF_IMP_H

/*
 * init isp module interface, it must be called firstly before any operataion
 * to isp
 * normally called when isp device is probed
 */
int ispm_if_init(struct isp_module_if *intf, struct amd_cam *pamd_cam);
/* unit isp module interface, normally called when isp deiveice is removed */
void ispm_if_fini(struct isp_module_if *intf);

enum imf_ret_value open_camera(enum camera_port_id cam_id,
			       s32 res_fps_id,
			       uint32_t flag);

enum imf_ret_value close_camera(enum camera_port_id cam_id);

enum imf_ret_value set_stream_buf(enum camera_port_id cam_id,
				  enum stream_id stream_id,
				  struct sys_img_buf_info *buf);

enum imf_ret_value set_stream_para(enum camera_port_id cam_id,
				   enum stream_id stream_id,
				   enum para_id para_type,
				   void *para_value);

enum imf_ret_value start_stream(enum camera_port_id cam_id,
				enum stream_id stream_id);

enum imf_ret_value stop_stream(enum camera_port_id cam_id,
			       enum stream_id stream_id);

void reg_notify_cb(enum camera_port_id cam_id,
		   func_isp_module_cb cb,
		   void *cb_context);

void unreg_notify_cb(enum camera_port_id cam_id);

enum imf_ret_value set_roi(enum camera_port_id cam_id,
			   u32 type,
			   struct isp_roi_info *roi);

#endif
