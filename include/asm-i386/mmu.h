#ifndef _ASM_I386_MMU_H
#define _ASM_I386_MMU_H

/* 存储与特定处理器相关的重要内存管理信息，
i386 架构没有 MMU 上下文，所以我们在这里只放置了段信息(实际上是LDT表)。*/
typedef struct
{
    void *segments;
} mm_context_t;

#endif /* _ASM_I386_MMU_H */