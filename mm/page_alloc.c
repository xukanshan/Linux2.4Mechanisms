#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/swapctl.h>
#include <linux/interrupt.h>
#include <linux/bootmem.h>

/* 从struct page形成的链表中删除一个page */
#define memlist_del list_del

/* 向struct page形成的链表中加入一个page */
#define memlist_add_head list_add

/* 抽象每个内存节点的pg_data_t结构体形成的单链表 */
pg_data_t *pgdat_list;

/* 初始化一个内存列表 */
#define memlist_init(x) INIT_LIST_HEAD(x)

/* 维护活跃页面。活跃页面是指那些近期被频繁访问的页面，因此它们被认为是当前重要的，
并且不太可能在短期内被换出（即从物理内存移至交换空间） */
struct list_head active_list;

/* 维护“非活跃但已修改”（inactive dirty）的页面。
这些页面不像活跃页面那样经常被访问，但它们自从被载入或上次被写入以来已经被修改过。
这个链表的存在是为了优化页面的换出决策。由于这些页面被修改过，
它们不能直接被换出，而需要先写回到磁盘（如果它们是映射到文件的）或者移到交换空间 */
struct list_head inactive_dirty_list;

/* 一个内存节点内的区域（zone）的名字列表 */
static char *zone_names[MAX_NR_ZONES] = {"DMA", "Normal", "HighMem"};

/* 设置 Linux 内核内存管理中不同内存区域（zones）的平衡比率，
这个值用于计算内存区域在内存回收和分配策略中的页面数阈值，
比率值越低，表示该区域被允许占用的空闲页面数越多，反之则越少。
但实际上这个比率值的使用是mask = 区域页数/比率，比率更大的，区域页数也更大。
而且实际页数增加倍数远大于比率增加倍数，所以变相是将mask拉得相近一些。
这些值通常是基于系统架构和硬件特性精心选择的 */
static int zone_balance_ratio[MAX_NR_ZONES] = {
    32,
    128,
    128,
};

/* 这个数组为每个内存区域定义了最小页面数阈值，
这意味着每个内存区域至少需要保留 10 页作为空闲页面，这有助于避免区域出现过度的内存压力。
当一个内存区域的空闲页面数降至 zone_balance_min 以下时，内核会采取措施来回收或释放内存。 */
static int zone_balance_min[MAX_NR_ZONES] = {
    10,
    10,
    10,
};

/* 这个数组为每个内存区域定义了最大页面数阈值，
这意味着每个内存区域在正常情况下最多可以保留 255 页作为空闲页面，以避免过多的空闲内存占用。
当空闲页面数接近 zone_balance_max 时，内核可能会减少内存的回收行为，因为有足够的空闲页面可供使用。 */
static int zone_balance_max[MAX_NR_ZONES] = {
    255,
    255,
    255,
};

/* 将给定的值 x 向上对齐到最接近的 long 类型的大小的倍数 */
#define LONG_ALIGN(x) (((x) + (sizeof(long)) - 1) & ~((sizeof(long)) - 1))

/* 用于在 Linux 内核的内存管理中构建区域列表。
它是基于每个节点的内存区域（如 DMA, Normal, HighMem）来构建适合不同类型内存分配请求的区域列表，参数：
pgdat: 指向管理整个节点内存的pg_data_t指针 */
static inline void build_zonelists(pg_data_t *pgdat)
{
    int i, j, k; /*  用于循环 */

    for (i = 0; i < NR_GFPINDEX; i++) /* 循环遍历不同的内存分配类型 */
    {
        zonelist_t *zonelist; /* 声明指向 zonelist_t 结构的指针 */
        zone_t *zone;         /* 声明指向 zone_t 结构的指针 */

        zonelist = pgdat->node_zonelists + i;   /* 定位到当前节点的特定区域(zone)列表 */
        memset(zonelist, 0, sizeof(*zonelist)); /* 将区域列表清零 */

        zonelist->gfp_mask = i; /* 设置区域列表的 GFP 掩码，这决定了哪些类型的内存分配请求可以使用该列表 */
        j = 0;                  /* 初始化索引 j */
        k = ZONE_NORMAL;        /* 初始化区域类型 k */
        if (i & __GFP_HIGHMEM)  /* 如果 GFP 标志指示高端内存，设置 k 为高端内存区域 */
            k = ZONE_HIGHMEM;
        if (i & __GFP_DMA) /* 如果 GFP 标志指示 DMA 兼容内存，设置 k 为 DMA 区域 */
            k = ZONE_DMA;

        switch (k) /* 根据之前设置的区域类型 k 的值来执行不同的代码块 */
        {
        default:
            BUG(); /* 如果 k 的值不是期望中的任何一种（即不是 ZONE_HIGHMEM, ZONE_NORMAL, 或 ZONE_DMA），则调用 BUG() 函数 */
        case ZONE_HIGHMEM:
            zone = pgdat->node_zones + ZONE_HIGHMEM; /* 如果高端内存区域的大小不为零（即存在可用内存） */
            if (zone->size)                          /* 如果高端内存区域的大小不为零（即存在可用内存） */
            {
                zonelist->zones[j++] = zone; /* 将该区域添加到区域列表中 */
            }
        case ZONE_NORMAL:                           /* 处理普通内存区域 */
            zone = pgdat->node_zones + ZONE_NORMAL; /* 将 zone 指向普通内存区域 */
            if (zone->size)                         /* 如果普通内存区域的大小不为零  */
                zonelist->zones[j++] = zone;        /* 将该区域添加到区域列表中 */
        case ZONE_DMA:                              /*  最后处理 DMA 兼容内存区域 */
            zone = pgdat->node_zones + ZONE_DMA;    /* 将 zone 指向 DMA 区域 */
            if (zone->size)                         /* 如果 DMA 区域的大小不为零 */
                zonelist->zones[j++] = zone;        /* 将该区域添加到区域列表中 */
        }
        zonelist->zones[j++] = NULL; /*  在区域列表的末尾添加 NULL，表示列表的结束 */
    }
}

/* 函数的核心目的是为特定的节点初始化其内存区域，
完成工作：1、为内存节点所有zone创建一个page数组，并初始化每个struct page；2、设定pglist相关成员；
3、设定pglist内node_zones数组内每个zone结构体；4、初始化管理整个系统剩余空间的freepages结构体；
5、初始化每个zone内的伙伴系统元数据, 就是zone内的free_area数组；6、设置不同内存分配策略对应的zone列表，就是pglist内node_zones数组；
可以说，该函数的核心工作就是设定管理内存节点的pglist结构体，然后由此
延伸到node_zones数组（每个都是管理内存区域的zone），page数组（zone结构体延伸），freepages结构体，free_area数组（伙伴系统用），node_zones数组（分配策略用）
参数：
nid: 节点ID，用于标识多处理器系统中的特定节点。
pgdat: 指向结构 pg_data_t 的指针，代表要处理某个节点的内存数据。
gmap: 指向描述整个系统的page数组的二级指针。
zones_size: 区域大小数组，表示每个内存区域的大小。以页为单位
zone_start_paddr: 第一个内存区域的物理地址。
zholes_size: 区域内空洞的大小数组。
lmem_map: 指向要处理的内存节点的page数组 */
void __init free_area_init_core(int nid, pg_data_t *pgdat, struct page **gmap,
                                unsigned long *zones_size, unsigned long zone_start_paddr,
                                unsigned long *zholes_size, struct page *lmem_map)
{
    struct page *p;                                   /* 用于后续操作页面 */
    unsigned long i, j;                               /* 用于循环迭代 */
    unsigned long map_size;                           /* 用于存储内存映射所需的大小 */
    unsigned long totalpages, offset, realtotalpages; /* 内存节点所有区域的总页数、偏移量、实际总页数 */
    unsigned int cumulative = 0;                      /* cumulative 用于记录处理过的所有区域的总页数 */

    totalpages = 0;                    /* 初始化总页数为 0 */
    for (i = 0; i < MAX_NR_ZONES; i++) /* 遍历区域大小数组 */
    {
        unsigned long size = zones_size[i]; /* 读取每个区域的大小 */
        totalpages += size;                 /* 累内存节点加各个区域的大小来计算总页数 */
    }
    realtotalpages = totalpages;              /* 实际总页数等于计算得到的总页数 */
    if (zholes_size)                          /* 如果 zholes_size（表示每个区域内空洞的大小）不为空 */
        for (i = 0; i < MAX_NR_ZONES; i++)    /* 再次遍历所有区域 */
            realtotalpages -= zholes_size[i]; /* 从实际总页数中减去每个区域内的空洞大小，得到最终的实际可用总页数 */

    printk("On node %d totalpages: %lu\n", nid, realtotalpages); /* 打印当前节点（nid）的实际总页数（realtotalpages） */

    memlist_init(&active_list);         /* 初始化活跃页链表 */
    memlist_init(&inactive_dirty_list); /* 初始化非活跃已修改链表 */

    /* 计算整个节点的内存映射所需的大小, 加1可能用于存储某些元数据或对齐目的 */
    map_size = (totalpages + 1) * sizeof(struct page);

    /* 检查 lmem_map（局部内存映射的指针）是否为空。 */
    if (lmem_map == (struct page *)0)
    {
        /* 如果是空（即未初始化），则需要分配内存映射 */
        lmem_map = (struct page *)alloc_bootmem_node(pgdat, map_size);

        /* 调整 lmem_map 的地址，以确保它在一个内存映射边界上对齐。
        这对于确保地址到页面编号的映射（通过 MAP_NR 宏）的正确性至关重要 */
        lmem_map = (struct page *)(PAGE_OFFSET + MAP_ALIGN((unsigned long)lmem_map - PAGE_OFFSET));
    }
    /* 设置了几个指针指向刚刚分配并对齐的内存映射。
    gmap（全局内存映射指针）和 pgdat->node_mem_map（当前节点的内存映射指针）都被设置为 lmem_map 的值 */
    *gmap = pgdat->node_mem_map = lmem_map;
    pgdat->node_size = totalpages;                  /* 设置当前节点的大小为之前计算的总页数 */
    pgdat->node_start_paddr = zone_start_paddr;     /* 设置当前节点的起始物理地址 */
    pgdat->node_start_mapnr = (lmem_map - mem_map); /* 计算 lmem_map 相对于全局内存映射 mem_map 的偏移量 */

    for (p = lmem_map; p < lmem_map + totalpages; p++) /* 遍历本节点的所有内存页面 */
    {
        set_page_count(p, 0); /* 将页面 p 的引用计数设置为0 */
        /* 将页面 p 标记为保留， 保留的页面不会被常规的内存分配器分配出去，
        这通常用于保护内核代码和数据不被意外覆盖或用于其他目的 */
        SetPageReserved(p);
        /* 调用初始化页面 p 的等待队列头。等待队列用于在内核中管理线程或进程的睡眠和唤醒。
        通过初始化等待队列，页面可以被用于各种同步机制 */
        init_waitqueue_head(&p->wait);
        /* 初始化页面 p 的链表头 */
        memlist_init(&p->list);
    }

    offset = lmem_map - mem_map;       /* 计算从全局内存映射 (mem_map) 到局部内存映射 (lmem_map) 的偏移量 */
    for (j = 0; j < MAX_NR_ZONES; j++) /* 历节点上的所有内存区域 */
    {
        zone_t *zone = pgdat->node_zones + j; /* zone 指针指向当前处理的内存区域 */
        unsigned long mask;
        unsigned long size, realsize; /* 记录zone的大小、真实大小 */

        realsize = size = zones_size[j]; /* 设置当前区域的大小 (size) 和实际大小 (realsize)。这两个值初始相同 */
        if (zholes_size)                 /* 如果 zholes_size（空洞大小数组）存在 */
            realsize -= zholes_size[j];  /* 从实际大小中减去相应的空洞大小 */

        printk("zone(%lu): %lu pages.\n", j, size); /* 打印区域信息 */
        zone->size = size;                          /* 设置区域的大小 */
        zone->name = zone_names[j];                 /* 设置区域的名称 */
        zone->lock = SPIN_LOCK_UNLOCKED;            /* 初始化区域的自旋锁为解锁状态 */
        zone->zone_pgdat = pgdat;                   /* 设置区域的页面数据指针指向管理本节点内存的pgdata */
        zone->free_pages = 0;                       /* 初始化空闲页计数 */
        zone->inactive_clean_pages = 0;             /* 初始化非活动干净页计数 */
        zone->inactive_dirty_pages = 0;             /* 初始化非活动脏页计数 */
        memlist_init(&zone->inactive_clean_list);   /* 初始化非活动干净页列表 */
        if (!size)                                  /* 如果 size 为 0，意味着该区域没有内存页，继续处理下一个区域 */
            continue;

        zone->offset = offset; /* 设置偏移量（offset）。offset 表示从全局内存映射（mem_map）开始到当前区域开始的页数 */
        cumulative += size;    /* 累加当前区域的大小（size）到 cumulative 变量。 */
        /* 根据实际区域大小（realsize）和该区域的平衡比率（zone_balance_ratio[j]）计算得出mask。
        这个值用于确定区域的最小、低和高页面数目的基线 */
        mask = (realsize / zone_balance_ratio[j]);
        if (mask < zone_balance_min[j])
            mask = zone_balance_min[j]; /* 计算出的 mask 值小于区域的最小阈值（zone_balance_min[j]），则将 mask 设置为这个最小阈值 */
        else if (mask > zone_balance_max[j])
            mask = zone_balance_max[j]; /* 如果 mask 大于区域的最大阈值（zone_balance_max[j]），则将 mask 设置为最大阈值 */
        /* 这几行代码为区域设置了三个重要的页面数阈值：最小（pages_min）、低（pages_low）和高（pages_high） */
        zone->pages_min = mask;
        zone->pages_low = mask * 2;
        zone->pages_high = mask * 3;

        freepages.min += mask;                     /* 剩余空间的最小水位线 + 每个区域的最小水位线 */
        freepages.low += mask * 2;                 /* 剩余空间的低水位线 + 每个区域的低水位线 */
        freepages.high += mask * 3;                /* 剩余空间的高水位线 + 每个区域的高水位线 */
        zone->zone_mem_map = mem_map + offset;     /* zone指向本区域第一个page的指针指向管理整个系统的page数组对应位置 */
        zone->zone_start_mapnr = offset;           /* 设置了区域开始的内存映射编号 */
        zone->zone_start_paddr = zone_start_paddr; /* 设置当前内存区域的起始物理地址 */

        for (i = 0; i < size; i++) /* 循环遍历当前内存区域中的每一页 */
        {
            struct page *page = mem_map + offset + i; /* 获取当前页面的指针 */
            /* 将当前页面的 zone 字段设置为当前内存区域。这样，每个页面都知道它属于哪个内存区域 */
            page->zone = zone;
            if (j != ZONE_HIGHMEM) /* 检查当前区域是否不是高内存区域 */
            {

                page->virtual = __va(zone_start_paddr); /* 设置页面在内核地址空间映射的虚拟地址 */
                zone_start_paddr += PAGE_SIZE;          /* 更新区域的起始物理地址，为下一个页面准备 */
            }
        }

        offset += size;                 /* offset增加当前区域的大小。这样，在处理下一个区域时，offset 指向正确的起始位置 */
        mask = -1;                      /* 在 C 语言中，-1 表示所有位都设置为 1。这个 mask 将用于计算位图的大小 */
        for (i = 0; i < MAX_ORDER; i++) /* 初始化每个zone的伙伴系统（也就是zone内那10个free_area_t结构体） */
        {
            unsigned long bitmap_size;                   /* 用于存储计算出的位图大小的变量 */
            memlist_init(&zone->free_area[i].free_list); /* 初始化当前区域所有伙伴系统内存池的空闲内存块链表 */
            mask += mask;                                /* 通过将 mask 的当前值加倍来更新 mask。在位操作中，这相当于将 mask 左移一位 */
            /* 这个操作实际上是将 size 增加到足够大，以至于能被 mask 定义的对齐边界整除。
            然后，使用 & mask 操作确保结果是对齐的，-2表示2字节对齐，-4表示4字节对齐。也就是最后size是向上取整到mask对应的倍数 */
            size = (size + ~mask) & mask;
            /* 计算伙伴系统中每个阶（order）来管理整个zone对应的位图大小，比如zone 大小为16MB（size为16MB对应的页面数），
            那么就会依次计算，每个位管理1页对应的位图大小，每个位管理两页位图的大小。由于上一步对齐操作，导致出现这样的结果：
            计算每个位管理1页时候的位图大小时，size已经向上取整为2的倍数；
            计算每个位图管理2页时候的位图大小时，size已经向上取整为4的倍数，现在还不知道这会产生什么样的后果 */
            bitmap_size = size >> i;
            bitmap_size = (bitmap_size + 7) >> 3;  /* 相当于对 bitmap_size 向上取整到最近的 8 的倍数，因为要以字节为单位为位图申请内存 */
            bitmap_size = LONG_ALIGN(bitmap_size); /* bitmap_size 向上对齐到最近的 long 类型的边界 */
            /* 为每个阶分配位图 */
            zone->free_area[i].map = (unsigned int *)alloc_bootmem_node(pgdat, bitmap_size);
        }
    }
    build_zonelists(pgdat);
}

/* 初始化管理内存节点的pglist结构体，传入参数：
zones_size，记录内存节点区域大小的数组指针 */
void __init free_area_init(unsigned long *zones_size)
{
    /* 调用函数来完成管理内存节点的pglist结构体的初始化 */
    free_area_init_core(0, &contig_page_data, &mem_map, zones_size, 0, 0, 0);
}

/* 检查一个页面x是否在zone范围之内 */
#define BAD_RANGE(zone, x) (((zone) != (x)->zone) || (((x)-mem_map) < (zone)->offset) || (((x)-mem_map) >= (zone)->offset + (zone)->size))

/* 释放伙伴系统中的块，并合并成更大的块, 参数：
page：释放的内存页的 struct page 结构的指针
order：要释放的内存页的大小，以 2 的幂为单位 */
static void __free_pages_ok(struct page *page, unsigned long order)
{
    /* index: 用于存储释放和伙伴块在其伙伴系统（buddy system）中的索引。
    page_idx: 用于存储当前正在处理的页面相对于其内存区域（zone）的基地址的索引
    mask: 用于计算和操作内存页索引的掩码
    flags: 用于保存在执行自旋锁操作时的中断状态，确保线程安全 */
    unsigned long index, page_idx, mask, flags;
    /* area: 指向 free_area_t 结构的指针，这个结构用于表示不同阶（order）的空闲页面链表。
    每个阶的空闲页面链表包含了相应大小的空闲内存块 */
    free_area_t *area;
    struct page *base; /* base: 指向内存区域（zone）中第一个 page 结构的指针。 */
    zone_t *zone;      /* zone: 指向 zone_t 结构的指针，表示当前页面所在的内存区域。 */

    if (page->buffers)
        /* 进入if，说明试图释放一个仍然有缓冲区的页面，报错 */
        BUG();
    if (page->mapping)
        /* 进入if，意味着这个页面当前被映射到某个文件或地址空间，
        不能释放一个正在使用中的映射页面 */
        BUG();
    if (!VALID_PAGE(page))
        /* 进入if，说明要释放的页面是个无效页面 */
        BUG();
    if (PageSwapCache(page))
        /* 进入if，表示页面的内容被交换到磁盘，则不能释放
        （因为此时页面中仍然有一些管理数据，如页面内在磁盘中的位置） */
        BUG();
    if (PageLocked(page))
        /* 进入if，表示页面被锁定了，意味着它正在被某个进程或内核线程使用。
        试图释放被锁定的页面会引起竞争条件或内存损坏 */
        BUG();
    if (PageDecrAfter(page))
        /* 进入if，表示该页面有特殊引用计数行为。例如，某些页面可能被内核标记为需要在特定时刻释放，
        或者在特定的操作完成后才能释放。这样的页面不能释放 */
        BUG();
    if (PageActive(page))
        /* 进入if，表示该页面是活跃的页面是正在被使用的，不能释放 */
        BUG();
    if (PageInactiveDirty(page))
        /* 进入if，表示该页是脏的非活跃页面包含尚未写回磁盘的数据，不能释放 */
        BUG();
    if (PageInactiveClean(page))
        /* 进入if，表示该页面是非活跃页面但是是干净的，也不能释放，
        因为非活跃页面可能包含了之前被使用过但目前暂时不活跃的数据。这些数据可能在未来会再次被访问 */
        BUG();

    page->flags &= ~((1 << PG_referenced) | (1 << PG_dirty)); /* 清除referenced和dirty位 */
    page->age = PAGE_AGE_START;                               /* 将页面的年龄设置为初始值 */

    zone = page->zone; /* 得到页面所在的内存区 */

    mask = (~0UL) << order;        /* 创建一个掩码，高位全是1，低order位全是0 */
    base = mem_map + zone->offset; /* 得到页面所在内存区域起始页面的struct page */
    page_idx = page - base;        /* 计算当前页面相对于内存区起始页面的偏移 */
    /* 检查页面索引是否正确对齐，在伙伴系统中，每个大小为 2^order 的页面块应当从对应的对齐位置开始 */
    if (page_idx & ~mask)
        /* 进入if，说明没有对齐 */
        BUG();
    /* 计算要释放的块与伙伴块在伙伴系统位图中的偏移，举例子帮你理解这里面的关键点：
    0阶free_area_t的位图，当中偏移为1的位，表示的是偏移为2和3的页面中一个页面空闲，另一个页面被使用，
    如果两个页面都空闲或都在使用，那么这个位就是0。所以当释放页面的时候，检查这个位如果位1，就证明了另一个伙伴块是
    空闲的。为什么这样，需要深入理解伙伴系统的页面分配机制。文章：https://www.bilibili.com/read/cv16402064/ */
    index = page_idx >> (1 + order);
    area = zone->free_area + order;        /* 根据页面的大小（order），获取对应的 free_area 结构 */
    spin_lock_irqsave(&zone->lock, flags); /* 获取自旋锁的同时关闭本地中断，保存当前中断状态到flags */
    /* 从区域的空闲页面计数中加上相应数量的页面，mask 在此处表示释放的页面数，
    因为它是根据页面的大小（order）计算出来的，前面全是1，后面跟几个0，是负数的表达 */
    zone->free_pages -= mask;
    /* 循环条件检查是否已经到达了最大内存块的一半，当mask增加到-512的时候，就会停止 */
    while (mask + (1 << (MAX_ORDER - 1)))
    {
        /* buddy1指向可以和要释放块合并的页面，buddy2指向要释放块的页面 */
        struct page *buddy1, *buddy2;
        /* 检查 area 指针（free_area结构指针）没有超出zone结构体中的free_area结构体数组范围 */
        if (area >= zone->free_area + MAX_ORDER)
            /* 进入if，说明超过了 */
            BUG();
        /* 检查并取反位图中相应的位，以确定伙伴块是空闲的 */
        if (!test_and_change_bit(index, area->map))
            /* 进入if，说明对应位是0，表示释放块与伙伴块都是空闲或占用，退出 */
            break;
        /* 来到这里，说明上面的位是1，说明释放块的伙伴块是空闲的，可以合并 */
        /* 得到可以和要释放的块进行合并的块的页面 */
        buddy1 = base + (page_idx ^ -mask);
        buddy2 = base + page_idx;    /* 得到要释放块的页面 */
        if (BAD_RANGE(zone, buddy1)) /* 检查伙伴页面1在内存区域的有效范围内 */
            BUG();
        if (BAD_RANGE(zone, buddy2)) /* 检查伙伴页面2在内存区域的有效范围内 */
            BUG();
        /* 将可以和要释放的块进行合并的块的页面从free_area_t的链表中释放 */
        memlist_del(&buddy1->list);
        mask <<= 1; /* 更新 mask 来表示更大的内存块大小 */
        area++;     /* 将 area 指针移动到更高的阶 */
        /* 更新 index 和 page_idx 来反映新的伙伴块的位置和大小 */
        /* index缩小一半，因为两个块准备合并成更大的一块，然后下一循环要寻找更大块的伙伴块，
        其在位图中的偏移就是index/2 */
        index >>= 1;
        page_idx &= mask; /* 得到两个块合并后的更大块的起始页在zone中的偏移 */
    }
    /* 将页面（或合并后的更大页面块）加入到相应阶的空闲链表中 */
    memlist_add_head(&(base + page_idx)->list, &area->free_list);
    spin_unlock_irqrestore(&zone->lock, flags); /* 释放自旋锁，并恢复之前保存的中断状态 */
    if (memory_pressure > NR_CPUS)              /* 见memory_pressure值含义解释 */
        memory_pressure--;
}

/* 检查页面满足可以释放的条件后释放页面, 参数：
page：释放的内存页的 struct page 结构的指针
order：要释放的内存页的大小，以 2 的幂为单位 */
void __free_pages(struct page *page, unsigned long order)
{
    /* 检查页面是否设置了reserved和尝试减少页面的引用计数，并测试是否变为零 */
    if (!PageReserved(page) && put_page_testzero(page))
        /* 进来if，说明页面没有设定reserved和引用计数为0，那么就可以释放 */
        __free_pages_ok(page, order); /* 释放页面，并尝试合并成更大块 */
}

/* 遍历系统中的每个节点和每个节点中的内存区域，来计算系统中所有空闲页面的总数 */
unsigned int nr_free_pages(void)
{
    unsigned int sum;              /* 累计系统中所有空闲页的数量 */
    zone_t *zone;                  /* 后面指向内存区域 */
    pg_data_t *pgdat = pgdat_list; /* 指向表示一个管理节点（node）内存的数据结构 */

    sum = 0;      /*  初始化 sum 为 0，准备开始统计空闲页的数量 */
    while (pgdat) /* 遍历系统中的所有节点 */
    {
        /*  在当前节点内，遍历所有内存区域（zone） */
        for (zone = pgdat->node_zones; zone < pgdat->node_zones + MAX_NR_ZONES; zone++)
            sum += zone->free_pages; /* 累加当前内存区域中的空闲页面数到 sum */
        pgdat = pgdat->node_next;    /*  移动到下一个节点 */
    }
    return sum; /*  函数返回累加后的空闲页面总数 */
}