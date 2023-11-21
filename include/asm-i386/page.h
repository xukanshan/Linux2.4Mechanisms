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
这种情况在 Linux 内核开发中并不少见，且通常不会影响编译和运行。但这里为了优雅，防止编译器警告，加了个#include <linux/kernel.h> */
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



/* 全局页目录（Page Global Directory）表项内容 */
typedef struct
{
    unsigned long pgd;
} pgd_t;

/* 页中间目录（Page Middle Directory）表项内容 */
typedef struct
{
    unsigned long pmd;
} pmd_t;

/* 页表（Page Table Entry）表项内容 */
typedef struct
{
    unsigned long pte_low;
} pte_t;

/* 用于表示一个页表表项的属性位 */
typedef struct
{
    unsigned long pgprot;
} pgprot_t;

/* 获取一个pgd表项的内容，传入x是pgd_t类型 */
#define pgd_val(x) ((x).pgd)

/* 获取一个pmd表项的内容，传入x是pmd_t类型 */
#define pmd_val(x) ((x).pmd)

/* 获取一个ptd表项的内容，传入x是pte_t类型 */
#define pte_val(x) ((x).pte_low)

/* 获取一个表项的属性位，传入x是pgprot_t类型 */
#define pgprot_val(x) ((x).pgprot)

/* 这些宏定义用于创建页表条目。它们将一个给定的值转换为相应类型的页表条目。
这些宏中，花括号 { (x) } 用于创建一个临时的结构体或联合体实例。
这种语法在 C 语言中被称为复合字面量（compound literal）。
可以在不需要显式声明和初始化一个结构体变量的情况下，直接创建并初始化结构体。
复合字面量本质上是用于创建和初始化特定类型的实例。这个类型可以是结构体、数组、联合体等复杂数据类型。
因此，在使用复合字面量之前，必须确保相应的类型已经在代码中被定义。
比如：假设我们有一个表示二维点的结构体：
typedef struct {
    int x;
    int y;
} Point;
在没有复合字面量的情况下，如果想创建并初始化一个 Point 类型的变量，会这样做：
Point p;
p.x = 10;
p.y = 20;
使用复合字面量，可以在一行中完成同样的操作
Point p = (Point) { .x = 10, .y = 20 };
*/
/* 直接创建一个 pte_t 实例，并初始化其值为x */
#define __pte(x) ((pte_t){(x)})

/* 直接创建一个 pmd_t 实例，并初始化其值为x */
#define __pmd(x) ((pmd_t){(x)})

/* 直接创建一个 pgd_t 实例，并初始化其值为x */
#define __pgd(x) ((pgd_t){(x)})

/* 直接创建一个 prog_t 实例，并初始化其值为x */
#define __pgprot(x) ((pgprot_t){(x)})

#endif /* __ASSEMBLY__ */

/* 任何数字，只要去与这个，就会低12位就会变成0，即向下取整4KB的整数倍 */
#define PAGE_MASK (~(PAGE_SIZE - 1))

#endif /* _ASM_I386_PAGE_H */