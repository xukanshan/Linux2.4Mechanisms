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

/* page的falgs字段中表示该页被锁定的位（意味着它正在被某个进程或内核线程使用。
试图释放被锁定的页面会引起竞争条件或内存损坏）相对于flags的位偏移 */
#define PG_locked 0

/* page的falgs字段中表示该页被访问过的位相对于flags的位偏移 */
#define PG_referenced 2

/* page的falgs字段中表示该页被修改过，但是还没有被写入磁盘同步的位相对于flags的位偏移 */
#define PG_dirty 4

/* Decrement After，page的falgs字段中表示该页被特殊引用的位（内核在某些情况下对某些页面进行额外的引用计数管理。
例如，某些页面可能被内核标记为需要在特定时刻释放，或者在特定的操作完成后才能释放。
这种情况下，页面的引用计数可能被特别处理或标记）相对于flags的位偏移 */
#define PG_decr_after 5

/* page的falgs字段中表示该页是活跃的位（指的是那些被频繁访问或最近使用过的内存页，
这个概念是为了实现一种称为“最近最少使用”（Least Recently Used，LRU）的页面置换算法，
这个算法用于决定哪些页面应该保留在内存中，哪些可以被移出到交换空间）相对于flags的位偏移 */
#define PG_active 6

/* page的falgs字段中表示该页脏的非活跃页面包含尚未写回磁盘的数据的位相对于flags的位偏移 */
#define PG_inactive_dirty 7

/* page的falgs字段中表示该页处于交换缓存中
（当系统的物理内存不足时，一些内存页面会被移到磁盘上的交换空间以释放内存。
这些页面的内容存储在交换空间中，此时仍需要在内存中保留一定的信息以便于快速访问）
的位相对于flags的位偏移 */
#define PG_swap_cache 9

/* page的falgs字段中表示该页干净的且非活跃的位相对于flags的位偏移 */
#define PG_inactive_clean 11

/* page的falgs字段中表示该页被保留的位（不能被普通的内存分配器分配）相对于flags的位偏移 */
#define PG_reserved 31

/* mm/page_alloc.c */
extern void free_area_init(unsigned long *zones_size);

/* 这个结构体是对一个物理内存页面的抽象 */
typedef struct page
{
    /* 用于将页面链接到伙伴系统中，比如order为2的两个块，page256起始的块和page512块都是这么大，
    那么这两个page的这个字段就会链接到管理order为2的free_area_t的链表字段中 */
    struct list_head list;
    /* 该页所属的地址空间。如果这个页面是文件映射页，mapping 将指向文件的 address_space 结构 */
    struct address_space *mapping;
    // unsigned long index;           /* 在 address_space 中的页面索引。对于文件映射页，它表示该页在文件中的偏移量 */
    // struct page *next_hash;        /* 用于将页面插入到散列表中，以便快速查找 */
    atomic_t count;      /* 引用计数，表示有多少实体正在使用这个页面。当计数降至0时，页面可以被回收 */
    unsigned long flags; /* 一组位标志，表示页面的不同状态和属性（如脏、活跃、锁定等） */
    // struct list_head lru;          /* 用于将页面链接到最近最少使用（LRU）列表中 */
    unsigned long age;      /* 页面的年龄，用于页面替换算法，帮助确定哪个页面应该被换出 */
    wait_queue_head_t wait; /* 当页面被锁定时，其他想要访问该页面的进程会在这个等待队列上等待 */
    // struct page **pprev_hash;      /* 一个指向散列表中前一个页面的指针，用于维护散列表的完整性 */
    /* 如果该页面被用作缓冲区(如文件系统数据，文件系统元数据)，
    这个指针将指向相关的缓冲区头结构 */
    struct buffer_head *buffers;
    void *virtual;            /* 如果页面被映射到内核虚拟地址空间，这个指针非空，并指向该页面在内核地址空间中的虚拟地址 */
    struct zone_struct *zone; /* 指向描述该页面所属内存区域的 zone_struct 结构的指针 */
} mem_map_t;

/* mm/memory.c */
extern mem_map_t *mem_map;

/* 用于设定一个page的引用计数 */
#define set_page_count(p, v) atomic_set(&(p)->count, v)

/* 将一个page设定为保留的（不能被普通的内存分配器分配），是通过设定page的flags字段完成的 */
#define SetPageReserved(page) set_bit(PG_reserved, &(page)->flags)

/* 将一个page的reserved清除（能被普通的内存分配器分配），是通过设定page的flags字段完成的 */
#define ClearPageReserved(page) clear_bit(PG_reserved, &(page)->flags)

/* 返回page的flags中的reserved状态 */
#define PageReserved(page) test_bit(PG_reserved, &(page)->flags)

/* 返回page的flags中的swap_cache状态（详见page中flags中swap_cache的解释） */
#define PageSwapCache(page) test_bit(PG_swap_cache, &(page)->flags)

/* 返回page的flags中的locked状态（详见page中flags中locked的解释） */
#define PageLocked(page) test_bit(PG_locked, &(page)->flags)

/* 返回page的flags中的decr_after状态（详见page中flags中decr_after的解释） */
#define PageDecrAfter(page) test_bit(PG_decr_after, &(page)->flags)

/* 返回page的flags中的active状态（详见page中flags中active的解释） */
#define PageActive(page) test_bit(PG_active, &(page)->flags)

/* 返回page的flags中的inactive_dirty状态 */
#define PageInactiveDirty(page) test_bit(PG_inactive_dirty, &(page)->flags)

/* 返回page的flags中的inactive_clean状态 */
#define PageInactiveClean(page) test_bit(PG_inactive_clean, &(page)->flags)

/* 将page的count -1，然后测试页面的count是不是0，如果是就返回1，不是返回0 */
#define put_page_testzero(p) atomic_dec_and_test(&(p)->count)

/* arch/i386/mm/init.c */
extern void mem_init(void);

/* mm/memory.c */
extern unsigned long max_mapnr;

/* mm/memory.c */
extern unsigned long num_physpages;

/* mm/memory.c */
extern void *high_memory;

/* 释放一个页，调用的是伙伴系统释放接口，将这个页面的释放视作
对一个4KB块的释放 */
#define __free_page(page) __free_pages((page), 0)

#endif /* _LINUX_MM_H */