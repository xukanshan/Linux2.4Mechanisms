/* 包含与串行通信（serial communication）相关的代码 ,
 * 只包含这些基础功能的实现代码，而与特定硬件平台相关的代码会在另外的文件中。
 * 例如，针对不同硬件架构或特定的串行通信硬件，会有专门的代码文件实现其特定的配置和控制逻辑。
 */

#include <asm-i386/io.h>
#include <linux/serialP.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>

/* 定义这个宏是为了可以随时方便修改到底是否内联 */
#define _INLINE_ inline

/* 通过串行接口发送一个字节的数据。函数根据info结构体中的io_type成员变量的值选择不同的数据发送方式。
 * info 是一个指向async_struct结构体的指针，通常它包含了与特定串行端口相关的配置和状态信息。
 * offset 用来指定想要访问或修改的寄存器的偏移量
 * value 将要写入特定寄存器的数据值
 */
static _INLINE_ void serial_out(struct async_struct *info, int offset, int value)
{
    /* 根据不同的I/O类型选择不同的数据发送实现 */
    switch (info->io_type)
    {
    /* 对于映射到内存的串行I/O操作。 */
    case SERIAL_IO_MEM:
        writeb(value, (unsigned long)info->iomem_base + (offset << info->iomem_reg_shift));
        break;

    /* 默认情况，使用基本的I/O端口访问。 */
    default:
        outb(value, info->port + offset);
    }
}

/* 通过串行接口发送一个字节的数据。函数根据info结构体中的io_type成员变量的值选择不同的数据发送方式。
 * info 是一个指向async_struct结构体的指针，通常它包含了与特定串行端口相关的配置和状态信息。
 * offset 用来指定想要访问或修改的寄存器的偏移量
 */
static _INLINE_ unsigned int serial_in(struct async_struct *info, int offset)
{
    /* 根据不同的I/O类型选择不同的数据接收实现 */
    switch (info->io_type)
    {
    /* 对于映射到内存的串行I/O操作。 */
    case SERIAL_IO_MEM:
        return readb((unsigned long)info->iomem_base + (offset << info->iomem_reg_shift));

    /* 默认情况，使用基本的I/O端口访问。 */
    default:
        return inb(info->port + offset);
    }
}

/* 用于记录串行通信中发生了意外，需要终止传输 */
static int lsr_break_flag;

/* 定义一个宏，表示串行端口的发送器与串行通信的发送保持寄存器都为空，
那么这时候就可以发送数据 */
#define BOTH_EMPTY (UART_LSR_TEMT | UART_LSR_THRE)

/* 这个函数的作用是等待，
直到串行端口的发送器（transmitter，它负责处理并发送所有的串行数据，
在发送过程中，发送器会在数据前添加起始位，在数据后添加停止位，有时还会包括奇偶校验位）空闲，
并且发送保持寄存器（transmit holding register，他是UART（或其他类型的串行接口）中的一个缓冲区，
用于暂存待发送的数据。当软件写入一个字节到发送保持寄存器时，发送器会从中取出这个字节，
并在物理线路上串行地发送出去）为空，
这意味着可以发送新的数据 */
static inline void wait_for_xmitr(struct async_struct *info)
{
    /* status用于存储读取的串行端口状态，
    tmout是一个超时计数器，用来防止在硬件故障的情况下代码陷入无限循环 */
    unsigned int status, tmout = 1000000;

    do
    {
        /* 读取线路状态寄存器（UART_LSR）的当前值，在串行通信中，这个寄存器的状态用来表明串行端口的当前状态
        其中包含了许多标志位，用于指示各种类型的线路状态信息，比如：
        发送保持寄存器为空（THRE，Transmitter Holding Register Empty）：这个位如果设置，表示CPU可以发送新的数据，因为发送保持寄存器是空的。
        发送器空（TEMT，Transmitter Empty）：如果这个位设置了，表示发送器不仅为空，而且所有正在进行的串行传输都已完成。
        接收数据就绪（DR，Data Ready）：这个位如果设置了，表示接收缓冲寄存器中有数据可以读取。
        接收器溢出错误（OE，Overrun Error）：表示接收缓冲区已满，但仍有数据到来，导致一些数据丢失。
        奇偶校验错误（PE，Parity Error）：如果设置了，表示接收的数据中奇偶校验位不正确。
        帧错误（FE，Framing Error）：如果设置了，表示接收的数据中停止位未被检测到。
        中断标志（BI，Break Interrupt）：当这个位被设置时，表示检测到发送方发送的一个断开信号。*/
        status = serial_in(info, UART_LSR);

        /* 检查UART_LSR_BI位是否设置。如果设置了，
        这意味着检测到了断开（break）条件，
        然后设置一个名为lsr_break_flag的变量 */
        if (status & UART_LSR_BI)
            lsr_break_flag = UART_LSR_BI;

        /* 防止在串行端口不可用时代码陷入死循环 */
        if (--tmout == 0)
            break;
        /* 当两个条件UART_LSR_THRE（发送保持寄存器空）和UART_LSR_TEMT（发送器空）的位都被设置时，才不用继续耗时 */
    } while ((status & BOTH_EMPTY) != BOTH_EMPTY);
}

/* 名字由来，asynchronous serial console，
异步串行通信（asynchronous serial communication）是一种串行通信方法，
它不需要发送和接收两端的时钟同步。
它会被初始化为一个默认的串行控制台配置。 */
static struct async_struct async_sercons;

/* 这个函数的作用是将一个字符串安全地输出到串行端口，
同时在输出过程中禁用了串行端口的中断，
以防止在字符串发送过程中发生的中断可能导致的冲突。
发送完成后，它会恢复原始的中断使能寄存器的设置。
函数接收三个参数：
指向 console 结构体的指针 co，
指向要写入的字符数组的指针 s，
以及要写入的字符数 count。
没有使用到传入参数 co, 可能是为了将来的拓展，虽然在当前的代码版本中还未使用*/
static void serial_console_write(struct console *co, const char *s, unsigned count)
{
    static struct async_struct *info = &async_sercons;
    int ier; /*  ier 用于保存中断使能寄存器（Interrupt Enable Register）的值 */
    unsigned i;

    ier = serial_in(info, UART_IER);  /* serial_in 函数读取当前的中断使能寄存器（UART_IER）的值并保存在 ier 中。 */
    serial_out(info, UART_IER, 0x00); /* 然后通过 serial_out 函数写入 0x00 到 UART_IER 寄存器，这样做实际上是禁用了串行端口的所有中断。*/

    for (i = 0; i < count; i++, s++) /* 遍历字符串中的每个字符。 */
    {
        wait_for_xmitr(info);          /* 在发送每个字符前，调用 wait_for_xmitr 函数等待传输寄存器为空，确保我们可以发送下一个字符。 */
        serial_out(info, UART_TX, *s); /* 通过 serial_out 函数，将当前字符输出到传输寄存器（UART_TX）。 */
        if (*s == 10)                  /* 如果当前字符是换行符（ASCII码为10），）。 */
        {
            wait_for_xmitr(info);
            serial_out(info, UART_TX, 13); /* 则还需要输出一个回车符（ASCII码为13 */
        }
    }

    wait_for_xmitr(info);
    serial_out(info, UART_IER, ier); /* 恢复之前保存的中断使能寄存器的值。 */
}
