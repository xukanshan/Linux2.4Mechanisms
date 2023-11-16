#include <linux/init.h>
/* 这个文件所谓的 "引导内存分配器" (boot-time memory allocator)，
它在系统启动阶段分配内存，这是在常规内存管理系统（比如页式内存管理）启动之前的一种简化方式。
Linux内核启动时，需要一些临时的内存来设置各种数据结构，比如页表、内核的数据结构等。
在这个阶段，内核的正常内存管理器（如伙伴系统或slab分配器）尚未初始化，
因此需要一个简单的内存分配机制来应对这个时期的需求。bootmem.c 提供的就是这样一个机制。
一旦系统完成启动，并且标准的内存管理器（如页式内存管理器）准备就绪，
bootmem.c 中的引导内存分配器就会被淘汰，系统转而使用更高级的内存管理机制。
这种设计允许Linux内核在早期启动过程中高效地管理内存，同时为后续更复杂的内存管理奠定基础。 */

#include <linux/mmzone.h>
#include <linux/bootmem.h>
#include <asm-i386/page.h>
#include <asm-i386/io.h>
#include <linux/string.h>

/* 初始化一个节点的引导内存管理器。它设置了一个位图来跟踪哪些页面是可用的，哪些是已经被占用的。参数：
pg_data_t *pgdat: 指向一个包含节点（node）相关数据的结构，该结构抽象了一个内存节点
unsigned long mapstart: 引导内存位图的起始地址。
unsigned long start: 引导内存分配器管理的内存区域的开始页面号。
unsigned long end: 引导内存分配器管理的内存区域的结束页面号。 */
static unsigned long __init init_bootmem_core(pg_data_t *pgdat, unsigned long mapstart, unsigned long start, unsigned long end)
{
    bootmem_data_t *bdata = pgdat->bdata;            /* 获取指向引导内存管理数据结构的指针 */
    unsigned long mapsize = ((end - start) + 7) / 8; /* 向上取整计算需要多少字节来表示引导内存分配器的位图 */

    pgdat->node_next = pgdat_list; /* 以下两步是将当前节点加入到全局节点列表中。这是一个典型的链表插入操作 */
    pgdat_list = pgdat;

    /* 对mapsize 进行对齐处理，确保它是 sizeof(long) 的整数倍。这有助于提高访问效率 */
    mapsize = (mapsize + (sizeof(long) - 1UL)) & ~(sizeof(long) - 1UL);
    bdata->node_bootmem_map = phys_to_virt(mapstart << PAGE_SHIFT); /* 设置引导内存位图的虚拟地址 */
    bdata->node_boot_start = (start << PAGE_SHIFT);                 /* 设置节点的引导内存开始地址 */
    bdata->node_low_pfn = end;                                      /* 设置节点的最高页面帧号 */

    /* 初始化位图，将所有页面标记为已保留（即设置所有位为 1）。这表示所有页面默认不可用，直到显式地标记为可用 */
    memset(bdata->node_bootmem_map, 0xff, mapsize);
    return mapsize; /* 返回位图的大小。 */
}

unsigned long max_low_pfn; /* 记录内存管理系统可管理的最大物理页面号 */
unsigned long min_low_pfn; /* 记录内存管理系统可管理的起始页面帧号 */

/* 初始化引导时的内存分配器，该内存分配器并不将所有的内存用于管理，因为它只是引导期间使用，管理的内存边界是896m。参数：
start: 表示用于引导内存分配器管理的起始页页帧号
pages: 引导内存管理系统管理的内存大小对应的页面数，不超过896m对应的页面数 */
unsigned long __init init_bootmem(unsigned long start, unsigned long pages)
{
    max_low_pfn = pages; /* 记录了引导内存管理系统管理的内存大小对应的页面数，不超过896m对应的页面数 */
    min_low_pfn = start; /* 记录了内存管理系统可管理的起始页面帧号 */
    /* 初始化引导内存分配器，并返回引导内存分配器的位图大小 */
    return (init_bootmem_core(&contig_page_data, start, 0, pages));
}