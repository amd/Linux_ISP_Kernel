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

#include "chip_mask.h"
#include "chip_offset_byte.h"
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/time.h>
#include "isp_module_if.h"
#include "sensor_properties_pub.h"
#include "param_types_pub.h"
#include "cmd_resp_pub.h"
#include "os_advance_type.h"
#include "amd_isp_debug.h"
#include "isp_common.h"
#include "isp_utils.h"
#include "swisp_if.h"
#include "swisp_if_imp.h"
#include "isp_fw_if.h"
#include "isp_mc_addr_mgr.h"
#include "isp_fw_cmd_resp.h"

u32 get_nxt_cmd_seq_num(struct isp_context *isp)
{
	u32 seq_num;

	if (!isp)
		return 1;

	seq_num = isp->host2fw_seq_num++;
	return seq_num;
}

u32 compute_check_sum(u8 *buf, u32 buf_size)
{
	u32 i;
	u32 checksum = 0;
	u32 *buffer;
	u8 *surplus_ptr;

	buffer = (u32 *)buf;
	for (i = 0; i < buf_size / sizeof(u32); i++)
		checksum += buffer[i];

	surplus_ptr = (u8 *)&buffer[i];
	/* add surplus data crc checksum */
	for (i = 0; i < buf_size % sizeof(u32); i++)
		checksum += surplus_ptr[i];

	return checksum;
}

bool no_fw_cmd_ringbuf_slot(struct isp_context *isp,
			    enum fw_cmd_resp_stream_id cmd_buf_idx)
{
	u32 rreg;
	u32 wreg;
	u32 rd_ptr, wr_ptr;
	u32 new_wr_ptr;
	u32 len;

	isp_get_cmd_buf_regs(cmd_buf_idx, &rreg, &wreg, NULL, NULL, NULL);
	isp_fw_buf_get_cmd_base(isp, cmd_buf_idx,
				NULL, NULL, &len);

	rd_ptr = isp_reg_read(rreg);
	wr_ptr = isp_reg_read(wreg);

	new_wr_ptr = wr_ptr + sizeof(struct cmd_t);

	if (wr_ptr >= rd_ptr) {
		if (new_wr_ptr < len) {
			return 0;
		} else if (new_wr_ptr == len) {
			if (rd_ptr == 0)
				return true;
			else
				return false;
		} else {
			new_wr_ptr -= len;

			if (new_wr_ptr < rd_ptr)
				return false;
			else
				return true;
		}
	} else {
		if (new_wr_ptr < rd_ptr)
			return false;
		else
			return true;
	}
}

result_t insert_isp_fw_cmd(struct isp_context *isp,
			   enum fw_cmd_resp_stream_id stream,
			   struct cmd_t *cmd)
{
	u64 mem_sys;
	u64 mem_addr;
	u32 rreg;
	u32 wreg;
	u32 wr_ptr, old_wr_ptr, rd_ptr;
	u32 len;

	if (!isp || !cmd) {
		ISP_PR_ERR("%s: fail bad cmd[0x%p]\n", __func__, cmd);
		return RET_FAILURE;
	}

	if (stream > FW_CMD_RESP_STREAM_ID_3) {
		ISP_PR_ERR("%s: fail bad stream id[%d]\n",
			   __func__, stream);
		return RET_FAILURE;
	}

	switch (cmd->cmd_id) {
	case CMD_ID_GET_FW_VERSION:
	case CMD_ID_SET_LOG_LEVEL:
		stream = FW_CMD_RESP_STREAM_ID_GLOBAL;
		break;
	default:
		break;
	}

	isp_get_cmd_buf_regs(stream, &rreg, &wreg, NULL, NULL, NULL);
	isp_fw_buf_get_cmd_base(isp, stream,
				&mem_sys, &mem_addr, &len);

	if (no_fw_cmd_ringbuf_slot(isp, stream)) {
		ISP_PR_ERR("%s: fail no cmdslot %s(%d)\n", __func__,
			   isp_dbg_get_stream_str(stream), stream);
		return RET_FAILURE;
	}

	wr_ptr = isp_reg_read(wreg);
	rd_ptr = isp_reg_read(rreg);
	old_wr_ptr = wr_ptr;
	if (rd_ptr > len) {
		ISP_PR_ERR("%s: fail %s(%u),rd_ptr %u(should<=%u),wr_ptr %u\n",
			   __func__, isp_dbg_get_stream_str(stream),
			   stream, rd_ptr, len, wr_ptr);
		return RET_FAILURE;
	}

	if (wr_ptr > len) {
		ISP_PR_ERR("%s: fail %s(%u),wr_ptr %u(should<=%u), rd_ptr %u\n",
			   __func__, isp_dbg_get_stream_str(stream),
			   stream, wr_ptr, len, rd_ptr);
		return RET_FAILURE;
	}

	if (wr_ptr < rd_ptr) {
		mem_addr += wr_ptr;

		memcpy((u8 *)(mem_sys + wr_ptr),
		       (u8 *)cmd, sizeof(struct cmd_t));
	} else {
		if ((len - wr_ptr) >= (sizeof(struct cmd_t))) {
			mem_addr += wr_ptr;

			memcpy((u8 *)(mem_sys + wr_ptr),
			       (u8 *)cmd, sizeof(struct cmd_t));
		} else {
			u32 size;
			u64 dst_addr;
			u8 *src;

			dst_addr = mem_addr + wr_ptr;
			src = (u8 *)cmd;
			size = len - wr_ptr;

			memcpy((u8 *)(mem_sys + wr_ptr), src, size);

			src += size;
			size = sizeof(struct cmd_t) - size;
			dst_addr = mem_addr;

			memcpy((u8 *)(mem_sys), src, size);
		}
	}

	wr_ptr += sizeof(struct cmd_t);
	if (wr_ptr >= len)
		wr_ptr -= len;

	isp_reg_write(wreg, wr_ptr);

	return RET_SUCCESS;
}

struct isp_cmd_element *isp_append_cmd_2_cmdq(struct isp_context *isp,
					      struct isp_cmd_element *command)
{
	struct isp_cmd_element *tail_element = NULL;
	struct isp_cmd_element *copy_command = NULL;

	if (!command) {
		ISP_PR_ERR("%s: NULL cmd pointer\n", __func__);
		return NULL;
	}

	copy_command = isp_sys_mem_alloc(sizeof(struct isp_cmd_element));
	if (!copy_command) {
		ISP_PR_ERR("%s: memory allocate fail\n", __func__);
		return NULL;
	}

	memcpy(copy_command, command, sizeof(struct isp_cmd_element));
	copy_command->next = NULL;
	isp_mutex_lock(&isp->cmd_q_mtx);
	if (!isp->cmd_q) {
		isp->cmd_q = copy_command;
		goto quit;
	}

	tail_element = isp->cmd_q;

	/* find the tail element */
	while (tail_element->next)
		tail_element = tail_element->next;

	/* insert current element after the tail element */
	tail_element->next = copy_command;
quit:
	isp_mutex_unlock(&isp->cmd_q_mtx);
	return copy_command;
}

struct isp_cmd_element *isp_rm_cmd_from_cmdq(struct isp_context *isp,
					     u32 seq_num,
					     u32 cmd_id, int32 signal_evt)
{
	struct isp_cmd_element *curr_element;
	struct isp_cmd_element *prev_element;

	isp_mutex_lock(&isp->cmd_q_mtx);

	curr_element = isp->cmd_q;
	if (!curr_element) {
		ISP_PR_ERR("%s: fail empty q\n", __func__);
		goto quit;
	}

	/* process the first element */
	if (curr_element->seq_num == seq_num &&
	    curr_element->cmd_id == cmd_id) {
		isp->cmd_q = curr_element->next;
		curr_element->next = NULL;
		goto quit;
	}

	prev_element = curr_element;
	curr_element = curr_element->next;

	while (curr_element) {
		if (curr_element->seq_num == seq_num &&
		    curr_element->cmd_id == cmd_id) {
			prev_element->next = curr_element->next;
			curr_element->next = NULL;
			goto quit;
		}

		prev_element = curr_element;
		curr_element = curr_element->next;
	}

	ISP_PR_ERR("%s: cmd(0x%x,seq:%u) not found\n",
		   __func__, cmd_id, seq_num);
quit:
	if (curr_element && curr_element->evt && signal_evt) {
		ISP_PR_DBG("%s: signal event %p\n",
			   __func__, curr_element->evt);
		isp_event_signal(0, curr_element->evt);
	}
	isp_mutex_unlock(&isp->cmd_q_mtx);
	return curr_element;
}

void isp_get_cmd_buf_regs(enum fw_cmd_resp_stream_id idx,
			  u32 *rreg, u32 *wreg,
			  u32 *baselo_reg, u32 *basehi_reg,
			  u32 *size_reg)
{
	switch (idx) {
	case FW_CMD_RESP_STREAM_ID_1:
		if (rreg)
			*rreg = ISP_RB_RPTR1;
		if (wreg)
			*wreg = ISP_RB_WPTR1;
		if (baselo_reg)
			*baselo_reg = ISP_RB_BASE_LO1;
		if (basehi_reg)
			*basehi_reg = ISP_RB_BASE_HI1;
		if (size_reg)
			*size_reg = ISP_RB_SIZE1;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		if (rreg)
			*rreg = ISP_RB_RPTR2;
		if (wreg)
			*wreg = ISP_RB_WPTR2;
		if (baselo_reg)
			*baselo_reg = ISP_RB_BASE_LO2;
		if (basehi_reg)
			*basehi_reg = ISP_RB_BASE_HI2;
		if (size_reg)
			*size_reg = ISP_RB_SIZE2;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		if (rreg)
			*rreg = ISP_RB_RPTR3;
		if (wreg)
			*wreg = ISP_RB_WPTR3;
		if (baselo_reg)
			*baselo_reg = ISP_RB_BASE_LO3;
		if (basehi_reg)
			*basehi_reg = ISP_RB_BASE_HI3;
		if (size_reg)
			*size_reg = ISP_RB_SIZE3;
		break;
	case FW_CMD_RESP_STREAM_ID_GLOBAL:
		if (rreg)
			*rreg = ISP_RB_RPTR4;
		if (wreg)
			*wreg = ISP_RB_WPTR4;
		if (baselo_reg)
			*baselo_reg = ISP_RB_BASE_LO4;
		if (basehi_reg)
			*basehi_reg = ISP_RB_BASE_HI4;
		if (size_reg)
			*size_reg = ISP_RB_SIZE4;
		break;
	default:
		ISP_PR_ERR("fail id[%d]", idx);
		break;
	}
}

void isp_get_resp_buf_regs(enum fw_cmd_resp_stream_id idx,
			   u32 *rreg, u32 *wreg,
			   u32 *baselo_reg, u32 *basehi_reg,
			   u32 *size_reg)
{
	switch (idx) {
	case FW_CMD_RESP_STREAM_ID_1:
		if (rreg)
			*rreg = ISP_RB_RPTR9;
		if (wreg)
			*wreg = ISP_RB_WPTR9;
		if (baselo_reg)
			*baselo_reg = ISP_RB_BASE_LO9;
		if (basehi_reg)
			*basehi_reg = ISP_RB_BASE_HI9;
		if (size_reg)
			*size_reg = ISP_RB_SIZE9;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		if (rreg)
			*rreg = ISP_RB_RPTR10;
		if (wreg)
			*wreg = ISP_RB_WPTR10;
		if (baselo_reg)
			*baselo_reg = ISP_RB_BASE_LO10;
		if (basehi_reg)
			*basehi_reg = ISP_RB_BASE_HI10;
		if (size_reg)
			*size_reg = ISP_RB_SIZE10;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		if (rreg)
			*rreg = ISP_RB_RPTR11;
		if (wreg)
			*wreg = ISP_RB_WPTR11;
		if (baselo_reg)
			*baselo_reg = ISP_RB_BASE_LO11;
		if (basehi_reg)
			*basehi_reg = ISP_RB_BASE_HI11;
		if (size_reg)
			*size_reg = ISP_RB_SIZE11;
		break;
	case FW_CMD_RESP_STREAM_ID_GLOBAL:
		if (rreg)
			*rreg = ISP_RB_RPTR12;
		if (wreg)
			*wreg = ISP_RB_WPTR12;
		if (baselo_reg)
			*baselo_reg = ISP_RB_BASE_LO12;
		if (basehi_reg)
			*basehi_reg = ISP_RB_BASE_HI12;
		if (size_reg)
			*size_reg = ISP_RB_SIZE12;
		break;
	default:
		if (rreg)
			*rreg = 0;
		if (wreg)
			*wreg = 0;
		if (baselo_reg)
			*baselo_reg = 0;
		if (basehi_reg)
			*basehi_reg = 0;
		if (size_reg)
			*size_reg = 0;
		ISP_PR_ERR("fail idx (%u)", idx);
		break;
	}
}

void isp_init_fw_ring_buf(struct isp_context *isp,
			  enum fw_cmd_resp_stream_id idx, u32 cmd)
{
	u32 rreg;
	u32 wreg;
	u32 baselo_reg;
	u32 basehi_reg;
	u32 size_reg;
	u64 mc;
	u32 lo;
	u32 hi;
	u32 len;

	if (cmd) {
		/* command buffer */
		if (!isp || idx > FW_CMD_RESP_STREAM_ID_3) {
			ISP_PR_ERR("(%u:cmd) fail,bad para", idx);
			return;
		}

		isp_get_cmd_buf_regs(idx, &rreg, &wreg,
				     &baselo_reg, &basehi_reg, &size_reg);
		isp_fw_buf_get_cmd_base(isp, idx, NULL, &mc, &len);
	} else {
		/* response buffer */
		if (!isp || idx > FW_CMD_RESP_STREAM_ID_3) {
			ISP_PR_ERR("(%u:resp) fail,bad para", idx);
			return;
		}

		isp_get_resp_buf_regs(idx, &rreg, &wreg,
				      &baselo_reg, &basehi_reg, &size_reg);
		isp_fw_buf_get_resp_base(isp, idx, NULL, &mc, &len);
	}

	ISP_PR_DBG("init %s ringbuf %u, mc 0x%llx(%u)",
		   cmd ? "cmd" : "resp", idx, mc, len);

	isp_split_addr64(mc, &lo, &hi);

	isp_reg_write(rreg, 0);
	isp_reg_write(wreg, 0);
	isp_reg_write(baselo_reg, lo);
	isp_reg_write(basehi_reg, hi);
	isp_reg_write(size_reg, len);

	ISP_PR_DBG("rreg(0x%x)=0x%x", rreg, isp_reg_read(rreg));
	ISP_PR_DBG("wreg(0x%x)=0x%x", wreg, isp_reg_read(wreg));
	ISP_PR_DBG("baselo_reg(0x%x)=0x%x",
		   baselo_reg, isp_reg_read(baselo_reg));
	ISP_PR_DBG("basehi_reg(0x%x)=0x%x",
		   basehi_reg, isp_reg_read(basehi_reg));
	ISP_PR_DBG("size_reg(0x%x)=0x%x", size_reg, isp_reg_read(size_reg));
}

static enum fw_cmd_resp_stream_id
isp_get_stream_id_from_cid(struct isp_context __maybe_unused *isp,
			   enum camera_port_id cid)
{
	if (isp->sensor_info[cid].stream_id != FW_CMD_RESP_STREAM_ID_MAX)
		return isp->sensor_info[cid].stream_id;

	switch (cid) {
	case CAMERA_PORT_0:
		isp->sensor_info[cid].stream_id = FW_CMD_RESP_STREAM_ID_1;
		break;
	case CAMERA_PORT_1:
		isp->sensor_info[cid].stream_id = FW_CMD_RESP_STREAM_ID_2;
		break;
	case CAMERA_PORT_2:
		isp->sensor_info[cid].stream_id = FW_CMD_RESP_STREAM_ID_3;
		break;
	default:
		ISP_PR_ERR("Invalid cid[%d].", cid);
		return FW_CMD_RESP_STREAM_ID_MAX;
	};
	return isp->sensor_info[cid].stream_id;
}

int fw_if_send_img_buf(struct isp_context *isp,
		       struct isp_mapped_buf_info *buffer,
		       enum camera_port_id cam_id, enum stream_id stream_id)
{
	struct cmd_send_buffer cmd;
	enum fw_cmd_resp_stream_id stream;
	int result;

	if (!is_para_legal(isp, cam_id) || !buffer || stream_id > STREAM_ID_ZSL) {
		ISP_PR_ERR("fail para,isp %p,buf %p,cid %u,sid %u\n",
			   isp, buffer, cam_id, stream_id);
		return RET_FAILURE;
	};

	memset(&cmd, 0, sizeof(cmd));
	switch (stream_id) {
	case STREAM_ID_PREVIEW:
		cmd.buffer_type = BUFFER_TYPE_PREVIEW;
		break;
	case STREAM_ID_VIDEO:
		cmd.buffer_type = BUFFER_TYPE_VIDEO;
		break;
	case STREAM_ID_ZSL:
		cmd.buffer_type = BUFFER_TYPE_STILL;
		break;
	default:
		ISP_PR_ERR("fail bad sid %d\n", stream_id);
		return RET_FAILURE;
	};
	stream = isp_get_stream_id_from_cid(isp, cam_id);
	cmd.buffer.vmid_space.bit.vmid = 0;
	cmd.buffer.vmid_space.bit.space = ADDR_SPACE_TYPE_GPU_VA;
	isp_split_addr64(buffer->y_map_info.mc_addr,
			 &cmd.buffer.buf_base_a_lo, &cmd.buffer.buf_base_a_hi);
	cmd.buffer.buf_size_a = buffer->y_map_info.len;

	isp_split_addr64(buffer->u_map_info.mc_addr,
			 &cmd.buffer.buf_base_b_lo, &cmd.buffer.buf_base_b_hi);
	cmd.buffer.buf_size_b = buffer->u_map_info.len;

	isp_split_addr64(buffer->v_map_info.mc_addr,
			 &cmd.buffer.buf_base_c_lo, &cmd.buffer.buf_base_c_hi);
	cmd.buffer.buf_size_c = buffer->v_map_info.len;

	result = isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER,
				 stream,
				 FW_CMD_PARA_TYPE_DIRECT, &cmd, sizeof(cmd));

	if (result != RET_SUCCESS) {
		ISP_PR_ERR("fail send,buf %p,cid %u,sid %u\n",
			   buffer, cam_id, stream_id);
		return RET_FAILURE;
	}
	ISP_PR_DBG("suc,buf %p,cid %u,sid %u, addr:%llx, %llx, %llx\n",
		   buffer, cam_id, stream_id,
		   buffer->y_map_info.mc_addr,
		   buffer->u_map_info.mc_addr,
		   buffer->v_map_info.mc_addr);

	return RET_SUCCESS;
}
