#ifndef _ASM_I386_IO_H
#define _ASM_I386_IO_H
/* 这个头文件中，我将所有的extern inline 替换成了static inline，原因见makefile文件的#-fgnu89-inline参数解释 */



/*
 * 用于将地址 x 转换为用于I/O操作的虚拟地址。
 * 当我们进行I/O操作时，我们需要确保我们使用的是正确映射的虚拟地址，
 * 以便硬件能够在页机制下正确解析和定位到实际的物理位置。
 * 在某些体系结构或配置中，这可能涉及到复杂的地址转换过程。
 * 根据这条定义处于Linux 2.4 #if 1预处理器指令下猜测，
 * 我们预期传入的x已经是一个适当的虚拟地址。
 * 可能存在情况：在其他体系结构或配置中，如s390，__io_virt 可能涵盖更多的地址转换逻辑。
 */
#define __io_virt(x) ((void *)(x))

/* 用于向一个地址写入一字节数据 */
#define writeb(b, addr) (*(volatile unsigned char *)__io_virt(addr) = (b))

/* 用于从一个地址读入一字节数据 */
#define readb(addr) (*(volatile unsigned char *)__io_virt(addr))

/* linux 2.4使用__OUT1与__OUT2共同拼凑出outb函数，是为了方便同时定义outb和outb_p（p来源于pause，带有延时功能的输出字节），
以及其他I/O操作函数的相应带有延时的版本。outb函数相比于out函数，额外多了向端口 0x80 发送一个值，
端口 0x80 通常用于访问不在用的设备，因此它可以在不产生副作用的情况下用于延迟
这个额外的延迟相对很小，它大约等同于一次端口输出操作的时间。
在历史上，尤其是在较慢的计算机硬件上，像端口 0x80 这样的端口被用作延迟端口的原因是因为 I/O 操作通常比 CPU 指令要慢。
所以，在进行 I/O 操作，尤其是向某些需要一定时间来响应的硬件端口写入时，这样的额外操作可以防止 CPU 过快地执行下一条指令。
这里我不支持outb_p函数 */

/* 这个宏就是为了处理outb与outb_p名字的差异 */
#define __OUT1(s, x)                                                 \
    static inline void out##s(unsigned x value, unsigned short port) \
    {

/* 这个宏就是outb与outb_p共同的部分 */
#define __OUT2(s, s1, s2) \
__asm__ __volatile__ ("out" #s " %" s1 "0,%" s2 "1"

/* outb利用__OUT1与__OUT2再加上一点输出输入指定即可生成；outb_p则还需要插入一个向0x80输出字节的宏在__OUT2后面 */
#define __OUT(s, s1, x)                                     \
__OUT1(s,x) __OUT2(s,s1,"w") : : "a" (value), "Nd" (port)); \
    }

/* 以下三个会被宏展开成outb, outw, outl函数 */
__OUT(b, "b", char)
__OUT(w, "w", short)
__OUT(l, , int)

/* 这个宏就是为了处理inb与inb_p名字的差异 */
#define __IN1(s)                                         \
    static inline RETURN_TYPE in##s(unsigned short port) \
    {                                                    \
        RETURN_TYPE _v;

/* 这个宏就是inb与inb_p共同的部分 */
#define __IN2(s, s1, s2) \
__asm__ __volatile__ ("in" #s " %" s2 "1,%" s1 "0"

/* inb利用__IN1与__IN2再加上一点输出输入指定即可生成；inb_p则还需要插入一个向0x80输出字节的宏在__IN2后面 */
#define __IN(s, s1, i...)                                  \
__IN1(s) __IN2(s,s1,"w") : "=a" (_v) : "Nd" (port) ,##i ); \
    return _v;                                             \
    }

/* 以下三个会被宏展开成inb, inw, inl函数，返回值的类型使用了RETURN_TYPE宏来进行规定 */
#define RETURN_TYPE unsigned char
__IN(b, "")
#undef RETURN_TYPE
#define RETURN_TYPE unsigned short
__IN(w, "")
#undef RETURN_TYPE
#define RETURN_TYPE unsigned int
__IN(l, "")
#undef RETURN_TYPE

/* ins的内联汇编实现 */
#define __INS(s)                                                                            \
    static inline void ins##s(unsigned short port, void *addr, unsigned long count)         \
    {                                                                                       \
        __asm__ __volatile__("rep ; ins" #s                                                 \
                             : "=D"(addr), "=c"(count) : "d"(port), "0"(addr), "1"(count)); \
    }

/* 以下三个会被宏展开成insb, insw, insl函数 */
__INS(b)
__INS(w)
__INS(l)

/* outs的内联汇编实现 */
#define __OUTS(s)                                                                           \
    static inline void outs##s(unsigned short port, const void *addr, unsigned long count)  \
    {                                                                                       \
        __asm__ __volatile__("rep ; outs" #s                                                \
                             : "=S"(addr), "=c"(count) : "d"(port), "0"(addr), "1"(count)); \
    }

/* 以下三个会被宏展开成outsb, outsw, outsl函数 */
__OUTS(b)
__OUTS(w)
__OUTS(l)

/* 将虚拟地址转换成物理地址 */
static inline unsigned long virt_to_phys(volatile void *address)
{
    return __pa(address);
}

/* 将虚拟地址转换成总线地址，实际就是物理地址 */
#define virt_to_bus virt_to_phys

/* 将总线地址转换成页机制下的虚拟地址，实际就是将物理地址转换成虚拟地址 */
#define bus_to_virt phys_to_virt

/* 将物理地址转换成虚拟地址 */
static inline void *phys_to_virt(unsigned long address)
{
    return __va(address);
}

/* 定义了端口号的最大值 */
#define IO_SPACE_LIMIT 0xffff

#endif /* _ASM_I386_IO_H */