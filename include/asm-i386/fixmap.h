#ifndef _ASM_I386_FIX_MAP_H
#define _ASM_I386_FIX_MAP_H
/* 用于处理固定映射（fixed mappings）的头文件，
固定映射机制：就是让不同的物理页不断映射到相同的虚拟页中，这样可以实现相同虚拟地址访问不同的页 */



/* 固定映射区域的顶部（起始）地址，也就是4G空间最后8KB的起始位置 */
#define FIXADDR_TOP (0xffffe000UL)

/* 将固定映射区域中的索引转换为对应的虚拟地址，它基于固定映射区域从 FIXADDR_TOP 向下增长（ */
#define __fix_to_virt(x) (FIXADDR_TOP - ((x) << PAGE_SHIFT))

/* 定义了一系列固定映射（fixed mappings）的地址标识符, 在Linux2.4中会
根据内核的配置选项（如 CONFIG_X86_LOCAL_APIC、CONFIG_X86_IO_APIC、CONFIG_X86_VISWS_APIC、CONFIG_HIGHMEM 等），
不同的映射地址会被包含或排除在编译过程中，也就是说最后会依据配置专门留了块固定映射区域给某些功能。这里我都不支持*/
enum fixed_addresses
{
    __end_of_fixed_addresses /* 用这个值来表示有多少个要添加的固定映射的标识符 */
};

#endif /* _ASM_I386_FIX_MAP_H */