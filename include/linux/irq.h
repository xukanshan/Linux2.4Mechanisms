#ifndef _LINUX_IRQ_H
#define _LINUX_IRQ_H

#include <linux/cache.h>
#include <linux/spinlock.h>
#include <asm-i386/irq.h>
#include <asm-i386/hw_irq.h>

/* 中断请求描述符内的中断状态描述字段 */
/* 中断请求正在被处理 */
#define IRQ_INPROGRESS 1
/* 中断请求禁用 */
#define IRQ_DISABLED 2
/* 中断请求（IRQ）正在被自动检测，统启动时或当添加新的硬件设备时，
内核会尝试自动检测并配置相应的IRQ。在这个过程中，IRQ_AUTODETECT 标志被设置，表明该IRQ正处于自动检测状态 */
#define IRQ_AUTODETECT 16 /* IRQ is being autodetected */
/* 表示IRQ尚未被触发或观察到，常用于自动检测过程中 */
#define IRQ_WAITING 32 /* IRQ not yet seen - for autodetection */

/* 为不同类型的硬件中断提供了一个标准化的处理接口，允许内核以统一的方式管理和处理这些中断 */
struct hw_interrupt_type
{
    const char *typename; /* 存储该中断类型的名称或描述 */
    /* 启动特定的硬件中断时被调用，在启动中断处理之前，它会进行必要的初始化工作。
    它接受一个中断号（irq）作为参数，并返回一个无符号整数，通常用于表示操作的状态 */
    unsigned int (*startup)(unsigned int irq);
    /* 关闭特定的IRQ。在不再需要处理某个特定的中断时，它会进行必要的清理工作。它接受一个中断号作为参数 */
    void (*shutdown)(unsigned int irq);
    /* 用于启用某个特定的IRQ。它通常会取消屏蔽对应的中断，允许它被处理。它接受一个中断号作为参数 */
    void (*enable)(unsigned int irq);
    /* 用于禁用某个特定的IRQ。它通常会屏蔽对应的中断，防止它被处理。它接受一个中断号作为参数 */
    void (*disable)(unsigned int irq);
    /* 用于确认（acknowledge）特定的硬件中断。在某些中断控制器中，
    接收到中断后需要发送一个确认信号。它接受一个中断号作为参数 */
    void (*ack)(unsigned int irq);
    /* 用于结束对某个IRQ的处理。这通常是在IRQ处理完成后调用的，以恢复对该IRQ的正常响应。接受一个中断号作为参数 */
    void (*end)(unsigned int irq);
    /* 用于设置特定中断的CPU亲和性（affinity）。这允许系统将中断处理绑定到特定的CPU上。
    它接受一个中断号和一个CPU掩码作为参数 */
    void (*set_affinity)(unsigned int irq, unsigned long mask);
};

typedef struct hw_interrupt_type hw_irq_controller;

/* 对一个中断源的完整抽象，包含了处理和管理该中断所需的所有关键信息 */
typedef struct
{
    unsigned int status; /* 存储中断的状态信息，如中断是否被禁用、是否正在处理中 */
    /* 一个指向硬件特定中断控制器的指针，该控制器包含了处理该中断所需的底层硬件操作的函数，例如如何启用或禁用中断 */
    hw_irq_controller *handler;
    /* 包含中断处理程序（即中断服务例程）的信息。当中断发生时，内核会调用 action 指向的函数来处理该中断 */
    struct irqaction *action;
    /* 用于追踪特定中断被禁用的次数，只有当 depth 降回到0时，中断才真正被启用 */
    unsigned int depth;
    spinlock_t lock; /* 一个自旋锁，用于保护该中断描述符的访问 */
} ____cacheline_aligned irq_desc_t;

/* arch/i386/kernel/irq.c */
extern volatile unsigned long irq_err_count;

/* arch/i386/kernel/irq.c */
extern irq_desc_t irq_desc[NR_IRQS];

/* arch/i386/kernel/irq.h */
extern hw_irq_controller no_irq_type;

#endif /* _LINUX_IRQ_H */