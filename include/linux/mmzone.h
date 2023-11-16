#ifndef _MM_ZONE_H
#define _MM_ZONE_H
/* 主要用于定义与内存管理相关的数据结构和函数 */

/* 在多节点系统中，每个节点（例如在NUMA架构中的每个内存节点）都有一个pglist_data结构来描述其内存区域，
它为内核提供了一个描述单个内存节点的方式，是对内存节点的抽象 */
typedef struct pglist_data
{
    // zone_t node_zones[MAX_NR_ZONES]; /* 这个数组表示节点上的内存区域，每个区域对应不同的内存类型（例如DMA、Normal、HighMem等） */

    // /* 这是一个分区列表，用于内存分配。它会基于不同的分配标志（例如GFP_KERNEL、GFP_HIGHMEM等）提供合适的内存区域列表 */
    // zonelist_t node_zonelists[NR_GFPINDEX];

    // /* 这是指向该节点的页描述符数组的指针。页描述符是struct page类型的，它为节点上的每一页提供了一个描述。 */
    // struct page *node_mem_map;
    // unsigned long *valid_addr_bitmap; /* 一个位图，表示此节点中有效的物理地址 */
    struct bootmem_data *bdata; /* 指向bootmem数据的指针，该数据用于引导时的内存分配 */
    // unsigned long node_start_paddr;   /* 节点开始的物理地址 */
    // unsigned long node_start_mapnr;   /* 在全局mem_map中的开始页号。mem_map是一个全局的页描述符数组，用于描述所有的物理页。 */
    // unsigned long node_size;          /* 该节点的大小（以页为单位） */
    // int node_id;                      /* 节点的ID或编号 */
    struct pglist_data *node_next; /* 指向下一个pglist_data结构的指针，通常用于通过所有的内存节点进行迭代 */
} pg_data_t;

extern pg_data_t *pgdat_list;      /* 定义在page_alloc.c中，用于管理所有节点的pg_data_t形成的单链表 */
extern pg_data_t contig_page_data; /* 定义在numa.c中，主要管理的是整个系统的物理内存页面 */

#endif /* _MM_ZONE_H */