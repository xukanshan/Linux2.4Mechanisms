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

/* 描述了自旋锁，属于 #ifdef CONFIG_SMP 的 #else 下 #if (__GNUC__ > 2)，
为什么定义成空结构体，原因与spinlock_t定义成空结构体相同 */
typedef struct
{
} rwlock_t;

/* 初始化一个读写锁（定义成空结构体）为解锁状态，
这个宏实际上创建了一个空的rwlock_t实例 */
#define RW_LOCK_UNLOCKED \
    (rwlock_t) {}

/* 属于 #ifdef CONFIG_SMP 的 #else 下 #if，这个宏用于读写锁的写者上锁，但是实际是什么都没有做，
lock 被转换为 void 类型。这实际上是一种通用的方法来显式地忽略一个变量或参数，避免编译器警告 unused variable */
#define write_lock(lock) (void)(lock)

/* 属于 #ifdef CONFIG_SMP 的 #else 下 #if，这个宏用于读写锁的写者解锁，但是实际是什么都没有做 */
#define write_unlock(lock) \
    do                     \
    {                      \
    } while (0)

#endif /* _LINUX_SPINLOCK_H */