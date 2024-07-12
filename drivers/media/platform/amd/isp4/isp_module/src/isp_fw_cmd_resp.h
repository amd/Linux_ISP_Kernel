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

#ifndef ISP_FW_CMD_RESP_H
#define ISP_FW_CMD_RESP_H
#include "isp_common.h"

result_t isp_get_f2h_resp(struct isp_context *isp,
			  enum fw_cmd_resp_stream_id stream,
			  struct resp_t *response);

result_t isp_send_fw_cmd_ex(struct isp_context *isp,
			    enum camera_port_id cam_id,
			    uint32 cmd_id,
			    enum fw_cmd_resp_stream_id stream,
			    enum fw_cmd_para_type directcmd,
			    void *package,
			    uint32 package_size,
			    struct isp_event *evt,
			    uint32 *seq,
			    void *resp_pl,
			    uint32 *resp_pl_len);

result_t isp_send_fw_cmd(struct isp_context *isp,
			 uint32 cmd_id,
			 enum fw_cmd_resp_stream_id stream,
			 enum fw_cmd_para_type directcmd,
			 void *package,
			 uint32 package_size);

result_t isp_send_fw_cmd_sync(struct isp_context *isp,
			      uint32 cmd_id,
			      enum fw_cmd_resp_stream_id stream,
			      enum fw_cmd_para_type directcmd,
			      void *package,
			      uint32 package_size,
			      uint32 timeout /* in ms */,
			      void *resp_pl,
			      uint32 *resp_pl_len);

#endif
