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
#include <asm-i386/bitops.h>
#include <linux/string.h>
#include <linux/stddef.h>

/* 初始化一个节点的引导内存管理器。它设置了一个位图来跟踪哪些页面是可用的，哪些是已经被占用的。参数：
pg_data_t *pgdat: 指向一个包含节点（node）相关数据的结构，该结构抽象了一个内存节点
unsigned long mapstart: 引导内存位图的起始地址。
unsigned long start: 引导内存分配器管理的内存区域的开始页帧号。
unsigned long end: 引导内存分配器管理的内存区域的结束页帧号。 */
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

/* 用于释放引导内存分配器管理的内存。参数：
bdata: 一个指向引导内存分配器的指针；
addr: 要释放的内存的起始地址；
size：要释放的内存的大小 */
static void __init free_bootmem_core(bootmem_data_t *bdata, unsigned long addr, unsigned long size)
{
    unsigned long i;                                                         /* 循环变量 */
    unsigned long start;                                                     /* 存储距离起始地址向上最近页的页帧号 */
    unsigned long sidx;                                                      /* 存储距离起始地址向上最近页在页面位图中的索引 */
    unsigned long eidx = (addr + size - bdata->node_boot_start) / PAGE_SIZE; /* 计算结束地址向下最近页在页面位图中的索引 */
    unsigned long end = (addr + size) / PAGE_SIZE;                           /* 存储距离结束地址向下最近页的页帧号 */

    if (!size) /* 如果 size 为零，则触发 BUG() 宏 */
        BUG();
    if (end > bdata->node_low_pfn) /* 如果计算出的 end 值超过了节点的最低物理帧号，也触发 BUG() 宏 */
        BUG();

    start = (addr + PAGE_SIZE - 1) / PAGE_SIZE;          /* 计算距离起始地址向上最近页的页帧号 */
    sidx = start - (bdata->node_boot_start / PAGE_SIZE); /* 计算距离起始地址向上最近页在页面位图中的索引 */

    for (i = sidx; i < eidx; i++)
    {
        /* 使用 test_and_clear_bit 函数测试并清除 bdata->node_bootmem_map 中的位。如果位已经被清除（即函数返回 0），则触发 BUG() 宏 */
        if (!test_and_clear_bit(i, bdata->node_bootmem_map))
            BUG();
    }
}

/* 系统引导期间保留一段特定的物理内存。通过操作位图来跟踪哪些页面已被保留，确保不会重复使用这些页面，参数：
bdata: 指向一个引导内存分配器。
addr:要保留内存的起始地址。
size: 要保留的内存大小。 */
static void __init reserve_bootmem_core(bootmem_data_t *bdata, unsigned long addr, unsigned long size)
{
    unsigned long i; /* 循环变量 */
    /* 这里考虑了页对齐，任何部分占用的页都会被完全保留 */
    unsigned long sidx = (addr - bdata->node_boot_start) / PAGE_SIZE;                        /* 向下取整计算起始页在引导内存分配器位图中的索引 */
    unsigned long eidx = (addr + size - bdata->node_boot_start + PAGE_SIZE - 1) / PAGE_SIZE; /* 向上取整计算结束页在引导内存分配器位图中的索引 */
    unsigned long end = (addr + size + PAGE_SIZE - 1) / PAGE_SIZE;                           /* 向上取整计算结束页的页帧号 */

    if (!size) /* 如果尺寸为0，则触发一个错误（BUG） */
        BUG();

    if (end > bdata->node_low_pfn) /* 如果计算出的 end 值超过了节点的最低物理帧号，也触发 BUG() 宏 */
        BUG();
    for (i = sidx; i < eidx; i++)
        if (test_and_set_bit(i, bdata->node_bootmem_map))              /* 测试并设置引导内存映射中相应的位（置1）。如果该位已经被设置，表示该页已经被保留 */
            printk("hm, page %08lx reserved twice.\n", i * PAGE_SIZE); /* 如果某个页面已经被保留（即 test_and_set_bit 返回真），则打印一条消息，表明该页已被两次保留 */
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

/* 用于释放引导内存分配器管理的内存，也就是分配器的位图置0。参数：
addr：要释放的内存空间的起始地址；
size: 要释放的内存空间的字节大小 */
void __init free_bootmem(unsigned long addr, unsigned long size)
{
    /* 调用函数去完成释放引导内存分配器管理的内存 */
    return (free_bootmem_core(contig_page_data.bdata, addr, size));
}

/* 用于保留引导内存，也就是分配器的位图置1。参数：
addr：要保留的内存空间的起始地址；
size: 要保留的内存空间的字节大小 */
void __init reserve_bootmem(unsigned long addr, unsigned long size)
{
    reserve_bootmem_core(contig_page_data.bdata, addr, size); /* 调用函数来保留内存 */
}

/* 用于在系统引导时分配内存,基本原理：扫描内存节点对应的引导内存分配器的位图来进行分配，
但是这样的分配会引发内存碎片，所以在这个机制下又添加了内存分配合并策略，也就是传统的顺序分配。参数：
bdata:指向引导内存分配器的指针。
size: 这是要分配的内存块的大小。
align: 指定了分配的内存的起始地址必须满足的对齐方式。
goal: 指定了内存分配应该尽可能从哪个物理地址开始。 */
static void *__init __alloc_bootmem_core(bootmem_data_t *bdata, unsigned long size, unsigned long align, unsigned long goal)
{
    unsigned long i, start = 0;           /* i用于循环计数，start记录分配的起始位置，初始化为0） */
    void *ret;                            /* 返回分配的内存地址 */
    unsigned long offset, remaining_size; /* 返回地址在页内偏移量和页内剩余大小 */
    /* areasize记录需要分配的页数；preferred表示分配首选页面；incr表示页帧增长步长 */
    unsigned long areasize, preferred, incr;
    /* 计算内存节点结束的页帧号 */
    unsigned long eidx = bdata->node_low_pfn - (bdata->node_boot_start >> PAGE_SHIFT);

    if (!size) /* 如果请求的内存大小为0，则触发错误 */
        BUG();

    /* 初始化 preferred 变量，它表示首选的起始页帧号。
    如果 goal 在有效范围内，则从 goal 开始寻找空间，否则从0开始 */
    if (goal && (goal >= bdata->node_boot_start) && ((goal >> PAGE_SHIFT) < bdata->node_low_pfn))
    {
        preferred = goal - bdata->node_boot_start;
    }
    else
        preferred = 0;

    /*  计算向上对齐后的首选页帧号 */
    preferred = ((preferred + align - 1) & ~(align - 1)) >> PAGE_SHIFT;
    areasize = (size + PAGE_SIZE - 1) / PAGE_SIZE; /* 向上取整计算需要的页数 */
    /* C 语言的条件运算符，形式为 condition ? expr1 : expr2。如果 condition 非零，结果是 expr1，否则是 expr2。
    这里使用了一个变体：expr1 ?: expr2。这里如果 expr1 为非零（即真），结果就是 expr1 的值；如果 expr1 为零，结果是 expr2 的值 */
    incr = align >> PAGE_SHIFT ?: 1; /* 计算页帧号增加的步长，如果没有指定对齐，则默认为1 */

restart_scan:
    /* 遍历页帧号，检查是否有足够的连续空页 */
    for (i = preferred; i < eidx; i += incr)
    {
        unsigned long j;
        /* 如果当前页帧 i 已被占用，则起始页帧+incr，从之后的位置继续寻找 */
        if (test_bit(i, bdata->node_bootmem_map))
            continue;
        /* 检查从 i+1 到 i + areasize 的所有页帧是否都未被占用 */
        for (j = i + 1; j < i + areasize; ++j)
        {
            /* 如果 j 超过了 eidx，表示没有足够的连续空间，跳转到 fail_block。 */
            if (j >= eidx)
                goto fail_block;
            /* 如果页帧 j 被占用，跳转到 fail_block */
            if (test_bit(j, bdata->node_bootmem_map))
                goto fail_block;
        }
        /* 如果找到了足够的连续空间，记录起始页帧号 i 到 start，然后跳转到 found */
        start = i;
        goto found;
    fail_block:;
    }
    /* 如果首选的起始页帧号 preferred 不为0（这意味着首次尝试是从一个非零的特定位置开始的。
    因为 goal 参数指定了一个优先考虑的内存位置）且没有找到合适的区域，将 preferred 设置为0并重新开始扫描
    这种方法允许代码首先尝试满足可能的最佳或优化的内存分配（基于 goal），如果那里没有足够的空间，
    它会退回到一个更通用的搜索，覆盖所有可用的内存空间。 */
    if (preferred)
    {
        preferred = 0;
        goto restart_scan;
    }
found:
    if (start >= eidx) /* 如果找到的区域起始已经超过了eidx，触发bug宏 */
        BUG();

    /*  这个条件检查是否可以将新的分配与前一个分配合并。如果新请求的对齐小于等于页面大小，
    且之前有分配（bdata->last_offset 非零），且新的分配紧接在上一个分配之后（bdata->last_pos + 1 == start），
    则进入合并逻辑， 以合并相邻的内存分配以节省空间。举个例子，上次我调用函数分配了1KB空间，然后这次需要2KB空间，
    没有合并分配逻辑的话，将会两次均分配4KB大小空间，有了合并分配，这两次将会放在同一个4KB页面内。
    这种机制就是传统的顺序分配，只不过在位图机制下（小于4KB的内存分配最后都会分配4KB），才显得叫做优化 */
    if (align <= PAGE_SIZE && bdata->last_offset && bdata->last_pos + 1 == start)
    {
        /*  计算新分配的偏移量。它确保偏移量满足对齐要求，比如上次分配的结束last_offset是5，然后align是4，
        处理过后，offset就是8了 */
        offset = (bdata->last_offset + align - 1) & ~(align - 1);
        if (offset > PAGE_SIZE) /* 如果计算出的偏移量超出了一个页面的大小，则触发错误 */
            BUG();
        remaining_size = PAGE_SIZE - offset; /* 计算上一个分配的页面中剩余的空间 */
        if (size < remaining_size)           /* 如果请求的大小小于剩余空间，不需要新的页帧；否则，计算还需要多少页帧 */
        {
            areasize = 0;                                                                      /* 由于不需要额外的页面，所以设置 areasize 为0 */
            bdata->last_offset = offset + size;                                                /* 更新最后的偏移量，去掉页大小的影响 */
            ret = phys_to_virt(bdata->last_pos * PAGE_SIZE + offset + bdata->node_boot_start); /* 计算并返回新分配的物理内存地址的虚拟地址，挨着上次分配的结束位置 */
        }
        else /*  如果不能合并分配，则进行常规分配 */
        {
            remaining_size = size - remaining_size;                                            /* 计算除去上一个分配剩余部分后需要的大小 */
            areasize = (remaining_size + PAGE_SIZE - 1) / PAGE_SIZE;                           /* 计算所需额外页面的数量 */
            ret = phys_to_virt(bdata->last_pos * PAGE_SIZE + offset + bdata->node_boot_start); /* 计算并返回新分配的起始虚拟地址，也挨着上次分配的结束位置 */
            bdata->last_pos = start + areasize - 1;                                            /* 更新最后一个分配位置 */
            bdata->last_offset = remaining_size;                                               /* 更新最后一个分配的偏移量 */
        }
        bdata->last_offset &= ~PAGE_MASK; /* last_offset低12位置为1，高位全部清零。因为last_offset可能超过4KB（因为size比较大） */
    }
    else /* 如果不能合并，将此次分配作为一个新的独立分配 */
    {
        bdata->last_pos = start + areasize - 1;                         /* 更新最后位置为当前起始位置加上分配的页数减一 */
        bdata->last_offset = size & ~PAGE_MASK;                         /* 更新最后的偏移量 */
        ret = phys_to_virt(start * PAGE_SIZE + bdata->node_boot_start); /*  计算分配的物理地址转换为虚拟地址 */
    }

    /* 遍历分配的每个页帧 */
    for (i = start; i < start + areasize; i++)
        /* 标记为已使用，如果已经被标记，则触发错误 */
        if (test_and_set_bit(i, bdata->node_bootmem_map))
            BUG();
    memset(ret, 0, size); /* 将分配的内存区域清零 */
    return ret;           /* 返回分配的内存地址 */
}

/* 用于在系统引导时分配内存, 参数：
size: 这是要分配的内存块的大小。
align: 指定了分配的内存的起始地址必须满足的对齐方式。
goal: 指定了内存分配应该尽可能从哪个物理地址开始。 */
void *__init __alloc_bootmem(unsigned long size, unsigned long align, unsigned long goal)
{

    pg_data_t *pgdat = pgdat_list; /* 创建一个指向系统中第一个物理内存节点的指针 */
    void *ptr;                     /* 存储分配的内存地址 */

    while (pgdat) /* 遍历所有的物理内存节点 */
    {
        /* 在当前节点上尝试分配内存，如果分配成功，函数会返回分配的内存地址 */
        if ((ptr = __alloc_bootmem_core(pgdat->bdata, size, align, goal)))
            return (ptr);
        pgdat = pgdat->node_next; /* 如果当前节点上无法分配足够的内存，函数会移动到下一个内存节点 */
    }
    BUG();       /* 如果所有节点都无法满足内存分配要求，函数会调用 BUG() 宏 */
    return NULL; /* 在发生错误时，函数返回 NULL，表示内存分配失败 */
}

/* 在某个内存节点上进行引导内存分配，参数：
pgdat: 一个指向内存节点的指正。
size: 要分配的内存大小。
align: 内存对齐要求。
goal: 分配的内存的目标起始地址 */
void *__init __alloc_bootmem_node(pg_data_t *pgdat, unsigned long size, unsigned long align, unsigned long goal)
{
    void *ptr;

    ptr = __alloc_bootmem_core(pgdat->bdata, size, align, goal);
    if (ptr)
        return (ptr);
    BUG();
    return NULL;
}