#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <asm-i386/system.h>
#include <asm-i386/io.h>
#include <asm-i386/atomic.h>
#include <asm-i386/desc.h>

/* idt表，包含256个中断门描述符，在Linux2.4中，这里还指定了段名字，
以在链接时指定页面对齐，来简化对奔腾 F0 0F 故障的解决方法，但我们这里没有必要
数组的第一个desc_struct实例被初始化，a和b成员都被设置为0。
根据C语言的初始化规则，数组剩余的元素也会被初始化为相同类型的默认值，即每个成员都是0。
它会被放在bss段中，bss段用于存储未初始化的数据或者初始化为零的静态变量。
之所以这样做是为了节省空间，因为.bss段在磁盘上的可执行文件中不占用实际的空间，
它只在程序被加载到内存时由操作系统分配并清零 */
struct desc_struct idt_table[256] = {
    {0, 0},
};

/* 负责将正确的值写入 IDT 的特定位置，以设置idt描述符，参数：
gate_addr：要设置的门描述符的地址。
type：门描述符的类型。
dpl：决定了哪些特权级别的代码可以使用这个门。
addr：中断或异常处理程序的地址 */
#define _set_gate(gate_addr, type, dpl, addr)                                             \
    do                                                                                    \
    {                                                                                     \
        int __d0, __d1;                                                                   \
        __asm__ __volatile__("movw %%dx,%%ax\n\t"                                         \
                             "movw %4,%%dx\n\t"                                           \
                             "movl %%eax,%0\n\t"                                          \
                             "movl %%edx,%1"                                              \
                             : "=m"(*((long *)(gate_addr))),                              \
                               "=m"(*(1 + (long *)(gate_addr))), "=&a"(__d0), "=&d"(__d1) \
                             : "i"((short)(0x8000 + (dpl << 13) + (type << 8))),          \
                               "3"((char *)(addr)), "2"(__KERNEL_CS << 16));              \
    } while (0)

/* 设置一个陷阱门以指向特定处理函数，陷阱门与中断门的区别在于陷阱门进入之后中断门状态维持不变，
陷阱门的处理函数是可中断的。dpl为0是为了不让用户态程序触发。参数：
n 是中断向量号，addr 是处理该中断的函数的地址 */
static void __init set_trap_gate(unsigned int n, void *addr)
{
    /* 调用函数完成对应idt描述符的设置 */
    _set_gate(idt_table + n, 15, 0, addr);
}

/* 设置一个中断门以指向特定处理函数，中断门进入之后，中断状态会自动关闭（不响应其他中断）。
dpl为0，因为硬件产生中断（cpu异常或外部硬件）dpl会被忽略。参数：
n 是中断向量号，addr 是处理该中断的函数的地址 */
void set_intr_gate(unsigned int n, void *addr)
{
    /* 调用函数完成对应idt描述符的设置 */
    _set_gate(idt_table + n, 14, 0, addr);
}

/* 设置一个陷阱门以指向特定处理函数，用陷阱门而不用中断门，是因为系统调用是可以中断的。
dpl为3是因为此陷阱门是用于用户态程序触发。参数：
n 是中断向量号，addr 是处理该中断的函数的地址 */
static void __init set_system_gate(unsigned int n, void *addr)
{
    /* 调用函数完成对应idt描述符的设置 */
    _set_gate(idt_table + n, 15, 3, addr);
}

/* 用于在内存中设置任务状态段（TSS）或局部描述符表（LDT）的描述符, 参数：
n：要设定的描述符相对于GDT表的起始偏移。
addr：描述符要指向的地址。
limit：描述符的limit字段。
type：描述符的类型 */
#define _set_tssldt_desc(n, addr, limit, type) \
    __asm__ __volatile__("movw %w3,0(%2)\n\t"  \
                         "movw %%ax,2(%2)\n\t" \
                         "rorl $16,%%eax\n\t"  \
                         "movb %%al,4(%2)\n\t" \
                         "movb %4,5(%2)\n\t"   \
                         "movb $0,6(%2)\n\t"   \
                         "movb %%ah,7(%2)\n\t" \
                         "rorl $16,%%eax"      \
                         : "=m"(*(n)) : "a"(addr), "r"(n), "ir"(limit), "i"(type))

/* 在GDT中设置一个描述符指向tss，参数：
n：表明是cpu的编号（因为tss是一个cpu一个）
addr：tss的地址 */
void set_tss_desc(unsigned int n, void *addr)
{
    /* 调用函数来完成gdt表对应的描述符设置 */
    _set_tssldt_desc(gdt_table + __TSS(n), (int)addr, 235, 0x89);
}

/* 默认的LDT表 */
struct desc_struct default_ldt[] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};

/* 在GDT中设置一个描述符指向ldt，参数：
n：表明是cpu的编号（因为ldtr是一个cpu一个）
addr：ldt的地址
size: ldt内的条目数量 */
void set_ldt_desc(unsigned int n, void *addr, unsigned int size)
{
    _set_tssldt_desc(gdt_table + __LDT(n), (int)addr, ((size << 3) - 1), 0x82);
}

/* 设定intel cpu的保留中断与系统调用中断，为每个cpu设置必要的状态和环境 */
void __init trap_init(void)
{
    /* 这些全部都是在进行异常处理函数设置，很多异常不属于主要机制，故用NULL替代 */
    set_trap_gate(0, NULL);   /* 原：&divide_error，用于处理除零错误（Divide Error） */
    set_trap_gate(1, NULL);   /* 原：&debug，用于处理调试异常 */
    set_intr_gate(2, NULL);   /* 原：&nmi，设置非屏蔽中断（NMI），它不能被普通的中断屏蔽机制屏蔽 */
    set_system_gate(3, NULL); /* 原：&int3，用于处理断点异常（Breakpoint Exception），通常用于调试 */
    set_system_gate(4, NULL); /* 原：&overflow，设置溢出异常的处理 */
    set_system_gate(5, NULL); /* 原：&bounds，设置边界范围超出异常的处理 */
    set_trap_gate(6, NULL);   /* 原：&invalid_op，设置非法操作码异常的处理 */
    set_trap_gate(7, NULL);   /* 原：&device_not_available, 设备不可用异常的处理 */
    set_trap_gate(8, NULL);   /* 原：&double_fault，设置双重故障异常的处理 */
    set_trap_gate(9, NULL);   /* 原：&coprocessor_segment_overrun，协处理器段溢出的处理 */
    set_trap_gate(10, NULL);  /* 原：&invalid_TSS，无效任务状态段（TSS）的处理 */
    set_trap_gate(11, NULL);  /* 原：&segment_not_present，段不存在异常的处理 */
    set_trap_gate(12, NULL);  /* 原：&stack_segment，栈段异常的处理 */
    set_trap_gate(13, NULL);  /* 原：&general_protection， 通用保护异常的处理 */
    set_trap_gate(14, NULL);  /* 原：&page_fault，页错误异常。这个要支持，暂未实现 */
    set_trap_gate(15, NULL);  /* 原：&spurious_interrupt_bug，虚假中断的处理 */
    set_trap_gate(16, NULL);  /* 原：&coprocessor_error，协处理器错误的处理 */
    set_trap_gate(17, NULL);  /* 原：&alignment_check)， 对齐检查异常的处理 */
    set_trap_gate(18, NULL);  /* 原：&machine_check，机器检查异常的处理 */
    set_trap_gate(19, NULL);  /* 原：&simd_coprocessor_error，SIMD 协处理器错误的处理 */
    /* 原：&system_call，设置系统调用的处理。允许用户空间程序通过一个特定的中断来请求内核服务 */
    set_system_gate(SYSCALL_VECTOR, NULL);

    cpu_init(); /* 每个 CPU 设置必要的状态和环境 */
}
