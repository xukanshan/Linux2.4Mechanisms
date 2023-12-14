#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>

#define CACHE_NAMELEN 20 /* slab缓存池管理的对象名字长度 */

/* 对 SLAB 缓存池的抽象，其实例代表一类特定类型的对象（如 task_struct）。
每个slab缓存池内有大量的slab，然后每个slab会被划分成固定大小的区域用于固定大小的内存分配，
如task_struct的分配 */
struct kmem_cache_s
{
    struct list_head slabs;         /* 管理所有 slab 的链表头 */
    struct list_head *firstnotfull; /* 指向第一个非满的 slab */
    unsigned int objsize;           /* 管理的每个对象的大小，比如task_struct的大小 */
    unsigned int flags;             /* 缓存的常量标志，定义了缓存的特定属性和行为 */
    unsigned int num;               /* 每个 slab 中的对象数量 */
    spinlock_t spinlock;            /* 保护该缓存池的自旋锁 */
    unsigned int gfporder;          /* 定义了每个 slab 分配内存时所需的页框数（以 2 的幂表示） */
    unsigned int gfpflags;          /* 分配内存时使用的 GFP（Get Free Page）标志，可以指定内存分配的行为 */
    size_t colour;                  /* 缓存着色范围。着色是一种内存优化技术，用于减少 CPU 缓存冲突 */
    unsigned int colour_off;        /* 着色的偏移量 */
    unsigned int colour_next;       /* 下一个要使用的着色值 */
    /* 指向的slab缓冲池用于管理指向 slab 本身的指针，比如一个 slab 缓存池专门用于 task_struct 的分配，
    slabp_cache 指向的缓存池就是用来管理这个 task_struct 缓存池中所有 slab 的指针，
    当需要分配或回收一个对象时，可以快速通过 slabp_cache 定位到对应的 slab */
    kmem_cache_t *slabp_cache;
    unsigned int growing; /* 指示该缓存是否正在增长 */
    unsigned int dflags;  /* 动态标志，用于运行时控制缓存行为 */
    /* 构造和析构函数指针，用于在对象被创建或销毁时执行特定操作 */
    void (*ctor)(void *, kmem_cache_t *, unsigned long);
    void (*dtor)(void *, kmem_cache_t *, unsigned long);
    unsigned long failures;   /* 记录缓存分配失败的次数 */
    char name[CACHE_NAMELEN]; /* 缓存的名称 */
    struct list_head next;    /* 将这个缓存池结构链接到全局缓存池链表 */
};

/* 创建了一个叫cache_cache的slab缓存池，并进行了初始化
它用于管理所有的slab缓存池，也就是管理其他的kmem_cache_t 结构体 */
static kmem_cache_t cache_cache = {
    .slabs = LIST_HEAD_INIT(cache_cache.slabs),
    .firstnotfull = &cache_cache.slabs,
    .objsize = sizeof(kmem_cache_t),
    .flags = SLAB_NO_REAP,
    .spinlock = SPIN_LOCK_UNLOCKED,
    .colour_off = L1_CACHE_BYTES,
    .name = "kmem_cache",
};

/* 信号量，用于操作slab缓存池 */
static struct semaphore cache_chain_sem;

/* 用于访问总slab缓存池的下一个，也就是第一个真正用于资源管理的slab缓存池 */
#define cache_chain (cache_cache.next)

typedef unsigned int kmem_bufctl_t;

/* 一个内存 slab 的抽象 */
typedef struct slab_s
{
    /*  slab 分配器中，slabs 通常被组织成几个链表，如完全使用的 slab、部分使用的 slab 和完全空闲的 slab */
    struct list_head list;
    /* 存储“颜色偏移”（colour offset）。在 slab 分配器中，为了优化 CPU 缓存的使用，
    相邻的对象可能会被赋予不同的内存地址偏移。这种技术称为“缓存着色” */
    unsigned long colouroff;
    /* 这是指向 slab 中第一个对象的指针。它可能包含了上面提到的颜色偏移量，这意味着实际的内存地址可能被调整过 */
    void *s_mem;
    /* 这个成员表示当前 slab 中有多少对象正在被使用 */
    unsigned int inuse;
    /* 指向 slab 中下一个空闲对象 */
    kmem_bufctl_t free;
} slab_t;

/* 规定了一个slab内可供固定分配的内存区域大小的上限 */
#define SLAB_LIMIT 0xffffFFFE

/* 计算在给定空间大小下，这个slab能够存放多少个
固定大小的区域（考虑到缓存对齐和额外开销）。参数：
gfporder 表示分配的页面数量（以 2 的幂次表示），
size 是每个缓存对象的大小，
flags 是控制标志，
left_over 用于存储剩余空间大小的指针，
num 是存储每页可以放置对象数量的指针 */
static void kmem_cache_estimate(unsigned long gfporder, size_t size, int flags, size_t *left_over, unsigned int *num)
{
    int i;                                  /* 用于循环 */
    size_t wastage = PAGE_SIZE << gfporder; /* 计算总可用空间大小 */
    size_t extra = 0;                       /* 计算每个缓存对象额外的开销 */
    size_t base = 0;                        /* 计算每个slab的基本开销 */

    if (!(flags & CFLGS_OFF_SLAB)) /* 检查传入flags是否设置了CFLGS_OFF_SLAB */
    {                              /* 进来就是没设置 */

        base = sizeof(slab_t);         /* 计算每个slab的基本开销 */
        extra = sizeof(kmem_bufctl_t); /* slab管理的每个对象都需要一个kmem_bufctl_t */
    }
    i = 0;
    /* 计算可以放入给定内存空间的缓存对象的最大数量，
    L1_CACHE_ALIGN 用于确保内存对齐，以优化 CPU 缓存的效率 */
    while (i * size + L1_CACHE_ALIGN(base + i * extra) <= wastage)
        i++;
    if (i > 0) /* 上面循环结束i会比真实能放入数量大1，这里对 i 进行调整 */
        i--;

    if (i > SLAB_LIMIT)
        i = SLAB_LIMIT; /* 确保 i 不超过定义的 SLAB_LIMIT 上限 */

    *num = i;                                    /* 将i赋值给num指针 */
    wastage -= i * size;                         /* 剩余内存空间减去用于实际分配的空间 */
    wastage -= L1_CACHE_ALIGN(base + i * extra); /* 剩余内存空间要减去开销 */
    *left_over = wastage;                        /* 将剩余空间赋值给left_over指针 */
}

/* 初始化 slab 分配器: 核心就是初始化了cache_cache（总slab缓存池，
管理所有的slab缓存池，也就是kmem_cache_t结构体的分配） */
void __init kmem_cache_init(void)
{
    size_t left_over;             /* 存储后续计算中剩余的内存大小 */
    init_MUTEX(&cache_chain_sem); /* 初始化信号量 */
    INIT_LIST_HEAD(&cache_chain); /* 初始化总的slab缓存池的next成员 */
    /* 调用后left_over存放这页剩余内存大小，cache_cache.num存放一页能够存放多少个cache_cache.num */
    kmem_cache_estimate(0, cache_cache.objsize, 0, &left_over, &cache_cache.num);
    if (!cache_cache.num) /* 如果一页放不下一个cache_cache.num，那么报错 */
        BUG();
    cache_cache.colour = left_over / cache_cache.colour_off; /* 计算“颜色”（colouring）的数量 */
    /* 初始化 colour_next，这是用于跟踪下一个分配应使用的颜色偏移的变量 */
    cache_cache.colour_next = 0;
}