#ifndef _LINUX_TIMEX_H
#define _LINUX_TIMEX_H

#include <asm-i386/param.h>
#include <asm-i386/timex.h>

/* LATCH 的值被用于设置计数器0的 counter_value 寄存器，
它决定了在触发下一个时钟中断之前必须发生的内部时钟滴答的次数。
这里，内部时钟频率为 CLOCK_TICK_RATE（ 1193180Hz），
我们希望每秒触发 HZ（例如 100）次中断，因此需要每 CLOCK_TICK_RATE/HZ
​次内部时钟滴答触发一次中断。+ HZ / 2 的加入是为了确保计算 CLOCK_TICK_RATE / HZ 时，
结果是四舍五入而不是向下取整，从而使得中断触发的频率尽可能接近预定值 */
#define LATCH ((CLOCK_TICK_RATE + HZ / 2) / HZ)

#endif /* _LINUX_TIMEX_H */