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

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pgtable.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include "isp_common.h"
#include "buffer_mgr.h"
#include "isp_fw_if.h"
#include "amd_isp_debug.h"
#include "isp_module_if_imp.h"
#include "isp_module_if_imp_inner.h"
#include "isp_mc_addr_mgr.h"
#include "swisp_if_imp.h"
#include "isp_fw_cmd_resp.h"

static bool is_fw_cmd_supported(u32 cmd)
{
	switch (cmd) {
	case CMD_ID_GET_FW_VERSION:
	case CMD_ID_SET_LOG_LEVEL:
	case CMD_ID_SET_LOG_MODULE:
	case CMD_ID_SET_LOG_MODULE_LEVEL:
	case CMD_ID_SEND_BUFFER:
	case CMD_ID_SET_OUT_CHAN_PROP:
	case CMD_ID_SET_STREAM_CONFIG:
	case CMD_ID_START_STREAM:
	case CMD_ID_STOP_STREAM:
	case CMD_ID_ENABLE_OUT_CHAN:
	case CMD_ID_SET_OUT_CHAN_FRAME_RATE_RATIO:
	case CMD_ID_SET_3A_ROI:
	case CMD_ID_ENABLE_PREFETCH:
		return true;
	default:
		return false;
	}
}

result_t isp_get_f2h_resp(struct isp_context *isp,
			  enum fw_cmd_resp_stream_id stream,
			  struct resp_t *response)
{
	u32 rd_ptr;
	u32 wr_ptr;
	u32 rd_ptr_dbg;
	u32 wr_ptr_dbg;
	u64 mem_addr;
	u32 rreg;
	u32 wreg;
	u32 checksum;
	u32 len;
	void **mem_sys;

	isp_get_resp_buf_regs(stream, &rreg, &wreg, NULL, NULL, NULL);
	isp_fw_buf_get_resp_base(isp, stream,
				 (u64 *)&mem_sys, &mem_addr, &len);

	rd_ptr = isp_reg_read(rreg);
	wr_ptr = isp_reg_read(wreg);
	rd_ptr_dbg = rd_ptr;
	wr_ptr_dbg = wr_ptr;

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

	if (rd_ptr < wr_ptr) {
		if ((wr_ptr - rd_ptr) >= (sizeof(struct resp_t))) {
			memcpy((u8 *)response, (u8 *)mem_sys + rd_ptr,
			       sizeof(struct resp_t));

			rd_ptr += sizeof(struct resp_t);
			if (rd_ptr < len) {
				isp_reg_write(rreg, rd_ptr);
			} else {
				ISP_PR_ERR("%s(%u),rd %u(should<=%u),wr %u\n",
					   isp_dbg_get_stream_str(stream),
					   stream, rd_ptr, len, wr_ptr);
				return RET_FAILURE;
			}
			goto out;
		} else {
			ISP_PR_ERR("sth wrong with wptr and rptr\n");
			return RET_FAILURE;
		}
	} else if (rd_ptr > wr_ptr) {
		u32 size;
		u8 *dst;
		u64 src_addr;

		dst = (u8 *)response;

		src_addr = mem_addr + rd_ptr;
		size = len - rd_ptr;
		if (size > sizeof(struct resp_t)) {
			mem_addr += rd_ptr;
			memcpy((u8 *)response,
			       (u8 *)(mem_sys) + rd_ptr,
			       sizeof(struct resp_t));
			rd_ptr += sizeof(struct resp_t);
			if (rd_ptr < len) {
				isp_reg_write(rreg, rd_ptr);
			} else {
				ISP_PR_ERR("%s(%u),rd %u(should<=%u),wr %u\n",
					   isp_dbg_get_stream_str(stream),
					   stream, rd_ptr, len, wr_ptr);
				return RET_FAILURE;
			}
			goto out;
		} else {
			if ((size + wr_ptr) < (sizeof(struct resp_t))) {
				ISP_PR_ERR("sth wrong with wptr and rptr1\n");
				return RET_FAILURE;
			}

			memcpy(dst, (u8 *)(mem_sys) + rd_ptr, size);

			dst += size;
			src_addr = mem_addr;
			size = sizeof(struct resp_t) - size;
			if (size)
				memcpy(dst, (u8 *)(mem_sys), size);
			rd_ptr = size;
			if (rd_ptr < len) {
				isp_reg_write(rreg, rd_ptr);
			} else {
				ISP_PR_ERR("%s(%u),rd %u(should<=%u),wr %u\n",
					   isp_dbg_get_stream_str(stream),
					   stream, rd_ptr, len, wr_ptr);
				return RET_FAILURE;
			}
			goto out;
		}
	} else {/* if (rd_ptr == wr_ptr) */
		return RET_TIMEOUT;
	}

out:
	checksum = compute_check_sum((u8 *)response,
				     (sizeof(struct resp_t) - 4));

	if (checksum != response->resp_check_sum) {
		ISP_PR_ERR("resp checksum[0x%x],should 0x%x,rdptr %u,wrptr %u",
			   checksum, response->resp_check_sum,
			   rd_ptr_dbg, wr_ptr_dbg);

		ISP_PR_ERR("%s(%u), seqNo %u, resp_id %s(0x%x)\n",
			   isp_dbg_get_stream_str(stream), stream,
			   response->resp_seq_num,
			   isp_dbg_get_resp_str(response->resp_id),
			   response->resp_id);

		return RET_FAILURE;
	}

	return RET_SUCCESS;
}

result_t isp_send_fw_cmd_ex(struct isp_context *isp,
			    enum camera_port_id cam_id,
			    u32 cmd_id,
			    enum fw_cmd_resp_stream_id stream,
			    enum fw_cmd_para_type directcmd,
			    void *package,
			    u32 package_size,
			    struct isp_event *evt,
			    u32 *seq,
			    void *resp_pl,
			    u32 *resp_pl_len)
{
	u64 package_base = 0;
	u64 pack_sys = 0;
	u32 pack_len;
	result_t ret = RET_FAILURE;
	u32 seq_num;
	struct isp_cmd_element command_element = { 0 };
	struct isp_cmd_element *cmd_ele = NULL;
	isp_ret_status_t os_status;
	u32 sleep_count;
	struct cmd_t cmd;
	struct cmd_param_package_t *pkg;
	struct isp_gpu_mem_info *gpu_mem = NULL;

	if (directcmd && package_size > sizeof(cmd.cmd_param)) {
		ISP_PR_ERR("fail pkgsize(%u)>%lu cmd:0x%x,stream %d\n",
			   package_size, sizeof(cmd.cmd_param), cmd_id, stream);
		return ret;
	}

	if (package_size && !package) {
		ISP_PR_ERR("-><- %s, fail null pkg cmd:0x%x,stream %d\n",
			   __func__, cmd_id, stream);
		return ret;
	}
	/* if commands need to be ignored for debug for fw not support list them here */
	if (!is_fw_cmd_supported(cmd_id)) {
		ISP_PR_WARN("cmd:%s(0x%08x) not supported,ret directly\n",
			    isp_dbg_get_cmd_str(cmd_id), cmd_id);
		if (evt)
			isp_event_signal(RET_SUCCESS, evt);

		return RET_SUCCESS;
	}

	/* Semaphore check */
	if (!isp_semaphore_acquire(isp)) {
		ISP_PR_ERR("fail acquire isp semaphore cmd:0x%x,stream %d\n",
			   cmd_id, stream);
		return ret;
	}

	os_status = isp_mutex_lock(&isp->command_mutex);
	sleep_count = 0;
	while (1) {
		if (no_fw_cmd_ringbuf_slot(isp, stream)) {
			u32 rreg;
			u32 wreg;
			u32 len;
			u32 rd_ptr, wr_ptr;

			if (sleep_count < MAX_SLEEP_COUNT) {
				msleep(MAX_SLEEP_TIME);
				ISP_PR_DBG("sleep for no cmd ringbuf slot\n");
				sleep_count++;
				continue;
			}
			isp_get_cmd_buf_regs(stream, &rreg, &wreg,
					     NULL, NULL, NULL);
			isp_fw_buf_get_cmd_base(isp, stream,
						NULL, NULL, &len);

			rd_ptr = isp_reg_read(rreg);
			wr_ptr = isp_reg_read(wreg);
			ISP_PR_ERR("fail no cmdslot cid:%d,stream %s(%d)\n",
				   cmd_id,
				   isp_dbg_get_stream_str(stream),
				   stream);
			ISP_PR_ERR("rreg %u,wreg %u,len %u\n",
				   rd_ptr, wr_ptr, len);

			ret = RET_TIMEOUT;
			goto busy_out;
		}
		break;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd_id = cmd_id;
	switch (stream) {
	case FW_CMD_RESP_STREAM_ID_1:
		cmd.cmd_stream_id = STREAM_ID_1;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		cmd.cmd_stream_id = STREAM_ID_2;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		cmd.cmd_stream_id = STREAM_ID_3;
		break;
	default:
		cmd.cmd_stream_id = (u16)STREAM_ID_INVALID;
		break;
	}

	if (directcmd) {
		if (package && package_size)
			memcpy(cmd.cmd_param, package, package_size);
	}

	else if (package_size <= isp_get_cmd_pl_size()) {
		ret = isp_fw_get_nxt_indirect_cmd_pl
		      (&isp->fw_indirect_cmd_pl_buf_mgr,
		       &pack_sys, &package_base, &pack_len);

		if (ret != RET_SUCCESS) {
			ISP_PR_ERR("-><- %s,no enough pkg buf(0x%08x)\n",
				   __func__, cmd_id);
			goto failure_out;
		}
		memcpy((void *)pack_sys, package, package_size);

		pkg = (struct cmd_param_package_t *)cmd.cmd_param;
		isp_split_addr64(package_base, &pkg->package_addr_lo,
				 &pkg->package_addr_hi);
		pkg->package_size = package_size;
		pkg->package_check_sum = compute_check_sum((u8 *)package, package_size);
	} else {
		ISP_PR_ERR("fail too big indCmdPlSize %u,max %u,camId %d\n",
			   package_size, isp_get_cmd_pl_size(), cmd_id);
		ret = RET_NULL_POINTER;
		goto failure_out;
	}

	seq_num = get_nxt_cmd_seq_num(isp);
	cmd.cmd_seq_num = seq_num;
	cmd.cmd_check_sum = compute_check_sum((u8 *)&cmd, sizeof(cmd) - 1);

	if (seq)
		*seq = seq_num;
	command_element.seq_num = seq_num;
	command_element.cmd_id = cmd_id;
	command_element.mc_addr = package_base;
	command_element.evt = evt;
	command_element.gpu_pkg = gpu_mem;
	command_element.resp_payload = resp_pl;
	command_element.resp_payload_len = resp_pl_len;
	command_element.stream = stream;
	command_element.i2c_reg_addr = (u16)I2C_REGADDR_NULL;
	command_element.cam_id = cam_id;

	cmd_ele = isp_append_cmd_2_cmdq(isp, &command_element);
	if (!cmd_ele) {
		ISP_PR_ERR("-><- %s, fail for isp_append_cmd_2_cmdq\n",
			   __func__);
		ret = RET_NULL_POINTER;
		goto failure_out;
	}

	/* same cmd log format as FW team's,
	 * so it'll be easy to compare and debug if there is sth wrong.
	 */
	ISP_PR_DBG("cmd_id = 0x%08x, name = %s\n",
		   cmd_id, isp_dbg_get_cmd_str(cmd_id));
	ISP_PR_DBG("cmd_stream_id = %u\n", cmd.cmd_stream_id);
	ISP_PR_DBG("cmd_param[0]: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
		   cmd.cmd_param[0], cmd.cmd_param[1],
		   cmd.cmd_param[2], cmd.cmd_param[3]);
	ISP_PR_DBG("cmd_param[4]: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
		   cmd.cmd_param[4], cmd.cmd_param[5],
		   cmd.cmd_param[6], cmd.cmd_param[7]);
	ISP_PR_DBG("cmd_param[8]: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
		   cmd.cmd_param[8], cmd.cmd_param[9],
		   cmd.cmd_param[10], cmd.cmd_param[11]);

	if (cmd_id == CMD_ID_SEND_BUFFER) {
		struct cmd_send_buffer *p = (struct cmd_send_buffer *)package;
		u32 total = p->buffer.buf_size_a + p->buffer.buf_size_b +
			    p->buffer.buf_size_c;
		u64 y = isp_join_addr64
			(p->buffer.buf_base_a_lo, p->buffer.buf_base_a_hi);
		u64 u = isp_join_addr64
			(p->buffer.buf_base_b_lo, p->buffer.buf_base_b_hi);
		u64 v = isp_join_addr64
			(p->buffer.buf_base_c_lo, p->buffer.buf_base_c_hi);

		ISP_PR_DBG("%s(0x%08x:%s) %s,sn:%u,%s,0x%llx,0x%llx,0x%llx,%u",
			   isp_dbg_get_cmd_str(cmd_id), cmd_id,
			   isp_dbg_get_stream_str(stream),
			   directcmd ? "direct" : "indirect", seq_num,
			   isp_dbg_get_buf_type(p->buffer_type),
			   y, u, v, total);

	} else {
		ISP_PR_DBG("%s(0x%08x:%s)%s,sn:%u\n",
			   isp_dbg_get_cmd_str(cmd_id), cmd_id,
			   isp_dbg_get_stream_str(stream),
			   directcmd ? "direct" : "indirect", seq_num);
	}

	isp_get_cur_time_tick(&cmd_ele->send_time);
	ret = insert_isp_fw_cmd(isp, stream, &cmd);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("%s: fail for insert_isp_fw_cmd camId %s(0x%08x)\n",
			   __func__, isp_dbg_get_cmd_str(cmd_id), cmd_id);
		isp_rm_cmd_from_cmdq(isp, cmd_ele->seq_num,
				     cmd_ele->cmd_id, false);
		goto failure_out;
	}

	isp_mutex_unlock(&isp->command_mutex);
	isp_semaphore_release(isp);

	return ret;

failure_out:
	if (package_base)
		isp_fw_ret_indirect_cmd_pl(&isp->fw_indirect_cmd_pl_buf_mgr,
					   package_base);
	if (cmd_ele)
		isp_sys_mem_free(cmd_ele);

busy_out:
	isp_mutex_unlock(&isp->command_mutex);
	isp_semaphore_release(isp);

	return ret;
}

result_t isp_send_fw_cmd(struct isp_context *isp,
			 u32 cmd_id,
			 enum fw_cmd_resp_stream_id stream,
			 enum fw_cmd_para_type directcmd,
			 void *package,
			 u32 package_size)
{
	if (stream >= FW_CMD_RESP_STREAM_ID_MAX) {
		ISP_PR_ERR("%s: invalid fw strId:%d\n", __func__, stream);
		return RET_FAILURE;
	}

	return isp_send_fw_cmd_ex(isp, CAMERA_PORT_MAX,
				  cmd_id, stream, directcmd, package,
				  package_size, NULL, NULL, NULL, NULL);
}

result_t isp_send_fw_cmd_sync(struct isp_context *isp,
			      u32 cmd_id,
			      enum fw_cmd_resp_stream_id stream,
			      enum fw_cmd_para_type directcmd,
			      void *package,
			      u32 package_size,
			      u32 timeout /* in ms */,
			      void *resp_pl,
			      u32 *resp_pl_len)
{
	result_t ret = RET_FAILURE;
	struct isp_event evt;
	uint32 seq;

	if (stream >= FW_CMD_RESP_STREAM_ID_MAX) {
		ISP_PR_ERR("%s: invalid fw strId:%d\n", __func__, stream);
		return RET_FAILURE;
	}

	isp_event_init(&evt, 1, 0);

	ret = isp_send_fw_cmd_ex(isp, CAMERA_PORT_MAX,
				 cmd_id, stream, directcmd, package,
				 package_size, &evt, &seq,
				 resp_pl, resp_pl_len);

	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("%s: fail(%d) send cmd\n", __func__, ret);
		return ret;
	}

	ISP_PR_DBG("before wait cmd:0x%x,evt:%p\n", cmd_id, &evt);
	ret = isp_event_wait(&evt, timeout);
	ISP_PR_DBG("after wait cmd:0x%x,evt:%p\n", cmd_id, &evt);

	if (ret != RET_SUCCESS)
		ISP_PR_ERR("%s: fail(%d) timeout\n", __func__, ret);

	if (ret == RET_TIMEOUT) {
		struct isp_cmd_element *ele;

		ele = isp_rm_cmd_from_cmdq(isp, seq, cmd_id, false);
		if (ele) {
			if (ele->mc_addr)
				isp_fw_ret_indirect_cmd_pl
				(&isp->fw_indirect_cmd_pl_buf_mgr,
				 ele->mc_addr);

			isp_sys_mem_free(ele);
		}
	}

	return ret;
}
