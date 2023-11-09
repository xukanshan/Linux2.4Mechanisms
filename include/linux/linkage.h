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

/* 这个CPP_ASMLINKAGE在Linux2.4中是包含在 #ifdef __cplusplus 之下，
当使用C++编译器编译代码时，该宏会被定义。
在C++中，extern "C"用来告诉C++编译器按照C语言的链接约定来处理被声明的函数，
即禁用C++的函数重载，保证在链接时能够与C语言编译生成的对象文件兼容。
对于非C++环境（即普通的C环境），CPP_ASMLINKAGE为空，不提供任何修饰。
因此，在C环境下，asmlinkage不会影响函数的声明 */
#define CPP_ASMLINKAGE
/* 用于告诉编译器该函数的参数不会通过寄存器传递，
而是完全通过堆栈来传递。这通常用于系统调用函数，
因为用户空间的程序不能直接设置寄存器来调用内核函数，
它们通过软件中断向内核传递参数，
这些参数由内核在栈上找到。 */
#define asmlinkage CPP_ASMLINKAGE

#endif /* _LINUX_LINKAGE_H */