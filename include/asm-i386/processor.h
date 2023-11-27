#ifndef _ASM_I386_PROCESSOR_H
#define _ASM_I386_PROCESSOR_H

#include <linux/sched.h>
#include <asm-i386/segment.h>
#include <asm-i386/desc.h>
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

#endif /* _ASM_I386_PROCESSOR_H */