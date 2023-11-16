#include <linux/init.h>
#include <linux/multiboot.h>
#include <asm-i386/e820.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <asm-i386/io.h>
#include <asm-i386/page.h>
#include <linux/bootmem.h>

/* 用于存储grub返回的multiboot_t结构体的地址，
我懒得为这个变量单独写个c文件，就放在这里吧, 因为主要就是这个文件中的代码会用到
在head.S中，我们已经从ebx中为这个变量放上了正确的地址 */
struct multiboot_t *grub_multiboot_ptr;

/* 该结构体存储e820结构体信息 */
struct e820map e820;

/* 以下这两个字段，实质是grub通过INT 0x15, AX=0xE801返回值计算得出 */
/* 这个字段表示低内存的大小，单位是千字节。低内存指的是PC兼容系统中的前640千字节的内存，
该值比理论64k值少了1K，估计是不让使用EBDA区域（扩展BIOS数据区，9fc000 - 9ffff，刚好1KB）
在启动过程中，BIOS 会将自己的某些数据结构和运行时信息放在这个区域中 */
#define MEM_LOWER_K (grub_multiboot_ptr->mem_lower)
/* 这个字段代表高内存的大小，单位是千字节。高内存指的是PC兼容系统中第1兆字节以上的内存空间，
该值比理论511m值少了128K，可能为一些硬件保留的内存，范围就是512m最高的那128KB */
#define MEM_UPPER_K (grub_multiboot_ptr->mem_upper)

/* 在1mb内存布局中可以使用的部分叫做lowmem，其大小应该是640KB，范围为0 - a0000(不含)，
但是这里不知道为什么是size是9f000，比640KB少了4KB，可能是为了多留一些空间另作他用吧*/
#define LOWMEMSIZE() (0x9f000)

/* 将数据拷贝到e820数组中，参数：
start:内存区域的起始地址，
size:内存区域的字节大小，
type:内存区域的类型 */
void __init add_memory_region(unsigned long long start, unsigned long long size, int type)
{
    int x = e820.nr_map;

    if (x == E820MAX)
    {
        printk("Ooops! Too many entries in the memory map!\n");
        return;
    }

    e820.map[x].addr = start;
    e820.map[x].size = size;
    e820.map[x].type = type;
    e820.nr_map++;
}

/* 这个函数就是用来打印出已经经过合法性检查，已经添加e820map的内存区域信息。 */
static void __init print_memory_map(char *who)
{
    int i;

    for (i = 0; i < e820.nr_map; i++)
    {
        printk(" %s: %016Lx @ %016Lx ", who,
               e820.map[i].size, e820.map[i].addr);
        switch (e820.map[i].type)
        {
        case E820_RAM:
            printk("(usable)\n");
            break;
        case E820_RESERVED:
            printk("(reserved)\n");
            break;
        case E820_ACPI:
            printk("(ACPI data)\n");
            break;
        case E820_NVS:
            printk("(ACPI NVS)\n");
            break;
        default:
            printk("type %lu\n", e820.map[i].type);
            break;
        }
    }
}

/* Linux2.4中，copy_e820_map功能是复制BIOS提供的e820内存映射从0页拷贝（原Linux2.4在head.S中会拷贝到0页中）到一个安全的地方，
并在复制的过程中对其进行一些基本的检查。内存映射（memory map）是系统中可用物理内存区域的列表。
e820内存映射是通过BIOS中断0xE820提供的，并且通常在操作系统的早期启动阶段被使用来识别物理内存的大小和位置。
但是在我们的系统中，我们是grub使用multiboot规范引导，关于内存的信息存储在引导结束后返回在ebx中的地址（变量grub_multiboot_ptr）指向的结构体中，
而且我也没有拷贝到0页中去。所以，我们写一个函数copy_multiboot_mmap来平替copy_e820_map，
名字由来：multiboot返回的结构体中，关于内存映射的成员都以mmap开头。该函数就是改了改copy_e820_map函数。
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
在启动阶段，操作系统需要知道可用的物理内存布局，这样才能对内存进行管理和分配, 在Linux2.4中，
它先调用copy_e820_map来从0页中复制setup传递过来的e820结构体信息，
如果复制的信息没有通过合理性检查，那么就会通过其他BIOS调用（INT 0x15, AX=0xE801 和 INT 0x15, AX=0x88）得到的内存信息来直接创建两个内存映射。
0xe801分别检测出低15m和16m-4g内存（最大支持4G），对应ALT_MEM_K（取自alternative，并且是算出了总大小的 ）。
0x88最多检测64m内存，对应EXT_MEM_K（取自extended），详见大象书p177。
由于我们是grub使用multiboot规范启动，获得内存区域信息要靠multiboot规范提供，
所以我写了个函数copy_multiboot_mmap来替代copy_e820_map。并且失败了要靠multiboot内提供的mem_lower与mem_upper（0xe801），
来重新添加内存区域，mem_lower与mem_upper含义见对应的宏定义。由于grub限制，我们相比于原Linux2.4，少了调用0x88后的数据。
关于内存区域映射的处理，只有内存区域映射信息的原始来源（由grub支持multiboot规范提供）、
拷贝到0页、拷贝方式（函数copy_multiboot_mmap）、失败后从其他渠道得到内存区域信息(0xe801)的形式、少了调用0x88不同，其他完全相同 */
void __init setup_memory_region(void)
{
    char *who = "BIOS-e820"; /* 表示谁提供的内存区域信息，虽然我们是由grub使用的multiboot规范得到的mmap，但是本质上还是由bios的e820中断提供 */
    if (copy_multiboot_mmap((struct mmap_entry_t *)(grub_multiboot_ptr)->mmap_addr, grub_multiboot_ptr->mmap_length / sizeof(struct mmap_entry_t)) < 0)
    {
        /* 如果从multiboot提供的mmap获得内存区域信息失败，那么就改为从其他渠道获得内存区域信息，我们是grub引导，
        grub提供的multiboot结构体信息，也就是结构体内部的mem_lower与mem_upper，这两个本质上是调用bios INT 0x15, AX=0xE801返回值计算得出 */
        who = "BIOS-E801";
        e820.nr_map = 0;
        /* lowmemsize Linux2.4中定义是636k，而mem_lower_k是639k，所以这里取小值 */
        add_memory_region(0, LOWMEMSIZE() < MEM_LOWER_K << 10 ? LOWMEMSIZE() : MEM_LOWER_K << 10, E820_RAM);
        add_memory_region(HIGH_MEMORY, MEM_UPPER_K << 10, E820_RAM);
    }
    print_str("BIOS-provided physical RAM map:\n");
    print_memory_map(who);
}

/* 在链接脚本中使用 _text = .; _text 被定义为一个标签，它指向一个特定的内存地址。当声明 extern char _text;，
实际上是在告诉编译器：“存在一个名为 _text 的符号，它在其他地方定义了。” ，编译器并不关心 _text 是变量还是标签；它只是知道有这么个符号。
在链接脚本中定义标签，然后.c中extern char 标签，然后取标签地址。这是一个常见的做法，
尤其是在嵌入式系统或操作系统的内核代码中。这样做可以让 C 代码访问链接脚本中定义的特定内存地址。*/
extern char _text, _etext, _edata, _end;

/* 用来表示内核代码段的资源，起始地址是1m，结束地址初始化为0 */
static struct resource code_resource = {"Kernel code", 0x100000, 0};

/* 用来表示内核数据段的资源 */
static struct resource data_resource = {"Kernel data", 0, 0};

void __init setup_arch(char **cmdline_p)
{

    unsigned long start_pfn;    /* 用于记录内存管理系统可管理的内存起始页面帧号 */
    unsigned long max_pfn;      /* 记录内存条可用最大内存地址对应的页面帧号 */
    unsigned long max_low_pfn;  /* 记录32位系统下内存管理系统最少可管理内存 */
    int i;                      /* 循环变量 */
    unsigned long bootmap_size; /* 记录引导内存分配器的位图大小 */
    setup_memory_region();      /* 建立内存区域映射 */
    /* _text由链接脚本提供。假设_text标签指代的地址是0x1000，最后start_code的赋值就是0x1000。其实无论变量也好，标签也好，都是指代了个地址，
    &就是得到这个符号（标签，变量）所指代的地址 */
    init_mm.start_code = (unsigned long)&_text;
    init_mm.end_code = (unsigned long)&_etext; /* _etext是由链接脚本提供的标签 */
    init_mm.end_data = (unsigned long)&_edata; /* _edata是由链接脚本提供的标签 */
    init_mm.brk = (unsigned long)&_end;        /* _end是由链接脚本提供的标签 */

    code_resource.start = virt_to_bus(&_text);
    code_resource.end = virt_to_bus(&_etext) - 1;
    data_resource.start = virt_to_bus(&_etext);
    data_resource.end = virt_to_bus(&_edata) - 1;

/* PFN 代表 "Page Frame Number, UP 和 DOWN 分别指向上舍入和向下舍入到最接近的页边界
PHYS 指的是将页帧号转换为物理地址 */
/* 用于将一个地址向上舍入到最接近的页边界虚拟地址 */
#define PFN_UP(x) (((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)

/* 这个宏用于将一个地址向下舍入到最接近的页边界虚拟地址 */
#define PFN_DOWN(x) ((x) >> PAGE_SHIFT)

/* 于将页帧号转换回表示的物理地址 */
#define PFN_PHYS(x) ((x) << PAGE_SHIFT)

/* 预留给vmalloc与initrd使用的内存，vmalloc分配会虚拟地址连续，kmalloc物理地址连续
initrd(Initial RAM Disk) 是一个在内核启动初期加载到 RAM 的临时根文件系统。它主要用于预加载某些驱动程序或其他必要的东西，
直到真正的 root 文件系统可以被挂载。 */
#define VMALLOC_RESERVE (unsigned long)(128 << 20)

/* 这定义了内核最大可直接线性映射（也就是说，整个内存条的物理地址都在内核虚拟地址空间范围内可以被直接访问）
的物理地址是1G（-3G是3G的原码取反，然后+1，最后是1G）-128MB = 896MB。 */
#define MAXMEM (unsigned long)(-PAGE_OFFSET - VMALLOC_RESERVE)

/* 内核最大可直接映射物理地址的下边界页表示的物理地址 */
#define MAXMEM_PFN PFN_DOWN(MAXMEM)

/* 不使用物理地址扩展（PAE）模式下的最大页帧号，为4g >> 12，也就是 1 << 20 */
#define MAX_NONPAE_PFN (1 << 20)

    start_pfn = PFN_UP(__pa(&_end)); /* 计算内存管理系统实际可以管理的页帧号（上边界） */
    max_pfn = 0;                     /* 初始化max_pfn */
    /* 遍历之前添加到e820map中的所有内存区域映射，找出内存条提供的可用内存的最高页面帧号 max_pfn */
    for (i = 0; i < e820.nr_map; i++)
    {
        unsigned long start, end;
        if (e820.map[i].type != E820_RAM)                    /* 内存管理系统管理的内存对应的e820结构体type自然是RAM */
            continue;                                        /* 不是RAM，自然跳过 */
        start = PFN_UP(e820.map[i].addr);                    /* 找到内存条提供的可用内存的上边界页面帧号 */
        end = PFN_DOWN(e820.map[i].addr + e820.map[i].size); /* 找到内存条提供的可用内存的下边界页面帧号 */
        if (start >= end)                                    /* 这种情况一定是end值溢出了 */
            continue;
        if (end > max_pfn) /* 取最大的那个内存条提供的可用内存的下边界页面帧号 */
            max_pfn = end;
    }
    /*
     * Determine low and high memory ranges:
     */
    max_low_pfn = max_pfn; /* 先让32位系统下内存管理系统最小可管理内存等于内存条提供的可用内存 */
    /* 如果内存条提供的可用内存大于32位系统可以直接映射的物理内存（896m）*/
    if (max_low_pfn > MAXMEM_PFN)
    {
        max_low_pfn = MAXMEM_PFN; /* 那么32位系统下内存管理系统最小可管理内存就是896m，这部分一定可以被管理 */

/* 以下五个预编译条件包裹的代码留在这里，是为了更好理解高端内存与PAE机制的实际效果 */
#ifndef CONFIG_HIGHMEM
        /* 以下五句代码包裹在#ifndef CONFIG_HIGHMEM内，表示没有开启CONFIG_HIGHMEM，我留在这里是为了更好理解高端内存机制与PAE机制
        而此时内存条提供的可用内存大于32位系统可以直接映射的物理内存（896m），那么打印警告信息，实际可被管理的内存就只有896m */
        printk(KERN_WARNING "Warning only %ldMB will be used.\n", MAXMEM >> 20);
        /* 如果内存条提供的可用内存对应的最大页帧号，大于了没有开启PAE机制对应的支持最大的页帧号（4GB对应的页面帧号），
        说明此时内存条提供的可用内存不仅大于896MB，还大于4GB，那么打印警告信息，建议开启PAE机制（该机制允许32位系统支持最大64G内存） */
        if (max_pfn > MAX_NONPAE_PFN)
            printk(KERN_WARNING "Use a PAE enabled kernel.\n");
        else /* 来到这里，说明内存条提供的可用内存大于896m，但是小于4GB，那么建议开启高端内存机制 */
            printk(KERN_WARNING "Use a HIGHMEM enabled kernel.\n");
#else /* #define CONFIG_HIGHMEM */
#ifndef CONFIG_X86_PAE
        /* 以下四句包裹在#ifndef CONFIG_HIGHMEM的#else 和#ifndef CONFIG_X86_PAE，表示开启了高端内存机制，但是没有开启PAE， \
         */
        /* 此时内存条提供的可用内存大于4G */
        if (max_pfn > MAX_NONPAE_PFN)
        {
            max_pfn = MAX_NONPAE_PFN;                                /* 那么最大可被管理的内存就是4G */
            printk(KERN_WARNING "Warning only 4GB will be used.\n"); /* 打印警告信息，只有4G内存可被管理 */
            printk(KERN_WARNING "Use a PAE enabled kernel.\n");      /* 建议开启PAE */
        }
#endif /* CONFIG_X86_PAE */
#endif /* CONFIG_HIGHMEM */
    }

    /* 初始化引导内存分配器，并得到引导内存分配器的位图大小 */
    bootmap_size = init_bootmem(start_pfn, max_low_pfn);
}