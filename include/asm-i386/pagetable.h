#ifndef _ASM_I386_PAGETABLE_H
#define _ASM_I386_PAGETABLE_H

#include <asm-i386/page.h>

#define TWOLEVEL_PGDIR_SHIFT 22
#define BOOT_USER_PGD_PTRS (__PAGE_OFFSET >> TWOLEVEL_PGDIR_SHIFT)
#define BOOT_KERNEL_PGD_PTRS (1024 - BOOT_USER_PGD_PTRS)

#endif /* _ASM_I386_PAGETABLE_H */