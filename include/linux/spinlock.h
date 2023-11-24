#ifndef _LINUX_SPINLOCK_H
#define _LINUX_SPINLOCK_H

/* 描述了自旋锁，属于 #ifdef CONFIG_SMP 的 #else 下 #if (__GNUC__ > 2)，
我们的gcc版本远高于2，自然就定义成空结构体。
在单处理器配置（非SMP，即单核处理器）或者不支持内核抢占的情况下，内核可能不需要实际的自旋锁机制。
在这种情况下，自旋锁可以被定义为空结构体，因为不存在真正的并发访问共享资源的问题。这样做可以减少内核的复杂性并提高效率 */
typedef struct
{
} spinlock_t;

/* 初始化一个自旋锁（定义成空结构体）为解锁状态，
这个宏实际上创建了一个空的spinlock_t实例 */
#define SPIN_LOCK_UNLOCKED \
    (spinlock_t) {}

#endif /* _LINUX_SPINLOCK_H */