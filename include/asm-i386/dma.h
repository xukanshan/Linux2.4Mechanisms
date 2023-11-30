#ifndef _ASM_I386_DMA_H
#define _ASM_I386_DMA_H
/* DMA区域：在Linux和其他操作系统中，
DMA（Direct Memory Access）是一种让外部硬件（如磁盘控制器、网络卡等）直接与主内存进行数据交换的技术，而不需要CPU的介入。
这样可以提高数据传输的效率，因为CPU可以在DMA操作进行时执行其他任务。
早期的一些设备，由于其硬件寻址限制，只能访问最低的16MB物理内存。
这就意味着当CPU或操作系统与这些设备交互时，必须确保相关的数据位于这个0-16MB的范围内。
因此，为了确保这些设备能够正常工作，操作系统需要在物理内存中预留一块0-16MB的空间。
所以，当一个早期的设备需要进行DMA操作时，操作系统从这个DMA区域中分配内存。
这确保了数据位于设备可以访问的物理地址范围内。 */

#include <linux/spinlock.h> 
#include <asm-i386/io.h>         

/* 指定直接内存访问（DMA）能够访问的最大物理地址 */
#define MAX_DMA_ADDRESS (PAGE_OFFSET + 0x1000000)

#endif /* _ASM_I386_DMA_H */