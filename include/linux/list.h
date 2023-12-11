#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H

/* 初始化链表头，链表头节点的 next 和 prev 指针指向自身 */
#define INIT_LIST_HEAD(ptr)  \
    do                       \
    {                        \
        (ptr)->next = (ptr); \
        (ptr)->prev = (ptr); \
    } while (0)

/* 双链表节点 */
struct list_head
{
    struct list_head *next, *prev;
};

/* 初始化一个双链表节点，就是让prev和next均指向自己 */
#define INIT_LIST_HEAD(ptr)  \
    do                       \
    {                        \
        (ptr)->next = (ptr); \
        (ptr)->prev = (ptr); \
    } while (0)

#endif /* _LINUX_LIST_H */