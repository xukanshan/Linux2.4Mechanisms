#ifndef _LINUX_IRQ_CPUSTAT_H
#define _LINUX_IRQ_CPUSTAT_H

/* kernel/softirq.c */
extern irq_cpustat_t irq_stat[];

/* 位于#ifdef CONFIG_SMP 下的 #else，用于访问管理某个cpu中断状态的结构体irq_cpustat_t的member成员，
原来是((void)(cpu), irq_stat[0].member)，编译会报错：不是一个lvalue，不可以在赋值表达式的左侧使用。
因为原来的表达方式是个逗号表达式，所以这里直接去掉了(void)(cpu) */
#define __IRQ_STAT(cpu, member) (irq_stat[0].member)

/* 用于访问管理某个cpu中断状态的结构体irq_cpustat_t的__softirq_mask成员 */
#define softirq_mask(cpu) __IRQ_STAT((cpu), __softirq_mask)

#endif /* _LINUX_IRQ_CPUSTAT_H */