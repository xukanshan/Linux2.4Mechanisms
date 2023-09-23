#ifndef _I386_PAGE_H
#define _I386_PAGE_H

#define PAGE_SHIFT 12 // 表示将1左移多少位才能得到页面大小
#define PAGE_SIZE (1UL << PAGE_SHIFT)

#define __PAGE_OFFSET (0xC0000000)
#define PAGE_OFFSET ((unsigned long)__PAGE_OFFSET)
/**
 * P--位0是存在（Present）标志，用于指明表项对地址转换是否有效。P=1表示有效；P=0表示无效。
 * 在页转换过程中，如果说涉及的页目录或页表的表项无效，则会导致一个异常。
 * 如果P=0，那么除表示表项无效外，其余位可供程序自由使用。
 * 例如，操作系统可以使用这些位来保存已存储在磁盘上的页面的序号。
 */
#define PAGE_PRESENT 0x1
#define PAGE_USER 0x4
/**
 * R/W--位1是读/写（Read/Write）标志。如果等于1，表示页面可以被读、写或执行。
 * 如果为0，表示页面只读或可执行。
 * 当处理器运行在超级用户特权级（级别0、1或2）时，则R/W位不起作用。
 * 页目录项中的R/W位对其所映射的所有页面起作用。
 */
#define PAGE_WRITE 0x2

typedef struct
{
    unsigned long pgd;
} pgd_t;

typedef struct
{
    unsigned long pte_low;
} pte_t;

typedef struct
{
    unsigned long pgprot;
} pgprot_t;

#endif
