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

#ifndef ISP_FW_INTERFACE_H
#define ISP_FW_INTERFACE_H

#include "isp_common.h"
#include "isp_fw_if/drv_isp_if.h"

enum isp_fw_work_buf_type {
	ISP_FW_WORK_BUF_TYPE_FW,
	ISP_FW_WORK_BUF_TYPE_PACKAGE,
	ISP_FW_WORK_BUF_TYPE_H2F_RING,
	ISP_FW_WORK_BUF_TYPE_F2H_RING
};

struct isp_config_sensor_curr_expo_param {
	enum sensor_id sensor_id;	/* front camera or rear camera */
	u32 curr_gain;	/* current gain */
	u32 curr_integration_time;	/* current integration time */
};

struct isp_create_param {
	u32 unused;
};

u32 isp_fw_get_work_buf_size(enum isp_fw_work_buf_type type);
/* u32 isp_fw_get_work_buf_num(enum isp_fw_work_buf_type type); */

/* refer to  _aidt_isp_create */
int fw_create(struct isp_context *isp);

/* refer to _aidt_isp_start_preview */
int fw_send_start_prev_cmd(struct isp_context *isp_context);
/* refer to fw_send_stop_prev_cmd */
int fw_send_stop_prev_cmd(struct isp_context *isp_context,
			  enum camera_port_id camera_id);
int isp_fw_stop_stream(struct isp_context *isp_context,
		       enum camera_port_id camera_id);
/* refer to _aidt_api_config_af */
int fw_cfg_af(struct isp_context *isp_context, enum camera_port_id camera_id);
/* refer to _aidt_no_slot_for_host2fw_command */
bool no_fw_cmd_ringbuf_slot(struct isp_context *isp,
			    enum fw_cmd_resp_stream_id cmd_buf_idx);

/* refer to _get_next_command_seq_num */
u32 get_nxt_cmd_seq_num(struct isp_context *pcontext);
/* refer to _compute_checksum */
u32 compute_check_sum(u8 *buffer,
		      u32 buffer_size);

int fw_if_send_prev_buf(struct isp_context *pcontext,
			enum camera_port_id cam_id,
			struct isp_img_buf_info *buffer);
int fw_if_send_img_buf(struct isp_context *isp,
		       struct isp_mapped_buf_info *buffer,
		       enum camera_port_id cam_id, enum stream_id stream_id);

/* _aidt_queue_command_insert_tail */
struct isp_cmd_element *isp_append_cmd_2_cmdq(struct isp_context *isp,
					      struct isp_cmd_element *command);
struct isp_cmd_element *isp_rm_cmd_from_cmdq(struct isp_context *isp,
					     u32 seq_num, u32 cmd_id,
					     int signal_evt);

/* refer to _aidt_insert_host2fw_command */
int insert_isp_fw_cmd(struct isp_context *isp,
		      enum fw_cmd_resp_stream_id cmd_buf_idx,
		      struct cmd_t *cmd);

int isp_fw_start(struct isp_context *pcontext);
int isp_fw_boot(struct isp_context *pcontext);
int isp_fw_config_stream(struct isp_context *isp_context,
			 enum camera_port_id cam_id, int pipe_id,
			 enum stream_id stream_id);
int isp_fw_start_streaming(struct isp_context *isp_context,
			   enum camera_port_id cam_id, int pipe_id);
#endif
