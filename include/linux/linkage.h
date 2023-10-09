#ifndef _LINUX_LINKAGE_H
#define _LINUX_LINKAGE_H

/* X与:之间的注释符, 只是为了保证中间没有多余空格，可以去掉 */
#define SYMBOL_NAME_LABEL(X) X /**/:

#define SYMBOL_NAME(X) X

/* 下一条执行四字节对齐, 中间填充0x90(nop指令，表示什么都不做) */
#define __ALIGN .align 4, 0x90
#define ALIGN __ALIGN

/* 这个宏用于定义一个标号，并且将其全局化，并且标号开始代码4字节对齐 */
#define ENTRY(name)           \
    .globl SYMBOL_NAME(name); \
    ALIGN;                    \
    SYMBOL_NAME_LABEL(name)

#endif /* _LINUX_LINKAGE_H */