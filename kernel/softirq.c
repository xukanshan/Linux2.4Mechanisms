#include <linux/mm.h>
#include <linux/kernel_stat.h>
#include <linux/interrupt.h>
#include <linux/smp_lock.h>
#include <linux/init.h>

/* 用于存储指向不同下半部处理函数的指针的数组，共32个 */
static void (*bh_base[32])(void);

/* 初始化底半部（bottom half）处理（应该叫做中断下半部分处理）参数：
nr 通常是下半部分处理的标识符，
routine 代表下半部分要执行的任务 */
void init_bh(int nr, void (*routine)(void))
{
    /* 将 routine 函数的地址赋值给 bh_base 数组的 nr 索引处，从而将指定的下半部处理函数与其标识符关联起来 */
    bh_base[nr] = routine;
    mb(); /* 确保之前的对 bh_base 数组的赋值在CPU上完成后，才会执行后续的操作 */
}

/* 设置一个小任务（struct tasklet_struct），参数：
t 代表要设置的小任务
func指向小任务节激活时要执行的函数。
data 是传递给小任务函数的数据。 */
void tasklet_init(struct tasklet_struct *t, void (*func)(unsigned long), unsigned long data)
{
    t->func = func;           /* 设置小任务要执行的函数 */
    t->data = data;           /* 设置小任务执行时需要的数据 */
    t->state = 0;             /* 初始化小任务的状态 */
    atomic_set(&t->count, 0); /* 初始化小任务的引用计数 */
}

/* 统一的底半部处理函数，在内部再执行具体的底半部处理函数，
其地位等同于do_IRQ处理硬件中断，暂未实现 */
static void bh_action(unsigned long nr)
{
}

/* 这个数组用于存储和管理一组预定义的软中断，这些软中断用于处理各种下半部任务 */
static struct softirq_action softirq_vec[32] __cacheline_aligned;

/* 创建了一个锁用来互斥操作softirq_action数组，并且将锁初始化为解锁状态 */
static spinlock_t softirq_mask_lock = SPIN_LOCK_UNLOCKED;

/* 此数组用来管理每个cpu的中断状态 */
irq_cpustat_t irq_stat[NR_CPUS];

/* 在内核的软中断向量表中注册一个新的软中断处理函数，参数：
nr：软中断的编号，
action：指向软中断处理函数的指针，
data：传递给软中断处理函数的数据*/
void open_softirq(int nr, void (*action)(struct softirq_action *), void *data)
{
    unsigned long flags; /* 存储中断状态 */
    int i;               /* 循环迭代 */

    spin_lock_irqsave(&softirq_mask_lock, flags); /* 获取自旋锁的同时关闭本地中断，保存当前中断状态到flags */
    softirq_vec[nr].data = data;                  /* 设置软中断执行所需的数据 */
    softirq_vec[nr].action = action;              /* 设置软中断要执行的函数 */

    for (i = 0; i < NR_CPUS; i++)                      /* 遍历每个cpu */
        softirq_mask(i) |= (1 << nr);                  /* 在相应的 CPU 上启用了这个软中断 */
    spin_unlock_irqrestore(&softirq_mask_lock, flags); /* 释放自旋锁，并恢复之前保存的中断状态 */
}

/* 这个数组用于存储和管理一组预定底半部处理函数 */
struct tasklet_struct bh_task_vec[32];

/* 统一的普通优先级小任务处理函数，在内部再执行具体的小任务处理函数，
其地位等同于do_IRQ处理硬件中断，暂未实现 */
static void tasklet_action(struct softirq_action *a)
{
}

/* 统一的高优先级小任务处理函数，在内部再执行具体的小任务处理函数，
其地位等同于do_IRQ处理硬件中断，暂未实现 */
static void tasklet_hi_action(struct softirq_action *a)
{
}

/* 初始化了软中断机制（中断下半部分处理）：让32个底半部执行函数都指向统一的底半部处理函数bh_action，
在软中断中注册了统一的普通优先级、高优先级小任务处理函数 */
void __init softirq_init()
{
    int i; /* 循环迭代 */
    for (i = 0; i < 32; i++)
        /* 让32个底半部执行函数都指向统一的底半部处理函数bh_action */
        tasklet_init(bh_task_vec + i, bh_action, i);
    /* 在软中断中注册统一普通优先级小任务处理函数 */
    open_softirq(TASKLET_SOFTIRQ, tasklet_action, NULL);
    /* 在软中断中注册统一高优先级小任务处理函数 */
    open_softirq(HI_SOFTIRQ, tasklet_hi_action, NULL);
}