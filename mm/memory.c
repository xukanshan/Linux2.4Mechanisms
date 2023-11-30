#include <linux/mm.h>
#include <linux/smp_lock.h>
#include <linux/swapctl.h>

/* 用来表示整个系统的物理内存页数组 */
mem_map_t *mem_map;