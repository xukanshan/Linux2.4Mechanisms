#ifndef _LINUX_SERIAL_REG_H
#define _LINUX_SERIAL_REG_H
/* 这个头文件定义了与串行通信相关的寄存器。
这些寄存器是UART（Universal Asynchronous Receiver/Transmitter，通用异步接收/发送器）的一部分，
用于控制串行端口的操作。
该头文件允许内核开发者和模块通过标准名称来操作串行端口的硬件寄存器，
而不是直接使用硬编码的值，
这样提高了代码的可读性和可维护性。
在UART的寄存器映射中，
不同的寄存器可以共享相同的偏移地址，
因为它们是通过不同的方式访问的。
这种设计是通过寄存器选择机制实现的，
这允许节省空间和简化硬件设计。
比如，Linux2.4中将UART_DLM，UART_IER，和UART_FCTR 都被定义为 1，
这意味着它们在寄存器地址映射中具有相同的偏移量，
但实际上它们是根据不同的控制位或模式来区分的。
比如对于UART_DLM通过设置线路控制寄存器（LCR）中的DLAB（Divisor Latch Access Bit）位来实现访问
这些宏定义不会冲突，
因为它们访问相同偏移量的不同寄存器取决于其他控制寄存器的设置。
在相同的地址上可以定义多个逻辑寄存器，
但任何时候只有在特定模式下才能访问其中的一个。
这样，同一个地址在不同模式下可以代表不同的寄存器*/

/* 对UART（通用异步接收/发送器）的传输缓冲区的一个寄存器偏移量的定义
当UART的DLAB（Divisor Latch Access Bit）位为0时，这个寄存器用于装载要发送的数据字节。
DLAB是用来访问波特率除数寄存器的一个位，当设置为1时可以修改波特率除数，设置为0时则可以进行正常的数据传输。
TX"通常是"Transmit"（发送）的简写。相对应的，"RX"则是"Receive"（接收）的简写 */
#define UART_TX 0

/* 代表中断使能寄存器（interruption enbale register）在 UART 寄存器集内的偏移地址。 */
#define UART_IER 1

/* 线路状态寄存器在 UART 寄存器集内的偏移地址 */
#define UART_LSR 5

/* 串行通信中线路状态寄存器中的中断指示位，Break interrupt indicator，
如果置位，表示收到了发送方发送的发生意外，需要中止传输信号 */
#define UART_LSR_BI 0x10

/* 表示串行端口的发送器状态的的标志位 Transmitter empty */
#define UART_LSR_TEMT 0x40

/* 表示串行通信的发送保持寄存器状态的标志位， Transmit-hold-register empty */
#define UART_LSR_THRE 0x20

#endif /* _LINUX_SERIAL_REG_H */