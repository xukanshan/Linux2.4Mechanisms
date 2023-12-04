#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/timex.h>
#include <linux/malloc.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/proc_fs.h>
#include <asm-i386/io.h>
#include <asm-i386/system.h>
#include <asm-i386/bitops.h>
#include <asm-i386/delay.h>
#include <asm-i386/desc.h>
#include <asm-i386/irq.h>

/* 记录虚假中断发生的次数 */
volatile unsigned long irq_err_count;

/* 存储系统中所有中断请求的描述符，它为系统中每个可能的中断提供了必要的数据结构和管理方式 */
irq_desc_t irq_desc[NR_IRQS] __cacheline_aligned = {[0 ... NR_IRQS - 1] = {0, &no_irq_type, NULL, 0, SPIN_LOCK_UNLOCKED}};

/* 一个hw_interrupt_type结构体内的startup 函数，什么都没做 */
static unsigned int startup_none(unsigned int irq) { return 0; }

/* 一个hw_interrupt_type结构体内的指针指向的函数，什么都没做 */
static void disable_none(unsigned int irq) {}

/* 一个hw_interrupt_type结构体内的指针指向的函数，什么都没做 */
static void enable_none(unsigned int irq) {}

/* 一个hw_interrupt_type结构体内的ack 函数，什么都没做 */
static void ack_none(unsigned int irq) {}

/* 将hw_interrupt_type结构体内的shutdown 函数指针指向disable_none */
#define shutdown_none disable_none

/* 将hw_interrupt_type结构体内的end 函数指针指向enable_none */
#define end_none enable_none

/* 一个hw_interrupt_type已初始化好的实例，用于一些结构体内hw_interrupt_type指针初始化 */
struct hw_interrupt_type no_irq_type = {
    "none",
    startup_none,
    shutdown_none,
    enable_none,
    disable_none,
    ack_none,
    end_none};

/* 暂时空着 */
asmlinkage unsigned int do_IRQ(struct pt_regs regs)
{
    return 1;
}