#ifndef _ASM_I386_IRQ_H
#define _ASM_I386_IRQ_H

/* 系统支持的中断数量为224。是256去掉CPU专用的32个向量（因为他们不可被操作系统分配）  */
#define NR_IRQS 224

#endif /* _ASM_I386_IRQ_H */