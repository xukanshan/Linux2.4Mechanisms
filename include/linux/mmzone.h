#ifndef _MM_ZONE_H
#define _MM_ZONE_H
/* 主要用于定义与内存管理相关的数据结构和函数。
zone结构体：在Linux内核中用于描述和管理物理内存的不同区域。
每个zone都代表了一个特定的连续的物理内存范围。zone结构体中包含了很多与该内存区域相关的信息 */



/* 这个常量通常用于指定内存分配请求类型的数量。在内存管理上下文中，GFP（Get Free Page）标志用于指定不同类型的内存分配请求。
例如，某些分配可能需要普通内存，而其他分配可能需要能进行直接内存访问（DMA）的内存或者高端（HighMem）内存。
0x100 : 这个值表明内核可能会处理多达 256 种不同的内存分配请求类型。这种分类可以基于不同的标准，如是否允许睡眠、是否需要高端内存等。
在实际的内核实现中，这些不同的请求类型通常通过位掩码组合来表示，每种组合代表一种内存分配场景。 */
#define NR_GFPINDEX 0x100

/* 定义了“伙伴系统”（buddy allocator）中使用的最大阶数 */
#define MAX_ORDER 10

/* 在伙伴内存分配系统中用于抽象每个2的order阶的内存块池，
它抽象的是内存池，这个内存池用于2的order阶内存块分配 */
typedef struct free_area_struct
{
    struct list_head free_list; /* 用于链接所有同一大小的空闲内存块 */
    unsigned int *map;          /* 指向位图的指针，这个位图用于表示内存块的占用情况 */
} free_area_t;

/* 可以被直接内存访问（Direct Memory Access, DMA）操作所使用的物理内存 */
#define ZONE_DMA 0

/* 普通的可分页内存，这部分内存可以被大部分的进程正常访问 */
#define ZONE_NORMAL 1

/* 高端内存 */
#define ZONE_HIGHMEM 2

/* 指定了系统中zone的最大数量，一般就是以上三个 */
#define MAX_NR_ZONES 3

/* 对计算机物理内存中的一个内存区域的抽象。如dma_zone，normal_zone，high_zone */
typedef struct zone_struct
{
    spinlock_t lock;                    /* 用于同步访问区域的自旋锁。这是一个用于防止多个处理器同时修改区域数据的锁 */
    unsigned long offset;               /* 表示本zone的起始页面在本内存节点所有页面中的起始位置 */
    unsigned long free_pages;           /* 表示该区域中空闲页的数量 */
    unsigned long inactive_clean_pages; /* 表示处于非活跃状态且已被清理（不含脏数据）的页的数量 */
    unsigned long inactive_dirty_pages; /* 表示处于非活跃状态且含有脏数据（即已被修改但未写回到磁盘）的页的数量 */
    /* pages_min:这是内存区域（zone）的最小页面保留阈值。当区域中空闲页面的数量降至此阈值以下时，内核会开始回收内存，
    这个阈值旨在防止内存耗尽到达临界点，确保系统始终有一定量的空闲页面可用于紧急需求。
    pages_low:这是低水位线，比 pages_min 高。当空闲页面数量降至此水位线以下时，内核会更积极地回收内存。
    低水位线是一种警告级别，表明可用内存正在变得紧张，系统需要采取措施以防止进一步降至最小阈值。
    pages_high:这是高水位线，比 pages_low 更高。当空闲页面的数量升至此水位线以上时，内核可以减少内存回收的力度，
    因为此时有足够的空闲内存可用。 */
    unsigned long pages_min, pages_low, pages_high;
    struct list_head inactive_clean_list; /* 用于管理那些非活跃且清理过的页 */
    free_area_t free_area[MAX_ORDER];     /* 这是一个数组，用于表示不同大小的空闲区域。用于伙伴系统分配内存 */
    char *name;                           /* 区域的名称 */
    unsigned long size;                   /* 表示该区域的大小 */

    struct pglist_data *zone_pgdat; /* 指向管理本节点内存的pglist_data指针 */
    unsigned long zone_start_paddr; /* 区域开始的物理地址 */
    unsigned long zone_start_mapnr; /* 区域开始的映射编号 */
    struct page *zone_mem_map;      /* 指向区域内第一页的struct page结构的指针 */
} zone_t;

/* 结构体用于管理适合某个分配类型的一组内存区域（zones），以满足不同的内存分配请求。
zones数组内每个zone_t结构体按照优先级排列，每当内核分配内存，它会找到一个zonelist_struct结构体，
然后检查GFP掩码是否与该结构体内gpf_mask值相等，然后去zones数组内第一个zone_t对应的zone分配内存。
如果该zone不能满足分配要求，那么就去zones内下一个zone_t对应的zone */
typedef struct zonelist_struct
{
    zone_t *zones[MAX_NR_ZONES + 1]; /* 一个指向 zone_t 结构的指针数组, + 1这个数组以 NULL 结尾，方便遍历确定结尾 */
    /* 这是一个整型变量，用来存储内存分配请求的 GFP（Get Free Page）掩码。
    GFP 掩码是一组标志，用于指定内存分配请求的特性，比如是否可以睡眠等待内存释放、需要哪种类型的内存（普通、DMA、高端等）。
    gfp_mask 的值决定了可以使用哪些内存区域来满足特定的内存请求 */
    int gfp_mask;
} zonelist_t;

/* 在多节点系统中，每个节点（例如在NUMA架构中的每个内存节点）都有一个pglist_data结构来描述其内存区域，
它为内核提供了一个描述单个内存节点的方式，是对内存节点的抽象 */
typedef struct pglist_data
{
    zone_t node_zones[MAX_NR_ZONES]; /* 这个数组表示节点上的内存区域，每个区域对应不同的内存类型（例如DMA、Normal、HighMem等） */

    /* 这是一个分区列表，用于内存分配。它会基于不同的分配标志（例如GFP_KERNEL、GFP_HIGHMEM等）提供合适的内存区域列表 */
    zonelist_t node_zonelists[NR_GFPINDEX];

    /* 指向该节点的页描述符数组的指针。页描述符是struct page类型的，它为节点上的每一页提供了一个描述。 */
    struct page *node_mem_map;
    // unsigned long *valid_addr_bitmap; /* 一个位图，表示此节点中有效的物理地址 */
    struct bootmem_data *bdata;     /* 指向bootmem数据的指针，该数据用于引导时的内存分配 */
    unsigned long node_start_paddr; /* 节点开始的物理地址 */
    unsigned long node_start_mapnr; /* 在全局mem_map中的开始页号。mem_map是一个全局的页描述符数组，用于描述所有的物理页。 */
    unsigned long node_size;        /* 该节点的大小（以页为单位） */
    int node_id;                    /* 节点的ID或编号 */
    struct pglist_data *node_next;  /* 指向下一个pglist_data结构的指针，通常用于通过所有的内存节点进行迭代 */
} pg_data_t;

extern pg_data_t *pgdat_list;      /* 定义在page_alloc.c中，用于管理所有节点的pg_data_t形成的单链表 */
extern pg_data_t contig_page_data; /* 定义在numa.c中，主要管理的是整个系统的物理内存页面 */

/* mm/page_alloc.c */
extern void free_area_init_core(int nid, pg_data_t *pgdat, struct page **gmap,
                                unsigned long *zones_size, unsigned long paddr,
                                unsigned long *zholes_size, struct page *pmap);

/* 目的是对给定的地址 x 进行向上对齐，确保它是 mem_map_t 类型大小的整数倍 */
#define MAP_ALIGN(x) ((((x) % sizeof(mem_map_t)) == 0) ? (x) : ((x) + sizeof(mem_map_t) - ((x) % sizeof(mem_map_t))))

#endif /* _MM_ZONE_H */