#include <linux/mm.h>
#include <linux/smp_lock.h>
#include <linux/swapctl.h>

/* 用来记录最大可映射物理页面数量，如果CONFIG_HIGHMEM，那么就是高端内存结束的页面，
如果没有CONFIG_HIGHMEM，那么自然就是低端内存结束页面 */
unsigned long max_mapnr;

/* 用来记录物理页面的数量 */
unsigned long num_physpages;

/* 记录内核可以直接寻址的边界 */
void * high_memory;

/* 用来表示整个系统的物理内存页数组 */
mem_map_t *mem_map;