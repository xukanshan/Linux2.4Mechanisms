#include <linux/init.h>
#include <asm-i386/page.h>
#include <linux/bootmem.h>
#include <asm-i386/pagetable.h>
#include <asm-i386/pagetable-2level.h>
#include <asm-i386/fixmap.h>

/* 初始化一段固定的虚拟地址范围的页表, 也就是只为这段虚拟地址分配了页表，但是没有映射到物理页。参数：
开始和结束的虚拟地址（start 和 end），以及页全局目录（pgd）的基地址 */
static void __init fixrange_init(unsigned long start, unsigned long end, pgd_t *pgd_base)
{
    pgd_t *pgd;              /* 指向页全局目录表表项的指针 */
    pmd_t *pmd;              /* 指向页中间目录表表项的指针 */
    pte_t *pte;              /* 指向页表表项的指针 */
    int i, j;                /* 两个循环变量 */
    unsigned long vaddr;     /* 计算要添加固定映射的虚拟地址 */
    vaddr = start;           /* 传入的起始虚拟地址赋值给 vaddr */
    i = __pgd_offset(vaddr); /* 得到对应的页全局目录表表项索引 */
    j = __pmd_offset(vaddr); /* 得到对应的页中间目录表表项索引 */
    pgd = pgd_base + i;      /* 根据偏移量和页全局目录基地址，找到当前虚拟地址对应的页全局目录项 */

    /* 遍历页全局目录项，直到达到指定的结束地址或遍历完所有全局目录项 */
    for (; (i < PTRS_PER_PGD) && (vaddr != end); pgd++, i++)
    {
        pmd = (pmd_t *)pgd; /* 得到虚拟地址页中间目录表表项地址，这句位于#if CONFIG_X86_PAE的#else中 */

        /* 遍历页中间目录表表项，直到达到指定的结束地址或遍历完所有页中间目录项 */
        for (; (j < PTRS_PER_PMD) && (vaddr != end); pmd++, j++)
        {
            if (pmd_none(*pmd)) /* 如果页中间目录项未初始化，则进行初始化 */
            {
                pte = (pte_t *)alloc_bootmem_low_pages(PAGE_SIZE); /* 分配一个页表 */
                set_pmd(pmd, __pmd(_KERNPG_TABLE + __pa(pte)));    /* 设置页中间目录项 */
                if (pte != pte_offset(pmd, 0))                     /* 检查页表项的正确性 */
                    BUG();
            }
            vaddr += PMD_SIZE; /* 更新虚拟地址，移动到下一个页中间目录项所覆盖的地址范围 */
        }
        j = 0; /* 重置页中间目录项的偏移量 */
    }
}

/* 扩充由startup_32在第一阶段创建页目录表和页表（当时不知道内存有多大，所只为开始的8MB建立了映射，
现在知道了，自然就可以扩充），内核地址空间将映射到线性映射的结束位置。
并且并且为一段用于固定映射虚拟地址初始化了页表，但没有映射到物理页 */
static void __init pagetable_init(void)
{
    unsigned long vaddr, end;                           /* vaddr记录要添加映射的起始地址，end是要扩展映射的边界 */
    pgd_t *pgd, *pgd_base;                              /* 指向页全局目录表表项的指针，与指向页全局目标表基址的指针 */
    int i, j, k;                                        /* 循环变量 */
    pmd_t *pmd;                                         /* 指向页中间目录表表项的指针 */
    pte_t *pte;                                         /* 指向页表表项的指针 */
    end = (unsigned long)__va(max_low_pfn * PAGE_SIZE); /* 计算要扩展映射的边界 */
    pgd_base = swapper_pg_dir;                          /* 得到页目录表基址，swapper_pg_dir定义在head.S中 */
    /* 我们要将整个直接线性映射区映射到内核地址空间，自然要得到3G对应的页全局目标表表项索引 */
    i = __pgd_offset(PAGE_OFFSET);
    pgd = pgd_base + i; /* 得到3G对应的页全局目录表表项的地址 */

    for (; i < PTRS_PER_PGD; pgd++, i++) /* 遍历整个内核地址空间的页全局目录表表项 */
    {
        vaddr = i * PGDIR_SIZE;    /* 计算当前全局目录项所代表的虚拟地址 */
        if (end && (vaddr >= end)) /* end为0，或者如果已处理所有需要的地址，则退出循环 */
            break;
        /* 将页全局目录项转换为页中间目录项，这句属于#if CONFIG_X86_PAE 的 #else
        如果开启了PAE，那么就是3级页表，能够映射64G的内存，如果没有开启，那么就是2级页表，所以页中间目录表就是页全局目录表*/
        pmd = (pmd_t *)pgd;

        if (pmd != pmd_offset(pgd, 0)) /* 两级页表体系，pmd_offset返回的页中间目录表表项与页全局目录表表项相同*/
            BUG();
        for (j = 0; j < PTRS_PER_PMD; pmd++, j++) /* 遍历页中间目录表表项 */
        {
            vaddr = i * PGDIR_SIZE + j * PMD_SIZE; /* 计算当前页中间目录表表项所代表的虚拟地址 */
            if (end && (vaddr >= end))             /* end为0，或者如果已处理所有需要的地址，则退出循环 */
                break;

            pte = (pte_t *)alloc_bootmem_low_pages(PAGE_SIZE); /* 为页表分配内存 */
            set_pmd(pmd, __pmd(_KERNPG_TABLE + __pa(pte)));    /* 设置页中间目录项 */

            if (pte != pte_offset(pmd, 0)) /* 确保页表地址已经被设定好了 */
                BUG();

            for (k = 0; k < PTRS_PER_PTE; pte++, k++) /* 遍历页表项 */
            {
                vaddr = i * PGDIR_SIZE + j * PMD_SIZE + k * PAGE_SIZE; /* 计算当前页表表项所代表的虚拟地址 */
                if (end && (vaddr >= end))                             /* end为0，或者如果已处理所有需要的地址，则退出循环 */
                    break;
                *pte = mk_pte_phys(__pa(vaddr), PAGE_KERNEL); /* 设置页表表项 */
            }
        }
    }
    /* 向下4MB取整计算要添加固定映射的起始虚拟地址，因为初始化固定映射是为这部分创立页表，但是不映射到具体的页，
    自然就要向下4MB取整，因为一张页表管理4MB地址空间 */
    vaddr = __fix_to_virt(__end_of_fixed_addresses - 1) & PMD_MASK;

    /* 调用函数来为用于固定映射的虚拟地址分配页表，但是没有映射到具体的物理页*/
    fixrange_init(vaddr, 0, pgd_base);
}

/*进一步扩充页表映射 - 注意前 8MB 已经由 head.S 映射。
还取消映射了虚拟内核地址 0 处的页面，这样我们就可以捕获内核中那些讨厌的 NULL 引用错误。 */
void __init paging_init(void)
{
    /* 扩充由startup_32在第一阶段创建页目录表和页表到线性映射的结束位置，并且初始化用于固定映射的虚拟地址的页表 */
    pagetable_init();

    /* 更新cr3，以刷新TLB中的页目录，这样做会使TLB中的所有内容失效，从而导致下一次地址转换时必须从页目录和页表中重新获取映射。
    这确保了TLB与当前的%cr3指向的页目录保持同步 */
    __asm__("movl %%ecx,%%cr3\n" ::"c"(__pa(swapper_pg_dir)));

    // __flush_tlb_all();

    // #ifdef CONFIG_HIGHMEM
    //     kmap_init();
    // #endif
    //     {
    //         unsigned long zones_size[MAX_NR_ZONES] = {0, 0, 0};
    //         unsigned int max_dma, high, low;

    //         max_dma = virt_to_phys((char *)MAX_DMA_ADDRESS) >> PAGE_SHIFT;
    //         low = max_low_pfn;
    //         high = highend_pfn;

    //         if (low < max_dma)
    //             zones_size[ZONE_DMA] = low;
    //         else
    //         {
    //             zones_size[ZONE_DMA] = max_dma;
    //             zones_size[ZONE_NORMAL] = low - max_dma;
    // #ifdef CONFIG_HIGHMEM
    //             zones_size[ZONE_HIGHMEM] = high - low;
    // #endif
    //         }
    //         free_area_init(zones_size);
    //     }
    return;
}