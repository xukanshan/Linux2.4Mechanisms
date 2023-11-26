#ifndef _LINUX_SMP_H
#define _LINUX_SMP_H

/* 位于#ifdef CONFIG_SMP的#else下，用于获取当前cpu编号，
我们不支持多处理器，所以函数返回编号直接定义成0 */
#define smp_processor_id() 0

#endif /* _LINUX_SMP_H */