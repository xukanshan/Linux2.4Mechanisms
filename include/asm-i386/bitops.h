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

/* 函数用于测试并设置一个位于特定内存位置的位。参数解释与sbbl指令作用参见test_and_clear_bit注释
btsl（Bit Test and Set Long）指令会检测指定位的状态，然后将其设置为1。
如果检测的位原本就是1，那么 btsl 指令会设置进位标志（CF）为1；如果原本是0，则设置进位标志为0 */
static __inline__ int test_and_set_bit(int nr, volatile void *addr)
{
    int oldbit; /* 存储原始位的状态 */

    __asm__ __volatile__(LOCK_PREFIX
                         "btsl %2,%1\n\tsbbl %0,%0"
                         : "=r"(oldbit), "=m"(ADDR)
                         : "Ir"(nr) : "memory");
    return oldbit;
}

/* 测试位图中的特定位是否被设置。参数：
nr：要测试的位相对于addr的位偏移，
addr：要测试的位所在内存区域的起始地址，一般就是位图地址。
((const volatile unsigned int *)addr)[nr >> 5] 这个操作实际上是在计算 nr 位于位图哪个 unsigned int中，
1UL << (nr & 31)， 取 nr 的低 5 位，左移操作 << 将 1 移到 nr 所指定的位位置，这个操作实际上是在准备nr所在unsigned int测试位置为1的情况。
联合起来就是它首先确定 nr 位于哪个 unsigned int 中，然后检查该 unsigned int 中对应的位是否被设置。
要理解这种方法，举个例子，确定98在数轴中的位置：前一个数字（9）：表示 98 落在以 10 为单位的第 9 个格子中，
后一个数字（8）：表示在这个 10 单位大小的格子内，98 的具体位置偏移了 8*/
static __inline__ int constant_test_bit(int nr, const volatile void *addr)
{
    return ((1UL << (nr & 31)) & (((const volatile unsigned int *)addr)[nr >> 5])) != 0;
}

/* 测试位图中的特定位是否被设置。参数：
nr：要测试的位相对于addr的位偏移，
addr：要测试的位所在内存区域的起始地址，一般就是位图地址。
btl （Bit Test）用于内存中的特定位，如果测试的位是 1，则会设置 CPU 的标志寄存器中的进位标志；如果测试的位是 0，则清除进位标志 */
static __inline__ int variable_test_bit(int nr, volatile void *addr)
{
    int oldbit;

    __asm__ __volatile__(
        "btl %2,%1\n\tsbbl %0,%0"
        : "=r"(oldbit)
        : "m"(ADDR), "Ir"(nr));
    return oldbit;
}

/* 用于测试位图中某一位是否被设置为1，参数：
nr：要测试的位相对于addr的位偏移
addr：要测试的位所在内存区域的起始地址
__builtin_constant_p 是 GCC 提供的一个内建函数，用于在编译时检查给定的参数是否是一个常量表达式。
如果 nr 是一个编译时常量（编译过程中其值已经确定并且在整个程序执行期间不会改变的常量），
表达式返回 true，否则返回 false。编译时常量与非常量对应两个处理函数主要是出于性能优化的考虑。
constant_test_bit 是编译时常量对应处理函数。它通过简单的位运算（位移和位与操作）来测试位图中的位，这些操作在编译时就可以确定和优化，效率很高。
variable_test_bit 是编译时常量对应处理函数。虽然使用内联汇编，但是运行效率不如constant_test_bit高。
gcc -fno-builtin, 它会禁用对所有内建函数的优化，这些函数通常是标准库函数的特殊版本，例如 memcpy、strcpy 等, 
然而__builtin_xx 是特殊的内建函数，不是实现某个标准库函数的特定功能，所以-fno-builtin对其无效 */
#define test_bit(nr, addr) \
    (__builtin_constant_p(nr) ? constant_test_bit((nr), (addr)) : variable_test_bit((nr), (addr)))

#endif /* _ASM_I386_BITOPS_H */