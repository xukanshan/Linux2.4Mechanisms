#ifndef _ASM_I386_PAGETABLE_H
#define _ASM_I386_PAGETABLE_H

#define TWOLEVEL_PGDIR_SHIFT 22
#define BOOT_USER_PGD_PTRS (__PAGE_OFFSET >> TWOLEVEL_PGDIR_SHIFT)
#define BOOT_KERNEL_PGD_PTRS (1024 - BOOT_USER_PGD_PTRS)

/* 不让汇编识别以下 */
#ifndef __ASSEMBLY__

#include <asm-i386/page.h>
#include <asm-i386/pagetable-2level.h>

/* 定义在head.S中 */
extern pgd_t swapper_pg_dir[1024];

/* arch/i386/mm/init.c */
extern void paging_init(void);

/* 得到一个虚拟地址对应的页全局目录表表项索引 */
#define pgd_index(address) ((address >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))
#define __pgd_offset(address) pgd_index(address)

/* 给定一个虚拟地址，然后得到该地址对应的页表目录索引 */
#define __pte_offset(address) ((address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))

/* 给定页中间目录表表项的内容，然后获取页中间目录表表项中存储的页表地址（会转换为虚拟地址） */
#define pmd_page(pmd) ((unsigned long)__va(pmd_val(pmd) & PAGE_MASK))

/* 给定一个虚拟地址，然后得到该地址对应的页中间目录表表项索引，最后结果一定是0 */
#define __pmd_offset(address) (((address) >> PMD_SHIFT) & (PTRS_PER_PMD - 1))

/* 判断一个页中间目录表表项是否设定 */
#define pmd_none(x) (!pmd_val(x))

/* 给定一个页中间目录表表项项地址和一个虚拟地址，得到该虚拟地址对应的页表项的虚拟地址 */
#define pte_offset(dir, address) ((pte_t *)pmd_page(*(dir)) + __pte_offset(address))

/* 定义1个页全局目录表表项管理的地址空间大小，4MB */
#define PGDIR_SIZE (1UL << PGDIR_SHIFT)

/* 定义1个页中间目录表表项管理的地址空间大小，2级页表体系中
页全局目录表与页中间目录表概念相同，所以也是4MB */
#define PMD_SIZE (1UL << PMD_SHIFT)

/* 将一个虚拟地址转换成对应的页中间目录表管理的起始虚拟地址
哪个数字与这个数，将会留下高10位 */
#define PMD_MASK (~(PMD_SIZE - 1))

/* 如果这个位被设置，那么页表项指向一个实际的物理内存页；如果未设置，表示该页不在物理内存中 */
#define _PAGE_PRESENT 0x001

/* 如果设置了这个位，页面可被读写；如果未设置，页面只能被读 */
#define _PAGE_RW 0x002

/* 如果设置了这个位，用户模式的进程可以访问这个页；如果未设置，只有内核模式下的代码可以访问 */
#define _PAGE_USER 0x004

/* 设置页表的写通（Page Write-Through）特性。
被设置为 1 时，启用“写通”策略。在这种模式下，对页面的写操作同时更新到缓存和物理内存。这样做可以减少数据的不一致性，
但可能会降低写操作的性能，因为每次写操作都需要访问物理内存。
在默认情况下（即当 _PAGE_PWT 未设置时），页面使用“写回”（Write-Back）策略，这意味着数据被写入缓存，
而不是直接写入到物理内存。当缓存行被淘汰时，如果数据有变动，则写回到内存。
写通策略特别适用于需要高数据一致性的情况，例如，当多个处理器或设备可能同时访问同一内存区域时。 */
#define _PAGE_PWT 0x008

/* 设置页表的缓存禁用（Page Cache Disable）特性。
当 _PAGE_PCD 设置为 1 时，禁用该页面的缓存。这意味着所有对该页面的读写操作都会直接访问物理内存，而不是首先查找 CPU 的缓存。
在默认情况下（即当 _PAGE_PCD 未设置时），页面可以被 CPU 缓存。 */
#define _PAGE_PCD 0x010

/* 表示该页被访问过（Accessed）。这个位用来帮助操作系统确定哪些页是活跃的 */
#define _PAGE_ACCESSED 0x020

/*  表示该页被写过（Dirty）。这个位用于页面写回操作，帮助确定哪些页面需要被写回到磁盘 */
#define _PAGE_DIRTY 0x040

/* 内核使用的页全局目录表表项属性位设定，存在，可读可写，已经被访问过，已经写过 */
#define _KERNPG_TABLE (_PAGE_PRESENT | _PAGE_RW | _PAGE_ACCESSED | _PAGE_DIRTY)

/* 内核使用的页表表项属性位设定，存在，可读可写，已经被访问过，已经写过 */
#define __PAGE_KERNEL (_PAGE_PRESENT | _PAGE_RW | _PAGE_DIRTY | _PAGE_ACCESSED)

/* 传入页表表项要填入的物理地址，与页表属性位。然后将其做成一个页表表现的内容 */
#define mk_pte_phys(physpage, pgprot) __mk_pte((physpage) >> PAGE_SHIFT, pgprot)

/* 原来MAKE_GLOBAL： if (cpu_has_pge) __ret = __pgprot((x) | _PAGE_GLOBAL); else __ret = __pgprot(x);
由于不支持cpu_has_pge判断，所以直接改成了__ret = __pgprot(x);这么一句
这个宏是依据cpu_has_pge来设定页全局使能（Page Global Enable, PGE）"
通常，每当处理器的任务或上下文切换时，CPU的内存管理单元（MMU）需要刷新（重载）页表缓存，
这称为 TLB（Translation Lookaside Buffer）刷新。PGE 特性允许某些页表项被标记为“全局”的，
这意味着这些页表项在任务切换时不会从 TLB 中被清除。
如果支持pge，对系统内核所在虚拟地址空间的页表表项设置 PGE（页全局使能）是非常有益的*/
#define MAKE_GLOBAL(x)       \
    ({                       \
        pgprot_t __ret;      \
        __ret = __pgprot(x); \
        __ret;               \
    })

/* 设定内核使用的页表表项属性位，通用设定 + 是否支持全局是能 */
#define PAGE_KERNEL MAKE_GLOBAL(__PAGE_KERNEL)

#endif /* __ASSEMBLY__ */
#endif /* _ASM_I386_PAGETABLE_H */