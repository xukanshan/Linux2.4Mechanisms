#include <linux/sched.h>
#include <asm-i386/processor.h>
#include <linux/threads.h>
#include <linux/cache.h>

/* 这个变量就是swapper进程的pcb，同时变量+8192的位置也是用作启动时候的栈区，
并且调用INIT_TASK来初始化 */
union task_union init_task_union __attribute__((__section__(".data.init_task"))) = {INIT_TASK(init_task_union.task)};

/* 这个就是swapper的mm_struct，也是整个内核的mm_struct */
struct mm_struct init_mm = INIT_MM(init_mm);

/*每个 CPU 的 TSS 段，并且调用INIT_TSS初始化数组内每个成员，
[0 ... NR_CPUS - 1] 是一个范围指定符，它表示从数组的第一个元素（索引 0）到最后一个元素（索引 NR_CPUS - 1）*/
struct tss_struct init_tss[NR_CPUS] __cacheline_aligned = {[0 ... NR_CPUS - 1] = INIT_TSS};
