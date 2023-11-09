#ifndef _ASM_I386_TYPES_H
#define _ASM_I386_TYPES_H

/* 这个文件就是与架构高度相关的数据定义 */
/*
通常带有下划线的类型是内核私有的或者是为了某些特殊目的而保留的。
这意味着它们主要用于内核的内部实现，并不意味着被外部模块或用户空间程序直接使用。
内核开发者可能会选择使用这种类型命名，来强调这是一个内部类型，或者是为了避免与用户空间的类型冲突。

没有下划线的类型是更通用的类型定义，它们可能被内核本身使用，也可能在内核与用户空间的接口中使用。
在很多情况下，没有下划线的类型可能会被定义为等同于有下划线的类型。
*/

typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;
typedef unsigned long long __u64;

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

#endif /* _ASM_I386_TYPES_H */