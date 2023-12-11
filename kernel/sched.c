#include <linux/mm.h>
#include <linux/init.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>
#include <asm-i386/mmu_context.h>

/* kernel/timer.c */
extern void init_timervecs(void);

/* 初始化pcb哈希表，初始化五层时间轮，注册几个下半部分处理函数，
将当前任务的TLB设置为懒惰模式 */
void __init sched_init(void)
{
    int cpu = smp_processor_id(); /* 获取当前CPU的ID */
    int nr;                       /* 循环中用作迭代变量 */

    init_task.processor = cpu; /* init_task的处理器字段为当前CPU的ID */
    /* 初始化PCB哈希表 */
    for (nr = 0; nr < PIDHASH_SZ; nr++)
        pidhash[nr] = NULL;

    init_timervecs(); /* 初始化五层定时器向量（时间轮） */

    init_bh(TIMER_BH, timer_bh);         /* 在bh_base中注册定时器的下半部分处理函数 */
    init_bh(TQUEUE_BH, tqueue_bh);       /* 在bh_base中注册任务队列下半部分处理函数 */
    init_bh(IMMEDIATE_BH, immediate_bh); /* 在bh_base中注册立即执行的下半部分处理函数 */

    /* 增加init_mm的引用计数，可能因为调度系统在启动时需要确保对内核的内存映射有一个稳定的引用，
    尤其是在系统开始创建和调度多个用户空间进程之前 */
    atomic_inc(&init_mm.mm_count);
    /* 将当前任务的TLB设置为懒惰模式 */
    enter_lazy_tlb(&init_mm, current, cpu);
}