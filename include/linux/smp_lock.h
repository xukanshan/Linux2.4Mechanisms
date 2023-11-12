#ifndef _LINUX_SMPLOCK_H
#define _LINUX_SMPLOCK_H

/* 这个锁用于初始化，包含在 #ifndef CONFIG_SMP 中，在多核系统中，我们肯定只能让一个主cpu去完成初始化工作，
所以其他cpu必须锁住，但是我们的Linux2.4复现只用于单核之上，所以这个锁就是什么都不做 */
#define lock_kernel() \
    do                \
    {                 \
    } while (0)

#endif /* _LINUX_SMPLOCK_H */