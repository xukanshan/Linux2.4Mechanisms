#ifndef _LINUX_INTERRUPT_H
#define _LINUX_INTERRUPT_H

#include <asm-i386/hardirq.h>

/* 代表了一个中断处理动作，由于多个设备会共享中断引脚，
所以对同一个中断信号的处理是不一样的，可能需要不同的程序来处理。
irq_desc_t描述了一个中断信号源，而irqaction就描述了对中断的处理动作 */
struct irqaction
{
    /* 指向当中断发生时要调用的处理函数。这个函数通常具有三个参数：
    中断号（int）、设备特定的信息（void *dev_id），以及指向寄存器状态的指针（struct pt_regs *），
    这可以提供中断发生时的处理器状态信息 */
    void (*handler)(int, void *, struct pt_regs *);
    /* 用于表示中断处理的各种属性和行为。例如，它可以指定中断是否可以共享（SA_SHIRQ），
    是否是一个快速中断处理程序（SA_INTERRUPT） */
    unsigned long flags;
    /* 这通常用于中断屏蔽。它可以指定在处理当前中断时需要屏蔽哪些其他中断 */
    unsigned long mask;
    /* 用于存储与此中断处理动作相关联的名称。这通常用于调试目的，以帮助识别不同的中断处理程序 */
    const char *name;
    /* 指向与中断处理程序相关联的设备的特定信息或数据结构。这允许中断处理程序访问它需要的设备特定数据 */
    void *dev_id;
    /* 中断处理程序可以被链接成一个链表，这允许多个处理程序按需响应同一个中断 */
    struct irqaction *next;
};

/* kernel/softirq.c */
extern void init_bh(int nr, void (*routine)(void));

/* 定义了不同的中断下半部分处理函数指针在bh_base中的下标 */
enum
{
    TIMER_BH = 0, /* 用于定时器相关的下半部处理 */
    TQUEUE_BH,    /* 用于任务队列相关的下半部处理 */
    DIGI_BH,      /* 用于特定硬件（如Digi板）的下半部处理 */
    SERIAL_BH,    /* 用于串行端口相关的下半部处理 */
    RISCOM8_BH,   /* 用于RISCOM8串行卡的下半部处理 */
    SPECIALIX_BH, /* 用于Specialix硬件的下半部处理 */
    AURORA_BH,    /* 是针对特定硬件（如Aurora）的下半部处理 */
    ESP_BH,       /* 与某些类型的SCSI控制器相关的下半部处理 */
    SCSI_BH,      /* 用于SCSI系统的下半部处理 */
    IMMEDIATE_BH, /* 用于立即执行的下半部处理 */
    CYCLADES_BH,  /* 用于Cyclades串行卡的下半部处理 */
    CM206_BH,     /* 与CM206光盘驱动器相关的下半部处理 */
    JS_BH,        /* 用于操纵杆（Joystick）设备的下半部处理 */
    MACSERIAL_BH, /* 用于Macintosh串行端口的下半部处理 */
    ISICOM_BH     /* 用于ISI串行卡的下半部处理 */
};

/* kernel/softirq.c */
extern void softirq_init(void);

/* 对一个小任务的抽象 */
struct tasklet_struct
{
    struct tasklet_struct *next; /* 将多个小任务链接在一起，形成一个任务队列 */
    unsigned long state;         /* 一个位字段，不同的位表示不同的状态（如是否已调度执行、是否正在执行等） */
    atomic_t count;              /* 引用计数器 */
    void (*func)(unsigned long); /* 指向小任务被激活时要执行的函数 */
    unsigned long data;          /* 存储传递给小任务函数的数据 */
};

/* 软中断动作 */
struct softirq_action
{
    void (*action)(struct softirq_action *); /* 指向一个具体的软中断处理函数 */
    void *data;                              /* 用于向软中断处理函数传递额外的数据或状态信息 */
};

/* kernel/softirq.c, 这个声明中有函数指针，函数传入参数是struct softirq_action
指针类型，所以这个声明要放在struct softirq_action之后，否则报错，我不理解 */
extern void open_softirq(int nr, void (*action)(struct softirq_action *), void *data);

/* 标识不同软中断 */
enum
{
    HI_SOFTIRQ = 0, /* 高优先级的小任务处理的软中断 */
    /* 网络传输（Network Transmit）软中断。这个软中断用于处理网络包的发送。
    当网络子系统有数据包需要发送时，它会触发这个软中断来处理发送操作 */
    NET_TX_SOFTIRQ,
    /* 网络接收（Network Receive）软中断。这个软中断用于处理网络包的接收。
    当网络接口接收到数据包时，这个软中断被触发，以便在下半部处理这些包 */
    NET_RX_SOFTIRQ,
    /* 通用小任务（tasklets）处理的软中断。用于执行普通优先级的小任务的软中断 */
    TASKLET_SOFTIRQ
};

#endif /* _LINUX_INTERRUPT_H */