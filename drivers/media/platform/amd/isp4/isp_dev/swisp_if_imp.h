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

#ifndef SW_ISP_IF_IMP_H
#define SW_ISP_IF_IMP_H

#define RREG_FAILED_VAL 0xFFFFFFFF

/*
 *init sw isp interface, it must be called firstly to make sure
 *isp_reg_read/write, etc functions can work
 *normally called when isp device is probed
 */
int swisp_if_init(struct sw_isp_if *intf, struct amd_cam *pamd_cam);
/* unit sw isp interface, normally called when isp deiveice is removed */
void swisp_if_fini(struct sw_isp_if *intf);

u32 isp_reg_read(u32 reg);
void isp_reg_write(u32 reg, u32 val);
void isp_indirect_wreg(u32 reg, u32 val);
u32 isp_indirect_rreg(u32 reg);
int isp_clock_set(u32 xclk_mhz, u32 iclk_mhz, u32 sclk_mhz);
int isp_power_set(int enable);
struct isp_gpu_mem_info *isp_gpu_mem_alloc(u32 mem_size);
int isp_gpu_mem_free(struct isp_gpu_mem_info *mem_info);

#endif
