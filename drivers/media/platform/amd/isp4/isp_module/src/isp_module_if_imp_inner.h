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

#include "os_base_type.h"
#include "isp_module_if.h"

#ifndef ISP_MODULE_IF_IMP_INNER_H
#define ISP_MODULE_IF_IMP_INNER_H

#define ISP_NEED_CLKSWITCH(frm_ctrl) \
	((frm_ctrl)->frame_ctrl.private_data.data[SWITCH_LOW_CLK_IDX] == \
	 CLOCK_SWITCH_ENABLE)

void isp_module_if_init(struct isp_module_if *p_interface);
void isp_module_if_init_ext(struct isp_module_if *p_interface);

int set_fw_bin_imp(void *context, void *fw_data, unsigned int fw_len);
int set_calib_bin_imp(void *context, enum camera_port_id cam_id,
		      void *calib_data, unsigned int len);

void de_init(void *context);
int open_camera_imp(void *context, enum camera_port_id camera_id,
		    u32 res_fps_id, u32 flag);
int close_camera_imp(void *context, enum camera_port_id cid);

enum imf_ret_value start_stream_imp(void *context,
				    enum camera_port_id cam_id,
				    enum stream_id stream_id);
int set_test_pattern_imp(void *context, enum camera_port_id cam_id,
			 void *param);
int switch_profile_imp(void *context, enum camera_port_id cid,
		       unsigned int prf_id);
int start_preview_imp(void *context, enum camera_port_id camera_id);
enum imf_ret_value stop_stream_imp(void *context,
				   enum camera_port_id cid,
				   enum stream_id sid);
int stop_preview_imp(void *context,
		     enum camera_port_id camera_id, bool pause);
int set_preview_buf_imp(void *context, enum camera_port_id camera_id,
			struct sys_img_buf_handle *buf_hdl);
enum imf_ret_value set_stream_buf_imp(void *context, enum camera_port_id cam_id,
				      enum stream_id stream_id,
				      struct sys_img_buf_info *buf_hdl);
int set_stream_para_imp(void *context, enum camera_port_id cam_id,
			enum stream_id stream_id, enum para_id para_type,
			void *para_value);
enum imf_ret_value set_roi_imp(void *context,
			       enum camera_port_id cam_id,
			       u32 type,
			       struct isp_roi_info *roi);

void reg_notify_cb_imp(void *context, enum camera_port_id cam_id,
		       func_isp_module_cb cb, void *cb_context);
void unreg_notify_cb_imp(void *context, enum camera_port_id cam_id);

#endif
