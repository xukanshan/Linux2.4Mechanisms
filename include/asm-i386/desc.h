#ifndef _ASM_I386_DESC_H
#define _ASM_I386_DESC_H

/* 第一个任务状态段（TSS）条目在全局描述符表（GDT）中的索引是12。具体布局可参照head.S代码最后定义的GDT表。 */
#define __FIRST_TSS_ENTRY 12

/* 利用给定的TSS编号n来计算在GDT中的索引位置
(n) << 2：这部分是将n向左位移两位，等同于将n乘以4。
+ __FIRST_TSS_ENTRY：这部分是将之前位移的结果加上一个基础值__FIRST_TSS_ENTRY。这个基础值代表GDT中第一个TSS条目的索 */
#define __TSS(n) (((n) << 2) + __FIRST_TSS_ENTRY)

/* 编译.S文件时，加上-D__ASSEMBLY__参数，等同于在编译.S文件时在最开始添加了#define __ASSEMBLY__ 1这一行
这样.S文件就可以不识别这个定义，结构体定义编译gcc编译汇编时无法识别，所以要这么做 */
#ifndef __ASSEMBLY__
struct desc_struct
{
    unsigned long a, b;
};
#endif /* !__ASSEMBLY__ */

#endif /* _ASM_I386_DESC_H */
