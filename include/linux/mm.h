#ifndef _LINUX_MM_H
#define _LINUX_MM_H

#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/mmzone.h>
#include <asm-i386/page.h>
#include <asm-i386/pgtable.h>
#include <asm-i386/atomic.h>

/* 该宏位于#ifdef CONFIG_HIGHMEM（定义为0x10） 的 #else 下，
用于表示高端内存区域（HighMem）的分配请求对应的GPF掩码的位 */
#define __GFP_HIGHMEM 0x0

/* 用于表示DMA的分配请求对应的GPF掩码的位 */
#define __GFP_DMA 0x08

/* page的falgs字段中表示该页被保留的位（不能被普通的内存分配器分配）相对于flags的位偏移 */
#define PG_reserved 31

/* mm/page_alloc.c */
extern void free_area_init(unsigned long *zones_size);

/* 这个结构体是对一个物理内存页面的抽象 */
typedef struct page
{
    struct list_head list; /* 用于将页面链接到其他列表中，比如空闲列表或活动/非活动列表 */
    // struct address_space *mapping; /* 该页所属的地址空间。如果这个页面是文件映射页，mapping 将指向文件的 address_space 结构 */
    // unsigned long index;           /* 在 address_space 中的页面索引。对于文件映射页，它表示该页在文件中的偏移量 */
    // struct page *next_hash;        /* 用于将页面插入到散列表中，以便快速查找 */
    atomic_t count;      /* 引用计数，表示有多少实体正在使用这个页面。当计数降至0时，页面可以被回收 */
    unsigned long flags; /* 一组位标志，表示页面的不同状态和属性（如脏、活跃、锁定等） */
    // struct list_head lru;          /* 用于将页面链接到最近最少使用（LRU）列表中 */
    // unsigned long age;             /* 页面的年龄，用于页面替换算法，帮助确定哪个页面应该被换出 */
    wait_queue_head_t wait; /* 当页面被锁定时，其他想要访问该页面的进程会在这个等待队列上等待 */
    // struct page **pprev_hash;      /* 一个指向散列表中前一个页面的指针，用于维护散列表的完整性 */
    // struct buffer_head *buffers;   /* 如果该页面被用作缓冲区，这个指针将指向相关的缓冲区头结构 */
    void *virtual;            /* 如果页面被映射到内核虚拟地址空间，这个指针非空，并指向该页面在内核地址空间中的虚拟地址 */
    struct zone_struct *zone; /* 指向描述该页面所属内存区域的 zone_struct 结构的指针 */
} mem_map_t;

/* mm/memory.c */
extern mem_map_t *mem_map;

/* 用于设定一个page的引用计数 */
#define set_page_count(p, v) atomic_set(&(p)->count, v)

/* 将一个page设定为保留的（不能被普通的内存分配器分配），是通过设定page的flags字段完成的 */
#define SetPageReserved(page) set_bit(PG_reserved, &(page)->flags)

#endif /* _LINUX_MM_H */