#ifndef _LINUX_SWAPCTL_H
#define _LINUX_SWAPCTL_H
/*  用于控制和管理交换（swap）空间 */

/* 对整个系统内存中的剩余空间（即空闲页）的抽象 */
typedef struct freepages_v1
{
    unsigned int min;  /* 保留内存页的最低阈值。当系统的空闲内存页数降至此水平时，内核开始采取行动，释放内存页以维持系统的基本运行 */
    unsigned int low;  /* 预警阈值。当空闲内存页数降至此水平时，内核更积极地释放内存 */
    unsigned int high; /* 内存充足的阈值。当空闲内存页数高于此水平时，内核可以更自由地分配内存 */
} freepages_v1;
typedef freepages_v1 freepages_t;

/* mm/swap.c */
extern freepages_t freepages;

#endif /* _LINUX_SWAPCTL_H */