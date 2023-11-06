#ifndef _LINUX_SERIALP_H
#define _LINUX_SERIALP_H
/* 头文件名：serial private
 * 名字意为一个私有的头文件，不是为了被外部模块或驱动程序使用，
 * 该头文件提供没有高级功能的串行驱动程序支持。
 */

#include <linux/termios.h>

/* 这个结构体在Linux内核中是对一个串行端口的抽象。用于Linux内核中的异步串行驱动的支持，串行通信可以是同步或异步的
 * 名称“async”来源于“asynchronous”（异步）。
 * 在驱动程序中，它包含了管理和操作一个物理串行端口所需的所有信息。
 * 通过这种抽象，内核代码可以与串行硬件进行互动而不必关心硬件的具体细节，
 * 这使得代码更加清晰、通用，并且易于维护。
 * 这种抽象还使得驱动能够以统一的方式支持多种硬件，
 * 只要这些硬件遵循相同的基本接口或协议。
 * 其中硬件的具体实现细节被封装在一组统一的接口背后。
 * 通过struct async_struct，Linux内核可以提供统一的串行通信功能，
 * 同时支持多种串行设备和配置。
 */

struct async_struct
{
    // int magic;                  /* 通常用于运行时检查结构体是否已正确初始化或是否被损坏 */
    unsigned long port; /* 串行端口的I/O端口地址 */
    // int hub6;                   /* 标志位，用于指示是否是一个Hub-6设备 */
    // int flags;                  /* 用于存储各种标志位，通常用于状态标记等 */
    // int xmit_fifo_size;         /* 串行端口的发送FIFO的大小 */
    // struct serial_state *state; /* 指向描述串行端口硬件信息的serial_state结构体的指针 */
    // struct tty_struct *tty;     /* 指向代表终端设备的tty_struct结构体的指针 */
    // int read_status_mask;       /* 这两个通常用于掩码接收字符的状态位 */
    // int ignore_status_mask;
    // int timeout;     /* 通信超时值 */
    // int quot;        /* 通常用于波特率的分频 */
    // int x_char;      /* 用于存储xon/xoff流控字符 */
    // int close_delay; /* 这两个关联关闭端口的延迟和等待时间 */
    // unsigned short closing_wait;
    // unsigned short closing_wait2;
    // int IER;                   /* Interrupt Enable Register */
    // int MCR;                   /* Modem control register */
    // int LCR;                   /* Line control register */
    // int ACR;                   /* 16950 Additional Control Reg. */
    // unsigned long event;       /* 用于通知事件的标志位 */
    // unsigned long last_active; /* 通常记录端口最后一次活动的时间 */
    // int line;                  /* 串行线路号 */
    // int blocked_open;          /* # 记录阻塞打开操作的数量 */
    // long session;              /* 这两个通常关联到打开串行端口的进程 */
    // long pgrp;
    // struct circ_buf xmit; /* 用于管理传输数据的环形缓冲区的circ_buf结构 */
    // spinlock_t xmit_lock; /* 一个自旋锁，用于同步访问传输环形缓冲区 */
    u8 *iomem_base;      /* 许多串行接口都是8位的。在进行MMIO时，设备的物理地址需要映射到虚拟地址空间，以便CPU能够访问。
                             iomem_base指向的就是这样一个虚拟地址空间中的起始地址 */
    u16 iomem_reg_shift; /* 在有些体系结构中，硬件寄存器的地址可能不是连续的。为了到达下一个寄存器，
                            可能需要在地址上加上一个偏移量。这个偏移量并不总是与寄存器的实际大小相匹配，尤其是在不同的硬件设计中。
                            iomem_reg_shift表示访问连续的硬件寄存器时地址需要左移的位数。
                            例如，如果每个寄存器实际上占用的是一个字（word，通常是16位或2字节）的空间，
                            即使它们只是8位宽的寄存器，那么iomem_reg_shift的值可能就是1（因为每次地址增加2即可到达下一个寄存器），
                            这样的设计使得驱动程序能够以一种与物理布局无关的方式来访问串行端口的硬件寄存器 */
    int io_type;         /* 描述I/O类型的标识符 */
    //     struct tq_struct tqueue; /* 用于底半部处理的任务队列 */
    // #ifdef DECLARE_WAITQUEUE     /* 这六个用于进程等待的队列头或者队列指针 */
    //     wait_queue_head_t open_wait;
    //     wait_queue_head_t close_wait;
    //     wait_queue_head_t delta_msr_wait;
    // #else
    //     struct wait_queue *open_wait;
    //     struct wait_queue *close_wait;
    //     struct wait_queue *delta_msr_wait;
    // #endif
    //     struct async_struct *next_port; /* 这两个链接到其他async_struct的指针，形成一个链表，方便管理多个串行端口 */
    //     struct async_struct *prev_port;
};

#endif /* _LINUX_SERIALP_H */