#ifndef _ASM_I386_DESC_H
#define _ASM_I386_DESC_H

/* 第一个任务状态段（TSS）条目在全局描述符表（GDT）中的索引是12。具体布局可参照head.S代码最后定义的GDT表。 */
#define __FIRST_TSS_ENTRY 12

/* 利用给定的TSS编号n来计算在GDT中的索引位置
(n) << 2：这部分是将n向左位移两位，等同于将n乘以4。
+ __FIRST_TSS_ENTRY：这部分是将之前位移的结果加上一个基础值__FIRST_TSS_ENTRY。这个基础值代表GDT中第一个TSS条目的索 */
#define __TSS(n) (((n) << 2) + __FIRST_TSS_ENTRY)

/* 编译.S文件时，加上-D__ASSEMBLY__参数，等同于在编译.S文件时在最开始添加了#define __ASSEMBLY__ 1这一行
这样.S文件就可以不识别这个定义，结构体定义编译gcc编译汇编时无法识别，所以要这么做 */
#ifndef __ASSEMBLY__

#include <linux/smp.h>
#include <asm-i386/ldt.h>

struct desc_struct
{
    unsigned long a, b;
};

/* 定义第一个LDT对应的GDT描述符在GDT表中的偏移 */
#define __FIRST_LDT_ENTRY (__FIRST_TSS_ENTRY + 1)

/* 利用给定的LDT编号n来计算在GDT中的索引位置 */
#define __LDT(n) (((n) << 2) + __FIRST_LDT_ENTRY)

/* 这两个都定义在head.S中，定义的是要加载进入GDTR与IDTR的48位数据 */
extern struct desc_struct *idt, *gdt;

/* 定义在head.S中 */
extern struct desc_struct gdt_table[];

/* 用于表示要加载进入GDRT与IDTR的48位数据的结构 */
struct Xgt_desc_struct
{
    unsigned short size;
    unsigned long address __attribute__((packed));
};

/* gdt指向48位要加载进入GDTR数组中的偏移两字节的位置（head.S中的SYMBOL_NAME(gdt)），
所以地址要-2字节指向48位要加载数据的起始 */
#define gdt_descr (*(struct Xgt_desc_struct *)((char *)&gdt - 2))

/* 同上gdt_descr的解释 */
#define idt_descr (*(struct Xgt_desc_struct *)((char *)&idt - 2))

/* arch/i386/kernel/traps.c */
extern void set_tss_desc(unsigned int n, void *addr);

/* 加载TR寄存器的值，该寄存器指向GDT中的一个描述符，以指向当前任务的TSS */
#define load_TR(n) __asm__ __volatile__("ltr %%ax" ::"a"(__TSS(n) << 3))

/* 加载LDTR寄存器的值，该寄存器值指向GDT中的一个描述符，以指向当前任务的LDT */
#define __load_LDT(n) __asm__ __volatile__("lldt %%ax" ::"a"(__LDT(n) << 3))

/* arch/i386/kernel/traps.c */
extern struct desc_struct default_ldt[];

/* arch/i386/kernel/traps.c */
extern void set_ldt_desc(unsigned int n, void *addr, unsigned int size);

/* 加载该任务的LDT表到LDTR中，如果该任务没有LDT表，
就加载默认的LDT表到LDTR中，不需要在这个头文件中包含sched.h，只需要在使用这个头文件的.c文件中
在包含这个desc.h头文件之前包含sched.h就可以了。参数：
mm：该任务的虚拟内存管理结构体*/
static inline void load_LDT(struct mm_struct *mm)
{
    int cpu = smp_processor_id();          /* 获取当前 CPU 的编号 */
    void *segments = mm->context.segments; /* 指向该进程 LDT 的指针 */
    int count = LDT_ENTRIES;               /* LDT 中的条目数 */

    if (!segments) /* 检查该进程有没有自己的 LDT */
    {
        segments = &default_ldt[0]; /* 设置为指向默认 LDT  */
        count = 5;                  /* 设置为默认 LDT 的条目数 */
    }

    set_ldt_desc(cpu, segments, count); /* 在GDT当中设置指向ldt的描述符 */
    __load_LDT(cpu);                    /* 加载 LDT 到 LDTR寄存器 */
}

#endif /* !__ASSEMBLY__ */
#endif /* _ASM_I386_DESC_H */
