# SPDX-License-Identifier: GPL-2.0
#
#Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

obj-$(CONFIG_AMD_ISP4) += amd_capture.o
amd_capture-objs := 	isp_dev/amd_stream.o \
			isp_dev/swisp_if_imp.o \
			isp_dev/amd_isp_debug.o \
			isp_module/src/isp_fw_boot.o \
			isp_module/src/os_advance_type.o \
			isp_module/src/isp_module_if_imp.o \
			isp_module/src/isp_fw_cmd_resp.o \
			isp_module/src/isp_fw_thread.o \
			isp_module/src/isp_mc_addr_mgr.o \
			isp_module/src/isp_fw_if.o \
			isp_module/src/isp_queue.o \
			isp_module/src/isp_utils.o \
			isp_module/src/isp_pwr.o \

ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp4/isp_module
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp4/isp_module/inc
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp4/isp_module/src
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp4/isp_module/src/isp_fw_if
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp4/isp_module/src/isp_fw_if/hw_reg
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp4/isp_dev
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp4/cam_dev
ccflags-y += -I$(srctree)/drivers/media/platform/amd/isp4
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/include
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/amdgpu
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/pm/inc
ccflags-y += -I$(srctree)/include/drm
ccflags-y += -I$(srctree)/include
ccflags-y += -I$(srctree)/include/uapi/drm
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/scheduler
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/powerplay/inc
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/acp/include
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/display
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/display/include
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/display/modules/inc
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/display/dc
ccflags-y += -I$(srctree)/drivers/gpu/drm/amd/display/amdgpu_dm
ccflags-y += -I$(srctree)/../external/libdrm/amdgpu

ifneq ($(call cc-option, -mpreferred-stack-boundary=4),)
	cc_stack_align := -mpreferred-stack-boundary=4
endif

ifneq ($(CONFIG_AMD_PRINT_FLOAT_INFO),)
CFLAGS_isp_dev/amd_stream.o += -mhard-float -msse -DPRINT_FLOAT_INFO
endif

ccflags-y += $(cc_stack_align)
ccflags-y += -DAMD_ISP4

ifneq ($(CONFIG_AMD_ISP4),)
ccflags-y += -DISP4_SILICON
endif

ccflags-y += -DCONFIG_COMPAT
#ccflags-y += -DUSING_KGD_CGS
#ccflags-y += -DISP_BRINGUP_WORKAROUND
ccflags-y += -DISP_LOCAL_TEST
ccflags-y += -DAMD_SUPPORT_ISP_TUNING
ccflags-y += -DRAW_DUMP
ccflags-y += -DENABLE_ISP_DYNAMIC_CLOCK
ccflags-y += -DUSING_PSP_TO_LOAD_ISP_FW
