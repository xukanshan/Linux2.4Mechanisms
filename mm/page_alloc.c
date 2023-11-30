#include <linux/mm.h>
#include <linux/swapctl.h>
#include <linux/interrupt.h>
#include <linux/bootmem.h>

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