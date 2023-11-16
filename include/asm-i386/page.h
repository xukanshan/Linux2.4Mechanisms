#ifndef _ASM_I386_PAGE_H
#define _ASM_I386_PAGE_H

/* 最低一级页表所占地址位数 */
#define PAGE_SHIFT 12

/* 定义页面大小，方法是1 << 最低一级页表所占地址（12）位数 */
#define PAGE_SIZE (1UL << PAGE_SHIFT)

/* 定义内核的起始虚拟地址 */
#define __PAGE_OFFSET (0xC0000000)

#define PAGE_OFFSET ((unsigned long)__PAGE_OFFSET)

/* 将虚拟地址转换成物理地址 */
#define __pa(x) ((unsigned long)(x)-PAGE_OFFSET)

/* 将物理地址转换成虚拟地址 */
#define __va(x) ((void *)((unsigned long)(x) + PAGE_OFFSET))

#endif /* _ASM_I386_PAGE_H */