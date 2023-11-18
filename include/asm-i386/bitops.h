#ifndef _ASM_I386_BITOPS_H
#define _ASM_I386_BITOPS_H

/* 在SMP中，这个就是一把锁，但是我们这是单核，所以直接定义成空 */
#define LOCK_PREFIX ""

/* 即使 ADDR 在 C 语言的上下文中表示一个值，但在汇编指令的上下文中，编译器会将其视为该值所在的内存地址。
核心就在于理解约束m用于引用输入输出的内存位置。好好理解下这个例子中result为什么是有效的
asm ("divb  %2\n"
     "movl  %eax,   %0"
     : "=m"(result)
     : "a"(dividend), "m"(divisor)); */
#define ADDR (*(volatile long *)addr)

/* 用于测试和清除位于给定地址的特定位，两个参数：
nr：一个整数，表示要测试和清除的位的在其内存区域中位的索引
addr：指向要改变和测试的位所在的内存区域的起始地址的指针
btrl 指令（Bit Test and Reset）用于测试并重置（清除）指定的位。它检查指定地址处的一个位（即一个特定的二进制位置），然后将该位设置为 0。
其第一个操作数是要操作的位的索引，第二个操作数是包含该位的地址
sbbl 指令（Subtract with Borrow Long）执行的是带借位的长整型减法。它将第二个操作数和进位标志（Carry Flag）的值相减，然后从第一个操作数中减去这个结果
sbbl %0,%0 实际上是一种巧妙的方法来将 oldbit 设置为 btrl 操作之前测试位的状态：如果该位之前是 1，btrl 会导致进位标志被设置，sbbl 随后会将 oldbit 设置为 1；
如果该位之前是 0，进位标志不会被设置，sbbl 将 oldbit 设置为 0。oldbit获取的是sbbl的结果 */
static __inline__ int test_and_clear_bit(int nr, volatile void *addr)
{
    int oldbit; /* 存储原始位的状态 */

    __asm__ __volatile__(LOCK_PREFIX
                         "btrl %2,%1\n\tsbbl %0,%0"
                         : "=r"(oldbit), "=m"(ADDR)
                         : "Ir"(nr) : "memory");
    return oldbit;
}

#endif /* _ASM_I386_BITOPS_H */