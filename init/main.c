#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/delay.h>
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

/* this should be approx 2 Bo*oMips to start (note initial shift), and will
   still work even if initially too large, it will just take slightly longer */
/*  对CPU 每个 jiffy能够执行的空循环次数的一个初始估计 */
unsigned long loops_per_jiffy = (1 << 12);

/* Loops Per Second Precision，这个值表示计算一定时间（通常是一个 jiffy）内 CPU 能执行多少个空循环的
精度，每增加一位精度，校准过程的平均时间就会增加大约 1.5/HZ 秒 */
#define LPS_PREC 8

/* 确定系统中每个 jiffy 内 CPU 可以执行多少个空循环，即 loops_per_jiffy 的值 */
void __init calibrate_delay(void)
{
    unsigned long ticks, loopbit; /* ticks用于存储时间，loopbit用于二分查找 */
    int lps_precision = LPS_PREC; /* 确定二分查找的精度 */

    loops_per_jiffy = (1 << 12); /* 初始设置 loops_per_jiffy 的值为 2 的 12 次方 */

    printk("Calibrating delay loop... "); /* 打印提示信息 */
    while (loops_per_jiffy <<= 1)         /* 左移 loops_per_jiffy 的值，并在每次循环中检是否为0（防止溢出） */
    {
        ticks = jiffies;         /* 记录当前的 jiffies */
        while (ticks == jiffies) /* 等待 jiffies 值变化，即等待下一个时钟滴答开始 */
            ;
        ticks = jiffies;          /* 再次记录当前的 jiffies 值 */
        __delay(loops_per_jiffy); /* 执行一个loops_per_jiffy次数的空循环 */
        /* 计算 __delay 函数执行期间 jiffies 值的变化量，即函数执行所花费的时钟滴答数 */
        ticks = jiffies - ticks;
        if (ticks)
            /* 如果 ticks 非零，意味着 __delay 函数的执行时间至少跨越了一个时钟滴答（jiffy），
            此时循环终止。这表示找到了一个大致的 loops_per_jiffy 值，使得 __delay 执行时间超过一个 jiffy */
            break;
    }
    /* 将 loops_per_jiffy 右移一位，即将其减半。因为前面的循环可能使 loops_per_jiffy 略大于实际需要的值，
    现在新的值会小于实际需求值，所以后面要用二分法去增加逼近 */
    loops_per_jiffy >>= 1;
    loopbit = loops_per_jiffy; /* 初始化 loopbit 变量，其最高位用于在二分查找中标记正在检测的位 */
    /* 不断将 loopbit 右移一位，并检查最高位对 loops_per_jiffy 值的影响来进行二分查找 */
    while (lps_precision-- && (loopbit >>= 1))
    {
        loops_per_jiffy |= loopbit; /* 通过或操作将要检测位设置为 1 */
        ticks = jiffies;            /* 记录当前的 jiffies */
        while (ticks == jiffies)    /* 等待 jiffies 值变化，即等待下一个时钟滴答开始 */
            ;
        ticks = jiffies;          /* 再次记录当前的 jiffies 值 */
        __delay(loops_per_jiffy); /* 执行一个loops_per_jiffy次数的空循环 */
        if (jiffies != ticks)
            /* 进入if，说明 __delay 函数的运行时间超过了一个时钟滴答 */
            loops_per_jiffy &= ~loopbit; /* 通过与非当前位的操作，将 loops_per_jiffy 中当前正在检测的位清零 */
        /* 如果没有执行上面的if，说明将要检测的位置为1后，增加值并没有让__delay的运行超过一个滴答，所以可以保留下来 */
    }
    /* 计算出一个所谓的 "BogoMIPS" 值（一个非正式的性能指标），并输出 */
    printk("%lu.%02lu BogoMIPS\n", loops_per_jiffy / (500000 / HZ), (loops_per_jiffy / (5000 / HZ)) % 100);
}

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
    // sti();   /* 中断机制并不完善，暂不执行 */
    /* 确定系统中每个 jiffy 内 CPU 可以执行多少个空循环，即 loops_per_jiffy 的值，
    由于其依赖现在并不不完善时钟中断，暂不执行 */
    // calibrate_delay();
    /* 清空0页，释放引导期间分配的内存，以及释放用于引导内存分配的位图本身，
    计算被保留的页面数，计算内核代码段，数据段，初始化段大小 */
    mem_init();
    while (1)
        ;
}