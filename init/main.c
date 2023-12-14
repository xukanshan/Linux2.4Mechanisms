#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/smp_lock.h>
#include <linux/bootmem.h>
#include <asm-i386/io.h>

/* arch/i386/kernel/setup.c */
extern void setup_arch(char **);

/* arch/i386/kernel/i8259.c */
extern void init_IRQ(void);

/* arch/i386/kernel/time.c */
extern void time_init(void);

/* kernel/softirq.c */
extern void softirq_init(void);

asmlinkage void __init start_kernel(void)
{
    char *command_line;
    lock_kernel();             /* 只让主cpu进行后续初始化工作，其他cpu就被锁住，但是我们这是个用于单核系统，这函数什么都没有做 */
    setup_arch(&command_line); /* 进行体系化初始工作，主要是内存资源管理与寻址相关 */
    trap_init();               /* 设定intel cpu的保留中断与系统调用中断，为每个cpu设置必要的状态和环境 */
    /* 初始化主从8259A PIC，设置8295A的中断处理函数集合，设置和中断请求描述符表，
    设置中断门描述符对应的处理函数，设置时钟中断为100HZ 。设定IRQ2的中断动作为空动作，并打开中断引脚2（用于级联的） */
    init_IRQ();
    /* 初始化pcb哈希表，初始化五层时间轮，注册几个下半部分处理函数，
    将当前任务的TLB设置为懒惰模式 */
    sched_init();
    time_init(); /* 注册时钟中断的中断动作 */
    /* 初始化了软中断机制（中断下半部分处理）：让32个底半部执行函数都指向统一的底半部处理函数bh_action，
    在软中断中注册了统一的普通优先级、高优先级小任务处理函数 */
    softirq_init();
    /* 初始化 slab 分配器: 核心就是初始化了cache_cache（总slab缓存池，
    管理所有的slab缓存池，也就是kmem_cache_t结构体的分配） */
    kmem_cache_init();
    while (1)
        ;
}