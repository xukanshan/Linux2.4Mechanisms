#ifndef _LINUX_TERMIOS_H
#define _LINUX_TERMIOS_H
/* 名字由来：terminal I/O settings
termios.h是一个与终端I/O（输入/输出）相关的头文件。通常与终端控制相关的API和结构有关。
它定义了终端I/O接口和控制函数，这些函数允许程序员更改终端接口的行为，
例如设置波特率、字符大小、硬件流控制等。它实际上是一个封装层，确保当内核代码或用户空间程序包含<linux/termios.h>时，
它们能够得到正确的终端I/O定义 <asm/termios.h>将包含真正的体系结构相关的终端I/O定义*/

#include <linux/types.h>

#endif /* _LINUX_TERMIOS_H */
