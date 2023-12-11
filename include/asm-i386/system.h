#ifndef _ASM_I386_SYSTEM_H
#define _ASM_I386_SYSTEM_H

/* 关中断 */
#define __cli() __asm__ __volatile__("cli" : : : "memory")

/* 开中断 */
#define __sti() __asm__ __volatile__("sti" : : : "memory")

/* 用于读取cr0寄存器的值 */
#define read_cr0() ({       \
    unsigned int __dummy;   \
    __asm__(                \
        "movl %%cr0,%0\n\t" \
        : "=r"(__dummy));   \
    __dummy;                \
})

/* 用于向cr0寄存器写入特定的值 */
#define write_cr0(x) \
    __asm__("movl %0,%%cr0" : : "r"(x));

/* 用于设置 CR0 寄存器中的任务切换标志（TS标志），
TS标志主要用于控制浮点单元（FPU）的延迟保存和恢复。
当 TS 标志被设置时，如果执行浮点指令，处理器会触发一个设备不可用异常（#NM），
这通常导致操作系统保存上一个任务的 FPU 状态（因为当前FPU状态是上一个任务留下来的，同时当前任务要使用FPU，意味着要改变FPU的状态），
并为当前任务加载 FPU 状态。这种机制被称为“延迟保存和恢复”，因为只有在实际需要时，FPU 状态才会被保存和恢复 */
#define stts() write_cr0(8 | read_cr0())

/* 保存当前的中断状态到变量 x 中，并禁用本地中断 */
#define local_irq_save(x) __asm__ __volatile__("pushfl ; popl %0 ; cli" : "=g"(x) : : "memory")

/* 恢复保存在x中的中断状态 */
#define __restore_flags(x) __asm__ __volatile__("pushl %0 ; popfl" : : "g"(x) : "memory")

/* 恢复保存在x中的中断状态 */
#define local_irq_restore(x) __restore_flags(x)

/* 位于#ifndef CONFIG_X86_XMM 之下，XMM" 这个名字来源于 "Streaming SIMD Extensions"（流式单指令多数据扩展），
通常缩写为 SSE。SSE 指令集引入了一组新的寄存器，称为 XMM 寄存器。旨在提高处理浮点运算、数字信号处理和多媒体任务的效率。
这对于需要进行大量浮点运算的应用程序，如图形处理、科学计算和机器学习等，尤其重要。
addl $0,0(%%esp) 实际上是一个假操作（加0），不改变任何寄存器或内存的值，但它会强制CPU完成所有之前的内存操作。
因为lock 前缀是x86架构中一个特殊的指令前缀，用于确保紧随其后的指令的原子性。
当CPU执行带有 lock 前缀的指令时，它会确保所有先前的读写操作已经完成，并且在后面的指令完成前，不会开始任何后续的内存操作。
这样，即使 addl 操作本身不修改任何数据，lock 前缀仍然会导致处理器在执行这条指令时完成所有挂起的内存操作*/
#define mb() __asm__ __volatile__("lock; addl $0,0(%%esp)" : : : "memory")

#endif /* _ASM_I386_SYSTEM_H */