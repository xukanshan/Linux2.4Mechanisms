#ifndef _ASM_I386_PAGE_H
#define _ASM_I386_PAGE_H

/* 最低一级页表所占地址位数 */
#define PAGE_SHIFT 12

/* 定义页面大小，方法是1 << 最低一级页表所占地址（12）位数 */
#define PAGE_SIZE (1UL << PAGE_SHIFT)

/* 定义内核的起始虚拟地址 */
#define __PAGE_OFFSET (0xC0000000)

#define PAGE_OFFSET ((unsigned long)__PAGE_OFFSET)

/* 将虚拟地址转换成物理地址 */
#define __pa(x) ((unsigned long)(x)-PAGE_OFFSET)

/* 将物理地址转换成虚拟地址 */
#define __va(x) ((void *)((unsigned long)(x) + PAGE_OFFSET))

#ifndef __ASSEMBLY__
/* 这里使用了printk函数，但是Linux2.4的page.h中没有任何头文件直接或间接包含了该函数的声明，
这种情况在 Linux 内核开发中并不少见，且通常不会影响编译和运行。但这里为了防止编译器警告，加了个#include <linux/kernel.h> */
#include <linux/kernel.h>

/* .byte 是一个汇编指令，用于直接在代码中插入字节。在这里，它插入了两个字节：0x0f 和 0x0b。
0x0f, 0x0b: 这两个字节组合起来代表了一个特定的 x86 指令，这个指令是 ud2（未定义指令）。
这个指令的目的是提供一种可靠的方法来生成一个无效操作码异常（Invalid Opcode Exception）。
当 CPU 执行到这个指令时，会认为它是一个非法的或未定义的操作码，并触发一个异常。
在 Linux 内核的上下文中，使用 ud2 指令的目的是在检测到严重错误（即“BUG”）时故意触发一个异常。
这样做的好处是，它会立即停止程序的执行，并且通常会导致操作系统生成核心转储（core dump），
从而使开发者可以调查是什么导致了这个异常。 */
#define BUG()                                                 \
    do                                                        \
    {                                                         \
        printk("kernel BUG at %s:%d!\n", __FILE__, __LINE__); \
        __asm__ __volatile__(".byte 0x0f,0x0b");              \
    } while (0)

#endif /* __ASSEMBLY__ */

#endif /* _ASM_I386_PAGE_H */