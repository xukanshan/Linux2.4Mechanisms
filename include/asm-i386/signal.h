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

#endif /* _ASM_I386_SIGNAL_H */