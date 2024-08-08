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

#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include "amd_isp_debug.h"
#include "amd_common.h"
#include "swisp_if_imp.h"
#include "isp_module_if.h"
#include "isp_module_if_imp.h"
#include "amd_stream.h"
#include "isp_common.h"
#include "isp_mc_addr_mgr.h"

static void isp_fw_pl_list_destroy(struct isp_fw_cmd_pay_load_buf *head)
{
	struct isp_fw_cmd_pay_load_buf *temp;

	if (!head)
		return;

	while (head) {
		temp = head;
		head = head->next;
		isp_sys_mem_free(temp);
	}
}

void isp_fw_indirect_cmd_pl_buf_init(struct isp_fw_work_buf_mgr *mgr,
				     uint64 sys_addr,
				     uint64 mc_addr,
				     uint32 len)
{
	uint32 pkg_size;
	uint64 base_sys;
	uint64 base_mc;
	uint64 next_sys;
	uint64 next_mc;
	uint32 i;
	struct isp_fw_cmd_pay_load_buf *new_buffer;
	struct isp_fw_cmd_pay_load_buf *tail_buffer;

	if (!mgr || !sys_addr || !mc_addr || !len) {
		ISP_PR_ERR("-><- %s fail param %llx %llx %u\n",
			   __func__, sys_addr, mc_addr, len);
	}

	memset(mgr, 0, sizeof(struct isp_fw_work_buf_mgr));
	mgr->sys_base = sys_addr;
	mgr->mc_base = mc_addr;
	mgr->pay_load_pkg_size = isp_get_cmd_pl_size();

	isp_mutex_init(&mgr->mutex);
	ISP_PR_DBG("-> %s, sys 0x%llx,mc 0x%llx,len %u\n", __func__,
		   sys_addr, mc_addr, len);
	pkg_size = mgr->pay_load_pkg_size;

	base_sys = sys_addr;
	base_mc = mc_addr;
	next_sys = sys_addr;
	next_mc = mc_addr;

	i = 0;
	while (true) {
		new_buffer = isp_sys_mem_alloc(sizeof(*new_buffer));
		if (!new_buffer) {
			ISP_PR_ERR("%s fail to alloc %uth pl buf\n", __func__,
				   i);
			break;
		};

		next_mc = ISP_ADDR_ALIGN_UP(next_mc,
					    ISP_FW_CMD_PAY_LOAD_BUF_ALIGN);
		if ((next_mc + pkg_size - base_mc) > len) {
			isp_sys_mem_free(new_buffer);
			break;
		}

		i++;
		next_sys = base_sys + (next_mc - base_mc);
		new_buffer->mc_addr = next_mc;
		new_buffer->sys_addr = next_sys;
		new_buffer->next = NULL;
		if (!mgr->free_cmd_pl_list) {
			mgr->free_cmd_pl_list = new_buffer;
		} else {
			tail_buffer = mgr->free_cmd_pl_list;
			while (tail_buffer->next)
				tail_buffer = tail_buffer->next;
			tail_buffer->next = new_buffer;
		}
		next_mc += pkg_size;
	}
	mgr->pay_load_num = i;
	/* print_fw_work_buf_info(&l_isp_work_buf_mgr); */
	ISP_PR_DBG("<- %s suc, pl_num %u\n", __func__, mgr->pay_load_num);
	return;
};

void isp_fw_indirect_cmd_pl_buf_uninit(struct isp_fw_work_buf_mgr *mgr)
{
	if (!mgr)
		return;
	isp_fw_pl_list_destroy(mgr->free_cmd_pl_list);
	mgr->free_cmd_pl_list = NULL;
	isp_fw_pl_list_destroy(mgr->used_cmd_pl_list);
	mgr->used_cmd_pl_list = NULL;
}

s32 isp_fw_get_nxt_indirect_cmd_pl(struct isp_fw_work_buf_mgr *mgr,
				   uint64 *sys_addr,
				   uint64 *mc_addr, uint32 *len)
{
	struct isp_fw_cmd_pay_load_buf *temp;
	struct isp_fw_cmd_pay_load_buf *tail;
	s32 ret;

	if (!mgr) {
		ISP_PR_ERR("-><- %s fail for bad para\n", __func__);
		return RET_INVALID_PARAM;
	}

	isp_mutex_lock(&mgr->mutex);
	if (!mgr->free_cmd_pl_list) {
		ISP_PR_ERR("-><- %s fail for no free\n", __func__);
		ret = RET_FAILURE;
		goto quit;
	} else {
		temp = mgr->free_cmd_pl_list;
		mgr->free_cmd_pl_list = temp->next;
		temp->next = NULL;
		if (sys_addr)
			*sys_addr = temp->sys_addr;
		if (mc_addr)
			*mc_addr = temp->mc_addr;
		if (len)
			*len = mgr->pay_load_pkg_size;
		if (!mgr->used_cmd_pl_list) {
			mgr->used_cmd_pl_list = temp;
		} else {
			tail = mgr->used_cmd_pl_list;
			while (tail->next)
				tail = tail->next;
			tail->next = temp;
		};
		ret = RET_SUCCESS;
		ISP_PR_ERR("-><- %s, sys:0x%llx(%u), mc:0x%llx\n", __func__,
			   temp->sys_addr,
			   mgr->pay_load_pkg_size, temp->mc_addr);
		goto quit;
	}
quit:
	isp_mutex_unlock(&mgr->mutex);
	return ret;
}

s32 isp_fw_ret_indirect_cmd_pl(struct isp_fw_work_buf_mgr *mgr, uint64 mc_addr)
{
	struct isp_fw_cmd_pay_load_buf *temp = NULL;
	struct isp_fw_cmd_pay_load_buf *tail;
	s32 ret;

	if (!mgr) {
		ISP_PR_ERR("-><- %s fail for bad para\n", __func__);
		return RET_INVALID_PARAM;
	}
	if (!mgr->used_cmd_pl_list) {
		ISP_PR_ERR("-><- %s fail for no used list\n", __func__);
		return RET_FAILURE;
	}
	isp_mutex_lock(&mgr->mutex);
	if (mgr->used_cmd_pl_list->mc_addr == mc_addr) {
		temp = mgr->used_cmd_pl_list;
		mgr->used_cmd_pl_list = temp->next;
	} else {
		tail = mgr->used_cmd_pl_list;
		while (tail->next) {
			if (tail->next->mc_addr != mc_addr) {
				tail = tail->next;
			} else {
				temp = tail->next;
				tail->next = temp->next;
				break;
			};
		};
		if (!temp) {
			ISP_PR_ERR
			("-><- %s fail for no used found\n", __func__);
			ret = RET_FAILURE;
			goto quit;
		} else {
			ISP_PR_ERR("-><- %s, ret mc:0x%llx\n", __func__,
				   mc_addr);
		}
	};
	temp->next = mgr->free_cmd_pl_list;
	mgr->free_cmd_pl_list = temp;
	ret = RET_SUCCESS;
quit:
	isp_mutex_unlock(&mgr->mutex);
	return ret;
}

void isp_fw_buf_get_cmd_base(struct isp_context *isp,
			     enum fw_cmd_resp_stream_id id,
			     u64 *sys_addr, u64 *mc_addr, u32 *len)
{
	u32 offset;
	u32 idx;
	u64 mc_base;
	u8 *sys_base;
	u32 aligned_rb_chunck_size;

	if (!isp) {
		ISP_PR_ERR("fail for null isp");
		return;
	}

	if (!isp->fw_cmd_resp_buf) {
		ISP_PR_ERR("fail for no fw_cmd_resp_buf");
		return;
	}

	mc_base = isp->fw_cmd_resp_buf->gpu_mc_addr;
	sys_base = (u8 *)isp->fw_cmd_resp_buf->sys_addr;
	/* cmd/resp rb base address should be 64Bytes aligned; */
	aligned_rb_chunck_size = RB_PMBMAP_MEM_CHUNK & 0xffffffc0;

	switch (id) {
	case FW_CMD_RESP_STREAM_ID_GLOBAL:
		idx = 3;
		break;
	case FW_CMD_RESP_STREAM_ID_1:
		idx = 0;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		idx = 1;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		idx = 2;
		break;
	default:
		ISP_PR_ERR("fail, bad id %d", id);
		if (len)
			*len = 0;
		return;
	}

	offset = aligned_rb_chunck_size * idx;
	if (len)
		*len = ISP_FW_CMD_BUF_SIZE;
	if (sys_addr)
		*sys_addr = (u64)(sys_base + offset);
	if (mc_addr)
		*mc_addr = mc_base + offset;
};

void isp_fw_buf_get_resp_base(void *context,
			      enum fw_cmd_resp_stream_id id,
			      u64 *sys_addr, u64 *mc_addr, u32 *len)
{
	struct isp_context *isp = (struct isp_context *)context;
	u32 offset;
	u32 idx;
	u64 mc_base;
	u8 *sys_base;
	u32 aligned_rb_chunck_size;

	if (!isp) {
		ISP_PR_ERR("fail for null isp");
		return;
	}

	if (!isp->fw_cmd_resp_buf || !isp->fw_cmd_resp_buf) {
		ISP_PR_ERR("fail for no fw_cmd_resp_buf");
		return;
	}

	mc_base = isp->fw_cmd_resp_buf->gpu_mc_addr;
	sys_base = (u8 *)isp->fw_cmd_resp_buf->sys_addr;
	/* cmd/resp rb base address should be 64Bytes aligned; */
	aligned_rb_chunck_size = RB_PMBMAP_MEM_CHUNK & 0xffffffc0;

	switch (id) {
	case FW_CMD_RESP_STREAM_ID_GLOBAL:
		idx = 3;
		break;
	case FW_CMD_RESP_STREAM_ID_1:
		idx = 0;
		break;
	case FW_CMD_RESP_STREAM_ID_2:
		idx = 1;
		break;
	case FW_CMD_RESP_STREAM_ID_3:
		idx = 2;
		break;
	default:
		ISP_PR_ERR("fail, bad id %d", id);
		if (len)
			*len = 0;
		return;
	}

	offset = aligned_rb_chunck_size * (idx + RESP_CHAN_TO_RB_OFFSET - 1);

	if (len)
		*len = ISP_FW_RESP_BUF_SIZE;	/* ISP_FW_RESP_BUF_SIZE; */
	if (sys_addr)
		*sys_addr = (u64)(sys_base + offset);
	if (mc_addr)
		*mc_addr = mc_base + offset;
};

void isp_fw_get_indirect_cmd_pl_buf_base(struct isp_fw_work_buf_mgr *mgr,
					 u64 *sys_addr,
					 u64 *mc_addr, u32 *len)
{
	u32 offset;

	if (!mgr)
		return;
	offset =
		ISP_FW_CODE_BUF_SIZE + ISP_FW_STACK_BUF_SIZE +
		ISP_FW_HEAP_BUF_SIZE + ISP_FW_TRACE_BUF_SIZE +
		ISP_FW_CMD_BUF_SIZE * ISP_FW_CMD_BUF_COUNT +
		ISP_FW_RESP_BUF_SIZE * ISP_FW_RESP_BUF_COUNT;
	if (len)
		*len = ISP_FW_CMD_PAY_LOAD_BUF_SIZE;
	if (sys_addr)
		*sys_addr =
			mgr->sys_base + offset;
	if (mc_addr)
		*mc_addr =
			mgr->mc_base + offset;
};
