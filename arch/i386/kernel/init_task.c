#include <linux/sched.h>

/* 这个变量就是swapper进程的pcb，同时变量+8192的位置也是用作启动时候的栈区 */
union task_union init_task_union __attribute__((__section__(".data.init_task"))) = {INIT_TASK(init_task_union.task)};