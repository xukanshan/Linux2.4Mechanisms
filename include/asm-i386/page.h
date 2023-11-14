#ifndef _ASM_I386_PAGE_H
#define _ASM_I386_PAGE_H

/* 定义内核的起始虚拟地址 */
#define __PAGE_OFFSET (0xC0000000)

#define PAGE_OFFSET ((unsigned long)__PAGE_OFFSET)

/* 将虚拟地址转换成物理地址 */
#define __pa(x) ((unsigned long)(x)-PAGE_OFFSET)

#endif /* _ASM_I386_PAGE_H */