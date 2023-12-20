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

/* 初始化链表，让两个节点都指向自己 */
#define LIST_HEAD_INIT(name) \
    {                        \
        &(name), &(name)     \
    }

/* 从一个双链表中删除一个节点，参数：
prev：要删除的节点的前一个节点
next：要删除的节点的后一个节点*/
static __inline__ void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

/* 从一个双链表中删除一个节点，参数：
entry：要删除的链表节点 */
static __inline__ void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

/* 向一个双链表中加入一个节点，参数：
new：要加入的节点
prev：要插入位置的前一个节点
next：要插入位置的后一个节点 */
static __inline__ void __list_add(struct list_head *new,
                                  struct list_head *prev,
                                  struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/* 向一个双链表中加入一个节点（尾插），参数：
new：要加入的链表节点
head：要加入的链表节点的位置 */
static __inline__ void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

#endif /* _LINUX_LIST_H */