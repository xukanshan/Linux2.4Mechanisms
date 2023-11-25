#ifndef _LINUX_IOPORT_H
#define _LINUX_IOPORT_H
/* ioport.h 提供了一系列的函数和宏定义，用于管理和操作 I/O 端口。 */

/* 对系统中硬件资源（如 I/O 端口、内存区域）的抽象 */
struct resource
{
    const char *name;                          /* 存储资源的名称 */
    unsigned long start, end;                  /* 定义了资源的范围 */
    unsigned long flags;                       /* 用于存储关于资源的不同标志。这些标志可能包含关于资源状态（如是否已分配、是否可以共享等）的信息 */
    struct resource *parent, *sibling, *child; /* 这三个指针提供了将资源组织成树状结构的方式 */
};

/* kernel/resource.c */
extern struct resource ioport_resource;

/* kernel/resource.c */
extern struct resource iomem_resource;

/* struct resource 内表示资源类型（结构体内部flags的值）的宏定义 */
/* io资源 */
#define IORESOURCE_IO 0x00000100

/* 内存资源 */
#define IORESOURCE_MEM 0x00000200

/* 表示这部分资源已经被占用或在使用中，并且不应被其他部分或驱动程序重新分配或使用 */
#define IORESOURCE_BUSY 0x80000000

/* kernel/resource.c */
extern int request_resource(struct resource *root, struct resource *new);

#endif /* _LINUX_IOPORT_H */