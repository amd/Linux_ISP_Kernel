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
#include "amd_stream.h"
#include "pipeline_id_pub.h"
#include "swisp_if_imp.h"
#include "isp_common.h"
#include "isp_mc_addr_mgr.h"
#include "isp_utils.h"
#include "isp_pwr.h"
#include "isp_fw_boot.h"
#include "isp_fw_if.h"
#include "isp_settings.h"
#include "isp_module_if.h"
#include "isp_module_if_imp.h"
#include "isp_module_if_imp_inner.h"
#include "isp_fw_cmd_resp.h"
#include "isp_fw_thread.h"

#define AGPIO85_ADDRESS     0x02D02654
#define OUTPUT_VALUE_BIT    22
#define STATUS_VALUE_BIT    16
#define OUTPUT_ENABLE_BIT   23

static struct isp_module_if *ispm_if_self;

void isp_pwr_unit_init(struct isp_pwr_unit *unit)
{
	if (unit) {
		unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
		isp_mutex_init(&unit->pwr_status_mutex);
		unit->on_time = 0;
		unit->on_time = MAX_ISP_TIME_TICK;
	}
}

static void enable_agpio85(bool_t high)
{
	uint32 prev_val;
	uint32 val;

	prev_val = isp_indirect_rreg(AGPIO85_ADDRESS);
	val = prev_val;
	if (high)
		val = val | (1 << OUTPUT_VALUE_BIT) | (1 << STATUS_VALUE_BIT);
	else
		val = val & ~((1 << OUTPUT_VALUE_BIT) | (1 << STATUS_VALUE_BIT));

	val = val | (1 << OUTPUT_ENABLE_BIT);

	ISP_PR_DBG("AGPIO85 write val(curr, prev) = (%x, %x)\n", val, prev_val);

	if (prev_val != val)
		isp_indirect_wreg(AGPIO85_ADDRESS, val);
}

static void ispm_context_uninit(struct isp_context *isp_context)
{
	enum camera_port_id cam_id;

	isp_clear_cmdq(isp_context);

	if (isp_context->fw_data) {
		isp_sys_mem_free(isp_context->fw_data);
		isp_context->fw_data = NULL;
		isp_context->fw_len = 0;
	}

	isp_fw_indirect_cmd_pl_buf_uninit
	(&isp_context->fw_indirect_cmd_pl_buf_mgr);
	if (isp_context->indirect_cmd_payload_buf) {
		isp_gpu_mem_free(isp_context->indirect_cmd_payload_buf);
		isp_context->indirect_cmd_payload_buf = NULL;
	}

	for (cam_id = CAMERA_PORT_0; cam_id < CAMERA_PORT_MAX; cam_id++) {
		struct isp_sensor_info *info;
		struct isp_stream_info *str_info;
		uint32 sid;

		info = &isp_context->sensor_info[cam_id];

		for (sid = STREAM_ID_PREVIEW; sid <= STREAM_ID_NUM; sid++) {
			str_info = &info->str_info[sid];

			isp_list_destory(&str_info->buf_free, NULL);
			isp_list_destory(&str_info->buf_in_fw, NULL);
		}
	}

	ISP_SET_STATUS(isp_context, ISP_STATUS_UNINITED);
}

static void ispm_context_init(struct isp_context *isp_info)
{
	enum camera_port_id cam_id;
	uint32 size;

	if (!isp_info) {
		ISP_PR_ERR("-><- %s fail bad param by null context\n",
			   __func__);
		return;
	}

	isp_info->fw_ctrl_3a = 1;

	isp_info->timestamp_fw_base = 0;
	isp_info->timestamp_sw_prev = 0;
	isp_info->timestamp_sw_base = 0;

	isp_info->isp_fw_ver = 0;

	isp_info->refclk = 24;

	isp_info->sensor_count = CAMERA_PORT_MAX;
	isp_mutex_init(&isp_info->ops_mutex);
	isp_mutex_init(&isp_info->map_unmap_mutex);
	isp_mutex_init(&isp_info->cmd_q_mtx);

	isp_mutex_init(&isp_info->command_mutex);
	isp_mutex_init(&isp_info->response_mutex);
	isp_mutex_init(&isp_info->isp_semaphore_mutex);
	isp_info->isp_semaphore_acq_cnt = 0;

	for (cam_id = CAMERA_PORT_0; cam_id < CAMERA_PORT_MAX; cam_id++) {
		struct isp_sensor_info *info;
		struct isp_stream_info *str_info;
		uint32 sid;

		info = &isp_info->sensor_info[cam_id];

		info->cid = cam_id;
		info->actual_cid = cam_id;
		info->tnr_enable = false;
		info->start_str_cmd_sent = false;
		info->status = START_STATUS_NOT_START;
		info->stream_id = FW_CMD_RESP_STREAM_ID_MAX;
		info->raw_width = 0;
		info->raw_height = 0;

		for (sid = STREAM_ID_PREVIEW; sid <= STREAM_ID_NUM; sid++) {
			str_info = &info->str_info[sid];

			isp_list_init(&str_info->buf_free);
			isp_list_init(&str_info->buf_in_fw);
		}
	}

	isp_pwr_unit_init(&isp_info->isp_pu_isp);
	isp_pwr_unit_init(&isp_info->isp_pu_dphy);

	for (cam_id = CAMERA_PORT_0; cam_id < CAMERA_PORT_MAX; cam_id++)
		isp_pwr_unit_init(&isp_info->isp_pu_cam[cam_id]);

	for (enum fw_cmd_resp_stream_id id = FW_CMD_RESP_STREAM_ID_1;
	     id < FW_CMD_RESP_STREAM_ID_MAX; id++) {
		isp_info->fw_cmd_resp_strs_info[id].status =
			FW_CMD_RESP_STR_STATUS_IDLE;
		isp_info->fw_cmd_resp_strs_info[id].cid_owner = CAMERA_PORT_MAX;
	}

	isp_info->host2fw_seq_num = 1;
	ISP_SET_STATUS(isp_info, ISP_STATUS_UNINITED);

	size = INDIRECT_BUF_SIZE * INDIRECT_BUF_CNT;
	if (!isp_info->indirect_cmd_payload_buf)
		isp_info->indirect_cmd_payload_buf = isp_gpu_mem_alloc(size);

	if (isp_info->indirect_cmd_payload_buf &&
	    isp_info->indirect_cmd_payload_buf->sys_addr)
		isp_fw_indirect_cmd_pl_buf_init
		(&isp_info->fw_indirect_cmd_pl_buf_mgr,
		 (uint64)isp_info->indirect_cmd_payload_buf->sys_addr,
		 isp_info->indirect_cmd_payload_buf->gpu_mc_addr, size);

	ISP_SET_STATUS(isp_info, ISP_STATUS_INITED);
	ISP_PR_DBG("<- %s succ\n", __func__);
}

enum imf_ret_value open_camera_imp(void *context,
				   enum camera_port_id cid,
				   u32 res_fps_id,
				   u32 flag)
{
	struct isp_context *isp = (struct isp_context *)context;
	uint32 index;
	enum camera_port_id actual_cid = cid;
	bool rel_sem = TRUE;

	ISP_PR_DBG("-> %s cid[%d] fpsid[%d]  flag:0x%x\n",
		   __func__, actual_cid, res_fps_id, flag);

	if (!is_para_legal(context, cid) ||
	    !is_para_legal(context, actual_cid)) {
		ISP_PR_ERR("<- %s fail for para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (ISP_GET_STATUS(isp) == ISP_STATUS_UNINITED) {
		ISP_PR_ERR("<- %s cid[%d] fail for isp uninit\n",
			   __func__, actual_cid);
		return IMF_RET_FAIL;
	}

	isp_mutex_lock(&isp->ops_mutex);
	if (isp->sensor_info[actual_cid].sensor_opened ||
	    isp->sensor_info[cid].sensor_opened) {
		ISP_PR_DBG("<- %s cid[%d] has opened, do nothing\n",
			   __func__, actual_cid);
		isp_mutex_unlock(&isp->ops_mutex);
		return IMF_RET_SUCCESS;
	}
	if (is_camera_started(isp, actual_cid)) {
		isp_mutex_unlock(&isp->ops_mutex);
		ISP_PR_DBG("<- %s cid[%d] suc for already\n",
			   __func__, actual_cid);
		return IMF_RET_SUCCESS;
	}

	ISP_PR_DBG("enable AGPIO85\n");
	enable_agpio85(true);

	if (isp->fw_mem_pool[cid] && isp->fw_mem_pool[cid]->sys_addr &&
	    isp->fw_mem_pool[cid]->mem_size < INTERNAL_MEMORY_POOL_SIZE) {
		/* the original buffer is too small, free it and do re-alloc */
		isp_gpu_mem_free(isp->fw_mem_pool[cid]);
		isp->fw_mem_pool[cid] = NULL;
	}
	if (!isp->fw_mem_pool[cid]) {
		isp->fw_mem_pool[cid] =
			isp_gpu_mem_alloc(INTERNAL_MEMORY_POOL_SIZE);
		if (!isp->fw_mem_pool[cid]) {
			isp_mutex_unlock(&isp->ops_mutex);
			ISP_PR_ERR("<- %s cid[%d] fail for mempool alloc\n",
				   __func__, actual_cid);
			return IMF_RET_SUCCESS;
		}
	}

	switch (actual_cid) {
	case CAMERA_PORT_1:
		isp->sensor_info[actual_cid].cam_type = CAMERA_PORT_1_RAW_TYPE;
		isp->sensor_info[cid].cam_type = CAMERA_PORT_1_RAW_TYPE;
		break;
	case CAMERA_PORT_2:
		isp->sensor_info[actual_cid].cam_type = CAMERA_PORT_2_RAW_TYPE;
		isp->sensor_info[cid].cam_type = CAMERA_PORT_2_RAW_TYPE;
		break;
	default:
		isp->sensor_info[actual_cid].cam_type = CAMERA_PORT_0_RAW_TYPE;
		isp->sensor_info[cid].cam_type = CAMERA_PORT_0_RAW_TYPE;
		break;
	}

	isp->sensor_info[actual_cid].start_str_cmd_sent = 0;
	isp->sensor_info[actual_cid].channel_buf_sent_cnt = 0;

	if (is_failure(isp_ip_pwr_on(isp, actual_cid, index,
				     flag & OPEN_CAMERA_FLAG_HDR))) {
		ISP_PR_ERR("isp_ip_pwr_on fail");
		goto fail;
	}

	if (!isp_semaphore_acquire(isp)) {
		/* try to continue opening sensor cause it may still work */
		ISP_PR_ERR("in %s, fail acquire isp semaphore,ignore\n",
			   __func__);
		rel_sem = FALSE;
	}

	if (rel_sem)
		isp_semaphore_release(isp);

	if (is_failure(isp_boot_isp_fw_boot(isp))) {
		ISP_PR_ERR("isp_fw_start fail");
		goto fail;
	}

	isp->sensor_info[actual_cid].sensor_opened = 1;
	isp->sensor_info[cid].sensor_opened = 1;
	isp_mutex_unlock(&isp->ops_mutex);

	get_available_fw_cmdresp_stream_id(isp, actual_cid);

	ISP_PR_DBG("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
fail:
	isp_mutex_unlock(&isp->ops_mutex);
	close_camera_imp(isp, cid);
	ISP_PR_DBG("<- %s, ret 0x%x\n", __func__, (unsigned int)IMF_RET_FAIL);
	return IMF_RET_FAIL;
}

enum imf_ret_value close_camera_imp(void *context, enum camera_port_id cid)
{
	struct isp_context *isp = (struct isp_context *)context;
	struct isp_sensor_info *sif;
	uint32 cnt;
	struct isp_cmd_element *ele = NULL;
	unsigned int index = 0;

	if (!is_para_legal(context, cid)) {
		ISP_PR_ERR("-><- %s, fail for para\n", __func__);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);
	ISP_PR_DBG("-> %s, cid %d\n", __func__, cid);
	sif = &isp->sensor_info[cid];
	if (sif->status == START_STATUS_STARTED) {
		ISP_PR_ERR("%s, fail stream still running\n", __func__);
		goto fail;
	}
	sif->status = START_STATUS_NOT_START;

	if (sif->fw_stream_id != FW_CMD_RESP_STREAM_ID_MAX)
		reset_fw_cmdresp_strinfo(isp, sif->fw_stream_id);

	/* index = get_index_from_res_fps_id(isp, cid, sif->cur_res_fps_id); */
	cnt = isp_get_started_stream_count(isp);
	if (cnt > 0) {
		ISP_PR_DBG("%s, no need power off isp\n", __func__);
		isp_clk_change(isp, cid, index, sif->hdr_enable, false);
		goto suc;
	}

	ISP_PR_DBG("%s, power off isp\n", __func__);

	isp_boot_disable_ccpu();
	isp_clk_change(isp, cid, index, sif->hdr_enable, false);
	ISP_SET_STATUS(isp, ISP_STATUS_PWR_OFF);
	isp_ip_pwr_off(isp);

	do {
		enum fw_cmd_resp_stream_id stream_id;

		stream_id = FW_CMD_RESP_STREAM_ID_GLOBAL;
		ele = isp_rm_cmd_from_cmdq_by_stream(isp, stream_id, false);

		if (!ele)
			break;
		if (ele->mc_addr)
			isp_fw_ret_indirect_cmd_pl
			(&isp->fw_indirect_cmd_pl_buf_mgr, ele->mc_addr);
		isp_sys_mem_free(ele);
	} while (ele);

	isp_gpu_mem_free(isp->fw_cmd_resp_buf);
	isp->fw_cmd_resp_buf = NULL;
	isp_gpu_mem_free(isp->fw_running_buf);
	isp->fw_running_buf = NULL;

	ISP_PR_DBG("disable AGPIO85 and delay 20ms\n");
	enable_agpio85(false);
	msleep(20);
suc:
	isp_mutex_unlock(&isp->ops_mutex);
	isp->sensor_info[cid].sensor_opened = 0;
	isp->prev_buf_cnt_sent = 0;
	ISP_PR_DBG("<- %s, suc\n", __func__);
	return IMF_RET_SUCCESS;
fail:
	isp_mutex_unlock(&isp->ops_mutex);
	ISP_PR_ERR("<- %s, fail\n", __func__);
	return IMF_RET_FAIL;
}

result_t isp_setup_fw_mem_pool(struct isp_context *isp,
			       enum camera_port_id cam_id,
			       enum fw_cmd_resp_stream_id fw_stream_id)
{
	struct cmd_send_buffer buf_type;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("-><- %s fail, bad para, isp %p, cam_id %u\n",
			   __func__, isp,
			   cam_id);
		return RET_FAILURE;
	}

	ISP_PR_DBG("-> %s, cid %u, fwStreamId %u\n", __func__,
		   cam_id, fw_stream_id);
	if (!isp->fw_mem_pool[cam_id]) {
		isp->fw_mem_pool[cam_id] =
			isp_gpu_mem_alloc(INTERNAL_MEMORY_POOL_SIZE);
	}

	if (!isp->fw_mem_pool[cam_id] || !isp->fw_mem_pool[cam_id]->sys_addr) {
		ISP_PR_ERR("<- %s fail for allocation mem\n", __func__);
		return RET_FAILURE;
	}

	memset(&buf_type, 0, sizeof(buf_type));
	buf_type.buffer_type = BUFFER_TYPE_MEM_POOL;
	buf_type.buffer.buf_tags = 0;
	buf_type.buffer.vmid_space.bit.vmid = 0;
	buf_type.buffer.vmid_space.bit.space = ADDR_SPACE_TYPE_GPU_VA;
	isp_split_addr64(isp->fw_mem_pool[cam_id]->gpu_mc_addr,
			 &buf_type.buffer.buf_base_a_lo,
			 &buf_type.buffer.buf_base_a_hi);
	buf_type.buffer.buf_size_a = (uint32)isp->fw_mem_pool[cam_id]->mem_size;

	if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, fw_stream_id,
			    FW_CMD_PARA_TYPE_DIRECT,
			    &buf_type, sizeof(buf_type)) != RET_SUCCESS) {
		ISP_PR_ERR("<- %s, send BUFFER_TYPE_MEM_POOL 0x%llx(%u) fail\n",
			   __func__,
			   isp->fw_mem_pool[cam_id]->gpu_mc_addr,
			   buf_type.buffer.buf_size_a);
		return RET_FAILURE;
	}
	ISP_PR_DBG("<- %s, send BUFFER_TYPE_MEM_POOL 0x%llx(%u) suc\n",
		   __func__, isp->fw_mem_pool[cam_id]->gpu_mc_addr,
		    buf_type.buffer.buf_size_a);
	return RET_SUCCESS;
};

void isp_free_fw_mem_pool(struct isp_context *isp, enum camera_port_id cam_id)
{
	if (!isp) {
		ISP_PR_ERR("-><- %s fail, bad para\n", __func__);
		return;
	}
	if (!isp->fw_mem_pool[cam_id] || !isp->fw_mem_pool[cam_id]->sys_addr) {
		ISP_PR_DBG("-><- %s, no fw_mem_pool\n", __func__);
	} else {
		isp_gpu_mem_free(isp->fw_mem_pool[cam_id]);
		isp->fw_mem_pool[cam_id] = NULL;
		ISP_PR_DBG("-><- %s, free fw_mem_pool\n", __func__);
	}
}

result_t isp_alloc_fw_drv_shared_buf(struct isp_context *isp,
				     enum camera_port_id cam_id,
				     enum fw_cmd_resp_stream_id fw_stream_id)
{
	struct fw_cmd_resp_str_info *stream_info;
	struct isp_sensor_info *sensor_info;
	uint32 i;
	uint32 size;

	if (!is_para_legal(isp, cam_id) ||
	    fw_stream_id >= FW_CMD_RESP_STREAM_ID_MAX) {
		ISP_PR_ERR("-><- %s fail bad para,isp:%p fw_stream_id %u\n",
			   __func__, isp,
			   fw_stream_id);
		return RET_FAILURE;
	}

	stream_info = &isp->fw_cmd_resp_strs_info[fw_stream_id];
	sensor_info = &isp->sensor_info[cam_id];

	ISP_PR_DBG("-> %s, cid %u,fw_cmd_resp_stream_id:%d\n", __func__,
		   cam_id, fw_stream_id);

	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		size = META_INFO_BUF_SIZE;
		if (!stream_info->meta_info_buf[i]) {
			stream_info->meta_info_buf[i] = isp_gpu_mem_alloc(size);
			if (stream_info->meta_info_buf[i]) {
				ISP_PR_DBG("alloc %uth meta_info_buf ok\n", i);
			} else {
				ISP_PR_ERR("alloc %uth meta_info_buf fail\n", i);
				return RET_FAILURE;
			}
		}
	}

	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		size = META_DATA_BUF_SIZE;
		if (!stream_info->meta_data_buf[i]) {
			stream_info->meta_data_buf[i] = isp_gpu_mem_alloc(size);
			if (stream_info->meta_data_buf[i]) {
				ISP_PR_DBG("alloc %uth meta_data_buf ok\n", i);
			} else {
				ISP_PR_ERR("alloc %uth meta_data_buf fail\n", i);
				return RET_FAILURE;
			}
		}
	}

	if (!stream_info->cmd_resp_buf) {
		size = MAX_CMD_RESPONSE_BUF_SIZE;
		stream_info->cmd_resp_buf = isp_gpu_mem_alloc(size);
		if (stream_info->cmd_resp_buf) {
			ISP_PR_DBG("alloc cmd_resp_buf ok\n");
		} else {
			ISP_PR_ERR("alloc cmd_resp_buf fail\n");
			return RET_FAILURE;
		}
	}

	return RET_SUCCESS;
}

void isp_free_fw_drv_shared_buf(struct isp_context *isp,
				enum camera_port_id cam_id,
				enum fw_cmd_resp_stream_id fw_stream_id)
{
	struct fw_cmd_resp_str_info *stream_info;
	struct isp_sensor_info *sensor_info;
	uint32 i;

	if (!is_para_legal(isp, cam_id) ||
	    fw_stream_id >= FW_CMD_RESP_STREAM_ID_MAX) {
		ISP_PR_ERR("-><- %s fail, bad para,isp:%p, fw_stream_id %u\n",
			   __func__, isp,
			   fw_stream_id);
		return;
	}

	stream_info = &isp->fw_cmd_resp_strs_info[fw_stream_id];
	sensor_info = &isp->sensor_info[cam_id];

	ISP_PR_DBG("-> %s, cid %u,fw_cmd_resp_stream_id:%d\n", __func__,
		   cam_id, fw_stream_id);

	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		if (stream_info->meta_info_buf[i]) {
			isp_gpu_mem_free(stream_info->meta_info_buf[i]);
			stream_info->meta_info_buf[i] = NULL;
		}
	}

	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		if (stream_info->meta_data_buf[i]) {
			isp_gpu_mem_free(stream_info->meta_data_buf[i]);
			stream_info->meta_data_buf[i] = NULL;
		}
	}

	if (stream_info->cmd_resp_buf) {
		isp_gpu_mem_free(stream_info->cmd_resp_buf);
		stream_info->cmd_resp_buf = NULL;
	}
}

result_t isp_init_stream(struct isp_context *isp, enum camera_port_id cam_id,
			 enum fw_cmd_resp_stream_id fw_stream_id)
{
	ISP_PR_DBG("-> %s, cid:%d, fw streamID: %d\n", __func__, cam_id, fw_stream_id);
	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("<- %s,fail, bad para,cid:%d\n", __func__, cam_id);
		return RET_FAILURE;
	}

	if (isp->fw_cmd_resp_strs_info[fw_stream_id].status ==
	    FW_CMD_RESP_STR_STATUS_INITIALED) {
		ISP_PR_DBG("(cid:%d fw_stream_id:%d),suc do none\n", cam_id, fw_stream_id);
		return RET_SUCCESS;
	}

	if (isp_setup_fw_mem_pool(isp, cam_id, fw_stream_id) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_setup_fw_mem_pool\n");
		return RET_FAILURE;
	}

	if (isp_alloc_fw_drv_shared_buf(isp, cam_id, fw_stream_id) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_alloc_fw_drv_shared_buf\n");
		return RET_FAILURE;
	}

	if (isp_setup_stream(isp, cam_id, fw_stream_id) != RET_SUCCESS) {
		ISP_PR_ERR("fail for isp_setup_stream\n");
		return RET_FAILURE;
	}

	ISP_PR_DBG("set fw stream_id %d to be initialed status\n", fw_stream_id);
	isp->fw_cmd_resp_strs_info[fw_stream_id].status =
		FW_CMD_RESP_STR_STATUS_INITIALED;

	return RET_SUCCESS;
}

void isp_reset_camera_info(struct isp_context *isp, enum camera_port_id cid)
{
	struct isp_sensor_info *info;
	enum stream_id stream_id;

	if (!is_para_legal(isp, cid))
		return;
	info = &isp->sensor_info[cid];

	info->cid = cid;
	info->actual_cid = cid;

	info->status = START_STATUS_NOT_START;
	memset(&info->ae_roi, 0, sizeof(info->ae_roi));
	memset(&info->af_roi, 0, sizeof(info->af_roi));
	memset(&info->awb_region, 0, sizeof(info->awb_region));
	for (stream_id = STREAM_ID_PREVIEW; stream_id <= STREAM_ID_NUM; stream_id++)
		isp_reset_str_info(isp, cid, stream_id);

	info->cur_res_fps_id = -1;
	info->tnr_enable = false;
	info->start_str_cmd_sent = false;
	info->stream_id = FW_CMD_RESP_STREAM_ID_MAX;
	info->sensor_opened = 0;
}

result_t isp_uninit_stream(struct isp_context *isp, enum camera_port_id cam_id,
			   enum fw_cmd_resp_stream_id fw_stream_id)
{
	struct isp_sensor_info *snr_info;
	uint32 i;
	struct isp_cmd_element *ele = NULL;
	uint32 out_cnt;

	if (!is_para_legal(isp, cam_id)) {
		ISP_PR_ERR("-><- %s fail, bad para,cid:%d\n",
			   __func__, cam_id);
		return RET_FAILURE;
	}

	if (isp->fw_cmd_resp_strs_info[fw_stream_id].status !=
	    FW_CMD_RESP_STR_STATUS_INITIALED) {
		ISP_PR_DBG
		("-><- %s (cid:%d, fwstri:%d) do none for not started\n",
		 __func__, cam_id, fw_stream_id);
		return RET_SUCCESS;
	}

	ISP_PR_DBG("-> %s (cid:%d,fw stream_id:%d)\n", __func__, cam_id, fw_stream_id);

	isp_get_stream_output_bits(isp, cam_id, &out_cnt);

	if (out_cnt > 0) {
		ISP_PR_DBG
		("<- %s (cid:%d) fail for there is still %u output\n",
		 __func__, cam_id, out_cnt);
		return RET_FAILURE;
	}

	isp->fw_cmd_resp_strs_info[fw_stream_id].status =
		FW_CMD_RESP_STR_STATUS_OCCUPIED;
	ISP_PR_DBG("%s: reset fw stream_id %d to be occupied\n", __func__,
		   fw_stream_id);

	isp_reset_camera_info(isp, cam_id);
	snr_info = &isp->sensor_info[cam_id];
	do {
		ele = isp_rm_cmd_from_cmdq_by_stream(isp, fw_stream_id, false);
		if (!ele)
			break;
		if (ele->mc_addr)
			isp_fw_ret_indirect_cmd_pl
			(&isp->fw_indirect_cmd_pl_buf_mgr,
			 ele->mc_addr);
		isp_sys_mem_free(ele);
	} while (ele);

	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		if (snr_info->meta_mc[i]) {
			isp_fw_ret_indirect_cmd_pl
			(&isp->fw_indirect_cmd_pl_buf_mgr,
			 snr_info->meta_mc[i]);
			snr_info->meta_mc[i] = 0;
		}
	}

	return RET_SUCCESS;
}

struct sys_img_buf_info *sys_img_buf_handle_cpy(struct sys_img_buf_info *hdl_in)
{
	struct sys_img_buf_info *ret;

	if (!hdl_in)
		return NULL;

	ret = isp_sys_mem_alloc(sizeof(struct sys_img_buf_info));

	if (ret) {
		memcpy(ret, hdl_in, sizeof(struct sys_img_buf_info));
	} else {
		ISP_PR_ERR("failed to alloc buf");
	};

	return ret;
};

void sys_img_buf_handle_free(struct sys_img_buf_info *hdl)
{
	isp_sys_mem_free(hdl);
}

struct isp_mapped_buf_info
*isp_map_sys_2_mc(struct isp_context *isp,
		  struct sys_img_buf_info *sys_img_buf,
		  u32 mc_align,
		  u16 cam_id,
		  u16 stream_id,
		  u32 y_len, u32 u_len, u32 v_len)
{
	struct isp_mapped_buf_info *mapped_buf;

	mapped_buf = isp_sys_mem_alloc(sizeof(struct isp_mapped_buf_info));
	memset(mapped_buf, 0, sizeof(struct isp_mapped_buf_info));

	mapped_buf->sys_img_buf_hdl = sys_img_buf;
	mapped_buf->camera_port_id = (u8)cam_id;
	mapped_buf->stream_id = (u8)stream_id;

	mapped_buf->y_map_info.len = sys_img_buf->planes[0].len;
	mapped_buf->y_map_info.mc_addr = sys_img_buf->planes[0].mc_addr;
	mapped_buf->y_map_info.sys_addr = (u64)sys_img_buf->planes[0].sys_addr;

	mapped_buf->u_map_info.len = sys_img_buf->planes[1].len;
	mapped_buf->u_map_info.mc_addr = sys_img_buf->planes[1].mc_addr;
	mapped_buf->u_map_info.sys_addr = (u64)sys_img_buf->planes[1].sys_addr;

	mapped_buf->v_map_info.len = sys_img_buf->planes[2].len;
	mapped_buf->v_map_info.mc_addr = sys_img_buf->planes[2].mc_addr;
	mapped_buf->v_map_info.sys_addr = (u64)sys_img_buf->planes[2].sys_addr;

	return mapped_buf;
}

void isp_unmap_sys_2_mc(struct isp_context *isp,
			struct isp_mapped_buf_info *buff)
{
}

void isp_take_back_str_buf(struct isp_context *isp,
			   struct isp_stream_info *str,
			   enum camera_port_id cid, enum stream_id sid)
{
	struct isp_mapped_buf_info *img_info = NULL;
	struct frame_done_cb_para *pcb = NULL;
	struct buf_done_info *done_info = NULL;

	if (!isp || !str) {
		ISP_PR_ERR("-><-%s Failed for nullptr of'isp' or 'str'\n",
			   __func__);
		return;
	}

	pcb = (struct frame_done_cb_para *)isp_sys_mem_alloc(sizeof(*pcb));
	if (pcb) {
		switch (sid) {
		case STREAM_ID_PREVIEW:
			done_info = &pcb->preview;
			break;
		case STREAM_ID_VIDEO:
			done_info = &pcb->video;
			break;
		case STREAM_ID_ZSL:
			done_info = &pcb->zsl;
			break;
		default:
			done_info = NULL;
			break;
		}
	} else {
		ISP_PR_ERR("-><- %s failed for nullptr of 'pcb'\n", __func__);
		return;
	}

	do {
		if (img_info) {
			isp_unmap_sys_2_mc(isp, img_info);
			if (img_info->sys_img_buf_hdl) {
				sys_img_buf_handle_free(img_info->sys_img_buf_hdl);
				img_info->sys_img_buf_hdl = NULL;
			}
			isp_sys_mem_free(img_info);
		}

		img_info = (struct isp_mapped_buf_info *)
			   isp_list_get_first(&str->buf_in_fw);
	} while (img_info);

	img_info = NULL;
	do {
		pcb->cam_id = cid;
		if (img_info) {
			isp_unmap_sys_2_mc(isp, img_info);
			if (img_info->sys_img_buf_hdl) {
				sys_img_buf_handle_free(img_info->sys_img_buf_hdl);
				img_info->sys_img_buf_hdl = NULL;
			}
			isp_sys_mem_free(img_info);
		}

		img_info = (struct isp_mapped_buf_info *)
			   isp_list_get_first(&str->buf_free);
	} while (img_info);

	isp_sys_mem_free(pcb);
}

s32 isp_get_pipeline_id(struct isp_context *isp, enum camera_port_id cid)
{
	uint32 pipe_id = MIPI0CSISCSTAT0_LME_ISP_PIPELINE_ID;

	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("%s fail, bad para,cid:%d\n", __func__, cid);
		return pipe_id;
	}

	if (cid == CAMERA_PORT_0)
		pipe_id = MIPI0CSISCSTAT0_LME_ISP_PIPELINE_ID;

	return pipe_id;
}

enum sensor_id isp_get_fw_sensor_id(struct isp_context *isp,
				    enum camera_port_id cid)
{
	enum camera_port_id actual_id = cid;

	if (cid >= CAMERA_PORT_MAX)
		return SENSOR_ID_INVALID;

	if (isp->sensor_info[cid].cam_type == CAMERA_TYPE_MEM)
		return SENSOR_ID_RDMA;
	if (actual_id == CAMERA_PORT_0)
		return SENSOR_ID_ON_MIPI0;
	if (actual_id == CAMERA_PORT_1)
		return SENSOR_ID_ON_MIPI2;
	if (actual_id == CAMERA_PORT_2)
		return SENSOR_ID_ON_MIPI2;

	return SENSOR_ID_INVALID;
}

result_t isp_set_stream_path(struct isp_context *isp, enum camera_port_id cid,
			     enum fw_cmd_resp_stream_id fw_stream_id)
{
	enum camera_port_id actual_id = cid;
	struct cmd_set_stream_cfg stream_path_cmd = {0};
	result_t ret;

	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("-><- %s fail, bad para,cid:%d\n", __func__, cid);
		return RET_FAILURE;
	}

	memset(&stream_path_cmd, 0, sizeof(stream_path_cmd));
	stream_path_cmd.stream_cfg.mipi_pipe_path_cfg.sensor_id =
		isp_get_fw_sensor_id(isp, actual_id);
	stream_path_cmd.stream_cfg.mipi_pipe_path_cfg.b_enable = true;
	stream_path_cmd.stream_cfg.isp_pipe_path_cfg.isp_pipe_id =
		isp_get_pipeline_id(isp, actual_id);

	stream_path_cmd.stream_cfg.b_enable_tnr = true;
	ISP_PR_DBG("cid %u,stream %d, sensor_id %d, pipeId 0x%x EnableTnr %u\n",
		   cid, fw_stream_id,
		   stream_path_cmd.stream_cfg.mipi_pipe_path_cfg.sensor_id,
		   stream_path_cmd.stream_cfg.isp_pipe_path_cfg.isp_pipe_id,
		   stream_path_cmd.stream_cfg.b_enable_tnr);

	ret = isp_send_fw_cmd(isp, CMD_ID_SET_STREAM_CONFIG, fw_stream_id,
			      FW_CMD_PARA_TYPE_DIRECT,
			      &stream_path_cmd, sizeof(stream_path_cmd));
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("<- %s fail for CMD_ID_SET_STREAM_CONFIG\n",
			   __func__);
		return RET_FAILURE;
	}

	return RET_SUCCESS;
}

result_t isp_setup_stream(struct isp_context *isp, enum camera_port_id cid,
			  enum fw_cmd_resp_stream_id fw_stream_id)
{
	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("-><- %s: fail for bad para,cid:%d\n",
			   __func__, cid);
		return RET_FAILURE;
	}

	if (isp_set_stream_path(isp, cid, fw_stream_id) != RET_SUCCESS) {
		ISP_PR_ERR("<- %s fail for set_stream_path\n", __func__);
		return RET_FAILURE;
	}

	ISP_PR_DBG("<- %s suc\n", __func__);
	return RET_SUCCESS;
}

void isp_reset_str_info(struct isp_context *isp, enum camera_port_id cid,
			enum stream_id sid)
{
	struct isp_sensor_info *sif;
	struct isp_stream_info *str_info;

	if (!is_para_legal(isp, cid) || sid > STREAM_ID_NUM)
		return;

	sif = &isp->sensor_info[cid];
	str_info = &sif->str_info[sid];
	str_info->format = PVT_IMG_FMT_INVALID;
	str_info->width = 0;
	str_info->height = 0;
	str_info->luma_pitch_set = 0;
	str_info->chroma_pitch_set = 0;
	str_info->max_fps_numerator = MAX_PHOTO_SEQUENCE_FRAME_RATE;
	str_info->max_fps_denominator = 1;
	str_info->start_status = START_STATUS_NOT_START;
	ISP_PR_DBG("%s,reset cam%d str[%d] Not start\n", __func__, cid, sid);
}

result_t isp_send_meta_buf(struct isp_context *isp, enum camera_port_id cid,
			   enum fw_cmd_resp_stream_id fw_stream_id)
{
	struct fw_cmd_resp_str_info *stream_info;
	uint32 i;
	struct cmd_send_buffer buf_type = { 0 };
	uint32 cnt = 0;

	if (!is_para_legal(isp, cid) ||
	    fw_stream_id >= FW_CMD_RESP_STREAM_ID_MAX) {
		ISP_PR_ERR("-><- %s fail, bad para,cid:%d, fw_stream_id %u\n",
			   __func__, cid, fw_stream_id);
		return RET_FAILURE;
	}

	stream_info = &isp->fw_cmd_resp_strs_info[fw_stream_id];
	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		if (!stream_info->meta_data_buf[i] ||
		    !stream_info->meta_data_buf[i]->sys_addr) {
			ISP_PR_ERR("in  %s(%u:%u) fail, no meta data buf(%u)\n",
				   __func__, cid, fw_stream_id, i);
			continue;
		}
		memset(&buf_type, 0, sizeof(buf_type));
		buf_type.buffer_type = BUFFER_TYPE_META_DATA;
		buf_type.buffer.buf_tags = 0;
		buf_type.buffer.vmid_space.bit.vmid = 0;
		buf_type.buffer.vmid_space.bit.space = ADDR_SPACE_TYPE_GPU_VA;
		isp_split_addr64(stream_info->meta_data_buf[i]->gpu_mc_addr,
				 &buf_type.buffer.buf_base_a_lo,
				 &buf_type.buffer.buf_base_a_hi);
		buf_type.buffer.buf_size_a =
			(uint32)stream_info->meta_data_buf[i]->mem_size;
		if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, fw_stream_id,
				    FW_CMD_PARA_TYPE_DIRECT, &buf_type,
				    sizeof(buf_type)) != RET_SUCCESS) {
			ISP_PR_ERR("in  %s(%u) send meta(%u) fail\n", __func__,
				   cid, i);
			continue;
		}
		cnt++;
	}

	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		if (!stream_info->meta_info_buf[i] ||
		    !stream_info->meta_info_buf[i]->sys_addr) {
			ISP_PR_ERR("in  %s(%u:%u) fail, no meta info buf(%u)\n",
				   __func__, cid, fw_stream_id, i);
			continue;
		}
		memset(stream_info->meta_info_buf[i]->sys_addr, 0,
		       stream_info->meta_info_buf[i]->mem_size);
		memset(&buf_type, 0, sizeof(buf_type));
		buf_type.buffer_type = BUFFER_TYPE_META_INFO;
		buf_type.buffer.buf_tags = 0;
		buf_type.buffer.vmid_space.bit.vmid = 0;
		buf_type.buffer.vmid_space.bit.space = ADDR_SPACE_TYPE_GPU_VA;
		isp_split_addr64(stream_info->meta_info_buf[i]->gpu_mc_addr,
				 &buf_type.buffer.buf_base_a_lo,
				 &buf_type.buffer.buf_base_a_hi);
		buf_type.buffer.buf_size_a =
			(uint32)stream_info->meta_info_buf[i]->mem_size;
		if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, fw_stream_id,
				    FW_CMD_PARA_TYPE_DIRECT, &buf_type,
				    sizeof(buf_type)) != RET_SUCCESS) {
			ISP_PR_ERR("in  %s(%u) send meta(%u) fail\n", __func__,
				   cid, i);
			continue;
		}
		cnt++;
	}
	if (cnt) {
		ISP_PR_DBG("-><- %s, cid %u, %u meta sent suc\n", __func__,
			   cid, cnt);
		return RET_SUCCESS;
	}

	ISP_PR_ERR("-><- %s, cid %u, fail, no meta sent\n",
		   __func__, cid);
	return RET_FAILURE;
}

result_t isp_kickoff_stream(struct isp_context *isp,
			    enum camera_port_id cid,
			    enum fw_cmd_resp_stream_id fw_stream_id,
			    uint32 w, uint32 h)
{
	struct isp_sensor_info *sif;
	struct cmd_config_mmhub_prefetch prefetch = {0};

	if (!is_para_legal(isp, cid) ||
	    fw_stream_id >= FW_CMD_RESP_STREAM_ID_MAX) {
		ISP_PR_ERR
		("-><- %s fail for para,cid:%d, fw_stream_id %u\n", __func__,
		 cid, fw_stream_id);
		return RET_FAILURE;
	};
	sif = &isp->sensor_info[cid];

	if (sif->status == START_STATUS_STARTED) {
		ISP_PR_DBG("-><- %s suc, do none for already started\n",
			   __func__);
		return RET_SUCCESS;
	} else if (sif->status == START_STATUS_START_FAIL) {
		ISP_PR_ERR("-><- %s fail for start fail before\n", __func__);
		return RET_FAILURE;
	}

	ISP_PR_DBG("-> %s cid:%d,w:%u,h:%u\n", __func__, cid, w, h);

	sif->status = START_STATUS_START_FAIL;

	isp->prev_buf_cnt_sent = 0;

	if (isp_send_meta_buf(isp, cid, fw_stream_id) != RET_SUCCESS) {
		ISP_PR_ERR("<- %s, fail for isp_send_meta_buf\n", __func__);
		return RET_FAILURE;
	};

	sif->status = START_STATUS_NOT_START;

	prefetch.b_rtpipe = 1;
	prefetch.b_soft_rtpipe = 1;
	prefetch.b_add_gap_for_yuv = 1;

	if (isp_send_fw_cmd(isp, CMD_ID_ENABLE_PREFETCH,
			    FW_CMD_RESP_STREAM_ID_GLOBAL,
			    FW_CMD_PARA_TYPE_DIRECT,
			    &prefetch, sizeof(prefetch)) != RET_SUCCESS) {
		ISP_PR_WARN("failed to config prefetch\n");
	} else {
		ISP_PR_DBG("config prefetch %d:%d suc\n",
			   prefetch.b_rtpipe,
			   prefetch.b_soft_rtpipe);
	}

	if (!sif->start_str_cmd_sent && sif->channel_buf_sent_cnt >=
	    MIN_CHANNEL_BUF_CNT_BEFORE_START_STREAM) {
		if (isp_send_fw_cmd(isp, CMD_ID_START_STREAM, fw_stream_id,
				    FW_CMD_PARA_TYPE_DIRECT, NULL, 0)
		    != RET_SUCCESS) {
			ISP_PR_ERR("<-%s fail for START_STREAM\n", __func__);
			return RET_FAILURE;
		}
		sif->start_str_cmd_sent = 1;
	} else {
		ISP_PR_DBG
		("%s no send START_STREAM, start_sent %u, buf_sent %u\n",
		 __func__, sif->start_str_cmd_sent,
		 sif->channel_buf_sent_cnt);
	}

	return RET_SUCCESS;
}

bool isp_get_str_out_prop(struct isp_sensor_info *sen_info,
			  struct isp_stream_info *str_info,
			  struct image_prop_t *out_prop)
{
	uint32 width = 0;
	uint32 height = 0;
	bool ret = true;

	if (!sen_info || !str_info || !out_prop) {
		ISP_PR_ERR
		("-><- %s, fail by param, seninfo %p, strinfo %p, outprop %p\n",
		 __func__, sen_info, str_info, out_prop);
		return false;
	}

	width = str_info->width;
	height = str_info->height;

	switch (str_info->format) {
	case PVT_IMG_FMT_NV12:
		out_prop->image_format = IMAGE_FORMAT_NV12;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->luma_pitch = str_info->luma_pitch_set;
		out_prop->chroma_pitch = out_prop->luma_pitch;
		break;
	case PVT_IMG_FMT_P010:
		/* Windows pass pitch in bytes, while AMD ISP expect the pitch */
		/* in pixels */
		/* For 10bit mode, 2 bytes / pixel, pitch should divided by 2 */
		out_prop->image_format = IMAGE_FORMAT_P010;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->luma_pitch = str_info->luma_pitch_set;
		out_prop->chroma_pitch = out_prop->luma_pitch;
		break;
	case PVT_IMG_FMT_L8:
		out_prop->image_format = IMAGE_FORMAT_NV12;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->luma_pitch = str_info->luma_pitch_set;
		out_prop->chroma_pitch = str_info->luma_pitch_set;
		break;
	case PVT_IMG_FMT_NV21:
		out_prop->image_format = IMAGE_FORMAT_NV21;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->luma_pitch = str_info->luma_pitch_set;
		out_prop->chroma_pitch = str_info->chroma_pitch_set;
		break;
	case PVT_IMG_FMT_YV12:
		out_prop->image_format = IMAGE_FORMAT_YV12;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->luma_pitch = str_info->luma_pitch_set;
		out_prop->chroma_pitch = str_info->chroma_pitch_set;
		break;
	case PVT_IMG_FMT_I420:
		out_prop->image_format = IMAGE_FORMAT_I420;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->luma_pitch = str_info->luma_pitch_set;
		out_prop->chroma_pitch = str_info->chroma_pitch_set;
		break;
	case PVT_IMG_FMT_YUV422P:
		out_prop->image_format = IMAGE_FORMAT_YUV422PLANAR;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->luma_pitch = str_info->luma_pitch_set;
		out_prop->chroma_pitch = str_info->chroma_pitch_set;
		break;
	case PVT_IMG_FMT_YUV422_SEMIPLANAR:
		out_prop->image_format = IMAGE_FORMAT_YUV422SEMIPLANAR;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->luma_pitch = str_info->luma_pitch_set;
		out_prop->chroma_pitch = str_info->chroma_pitch_set;
		break;
	case PVT_IMG_FMT_YUV422_INTERLEAVED:
		out_prop->image_format = IMAGE_FORMAT_YUV422INTERLEAVED;
		out_prop->width = width;
		out_prop->height = height;
		out_prop->luma_pitch = str_info->luma_pitch_set;
		out_prop->chroma_pitch = str_info->chroma_pitch_set;
		break;
	default:
		ISP_PR_ERR("-><- %s fail by picture color format:%d\n",
			   __func__, str_info->format);
		ret = false;
		break;
	}

	return ret;
}

result_t isp_setup_output(struct isp_context *isp, enum camera_port_id cid,
			  enum stream_id stream_id)
{
	enum fw_cmd_resp_stream_id fw_stream_id;
	struct cmd_set_out_ch_prop cmd_ch_prop = {0};
	struct cmd_set_out_ch_frame_rate_ratio cmd_ch_ratio = {0};
	struct cmd_enable_out_ch cmd_ch_en = {0};
	struct isp_stream_info *sif;
	struct isp_sensor_info *sen_info = NULL;
	struct image_prop_t *out_prop;

	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR
		("-><- %s fail, bad para,cid:%d,str:%d\n",
		 __func__, cid, stream_id);
		return RET_FAILURE;
	}

	fw_stream_id = isp_get_fwresp_stream_id(isp, cid, stream_id);
	sen_info = &isp->sensor_info[cid];
	sif = &isp->sensor_info[cid].str_info[stream_id];
	ISP_PR_DBG("-> %s cid:%d,str:%d\n", __func__, cid, stream_id);

	if (sif->start_status == START_STATUS_STARTED) {
		ISP_PR_DBG("<- %s,suc do none\n", __func__);
		return RET_SUCCESS;
	}

	if (sif->start_status == START_STATUS_START_FAIL) {
		ISP_PR_DBG("<- %s,fail do none\n", __func__);
		return RET_SUCCESS;
	}

	sif->start_status = START_STATUS_STARTING;
	out_prop = &cmd_ch_prop.image_prop;
	if (stream_id == STREAM_ID_PREVIEW) {
		cmd_ch_prop.ch = ISP_PIPE_OUT_CH_PREVIEW;
		cmd_ch_ratio.ch = ISP_PIPE_OUT_CH_PREVIEW;
		cmd_ch_en.ch = ISP_PIPE_OUT_CH_PREVIEW;
		cmd_ch_ratio.ratio = 1;
	} else if (stream_id == STREAM_ID_VIDEO) {
		cmd_ch_prop.ch = ISP_PIPE_OUT_CH_VIDEO;
		cmd_ch_ratio.ch = ISP_PIPE_OUT_CH_VIDEO;
		cmd_ch_en.ch = ISP_PIPE_OUT_CH_VIDEO;
		cmd_ch_ratio.ratio = 1;
	} else if (stream_id == STREAM_ID_ZSL) {
		cmd_ch_prop.ch = ISP_PIPE_OUT_CH_STILL;
		cmd_ch_ratio.ch = ISP_PIPE_OUT_CH_STILL;
		cmd_ch_en.ch = ISP_PIPE_OUT_CH_STILL;
		cmd_ch_ratio.ratio = 1;
	} else {
		/* now stream_id must be STREAM_ID_RAW */
		enum isp_pipe_out_ch_t ch = ISP_PIPE_OUT_CH_MIPI_RAW;

		cmd_ch_prop.ch = ch;
		cmd_ch_ratio.ch = ch;
		cmd_ch_en.ch = ch;
		cmd_ch_ratio.ratio = 1;
	}
	cmd_ch_en.is_enable = true;
	if (!isp_get_str_out_prop(sen_info, sif, out_prop)) {
		ISP_PR_ERR("<- %s fail,get out prop\n", __func__);
		return RET_FAILURE;
	}

	ISP_PR_DBG("%s,cid %d, stream %d\n", __func__,
		   cid, fw_stream_id);

	ISP_PR_DBG("in %s,channel:%s,fmt %s,w:h=%u:%u,lp:%u,cp%u\n",
		   __func__,
		   isp_dbg_get_out_ch_str(cmd_ch_prop.ch),
		   isp_dbg_get_out_fmt_str(cmd_ch_prop.image_prop.image_format),
		   cmd_ch_prop.image_prop.width, cmd_ch_prop.image_prop.height,
		   cmd_ch_prop.image_prop.luma_pitch,
		   cmd_ch_prop.image_prop.chroma_pitch);

	if (isp_send_fw_cmd(isp, CMD_ID_SET_OUT_CHAN_PROP,
			    fw_stream_id,
			    FW_CMD_PARA_TYPE_DIRECT,
			    &cmd_ch_prop,
			    sizeof(cmd_ch_prop)) != RET_SUCCESS) {
		sif->start_status = START_STATUS_START_FAIL;
		ISP_PR_ERR
		("<- %s fail,set out prop\n", __func__);
		return RET_FAILURE;
	};

	if (isp_send_fw_cmd(isp, CMD_ID_ENABLE_OUT_CHAN,
			    fw_stream_id,
			    FW_CMD_PARA_TYPE_DIRECT,
			    &cmd_ch_en, sizeof(cmd_ch_en)) != RET_SUCCESS) {
		sif->start_status = START_STATUS_START_FAIL;
		ISP_PR_ERR("<- %s,enable fail\n", __func__);
		return RET_FAILURE;
	}

	ISP_PR_DBG("%s,enable channel %s\n",
		   __func__, isp_dbg_get_out_ch_str(cmd_ch_en.ch));

	if (!sen_info->start_str_cmd_sent) {
		if (isp_kickoff_stream(isp, cid, fw_stream_id,
				       out_prop->width,
				       out_prop->height) != RET_SUCCESS) {
			ISP_PR_ERR("%s, kickoff stream fail\n",
				   __func__);
			return RET_FAILURE;
		}

		if (sen_info->start_str_cmd_sent) {
			sen_info->status = START_STATUS_STARTED;
			sif->start_status = START_STATUS_STARTED;
			ISP_PR_DBG("kickoff stream suc:STARTED\n");
		} else {
			ISP_PR_DBG("kickoff stream suc\n");
		}
	} else {
		ISP_PR_DBG("%s,stream running, no need kickoff\n", __func__);
		sif->start_status = START_STATUS_STARTED;
	}

	ISP_PR_DBG("<- %s,suc\n", __func__);
	return RET_SUCCESS;
}

/* start stream for cam_id, return 0 for success others for fail */
enum imf_ret_value start_stream_imp(void *context,
				    enum camera_port_id cam_id,
				    enum stream_id stream_id)
{
	struct isp_context *isp = context;
	result_t ret;
	enum pvt_img_fmt fmt;
	int32 ret_val = IMF_RET_SUCCESS;
	enum fw_cmd_resp_stream_id fw_stream_id;
	struct isp_stream_info *sif = NULL;
	struct isp_sensor_info *snrif = NULL;

	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("-><- %s fail bad para,isp:%p,cid:%d,str:%d\n",
			   __func__, context, cam_id, stream_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	if (stream_id > STREAM_ID_NUM) {
		ISP_PR_ERR("-><- %s fail bad para, invalid stream_id:%d\n",
			   __func__, stream_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	fmt = isp->sensor_info[cam_id].str_info[stream_id].format;
	if (fmt == PVT_IMG_FMT_INVALID || fmt >= PVT_IMG_FMT_MAX) {
		ISP_PR_ERR("-><- %s fail,cid:%d,str:%d,fmt not set\n",
			   __func__, cam_id, stream_id);
		return IMF_RET_FAIL;
	}

	isp_mutex_lock(&isp->ops_mutex);
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		isp_mutex_unlock(&isp->ops_mutex);
		ISP_PR_ERR("-><- %s(cid:%d,str:%d) fail, bad fsm %d\n",
			   __func__, cam_id, stream_id, ISP_GET_STATUS(isp));
		return IMF_RET_FAIL;
	}

	ISP_PR_DBG("-> %s,cid:%d,sid:%d\n", __func__, cam_id, stream_id);
	snrif = &isp->sensor_info[cam_id];
	sif = &isp->sensor_info[cam_id].str_info[stream_id];
	fw_stream_id = isp_get_fwresp_stream_id(isp, cam_id, stream_id);
	if (fw_stream_id < FW_CMD_RESP_STREAM_ID_GLOBAL ||
	    fw_stream_id >= FW_CMD_RESP_STREAM_ID_MAX) {
		ISP_PR_ERR
		("-><- fw_stream_id is illegal value, bad para, fw_stream_id: %d\n",
		fw_stream_id);
		ret = RET_FAILURE;
		goto quit;
	}

	ISP_PR_DBG("-> isp_start_stream cid:%d, str:%d, fw stream id: %d\n",
		   cam_id, stream_id, fw_stream_id);

	if (isp_init_stream(isp, cam_id, fw_stream_id) != RET_SUCCESS) {
		ISP_PR_ERR
		("<- isp_start_stream fail for isp_init_stream\n");
		ret = RET_FAILURE;
		goto quit;
	}

	if (sif->start_status == START_STATUS_NOT_START ||
	    sif->start_status == START_STATUS_STARTING) {
		if (sif->width && sif->height && sif->luma_pitch_set) {
			goto do_out_setup;
		} else {
			sif->start_status = START_STATUS_STARTING;
			ret = RET_SUCCESS;
			ISP_PR_DBG
			("<- isp_start_stream suc,setup out later\n");
			goto quit;
		}
	} else if (sif->start_status == START_STATUS_STARTED) {
		ret = RET_SUCCESS;
		ISP_PR_DBG("<- isp_start_stream suc,do none\n");
		goto quit;
	} else if (sif->start_status == START_STATUS_START_FAIL) {
		ret = RET_FAILURE;
		ISP_PR_ERR("<- isp_start_stream fail,previous fail\n");
		goto quit;
	} else if (sif->start_status == START_STATUS_START_STOPPING) {
		ret = RET_FAILURE;
		ISP_PR_ERR("<- isp_start_stream fail,in stopping\n");
		goto quit;
	} else {
		ret = RET_FAILURE;
		ISP_PR_ERR("<- isp_start_stream fail,bad stat %d\n",
			   sif->start_status);
		goto quit;
	}
do_out_setup:
	if (isp_setup_output(isp, cam_id, stream_id) != RET_SUCCESS) {
		ISP_PR_ERR("<- isp_start_stream fail for setup out\n");
		ret = RET_FAILURE;
	} else {
		ret = RET_SUCCESS;
		ISP_PR_DBG("<- isp_start_stream suc,setup out suc\n");
	}
quit:
	if (is_failure(ret))
		ret_val = IMF_RET_FAIL;
	else
		ret_val = IMF_RET_SUCCESS;

	isp_mutex_unlock(&isp->ops_mutex);
	if (ret_val != IMF_RET_SUCCESS) {
		stop_stream_imp(isp, cam_id, stream_id);
		ISP_PR_ERR("<- %s fail\n", __func__);
	} else {
		ISP_PR_DBG("<- %s suc\n", __func__);
	}

	return ret_val;
}

/* stop stream for cam_id, return 0 for success others for fail */
enum imf_ret_value stop_stream_imp(void *context,
				   enum camera_port_id cid,
				   enum stream_id sid)
{
	struct isp_context *isp = context;
	struct isp_stream_info *sif = NULL;
	int32 ret_val = IMF_RET_SUCCESS;
	struct cmd_enable_out_ch cmd_ch_disable;
	uint32 out_cnt = 0;
	uint32 timeout;
	enum fw_cmd_resp_stream_id fw_stream_id;
	struct isp_mapped_buf_info *cur = NULL;

	if (!is_para_legal(context, cid) || sid > STREAM_ID_NUM) {
		ISP_PR_ERR("-><- %s fail,bad para,isp:%p,cid:%d,sid:%d\n",
			   __func__, context, cid, sid);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);

	fw_stream_id = isp_get_fwresp_stream_id(isp, cid, sid);
	if (fw_stream_id < FW_CMD_RESP_STREAM_ID_GLOBAL ||
	    fw_stream_id >= FW_CMD_RESP_STREAM_ID_MAX) {
		ISP_PR_ERR("-><- %s Invalid fw_stream_id\n", __func__);
		ret_val = IMF_RET_FAIL;
		goto quit;
	}

	sif = &isp->sensor_info[cid].str_info[sid];
	if (!sif) {
		ISP_PR_ERR("-><- %s nullptr failure of sif\n", __func__);
		ret_val = IMF_RET_FAIL;
		goto quit;
	}

	ISP_PR_DBG("-> %s,cid:%d,str:%d,status %i\n",
		   __func__, cid, sid, sif->start_status);

	if (sif->start_status == START_STATUS_NOT_START)
		goto goon;

	switch (sid) {
	case STREAM_ID_PREVIEW:
		cmd_ch_disable.ch = ISP_PIPE_OUT_CH_PREVIEW;
		break;
	case STREAM_ID_VIDEO:
		cmd_ch_disable.ch = ISP_PIPE_OUT_CH_VIDEO;
		break;
	case STREAM_ID_ZSL:
		cmd_ch_disable.ch = ISP_PIPE_OUT_CH_STILL;
		break;
	default:
		ISP_PR_DBG("%s,never here\n", __func__);
		ret_val = IMF_RET_FAIL;
		break;
	}
	if (ret_val != IMF_RET_SUCCESS)
		goto quit;

	cmd_ch_disable.is_enable = false;

	if (sif->start_status != START_STATUS_STARTED)
		goto skip_stop;

	cur = (struct isp_mapped_buf_info *)
	      isp_list_get_first_without_rm(&sif->buf_in_fw);
	timeout = (1000 * 6);
#ifdef DO_SYNCHRONIZED_STOP_STREAM
	if (is_failure(isp_send_fw_cmd_sync(isp,
					    CMD_ID_ENABLE_OUT_CHAN,
					    fw_stream_id, FW_CMD_PARA_TYPE_DIRECT,
					    &cmd_ch_disable, sizeof(cmd_ch_disable),
					    300,
					    NULL, NULL))) {
#else /* <DO_SYNCHRONIZED_STOP_STREAM> */
	if (is_failure(isp_send_fw_cmd(isp,
				       CMD_ID_ENABLE_OUT_CHAN,
				       fw_stream_id, FW_CMD_PARA_TYPE_DIRECT,
				       &cmd_ch_disable,
				       sizeof(cmd_ch_disable)))) {
#endif /* <DO_SYNCHRONIZED_STOP_STREAM> */
		ISP_PR_ERR("%s,send disable str fail\n", __func__);
	} else {
		ISP_PR_DBG("%s wait disable suc\n", __func__);
	}

#ifdef DO_SYNCHRONIZED_STOP_STREAM
	if (isp_send_fw_cmd_sync(isp, CMD_ID_STOP_STREAM, fw_stream_id,
				 FW_CMD_PARA_TYPE_DIRECT,
				 NULL,
				 0,
				 timeout,
				 NULL, NULL) != RET_SUCCESS)
#else
	if (isp_send_fw_cmd(isp, CMD_ID_STOP_STREAM, fw_stream_id,
			    FW_CMD_PARA_TYPE_DIRECT,
			    NULL,
			    0) != RET_SUCCESS)
#endif
	{
		ISP_PR_ERR("in %s,send stop steam fail\n", __func__);
	} else {
		ISP_PR_DBG
		("in %s, wait stop stream suc\n", __func__);
	};

skip_stop:
	isp_take_back_str_buf(isp, sif, cid, sid);
	sif->start_status = START_STATUS_NOT_START;
	isp_reset_str_info(isp, cid, sid);

	ret_val = IMF_RET_SUCCESS;

goon:
	isp_get_stream_output_bits(isp, cid, &out_cnt);
	if (out_cnt > 0) {
		ret_val = IMF_RET_SUCCESS;
		goto quit;
	}

quit:
	if (ret_val != IMF_RET_SUCCESS) {
		ISP_PR_ERR("<- %s fail\n", __func__);
	} else {
		if (out_cnt == 0) {
			isp_uninit_stream(isp, cid, fw_stream_id);
			struct isp_sensor_info *sif;
			/* Poweroff sensor before stop stream as */
			sif = &isp->sensor_info[cid];
			if (cid < CAMERA_PORT_MAX &&
			    sif->cam_type != CAMERA_TYPE_MEM) {
				/* isp_snr_close(isp, cid); */
			} else {
				struct isp_pwr_unit *pwr_unit;

				pwr_unit = &isp->isp_pu_cam[cid];
				isp_mutex_lock(&pwr_unit->pwr_status_mutex);
				pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
				isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
			}
			sif->raw_width = 0;
			sif->raw_height = 0;
		}
		ISP_PR_DBG("<- %s suc\n", __func__);
	}
	isp_mutex_unlock(&isp->ops_mutex);

	return ret_val;
}

void reg_notify_cb_imp(void *context, enum camera_port_id cam_id,
		       func_isp_module_cb cb, void *cb_context)
{
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || !cb) {
		ISP_PR_ERR("cid[%d] fail for bad para", cam_id);
		return;
	}
	isp->evt_cb[cam_id] = cb;
	isp->evt_cb_context[cam_id] = cb_context;
	ISP_PR_DBG("cid[%d] suc", cam_id);
}

void unreg_notify_cb_imp(void *context, enum camera_port_id cam_id)
{
	struct isp_context *isp = context;

	ISP_PR_DBG("cid %u", cam_id);
	if (!is_para_legal(context, cam_id)) {
		ISP_PR_ERR("cid[%d] fail for bad para", cam_id);
		return;
	}
	isp->evt_cb[cam_id] = NULL;
	isp->evt_cb_context[cam_id] = NULL;
	ISP_PR_DBG("cid[%d] suc", cam_id);
}

result_t isp_set_stream_data_fmt(struct isp_context *isp_context,
				 enum camera_port_id cam_id,
				 enum stream_id stream_type,
				 enum pvt_img_fmt img_fmt)
{
	struct isp_stream_info *sif;

	if (!is_para_legal(isp_context, cam_id) ||
	    stream_type > STREAM_ID_NUM) {
		ISP_PR_ERR("-><- %s,fail para,isp%p,cid%d,sid%d\n",
			   __func__, isp_context, cam_id, stream_type);
		return RET_FAILURE;
	}

	if (img_fmt == PVT_IMG_FMT_INVALID || img_fmt >= PVT_IMG_FMT_MAX) {
		ISP_PR_ERR("-><- %s,fail fmt,cid%d,sid%d,fmt%d\n",
			   __func__, cam_id, stream_type, img_fmt);
		return RET_FAILURE;
	}

	sif = &isp_context->sensor_info[cam_id].str_info[stream_type];

	if (sif->start_status == START_STATUS_NOT_START) {
		sif->format = img_fmt;
		ISP_PR_DBG("-><- %s suc,cid %d,str %d,fmt %s\n",
			   __func__, cam_id, stream_type,
			   isp_dbg_get_pvt_fmt_str(img_fmt));
		return RET_SUCCESS;
	}

	if (sif->format == img_fmt) {
		ISP_PR_DBG("-><- %s suc,cid%d,str%d,fmt%s,do none\n",
			   __func__, cam_id, stream_type,
			   isp_dbg_get_pvt_fmt_str(img_fmt));
		sif->format = img_fmt;
		return RET_SUCCESS;
	}
	ISP_PR_ERR("-><- %s fail,cid%d,str%d,fmt%s,bad stat%d\n",
		   __func__, cam_id, stream_type,
		   isp_dbg_get_pvt_fmt_str(img_fmt),
		   sif->start_status);
	return RET_FAILURE;
}

result_t isp_set_str_res_fps_pitch(struct isp_context *isp_context,
				   enum camera_port_id cam_id,
				   enum stream_id stream_type,
				   struct pvt_img_res_fps_pitch *value)
{
	uint32 width;
	uint32 height;
	uint32 fps;
	uint32 luma_pitch;
	uint32 chroma_pitch;
	struct isp_stream_info *sif;
	result_t ret = RET_SUCCESS;

	if (!isp_context || cam_id >= CAMERA_PORT_MAX ||
	    stream_type > STREAM_ID_NUM || !value) {
		ISP_PR_ERR("-><- %s,fail para,isp %p,cid %d,sid %d\n",
			   __func__, isp_context, cam_id, stream_type);
		return RET_FAILURE;
	}

	width = value->width;
	height = value->height;
	fps = value->fps;
	luma_pitch = abs(value->luma_pitch);
	chroma_pitch = abs(value->chroma_pitch);

	if (width == 0 || height == 0 || luma_pitch == 0) {
		ISP_PR_ERR
		("-><- %s,fail para,cid%d,sid%d,w:h:p %d:%d:%d\n",
		 __func__, cam_id, stream_type, width, height, luma_pitch);
		return RET_FAILURE;
	}

	sif = &isp_context->sensor_info[cam_id].str_info[stream_type];
	ISP_PR_DBG("-> %s,cid%d,sid%d,lp%d,cp%d,w:%d,h:%d,fpsId:%d,strSta %u,chaSta %u\n",
		   __func__, cam_id, stream_type, luma_pitch, chroma_pitch, width, height,
		   fps, isp_context->sensor_info[cam_id].status, sif->start_status);

	if (sif->start_status == START_STATUS_NOT_START) {
		sif->width = width;
		sif->height = height;
		sif->fps = fps;
		sif->luma_pitch_set = luma_pitch;
		sif->chroma_pitch_set = chroma_pitch;
		ret = RET_SUCCESS;
		ISP_PR_DBG("<- %s suc, store\n", __func__);
	} else if (sif->start_status == START_STATUS_STARTING) {
		sif->width = width;
		sif->height = height;
		sif->fps = fps;
		sif->luma_pitch_set = luma_pitch;
		sif->chroma_pitch_set = chroma_pitch;

		ret = isp_setup_output(isp_context, cam_id, stream_type);
		if (ret == RET_SUCCESS) {
			ISP_PR_DBG("<- %s suc aft setup out\n", __func__);
			ret = RET_SUCCESS;
			goto quit;
		} else {
			ISP_PR_ERR("<- %s fail for setup out\n", __func__);
			ret = RET_FAILURE;
			goto quit;
		}
	} else {
		if (sif->width != width ||
		    sif->height != height ||
		    sif->fps != fps ||
		    sif->luma_pitch_set != luma_pitch ||
		    sif->chroma_pitch_set != chroma_pitch) {
			ISP_PR_ERR("<- %s fail for non-consis\n", __func__);
			ret = RET_FAILURE;
			goto quit;
		} else {
			ISP_PR_DBG("<- %s suc, do none\n", __func__);
			ret = RET_SUCCESS;
			goto quit;
		}
	}
quit:

	return ret;
}

enum imf_ret_value set_stream_para_imp(void *context,
				       enum camera_port_id cam_id,
				       enum stream_id stream_id,
				       enum para_id para_type,
				       void *para_value)
{
	result_t ret = RET_SUCCESS;
	int32 func_ret = IMF_RET_SUCCESS;
	struct isp_context *isp = context;

	if (!is_para_legal(context, cam_id) || stream_id > STREAM_ID_NUM) {
		ISP_PR_ERR("-><- %s fail bad para,isp%p,cid%d,sid%d\n",
			   __func__, isp, cam_id, stream_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);
	ISP_PR_DBG("-> %s,cid %d,sid %d,para %s(%d)\n",
		   __func__, cam_id, stream_id,
		   isp_dbg_get_para_str(para_type), para_type);

	switch (para_type) {
	case PARA_ID_DATA_FORMAT: {
		enum pvt_img_fmt data_fmat =
			(*(enum pvt_img_fmt *)para_value);
		ret = isp_set_stream_data_fmt(context, cam_id, stream_id,
					      data_fmat);
		if (is_failure(ret)) {
			ISP_PR_ERR("<- %s(FMT) fail for set fmt:%s\n",
				   __func__,
				   isp_dbg_get_pvt_fmt_str(data_fmat));
			func_ret = IMF_RET_FAIL;
			break;
		}
		ISP_PR_DBG("<- %s(FMT) suc set fmt:%s\n", __func__,
			   isp_dbg_get_pvt_fmt_str(data_fmat));
		break;
	}
	case PARA_ID_DATA_RES_FPS_PITCH: {
		struct pvt_img_res_fps_pitch *data_pitch =
			(struct pvt_img_res_fps_pitch *)para_value;
		ret = isp_set_str_res_fps_pitch(context, cam_id,
						stream_id, data_pitch);
		if (is_failure(ret)) {
			ISP_PR_ERR("<- %s(RES_FPS_PITCH) fail for set\n",
				   __func__);
			func_ret = IMF_RET_FAIL;
			break;
		}
		ISP_PR_DBG("<- %s(RES_FPS_PITCH) suc\n", __func__);
		break;
	}
	default:
		ISP_PR_ERR("<- %s fail for not supported\n", __func__);
		func_ret = IMF_RET_INVALID_PARAMETER;
		break;
	}
	isp_mutex_unlock(&isp->ops_mutex);
	return func_ret;
}

enum imf_ret_value set_stream_buf_imp(void *context, enum camera_port_id cam_id,
				      enum stream_id stream_id,
				      struct sys_img_buf_info *buf_hdl)
{
	struct isp_mapped_buf_info *gen_img = NULL;
	struct isp_context *isp = context;
	int result;
	int ret = IMF_RET_FAIL;
	unsigned int y_len;
	unsigned int u_len;
	unsigned int v_len;

	if (!is_para_legal(context, cam_id) || !buf_hdl ||
	    buf_hdl->planes[0].mc_addr == 0) {
		ISP_PR_ERR("fail bad para, isp[%p] cid[%d] sid[%d]",
			   context, cam_id, stream_id);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp_mutex_lock(&isp->ops_mutex);
	ISP_PR_DBG("cid[%d] sid[%d] %p(%u)",
		   cam_id, stream_id,
		   buf_hdl->planes[0].sys_addr,
		   buf_hdl->planes[0].len);
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		ISP_PR_ERR("fail fsm %d", ISP_GET_STATUS(isp));
		isp_mutex_unlock(&isp->ops_mutex);
		return ret;
	}

	buf_hdl = sys_img_buf_handle_cpy(buf_hdl);
	if (!buf_hdl) {
		ISP_PR_ERR("fail for sys_img_buf_handle_cpy");
		goto quit;
	}

	gen_img = isp_map_sys_2_mc(isp, buf_hdl, ISP_MC_ADDR_ALIGN,
				   cam_id, stream_id, y_len, u_len, v_len);

	/* isp_dbg_show_map_info(gen_img); */
	if (!gen_img) {
		ISP_PR_ERR("fail for isp_map_sys_2_mc");
		ret = IMF_RET_FAIL;
		goto quit;
	}

	result = fw_if_send_img_buf(isp, gen_img, cam_id, stream_id);
	if (result != RET_SUCCESS) {
		ISP_PR_ERR("fail for fw_if_send_img_buf");
		goto quit;
	}

	if (!isp->sensor_info[cam_id].start_str_cmd_sent) {
		isp->sensor_info[cam_id].channel_buf_sent_cnt++;

		if (isp->sensor_info[cam_id].channel_buf_sent_cnt >=
		    MIN_CHANNEL_BUF_CNT_BEFORE_START_STREAM) {
			result = isp_send_fw_cmd(isp, CMD_ID_START_STREAM,
						 isp_get_fwresp_stream_id(isp, cam_id, stream_id),
						 FW_CMD_PARA_TYPE_DIRECT, NULL, 0);

			if (result != RET_SUCCESS) {
				ISP_PR_ERR("<-%s fail to START_STREAM\n",
					   __func__);
				return RET_FAILURE;
			}
			isp->sensor_info[cam_id].start_str_cmd_sent = 1;
			isp->sensor_info[cam_id].str_info[stream_id].start_status =
								START_STATUS_STARTED;
			isp->sensor_info[cam_id].status = START_STATUS_STARTED;
		} else {
			ISP_PR_DBG
			("no send START_STREAM, start_sent %u, buf_sent %u\n",
			isp->sensor_info[cam_id].start_str_cmd_sent,
			isp->sensor_info[cam_id].channel_buf_sent_cnt);
		}
	}

	isp->sensor_info[cam_id].str_info[stream_id].buf_num_sent++;
	isp_list_insert_tail(&isp->sensor_info[cam_id].str_info[stream_id].buf_in_fw,
			     (struct list_node *)gen_img);
	ret = IMF_RET_SUCCESS;

quit:
	isp_mutex_unlock(&isp->ops_mutex);
	if (ret != IMF_RET_SUCCESS) {
		if (buf_hdl)
			sys_img_buf_handle_free(buf_hdl);
		if (gen_img) {
			isp_unmap_sys_2_mc(isp, gen_img);
			isp_sys_mem_free(gen_img);
		}
	}

	RET(ret);
	return ret;
}

enum imf_ret_value set_roi_imp(void *context,
			       enum camera_port_id cam_id,
			       u32 type,
			       struct isp_roi_info *roi)
{
	struct isp_context *isp;
	enum fw_cmd_resp_stream_id fw_stream_id = FW_CMD_RESP_STREAM_ID_MAX;
	uint32 fw_cmd = CMD_ID_SET_3A_ROI;
	uint32 i;
	struct aa_roi *roi_param;

	roi_param = kzalloc(sizeof(*roi_param), GFP_KERNEL);
	if (!is_para_legal(context, cam_id) || !roi || !roi_param) {
		ISP_PR_ERR("-><- %s fail bad para,isp %p,cid %d,roi %p\n",
			   __func__, context, cam_id, roi);
		kfree(roi_param);
		return IMF_RET_INVALID_PARAMETER;
	}

	isp = (struct isp_context *)context;
	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING) {
		ISP_PR_ERR("-><- %s fail fsm %d, cid %u\n",
			   __func__, ISP_GET_STATUS(isp), cam_id);
		kfree(roi_param);
		return IMF_RET_FAIL;
	}

	ISP_PR_DBG("-> %s cid %u type %u(1:AE 2:AWB 4:AF),kind %u(1:Touch 2:Face)\n",
		   __func__, cam_id, type, roi->kind);

	if (type & ISP_3A_TYPE_AF)
		roi_param->roi_type |= ROI_TYPE_MASK_AF;
	if (type & ISP_3A_TYPE_AE)
		roi_param->roi_type |= ROI_TYPE_MASK_AE;
	if (type & ISP_3A_TYPE_AWB)
		roi_param->roi_type |= ROI_TYPE_MASK_AWB;

	if (roi->kind & ISP_ROI_KIND_TOUCH)
		roi_param->mode_mask |= ROI_MODE_MASK_TOUCH;
	if (roi->kind & ISP_ROI_KIND_FACE)
		roi_param->mode_mask |= ROI_MODE_MASK_FACE;

	roi_param->touch_info.touch_num = roi->touch_info.num;
	for (i = 0; i < roi->touch_info.num; i++) {
		struct isp_touch_area_t *des = &roi_param->touch_info.touch_area[i];
		struct isp_touch_area *src = &roi->touch_info.area[i];

		des->points.top_left.x = src->points.top_left.x;
		des->points.top_left.y = src->points.top_left.y;
		des->points.bottom_right.x = src->points.bottom_right.x;
		des->points.bottom_right.y = src->points.bottom_right.y;
		des->touch_weight = src->weight;

		ISP_PR_DBG("touch %u/%u, top(%u:%u),bottom(%u:%u),weight %u\n",
			   i, roi_param->touch_info.touch_num,
			   des->points.top_left.x, des->points.top_left.y,
			   des->points.bottom_right.x,
			   des->points.bottom_right.y, des->touch_weight);
	}

	roi_param->fd_info.is_enabled = roi->fd_info.is_enabled;
	roi_param->fd_info.frame_count = roi->fd_info.frame_count;
	roi_param->fd_info.is_marks_enabled = roi->fd_info.is_marks_enabled;
	roi_param->fd_info.face_num = roi->fd_info.num;

	if (!roi_param->fd_info.frame_count)
		roi_param->fd_info.frame_count = isp->sensor_info[cam_id].poc;

	for (i = 0; i < roi->fd_info.num; i++) {
		struct isp_fd_face_info_t *des = &roi_param->fd_info.face[i];
		struct isp_fd_face_info *src = &roi->fd_info.face[i];

		des->face_id = src->face_id;
		des->score = src->score;
		des->face_area.top_left.x = src->face_area.top_left.x;
		des->face_area.top_left.y = src->face_area.top_left.y;
		des->face_area.bottom_right.x = src->face_area.bottom_right.x;
		des->face_area.bottom_right.y = src->face_area.bottom_right.y;
		des->marks.eye_left.x = src->marks.eye_left.x;
		des->marks.eye_left.y = src->marks.eye_left.y;
		des->marks.eye_right.x = src->marks.eye_right.x;
		des->marks.eye_right.y = src->marks.eye_right.y;
		des->marks.nose.x = src->marks.nose.x;
		des->marks.nose.y = src->marks.nose.y;
		des->marks.mouse_left.x = src->marks.mouse_left.x;
		des->marks.mouse_left.y = src->marks.mouse_left.y;
		des->marks.mouse_right.x = src->marks.mouse_right.x;
		des->marks.mouse_right.y = src->marks.mouse_right.y;

		ISP_PR_DBG
		("face %u/%u,en:%u,top(%u:%u),bottom(%u:%u),score %u,face_id %u",
		 i, roi_param->fd_info.frame_count,
		 roi_param->fd_info.is_marks_enabled,
		 des->face_area.top_left.x, des->face_area.top_left.y,
		 des->face_area.bottom_right.x, des->face_area.bottom_right.y,
		 des->score, des->face_id);

		if (roi_param->fd_info.is_marks_enabled) {
			ISP_PR_DBG
			("marks eye_left(%u:%u) eye_right(%u:%u) nose(%u:%u)\n",
			 des->marks.eye_left.x, des->marks.eye_left.y,
			 des->marks.eye_right.x, des->marks.eye_right.y,
			 des->marks.nose.x, des->marks.nose.y);
			ISP_PR_DBG
			("marks mouse_left(%u:%u) mouse_right(%u:%u)\n",
			 des->marks.mouse_left.x, des->marks.mouse_left.y,
			 des->marks.mouse_right.x, des->marks.mouse_right.y);
		}
	}

	/* Get fw stream id for normal stream */
	fw_stream_id = isp_get_fw_stream_id(isp, cam_id);
	if (fw_stream_id == FW_CMD_RESP_STREAM_ID_MAX) {
		ISP_PR_ERR("<- %s: failed for fw_stream_id:%d\n",
			   __func__, fw_stream_id);
		kfree(roi_param);
		return IMF_RET_FAIL;
	}

	if (is_failure(isp_send_fw_cmd(isp, fw_cmd, fw_stream_id,
				       FW_CMD_PARA_TYPE_INDIRECT,
				       &roi_param, sizeof(roi_param)))) {
		ISP_PR_ERR("<- %s: failed by send cmd for fw_stream_id:%d\n",
			   __func__, fw_stream_id);
		kfree(roi_param);
		return IMF_RET_FAIL;
	}

	ISP_PR_DBG("<- %s: suc for fw_stream_id:%d\n", __func__, fw_stream_id);
	kfree(roi_param);
	return IMF_RET_SUCCESS;
}

int ispm_if_init(struct isp_module_if *intf, struct amd_cam *pamd_cam)
{
	struct isp_context *context;

	if (!intf || !pamd_cam) {
		ISP_PR_ERR("-><- %s fail bad param intf:%p amd_cam:%p\n",
			   __func__, intf, pamd_cam);
		return -EINVAL;
	}

	context = kzalloc(sizeof(*context), GFP_KERNEL);
	if (!context) {
		ISP_PR_ERR("-><- %s fail for context allocation\n", __func__);
		return -ENOMEM;
	};
	memset(intf, 0, sizeof(*intf));
	intf->size = sizeof(*intf);
	intf->version = ISP_MODULE_IF_VERSION_1;

	intf->open_camera = open_camera_imp;
	intf->close_camera = close_camera_imp;
	intf->start_stream = start_stream_imp;
	intf->stop_stream = stop_stream_imp;
	intf->set_stream_buf = set_stream_buf_imp;
	intf->reg_notify_cb = reg_notify_cb_imp;
	intf->unreg_notify_cb = unreg_notify_cb_imp;
	intf->set_stream_para = set_stream_para_imp;
	intf->set_roi = set_roi_imp;

	intf->context = context;
	ispm_context_init(context);
	context->amd_cam = pamd_cam;
	ispm_if_self = intf;
	ISP_PR_DBG("-><- %s context:%p amd_cam:%p\n",
		   __func__, intf->context, context->amd_cam);

	return OK;
}

void ispm_if_fini(struct isp_module_if *intf)
{
	struct isp_context *context;

	if (!intf || !intf->context) {
		ISP_PR_ERR("-><- %s fail bad param intf:%p context:%p\n",
			   __func__, intf, intf->context);
	};

	ispm_if_self = NULL;
	context = (struct isp_context *)intf->context;
	ispm_context_uninit(context);

	kfree(context);
	memset(intf, 0, sizeof(struct isp_module_if));
}

enum imf_ret_value open_camera(enum camera_port_id cam_id,
			       s32 res_fps_id,
			       uint32_t flag)
{
	if (ispm_if_self && ispm_if_self->open_camera)
		return ispm_if_self->open_camera(ispm_if_self->context,
						 cam_id, res_fps_id, flag);
	else
		return IMF_RET_NOT_SUPPORT;
};

enum imf_ret_value close_camera(enum camera_port_id cam_id)
{
	if (ispm_if_self && ispm_if_self->open_camera)
		return ispm_if_self->close_camera(ispm_if_self->context, cam_id);
	else
		return IMF_RET_NOT_SUPPORT;
};

enum imf_ret_value set_stream_buf(enum camera_port_id cam_id,
				  enum stream_id stream_id,
				  struct sys_img_buf_info *buf)
{
	if (ispm_if_self && ispm_if_self->set_stream_buf)
		return ispm_if_self->set_stream_buf(ispm_if_self->context,
						    cam_id, stream_id, buf);
	else
		return IMF_RET_NOT_SUPPORT;
};

enum imf_ret_value set_stream_para(enum camera_port_id cam_id,
				   enum stream_id stream_id,
				   enum para_id para_type,
				   void *para_value)
{
	if (ispm_if_self && ispm_if_self->set_stream_para)
		return ispm_if_self->set_stream_para(ispm_if_self->context,
						     cam_id, stream_id,
						     para_type, para_value);
	else
		return IMF_RET_NOT_SUPPORT;
};

enum imf_ret_value start_stream(enum camera_port_id cam_id,
				enum stream_id stream_id)
{
	if (ispm_if_self && ispm_if_self->start_stream)
		return ispm_if_self->start_stream(ispm_if_self->context, cam_id, stream_id);
	else
		return IMF_RET_NOT_SUPPORT;
};

enum imf_ret_value stop_stream(enum camera_port_id cam_id,
			       enum stream_id stream_id)
{
	if (ispm_if_self && ispm_if_self->stop_stream)
		return ispm_if_self->stop_stream(ispm_if_self->context, cam_id, stream_id);
	else
		return IMF_RET_NOT_SUPPORT;
};

void reg_notify_cb(enum camera_port_id cam_id,
		   func_isp_module_cb cb,
		   void *cb_context)
{
	if (ispm_if_self && ispm_if_self->reg_notify_cb)
		return ispm_if_self->reg_notify_cb(ispm_if_self->context,
						   cam_id, cb, cb_context);
};

void unreg_notify_cb(enum camera_port_id cam_id)
{
	if (ispm_if_self && ispm_if_self->unreg_notify_cb)
		return ispm_if_self->unreg_notify_cb(ispm_if_self->context,
						     cam_id);
};

enum imf_ret_value set_roi(enum camera_port_id cam_id,
			   u32 type,
			   struct isp_roi_info *roi)
{
	if (ispm_if_self && ispm_if_self->set_roi)
		return ispm_if_self->set_roi(ispm_if_self->context, cam_id,
					     type, roi);
	else
		return IMF_RET_NOT_SUPPORT;
};
