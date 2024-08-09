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
#include "isp_fw_boot.h"
#include "isp_common.h"
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/time.h>
#include "amd_stream.h"
#include "isp_module_if.h"
#include "isp_fw_if/param_types_pub.h"
#include "os_advance_type.h"
#include "amd_isp_debug.h"
#include "isp_common.h"
#include "swisp_if.h"
#include "swisp_if_imp.h"
#include "isp_mc_addr_mgr.h"

void isp_boot_disable_ccpu(void)
{
	u32 reg_val;

	reg_val = isp_reg_read(ISP_CCPU_CNTL);
	ISP_PR_DBG("rd ISP_CCPU_CNTL 0x%x\n", reg_val);
	reg_val |= ISP_CCPU_CNTL__CCPU_HOST_SOFT_RST_MASK;
	ISP_PR_DBG("wr ISP_CCPU_CNTL 0x%x\n", reg_val);
	isp_reg_write(ISP_CCPU_CNTL, reg_val);
	usleep_range(100, 150);
	reg_val = isp_reg_read(ISP_SOFT_RESET);
	ISP_PR_DBG("rd ISP_SOFT_RESET 0x%x\n", reg_val);
	reg_val |= ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK;
	ISP_PR_DBG("wr ISP_SOFT_RESET 0x%x\n", reg_val);
	/* disable CCPU */
	isp_reg_write(ISP_SOFT_RESET, reg_val);
}

void isp_boot_enable_ccpu(void)
{
	u32 reg_val;

	reg_val = isp_reg_read(ISP_SOFT_RESET);
	ISP_PR_DBG("rd ISP_SOFT_RESET 0x%x\n", reg_val);
	reg_val &= (~ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK);
	ISP_PR_DBG("rd ISP_SOFT_RESET 0x%x\n", reg_val);
	isp_reg_write(ISP_SOFT_RESET, reg_val); /* bus reset */
	usleep_range(100, 150);
	reg_val = isp_reg_read(ISP_CCPU_CNTL);
	ISP_PR_DBG("rd ISP_CCPU_CNTL 0x%x\n", reg_val);
	reg_val &= (~ISP_CCPU_CNTL__CCPU_HOST_SOFT_RST_MASK);
	ISP_PR_DBG("rd ISP_CCPU_CNTL 0x%x\n", reg_val);
	isp_reg_write(ISP_CCPU_CNTL, reg_val);
}

int isp_boot_fw_init(struct isp_context *isp)
{
	u64 log_addr;
	u32 log_len = ISP_LOGRB_SIZE;

	if (!isp->fw_running_buf) {
		isp->fw_running_buf =
			isp_gpu_mem_alloc(log_len);

		if (isp->fw_running_buf) {
			ISP_PR_DBG("size %u, allocate gpu mem suc", log_len);
		} else {
			ISP_PR_ERR("size %u, fail to allocate gpu mem", log_len);
			return RET_OUTOFMEM;
		}
	}

	log_addr = isp->fw_running_buf->gpu_mc_addr;
	isp->fw_log_buf = (u8 *)isp->fw_running_buf->sys_addr;
	isp->fw_log_buf_len = log_len;

	isp_reg_write(ISP_LOG_RB_BASE_HI0, ((log_addr >> 32) & 0xffffffff));
	isp_reg_write(ISP_LOG_RB_BASE_LO0, (log_addr & 0xffffffff));
	isp_reg_write(ISP_LOG_RB_SIZE0, log_len);

	ISP_PR_DBG("ISP_LOG_RB_BASE_HI=0x%08x",
		   isp_reg_read(ISP_LOG_RB_BASE_HI0));
	ISP_PR_DBG("ISP_LOG_RB_BASE_LO=0x%08x",
		   isp_reg_read(ISP_LOG_RB_BASE_LO0));
	ISP_PR_DBG("ISP_LOG_RB_SIZE=0x%08x",
		   isp_reg_read(ISP_LOG_RB_SIZE0));

	isp_reg_write(ISP_LOG_RB_WPTR0, 0x0);
	isp_reg_write(ISP_LOG_RB_RPTR0, 0x0);

	return RET_SUCCESS;
}

int isp_boot_cmd_resp_rb_init(struct isp_context *isp)
{
	u32 i;
	u32 total_size;

	if (!isp->fw_cmd_resp_buf) {
		total_size = RB_PMBMAP_MEM_SIZE;

		isp->fw_cmd_resp_buf =
			isp_gpu_mem_alloc(total_size);
		if (isp->fw_cmd_resp_buf) {
			ISP_PR_DBG("size %u, allocate gpu mem suc", total_size);
		} else {
			ISP_PR_ERR("size %u, fail to allocate gpu mem", total_size);
			return RET_OUTOFMEM;
		}
	}
	for (i = 0; i < ISP_FW_CMD_BUF_COUNT; i++)
		isp_fw_buf_get_cmd_base(isp, i, &isp->fw_cmd_buf_sys[i],
					&isp->fw_cmd_buf_mc[i],
					&isp->fw_cmd_buf_size[i]);
	for (i = 0; i < ISP_FW_RESP_BUF_COUNT; i++)
		isp_fw_buf_get_resp_base(isp, i, &isp->fw_resp_buf_sys[i],
					 &isp->fw_resp_buf_mc[i],
					 &isp->fw_resp_buf_size[i]);

	for (i = 0; i < ISP_FW_CMD_BUF_COUNT; i++)
		isp_init_fw_ring_buf(isp, i, 1);
	for (i = 0; i < ISP_FW_RESP_BUF_COUNT; i++)
		isp_init_fw_ring_buf(isp, i, 0);

	return RET_SUCCESS;
}

int isp_boot_wait_fw_ready(u32 isp_status_addr)
{
	u32 reg_val;
	u32 timeout = 0;
	u32 fw_ready_timeout;
	u32 interval_ms = 1;
	u32 timeout_ms = 100;

	fw_ready_timeout = timeout_ms / interval_ms;

	/* wait for FW initialize done! */
	while (timeout < fw_ready_timeout) {
		reg_val = isp_reg_read(isp_status_addr);
		ISP_PR_DBG("ISP_STATUS(0x%x):0x%x", isp_status_addr, reg_val);

		if (reg_val & ISP_STATUS__CCPU_REPORT_MASK) {
			ISP_PR_DBG("CCPU bootup succeeds!");
			return RET_SUCCESS;
		}

		msleep(interval_ms);
		timeout++;
	}

	ISP_PR_ERR("CCPU bootup fails!");

	return RET_TIMEOUT;
}

int isp_boot_isp_fw_boot(struct isp_context *isp)
{
	int ret;

	if (ISP_GET_STATUS(isp) != ISP_STATUS_PWR_ON) {
		ISP_PR_ERR("invalid isp power status %d", ISP_GET_STATUS(isp));
		return RET_FAILURE;
	}

	isp_reg_write(ISP_POWER_STATUS, 0x7);
	isp_boot_disable_ccpu();

	ret = isp_boot_fw_init(isp);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("0:isp_boot_fw_init failed:%d", ret);
		return ret;
	};

	ret = isp_boot_cmd_resp_rb_init(isp);
	if (ret != RET_SUCCESS) {
		ISP_PR_ERR("1:isp_boot_cmd_resp_rb_init failed:%d", ret);
		return ret;
	}

	/* clear register */
	isp_reg_write(ISP_STATUS, 0x0);

	isp_boot_enable_ccpu();
	if (isp_boot_wait_fw_ready(ISP_STATUS) != RET_SUCCESS) {
		ISP_PR_ERR("ccpu fail by bootup timeout");
		return RET_FAILURE;
	}

	/* enable interrupt */
	isp_reg_write(ISP_SYS_INT0_EN, FW_RESP_RB_IRQ_EN_MASK);
	ISP_PR_DBG("ISP_SYS_INT0_EN=0x%x\n", isp_reg_read(ISP_SYS_INT0_EN));

	ISP_SET_STATUS(isp, ISP_STATUS_FW_RUNNING);
	ISP_PR_INFO("ISP FW boot suc!");
	return RET_SUCCESS;
}
