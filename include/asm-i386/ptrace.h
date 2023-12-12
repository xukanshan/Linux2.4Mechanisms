#ifndef _ASM_I386_PTRACE_H
#define _ASM_I386_PTRACE_H

/* 描绘了中断发生时压入内核栈的数据 */
struct pt_regs
{
    long ebx;
    long ecx;
    long edx;
    long esi;
    long edi;
    long ebp;
    long eax;
    int xds;
    int xes;
    long orig_eax;
    long eip;
    int xcs;
    long eflags;
    long esp;
    int xss;
};

#endif /* _ASM_I386_PTRACE_H */