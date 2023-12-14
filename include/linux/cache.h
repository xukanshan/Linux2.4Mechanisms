#ifndef _LINUX_CACHE_H
#define _LINUX_CACHE_H
/* 定义与CPU缓存对齐相关的宏和属性 */

#include <asm-i386/cache.h>

/* 定义在对称多处理（SMP）系统中用于缓存对齐的字节数，最后结果是32 */
#define SMP_CACHE_BYTES L1_CACHE_BYTES

/* 位于#ifndef __cacheline_aligned 下的 #ifdef MODULE 下的 #else，
确保某个数据结构或变量在内存中的对齐方式符合 CPU 缓存行的大小。这样做的目的是为了提高缓存的效率和性能。
并且数据放入一个内存段中，它专门用于存放需要缓存行对齐的数据，有助于减少缓存行冲突和提高内存访问效率*/
#define __cacheline_aligned                      \
    __attribute__((__aligned__(SMP_CACHE_BYTES), \
                   __section__(".data.cacheline_aligned")))

#define ____cacheline_aligned __attribute__((__aligned__(SMP_CACHE_BYTES)))

/* 让数据根据cpu L1缓存大小对齐，就是向上对齐到最近的L1大小的倍数 */
#define L1_CACHE_ALIGN(x) (((x) + (L1_CACHE_BYTES - 1)) & ~(L1_CACHE_BYTES - 1))

#endif /* _LINUX_CACHE_H */