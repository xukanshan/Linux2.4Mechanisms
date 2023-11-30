#ifndef _LINUX_MULTIBOOT_H
#define _LINUX_MULTIBOOT_H
/* 这个头文件是为了应对grub使用multiboot规范启动后的返回信息，grub启动后，会返回multiboot_t结构体地址（ebx寄存器中），
并且我们会用到mmap_addr指向的mmap_entry_t数组，从中取得grub使用multiboot规范启动后的内存区域信息 */

#include <linux/types.h>

/* multiboot_t结构体定义，grub使用multiboot规范启动后会返回这样一个结构体，其地址在ebx中，结构体字段是否有意义取决于multiboot文件头中的flags设置，
我们只需要用到mmap_length与mmap_addr这两个成员，其他不用关注 */
struct multiboot_t
{
    uint32_t flags; /* Multiboot 的版本信息 */
    /*
     * 从 BIOS 获知的可用内存
     *
     * mem_lower和mem_upper分别指出了低端和高端内存的大小，单位是KB。
     * 低端内存的首地址是0，高端内存的首地址是1M。
     * 低端内存的最大可能值是640K。
     * 高端内存的最大可能值是最大值减去1M。但并不保证是这个值。
     */
    uint32_t mem_lower;
    uint32_t mem_upper;

    uint32_t boot_device; /* 指出引导程序从哪个 BIOS 磁盘设备载入的OS映像 */
    uint32_t cmdline;     /* 内核命令行 */
    uint32_t mods_count;  /* boot 模块列表 */
    uint32_t mods_addr;

    /*
     * "flags"字段的 bit-5 设置了
     * ELF 格式内核映像的section头表。
     * 包括ELF 内核的 section header table 在哪里、每项的大小、一共有几项以及作为名字索引的字符串表。
     */
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;

    /*
     * 以下两项指出保存由BIOS提供的内存分布的缓冲区的地址和长度
     * mmap_addr是缓冲区的地址，mmap_length是缓冲区的总大小
     * 缓冲区由一个或者多个下面的大小/结构对 mmap_entry_t 组成
     */
    uint32_t mmap_length;
    uint32_t mmap_addr;

    uint32_t drives_length;    /* 指出第一个驱动器结构的物理地址 */
    uint32_t drives_addr;      /* 指出第一个驱动器这个结构的大小 */
    uint32_t config_table;     /* ROM 配置表 */
    uint32_t boot_loader_name; /* boot loader 的名字 */
    uint32_t apm_table;        /* APM 表 */
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
} __attribute__((packed));

/*
 * size是相关结构的大小，单位是字节，它可能大于最小值20
 * base_addr_low是启动地址的低32位，base_addr_high是高32位，启动地址总共有64位
 * length_low是内存区域大小的低32位，length_high是内存区域大小的高32位，总共是64位
 * type是相应地址区间的类型，1代表可用RAM，所有其它的值代表保留区域
 */
struct mmap_entry_t
{
    uint32_t size; /* 留意 size 是不含 size 自身变量的大小 */
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} __attribute__((packed));

#endif /* _LINUX_MULTIBOOT_H */