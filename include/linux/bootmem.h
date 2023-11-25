#ifndef _BOOTMEM_H
#define _BOOTMEM_H
#include <linux/init.h>
#include <linux/cache.h>
#include <asm-i386/page.h>
#include <asm-i386/dma.h>
#include <linux/mmzone.h>

extern void __init reserve_bootmem(unsigned long addr, unsigned long size);
extern unsigned long __init init_bootmem(unsigned long addr, unsigned long memend);
extern void __init free_bootmem(unsigned long addr, unsigned long size);

/* bootmem.c */
extern unsigned long max_low_pfn;

/* bootmem.c */
extern unsigned long min_low_pfn;

/* 它与Linux内核在系统启动早期的内存分配有关。在系统启动并且内存管理子系统还没有完全初始化之前，
内核需要一种简单和有效的方式来分配内存，这就是所谓的"boot-time memory allocator"，或简称为"bootmem分配器"。
bootmem_data结构体用于描述每个节点（例如在NUMA系统中的内存节点）上的bootmem分配器的状态和配置，其实就是一个管理位图的结构体。
一旦内存管理子系统（例如，buddy system）完全初始化并准备好接管，这个bootmem分配器就会被淘汰。
该结构体抽象了一个引导内存分配器 */
typedef struct bootmem_data
{
    unsigned long node_boot_start; /* 此内存节点上可用于bootmem分配器的第一个物理地址 */
    unsigned long node_low_pfn;    /* 节点上的最后一个物理页框号。它可以帮助确定节点的物理内存范围，这个值不会超过896MB */
    /* 这是一个位图，其中的每一位代表节点上的一个物理内存页。
    这个位图包括那些因为硬件原因或其他因素被标记为不可用的“洞”或“hole” */
    void *node_bootmem_map;
    /* 这两个字段是优化字段，用于跟踪最近一次分配的位置，从而加速连续的分配请求。 */
    unsigned long last_offset; /* 可用地址的起始偏移。这个偏移量用于确定在当前页面是否还有足够的空间来满足新的内存分配请求 */
    unsigned long last_pos;    /* 最后一次成功的内存分配在内存页中的位置（页帧号） */
} bootmem_data_t;

/* mm/bootmem.c */
extern void *__init __alloc_bootmem(unsigned long size, unsigned long align, unsigned long goal);

/* mm/bootmem.c */
extern void *__init __alloc_bootmem_node(pg_data_t *pgdat, unsigned long size, unsigned long align, unsigned long goal);

/* 分配一定数量的低端内存页面。这里的“低端内存”通常指的是物理地址较低的内存区域，
这部分内存通常由操作系统的早期引导代码所使用。参数 x 指定要分配的字节数。
此宏调用 __alloc_bootmem 函数，将页面大小（PAGE_SIZE）作为对齐参数，以及 0 作为内存区域的起始地址。 */
#define alloc_bootmem_low_pages(x) __alloc_bootmem((x), PAGE_SIZE, 0)

/* 在特定的内存节点上分配引导内存。这是在NUMA（非统一内存访问）系统架构中常见的，
pgdat 参数是对应的内存节点数据结构，而 x 是要分配的字节数。
展开后，SMP_CACHE_BYTES（32） 作为对齐参数，__pa(MAX_DMA_ADDRESS)（16MB） 作为分配起始地址。 */
#define alloc_bootmem_node(pgdat, x) __alloc_bootmem_node((pgdat), (x), SMP_CACHE_BYTES, __pa(MAX_DMA_ADDRESS))

/* 分配一定数量的低端内存空间，传入x是要分配的空间大小，展开后，SMP_CACHE_BYTES（32） 作为对齐参数，
0作为预期分配地址*/
#define alloc_bootmem_low(x) __alloc_bootmem((x), SMP_CACHE_BYTES, 0)

#endif /* _BOOTMEM_H */