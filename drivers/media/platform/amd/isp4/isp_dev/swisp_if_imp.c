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

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <drm/ttm/ttm_tt.h>
#include <linux/page_ref.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "amdgpu_object.h"

#include "amd_isp_debug.h"
#include "amd_common.h"
#include "isp_common.h"
#include "swisp_if_imp.h"
#include "isp_fw_if/isp_hw_reg.h"
#include "amd_stream.h"

#define ISP_NBIF_GPU_PCIE_INDEX 0xE
#define ISP_NBIF_GPU_PCIE_DATA 0xF
#define RMMIO_SIZE 524288
#define RREG_FAILED_VAL 0xFFFFFFFF
#define ISP_POWER_OFF_CMD         0x29
#define ISP_POWER_ON_CMD          0x2A
#define ISP_ALL_TILES             0x7FF
#define ISP_XCLK_CMD              0x2C
#define ISP_ICLK_CMD              0x2B

struct swisp_context {
	struct amd_cam *amd_cam;
};

static struct sw_isp_if *swisp_if_self;

static enum swisp_result swisp_alloc_gpumem(void *sw_isp,
					    struct isp_allocate_gpu_memory_input *memory_input,
					    struct isp_allocate_gpu_memory_output *memory_output)
{
	struct swisp_context *ctx = (struct swisp_context *)sw_isp;

	if (!ctx) {
		ISP_PR_ERR("-><- %s fail for null ctx\n", __func__);
		return SWISP_RESULT_ERROR_INVALIDPARAMS;
	}
	return SWISP_RESULT_ERROR_FUNCTION_NOT_SUPPORT;
}

static enum swisp_result swisp_release_gpumem(void *sw_isp,
					      struct isp_release_gpu_memory_input *memory_in)
{
	struct swisp_context *ctx = (struct swisp_context *)sw_isp;

	if (!ctx) {
		ISP_PR_ERR("-><- %s fail for null ctx\n", __func__);
		return SWISP_RESULT_ERROR_INVALIDPARAMS;
	}
	return SWISP_RESULT_ERROR_FUNCTION_NOT_SUPPORT;
}

static u32 swisp_read_reg(void *sw_isp, u32 reg)
{
	struct swisp_context *ctx = (struct swisp_context *)sw_isp;
	u32 ret;

	if (!ctx) {
		ISP_PR_ERR("-><- %s fail for null ctx\n", __func__);
		return RREG_FAILED_VAL;
	}

	if (IS_ERR(ctx->amd_cam->isp_mmio)) {
		ISP_PR_ERR("%s failed, invalid iomem handle!\n", __func__);
		return PTR_ERR(ctx->amd_cam->isp_mmio);
	}

	if (reg >= RMMIO_SIZE) {
		ISP_PR_ERR("-><- %s failed bad offset %u\n", __func__, reg);
		return RREG_FAILED_VAL;
	}
	ret = readl(ctx->amd_cam->isp_mmio + reg);

	return ret;

};

static void swisp_write_reg(void *sw_isp, u32 reg, u32 val)
{
	struct swisp_context *ctx = (struct swisp_context *)sw_isp;

	if (!ctx) {
		ISP_PR_ERR("-><- %s fail for null ctx\n", __func__);
		return;
	}

	if (IS_ERR(ctx->amd_cam->isp_mmio)) {
		ISP_PR_ERR("%s failed, invalid iomem handle!\n", __func__);
		return;
	}

	if (reg >= RMMIO_SIZE) {
		ISP_PR_ERR("-><- %s failed bad offset %u\n", __func__, reg);
		return;
	}
	writel(val, ctx->amd_cam->isp_mmio + reg);
};

static u32 swisp_indirect_read_reg(void *sw_isp, u32 reg)
{
	struct swisp_context *ctx = (struct swisp_context *)sw_isp;
	unsigned long pcie_index, pcie_data;
	u32 ret;
	void __iomem *pcie_index_offset;
	void __iomem *pcie_data_offset;

	if (!ctx || !ctx->amd_cam) {
		ISP_PR_ERR("-><- %s fail for null ctx %p, amd_cam %p\n",
			   __func__,
			   ctx,
			   ctx->amd_cam);
		return RREG_FAILED_VAL;
	}
	if (IS_ERR(ctx->amd_cam->isp_mmio)) {
		ISP_PR_ERR("%s failed, invalid iomem handle!\n", __func__);
		return PTR_ERR(ctx->amd_cam->isp_mmio);
	}

	if (reg < RMMIO_SIZE) {
		ISP_PR_ERR("-><- %s failed bad offset %u\n", __func__, reg);
		return RREG_FAILED_VAL;
	}

	pcie_index = ISP_NBIF_GPU_PCIE_INDEX;
	pcie_data = ISP_NBIF_GPU_PCIE_DATA;

	pcie_index_offset = ctx->amd_cam->isp_mmio + pcie_index * 4;
	pcie_data_offset = ctx->amd_cam->isp_mmio + pcie_data * 4;
	writel(reg, pcie_index_offset);
	ret = readl(pcie_data_offset);
	return ret;

};

static void swisp_indirect_write_reg(void *sw_isp, u32 reg, u32 val)
{
	struct swisp_context *ctx = (struct swisp_context *)sw_isp;
	unsigned long pcie_index, pcie_data;
	void __iomem *pcie_index_offset;
	void __iomem *pcie_data_offset;

	if (!ctx || !ctx->amd_cam) {
		ISP_PR_ERR("-><- %s fail for null ctx %p, amd_cam %p\n",
			   __func__,
			   ctx,
			   ctx->amd_cam);
		return;
	}
	if (IS_ERR(ctx->amd_cam->isp_mmio)) {
		ISP_PR_ERR("%s failed, invalid iomem handle!\n", __func__);
		return;
	}

	if (reg < RMMIO_SIZE) {
		ISP_PR_ERR("-><- %s failed bad offset %u\n", __func__, reg);
		return;
	}

	pcie_index = ISP_NBIF_GPU_PCIE_INDEX;
	pcie_data = ISP_NBIF_GPU_PCIE_DATA;

	pcie_index_offset = ctx->amd_cam->isp_mmio + pcie_index * 4;
	pcie_data_offset = ctx->amd_cam->isp_mmio + pcie_data * 4;
	writel(reg, pcie_index_offset);
	writel(val, pcie_data_offset);
};

static enum swisp_result isp_query_pmfw_mbox_status(void *sw_isp,
						    u32 *mbox_status)
{
	u32 retry = 10;
	enum swisp_result ret = SWISP_RESULT_OK;

	if (mbox_status) {
		do {
			*mbox_status =
				swisp_indirect_read_reg(sw_isp,
							HOST2PM_RESP_REG);
			usleep_range(5000, 10000);
		} while ((*mbox_status == 0) && (retry--));

		if (*mbox_status == 0) {
			ISP_PR_DBG("PMFW mbox not ready.\n");
			ret = SWISP_RESULT__ERROR_TIMEOUT;
		}
	} else {
		ISP_PR_ERR("Invalid mbox_status pointer.\n");
		ret = SWISP_RESULT_ERROR_GENERIC;
	}
	return ret;
}

static int set_xclk(struct swisp_context *ctx, u32 xclk_mhz)
{
	enum swisp_result ret = SWISP_RESULT_OK;
	u32 reg_val = 0;
	u32 mbox_status = 0;

	ret = isp_query_pmfw_mbox_status(ctx, &mbox_status);
	if (ret != SWISP_RESULT_OK) {
		ISP_PR_ERR("failed, invalid pmfw mbox status 0x%x!\n", ret);
		return SWISP_RESULT_ERROR_GENERIC;
	}

	/* set xclk */
	swisp_indirect_write_reg(ctx, HOST2PM_RESP_REG, 0);

	swisp_indirect_write_reg(ctx, HOST2PM_ARG_REG, xclk_mhz);

	swisp_indirect_write_reg(ctx, HOST2PM_MSG_REG, ISP_XCLK_CMD);

	ret = isp_query_pmfw_mbox_status(ctx, &mbox_status);
	if (ret != SWISP_RESULT_OK) {
		ISP_PR_ERR("failed, invalid pmfw mbox status 0x%x!\n",
			   ret);
		return SWISP_RESULT_ERROR_GENERIC;
	}

	if (mbox_status == ISPSMC_Result_OK) {
		reg_val = swisp_indirect_read_reg(ctx, HOST2PM_ARG_REG);
		ISP_PR_DBG("set xclk %d success, reg_val %d\n",
			   xclk_mhz, reg_val);
	} else {
		ret = SWISP_RESULT_ERROR_GENERIC;
		ISP_PR_ERR("failed, invalid pmfw response 0x%x\n", mbox_status);
	}
	return ret;
}

static int set_iclk(struct swisp_context *ctx, u32 iclk_mhz)
{
	enum swisp_result ret = SWISP_RESULT_OK;
	u32 reg_val = 0;
	u32 mbox_status = 0;

	ret = isp_query_pmfw_mbox_status(ctx, &mbox_status);
	if (ret != SWISP_RESULT_OK) {
		ISP_PR_ERR("failed, invalid pmfw mbox status 0x%x!\n", ret);
		return SWISP_RESULT_ERROR_GENERIC;
	}

	/* set iclk */
	swisp_indirect_write_reg(ctx, HOST2PM_RESP_REG, 0);

	swisp_indirect_write_reg(ctx, HOST2PM_ARG_REG, iclk_mhz);

	swisp_indirect_write_reg(ctx, HOST2PM_MSG_REG, ISP_ICLK_CMD);

	ret = isp_query_pmfw_mbox_status(ctx, &mbox_status);
	if (ret != SWISP_RESULT_OK) {
		ISP_PR_ERR("failed, invalid pmfw mbox status 0x%x!\n",
			   ret);
		return SWISP_RESULT_ERROR_GENERIC;
	}

	if (mbox_status == ISPSMC_Result_OK) {
		reg_val = swisp_indirect_read_reg(ctx, HOST2PM_ARG_REG);
		ISP_PR_DBG("set iclk %d success, reg_val %d\n",
			   iclk_mhz, reg_val);
	} else {
		ret = SWISP_RESULT_ERROR_GENERIC;
		ISP_PR_ERR("failed, invalid pmfw response 0x%x\n", mbox_status);
	}
	return ret;
}

static enum swisp_result swisp_req_clk(void *sw_isp,
				       struct isp_pm_req_min_clk_input *min_clk_input)
{
	struct swisp_context *ctx = (struct swisp_context *)sw_isp;

	if (!ctx || !min_clk_input) {
		ISP_PR_ERR("-><- %s invalid params\n", __func__);
		return SWISP_RESULT_ERROR_INVALIDPARAMS;
	}

	if (IS_ERR(ctx->amd_cam->isp_mmio)) {
		ISP_PR_ERR("%s failed, invalid iomem handle!\n", __func__);
		return SWISP_RESULT_ERROR_INVALIDPARAMS;
	}

	ISP_PR_DBG("request xclk %d iclk %d socclk %d",
		   min_clk_input->min_xclk,
		   min_clk_input->min_iclk,
		   min_clk_input->min_sclk);

	set_xclk(ctx, min_clk_input->min_xclk);

	set_iclk(ctx, min_clk_input->min_iclk);

	return SWISP_RESULT_OK;
}

static enum swisp_result swisp_req_pwr(void *sw_isp,
				       struct isp_pm_req_pwr_input *pwr_input)
{
	struct swisp_context *ctx = (struct swisp_context *)sw_isp;
	u32 mbox_status = 0;
	u32 reg_val = 0;
	enum swisp_result ret = SWISP_RESULT_OK;
	u8 *cmdstr;

	if (!ctx || !pwr_input) {
		ISP_PR_ERR("-><- %s fail for null ctx\n", __func__);
		return SWISP_RESULT_ERROR_INVALIDPARAMS;
	}

	if (IS_ERR(ctx->amd_cam->isp_mmio)) {
		ISP_PR_ERR("%s failed, invalid iomem handle!\n", __func__);
		return SWISP_RESULT_ERROR_INVALIDPARAMS;
	}

	cmdstr = pwr_input->power_up ? "on" : "off";
	ret = isp_query_pmfw_mbox_status(ctx, &mbox_status);
	if (ret != SWISP_RESULT_OK) {
		ISP_PR_ERR("%s %s failed, invalid pmfw mbox status 0x%x!\n",
			   __func__, cmdstr, ret);
		return SWISP_RESULT_ERROR_GENERIC;
	}

	swisp_indirect_write_reg(ctx, HOST2PM_RESP_REG, 0);

	swisp_indirect_write_reg(ctx, HOST2PM_ARG_REG, ISP_ALL_TILES);

	if (pwr_input->power_up) {
		swisp_indirect_write_reg(ctx, HOST2PM_MSG_REG,
					 ISP_POWER_ON_CMD);
	} else {
		swisp_indirect_write_reg(ctx, HOST2PM_MSG_REG,
					 ISP_POWER_OFF_CMD);
	}

	ret = isp_query_pmfw_mbox_status(ctx, &mbox_status);
	if (ret != SWISP_RESULT_OK) {
		ISP_PR_ERR("%s %s  failed, invalid pmfw mbox status 0x%x!\n",
			   __func__, cmdstr, ret);
		return SWISP_RESULT_ERROR_GENERIC;
	}

	if (mbox_status == ISPSMC_Result_OK) {
		reg_val = swisp_indirect_read_reg(ctx, HOST2PM_ARG_REG);
		ISP_PR_DBG("%s %s completed 0x%x\n",
			   __func__, cmdstr, reg_val);
	} else {
		ret = SWISP_RESULT_ERROR_GENERIC;
		ISP_PR_ERR("%s %s failed, invalid pmfw response 0x%x\n",
			   __func__, cmdstr, mbox_status);
	}

	return ret;
}

u32 isp_reg_read(u32 reg)
{
	if (swisp_if_self && swisp_if_self->read_reg)
		return swisp_if_self->read_reg(swisp_if_self->context, reg);
	else
		return 0xDEADBEEF;
}

void isp_reg_write(u32 reg, u32 val)
{
	if (swisp_if_self && swisp_if_self->write_reg)
		swisp_if_self->write_reg(swisp_if_self->context, reg, val);
}

void isp_indirect_wreg(u32 reg, u32 val)
{
	if (swisp_if_self && swisp_if_self->indirect_write_reg)
		swisp_if_self->indirect_write_reg(swisp_if_self->context, reg, val);
}

u32 isp_indirect_rreg(u32 reg)
{
	if (swisp_if_self && swisp_if_self->indirect_read_reg)
		return swisp_if_self->indirect_read_reg(swisp_if_self->context, reg);
	else
		return 0xDEADBEEF;
}

int isp_clock_set(u32 xclk_mhz, u32 iclk_mhz, u32 sclk_mhz)
{
	struct isp_pm_req_min_clk_input clk_input = { 0 };

	clk_input.sclk = 1;
	clk_input.iclk = 1;
	clk_input.xclk = 1;
	clk_input.min_xclk = xclk_mhz;
	clk_input.min_iclk = iclk_mhz;
	clk_input.min_sclk = sclk_mhz;

	if (swisp_if_self && swisp_if_self->req_clk)
		return swisp_if_self->req_clk(swisp_if_self->context, &clk_input);
	else
		return -1;
}

int isp_power_set(int enable)
{
	struct isp_pm_req_pwr_input pwr_input = {0};

	if (enable)
		pwr_input.power_up = 1;

	if (swisp_if_self && swisp_if_self->req_pwr)
		return swisp_if_self->req_pwr(swisp_if_self->context, &pwr_input);
	else
		return -1;
}

struct isp_gpu_mem_info *isp_gpu_mem_alloc(u32 mem_size)
{
	struct amdisp_platform_data *pltf_data;
	struct isp_gpu_mem_info *mem_info;
	struct amdgpu_device *adev;
	struct amdgpu_bo *bo = NULL;
	void *cpu_ptr;
	u64 gpu_addr;
	u32 ret;

	if (!swisp_if_self || !swisp_if_self->context) {
		ISP_PR_ERR("invalid swisp_if");
		return NULL;
	}

	struct swisp_context *swisp_ctx = (struct swisp_context *)swisp_if_self->context;

	if (!swisp_ctx->amd_cam) {
		ISP_PR_ERR("invalid amd_cam");
		return NULL;
	}

	if (mem_size == 0) {
		ISP_PR_ERR("invalid mem size");
		return NULL;
	}

	mem_info = kzalloc(sizeof(*mem_info), GFP_KERNEL);
	if (!mem_info) {
		ISP_PR_ERR("alloc failed");
		return NULL;
	}

	pltf_data = swisp_ctx->amd_cam->pltf_data;
	adev = (struct amdgpu_device *)pltf_data->adev;
	mem_info->mem_size = mem_size;
	mem_info->mem_align = ISP_MC_ADDR_ALIGN;
	mem_info->mem_domain = AMDGPU_GEM_DOMAIN_GTT;

	ret = amdgpu_bo_create_kernel(adev,
				      mem_info->mem_size,
				      mem_info->mem_align,
				      mem_info->mem_domain,
				      &bo,
				      &gpu_addr,
				      &cpu_ptr);

	if (!cpu_ptr || ret) {
		ISP_PR_ERR("gpuvm buffer alloc failed, size %u\n", mem_size);
		kfree(mem_info);
		return NULL;
	}

	mem_info->sys_addr = cpu_ptr;
	mem_info->gpu_mc_addr = gpu_addr;
	mem_info->mem_handle = (void *)bo;

	return mem_info;
}

int isp_gpu_mem_free(struct isp_gpu_mem_info *mem_info)
{
	struct amdisp_platform_data *pltf_data;
	struct amdgpu_device *adev;
	struct amdgpu_bo *bo;

	if (!mem_info) {
		ISP_PR_ERR("invalid mem_info");
		return -EINVAL;
	}

	if (!swisp_if_self || !swisp_if_self->context) {
		ISP_PR_ERR("invalid swisp_if");
		return -EINVAL;
	}

	struct swisp_context *swisp_ctx = (struct swisp_context *)swisp_if_self->context;

	if (!swisp_ctx->amd_cam) {
		ISP_PR_ERR("invalid amd_cam");
		return -EINVAL;
	}

	bo = (struct amdgpu_bo *)mem_info->mem_handle;
	pltf_data = swisp_ctx->amd_cam->pltf_data;
	adev = (struct amdgpu_device *)pltf_data->adev;

	amdgpu_bo_free_kernel(&bo, &mem_info->gpu_mc_addr, &mem_info->sys_addr);

	kfree(mem_info);

	return 0;
}

int swisp_if_init(struct sw_isp_if *intf, struct amd_cam *pamd_cam)
{
	struct swisp_context *context;

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
	memset(intf, 0, sizeof(struct sw_isp_if));
	intf->size = sizeof(struct sw_isp_if);
	intf->version = SWISP_IF_VERSION_1;
	intf->read_reg = swisp_read_reg;
	intf->write_reg = swisp_write_reg;
	intf->indirect_read_reg = swisp_indirect_read_reg;
	intf->indirect_write_reg = swisp_indirect_write_reg;
	intf->alloc_gpumem = swisp_alloc_gpumem;
	intf->release_gpumem = swisp_release_gpumem;
	intf->req_clk = swisp_req_clk;
	intf->req_pwr = swisp_req_pwr;
	context->amd_cam = pamd_cam;
	intf->context = context;
	swisp_if_self = intf;
	ISP_PR_DBG("-><- %s context:%p amd_cam:%p\n",
		   __func__, intf->context, context->amd_cam);
	return OK;
}

void swisp_if_fini(struct sw_isp_if *intf)
{
	struct swisp_context *context;

	if (!intf || !intf->context) {
		ISP_PR_ERR("-><- %s fail bad param intf:%p context:%p\n",
			   __func__, intf, intf->context);
	};

	swisp_if_self = NULL;
	context = (struct swisp_context *)intf->context;
	kfree(context);
	memset(intf, 0, sizeof(struct sw_isp_if));
}
