#ifndef _LINUX_SLAB_H
#define _LINUX_SLAB_H

#include <linux/mm.h>

/* mm/slab.c */
extern void kmem_cache_init(void);

typedef struct kmem_cache_s kmem_cache_t;

/* kmem_cache_t结构体flags的值 */

/* kmem_cache_t 的 flags 值，用于指示 slab 管理数据结构是存放在它们所管理的缓存块中，
还是放在专门的内存缓存区（一个单独的 slab）中 */
#define CFLGS_OFF_SLAB 0x010000UL

/* kmem_cache_t 的 flags 值，缓存池不会在内存紧张时被回收（reap） */
#define SLAB_NO_REAP 0x00001000UL

#endif /* _LINUX_SLAB_H */