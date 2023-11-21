#ifndef _ASM_I386_PAGETABLE_2LEVEL_H
#define _ASM_I386_PAGETABLE_2LEVEL_H

/* 定义页全局目录表索引在一个32位地址中的起始位 */
#define PGDIR_SHIFT 22

/* 每个页全局目录表的表项数目 */
#define PTRS_PER_PGD 1024

/* 定义页中间目录表索引在一个32位地址中的起始位，
由于是2级页表体系，所以页中间目录表与页全局目录表表概念上相同 */
#define PMD_SHIFT 22

/* 每个页中间目录表的表项数目，
由于是2级页表体系，所以页中间目录表与页全局目录表表概念上相同，
而这个宏经常用于遍历使用，所以相当于就遍历1次，也就是我们遍历的时候
就是遍历了一次页全局目录表表项自己 */
#define PTRS_PER_PMD 1

/* 每个页表拥有的页表表项数目 */
#define PTRS_PER_PTE 1024

/* 解决汇编代码不识别结构体定义问题 */
#ifndef __ASSEMBLY__

/* 用于根据传入的虚拟地址与管理这个虚拟地址的页全局目录表表项指针，
返回其虚拟地址对应的页中间目录表表项指针，
由于是2级页表体系，所以页中间目录表与页全局目录表是同一个概念，
所以这里就是简单返回管理其的页全局目录表表项指针
extern inline替换成了static inline */
static inline pmd_t *pmd_offset(pgd_t *dir, unsigned long address)
{
    return (pmd_t *)dir;
}

#endif /* __ASSEMBLY__ */

/* 用于设定一个pte表项的内容 */
#define set_pte(pteptr, pteval) (*(pteptr) = pteval)

/* 用于设定一个pmd表项的内容 */
#define set_pmd(pmdptr, pmdval) (*(pmdptr) = pmdval)

/* 用于设定一个pgd表项的内容 */
#define set_pgd(pgdptr, pgdval) (*(pgdptr) = pgdval)

/* 传入物理页索引（不是虚拟页表表项索引），以及属性位，然后返回这个页表的内容 */
#define __mk_pte(page_nr, pgprot) __pte(((page_nr) << PAGE_SHIFT) | pgprot_val(pgprot))

#endif /* _ASM_I386_PAGETABLE_2LEVEL_H */