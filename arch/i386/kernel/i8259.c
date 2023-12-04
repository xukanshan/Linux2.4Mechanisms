#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/malloc.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <asm-i386/system.h>
#include <asm-i386/irq.h>
#include <asm-i386/io.h>
#include <asm-i386/bitops.h>
#include <asm-i386/pgtable.h>
#include <asm-i386/delay.h>
#include <asm-i386/desc.h>
#include <linux/irq.h>

/* 生成统一的中断处理函数 */
BUILD_COMMON_IRQ()


/* 接收两个参数 x 和 y，并通过预处理器的字符串连接操作（##）将它们合并，然后传递给 BUILD_IRQ 宏。
例如，BI(0x0, 1) 会产生 BUILD_IRQ(0x01) */
#define BI(x,y) \
	BUILD_IRQ(x##y)

/* 用来生成16个连续的中断处理函数。它接收一个参数 x，然后为从 x0 到 xf 的每一个值调用 BI 宏。
例如，BUILD_16_IRQS(0x0) 会批量生成 BUILD_IRQ(0x00) 到 BUILD_IRQ(0x0f) */
#define BUILD_16_IRQS(x) \
	BI(x,0) BI(x,1) BI(x,2) BI(x,3) \
	BI(x,4) BI(x,5) BI(x,6) BI(x,7) \
	BI(x,8) BI(x,9) BI(x,a) BI(x,b) \
	BI(x,c) BI(x,d) BI(x,e) BI(x,f)


/* 展开为16个 BUILD_IRQ 调用，从 BUILD_IRQ(0x00) 到 BUILD_IRQ(0x0f)。
每个 BUILD_IRQ 调用都会生成一个中断处理函数，这些函数被设计为处理特定的中断号 */
BUILD_16_IRQS(0x0)


/* 启动8259A 中断（指的是启用硬件），为的是符合struct hw_interrupt_type
内的starup函数指针。但实际8259A不需要专门启用，所以直接调用的是enable_8259A_irq
（开启某条中断引脚线）。参数：
irq:中断引脚偏移 */
static unsigned int startup_8259A_irq(unsigned int irq)
{
    enable_8259A_irq(irq); /* 启用给定的中断 */
    return 0;
}

/* 用于结束处理 8259A PIC 的一个引脚的中断处理，参数：
irq:中断引脚偏移 */
static void end_8259A_irq(unsigned int irq)
{
    /* 检查中断是否被禁用或者是否正在处理中 */
    if (!(irq_desc[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS)))
        /* 重新启用这个中断。因为在处理某些类型的中断时，中断可能被暂时禁用，而在处理完成后需要重新启用 */
        enable_8259A_irq(irq);
}

/* 用于8259A的自旋锁，并且初始化为解锁状态 */
spinlock_t i8259A_lock = SPIN_LOCK_UNLOCKED;

/* 8259A的屏蔽某条中断引脚函数 */
#define shutdown_8259A_irq disable_8259A_irq

/* 一个函数声明，定义在本文件后面 */
void mask_and_ack_8259A(unsigned int);

/* 描述了如何处理与8259A可编程中断控制器（PIC）相关的中断 */
static struct hw_interrupt_type i8259A_irq_type = {
    "XT-PIC",
    startup_8259A_irq,
    shutdown_8259A_irq,
    enable_8259A_irq,
    disable_8259A_irq,
    mask_and_ack_8259A,
    end_8259A_irq,
    NULL};

/* 缓存的中断屏蔽寄存器的值，初始含义是关闭所有中断 */
static unsigned int cached_irq_mask = 0xffff;

/* 获取变量 y 的第 x 个字节。它首先取 y 的地址并转换为 unsigned char * 类型，
然后通过索引 [x] 来访问特定的字节。*/
#define __byte(x, y) (((unsigned char *)&(y))[x])

/* 获取 cached_irq_mask 的第0个字节，用于控制主8259A PIC */
#define cached_21 (__byte(0, cached_irq_mask))

/* 获取 cached_irq_mask 的第1个字节，用于控制从8259A PIC */
#define cached_A1 (__byte(1, cached_irq_mask))

/* 函数声明，函数定义在本文件后面部分 */
void mask_and_ack_8259A(unsigned int);


/* 初始化主从8259A PIC，设置8295A的中断处理函数集合，并在初始化完成后恢复原来的中断屏蔽状态，参数：
auto_eoi：用于控制8259A可编程中断控制器（PIC）的结束中断（End of Interrupt，EOI）模式，
自动EOI模式（auto_eoi = 1）: 在这种模式下，8259A PIC在派发中断后会自动处理结束中断信号。
这意味着处理器在处理完中断后不需要显式地向PIC发送一个EOI信号来告知它中断处理已完成。
这可以简化中断处理过程，特别是在处理快速或者高频中断时。
普通EOI模式（auto_eoi = 0）: 在这种模式下，每当中断处理完成时，处理器需要显式地向PIC发送一个EOI信号。
这是更传统的方式，允许更精细的控制中断处理的顺序和优先级。
这个函数涉及很多端口操作，具体意义可参见象书p311 */
void __init init_8259A(int auto_eoi)
{
    unsigned long flags; /* 保存中断状态 */
    /* 获取自旋锁的同时关闭本地中断，保存当前中断状态到flags */
    spin_lock_irqsave(&i8259A_lock, flags);
    outb(0xff, 0x21);       /* OCW1屏蔽所有主8259A中断 */
    outb(0xff, 0xA1);       /* OCW1屏蔽所有从8259A中断 */
    outb_p(0x11, 0x20);     /* ICW1: 边沿触发,级联8259, 需要ICW4 */
    outb_p(0x20 + 0, 0x21); /* ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27 */
    outb_p(0x04, 0x21);     /* ICW3: IR2接从片 */
    if (auto_eoi)           /* 根据 auto_eoi 参数设置自动结束中断（AEOI）模式或正常EOI模式 */
        outb_p(0x03, 0x21); /* ICW4: 8086模式, 自动EOI */
    else
        outb_p(0x01, 0x21); /* ICW4: 8086模式, 正常EOI */

    outb_p(0x11, 0xA0);     /* ICW1: 边沿触发,级联8259, 需要ICW4 */
    outb_p(0x20 + 8, 0xA1); /* ICW2: 起始中断向量号为0x28,也就是IR[8-15] 为 0x28 ~ 0x2F */
    outb_p(0x02, 0xA1);     /* ICW3: 设置从片连接到主片的IR2引脚 */
    outb_p(0x01, 0xA1);     /* ICW4: 8086模式, 正常EOI */

    if (auto_eoi) /* 根据 auto_eoi 参数，设置中断处理类型 */
        /* 自动eoi模式，这种模式下，8259A 收到第二个INTA信号后，
        就会自动将ISR中的对应BIT置为0，详见象书p313 */
        i8259A_irq_type.ack = disable_8259A_irq;
    else
        /* 手动eoi模式，需要主动给主片，从片发送eoi信号 */
        i8259A_irq_type.ack = mask_and_ack_8259A;

    udelay(100); /* 延迟100微秒，等待8259A完成初始化 */

    outb(cached_21, 0x21); /* 恢复之前保存的中断屏蔽状态 */
    outb(cached_A1, 0xA1); /* 恢复之前保存的中断屏蔽状态 */

    spin_unlock_irqrestore(&i8259A_lock, flags); /* 释放自旋锁，并恢复之前保存的中断状态 */
}

/* 初始化主从8259A PIC，设置8295A的中断处理函数集合，设置和中断请求描述符表 */
void __init init_ISA_irqs(void)
{
    int i; /* 循环计数 */

    init_8259A(0); /* 初始化主从8259A PIC，设置8295A的中断处理函数集合 */

    for (i = 0; i < NR_IRQS; i++) /* 循环遍历所有的中断请求（IRQ）描述符 */
    {
        irq_desc[i].status = IRQ_DISABLED; /* 中断的状态设置为禁用 */
        irq_desc[i].action = 0;            /* 处理函数设置为空 */
        irq_desc[i].depth = 1;             /* 设置中断的深度 */

        /* 16个传统风格的INTA-cycle中断，在早期的PC系统中，
        中断控制器（如8259A PIC）使用INTA-cycle来识别和响应最高优先级的中断请求。
        当CPU接收到一个中断信号后，它会发送一个INTA信号给中断控制器，
        以确认已经接收到中断并询问是哪个设备发出的中断请求。 */
        if (i < 16)
        {
            irq_desc[i].handler = &i8259A_irq_type; /* 处理函数设置 */
        }
        /* 中断号大于或等于16（通常是PCI中断），与传统的ISA中断不同，
        PCI系统支持更多的中断，并且它们通常是“动态填充”的 */
        else
        {
            irq_desc[i].handler = &no_irq_type; /* 处理函数设置 */
        }
    }
}


/* 用于生成中断处理函数的名称。它通过预处理器的字符串连接功能（##）来生成函数名。
例如，如果你调用 IRQ(0, 0)，它会生成 IRQ00_interrupt */
#define IRQ(x, y) IRQ##x##y##_interrupt

/* 用于生成16个中断处理函数的名称列表。它接受一个参数 x 并生成16个中断处理函数名，从 IRQ(x,0) 到 IRQ(x,f) */
#define IRQLIST_16(x)                               \
    IRQ(x, 0), IRQ(x, 1), IRQ(x, 2), IRQ(x, 3),     \
        IRQ(x, 4), IRQ(x, 5), IRQ(x, 6), IRQ(x, 7), \
        IRQ(x, 8), IRQ(x, 9), IRQ(x, a), IRQ(x, b), \
        IRQ(x, c), IRQ(x, d), IRQ(x, e), IRQ(x, f)

/* 一个函数指针数组, 每个元素都指向一个中断处理函数,
IRQLIST_16(0x0) 用于初始化数组的前16个元素，它们分别指向由 IRQ00_interrupt 到 IRQ0f_interrupt 定义的函数 */
void (*interrupt[NR_IRQS])(void) = {
    IRQLIST_16(0x0),
};

/* 初始化主从8259A PIC，设置8295A的中断处理函数集合，设置和中断请求描述符表，
设置中断门描述符对应的处理函数，设置时钟中断为100HZ */
void __init init_IRQ(void)
{
    int i;           /* 用于循环计数 */
    init_ISA_irqs(); /* 初始化主从8259A PIC，设置8295A的中断处理函数集合，设置和中断请求描述符表 */

    for (i = 0; i < NR_IRQS; i++) /* 遍历操作系统可以管理的中断号 */
    {
        int vector = FIRST_EXTERNAL_VECTOR + i;
        if (vector != SYSCALL_VECTOR) /* 0x80对应的idt描述符已经设置了 */
            set_intr_gate(vector, interrupt[i]);
    }

    /* 通过0x43端口写入8253控制字，这里0x34含义表示：选择计数器0，先读写低字节后读写高字节，
    工作方式选择2，比率发生器，计数方式为二进制。详见象书p350 */
    outb_p(0x34, 0x43);
    outb_p(LATCH & 0xff, 0x40); /* 先写入counter_value的低8位 */
    outb(LATCH >> 8, 0x40);     /* 再写入counter_value的高8位 */
}

/* 确认某个IRQ是否真正在被处理。原理：8259A 有两个主要的寄存器用于跟踪中断的状态：
中断请求寄存器（IRR）和中断服务寄存器（ISR）。IRR 维护着当前正在请求服务的中断。
ISR 维护着当前正在被服务的中断。函数从PIC的ISR寄存器读取当前值(发送OCW3, 详见象书p318)。
然后将这个值与IRQ特定的掩码进行按位与操作。如果结果非零，表明该IRQ正在被服务。
这个函数也是来判断一个中断是否是虚假中断，原理：真实中断的处理流程：硬件发送，然后IMR筛选，
仲裁器仲裁之后，8259A的IRR记录，然后发送信号给CPU，CPU回应准备好之后，
将中断移至ISR中。此时CPU再次发送信号询问中断向量号，然后回复之后，
CPU去执行中断处理函数。而虚假中断就是绕过了这个流程，由于非正常硬件状态，
发送了信号给CPU报告自己有中断要处理（IRR没有记录），然后CPU询问中断号，这个硬件状态就回复了一个虚假中断号，
所以自然ISR就没有记录。我们就可以通过这个来判断是不是虚假中断。参数：
irq：中断引脚偏移 */
static inline int i8259A_irq_real(unsigned int irq)
{
    int value;              /* 存储读取到的值 */
    int irqmask = 1 << irq; /* 生成一个位掩码，用于指定对应的IRQ */

    if (irq < 8) /* 如果IRQ号小于8，表示这是主8259A的中断 */
    {
        outb(0x0B, 0x20);            /* 切换到中断服务寄存器（ISR），这样之后可以通过端口 0x20 读取主PIC的ISR寄存器的当前值 */
        value = inb(0x20) & irqmask; /* 从ISR读取值并与irqmask进行与操作，以检查特定的IRQ是否正在被服务 */
        outb(0x0A, 0x20);            /* 将主8259A的寄存器切换回中断请求寄存器（IRR），这是为了将硬件恢复到其默认状态 */
        return value;                /* 如果这个IRQ正在被服务，返回非零值；否则返回0 */
    }
    /* 如果IRQ号大于或等于8，表示这是从8259A的中断 */
    outb(0x0B, 0xA0);                   /* 切换到中断服务寄存器（ISR） */
    value = inb(0xA0) & (irqmask >> 8); /* 检查特定的IRQ是否正在被服务 */
    outb(0x0A, 0xA0);                   /* 切换回中断请求寄存器（IRR） */
    return value;                       /* 如果这个IRQ正在被服务，返回非零值；否则返回0 */
}

/* 负责处理由8259A PIC触发的中断，包括屏蔽中断、发送结束中断信号、以及检测和处理虚假中断。
参数：要处理的中断引脚偏移
小心！8259A是一个脆弱的设备，它几乎必须按照这种方式来处理
（首先屏蔽它，然后发送EOI，而且向两个8259发送EOI的顺序很重要*/
void mask_and_ack_8259A(unsigned int irq)
{
    unsigned int irqmask = 1 << irq; /* 通过左移操作生成一个位掩码，用于指定对应的IRQ */
    unsigned long flags;             /* 用于保存中断状态 */

    spin_lock_irqsave(&i8259A_lock, flags); /* 获取自旋锁，关闭中断，并保存中断状态到flags中 */
    if (cached_irq_mask & irqmask)          /* 检测本次中断是否已经被屏蔽了 */
        goto spurious_8259A_irq;            /* 如果是已经被屏蔽的引脚产生的中断，那么按照虚假中断处理 */
    cached_irq_mask |= irqmask;             /* 处理某个引脚上发生的中断时，需要屏蔽这个引脚的中断 */

handle_real_irq:
    if (irq & 8) /* 根据IRQ号的值来区分处理主8259A或从8259A的中断 */
    {            /* 如果IRQ号大于等于8，说明它是从8259A的中断 */
        /* 一个哑操作（dummy operation），用于读取从8259A的中断屏蔽寄存器（IMR），可能用于延时 */
        inb(0xA1);
        outb(cached_A1, 0xA1); /* 让从8289A屏蔽与要处理的中断同一根引脚的中断 */
        /* 关于这个0x60其含义，参见象书p317 OCW2的设定 */
        outb(0x60 + (irq & 7), 0xA0); /* 向从8259A发送一个特定EOI信号, 告知结束某跟引脚发生的中断处理 */
        outb(0x62, 0x20);             /* 向主8259A发送一个特定EOI信号，告诉它从8259A上的IRQ2已经处理完毕 */
    }
    else /* 否则，是主8259A的中断 */
    {
        inb(0x21);              /* 哑操作，意义同上 */
        outb(cached_21, 0x21);  /* 让主8289A屏蔽与要处理的中断同一根引脚的中断 */
        outb(0x60 + irq, 0x20); /* 向主8259A发送一个特定EOI信号，0x60 是特定EOI命令，irq 是中断引脚偏移 */
    }
    spin_unlock_irqrestore(&i8259A_lock, flags); /* 释放自旋锁，并恢复之前保存的中断状态 */
    return;

spurious_8259A_irq:
    if (i8259A_irq_real(irq)) /* 调用 i8259A_irq_real 函数来确认这是否真的是一个虚假中断 */
        goto handle_real_irq; /* 如果不是，跳转回实际的中断处理代码 */

    { /* 如果确认是虚假中断 */
        static int spurious_irq_mask;
        if (!(spurious_irq_mask & irqmask)) /* 且之前没有报告过这个IRQ的虚假中断 */
        {
            printk("spurious 8259A interrupt: IRQ%d.\n", irq); /* 那么打印一条消息 */
            spurious_irq_mask |= irqmask;                      /* 还是按照真实中断一样，关闭这条引脚线 */
        }
        irq_err_count++;      /* 增加虚假中断计数器 irq_err_count */
        goto handle_real_irq; /* 结束处理中断，虽然不用这么做，但是这不会产生什么后果，而且使得处理逻辑更加统一 */
    }
}

/* 用于启用 8259A 中断，参数：
irq:中断引脚偏移 */
void enable_8259A_irq(unsigned int irq)
{
    unsigned int mask = ~(1 << irq); /* 生成一个位掩码，用于启用指定的中断引脚 */
    unsigned long flags;             /* 保存中断状态 */

    spin_lock_irqsave(&i8259A_lock, flags); /* 获取自旋锁，关闭中断，并保存中断状态到flags中 */
    cached_irq_mask &= mask;                /* 启用了特定的中断引脚 */
    /* 判断是向主PIC（端口0x21）还是从PIC（端口0xA1）发送操作 */
    if (irq & 8)
        outb(cached_A1, 0xA1); /* 发送OCW1，详见象书p316 */
    else
        outb(cached_21, 0x21);                   /* 发送OCW1，详见象书p316 */
    spin_unlock_irqrestore(&i8259A_lock, flags); /* 释放自旋锁，并恢复之前保存的中断状态 */
}

/* 用于屏蔽 8259A 中断，参数：
irq:中断引脚偏移 */
void disable_8259A_irq(unsigned int irq)
{
    unsigned int mask = 1 << irq; /* 生成一个位掩码，用于指定对应的中断引脚 */
    unsigned long flags;          /* 保存中断状态 */

    spin_lock_irqsave(&i8259A_lock, flags); /* 获取自旋锁，关闭中断，并保存中断状态到flags中 */
    cached_irq_mask |= mask;                /* 屏蔽了特定的中断引脚 */
    /* 判断是向主PIC（端口0x21）还是从PIC（端口0xA1）发送操作 */
    if (irq & 8)
        outb(cached_A1, 0xA1); /* 发送OCW1，详见象书p316 */
    else
        outb(cached_21, 0x21);                   /* 发送OCW1，详见象书p316 */
    spin_unlock_irqrestore(&i8259A_lock, flags); /* 释放自旋锁，并恢复之前保存的中断状态 */
}
