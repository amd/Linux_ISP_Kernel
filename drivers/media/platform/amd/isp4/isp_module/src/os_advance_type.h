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

#ifndef OS_ADVANCE_TYPE_H
#define OS_ADVANCE_TYPE_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/semaphore.h>

#define MAX_ISP_TIME_TICK 0x7fffffffffffffff
#define NANOSECONDS 10000000

typedef s32 isp_ret_status_t;
typedef s32 result_t;

struct isp_spin_lock {
	spinlock_t lock; /* spin lock */
};

struct isp_event {
	int automatic;
	int event;
	unsigned int result;
};

struct thread_handler {
	int stop_flag;
	struct isp_event wakeup_evt;
	struct task_struct *thread;
	struct mutex mutex; /* mutex */
	wait_queue_head_t waitq;
};

typedef int(*work_thread_prototype) (void *start_context);

#define os_read_reg32(address) (*((unsigned int *)address))
#define os_write_reg32(address, value) \
	(*((unsigned int *)address) = value)

#define isp_sys_mem_alloc(size) kmalloc(size, GFP_KERNEL)
#define isp_sys_mem_free(p) kfree(p)

#define isp_mutex_init(PM)	mutex_init(PM)
#define isp_mutex_destroy(PM)	mutex_destroy(PM)
#define isp_mutex_unlock(PM)	mutex_unlock(PM)

#define isp_spin_lock_init(s_lock)	spin_lock_init(&((s_lock).lock))
#define isp_spin_lock_lock(s_lock)	spin_lock(&((s_lock).lock))
#define isp_spin_lock_unlock(s_lock)	spin_unlock(&((s_lock).lock))

int isp_mutex_lock(struct mutex *p_mutex);

int isp_event_init(struct isp_event *p_event,
		   int auto_matic,
		   int init_state);
int isp_event_signal(unsigned int result,
		     struct isp_event *p_event);
int isp_event_reset(struct isp_event *p_event);
int isp_event_wait(struct isp_event *p_event,
		   unsigned int timeout_ms);
void isp_get_cur_time_tick(long long *ptimetick);
int isp_is_timeout(long long *start, long long *end,
		   unsigned int timeout_ms);
int create_work_thread(struct thread_handler *handle,
		       work_thread_prototype working_thread, void *context);
void stop_work_thread(struct thread_handler *handle);
int thread_should_stop(struct thread_handler *handle);

int polling_thread_wrapper(void *context);
int idle_detect_thread_wrapper(void *context);

int isp_write_file_test(struct file *fp, void *buf, ulong *len);

#endif
