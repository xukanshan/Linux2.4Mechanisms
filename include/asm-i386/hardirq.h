#ifndef _ASM_I386_HARDIRQ_H
#define _ASM_I386_HARDIRQ_H

#include <linux/threads.h>
#include <linux/irq.h>

/* 对单个 CPU 中断状态的抽象 */
typedef struct
{
    /* 用于追踪当前激活的软中断, 它是一个位掩码，每一位代表一个不同的软中断 */
    unsigned int __softirq_active;
    /* 这个字段用于存储当前被屏蔽的软中断。它同样是一个位掩码，每一位对应一个软中断 */
    unsigned int __softirq_mask;
    /* 追踪当前 CPU 上本地中断的嵌套深度。每次进入中断处理程序时增加，退出时减少。如果此值大于零，表示当前正在处理硬中断 */
    unsigned int __local_irq_count;
    /* 踪当前正在执行的中断下部分的数量 */
    unsigned int __local_bh_count;
    /* 追踪当前正在执行的系统调用数量 */
    unsigned int __syscall_count;
    /* 追踪非屏蔽中断（NMI）的数量。非屏蔽中断是一种高优先级的中断，通常用于紧急情况，如硬件故障 */
    unsigned int __nmi_count;
} ____cacheline_aligned irq_cpustat_t;

/* 这个里面定义了extern irq_cpustat_t irq_stat[];需要用到irq_cpustat_t的定义，
所以必须放在irq_cpustat_t定义之后 */
#include <linux/irq_cpustat.h>

#endif /* _ASM_I386_HARDIRQ_H */