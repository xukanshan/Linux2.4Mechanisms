#ifndef _ASM_I386_PROCESSOR_H
#define _ASM_I386_PROCESSOR_H


#include <asm-i386/segment.h>
#include <asm-i386/page.h>
#include <asm-i386/types.h>
#include <linux/threads.h>

#define IO_BITMAP_SIZE 32

/* tss的结构 */
struct tss_struct
{
    unsigned short back_link, __blh;
    unsigned long esp0;
    unsigned short ss0, __ss0h;
    unsigned long esp1;
    unsigned short ss1, __ss1h;
    unsigned long esp2;
    unsigned short ss2, __ss2h;
    unsigned long __cr3;
    unsigned long eip;
    unsigned long eflags;
    unsigned long eax, ecx, edx, ebx;
    unsigned long esp;
    unsigned long ebp;
    unsigned long esi;
    unsigned long edi;
    unsigned short es, __esh;
    unsigned short cs, __csh;
    unsigned short ss, __ssh;
    unsigned short ds, __dsh;
    unsigned short fs, __fsh;
    unsigned short gs, __gsh;
    unsigned short ldt, __ldth;
    unsigned short trace, bitmap;
    unsigned long io_bitmap[IO_BITMAP_SIZE + 1]; /* I/O 位图用于控制任务（例如进程）对 I/O 端口的访问权限 */
    /* 用于确保整个 tss_struct 结构体与 CPU 的缓存行对齐。这种对齐是为了提高缓存效率。
    通过添加这个填充数组，可以确保整个 TSS 结构体的大小正好是一个缓存行的大小（ 0x100 字节），
    这有助于减少 CPU 缓存中的冲突和不必要的数据迁移 */
    unsigned long __cacheline_filler[5];
};

/* 初始化时候用的栈顶，就是那个swapper进程的pcb的8KB末尾 */
#define init_stack (init_task_union.stack)

/* I/O 位图在 TSS 结构体的末尾，并通过 bitmap 字段指定其在 TSS 中的偏移量，
当 bitmap 字段设置为 INVALID_IO_BITMAP_OFFSET(估计是超过了TSS对应的GDT描述符limit)时，
表示 TSS 中没有有效的 I/O 位图 */
#define INVALID_IO_BITMAP_OFFSET 0x8000


/* 用于初始化一个 tss_struct 结构体的实例 */
#define INIT_TSS                                                           \
    {                                                                      \
        0, 0,                                       /* back_link, __blh */ \
            sizeof(init_stack) + (long)&init_stack, /* esp0 */             \
            __KERNEL_DS, 0,                         /* ss0 */              \
            0, 0, 0, 0, 0, 0,                       /* stack1, stack2 */   \
            0,                                      /* cr3 */              \
            0, 0,                                   /* eip,eflags */       \
            0, 0, 0, 0,                             /* eax,ecx,edx,ebx */  \
            0, 0, 0, 0,                             /* esp,ebp,esi,edi */  \
            0, 0, 0, 0, 0, 0,                       /* es,cs,ss */         \
            0, 0, 0, 0, 0, 0,                       /* ds,fs,gs */         \
            __LDT(0), 0,                            /* ldt */              \
            0, INVALID_IO_BITMAP_OFFSET,            /* tace, bitmap */     \
        {                                                                  \
            ~0,                                                            \
        } /* ioperm(permission) */                                         \
    }

/* arch/i386/kernel/init_task.c */
extern struct tss_struct init_tss[NR_CPUS];

/* 这个结构体是进程的上下文切换时保存和恢复 CPU 状态的关键部分。
它包含了执行线程所需的所有 CPU 信息，确保线程能够正确地保存和继续执行 */
struct thread_struct
{
    /* 这是内核模式下的栈指针。当从用户模式切换到内核模式时，CPU 使用这个值作为栈指针 */
    unsigned long esp0;
    unsigned long eip;
    unsigned long esp;
    unsigned long fs;
    unsigned long gs;
    unsigned long debugreg[8]; /* 这是硬件调试寄存器，用于设置断点和执行跟踪等调试操作 */
    /* 些字段用于存储异常或故障的详细信息。
    cr2 存储引起页错误的地址，trap_no 是异常号，error_code 是与异常相关的错误代码 */
    unsigned long cr2, trap_no, error_code;
    /* 这是用于存储浮点单元（FPU）状态的联合体。它包括保存所有浮点寄存器、控制和状态寄存器的信息 */
    // union i387_union i387;
    /* 如果线程在 virtual 8086 模式下运行，这个指针会指向相关信息的结构体。
    Virtual 8086 模式允许在保护模式下运行实模式（8086 模式）程序 */
    // struct vm86_struct *vm86_info;
    // unsigned long screen_bitmap; /* 在虚拟 8086 模式下，这可能用于存储屏幕映射信息 */
    /* 这些字段也是用于虚拟 8086 模式的，用于存储各种模式特定的标志和状态信息 */
    // unsigned long v86flags, v86mask, v86mode, saved_esp0;
    /* 输入/输出权限的标记。这决定了线程可以访问哪些 I/O 端口 */
    int ioperm;
    /* I/O 位图，用于管理线程对 I/O 端口的访问权限。这个位图直接映射到每个端口的访问权限 */
    unsigned long io_bitmap[IO_BITMAP_SIZE + 1];
};

/* 用于初始化一个struct thread_struct实例 */
#define INIT_THREAD                                                \
    {                                                              \
        0,                                                         \
            0, 0, 0, 0,                                            \
            {[0 ... 7] = 0}, /* debugging registers */             \
            0, 0, 0,                                               \
            /* {{0,},}, */ /* 387 state */ /* 0, 0, 0, 0, 0, 0, */ \
            0,                                                     \
        {                                                          \
            ~0,                                                    \
        } /* io permissions */                                     \
    }

#endif /* _ASM_I386_PROCESSOR_H */