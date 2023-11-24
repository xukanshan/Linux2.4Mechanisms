#ifndef _LINUX_WAIT_H
#define _LINUX_WAIT_H

#include <linux/list.h>
#include <linux/spinlock.h>

/* 将等待队列里面的那把锁定义成了自旋锁，但是在 #if USE_RW_WAIT_QUEUE_SPINLOCK 情况下，
是定义成了读写锁 rwlock_t。自旋锁即可满足复现主要机制，故采用 */
# define wq_lock_t spinlock_t

/* 定义在 #if USE_RW_WAIT_QUEUE_SPINLOCK 的 #else 下，
用于初始化等待队列内的锁为解锁状态 */
# define WAITQUEUE_RW_LOCK_UNLOCKED SPIN_LOCK_UNLOCKED


/* 管理一个等待队列 */
struct __wait_queue_head
{
    wq_lock_t lock;
    struct list_head task_list;
};

/* 管理一个等待队列 */
typedef struct __wait_queue_head wait_queue_head_t;

/* 初始化一个等待队列 */
static inline void init_waitqueue_head(wait_queue_head_t *q)
{
    q->lock = WAITQUEUE_RW_LOCK_UNLOCKED;
    INIT_LIST_HEAD(&q->task_list);
}

#endif /* _LINUX_WAIT_H */