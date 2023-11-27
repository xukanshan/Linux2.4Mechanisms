#ifndef _ASM_I386_ATOMIC_H
#define _ASM_I386_ATOMIC_H

/* 属于#ifdef CONFIG_SMP 下的 #else ，在原子操作中获取锁 */
#define LOCK ""

/* 用来表示一个数的相关操作应该是原子的 */
typedef struct
{
    volatile int counter;
} atomic_t;

/* 原子操作是指只要执行，就必须执行完毕，不能由其他任务在执行途中执行同样操作，
而赋值操作从这个意义上本就天然是原子操作 */
#define atomic_set(v, i) (((v)->counter) = (i))

/* 定义一个数据原子初始化方式，还是那句话，赋值本就是原子操作 */
#define ATOMIC_INIT(i) \
    {                  \
        (i)            \
    }

static __inline__ void atomic_inc(atomic_t *v)
{
    __asm__ __volatile__(
        LOCK "incl %0"
        : "=m"(v->counter)
        : "m"(v->counter));
}

#endif /* _ASM_I386_ATOMIC_H */