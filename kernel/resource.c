#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/malloc.h>
#include <linux/spinlock.h>
#include <asm-i386/io.h>


/* 资源被组织成一个树状结构，ioport_resource 和 iomem_resource 作为根资源，是这棵树的顶端。
所有的设备驱动程序在使用这些资源前必须先向内核注册它们所需的资源，以确保资源不会被多个驱动程序重复使用。
这种机制有助于操作系统有效地管理和分配硬件资源，避免资源冲突 */
/* 表示系统中所有I/O端口资源的根资源 */
struct resource ioport_resource = {"PCI IO", 0x0000, IO_SPACE_LIMIT, IORESOURCE_IO};

/* 表示系统中所有内存资源的根资源 */
struct resource iomem_resource = {"PCI mem", 0x00000000, 0xffffffff, IORESOURCE_MEM};

/* 在资源树中找到一个没有冲突的位置来插入新资源。如果找到了合适的位置，
新资源会被加入到树中；如果存在冲突，函数会返回指向冲突资源的指针，参数：
root 是一个资源树的根节点，代表一个更大的资源集合，而 new 是希望添加到这个资源树中的新资源 */
static struct resource *__request_resource(struct resource *root, struct resource *new)
{
    unsigned long start = new->start; /* start 是新资源的起始 */
    unsigned long end = new->end;     /* end 是新资源的起结束地址 */
    struct resource *tmp, **p;        /* tmp用于遍历资源树, p用于跟踪和修改资源树 */

    /* 这几行代码检查新资源的起始和结束地址是否有效。
    如果 end 小于 start，或者 new 资源的范围超出了 root 资源的范围
    （start 小于 root->start 或 end 大于 root->end），
    函数返回 root 资源，表示有冲突 */
    if (end < start)
        return root;
    if (start < root->start)
        return root;
    if (end > root->end)
        return root;
    p = &root->child; /* p 初始化为指向 root 资源的第一个子资源 */
    for (;;)          /* 遍历资源树 */
    {
        /* 这一行从 p 指向的地方取出一个资源节点（tmp）。
        p 最初指向 root 的子节点，随后在循环中会更新以指向其他节点。 */
        tmp = *p;
        /* 如果 tmp 为 NULL（即该位置为空），
        或者 tmp 的起始地址大于新资源的结束地址（tmp->start > end），
        则表示找到了可以插入新资源的地方 */
        if (!tmp || tmp->start > end)
        {
            new->sibling = tmp; /* 将新资源的 sibling 指针指向当前 tmp 节点，即将新资源插入到当前节点之前。 */
            *p = new;           /* 更新父节点的子节点指针（或当前节点的兄弟节点指针）来指向新资源 */
            new->parent = root; /* 设置新资源的父节点 */
            return NULL;        /*  返回 NULL 表示没有冲突，新资源已成功插入 */
        }
        p = &tmp->sibling; /* 更新 p 来指向当前节点的下一个兄弟节点，以继续遍历 */
        /* 如果当前节点的结束地址小于新资源的起始地址（tmp->end < start），
        使用 continue 语句跳过当前循环迭代，继续循环的下一次迭代 */
        if (tmp->end < start)
            continue;
        /* 如果不满足上述条件，表示找到了一个与新资源冲突的节点。
        在这种情况下，返回指向这个冲突节点的指针 tmp */
        return tmp;
    }
}

/* 一个全局的读写锁，用于控制对资源树访问的并发访问 */
static rwlock_t resource_lock = RW_LOCK_UNLOCKED;

/* 将新资源添加到系统资源树中的函数。它通过获取全局锁来保证操作的线程安全，
检查新资源是否与现有资源冲突，如果没有冲突，则将新资源添加到资源树中。两个参数：
root 是一个资源树的根节点，代表一个更大的资源集合，而 new 是希望添加到这个资源树中的新资源 */
int request_resource(struct resource *root, struct resource *new)
{
    struct resource *conflict; /* 用来接收来自 __request_resource 函数的返回值，表示是否有资源冲突 */

    write_lock(&resource_lock); /* 获取一个写入锁定 */
    /* 将 new 资源添加到 root 资源树中， 如果 new 资源与 root 资源树中的现有资源有冲突，
    __request_resource 会返回指向冲突资源的指针 */
    conflict = __request_resource(root, new);
    /* 释放之前获得的写入锁定 */
    write_unlock(&resource_lock);
    /* 如果 conflict 不为 NULL（意味着存在资源冲突），函数返回 -EBUSY（一个错误码，表示资源忙）。
    如果没有冲突，函数返回 0，表示资源请求成功 */
    return conflict ? -EBUSY : 0;
}
