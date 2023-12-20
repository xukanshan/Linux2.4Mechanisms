#ifndef _LINUX_SWAP_H
#define _LINUX_SWAP_H

/* 页面年龄的初始值，初始值不是0或者1可能是为了防止页面在刚被分配后很快又被交换出去，
如果一个页面的年龄开始于 0 或 1，那么它可能会在很短的时间内被认为是一个换出的候选。
又或者是0 或 1 可能有特殊的含义或被用于特殊的目的*/
#define PAGE_AGE_START 2

/* mm/swap.c */
extern int memory_pressure;

#endif /* _LINUX_SWAP_H */