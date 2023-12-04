#ifndef _ASM_I386_DELAY_H
#define _ASM_I386_DELAY_H

/* 如果传递的延迟值过大（超过20000微秒），udelay 宏会展开为对 __bad_udelay 函数的调用。
因为 __bad_udelay 函数实际在内核中没有定义，这将导致编译错误。
这种方式强制开发者在编译阶段就注意到这个潜在的问题，并重新考虑他们的延迟策略。
对于需要较长延迟的情况，应考虑使用 mdelay（毫秒级延迟）或 ndelay（纳秒级延迟） */
extern void __bad_udelay(void);

/* arch/i386/lib/delay.c */
extern void __const_udelay(unsigned long usecs);

/* arch/i386/lib/delay.c */
extern void __udelay(unsigned long usecs);

/* arch/i386/lib/delay.c */
extern void __delay(unsigned long loops);

/* udelay 用于实现微秒级延迟，如果n 是否为编译时已知的常量，并且大于20000，
调用 __bad_udelay。这通常意味着延迟时间过长，可能会影响系统性能或稳定性。
如果 n 小于或等于20000，它使用 __const_udelay 来实现延迟。
如果n不是常量，即它的值在编译时未知，这部分代码使用 __udelay 函数来实现延迟
0x10c6ul的解释减__udelay注释 */
#define udelay(n) (__builtin_constant_p(n) ? ((n) > 20000 ? __bad_udelay() : __const_udelay((n) * 0x10c6ul)) : __udelay(n))

#endif /* _ASM_I386_DELAY_H */