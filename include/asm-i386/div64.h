#ifndef _ASM_I386_DIV64
#define _ASM_I386_DIV64

/*进行64位数的除法操作。由于一些处理器原生不支持64位除法，这个宏通过组合32位操作来实现，分别处理高位和低位，然后重新组装结果
 __low: 这个变量用于存储64位整数（即n）的低32位部分。
__high: 这个变量存储64位整数（即n）的高32位部分。它在判断是否需要处理64位数的高位部分时非常重要。
__upper: 在进行除法运算时，__upper用作临时存储变量。如果n的高32位（__high）非零，__upper将用于存储__high除以base后的余数，
这个余数随后会与__low一起用于后续的除法运算。
__mod: 这个变量用于存储最终的除法余数，
asm("" : "=a"(__low), "=d"(__high) : "A"(n));它将n的低32位移动到__low中，高32位移动到__high中。
这里使用的"A"约束表示将64位数的低32放入eax，高32放入edx。这是32位x86架构处理64位值的常见方法。
__upper = __high;将高32位的值复制到 __upper
接下来的代码段检查n的高位部分（__high）是否非零。如果非零，它会将__high除以base，并将结果存回__high，余数存入__upper。
asm("divl %2" : "=a"(__low), "=d"(__mod) : "rm"(base), "0"(__low), "1"(__upper));
divl指令将edx:eax中的64位数除以一个32位操作数。"rm"(base) 指定除数 base，"rm"是一个约束条件，表示被使用的操作数可以放在一个寄存器或者内存中。
"0"(__low) 和 "1"(__upper) 指定被除数，它们合起来构成了一个64位的被除数（__upper 是高32位，__low 是低32位）。
除法操作的结果是：商存储在 eax（__low），余数存储在 edx（__mod）
asm("" : "=A"(n) : "a"(__low), "d"(__high));这行代码使用内联汇编更新变量 n 的值，将新的低32位（__low）和高32位（__high）合并成一个64位数。
宏最终返回__mod（在C宏中，最后一个表达式的结果可以被用作宏的返回值），这是除法的余数。
*/
#define do_div(n, base) ({                                                            \
    unsigned long __upper, __low, __high, __mod;                                      \
    asm("" : "=a"(__low), "=d"(__high) : "A"(n));                                     \
    __upper = __high;                                                                 \
    if (__high)                                                                       \
    {                                                                                 \
        __upper = __high % (base);                                                    \
        __high = __high / (base);                                                     \
    }                                                                                 \
    asm("divl %2" : "=a"(__low), "=d"(__mod) : "rm"(base), "0"(__low), "1"(__upper)); \
    asm("" : "=A"(n) : "a"(__low), "d"(__high));                                      \
    __mod;                                                                            \
})

#endif /* _ASM_I386_DIV64 */