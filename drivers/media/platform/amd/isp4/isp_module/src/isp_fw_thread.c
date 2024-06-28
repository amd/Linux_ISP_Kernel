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

#include "amdgpu_ih.h"
#include "amdgpu_drm.h"

#include "log.h"
#include "isp_common.h"
#include "buffer_mgr.h"
#include "isp_mc_addr_mgr.h"
#include "isp_queue.h"
#include "isp_module_if.h"
#include "isp_fw_thread.h"
#include "swisp_if_imp.h"
#include "isp_utils.h"
#include "isp_fw_if.h"
#include "isp_module_if_imp.h"
#include "isp_module_if_imp_inner.h"
#include "isp_fw_cmd_resp.h"

#define WORK_ITEM_INTERVAL  5 /* ms */

static struct isp_fw_resp_thread_para isp_resp_para[MAX_REAL_FW_RESP_STREAM_NUM];

static enum isp_aspect_ratio get_aspect_ratio(u32 width, u32 height)
{
	/*
	 * for 16:9, width * 1000 / height = 1777
	 * middle value of 16:9 and 16:10 are (1777 + 1600) = 1688
	 * for 16:10, width * 1000 / height = 1600
	 * middle value of 16:10 and 4:3 are (1600 + 1333) = 1466
	 * for 4:3, width * 1000 / height = 1333
	 */
	u32 div = width * 1000 / height;

	if (div <= 1466)
		return ISP_ASPECT_RATIO_4_3;
	else if (div <= 1688)
		return ISP_ASPECT_RATIO_16_10;
	else
		return ISP_ASPECT_RATIO_16_9;
}

static void update_output_crop_info(char *desc, struct buffer_meta_info_t *buf_meta)
{
	u32 img_w, img_h;
	u32 crop_w, crop_h;
	u32 crop_new_w, crop_new_h;

	if (!buf_meta) {
		ISP_PR_ERR("buf_meta is NULL\n");
		return;
	}
	if (!buf_meta->enabled) {
		ISP_PR_ERR("buf_meta is not enabled\n");
		return;
	}

	img_w = buf_meta->image_prop.width;
	img_h = buf_meta->image_prop.height;
	crop_w = buf_meta->crop_win_acq.window.h_size;
	crop_h = buf_meta->crop_win_acq.window.v_size;

	if (!img_w || !img_h || !crop_w || !crop_h) {
		ISP_PR_ERR("%s(%s) fail bad w:h,cropw:croph %u:%u, %u:%u\n",
			   __func__, desc ? desc : "", img_w, img_h, crop_w,
			   crop_h);
		return;
	}

	if (get_aspect_ratio(img_w, img_h) == get_aspect_ratio(crop_w, crop_h))
		return;

	crop_new_w = crop_w;
	crop_new_h = img_h * crop_new_w / img_w;
	if (crop_new_h % 2)
		crop_new_h--;
	if (crop_new_h < crop_h) {
		u32 x, y;

		x = buf_meta->crop_win_acq.window.h_offset;
		y = buf_meta->crop_win_acq.window.v_offset;
		buf_meta->crop_win_acq.window.v_offset +=
			(crop_h - crop_new_h) / 2;
		if (buf_meta->crop_win_acq.window.v_offset % 2)
			buf_meta->crop_win_acq.window.v_offset--;
		buf_meta->crop_win_acq.window.h_size = crop_new_w;
		buf_meta->crop_win_acq.window.v_size = crop_new_h;
		ISP_PR_INFO("%s(%s) cropinfo [%u,%u,%u,%u] to [%u,%u,%u,%u]\n",
			    __func__, desc ? desc : "", x, y, crop_w, crop_h,
			    buf_meta->crop_win_acq.window.h_offset,
			    buf_meta->crop_win_acq.window.v_offset, crop_new_w,
			    crop_new_h);
		return;
	}

	crop_new_h = crop_h;
	crop_new_w = img_w * crop_new_h / img_h;
	if (crop_new_w % 2)
		crop_new_w--;
	if (crop_new_w < crop_w) {
		u32 x, y;

		x = buf_meta->crop_win_acq.window.h_offset;
		y = buf_meta->crop_win_acq.window.v_offset;
		buf_meta->crop_win_acq.window.h_offset +=
			(crop_w - crop_new_w) / 2;
		if (buf_meta->crop_win_acq.window.h_offset % 2)
			buf_meta->crop_win_acq.window.h_offset--;
		buf_meta->crop_win_acq.window.h_size = crop_new_w;
		buf_meta->crop_win_acq.window.v_size = crop_new_h;
		ISP_PR_INFO("%s(%s) cropinfo [%u,%u,%u,%u] to [%u,%u,%u,%u]\n",
			    __func__, desc ? desc : "", x, y, crop_w, crop_h,
			    buf_meta->crop_win_acq.window.h_offset,
			    buf_meta->crop_win_acq.window.v_offset, crop_new_w,
			    crop_new_h);
		return;
	}
}

static void update_all_output_crop_info(struct meta_info_t *meta)
{
	if (!meta)
		return;
	update_output_crop_info("prev", &meta->preview);
	update_output_crop_info("video", &meta->video);
}

static struct isp_mapped_buf_info *
isp_preview_done(struct isp_context *isp, enum camera_port_id cid,
		 struct meta_info_t *meta, struct frame_done_cb_para *pcb)
{
	struct isp_mapped_buf_info *prev = NULL;

	if (!isp || cid >= CAMERA_PORT_MAX || !meta || !pcb) {
		ISP_PR_ERR
		("-><- %s,fail bad param, isp %p, cid %u, meta %p, pcb %p\n",
		 __func__, isp, cid, meta, pcb);
		return prev;
	}

	pcb->preview.status = BUF_DONE_STATUS_ABSENT;
	if (meta->preview.enabled &&
	    (meta->preview.status == BUFFER_STATUS_SKIPPED ||
	     meta->preview.status == BUFFER_STATUS_DONE ||
	     meta->preview.status == BUFFER_STATUS_DIRTY)) {
		struct isp_stream_info *str_info;

		str_info = &isp->sensor_info[cid].str_info[STREAM_ID_PREVIEW];
		prev = (struct isp_mapped_buf_info *)isp_list_get_first
		       (&str_info->buf_in_fw);

		if (!prev) {
			ISP_PR_ERR("%s,fail null prev\n", __func__);
		} else if (!prev->sys_img_buf_hdl) {
			ISP_PR_ERR("%s,fail null prev orig\n", __func__);
		} else {
			pcb->preview.buf = *prev->sys_img_buf_hdl;

			pcb->preview.status = BUF_DONE_STATUS_SUCCESS;
			{
				u64 mc_exp = prev->y_map_info.mc_addr;
				u64 mc_real = isp_join_addr64
					      (meta->preview.buffer.buf_base_a_lo,
					       meta->preview.buffer.buf_base_a_hi);

				if (mc_exp != mc_real) {
					ISP_PR_ERR
					("disorder:0x%llx expt 0x%llx recv\n",
					 mc_exp, mc_real);
				}
			}
		}
	} else if (meta->preview.enabled) {
		ISP_PR_ERR("%s,fail bad preview status %u(%s)\n", __func__,
			   meta->preview.status,
			   isp_dbg_get_buf_done_str(meta->preview.status));
	}

	if (prev)
		isp_unmap_sys_2_mc(isp, prev);

	return prev;
}

static struct isp_mapped_buf_info *
isp_video_done(struct isp_context *isp, enum camera_port_id cid,
	       struct meta_info_t *meta, struct frame_done_cb_para *pcb)
{
	struct isp_mapped_buf_info *video = NULL;

	if (!isp || cid >= CAMERA_PORT_MAX || !meta || !pcb) {
		ISP_PR_ERR
		("-><- %s,fail bad param, isp %p, cid %u, meta %p, pcb %p\n",
		 __func__, isp, cid, meta, pcb);
		return video;
	}
	pcb->video.status = BUF_DONE_STATUS_ABSENT;
	if (meta->video.enabled &&
	    (meta->video.status == BUFFER_STATUS_SKIPPED ||
	     meta->video.status == BUFFER_STATUS_DONE ||
	     meta->video.status == BUFFER_STATUS_DIRTY)) {
		video = (struct isp_mapped_buf_info *)isp_list_get_first
			(&isp->sensor_info[cid]
			 .str_info[STREAM_ID_VIDEO]
			 .buf_in_fw);

		if (!video) {
			ISP_PR_ERR("%s,fail null video\n", __func__);
		} else if (!video->sys_img_buf_hdl) {
			ISP_PR_ERR("%s,fail null video orig\n", __func__);
		} else {
			pcb->video.buf = *video->sys_img_buf_hdl;
			pcb->video.status = BUF_DONE_STATUS_SUCCESS;
		}
	} else if (meta->video.enabled) {
		ISP_PR_ERR("%s,fail bad video status %u(%s)\n", __func__,
			   meta->video.status,
			   isp_dbg_get_buf_done_str(meta->video.status));
	}
	if (video)
		isp_unmap_sys_2_mc(isp, video);

	return video;
}

static struct isp_mapped_buf_info *isp_zsl_done(struct isp_context *isp,
						enum camera_port_id cid,
						struct meta_info_t *meta,
						struct frame_done_cb_para *pcb)
{
	struct isp_mapped_buf_info *zsl = NULL;
	struct isp_sensor_info *sif;
	enum buffer_source_t orig_src = BUFFER_SOURCE_INVALID;

	if (!isp || cid >= CAMERA_PORT_MAX || !meta || !pcb) {
		ISP_PR_ERR
		("-><- %s,fail bad param, isp %p, cid %u, meta %p, pcb %p\n",
		 __func__, isp, cid, meta, pcb);
		return zsl;
	}
	sif = &isp->sensor_info[cid];
	pcb->zsl.status = BUF_DONE_STATUS_ABSENT;
	if (meta)
		orig_src = meta->still.source;

	if (meta->still.enabled &&
	    (meta->still.status == BUFFER_STATUS_SKIPPED ||
	     meta->still.status == BUFFER_STATUS_DONE ||
	     meta->still.status == BUFFER_STATUS_DIRTY)) {
		char *src = "";
		u32 cnt = 0;

		if (meta->still.enabled) {
			src = "zsl";
			zsl = (struct isp_mapped_buf_info *)isp_list_get_first
			      (&sif->str_info[STREAM_ID_ZSL].buf_in_fw);
			cnt = sif->str_info[STREAM_ID_ZSL].buf_in_fw.count;
		} else {
			ISP_PR_ERR
			("in %s,fail here,enable %u,status %d,src %d\n",
			 __func__, meta->still.enabled,
			 meta->still.status, orig_src);
		}

		if (!zsl) {
			ISP_PR_ERR("%s,fail null %s\n", __func__, src);
		} else if (!zsl->sys_img_buf_hdl) {
			ISP_PR_ERR("%s,fail null %s orig\n", __func__, src);
		} else {
			pcb->zsl.buf = *zsl->sys_img_buf_hdl;

			pcb->zsl.status = BUF_DONE_STATUS_SUCCESS;
		}
	} else if (meta->still.enabled) {
		ISP_PR_ERR("%s,fail bad still status %u(%s)\n", __func__,
			   meta->still.status,
			   isp_dbg_get_buf_done_str(meta->still.status));
	}

	if (zsl)
		isp_unmap_sys_2_mc(isp, zsl);

	if (meta)
		meta->still.source = orig_src;
	return zsl;
}

static void *isp_metainfo_get_sys_from_mc(struct isp_context *isp,
					  enum fw_cmd_resp_stream_id fw_stream_id,
					  u64 mc)
{
	u32 i;

	if (!isp || !mc || fw_stream_id >= FW_CMD_RESP_STREAM_ID_MAX) {
		ISP_PR_ERR
		("-><- %s, fail bad param, isp %p, mc 0x%llx, fw_stream_id %u\n",
		 __func__, isp, mc, fw_stream_id);
		return NULL;
	};

	for (i = 0; i < STREAM_META_BUF_COUNT; i++) {
		if (isp->fw_cmd_resp_strs_info[fw_stream_id].meta_info_buf[i]) {
			if (mc == isp->fw_cmd_resp_strs_info[fw_stream_id]
			    .meta_info_buf[i]->gpu_mc_addr)
				return isp->fw_cmd_resp_strs_info[fw_stream_id]
				       .meta_info_buf[i]->sys_addr;
		}
	}
	return NULL;
};

static enum camera_port_id
isp_get_cid_from_stream_id(struct isp_context *isp,
			   enum fw_cmd_resp_stream_id fw_stream_id)
{
	enum camera_port_id searched_cid;

	searched_cid = isp->fw_cmd_resp_strs_info[fw_stream_id].cid_owner;

	ISP_PR_DBG("%s get cid:%d for fw_stream_id:%d\n", __func__, searched_cid, fw_stream_id);

	return searched_cid;
}

static void resend_meta_in_framedone(struct isp_context *isp,
				     enum camera_port_id cid,
				     enum fw_cmd_resp_stream_id fw_stream_id,
				     u64 meta_info_mc, u64 meta_data_mc)
{
	struct cmd_send_buffer buf_type;

	if (!isp || cid >= CAMERA_PORT_MAX) {
		ISP_PR_ERR("-><- %s,fail bad param, isp %p, cid %u\n", __func__,
			   isp, cid);
		return;
	}

	if (isp->sensor_info[cid].status != START_STATUS_STARTED &&
	    isp->sensor_info[cid].status != START_STATUS_STARTING) {
		if (meta_info_mc)
			isp_fw_ret_indirect_cmd_pl
			(&isp->fw_indirect_cmd_pl_buf_mgr,
			 meta_info_mc);
		ISP_PR_WARN
		("not working status %i, meta_info 0x%llx, metaData 0x%llx\n",
		 isp->sensor_info[cid].status, meta_info_mc, meta_data_mc);
		return;
	}

	if (meta_info_mc) {
		memset(&buf_type, 0, sizeof(buf_type));
		buf_type.buffer_type = BUFFER_TYPE_META_INFO;
		buf_type.buffer.buf_tags = 0;
		buf_type.buffer.vmid_space.bit.vmid = 0;
		buf_type.buffer.vmid_space.bit.space = ADDR_SPACE_TYPE_GPU_VA;
		isp_split_addr64(meta_info_mc, &buf_type.buffer.buf_base_a_lo,
				 &buf_type.buffer.buf_base_a_hi);
		buf_type.buffer.buf_size_a = META_INFO_BUF_SIZE;
		if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, fw_stream_id,
				    FW_CMD_PARA_TYPE_DIRECT, &buf_type,
				    sizeof(buf_type)) != RET_SUCCESS) {
			ISP_PR_ERR("%s(%u) fail send meta_info 0x%llx\n",
				   __func__, cid, meta_info_mc);
			isp_fw_ret_indirect_cmd_pl
			(&isp->fw_indirect_cmd_pl_buf_mgr,
			 meta_info_mc);
		} else {
			ISP_PR_INFO("%s(%u), resend meta_info 0x%llx\n",
				    __func__, cid, meta_info_mc);
		}
	}

	if (meta_data_mc) {
		memset(&buf_type, 0, sizeof(buf_type));
		buf_type.buffer_type = BUFFER_TYPE_META_DATA;
		buf_type.buffer.buf_tags = 0;
		buf_type.buffer.vmid_space.bit.vmid = 0;
		buf_type.buffer.vmid_space.bit.space = ADDR_SPACE_TYPE_GPU_VA;
		isp_split_addr64(meta_data_mc, &buf_type.buffer.buf_base_a_lo,
				 &buf_type.buffer.buf_base_a_hi);
		buf_type.buffer.buf_size_a = META_DATA_BUF_SIZE;
		if (isp_send_fw_cmd(isp, CMD_ID_SEND_BUFFER, fw_stream_id,
				    FW_CMD_PARA_TYPE_DIRECT, &buf_type,
				    sizeof(buf_type)) != RET_SUCCESS) {
			ISP_PR_ERR("%s(%u) fail send metaData 0x%llx\n",
				   __func__, cid, meta_data_mc);
			isp_fw_ret_indirect_cmd_pl(&isp->fw_indirect_cmd_pl_buf_mgr, meta_data_mc);
		} else {
			ISP_PR_INFO("%s(%u), resend metaData 0x%llx\n",
				    __func__, cid, meta_data_mc);
		}
	}
}

void isp_fw_resp_cmd_done_extra(struct isp_context *isp,
				enum camera_port_id cid, struct resp_cmd_done *para,
				struct isp_cmd_element *ele)
{
	u32 major;
	u32 minor;
	u32 rev;
	u32 ver;
	u8 *payload;

	if (!isp || !para || !ele) {
		ISP_PR_ERR("-><- null pointer\n");
		return;
	}

	payload = para->payload;
	if (!payload) {
		ISP_PR_ERR("-><- struct resp_cmd_done payload null pointer\n");
		return;
	}

	switch (para->cmd_id) {
	case CMD_ID_GET_FW_VERSION:
		ver = *((u32 *)payload);
		major = (ver & FW_VERSION_MAJOR_MASK) >> FW_VERSION_MAJOR_SHIFT;
		minor = (ver & FW_VERSION_MINOR_MASK) >> FW_VERSION_MINOR_SHIFT;
		rev = (ver & FW_VERSION_BUILD_MASK) >> FW_VERSION_BUILD_SHIFT;
		isp->isp_fw_ver = ver;
		ISP_PR_INFO("fw version,maj:min:rev:sub %u:%u:%u\n", major,
			    minor, rev);
		if (major != FW_VERSION_MAJOR) {
			ISP_PR_ERR("fw major mismatch, expect %u\n",
				   FW_VERSION_MAJOR);
		} else if (minor != FW_VERSION_MINOR ||
			   rev != FW_VERSION_BUILD) {
			ISP_PR_WARN("fw minor mismatch, expect %u:%u\n",
				    FW_VERSION_MINOR, FW_VERSION_BUILD);
		}
		break;
	case CMD_ID_START_STREAM:
		break;
	case CMD_ID_SET_3A_ROI:
		ISP_PR_INFO("%s cmd_done (0x%x) for cid:%d\n", __func__,
			    para->cmd_id, cid);
		struct cmd_done_cb_para cmd_cd_param;

		memset(&cmd_cd_param, 0, sizeof(struct cmd_done_cb_para));
		cmd_cd_param.cam_id = cid;
		cmd_cd_param.cmd_id = para->cmd_id;
		cmd_cd_param.cmd_status = para->cmd_status;
		cmd_cd_param.cmd_seqnum = para->cmd_seq_num;
		cmd_cd_param.cmd_payload = *((u32 *)payload);
		isp->evt_cb[cid](isp->evt_cb_context[cid], CB_EVT_ID_CMD_DONE,
				 &cmd_cd_param);
		break;
	default:
		break;
	}
}

void isp_fw_resp_cmd_skip_extra(struct isp_context *isp,
				enum camera_port_id cid, struct resp_cmd_done *para,
				struct isp_cmd_element *ele)
{
	u8 *payload;
	struct isp_sensor_info *sif;

	if (!isp || !para || !ele) {
		ISP_PR_ERR("%s, null pointer\n", __func__);
		return;
	}

	payload = para->payload;
	if (!payload) {
		ISP_PR_ERR("%s, struct resp_cmd_done payload null pointer\n",
			   __func__);
		return;
	}

	sif = &isp->sensor_info[cid];
}

result_t isp_get_timestamp(struct isp_context *isp, enum camera_port_id cid,
			   u64 timestamp_fw, int64 *timestamp_sw)
{
#define ISP_TIMESTAMP_COUNTER_FREQUENCY 24000000 /* 24MHZ */
#define ISP_TIMESTAMP_COUNTER_BITWIDTH 64
	int64 timestamp_sw_base = 0;
	u64 timestamp_fw_base = 0;

	if (!is_para_legal(isp, cid)) {
		ISP_PR_ERR("%s fail for para, id %d\n", __func__, cid);
		goto fail;
	}

	timestamp_sw_base = isp->timestamp_sw_base;
	timestamp_fw_base = isp->timestamp_fw_base;
	if (timestamp_sw_base != 0 && timestamp_fw_base != 0) {
		if (timestamp_fw >= timestamp_fw_base) {
			/*
			 * keep all value under 100-ns unit.
			 * The register bitwidth to track FW timestamp is 64bit
			 */
			*timestamp_sw = timestamp_sw_base +
					(timestamp_fw - timestamp_fw_base) *
					NANOSECONDS /
					ISP_TIMESTAMP_COUNTER_FREQUENCY;
		} else {
			/* wrap around, keep here for other projects */
			/* if register bitwidth is not enough */
			*timestamp_sw = 0;

			ISP_PR_ERR
			("%s invalid timestamp, fw:%llx, fw_base:%llx\n",
			 __func__, timestamp_fw, timestamp_fw_base);
			goto fail;
		}
	} else {
		ISP_PR_ERR("%s invalid base timestamp, sw:%llx, fw:%llx\n",
			   __func__, timestamp_sw_base, timestamp_fw_base);
		goto fail;
	}

	ISP_PR_DBG("cid:%d, timestamp correlated from FW=%llx to SW=%llx",
		   cid, timestamp_fw, *timestamp_sw);
	return RET_SUCCESS;

fail:
	return RET_FAILURE;
}

void isp_fw_resp_cmd_done(struct isp_context *isp,
			  enum fw_cmd_resp_stream_id fw_stream_id,
			  struct resp_cmd_done *para)
{
	struct isp_cmd_element *ele;
	enum camera_port_id cid;

	cid = isp_get_cid_from_stream_id(isp, fw_stream_id);
	ele = isp_rm_cmd_from_cmdq(isp, para->cmd_seq_num, para->cmd_id, false);

	if (!ele) {
		ISP_PR_ERR("-><- stream %d,cmd %s(0x%08x)(%d),seq %u,no orig\n", fw_stream_id,
			   isp_dbg_get_cmd_str(para->cmd_id),
			   para->cmd_id, para->cmd_status, para->cmd_seq_num);
	} else {
		if (ele->resp_payload && ele->resp_payload_len) {
			*ele->resp_payload_len = min(*ele->resp_payload_len, 36);
			memcpy(ele->resp_payload, para->payload,
			       *ele->resp_payload_len);
		}

		ISP_PR_INFO("-><- cid %u, stream %d,cmd %s(0x%08x)(%d),seq %u\n",
			    cid, fw_stream_id,
			    isp_dbg_get_cmd_str(para->cmd_id),
			    para->cmd_id, para->cmd_status, para->cmd_seq_num);

		if (para->cmd_status == 0) {
			isp_fw_resp_cmd_done_extra(isp, cid, para, ele);
		} else if (para->cmd_status == 2) { /* process the skipped cmd */
			isp_fw_resp_cmd_skip_extra(isp, cid, para, ele);
		}
		if (ele->evt) {
			ISP_PR_INFO("signal event %p\n", ele->evt);
			isp_event_signal(para->cmd_status, ele->evt);
		}
		if (cid >= CAMERA_PORT_MAX) {
			if (fw_stream_id != FW_CMD_RESP_STREAM_ID_GLOBAL)
				ISP_PR_ERR("fail cid %d, sid %u\n", cid,
					   fw_stream_id);
		}

		if (ele->mc_addr)
			isp_fw_ret_indirect_cmd_pl(&isp->fw_indirect_cmd_pl_buf_mgr,
						   ele->mc_addr);

		isp_sys_mem_free(ele);
	}
}

void isp_fw_resp_frame_done(struct isp_context *isp,
			    enum fw_cmd_resp_stream_id fw_stream_id,
			    struct resp_param_package_t *para)
{
	struct meta_info_t *meta;
	u64 mc = 0;
	u64 meta_data_mc = 0;
	enum camera_port_id cid;
	struct isp_mapped_buf_info *prev = NULL;
	struct isp_mapped_buf_info *video = NULL;
	struct isp_mapped_buf_info *zsl = NULL;
	struct frame_done_cb_para *pcb = NULL;
	struct isp_sensor_info *sif;

	cid = isp_get_cid_from_stream_id(isp, fw_stream_id);
	if (cid >= CAMERA_PORT_MAX || cid < CAMERA_PORT_0) {
		ISP_PR_ERR("<- %s,fail,bad cid,streamid %d\n", __func__,
			   fw_stream_id);
		return;
	}

	sif = &isp->sensor_info[cid];
	mc = isp_join_addr64(para->package_addr_lo, para->package_addr_hi);
	meta = (struct meta_info_t *)isp_metainfo_get_sys_from_mc(isp, fw_stream_id, mc);
	if (mc == 0 || !meta) {
		ISP_PR_ERR("<- %s,fail,bad mc,streamid %d,mc %p\n", __func__,
			   fw_stream_id, meta);
		return;
	}
	sif->poc = meta->poc;
	pcb = (struct frame_done_cb_para *)isp_sys_mem_alloc
	      (sizeof(struct frame_done_cb_para));
	if (pcb) {
		memset(pcb, 0, sizeof(struct frame_done_cb_para));
	} else {
		ISP_PR_ERR("<- %s,cid %u,streamid %d, alloc pcb fail\n",
			   __func__, cid, fw_stream_id);
		return;
	}

	pcb->poc = meta->poc;
	pcb->cam_id = cid;
	update_all_output_crop_info(meta);
	memcpy(&pcb->meta_info, meta, sizeof(struct meta_info_t));

	/* IspProcEngDataInFrameDone(isp, cid, fw_stream_id, meta, pcb); */
	/* pcb->scene_mode = sif->scene_mode; */
	if (isp_get_timestamp(isp, cid,
			      ((u64)meta->time_stamp_lo) |
			      (((u64)meta->time_stamp_hi) << 32),
			      &pcb->time_stamp) != RET_SUCCESS) {
		pcb->time_stamp = isp->timestamp_sw_prev;
		ISP_PR_WARN("failed to get timestamp,cid %u,stream_id %d,timestamp:0x%llx\n",
			    cid, fw_stream_id, pcb->time_stamp);
	} else {
		isp->timestamp_sw_prev = pcb->time_stamp;
		ISP_PR_INFO("success to get timestamp,cid %u,stream_id %d,timestamp:0x%llx\n",
			    __func__, cid, fw_stream_id, pcb->time_stamp);
	}

	ISP_PR_INFO("%s,ts:%llu,cameraId:%d,streamId:%d,poc:%u,preview_en:%u,%s(%i)\n",
		    __func__, ktime_get_ns(), cid, fw_stream_id, meta->poc,
		    meta->preview.enabled,
		    isp_dbg_get_buf_done_str(meta->preview.status),
		    meta->preview.status);

	/* WA here to avoid miss valid RAW buffer, */
	/* currently FW didn't set "source". */
	meta->raw_mipi.source = BUFFER_SOURCE_STREAM;
	meta->byrp_tap_out.source = BUFFER_SOURCE_STREAM;

	prev = isp_preview_done(isp, cid, meta, pcb);
	video = isp_video_done(isp, cid, meta, pcb);
	zsl = isp_zsl_done(isp, cid, meta, pcb);

	if (pcb->preview.status != BUF_DONE_STATUS_ABSENT)
		isp_dbg_show_bufmeta_info("prev", cid, &meta->preview,
					  &pcb->preview.buf);

	if (pcb->video.status != BUF_DONE_STATUS_ABSENT)
		isp_dbg_show_bufmeta_info("video", cid, &meta->video,
					  &pcb->video.buf);

	if (pcb->zsl.status != BUF_DONE_STATUS_ABSENT)
		isp_dbg_show_bufmeta_info("zsl", cid, &meta->still,
					  &pcb->zsl.buf);

	if (meta->metadata.status == BUFFER_STATUS_DONE) {
		meta_data_mc =
			isp_join_addr64(meta->metadata.buffer.buf_base_a_lo,
					meta->metadata.buffer.buf_base_a_hi);
	};

	if (isp->evt_cb[cid]) {
		if (pcb->preview.status != BUF_DONE_STATUS_ABSENT ||
		    pcb->video.status != BUF_DONE_STATUS_ABSENT ||
		    pcb->zsl.status != BUF_DONE_STATUS_ABSENT) {
			isp->evt_cb[cid](isp->evt_cb_context[cid],
					 CB_EVT_ID_FRAME_DONE, pcb);
		}
	} else {
		ISP_PR_ERR("in %s,fail empty cb for cid %u\n", __func__, cid);
	}

	if (prev) {
		if (prev->sys_img_buf_hdl)
			isp_sys_mem_free(prev->sys_img_buf_hdl);
		isp_sys_mem_free(prev);
	}

	if (video) {
		if (video->sys_img_buf_hdl)
			isp_sys_mem_free(video->sys_img_buf_hdl);
		isp_sys_mem_free(video);
	}

	if (zsl) {
		if (zsl->sys_img_buf_hdl)
			isp_sys_mem_free(zsl->sys_img_buf_hdl);
		isp_sys_mem_free(zsl);
	}

	if (isp->sensor_info[cid].status == START_STATUS_STARTED)
		resend_meta_in_framedone(isp, cid, fw_stream_id, mc, meta_data_mc);

	if (pcb)
		isp_sys_mem_free(pcb);

	ISP_PR_DBG("stream_id:%d, status:%d\n", fw_stream_id,
		   isp->sensor_info[cid].status);
}

bool isp_semaphore_acquire_one_try(struct isp_context *isp)
{
	u8 i = 0;
	bool ret = true;

	if (!isp) {
		ISP_PR_ERR("-><- %s: fail for null isp\n", __func__);
		return false;
	}

	isp_mutex_lock(&isp->isp_semaphore_mutex);
	do {
		isp_reg_write(ISP_SEMAPHORE_0, ISP_SEMAPHORE_ID_X86);
		if (isp_reg_read(ISP_SEMAPHORE_0) == ISP_SEMAPHORE_ID_X86)
			break;

		i++;
	} while (i < ISP_SEMAPHORE_ATTEMPTS);

	if (i >= ISP_SEMAPHORE_ATTEMPTS) {
		ret = false;
	} else {
		ret = true;
		isp->isp_semaphore_acq_cnt++;
	}

	isp_mutex_unlock(&isp->isp_semaphore_mutex);
	return ret;
}

bool isp_semaphore_acquire(struct isp_context *isp)
{
	u8 i = 0;

	if (!isp) {
		ISP_PR_ERR("-><- %s: fail for null isp\n", __func__);
		return false;
	}
	do {
		if (isp_semaphore_acquire_one_try(isp))
			break;
		i++;
		msleep(ISP_SEMAPHORE_DELAY);
	} while (i < ISP_SEMAPHORE_ATTEMPTS);

	if (i >= ISP_SEMAPHORE_ATTEMPTS) {
		ISP_PR_ERR
		("%s: acquire isp_semaphore timeout[%dms]!!!, value 0x%x\n",
		 __func__, ISP_SEMAPHORE_ATTEMPTS * ISP_SEMAPHORE_DELAY,
		 isp_reg_read(ISP_SEMAPHORE_0));
		return false;
	} else {
		return true;
	}
}

void isp_semaphore_release(struct isp_context *isp)
{
	if (!isp) {
		ISP_PR_ERR("-><- %s: fail for null isp\n", __func__);
		return;
	}
	isp_mutex_lock(&isp->isp_semaphore_mutex);
	isp->isp_semaphore_acq_cnt--;

	if (isp->isp_semaphore_acq_cnt == 0) {
		if (isp_reg_read(ISP_SEMAPHORE_0) == ISP_SEMAPHORE_ID_X86) {
			isp_reg_write(ISP_SEMAPHORE_0, 0);
		} else {
			ISP_PR_ERR
			("cnt dec to %u, ISP_SEMAPHORE 0x%x should be 0x%x\n",
			 isp->isp_semaphore_acq_cnt,
			 isp_reg_read(ISP_SEMAPHORE_0),
			 ISP_SEMAPHORE_ID_X86);
		}
	}

	isp_mutex_unlock(&isp->isp_semaphore_mutex);
}

void isp_fw_resp_func(struct isp_context *isp,
		      enum fw_cmd_resp_stream_id fw_stream_id)
{
	struct resp_t resp;

	if (ISP_GET_STATUS(isp) < ISP_STATUS_FW_RUNNING)
		return;

	isp_fw_log_print(isp);

	while (true) {
		s32 ret;
		/* Semaphore check */
		if (!isp_semaphore_acquire(isp)) {
			ISP_PR_ERR
			("fail acquire isp semaphore stream_id %u\n",
			 fw_stream_id);
			break;
		}

		ret = isp_get_f2h_resp(isp, fw_stream_id, &resp);

		isp_semaphore_release(isp);
		if (ret != RET_SUCCESS)
			break;

		switch (resp.resp_id) {
		case RESP_ID_CMD_DONE:
			isp_fw_resp_cmd_done(isp, fw_stream_id,
					     (struct resp_cmd_done *)resp.resp_param);
			break;
		case RESP_ID_NOTI_FRAME_DONE:
			isp_fw_resp_frame_done(isp, fw_stream_id,
					       (struct resp_param_package_t *)resp.resp_param);
			break;
		default:
			ISP_PR_ERR("-><- fail respid %s(0x%x)\n",
				   isp_dbg_get_resp_str(resp.resp_id),
				   resp.resp_id);
			break;
		}
	}
}

s32 isp_fw_resp_thread_wrapper(void *context)
{
	struct isp_fw_resp_thread_para *para = context;
	struct isp_context *isp;
	struct thread_handler *thread_ctx;
	enum fw_cmd_resp_stream_id fw_stream_id;
	u64 timeout;

	if (!para) {
		ISP_PR_ERR("-><- invalid para\n");
		goto quit;
	}

	switch (para->idx) {
	case 0:
		fw_stream_id = FW_CMD_RESP_STREAM_ID_GLOBAL;
		break;
	case 1:
		fw_stream_id = FW_CMD_RESP_STREAM_ID_1;
		break;
	case 2:
		fw_stream_id = FW_CMD_RESP_STREAM_ID_2;
		break;
	case 3:
		fw_stream_id = FW_CMD_RESP_STREAM_ID_3;
		break;
	default:
		ISP_PR_ERR("-><- invalid idx[%d]\n", para->idx);
		goto quit;
	}

	isp = para->isp;
	thread_ctx = &isp->fw_resp_thread[para->idx];

	thread_ctx->wakeup_evt.event = 0;
	mutex_init(&thread_ctx->mutex);
	init_waitqueue_head(&thread_ctx->waitq);
	timeout = msecs_to_jiffies(WORK_ITEM_INTERVAL);

	ISP_PR_DBG("[%u] started\n", para->idx);

	while (true) {
		wait_event_interruptible_timeout(thread_ctx->waitq,
						 thread_ctx->wakeup_evt.event,
						 timeout);
		thread_ctx->wakeup_evt.event = 0;

		if (kthread_should_stop()) {
			ISP_PR_INFO("[%u] quit\n", para->idx);
			break;
		}

		mutex_lock(&thread_ctx->mutex);
		isp_fw_resp_func(isp, fw_stream_id);
		mutex_unlock(&thread_ctx->mutex);
	}

	mutex_destroy(&thread_ctx->mutex);

quit:
	return 0;
}

s32 isp_start_resp_proc_threads(struct isp_context *isp)
{
	u32 i;

	for (i = 0; i < MAX_REAL_FW_RESP_STREAM_NUM; i++) {
		isp_resp_para[i].idx = i;
		isp_resp_para[i].isp = isp;
		if (create_work_thread(&isp->fw_resp_thread[i],
				       isp_fw_resp_thread_wrapper,
				       &isp_resp_para[i]) != RET_SUCCESS) {
			ISP_PR_ERR("%s [%u]fail\n", __func__, i);
			goto fail;
		}
	}
	return RET_SUCCESS;
fail:
	isp_stop_resp_proc_threads(isp);
	ISP_PR_ERR("fail\n");
	return RET_FAILURE;
}

s32 isp_stop_resp_proc_threads(struct isp_context *isp)
{
	u32 i;

	for (i = 0; i < MAX_REAL_FW_RESP_STREAM_NUM; i++)
		stop_work_thread(&isp->fw_resp_thread[i]);

	return RET_SUCCESS;
}

void wake_up_resp_thread(struct isp_context *isp, u32 index)
{
	if (isp && index < MAX_REAL_FW_RESP_STREAM_NUM) {
		struct thread_handler *thread_ctx = &isp->fw_resp_thread[index];

		thread_ctx->wakeup_evt.event = 1;
		wake_up_interruptible(&thread_ctx->waitq);
	}
}

