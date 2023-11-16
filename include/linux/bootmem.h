#ifndef _BOOTMEM_H
#define _BOOTMEM_H
#include <linux/init.h>


extern unsigned long __init init_bootmem(unsigned long addr, unsigned long memend);

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
    unsigned long last_offset; /* 这两个字段是优化字段，用于跟踪最近一次分配的位置，从而加速连续的分配请求。 */
    unsigned long last_pos;
} bootmem_data_t;

#endif /* _BOOTMEM_H */