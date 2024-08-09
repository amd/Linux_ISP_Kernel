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

#ifndef AMD_COMMON_H
#define AMD_COMMON_H

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/debugfs.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-device.h>
#include "isp_module/inc/swisp_if.h"
#include "isp_module/inc/isp_module_if.h"

#define DRI_VERSION_MAJOR_SHIFT          (24)
#define DRI_VERSION_MINOR_SHIFT          (16)
#define DRI_VERSION_REVISION_SHIFT       (8)
#define DRI_VERSION_SUB_REVISION_SHIFT   (0)

#define DRI_VERSION_MAJOR_MASK           (0xff << DRI_VERSION_MAJOR_SHIFT)
#define DRI_VERSION_MINOR_MASK           (0xff << DRI_VERSION_MINOR_SHIFT)
#define DRI_VERSION_REVISION_MASK        (0xff << DRI_VERSION_REVISION_SHIFT)
#define DRI_VERSION_SUB_REVISION_MASK   (0xff << DRI_VERSION_SUB_REVISION_SHIFT)

#define DRI_VERSION_MAJOR          (0x4)
#define DRI_VERSION_MINOR          (0x0)
#define DRI_VERSION_REVISION       (0x1)
#define DRI_VERSION_SUB_REVISION   (0x0)
#define DRI_VERSION_STRING "ISP Driver Version: 4.0.1.0"
#define DRI_VERSION (((DRI_VERSION_MAJOR & 0xff) << DRI_VERSION_MAJOR_SHIFT) |\
	((DRI_VERSION_MINOR & 0xff) << DRI_VERSION_MINOR_SHIFT) |\
	((DRI_VERSION_REVISION & 0xff) << DRI_VERSION_REVISION_SHIFT) |\
	((DRI_VERSION_SUB_REVISION & 0xff) << DRI_VERSION_SUB_REVISION_SHIFT))

#define OK			0
#define MAX_HW_NUM		10
#define FW_STREAM_TYPE_NUM	7
#define MAX_REQUEST_DEPTH	10
#define NORMAL_YUV_STREAM_CNT	3
#define MAX_KERN_METADATA_BUF_SIZE 56320 /* 55kb */
/* 2^16(Format 16.16=>16 bits for integer part and 16 bits for fractional part) */
#define POINT_TO_FLOAT	65536
#define POINT_TO_DOUBLE	4294967296 /* 2^32(Format 32.32 ) */
#define STEP_NUMERATOR		1
#define STEP_DENOMINATOR	3
#define SIZE_ALIGN		8
#define SIZE_ALIGN_DOWN(size) \
	((unsigned int)(SIZE_ALIGN) * \
	((unsigned int)(size) / (unsigned int)(SIZE_ALIGN)))

enum sensor_idx {
	CAM_IDX_BACK = 0,
	CAM_IDX_FRONT_L = 1,
	CAM_IDX_FRONT_R = 2,
	CAM_IDX_MAX = 3,
};

struct isp4_capture_buffer {
	/*
	 * struct vb2_v4l2_buffer must be the first element
	 * the videobuf2 framework will allocate this struct based on
	 * buf_struct_size and use the first sizeof(struct vb2_buffer) bytes of
	 * memory as a vb2_buffer
	 */
	struct vb2_v4l2_buffer vb2;
	struct list_head list;
};

#define ISP4_VDEV_NUM		1
#define ISP4_VDEV_PREVIEW	0

struct isp4_video_dev {
	struct video_device vdev;
	struct media_pad vdev_pad;
	struct v4l2_pix_format format;

	/* mutex that protects vb2_queue */
	struct mutex vbq_lock;
	struct vb2_queue vbq;

	/*
	 * NOTE: in a real driver, a spin lock must be used to access the
	 * queue because the frames are generated from a hardware interruption
	 * and the isr is not allowed to sleep.
	 * Even if it is not necessary a spinlock in the vimc driver, we
	 * use it here as a code reference
	 */
	spinlock_t qlock;
	struct list_head buf_list;

	u32 sequence;
	u32 fw_run;
	struct task_struct *kthread;

	struct media_pipeline pipe;

	struct amd_cam *cam;
	struct v4l2_fract timeperframe;
};

struct amd_cam {
	struct isp4_video_dev isp_vdev[ISP4_VDEV_NUM];

	struct v4l2_subdev sdev;
	struct media_pad sdev_pad[ISP4_VDEV_NUM];

	struct v4l2_device v4l2_dev;
	struct media_device mdev;

	struct sw_isp_if swisp_if;
	struct isp_module_if ispm_if;

	void __iomem *isp_mmio;
	struct amdisp_platform_data *pltf_data;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dir;
#endif
};

enum stream_id get_vdev_stream_id(struct isp4_video_dev *vdev);

#endif /* amd_common.h */
