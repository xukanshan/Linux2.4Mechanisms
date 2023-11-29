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

#endif /* _ASM_I386_SYSTEM_H */