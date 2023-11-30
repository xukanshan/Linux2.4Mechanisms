#ifndef _ASM_I386_MMU_CONTEXT_H
#define _ASM_I386_MMU_CONTEXT_H

/* 函数位于#ifdef CONFIG_SMP 下的 #else 下
函数用于处理转换后备缓冲器（TLB）的懒惰更新。TLB 是 CPU 中用于加速虚拟地址到物理地址转换的缓存。
当切换到新的内存管理结构时，TLB 的内容可能需要更新。这个函数会设置相应的标志，使得 TLB 在实际需要时才更新，从而提高效率 */
static inline void enter_lazy_tlb(struct mm_struct *mm, struct task_struct *tsk, unsigned cpu)
{
}

#endif /* _ASM_I386_MMU_CONTEX_H */