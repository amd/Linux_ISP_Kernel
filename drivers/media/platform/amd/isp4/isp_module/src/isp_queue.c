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
#include "isp_queue.h"
#include "amd_isp_debug.h"

result_t isp_list_init(struct isp_list *list)
{
	if (!list)
		return RET_FAILURE;
	isp_mutex_init(&list->mutex);
	list->front =
		(struct list_node *)isp_sys_mem_alloc(sizeof(struct list_node));
	if (!list->front)
		return RET_FAILURE;

	list->rear = list->front;
	list->front->next = NULL;
	list->count = 0;
	return RET_SUCCESS;
}

result_t isp_list_destory(struct isp_list *list, func_node_process func)
{
	struct list_node *p;

	if (!list)
		return RET_FAILURE;
	isp_mutex_lock(&list->mutex);
	while (list->front != list->rear) {
		p = list->front->next;
		list->front->next = p->next;
		if (list->rear == p)
			list->rear = list->front;
		if (func)
			func(p);
	}
	isp_sys_mem_free(list->front);
	list->front = NULL;
	list->rear = NULL;
	isp_mutex_unlock(&list->mutex);

	isp_mutex_destroy(&list->mutex);

	return RET_SUCCESS;
};

result_t isp_list_insert_tail(struct isp_list *list, struct list_node *p)
{
	if (!list)
		return RET_FAILURE;
	isp_mutex_lock(&list->mutex);

	list->rear->next = p;
	list->rear = p;
	p->next = NULL;
	list->count++;
	isp_mutex_unlock(&list->mutex);
	return RET_SUCCESS;
}

struct list_node *isp_list_get_first(struct isp_list *list)
{
	struct list_node *p;

	if (!list)
		return NULL;

	isp_mutex_lock(&list->mutex);
	if (list->front == list->rear) {
		if (list->count)
			ISP_PR_ERR("fail bad count %u\n",
				   list->count);
		isp_mutex_unlock(&list->mutex);
		return NULL;
	}

	p = list->front->next;
	list->front->next = p->next;
	if (list->rear == p)
		list->rear = list->front;
	if (list->count)
		list->count--;
	else
		ISP_PR_ERR("fail bad 0 count\n");
	isp_mutex_unlock(&list->mutex);
	return p;
}

struct list_node *isp_list_get_first_without_rm(struct isp_list *list)
{
	struct list_node *p;

	if (!list)
		return NULL;

	isp_mutex_lock(&list->mutex);
	if (list->front == list->rear) {
		isp_mutex_unlock(&list->mutex);
		return NULL;
	}

	p = list->front->next;
	isp_mutex_unlock(&list->mutex);
	return p;
}

void isp_list_rm_node(struct isp_list *list, struct list_node *node)
{
	struct list_node *p;
	struct list_node *pre;

	if (!list || !node)
		return;

	isp_mutex_lock(&list->mutex);
	if (list->front == list->rear) {
		isp_mutex_unlock(&list->mutex);
		return;
	}

	p = list->front->next;
	if (!p) {
		ISP_PR_ERR("-><- fail cannot find node1\n");
		return;
	}
	if (p == node) {
		list->front->next = p->next;
		if (list->rear == p)
			list->rear = list->front;
	} else {
		pre = p;
		p = p->next;
		while (p) {
			if (p == node) {
				pre->next = p->next;
				if (list->rear == p)
					list->rear = pre;
				break;
			}
		}
		if (!p)
			ISP_PR_ERR("-><- fail cannot find node\n");
	}

	isp_mutex_unlock(&list->mutex);
}

u32 isp_list_get_cnt(struct isp_list *list)
{
	int i = 0;
	struct list_node *p;

	if (!list)
		return 0;
	isp_mutex_lock(&list->mutex);
	p = list->front;
	while (p != list->rear) {
		++i;
		p = p->next;
	}
	isp_mutex_unlock(&list->mutex);
	return i;
}

result_t isp_spin_list_init(struct isp_spin_list *list)
{
	if (!list)
		return RET_FAILURE;
	isp_spin_lock_init(list->lock);
	list->front =
		(struct list_node *)isp_sys_mem_alloc(sizeof(struct list_node));
	if (!list->front)
		return RET_FAILURE;

	list->rear = list->front;
	list->front->next = NULL;

	return RET_SUCCESS;
}

result_t isp_spin_list_destory(struct isp_spin_list *list,
			       func_node_process func)
{
	struct list_node *p;

	if (!list)
		return RET_FAILURE;
	isp_spin_lock_lock((list->lock));
	while (list->front != list->rear) {
		p = list->front->next;
		list->front->next = p->next;
		if (list->rear == p)
			list->rear = list->front;
		if (func)
			func(p);
	}
	isp_sys_mem_free(list->front);
	list->front = NULL;
	list->rear = NULL;
	isp_spin_lock_unlock(list->lock);

	return RET_SUCCESS;
};

result_t isp_spin_list_insert_tail(struct isp_spin_list *list,
				   struct list_node *p)
{
	if (!list)
		return RET_FAILURE;
	isp_spin_lock_lock(list->lock);

	list->rear->next = p;
	list->rear = p;
	p->next = NULL;
	isp_spin_lock_unlock(list->lock);
	return RET_SUCCESS;
}

struct list_node *isp_spin_list_get_first(struct isp_spin_list *list)
{
	struct list_node *p;

	if (!list)
		return NULL;

	isp_spin_lock_lock(list->lock);
	if (list->front == list->rear) {
		isp_spin_lock_unlock(list->lock);
		return NULL;
	}

	p = list->front->next;
	list->front->next = p->next;
	if (list->rear == p)
		list->rear = list->front;
	isp_spin_lock_unlock(list->lock);
	return p;
}

void isp_spin_list_rm_node(struct isp_spin_list *list, struct list_node *node)
{
	struct list_node *p;
	struct list_node *pre;

	if (!list || !node)
		return;

	isp_spin_lock_lock(list->lock);
	if (list->front == list->rear) {
		isp_spin_lock_unlock(list->lock);
		return;
	}

	p = list->front->next;
	if (p == node) {
		list->front->next = p->next;
		if (list->rear == p)
			list->rear = list->front;
	} else {
		pre = p;
		p = p->next;
		while (p) {
			if (p == node) {
				pre->next = p->next;
				if (list->rear == p)
					list->rear = pre;
				break;
			}
		}
		if (!p)
			ISP_PR_ERR("-><- cannot find node\n");
	}

	isp_spin_lock_unlock(list->lock);
}

u32 isp_spin_list_get_cnt(struct isp_spin_list *list)
{
	int i = 0;
	struct list_node *p;

	if (!list)
		return 0;
	isp_spin_lock_lock(list->lock);
	p = list->front;
	while (p != list->rear) {
		++i;
		p = p->next;
	}
	isp_spin_lock_unlock(list->lock);
	return i;
}
