#include <linux/sched.h>
#include <asm-i386/delay.h>

/* 不断loops - 1，然后判断loops是不是>=0，来决定是否继续-1，
loops 是函数的参数，表示延迟循环的次数。 */
static void __loop_delay(unsigned long loops)
{
    int d0;
    __asm__ __volatile__(
        "\tjmp 1f\n"
        ".align 16\n"
        "1:\tjmp 2f\n"
        ".align 16\n"
        "2:\tdecl %0\n\tjns 2b"
        : "=&a"(d0)
        : "0"(loops));
}

/* 用循环减值来实现延时，参数：
loops，要循环的次数 */
void __delay(unsigned long loops)
{
    /* 位于if(x86_udelay_tsc) __rdtsc_delay(loops) 的else下，
    由于int x86_udelay_tsc本就定义为0，且386不支持TSC，
    所以就不复现__rdtsc_delay，并改造了这里调用代码 */
    __loop_delay(loops);
}

/* 用于实现编译时已知的微秒级延迟，参数：
xloops：要延迟的微秒。
这里的内联汇编，eax中存放xloops（放大了2的31次方），
edx中存放current_cpu_data.loops_per_jiffy。然后mul之后，edx：eax是完整结果，但是将edx，
也就是高32位放入xloops中了。也就是将放大了2的31次方的结果又缩小回去了 */
inline void __const_udelay(unsigned long xloops)
{
    int d0;
    __asm__("mull %0"
            : "=d"(xloops), "=&a"(d0)
            : "1"(xloops), "0"(current_cpu_data.loops_per_jiffy));
    __delay(xloops * HZ); /* 延时了100个滴答 * 传入微秒数字 */
}

/* 实现编译时未知的微秒级延迟，然后调用 __const_udelay 来实际实现延迟。
参数：要有延迟的微秒。
0x000010c6 其值等于 2的31次方/1000000。我们要传入__const_udelay的是要延迟的微秒占一秒的比例，
但是一般我们要延迟的微秒是远远小于1s的，所以会导致直接n/1000000的结果为0。所以乘了个2的31次方来将所占比例放大。
之后乘以其他值之后（在__const_udelay中），再缩小回来（在__const_udelay中），就可以实现更高精度*/
void __udelay(unsigned long usecs)
{
    __const_udelay(usecs * 0x000010c6);
}