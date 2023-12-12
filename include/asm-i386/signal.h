#ifndef _ASM_I386_SIGNAL_H
#define _ASM_I386_SIGNAL_H

/* 表示自动重新启动被信号打断的系统调用。默认情况下，
当一个系统调用（如读取或写入操作）被信号中断时，它会提前结束并返回一个错误（通常是 EINTR 错误）。
如果一个信号处理函数irqaction内的flags 有 SA_RESTART 标志，
那么被该信号中断的系统调用将会自动重新启动，而不是返回错误 */
#define SA_RESTART 0x10000000

/* 用于irqaction内的flags表示中断处理动作与其他中断处理动作共享一个中断引脚 */
#define SA_SHIRQ 0x04000000

/* Signal Action sampling randomness, 用于表示一个中断是否具有随机性，
比如键盘的按下，鼠标的点击对应的中断就是真随机的。用于irqaction内的flags。
这里与SA_RESTART定义成相同，可能是因为二者用在不同场景中，不会冲突 */
#define SA_SAMPLE_RANDOM SA_RESTART

/* SA_INTERRUPT 标志最初是用来标记一个中断处理程序为快速中断处理程序。
如果一个中断处理程序被标记为快速中断处理程序，它将在执行期间屏蔽所有其他中断，以减少中断处理的延迟。
而普通中断处理程序则允许在执行过程中被其他中断打断。
然而，随着内核的演进，这种区分变得不再必要，因为内核对中断处理机制进行了优化和重构（中断下半部分处理机制）。
所以现在SA_INTERRUPT 标志基本上成了一个无操作的标志（no-op）。这意味着将此标志设置于中断处理程序不再改变处理程序的行为。
它的存在更多的是为了代码的历史连续性和可读性。 */
#define SA_INTERRUPT 0x20000000

#endif /* _ASM_I386_SIGNAL_H */