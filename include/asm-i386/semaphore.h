#ifndef _ASM_I386_SEMAPHORE_H
#define _ASM_I386_SEMAPHORE_H

#include <linux/linkage.h>
#include <asm-i386/system.h>
#include <asm-i386/atomic.h>
#include <linux/wait.h>

/* 信号量的结构定义 */
struct semaphore
{
    atomic_t count;         /* 资源的数量 */
    int sleepers;           /* 等待资源的任务数 */
    wait_queue_head_t wait; /* 等待队列 */
};

/* 设置信号量，参数：
sem：要设置的信号量
val：信号量的值 */
static inline void sema_init(struct semaphore *sem, int val)
{
    atomic_set(&sem->count, val);    /* 将信号量的 count 成员设置为传入的 val 值 */
    sem->sleepers = 0;               /* 等待信号量的线程数量设置为0 */
    init_waitqueue_head(&sem->wait); /* 初始化信号量的等待队列头 */
}

/* 初始化一个信号量，参数：
sem：要初始化的信号量 */
static inline void init_MUTEX(struct semaphore *sem)
{
    sema_init(sem, 1); /* 将信号量初始化为1，等同于互斥锁 */
}

#endif /* _ASM_I386_SEMAPHORE_H */