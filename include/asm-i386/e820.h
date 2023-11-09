#ifndef _ASM_I386_E820_H
#define _ASM_I386_E820_H

#define E820MAX 32 /* e820结构体数组最大元素数 */

/* 这些定义与grub使用multiboot提供的内存区域类型是兼容的，直接用 */
/* 正常可用内存类型为1 */
#define E820_RAM 1

/* 不可用的保留内存类型为2 */
#define E820_RESERVED 2

/* ACPI表已读的话, 此区域可以用作内存。ACPI表(ACPI Tables)是Advanced Configuration and Power Interface标准中的一个组成部分。
ACPI表是BIOS固化在固件中的一组数据结构,用于描述平台的电源管理及配置能力。它们在系统启动过程中被读取并解析。 */
#define E820_ACPI 3

/* 指BIOS或固件使用的非易失存储区域,包含变量、配置等信息。系统不应访问此存储区域。 */
#define E820_NVS 4

/* 我们将高于1m的内存叫做highmem，起始肯定是1m啦 */
#define HIGH_MEMORY (1024 * 1024)

#ifndef __ASSEMBLY__

/* 该结构体管理整个e820结构体数组 */
struct e820map
{
    int nr_map; /* 记录e820map内有多少有效e820结构体 */
    struct e820entry
    {
        unsigned long long addr; /* 记录内存区域的起始地址 */
        unsigned long long size; /* 记录内存区域的大小 */
        unsigned long type;      /* 记录内存区域的类型 */
    } map[E820MAX];
};
extern struct e820map e820;
#endif /*!__ASSEMBLY__*/

#endif /* _ASM_I386_E820_H */