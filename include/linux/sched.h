#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#define INIT_TASK_SIZE 2048 * sizeof(long)

/* 未来再来填充这个, 设定为8K大小 */
struct task_struct
{
    unsigned long tmp[2048];
};

#define INIT_TASK(tsk) \
    {                  \
        .tmp = { 0 }   \
    }

/*
 * union task_union 被设计用来合并并管理 task_struct 和内核栈所在的一个连续8K内存空间，
 * 因为通常我们将 task_struct（8k的低位置开始处） 和其对应的内核栈（栈底在8k结束，栈向低地址扩展）放在一起
 */
union task_union
{
    struct task_struct task;
    unsigned long stack[INIT_TASK_SIZE / sizeof(long)];
};

/* 它是对一个进程的内存空间的抽象表示,是对一个进程在内存中的映射和状态的全面描述*/
struct mm_struct
{
    // struct vm_area_struct *mmap;                              /* 指向虚拟内存区域（Virtual Memory Areas, VMAs）的链表头 */
    // struct vm_area_struct *mmap_avl;                          /* 指向VMAs的AVL树，用于更快的搜索和管理这些区域 */
    // struct vm_area_struct *mmap_cache;                        /* l缓存最近查找的VMA，以加速连续的相同查找 */
    // pgd_t *pgd;                                               /* 指向页全局目录（Page Global Directory） */
    // atomic_t mm_users;                                        /* 记录有多少用户进程正在使用这个内存空间 */
    // atomic_t mm_count;                                        /* 记录有多少个引用指向这个mm_struct结构体 */
    // int map_count;                                            /* 记录当前VMAs的数量 */
    // struct semaphore mmap_sem;                                /* 用于内存映射区域的信号量，保证对VMAs的并发访问的同步 */
    // spinlock_t page_table_lock;                               /* 自旋锁，用于保护页表操作的同步 */
    // struct list_head mmlist;                                  /* 链接所有活动的mm_struct实例的列表 */
    unsigned long start_code, end_code, start_data, end_data; /* 代码段与数据段的起始和结束地址 */
    unsigned long start_brk, brk, start_stack;                /* 动态内存分配（堆区）的起始地址和当前位置，栈的起始地址 */
    // unsigned long arg_start, arg_end, env_start, env_end;     /*  程序参数和环境变量的起始和结束地址 */
    // unsigned long rss, total_vm, locked_vm;                   /* 分别表示常驻集大小、总虚拟内存和锁定的虚拟内存大小 */
    // unsigned long def_flags;                                  /* 默认的内存区域标志 */
    // unsigned long cpu_vm_mask;                                /*  CPU虚拟内存掩码，用于处理器特定的内存管理 */
    // unsigned long swap_cnt;                                   /* 记录下一次交换操作要交换的页面数 */
    // unsigned long swap_address;                               /* 用于交换操作的地址 */

    // mm_context_t context; /* 体系结构特定的内存管理上下文，用于存储与特定硬件平台相关的信息 */
};

/* 该宏用于初始化一个mm_struct */
#define INIT_MM(name)                                         \
    {                                                         \
        /* .mmap = &init_mmap, */                             \
        /* .mmap_avl = NULL, */                               \
        /* .mmap_cache = NULL, */                             \
        /* .pgd = swapper_pg_dir, */                          \
        /* .mm_users = ATOMIC_INIT(2), */                     \
        /* .mm_count = ATOMIC_INIT(1), */                     \
        /* .map_count = 1, */                                 \
        /* .mmap_sem = __MUTEX_INITIALIZER(name.mmap_sem), */ \
        /* .page_table_lock = SPIN_LOCK_UNLOCKED, */          \
        /* .mmlist = LIST_HEAD_INIT(name.mmlist), */          \
    }

/* 声明swapper的mm_struct */
extern struct mm_struct init_mm;

#endif /* _LINUX_SCHED_H */