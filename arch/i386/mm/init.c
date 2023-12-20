#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/bootmem.h>
#include <asm-i386/processor.h>
#include <asm-i386/system.h>
#include <asm-i386/pgtable.h>
#include <asm-i386/dma.h>
#include <asm-i386/fixmap.h>
#include <asm-i386/e820.h>

/* 记录ram的总物理页面数 */
static unsigned long totalram_pages;

/* 记录总的高端内存页面数 */
static unsigned long totalhigh_pages;

/* arch/i386/kernel/head.S */
extern char _text, _etext, _edata, __bss_start, _end;

/* arch/i386/kernel/head.S */
extern char __init_begin, __init_end;

/* Linux2.4对于nr_free_pages没有头文件声明，我自己加的
mm/page_alloc.c */
extern unsigned int nr_free_pages(void);

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

/* 记录内存的结束帧号（需要这部分内存落在896M到4GB内存），
这个变量的赋值是在HIGH_MEM相关代码中处理的，由于我们不支持，所以这个变量在我们这里无用 */
unsigned long highend_pfn;

/*进一步扩充页表映射 - 注意前 8MB 已经由 head.S 映射。
还取消映射了虚拟内核地址 0 处的页面，这样我们就可以捕获内核中那些讨厌的 NULL 引用错误。
还完成管理内存节点的pglist结构体的初始化 */
void __init paging_init(void)
{
    /* 扩充由startup_32在第一阶段创建页目录表和页表到线性映射的结束位置，并且初始化用于固定映射的虚拟地址的页表 */
    pagetable_init();

    /* 更新cr3，以刷新TLB中的页目录，这样做会使TLB中的所有内容失效，从而导致下一次地址转换时必须从页目录和页表中重新获取映射。
    这确保了TLB与当前的%cr3指向的页目录保持同步 */
    __asm__("movl %%ecx,%%cr3\n" ::"c"(__pa(swapper_pg_dir)));

    __flush_tlb_all();

    {
        unsigned long zones_size[MAX_NR_ZONES] = {0, 0, 0}; /* 存储每个内存区域的大小 */
        unsigned int max_dma, high, low;                    /* max_dma 用于存储 DMA 区域的最大地址，high 和 low 用于存储系统中可用的最高和最低物理页帧号 */

        max_dma = virt_to_phys((char *)MAX_DMA_ADDRESS) >> PAGE_SHIFT; /* 获取 DMA 区域的最大页帧号 */
        low = max_low_pfn;                                             /* 设置 low 变量为内存管理系统可管理的最大物理页面号 */
        high = highend_pfn;                                            /* 设置 high 变量为系统可用的最最高物理页帧号 */

        if (low < max_dma) /* low 小于 max_dma，则整个可用内存都在 DMA 区域内，因此 zones_size[ZONE_DMA] 被设置为 low。 */
            zones_size[ZONE_DMA] = low;
        else /* 否则，DMA 区域被设置为 max_dma，普通区域 ZONE_NORMAL 被设置为 low - max_dma。 */
        {
            zones_size[ZONE_DMA] = max_dma;
            zones_size[ZONE_NORMAL] = low - max_dma;
        }
        free_area_init(zones_size); /* 完成管理内存节点的pglist结构体的初始化 */
    }
    return;
}

/* 通过检查 BIOS 提供的 e820 内存映射表，确定给定的页面号是否属于系统的可用RAM区域。参数：
pagenr：页面号码 */
static inline int page_is_ram(unsigned long pagenr)
{
    int i; /* 用于循环迭代 */
    /* 遍历e820结构体 */
    for (i = 0; i < e820.nr_map; i++)
    {
        unsigned long addr, end; /* 存储内存区域的起始和结束地址 */
        /* 检查当前e820条目的内存类型是不是 E820_RAM */
        if (e820.map[i].type != E820_RAM)
            /* 进入if，说明不是 */
            continue;
        /* 计算e820 ram区域的起始页面帧号 */
        addr = (e820.map[i].addr + PAGE_SIZE - 1) >> PAGE_SHIFT;
        /* 计算e820 ram区域的结束页面帧号 */
        end = (e820.map[i].addr + e820.map[i].size) >> PAGE_SHIFT;
        /* 判断页面号是否在ram区域内 */
        if ((pagenr >= addr) && (pagenr < end))
            /* 进入if，说明是 */
            return 1;
    }
    return 0;
}

/* 清空0页，释放引导期间分配的内存，以及释放用于引导内存分配的位图本身，
计算被保留的页面数，计算内核代码段，数据段，初始化段大小 */
void __init mem_init(void)
{
    int codesize, reservedpages, datasize, initsize;
    int tmp;

    if (!mem_map)
        BUG(); /* 如果管理整个系统的物理内存页数组不存在，就报错 */
    /* 此句位于#ifdef CONFIG_HIGHMEM 的 #else下，
    最大可映射页面数自然 = 物理页面数 = 最大低端内存（可以直接线性映射）页面数 */
    max_mapnr = num_physpages = max_low_pfn;
    /* 系统可直接寻址的边界自然是最大低端内存边界页转换过来的虚拟地址 */
    high_memory = (void *)__va(max_low_pfn * PAGE_SIZE);
    /* 清空0页，因为之前用来传递bios信息与命令行，其定义在head.S中 */
    memset(empty_zero_page, 0, PAGE_SIZE);
    /* 释放引导期间分配的内存，以及释放用于引导内存分配的位图本身，
    并将返回的页面数加到 totalram_pages */
    totalram_pages += free_all_bootmem();

    reservedpages = 0; /* 初始化被保留的页面计数 */
    /* 遍历从0到max_low_pfn（低端内存的最大页面帧号）的所有页面 */
    for (tmp = 0; tmp < max_low_pfn; tmp++)
        /* 判断页面是不是属于ram，并且是保留 */
        if (page_is_ram(tmp) && PageReserved(mem_map + tmp))
            /* 进入if，说明是属于ram，并且是被保留的 */
            reservedpages++; /* 计数器+1 */
    /* 计算内核代码段大小，_etext与_text由链接脚本提供 */
    codesize = (unsigned long)&_etext - (unsigned long)&_text;
    /* 计算内核数据段大小，_edata与_etext由链接脚本提供 */
    datasize = (unsigned long)&_edata - (unsigned long)&_etext;
    /* 计算内核初始化段大小，__init_end与__init_begin由链接脚本提供 */
    initsize = (unsigned long)&__init_end - (unsigned long)&__init_begin;
    /* 打印的信息包括可用内存、内核代码/数据/初始化段的大小和高端内存大小 */
    printk("Memory: %luk/%luk available (%dk kernel code, %dk reserved, %dk data, %dk init, %ldk highmem)\n",
           (unsigned long)nr_free_pages() << (PAGE_SHIFT - 10),
           max_mapnr << (PAGE_SHIFT - 10),
           codesize >> 10,
           reservedpages << (PAGE_SHIFT - 10),
           datasize >> 10,
           initsize >> 10,
           (unsigned long)(totalhigh_pages << (PAGE_SHIFT - 10)));
}