#include <linux/swapctl.h>

/* 创了一个freepages_t 实例 freepages，并全部初始化为0 */
freepages_t freepages = {
    0,
    0,
    0
};