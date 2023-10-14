#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

# define INIT_TASK_SIZE	2048*sizeof(long)

/* 未来再来填充这个, 设定为8K大小 */
struct task_struct
{
    unsigned long tmp[2048];
};

#define INIT_TASK(tsk)  \
{                       \
    .tmp = {0}          \
}

/*
 * union task_union 被设计用来合并并管理 task_struct 和内核栈所在的一个连续8K内存空间，
 * 因为通常我们将 task_struct（8k的低位置开始处） 和其对应的内核栈（栈底在8k结束，栈向地位置扩展）放在一起
 */
union task_union
{
    struct task_struct task;
    unsigned long stack[INIT_TASK_SIZE / sizeof(long)];
};




#endif /* _LINUX_SCHED_H */