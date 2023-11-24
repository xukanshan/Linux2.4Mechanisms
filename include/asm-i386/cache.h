#ifndef _ASM_I386_CACHE_H
#define _ASM_I386_CAHCE_H
/* 定义与i386cpu缓存对齐相关的宏和属性 */

/* 这个宏定义了x86 cpu L1缓存线的大小的移位量，在Linux2.4中并非定义在这个头文件中，
而是定义在arch/i386/defconfig中 */
#define CONFIG_X86_L1_CACHE_SHIFT 5

/* 定义了L1缓存线的大小的移位量。在不同的处理器架构中，L1缓存线的大小可能不同。
此宏通过读取配置选项来设置这个值 */
#define L1_CACHE_SHIFT (CONFIG_X86_L1_CACHE_SHIFT)

/* 计算并定义了L1缓存线的实际字节数 */
#define L1_CACHE_BYTES (1 << L1_CACHE_SHIFT)

#endif /* _ASM_I386_CAHCE_H */