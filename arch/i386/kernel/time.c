#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <asm-i386/io.h>
#include <asm-i386/irq.h>
#include <asm-i386/delay.h>
#include <asm-i386/processor.h>
#include <linux/timex.h>
#include <asm-i386/fixmap.h>
#include <linux/irq.h>

/* 时钟中断的处理函数，暂未实现 */
static void timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
}

/* 时钟中断对应的中断动作 */
static struct irqaction irq0 = {timer_interrupt, SA_INTERRUPT, 0, "timer", NULL, NULL};

/* 注册时钟中断的中断动作 */
void __init time_init(void)
{
    setup_irq(0, &irq0); /* 注册时钟中断的中断动作 */
}