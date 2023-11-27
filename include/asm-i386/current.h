#ifndef _ASM_I386_CURRENT_H
#define _ASM_I386_CURRENT_H

struct task_struct; /* 一个前向声明 */

/* 返回当前正在运行的线程或进程的task_struct，
工作原理就是用当前栈顶地址去掉低13位，也就是向下8k取整 */
static inline struct task_struct *get_current(void)
{
    struct task_struct *current;
    __asm__("andl %%esp, %0; " : "=r"(current) : "0"(~8191UL));
    return current;
}

/* 通过current可以找到当前正在运行的线程或进程的task_struct */
#define current get_current()

#endif /* _ASM_I386_CURRENT_H */