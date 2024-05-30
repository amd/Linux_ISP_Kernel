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

#include "log.h"
#include "amd_common.h"
#include "swisp_if_imp.h"
#include "isp_module_if.h"
#include "isp_module_if_imp.h"
#include "amd_stream.h"
#include "isp_common.h"
#include "isp_mc_addr_mgr.h"
#include "isp_pwr.h"
#include "isp_fw_thread.h"

int isp_clk_change(struct isp_context *isp, enum camera_port_id cid,
		   unsigned int index, int hdr_enable, int on)
{
	/* this part of code need to be added to set clock */
	return RET_SUCCESS;
}

int isp_ip_pwr_on(struct isp_context *isp, enum camera_port_id cid,
		  u32 index, s32 hdr_enable)
{
	int ret;
	struct isp_pwr_unit *pwr_unit;

	ISP_PR_INFO("cid %u, idx %u, hdr %u\n",
		    cid, index, hdr_enable);
	if (!isp) {
		ISP_PR_ERR("fail for null isp\n");
		return RET_INVALID_PARAM;
	}
	pwr_unit = &isp->isp_pu_isp;

	isp_mutex_lock(&pwr_unit->pwr_status_mutex);

	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		uint32 reg;

		isp->isp_semaphore_acq_cnt = 0;
		isp_power_set(true);
		/* ISPPG ISP Power Status */
		isp_reg_write(ISP_POWER_STATUS, 0x7FF);

		reg = isp_reg_read(ISP_VERSION);
		ISP_PR_INFO("hw ver 0x%x\n", reg);

		reg = isp_reg_read(ISP_STATUS);
		ISP_PR_DBG("ISP status  0x%x\n", reg);

		if (isp_start_resp_proc_threads(isp) != RET_SUCCESS) {
			ISP_PR_ERR("in %s, isp_start_resp_proc_threads fail\n",
				   __func__);
			ret = RET_FAILURE;
			goto quit;
		} else {
			ISP_PR_INFO("in %s,create resp threads ok\n", __func__);
		}
	}

	/* set clocks */
	isp_clock_set(600, 300, 800);

	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF)
		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_ON;

	if (ISP_GET_STATUS(isp) == ISP_STATUS_PWR_OFF) {
		/* commit it temporarily, it ISP can work, will remove it later */
		/* isp_hw_reset_all(isp); */
		/* should be 24M */
		if (isp->refclk != 24) {
			ISP_PR_ERR("fail isp->refclk %u should be 24\n",
				   isp->refclk);
		}
		/* isp_i2c_init(isp->iclk); */
		/* change to following according to aidt */
		ISP_SET_STATUS(isp, ISP_STATUS_PWR_ON);
	}
	ret = RET_SUCCESS;
	ISP_PR_INFO("ISP Power on\n");

quit:
	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	return ret;
};

int isp_ip_pwr_off(struct isp_context *isp)
{
	int ret;
	struct isp_pwr_unit *pwr_unit;
	uint32 reg_val;

	if (!isp) {
		ISP_PR_ERR("fail for null isp\n");
		return RET_INVALID_PARAM;
	}
	pwr_unit = &isp->isp_pu_isp;

	isp_stop_resp_proc_threads(isp);
	ISP_PR_INFO("isp stop resp proc streads suc");

	isp_mutex_lock(&pwr_unit->pwr_status_mutex);
	if (pwr_unit->pwr_status == ISP_PWR_UNIT_STATUS_OFF) {
		ret = RET_SUCCESS;
		ISP_PR_INFO("suc do none\n");
	} else {
		/* hold ccpu reset */
		reg_val = isp_reg_read(ISP_SOFT_RESET);
		reg_val |= ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK;
		isp_reg_write(ISP_SOFT_RESET, 0x0);

		isp_reg_write(ISP_POWER_STATUS, 0);

		ISP_PR_WARN("disable isp power tile");
		isp_power_set(false);

		pwr_unit->pwr_status = ISP_PWR_UNIT_STATUS_OFF;
		ISP_SET_STATUS(isp, ISP_STATUS_PWR_OFF);
		isp->sclk = 0;
		isp->iclk = 0;
		isp->xclk = 0;
		isp->refclk = 0;
		ISP_PR_INFO("ISP Power off\n");
		ret = RET_SUCCESS;
	}
	isp->clk_info_set_2_fw = 0;
	for (enum camera_port_id cam_port_id = 0; cam_port_id < CAMERA_PORT_MAX;
	     cam_port_id++)
		isp->snr_info_set_2_fw[cam_port_id] = 0;
	isp_mutex_unlock(&pwr_unit->pwr_status_mutex);
	return ret;
};
