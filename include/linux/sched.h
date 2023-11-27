#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <asm-i386/atomic.h>
#include <linux/stddef.h>

/* 定义了初始化时候使用的栈大小，共8K */
#define INIT_TASK_SIZE 2048 * sizeof(long)

/* 进程或线程的身份证 */
struct task_struct
{
    /* linux2.4这里有条提醒：结构体成员的内存偏移量在代码的其他部分已经固定下来了。
    因此，如果需要修改这些元素，需要非常谨慎，以避免破坏其他依赖于这些偏移量的代码部分 */

    /* 基本进程状态和控制信息 */
    // volatile long state; /* 进程的状态。可能的值有 -1（不可运行）、0（可运行）、>0（已停止） */
    // unsigned long flags; /* 进程标志，用于表示不同的进程状态或属性 */
    // int sigpending;      /* 表示是否有未处理的信号 */
    /* 进程的地址空间限制。用于区分用户态线程和内核线程的地址空间,
    0-0xBFFFFFFF for user-thead, 0-0xFFFFFFFF for kernel-thread */
    // mm_segment_t addr_limit;
    // struct exec_domain *exec_domain; /* 执行域，用于进程调度 */
    // volatile long need_resched;      /* 标记是否需要重新调度 */
    // unsigned long ptrace;            /* ptrace 标志，用于调试 */
    // int lock_depth;                  /* 内核锁深度，用于同步 */

    /* 调度和性能相关 */
    // long counter;               /* 进程运行时间计数器，用于调度决策 */
    // long nice;                  /* 进程的“nice”值，影响调度优先级 */
    // unsigned long policy;       /* 调度策略 */
    /* 指向描述进程的虚拟内存空间的 mm_struct 结构体, 内核线程的情况下，
    这个字段通常是 NULL，因为内核线程不拥有独立的地址空间，它们直接在内核空间运行 */
    struct mm_struct *mm;
    // int has_cpu, processor;     /* 分别标记进程是否在运行和运行的处理器编号 */
    // unsigned long cpus_allowed; /* 允许进程运行的 CPU 集合 */
    // struct list_head run_list;  /* 运行队列链表 */
    // unsigned long sleep_time;   /* 进程睡眠时间 */

    /* 任务链表和内存管理 */
    // struct task_struct *next_task, *prev_task; /* 指向链表中下一个和前一个任务的指针 */
    /*描述了当前 CPU 正在使用的内存管理信息， 在大多数情况下，对于用户空间进程，active_mm 与 mm 相同。
    但在特定情况下，比如在内核线程执行期间，active_mm 会指向最后运行的用户空间进程的 mm_struct。
    这允许内核线程运行在用户态进程的地址空间中，而不需要频繁地切换页表 */
    struct mm_struct *active_mm;

    /* 进程状态和控制 */
    // struct linux_binfmt *binfmt; /* 用于进程的二进制格式 */
    // int exit_code, exit_signal;  /* 进程退出时的状态码和信号 */
    // int pdeath_signal;           /*  父进程死亡时发送的信号  */
    // unsigned long personality;   /* 进程的执行特性。 */
    // int dumpable : 1;            /* 标记进程是否可转储 */
    // int did_exec : 1;            /* 标记进程是否执行过 */
    // pid_t pid;                   /* 进程的各种标识符，如进程 ID、进程组 ID、会话 ID 等 */
    // pid_t pgrp;
    // pid_t tty_old_pgrp;
    // pid_t session;
    // pid_t tgid;
    // int leader; /* 是否为会话组领导 */
    /*分别指向（原始）父进程、最年幼的子进程、较年轻的兄弟姐妹、
    较年长的兄弟姐妹。 (p->father 可以被p->p_pptr->pid 替代) */
    // struct task_struct *p_opptr, *p_pptr, *p_cptr, *p_ysptr, *p_osptr;
    // struct list_head thread_group; /* 线程组链表 */

    /* PID 散列表和等待队列 */
    // struct task_struct *pidhash_next;   /* PID 散列表中的下一个任务 */
    // struct task_struct **pidhash_pprev; /* PID 散列表中的上一个任务 */
    // wait_queue_head_t wait_chldexit;    /* 等待子进程退出的队列 */
    // struct semaphore *vfork_sem;        /* 用于 vfork() 的信号量 */

    /* 实时调度和计时器 */
    // unsigned long rt_priority;                                 /* 实时调度优先级 */
    // unsigned long it_real_value, it_prof_value, it_virt_value; /* 用于不同类型的间隔计时 */
    // unsigned long it_real_incr, it_prof_incr, it_virt_incr;
    // struct timer_list real_timer;                        /* 实时计时器 */
    // struct tms times;                                    /* 记录用户和系统时间的结构 */
    // unsigned long start_time;                            /* 进程开始时间 */
    // long per_cpu_utime[NR_CPUS], per_cpu_stime[NR_CPUS]; /* 每个 CPU 的用户和系统时间 */

    /* 内存管理错误和交换信息：这可以被认为是特定于内存管理（mm）或特定于线程的 */
    // unsigned long min_flt, maj_flt, nswap, cmin_flt, cmaj_flt, cnswap; /* 页面错误和交换次数统计 */
    // int swappable : 1;                                                 /* 是否可交换 */

    /* 进程凭证 */
    // uid_t uid, euid, suid, fsuid; /* 用户和组 ID */
    // gid_t gid, egid, sgid, fsgid;
    // int ngroups;
    // gid_t groups[NGROUPS];                                      /* 所属组信息 */
    // kernel_cap_t cap_effective, cap_inheritable, cap_permitted; /* 进程能力标记 */
    // int keep_capabilities : 1;                                  /* 是否保留能力 */
    // struct user_struct *user;                                   /* 用户数据 */

    /* 资源限制和其他信息 */
    // struct rlimit rlim[RLIM_NLIMITS]; /* 资源限制数组 */
    // unsigned short used_math;         /* 是否使用了数学协处理器 */
    // char comm[16];                    /* 进程名称 */

    /* 文件系统和信号处理 */
    // int link_count; /* 链接计数和终端信息 */
    // struct tty_struct *tty;
    // unsigned int locks;       /* 持有的文件锁数量 */
    // struct sem_undo *semundo; /* IPC 信号量信息 */
    // struct sem_queue *semsleeping;
    // struct thread_struct thread; /* CPU 特定的状态信息 */
    // struct fs_struct *fs;        /* 文件系统信息 */
    // struct files_struct *files;  /* 打开文件信息 */
    // spinlock_t sigmask_lock;     /* 信号掩码锁 */
    // struct signal_struct *sig;   /* 信号处理信息 */
    // sigset_t blocked;
    // struct sigpending pending;

    /* 扩展和锁 */
    // unsigned long sas_ss_sp; /* 用于信号处理的栈信息 */
    // size_t sas_ss_size;
    // int (*notifier)(void *priv); /*  通知回调和相关数据 */
    // void *notifier_data;
    // sigset_t *notifier_mask;
    // u32 parent_exec_id; /* 执行 ID */
    // u32 self_exec_id;
    // spinlock_t alloc_lock; /* 分配锁 */
};

/* 在启动阶段初始化swapper 的 task_struct  */
#define INIT_TASK(tsk)                                                           \
    {                                                                            \
        /* .state = 0, */                                                        \
        /* .flags = 0, */                                                        \
        /* .sigpending = 0, */                                                   \
        /* .addr_limit = KERNEL_DS, */                                           \
        /* .exec_domain = &default_exec_domain, */                               \
        /* .lock_depth = -1, */                                                  \
        /* .counter = DEF_COUNTER, */                                            \
        /* .nice = DEF_NICE, */                                                  \
        /* .policy = SCHED_OTHER, */                                             \
        .mm = NULL,                                                              \
        .active_mm = &init_mm,                                                   \
        /* .cpus_allowed = -1, */                                                \
        /* .run_list = LIST_HEAD_INIT(tsk.run_list), */                          \
        /* .next_task = &tsk, */                                                 \
        /* .prev_task = &tsk, */                                                 \
        /* .p_opptr = &tsk, */                                                   \
        /* .p_pptr = &tsk, */                                                    \
        /* .thread_group = LIST_HEAD_INIT(tsk.thread_group), */                  \
        /* .wait_chldexit = __WAIT_QUEUE_HEAD_INITIALIZER(tsk.wait_chldexit), */ \
        /* .real_timer = {.function = it_real_fn}, */                            \
        /* .cap_effective = CAP_INIT_EFF_SET, */                                 \
        /* .cap_inheritable = CAP_INIT_INH_SET, */                               \
        /* .cap_permitted = CAP_FULL_SET, */                                     \
        /* .keep_capabilities = 0, */                                            \
        /* .rlim = INIT_RLIMITS, */                                              \
        /* .user = INIT_USER, */                                                 \
        /* .comm = "swapper", */                                                 \
        /* .thread = INIT_THREAD, */                                             \
        /* .fs = &init_fs, */                                                    \
        /* .files = &init_files, */                                              \
        /* .sigmask_lock = SPIN_LOCK_UNLOCKED, */                                \
        /* .sig = &init_signals, */                                              \
        /* .pending = {NULL, &tsk.pending.head, {{0}}}, */                       \
        /* .blocked = {{0}}, .alloc_lock = SPIN_LOCK_UNLOCKED */                 \
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

/* 它是对一个进程的虚拟内存空间的抽象表示, 是对一个进程在内存中的映射和状态的全面描述*/
struct mm_struct
{
    // struct vm_area_struct *mmap;                              /* 指向虚拟内存区域（Virtual Memory Areas, VMAs）的链表头 */
    // struct vm_area_struct *mmap_avl;                          /* 指向VMAs的AVL树，用于更快的搜索和管理这些区域 */
    // struct vm_area_struct *mmap_cache;                        /* l缓存最近查找的VMA，以加速连续的相同查找 */
    // pgd_t *pgd;                                               /* 指向页全局目录（Page Global Directory） */
    // atomic_t mm_users;                                        /* 记录有多少用户进程正在使用这个内存空间 */
    atomic_t mm_count; /* 记录有多少个引用指向这个mm_struct结构体 */
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

/* 该宏用于初始化一个mm_struct。
mm_count初始化为1：初始引用：当 mm_struct 实例被创建时，它至少有一个用户（通常是创建它的进程或内核线程）。
内核使用：在 Linux 内核启动时，会创建一个名为 init_mm 的特殊 mm_struct 实例。
这个实例代表了内核的内存映射。由于内核始终在运行，因此 init_mm 总是至少有一个引用（即内核本身的引用）*/
#define INIT_MM(name)                                         \
    {                                                         \
        /* .mmap = &init_mmap, */                             \
        /* .mmap_avl = NULL, */                               \
        /* .mmap_cache = NULL, */                             \
        /* .pgd = swapper_pg_dir, */                          \
        /* .mm_users = ATOMIC_INIT(2), */                     \
        .mm_count = ATOMIC_INIT(1),                           \
        /* .map_count = 1, */                                 \
        /* .mmap_sem = __MUTEX_INITIALIZER(name.mmap_sem), */ \
        /* .page_table_lock = SPIN_LOCK_UNLOCKED, */          \
        /* .mmlist = LIST_HEAD_INIT(name.mmlist), */          \
    }

/* arch/i386/kernel/init_task.c */
extern struct mm_struct init_mm;

/* arch/i386/kernel/trap.c */
extern void trap_init(void);

/* arch/i386/kernel/sched.h */
extern void cpu_init(void);

/* arch/i386/kernel/init_task.c */
extern union task_union init_task_union;

/* arch/i386/kernel/init_task.c */
extern struct mm_struct init_mm;

#endif /* _LINUX_SCHED_H */