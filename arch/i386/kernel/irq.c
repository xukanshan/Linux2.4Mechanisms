#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/timex.h>
#include <linux/malloc.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/proc_fs.h>
#include <asm-i386/io.h>
#include <asm-i386/system.h>
#include <asm-i386/bitops.h>
#include <asm-i386/delay.h>
#include <asm-i386/desc.h>
#include <asm-i386/irq.h>

/* 记录虚假中断发生的次数 */
volatile unsigned long irq_err_count;

/* 存储系统中所有中断请求的描述符，它为系统中每个可能的中断提供了必要的数据结构和管理方式 */
irq_desc_t irq_desc[NR_IRQS] __cacheline_aligned = {[0 ... NR_IRQS - 1] = {0, &no_irq_type, NULL, 0, SPIN_LOCK_UNLOCKED}};

/* 一个hw_interrupt_type结构体内的startup 函数，什么都没做 */
static unsigned int startup_none(unsigned int irq) { return 0; }

/* 一个hw_interrupt_type结构体内的指针指向的函数，什么都没做 */
static void disable_none(unsigned int irq) {}

/* 一个hw_interrupt_type结构体内的指针指向的函数，什么都没做 */
static void enable_none(unsigned int irq) {}

/* 一个hw_interrupt_type结构体内的ack 函数，什么都没做 */
static void ack_none(unsigned int irq) {}

/* 将hw_interrupt_type结构体内的shutdown 函数指针指向disable_none */
#define shutdown_none disable_none

/* 将hw_interrupt_type结构体内的end 函数指针指向enable_none */
#define end_none enable_none

/* 一个hw_interrupt_type已初始化好的实例，用于一些结构体内hw_interrupt_type指针初始化 */
struct hw_interrupt_type no_irq_type = {
    "none",
    startup_none,
    shutdown_none,
    enable_none,
    disable_none,
    ack_none,
    end_none};

/* 暂时空着 */
asmlinkage unsigned int do_IRQ(struct pt_regs regs)
{
    return 1;
}

/* 将中断动作（struct irqaction）插入到对应的irq动作队列中, 参数：
irq，表示中断的引脚偏移；
new，表示新的中断处理动作 */
int setup_irq(unsigned int irq, struct irqaction *new)
{
    int shared = 0;                        /* 用于标记是否有多个动作共享同一个IRQ */
    unsigned long flags;                   /* 存中断状态 */
    struct irqaction *old, **p;            /* 用于处理现有的中断动作 */
    irq_desc_t *desc = irq_desc + irq;     /* 指向当前IRQ描述符的指针 */
    spin_lock_irqsave(&desc->lock, flags); /* 获取自旋锁，关闭中断，并保存中断状态到flags中 */
    p = &desc->action;                     /* 取出当前IRQ描述符内的irqaction指针 */
    if ((old = *p) != NULL)                /* 检查是否已经有中断动作与此IRQ相关联 */
    {                                      /* 如果有 */
        /* 判断当前IRQ描述符对应的的中断处理（也就是中断引脚）是否可以共享 */
        if (!(old->flags & new->flags &SA_SHIRQ))
        {                                               /* 如果不能共享 */
            spin_unlock_irqrestore(&desc->lock, flags); /* 释放自旋锁，并恢复之前保存的中断状态 */
            return -EBUSY;                              /* 返回 -EBUSY（一个错误码，表示资源忙） */
        }
        /* 来到这里说明当前IRQ描述符对应的中断引脚可用共享 */
        do /* 找到中断动作链表最末位的中断动作的 */
        {
            p = &old->next;
            old = *p;
        } while (old);
        shared = 1; /* 表示已经有多个中断动作共享一个IRQ */
    }
    *p = new; /* 在中断动作链表最某位插入新的中断动作 */

    if (!shared)         /* 判断是否已经有多个中断动作共享一个IRQ */
    {                    /* 如果现在当前IRQ有一个中断动作，说明该IRQ第一次设置，就进行对应设置 */
        desc->depth = 0; /* depth为0，表示该IRQ启用 */
        /* 清除禁用、自动检测和等待 */
        desc->status &= ~(IRQ_DISABLED | IRQ_AUTODETECT | IRQ_WAITING);
        desc->handler->startup(irq); /* 启用中断引脚 */
    }
    spin_unlock_irqrestore(&desc->lock, flags); /* 释放自旋锁，并恢复之前保存的中断状态 */
    return 0;                                   /* 返回0，表示成功设置了IRQ */
}