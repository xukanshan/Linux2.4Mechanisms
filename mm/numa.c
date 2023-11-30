#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/bootmem.h>
#include <linux/mmzone.h>
#include <linux/spinlock.h>

/* 文件主要涉及非一致性内存访问NUMA（Non-Uniform Memory Access）架构的内存管理。
NUMA是一种多处理器计算机内存设计配置，用于处理多个处理器之间的内存访问不一致性问题。
在这种架构下，每个处理器或处理器组都有自己的本地内存，处理器访问本地内存的速度会比访问远程处理器的内存快 */

/* contig_bootmem_data的作用是保存引导期间内存分配器的状态和信息
contig是连续（contiguous）的缩写，指的是这部分内存是连续的一块区域。
bootmem指的是引导内存（boot memory），即在系统启动过程中用到的内存。 */
static bootmem_data_t contig_bootmem_data;

/* 定义并初始化了一个pg_data_t类型的变量contig_page_data，
该变量表示整个系统第一个节点的pg_data_t（如果整个系统就一个节点，那么这个当然是代表整个系统的pg_data_t）
其成员bdata被设置为指向contig_bootmem_data */
pg_data_t contig_page_data = {bdata : &contig_bootmem_data};