#ifndef _LINUX_INIT_H
#define _LINUX_INIT_H
#define __init __attribute__((__section__(".text.init"))) // 这个宏 __init 使用 GCC 的属性（attribute）机制
// 来指定函数或变量应该被放置在哪个段（section）中。在这个例子中，使用 __init 标记的函数或变量会被放置在 .text.init 段中。
// 在 Linux 内核中，.text.init 段通常用于存放仅在初始化阶段需要的代码。一旦初始化完成，这部分内存可能会被释放，以节省系统资源。

#endif
