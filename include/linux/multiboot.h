#ifndef INCLUDE_MULTIBOOT_H_
#define INCLUDE_MULTIBOOT_H_

#include <asm/types.h>
/**
 * 参考 Multiboot 规范中的 Machine state
 * 启动后，在32位内核进入点，机器状态如下：
 *   1. CS 指向基地址为 0x00000000，限长为4G – 1的代码段描述符。
 *   2. DS，SS，ES，FS 和 GS 指向基地址为0x00000000，限长为4G – 1的数据段描述符。
 *   3. A20 地址线已经打开。
 *   4. 页机制被禁止。
 *   5. 中断被禁止。
 *   6. EAX = 0x2BADB002，魔数，说明操作系统是由符合 Multiboot 的 boot loader 加载的
 *   7. 系统信息和启动信息块的线性地址保存在 EBX中（相当于一个指针）。
 *      以下即为这个信息块的结构（Multiboot information structure）
 */

typedef struct multiboot_t
{
    uint32_t flags; // Multiboot 的版本信息，用于表示哪些其他字段是有效的或已经被设置。
    /**
     * 从 BIOS 获知的可用内存
     *
     * mem_lower和mem_upper分别指出了低端和高端内存的大小，单位是KB。
     * 低端内存的首地址是0，高端内存的首地址是1M。
     * 低端内存的最大可能值是640K。
     * 高端内存的最大可能值是最大值减去1M。但并不保证是这个值。
     */
    uint32_t mem_lower;
    uint32_t mem_upper;

    uint32_t boot_device; // 指出引导程序从哪个 BIOS 磁盘设备载入的OS映像
    uint32_t cmdline;     // 使用一个引导加载器（例如 GRUB）来启动一个操作系统内核时，
    // 可以向内核传递一系列参数。这些参数通常被称为"内核命令行参数"或简单地"内核参数"。
    // 这些参数可以用来影响内核的行为，例如告诉内核哪个设备应该作为根文件系统，或者禁用某个特定的硬件特性等。
    uint32_t mods_count; // 描述了启动模块的列表。mods_count 是模块的数量。mods_addr 是模块列表的地址。
    uint32_t mods_addr;

    /**
     * "flags"字段的 bit-5 设置了
     * ELF 格式内核映像的section头表。
     * 包括ELF 内核的 section header table 在哪里、每项的大小、一共有几项以及作为名字索引的字符串表。
     */
    uint32_t num;   // 表示 section header table 中的条目数。
    uint32_t size;  // 表示每个条目的大小。
    uint32_t addr;  // 是 section header table 的地址。
    uint32_t shndx; // shndx 提供了一个在 section header table 中的索引，它指向了 section header string table（区段名字表）。
    // 这样，如果知道 shndx 的值，就可以直接找到 section header string table 的位置，从而访问并解析该表中的字符串，获取区段的名称
    // 每个section header都有个sh_name字段，用这个在区段名字表中去索引，才能找到这个段叫什么名字。
    // 这些信息允许操作系统内核解析自己的ELF映像的结构

    /**
     * 当启动一个操作系统，了解物理内存的布局是非常重要的。尤其是在操作系统的早期阶段，例如在初始化内存管理时。
     * 以下两项指出保存由BIOS提供的内存分布的缓冲区的地址和长度
     * mmap_addr是缓冲区的地址，mmap_length是缓冲区的总大小
     * 缓冲区由一个或者多个下面的大小/结构对 mmap_entry_t 组成。每个 `mmap_entry_t` 通常包括：
     * 区域的大小。
     * 区域的起始地址。
     * 区域的长度。
     * 区域的类型（例如，可用、被保留、ACPI可回收等）。
     */
    uint32_t mmap_length;
    uint32_t mmap_addr;

    uint32_t drives_length; // 指出第一个驱动器结构的物理地址，驱动结构描述了计算机上的一个物理驱动器（如硬盘、光盘驱动器、软盘驱动器等）。
    // 这种结构提供了驱动器的详细信息，比如：
    // 驱动器编号（例如，第一个硬盘、第二个硬盘等）驱动器的物理几何（如柱面、磁头、每柱面的扇区数等）分区信息（如果可用）
    uint32_t drives_addr;  // 指出第一个驱动器这个结构的大小
    uint32_t config_table; // ROM 配置表，ROM配置表通常指的是在只读存储器（ROM）中存放的与硬件相关的配置信息。
    // 如：已安装的硬件设备列表（如驱动器、视频卡等）及其参数。系统中断向量和地址。各种硬件相关的设置或标志。
    // 随着UEFI的使用，现代的计算机系统越来越少地依赖这样的配置表
    uint32_t boot_loader_name; // boot loader 的名字
    uint32_t apm_table;        // APM 表高级电源管理（APM，Advanced Power Management）是一个早期用于PC兼容机的电源管理接口。
    // 它允许操作系统与BIOS合作，控制硬件的电源状态，以实现如节电模式、待机状态和休眠功能等。
    // 高级电源管理表（APM表）通常是一个数据结构，它包含了与APM相关的信息和功能。

    // 提供了有关 VESA BIOS 扩展 (VBE) 的信息。这通常与图形显示有关
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
} __attribute__((packed)) multiboot_t;

/**
 * size是相关结构的大小，单位是字节，它可能大于最小值20
 * base_addr_low是启动地址的低32位，base_addr_high是高32位，启动地址总共有64位
 * length_low是内存区域大小的低32位，length_high是内存区域大小的高32位，总共是64位
 * type是相应地址区间的类型，1代表可用RAM，所有其它的值代表保留区域
 */
typedef struct mmap_entry_t
{
    uint32_t size; // 表示这个结构体的大小，但是不包含size字段自身。
    // 它允许不同版本的结构体有不同的大小，而size字段可以告诉代码如何逐个遍历这些结构体。
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;  //表示该内存段类型的标识符。例如，它可能表示这段内存是可用的、或者被保留、或者有特定的用途等
    //常见的值可能包括（但不限于）：1: 表示这段内存是可用的，可以被操作系统使用。2: 表示这段内存是被保留的，不能被使用，通常由于硬件或其他原因。
} __attribute__((packed)) mmap_entry_t;

// 声明全局的 multiboot_t * 指针
// 内核未建立分页机制前暂存的指针

extern void *glb_mboot_ptr __attribute__((__section__(".data.init")));

#endif
