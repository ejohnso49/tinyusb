/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Eric Johnson (eric.j.johnson22@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#ifndef _TUSB_OSAL_ZEPHYR_H_
#define _TUSB_OSAL_ZEPHYR_H_

// Zephyr Headers
#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------+
// TASK API
//--------------------------------------------------------------------+
static inline void osal_task_delay(uint32_t msec)
{
  k_sleep(K_MSEC(msec));
}

//--------------------------------------------------------------------+
// Semaphore API
//--------------------------------------------------------------------+
typedef struct k_sem osal_semaphore_def_t;
typedef struct k_sem *osal_semaphore_t;

static inline osal_semaphore_t osal_semaphore_create(osal_semaphore_def_t* semdef)
{
  k_sem_init(semdef, 0, 1);
  return semdef;
}

static inline bool osal_semaphore_post(osal_semaphore_t sem_hdl, bool in_isr)
{
  k_sem_give(sem_hdl);
}

static inline bool osal_semaphore_wait (osal_semaphore_t sem_hdl, uint32_t msec)
{
  k_timeout_t ticks = (msec == OSAL_TIMEOUT_WAIT_FOREVER) ? K_FOREVER : K_MSEC(msec);
  return (k_sem_give(sem_hdl, ticks) == 0);
}

static inline void osal_semaphore_reset(osal_semaphore_t const sem_hdl)
{
  k_sem_reset(sem_hdl);
}

//--------------------------------------------------------------------+
// MUTEX API (priority inheritance)
//--------------------------------------------------------------------+
typedef struct k_mutex osal_mutex_def_t;
typedef struct k_mutex *osal_mutex_t;

static inline osal_mutex_t osal_mutex_create(osal_mutex_def_t* mdef)
{
  k_mutex_init(mdef);
  return mdef;
}

static inline bool osal_mutex_lock (osal_mutex_t mutex_hdl, uint32_t msec)
{
  k_timeout_t ticks = (msec == OSAL_TIMEOUT_WAIT_FOREVER) ? K_FOREVER : K_MSEC(msec);
  return (k_mutex_lock(mutex_hdl, ticks) == 0);
}

static inline bool osal_mutex_unlock(osal_mutex_t mutex_hdl)
{
  return (k_mutex_unlock(mutex_hdl) == 0);
}

//--------------------------------------------------------------------+
// QUEUE API
//--------------------------------------------------------------------+

// role device/host is used by OS NONE for mutex (disable usb isr) only
#define OSAL_QUEUE_DEF(_role, _name, _depth, _type) \
  static _type _name##_##buf[_depth];\
  osal_queue_def_t _name = { .depth = _depth, .item_sz = sizeof(_type), .buf = _name##_##buf };

typedef struct
{
  uint16_t depth;
  uint16_t item_sz;
  void*    buf;

  struct k_mem_slab slab;
  struct k_fifo fifo;
}osal_queue_def_t;

typedef osal_queue_def_t *osal_queue_t;

static inline osal_queue_t osal_queue_create(osal_queue_def_t* qdef)
{
  int rc = 0;

  rc = k_mem_slab_init(&qdef->slab, qdef->buf, qdef->item_sz, qdef->depth);
  if (rc) return NULL;

  k_fifo_init(&qdef->fifo);
  return &qdef;
}

static inline bool osal_queue_receive(osal_queue_t qhdl, void* data)
{
  int rc = 0;

  void *new_item = k_fifo_get(&qhdl->fifo, K_FOREVER);
  if (new_item == NULL) return false;

  memcpy(data, new_item, qhdl->item_sz);
  k_mem_slab_free(&qhdl->slab, &new_item);
  return true;
}

static inline bool osal_queue_send(osal_queue_t qhdl, void const * data, bool in_isr)
{
  void in_isr;
  void *new_item = NULL;
  int rc = 0;

  rc = k_mem_slab_alloc(&qhdl->slab, &new_item, K_NO_WAIT);
  if (rc) return false;

  memcpy(new_item, data, qhdl->item_sz);
  rc = k_fifo_alloc_put(&qhdl->fifo, new_item);

  if (rc) {
    k_mem_slab_free(&qhdl->slab, &new_item);
    return false;
  }

  return true;
}

static inline bool osal_queue_empty(osal_queue_t qhdl)
{
  return (k_fifo_is_empty(&qhdl->fifo) == 0);
}

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_OSAL_ZEPHYR_H_ */
