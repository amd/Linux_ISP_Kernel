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

#include "isp_common.h"
#include "amd_isp_debug.h"

#include <linux/time.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/fs.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[ISP][os_advance_type]"

#define USING_WAIT_QUEUE

static wait_queue_head_t g_evt_waitq_head;
wait_queue_head_t *g_evt_waitq_headp;

int isp_mutex_lock(struct mutex *p_mutex)
{
	mutex_lock(p_mutex);
	return RET_SUCCESS;
}

int isp_event_init(struct isp_event *p_event, int auto_matic,
		   int init_state)
{
	p_event->automatic = auto_matic;
	p_event->event = init_state;
	p_event->result = 0;

	if (!g_evt_waitq_headp) {
		g_evt_waitq_headp = &g_evt_waitq_head;
		init_waitqueue_head(g_evt_waitq_headp);
	}
	return RET_SUCCESS;
};

int isp_event_signal(unsigned int result, struct isp_event *p_event)
{
	p_event->result = result;
	p_event->event = 1;

#ifdef USING_WAIT_QUEUE
	if (g_evt_waitq_headp)
		wake_up_interruptible(g_evt_waitq_headp);
	else
		ISP_PR_ERR("no head");
#endif
	ISP_PR_DBG("signal evt %p,result %d\n", p_event, result);
	return RET_SUCCESS;
};

int isp_event_reset(struct isp_event *p_event)
{
	p_event->event = 0;

	return RET_SUCCESS;
};

int isp_event_wait(struct isp_event *p_event, unsigned int timeout_ms)
{
#ifdef USING_WAIT_QUEUE
	if (g_evt_waitq_headp) {
		int temp;

		if (p_event->event)
			goto quit;

		temp = wait_event_interruptible_timeout
		       ((*g_evt_waitq_headp),
			p_event->event,
			(timeout_ms * HZ / 1000));

		if (temp == 0)
			return RET_TIMEOUT;
	} else {
		ISP_PR_ERR("no head");
	}
#else
	long long start, end;

	isp_get_cur_time_tick(&start);
	while (!p_event->event) {
		isp_get_cur_time_tick(&end);
		if (p_event->event)
			goto quit;

		if (isp_is_timeout(&start, &end, timeout_ms))
			return RET_TIMEOUT;

		usleep_range(10000, 11000);
	}
#endif

quit:
	if (p_event->automatic)
		p_event->event = 0;

	ISP_PR_DBG("wait evt %p suc\n", p_event);
	return p_event->result;
};

void isp_get_cur_time_tick(long long *ptimetick)
{
	if (ptimetick)
		*ptimetick = get_jiffies_64();

};

int isp_is_timeout(long long *start, long long *end,
		   unsigned int timeout_ms)
{
	if (!start || !end || timeout_ms == 0)
		return 1;
	if ((*end - *start) * 1000 / HZ >= (long long)timeout_ms)
		return 1;
	else
		return 0;

};

int create_work_thread(struct thread_handler *handle,
		       work_thread_prototype working_thread, void *context)
{
	struct task_struct *resp_polling_kthread;

	if (!handle || !working_thread) {
		ISP_PR_ERR("illegal parameter\n");
		return RET_INVALID_PARM;
	};

	if (handle->thread) {
		ISP_PR_DBG("response thread has already created\n");
		return RET_SUCCESS;
	}

	handle->stop_flag = false;

	isp_event_init(&handle->wakeup_evt, 1, 0);

	resp_polling_kthread =
		kthread_run(working_thread, context, "amd_isp4_thread");

	if (IS_ERR(resp_polling_kthread)) {
		ISP_PR_ERR("create thread fail\n");
		return RET_FAILURE;
	}

	handle->thread = resp_polling_kthread;

	return RET_SUCCESS;

};

void stop_work_thread(struct thread_handler *handle)
{
	if (!handle) {
		ISP_PR_ERR("illegal parameter\n");
		return;
	};

	if (handle->thread) {
		handle->stop_flag = true;
		kthread_stop(handle->thread);
		handle->thread = NULL;
	} else {
		ISP_PR_ERR("thread is NULL, do nothing\n");
	}

};

int thread_should_stop(struct thread_handler __maybe_unused *handle)
{
	return kthread_should_stop();
};

/* add for dump image in kernel */
int isp_write_file_test(struct file *fp, void *buf, ulong *len)
{
	ssize_t ret = -1;

	if (!len)
		return ret;

	ret = __kernel_write(fp, buf, *len, 0);/* &fp->f_op */

	/* will crash when close */
	/* filp_close(fp, NULL); */

	return ret;
}
