#include <linux/mm.h>
#include <linux/swapctl.h>
#include <linux/init.h>
#include <asm-i386/dma.h>
#include <asm-i386/pgtable.h>

/* 创了一个freepages_t 实例 freepages，并全部初始化为0 */
freepages_t freepages = {
    0,
    0,
    0};

/* 该变量反映了系统在特定时间内面临的内存压力。包含了系统在一分钟内进行的页面窃取
（指的是操作系统从内存中的某些部分（如非活动页面列表）回收页面的过程）数量的平均值。
使用这个值来确定我们应该拥有多少非活动页面。如果页面窃取的数量较高，这可能意味着系统需要更多的非活动页面来应对内存压力
它的值随着内存分配请求（如页面分配）的增加而增加，而随着页面释放的操作而减少
在 reclaim_page（与页面分配有关） 和 __alloc_pages（与页面分配有关） 中：memory_pressure++
在 __free_pages_ok（与页面释放有关） 中：memory_pressure--
在 recalculate_vm_stats 中，该值会衰减（每秒一次） */
int memory_pressure;