#ifndef _ASM_I386_SYSTEM_H
#define _ASM_I386_SYSTEM_H

/* 关中断 */
#define __cli() __asm__ __volatile__("cli" : : : "memory")

/* 开中断 */
#define __sti() __asm__ __volatile__("sti" : : : "memory")

#endif /* _ASM_I386_SYSTEM_H */