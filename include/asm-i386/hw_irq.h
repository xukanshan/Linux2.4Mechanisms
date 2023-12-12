#ifndef _ASM_I386_HW_IRQ_H
#define _ASM_I386_HW_IRQ_H

/* 定义了系统调用使用的中断向量号 */
#define SYSCALL_VECTOR 0x80

/* 第一个外部中断向量的编号，因为前32个中断是intel保留给cpu内部使用 */
#define FIRST_EXTERNAL_VECTOR 0x20

/* 用于将传入的x转换成字符串，比如传入0，就转换成"0" */
#define __STR(x) #x
#define STR(x) __STR(x)

/* 将一个nr转换成nr_interrupt这种形式 */
#define IRQ_NAME2(nr) nr##_interrupt(void)

/* 将一个nr转换成IRQnr这种形式 */
#define IRQ_NAME(nr) IRQ_NAME2(IRQ##nr)

/* 在中断处理中保存执行环境，并准备内核用的数据段选择子 */
#define SAVE_ALL                                  \
    "cld\n\t"                                     \
    "pushl %es\n\t"                               \
    "pushl %ds\n\t"                               \
    "pushl %eax\n\t"                              \
    "pushl %ebp\n\t"                              \
    "pushl %edi\n\t"                              \
    "pushl %esi\n\t"                              \
    "pushl %edx\n\t"                              \
    "pushl %ecx\n\t"                              \
    "pushl %ebx\n\t"                              \
    "movl $" STR(__KERNEL_DS) ",%edx\n\t"         \
                              "movl %edx,%ds\n\t" \
                              "movl %edx,%es\n\t"

/* 构建的 common_interrupt 函数是大多数中断处理流程的统一入口点。
当发生中断时，系统首先跳转到这个入口点，然后 common_interrupt 函数负责保存当前状态、
确定中断源，并调用 do_IRQ 函数来处理特定的中断。call_do_IRQ 在内联汇编代码中，
这个标签仅仅是一个跳转点（或者说是一个地址），用于在汇编代码中进行流程控制。
这个标签的作用范围限定在汇编代码内部，对C语言代码不可见 */
#define BUILD_COMMON_IRQ()                                              \
    asmlinkage void call_do_IRQ(void);                                  \
    __asm__(                                                            \
        "\n" __ALIGN_STR "\n"                                           \
        "common_interrupt:\n\t" SAVE_ALL                                \
        "pushl $ret_from_intr\n\t" SYMBOL_NAME_STR(call_do_IRQ) ":\n\t" \
                                                                "jmp " SYMBOL_NAME_STR(do_IRQ));

/* 生成中断处理函数的宏定义。
这里-256的目的是为了将IRQ号转换成负数。因为系统调用与中断服务共用一部分子程序，
而系统调用在这个位置放的是系统调用号，为了区分系统调用号和中断号，所以将中断号转换成负数，
和正的系统调用号做区分，详见《情景分析》p213 */
#define BUILD_IRQ(nr)                                          \
    asmlinkage void IRQ_NAME(nr);                              \
    __asm__(                                                   \
        "\n"__ALIGN_STR                                        \
        "\n" SYMBOL_NAME_STR(IRQ) #nr "_interrupt:\n\t"        \
                                      "pushl $" #nr "-256\n\t" \
                                      "jmp common_interrupt");

/* arch/i386/kernel/i8259.c */
extern void disable_8259A_irq(unsigned int irq);

/* arch/i386/kernel/i8259.c */
extern void enable_8259A_irq(unsigned int irq);

#endif /* _ASM_I386_HW_IRQ_H */