#include <linux/init.h>
#include <linux/multiboot.h>
#include <asm-i386/e820.h>

/* 用于存储grub返回的multiboot_t结构体的地址，
我懒得为这个变量单独写个c文件，就放在这里吧, 因为主要就是这个文件中的代码会用到
在head.S中，我们已经从ebx中为这个变量放上了正确的地址 */
struct multiboot_t *grub_mboot_ptr;

/* 该结构体存储e820结构体信息 */
struct e820map e820;

/* 在Linux2.4中，这两个宏是定义的拷贝到0页的BIOS INT 0x15, AX=0xE801 和 INT 0x15, AX=0x88 的返回值，
这里我们平替到multiboot当中对应的数据，注意mem_upper对应的单位是KB*/
#define ALT_MEM_K (grub_mboot_ptr->mem_lower)
#define EXT_MEM_K (grub_mboot_ptr->mem_upper)

/* 在1mb内存布局中可以使用的部分叫做lowmem，其大小应该是640KB，范围为0 - a0000(不含)，
但是这里不知道为什么是size是9f000，可能是因为保守？*/
#define LOWMEMSIZE() (0x9f000)

/* 将数据拷贝到e820数组中，参数：
start:内存区域的起始地址，
size:内存区域的大小，
type:内存区域的类型 */
void __init add_memory_region(unsigned long long start, unsigned long long size, int type)
{
    int x = e820.nr_map;

    if (x == E820MAX)
    {
        // printk("Ooops! Too many entries in the memory map!\n");   /* 暂未实现printk */
        return;
    }

    e820.map[x].addr = start;
    e820.map[x].size = size;
    e820.map[x].type = type;
    e820.nr_map++;
}

/* 这个函数就是用来打印出已经经过合法性检查，已经添加e820map的内存区域信息。但暂未实现printk */
// static void __init print_memory_map(char *who)
// {
//     int i;

//     for (i = 0; i < e820.nr_map; i++)
//     {
//         printk(" %s: %016Lx @ %016Lx ", who,
//                e820.map[i].size, e820.map[i].addr);
//         switch (e820.map[i].type)
//         {
//         case E820_RAM:
//             printk("(usable)\n");
//             break;
//         case E820_RESERVED:
//             printk("(reserved)\n");
//             break;
//         case E820_ACPI:
//             printk("(ACPI data)\n");
//             break;
//         case E820_NVS:
//             printk("(ACPI NVS)\n");
//             break;
//         default:
//             printk("type %lu\n", e820.map[i].type);
//             break;
//         }
//     }
// }

/* Linux2.4中，copy_e820_map功能是复制BIOS提供的e820内存映射从0页拷贝（原Linux2.4在head.S中会拷贝到0页中）到一个安全的地方，
并在复制的过程中对其进行一些基本的检查。内存映射（memory map）是系统中可用物理内存区域的列表。
e820内存映射是通过BIOS中断0xE820提供的，并且通常在操作系统的早期启动阶段被使用来识别物理内存的大小和位置。
但是在我们的系统中，我们是grub使用multiboot规范引导，关于内存的信息存储在引导结束后返回在ebx中的地址（变量grub_mboot_ptr）指向的结构体中，
而且我也没有拷贝到0页中去。所以，我们写一个函数copy_multiboot_mmap来平替copy_e820_map，
名字由来：multiboot返回的结构体中，关于内存映射的成员都以mmap开头。该函数就是改了改copy_e820_map函数。
关于内存区域映射的处理，只有内存区域映射信息的原始来源（由grub支持multiboot规范提供）、
拷贝到0页、拷贝方式（函数copy_multiboot_mmap）不同，其他完全相同
参数：mmap是提供的mmap数组的起始地址，nr是mmap数组内的有效元素数量 */
static int __init copy_multiboot_mmap(struct mmap_entry_t *mmap, int nr)
{
    /* 我们检查内存映射表至少包含2个元素后才使用它，
    几乎每台已知的PC，都有两个内存区域：一个从0到640k，另一个从1mb起 */
    if (nr < 2)
        return -1;

    do
    {
        unsigned long long start = mmap->base_addr_low + (((unsigned long long)mmap->base_addr_high) << 32); /* 计算每个内存区域的起始 */
        unsigned long long size = mmap->length_low + (((unsigned long long)mmap->length_high) << 32);        /* 计算每个内存区域的大小 */
        unsigned long long end = start + size;                                                               /* 计算每个内存区域的结束 */
        unsigned long type = mmap->type;                                                                     /* 得到每个内存区域的类型 */

        if (start > end) /* 如果start > end，那么一定是64位值溢出，也就是地址范围超过了64位可表达范围 */
            return -1;

        /* 可能会有正常类型的内存区域落在了640k - 1m范围之内，我们要去要将落在这里面的部分剔除。
        原因可研究1m内存布局中，该范围是什么 */
        if (type == E820_RAM) /* 我们要处理的是正常内存区域的落在640k -1m范围内的部分 */
        {                     /* 判断条件，就是start < 1m，end > 640k，也就是正常内存区域有部分落在了640k - 1m */
            if (start < 0x100000ULL && end > 0xA0000ULL)
            { /* 如果是起始小于640k，但是结束落在了640k - 1m范围内，那么我们就抛弃落在640k - 1m范围内的部分 */
                if (start < 0xA0000ULL)
                    add_memory_region(start, 0xA0000ULL - start, type);
                /* 如果结束小于1m，但是起始落在了640k - 1m，那么直接跳过，正常内存区域一定不会是这个情况 */
                if (end <= 0x100000ULL)
                    continue;
                /* 来到这里，说明是起始落在了640k - 1m，但是结束超过了1m，所以起始就直接改成1m，然后由下面的add_memory_region进行拷贝 */
                start = 0x100000ULL;
                size = end - start;
            }
        }
        add_memory_region(start, size, type);
    } while (mmap++, --nr);
    return 0;
}

/* 它的作用是建立内存区域(memory region)，memory map 是系统中可用物理内存区域的列表。
在启动阶段，操作系统需要知道可用的物理内存布局，
这样才能对内存进行管理和分配, 在Linux2.4中，
它先调用copy_e820_map来从0页中复制setup传递过来的e820结构体信息，
如果复制的信息没有通过合理性检查，那么就会通过其他BIOS调用（INT 0x15, AX=0xE801 和 INT 0x15, AX=0x88）
得到的内存信息来直接创建两个内存映射。一个是0-64K，另一个是1m-合适的内存结束位置。
由于我们是grub使用multiboot规范启动，获得内存区域信息要靠multiboot规范提供，
所以我写了个函数copy_multiboot_mmap来替代copy_e820_map */
void __init setup_memory_region(void)
{
    char *who = "BIOS-e820"; /* 表示谁提供的内存区域信息，虽然我们是由grub使用的multiboot规范得到的mmap，但是本质上还是由bios的e820中断提供 */
    if (copy_multiboot_mmap((struct mmap_entry_t *)(grub_mboot_ptr)->mmap_addr, grub_mboot_ptr->mmap_length / sizeof(struct multiboot_t)) < 0)
    {
        unsigned long mem_size;

        /* 比较了两个BIOS中断调用（INT 0x15, AX=0xE801 和 INT 0x15, AX=0x88）的返回值，分别对应ALT_MEM_K和EXT_MEM_K。
        选择两者中较大的一个值作为内存大小 */
        if (ALT_MEM_K < EXT_MEM_K)
        {
            mem_size = EXT_MEM_K;
            who = "BIOS-88";
        }
        else
        {
            mem_size = ALT_MEM_K;
            who = "BIOS-e801";
        }
        e820.nr_map = 0;
        /* 配合INT 0x15, AX=0xE801 和 INT 0x15, AX=0x88获得的信息添加两个内存区域 */
        add_memory_region(0, LOWMEMSIZE(), E820_RAM);
        /* mem_size对应的变量是KB，转换成B自然要左移10位 */
        add_memory_region(HIGH_MEMORY, (mem_size << 10) - HIGH_MEMORY, E820_RAM);
    }
    // printk("BIOS-provided physical RAM map:\n"); /* 暂未实现printk */
    // print_memory_map(who);    /* 暂未实现printk */
}

void __init setup_arch(char **cmdline_p)
{

    setup_memory_region();
}