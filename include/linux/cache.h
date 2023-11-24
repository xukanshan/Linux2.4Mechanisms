#ifndef _LINUX_CACHE_H
#define _LINUX_CACHE_H
/* 定义与CPU缓存对齐相关的宏和属性 */

#include <asm-i386/cache.h>

/* 定义在对称多处理（SMP）系统中用于缓存对齐的字节数，最后结果是32 */
#define SMP_CACHE_BYTES L1_CACHE_BYTES

#endif /* _LINUX_CACHE_H */