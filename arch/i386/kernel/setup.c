#include <linux/init.h>
#include <linux/multiboot.h>
#include <asm-i386/e820.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <asm-i386/io.h>
#include <asm-i386/page.h>
#include <linux/bootmem.h>
#include <linux/smp.h>
#include <asm-i386/processor.h>
#include <asm-i386/bitops.h>
#include <asm-i386/system.h>
#include <asm-i386/desc.h>
#include <asm-i386/atomic.h>
#include <asm-i386/current.h>
#include <asm-i386/mmu_contex.h>

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
    printk("BIOS-provided physical RAM map:\n");
    print_memory_map(who);
}

/* 0xaa55 是标准PC兼容机上用于标记有效ROM的签名。
如果在指定地址 x 处的前两个字节是 0xaa55，这通常意味着这个地址是一个有效的ROM起始点。 */
#define romsignature(x) (*(unsigned short *)(x) == 0xaa55)

/* 系统最多能够处理6个ROM区域 */
#define MAXROMS 6

/* 每个元素代表了一个标准的I/O资源。I/O资源指的是系统中的I/O端口，
这些端口被用于与特定的硬件设备进行通信。这些资源在系统启动时被注册，
以确保操作系统能够正确地管理和使用这些硬件资源
"dma1", 第一个DMA（直接内存访问）控制器。
"pic1", 第一个PIC（可编程中断控制器）。
"timer", 系统计时器。
"keyboard", 键盘控制器。
"dma page reg", DMA页面寄存器。
"pic2", 第二个PIC。
"dma2", 第二个DMA控制器。
"fpu", 浮点单元 */
struct resource standard_io_resources[] = {
    {"dma1", 0x00, 0x1f, IORESOURCE_BUSY},
    {"pic1", 0x20, 0x3f, IORESOURCE_BUSY},
    {"timer", 0x40, 0x5f, IORESOURCE_BUSY},
    {"keyboard", 0x60, 0x6f, IORESOURCE_BUSY},
    {"dma page reg", 0x80, 0x8f, IORESOURCE_BUSY},
    {"pic2", 0xa0, 0xbf, IORESOURCE_BUSY},
    {"dma2", 0xc0, 0xdf, IORESOURCE_BUSY},
    {"fpu", 0xf0, 0xff, IORESOURCE_BUSY}};

/* 计算standard_io_resources数组中的元素数量 */
#define STANDARD_IO_RESOURCES (sizeof(standard_io_resources) / sizeof(struct resource))

/* 这个数组预定义了一些ROM区域，用于在系统初始化时被注册和管理。
在这个定义中，有两个预定义的ROM资源：系统ROM：通常存放系统固件（如BIOS）。地址范围是 0xF0000 到 0xFFFFF。
视频ROM：通常包含显卡的固件。地址范围是 0xc0000 到 0xc7fff。
这些预定义的资源可以通过 probe_roms 函数进一步探测和注册 */
static struct resource rom_resources[MAXROMS] = {
    {"System ROM", 0xF0000, 0xFFFFF, IORESOURCE_BUSY},
    {"Video ROM", 0xc0000, 0xc7fff, IORESOURCE_BUSY}};

/* 这个资源代表了内核代码所在的内存区域。起始地址0x100000（即1MB）是传统上x86架构中内核代码开始的地方。
结束地址初始化为0，通常意味着它将在后续的代码中被设置 */
static struct resource code_resource = {"Kernel code", 0x100000, 0};

/* 这个资源代表内核数据区域。起始和结束地址都初始化为0，
这表明它们将在后续的代码中被确定。内核数据区域通常包括全局变量、静态数据等 */
static struct resource data_resource = {"Kernel data", 0, 0};

/* 这个资源代表视频RAM区域。通常是显卡使用的内存区域。标志表示这个资源已经被占用，不能被分配 */
static struct resource vram_resource = {"Video RAM area", 0xa0000, 0xbffff, IORESOURCE_BUSY};

/* 探测系统中存在的ROM，并将其放入rom_resources对应的resource空位中，
然后添加进入iomem_resources资源树中。这几个rom如下：
0xf0000-0xfffff 的 bios rom
0xc0000-0xc7fff 的 video rom
0xc8000-0xe0000 的 extension rom
0xe0000-以上 的64KB 大小的 extension rom
对照1m内存布局，起始探测的就是0xc0000到0xfffff结束的所有rom*/
static void __init probe_roms(void)
{
    /* 用来跟踪发现的ROM数量，初始值设为1，
    因为函数稍后会首先注册一块预定义的ROM区域 */
    int roms = 1;
    unsigned long base;      /* 用于存储将要检查的内存地址 */
    unsigned char *romstart; /* 用于指向 base 地址处的内存 */

    /* 将bios对应的rom添加进入iomem_resource树中 */
    request_resource(&iomem_resource, rom_resources + 0);

    /* 探测位于C000:0000 - C7FF:0000范围内的ROM, 每次增加2048字节（即2KB，这是ROM块的标准大小） */
    for (base = 0xC0000; base < 0xE0000; base += 2048)
    {
        romstart = bus_to_virt(base); /*  将物理地址 base 转换为虚拟地址 */
        if (!romsignature(romstart))  /* romstart 指向的位置是否有有效的ROM签名 */
            continue;                 /* 如果没有，continue */
        /* 如果找到有效的ROM签名，就证明这块区域的确是video rom，那么就将rom_resources中的video rom添加到iomem_resource树中 */
        request_resource(&iomem_resource, rom_resources + roms);
        roms++; /* 自增，表示又发现了一个ROM */
        break;  /* 这意味着一旦找到第一个有效的视频ROM, 循环就会停止 */
    }

    /* 遍历从 0xC8000 到 0xE0000 的地址范围，每次增加 2048 字节（即2KB）。这个范围指定了扩展ROM可能存在的位置 */
    for (base = 0xC8000; base < 0xE0000; base += 2048)
    {
        unsigned long length; /* 存储rom区域的长度 */

        romstart = bus_to_virt(base); /* 函数将当前循环中的物理地址（base）转换为虚拟地址，并将其存储在 romstart 指针 */
        if (!romsignature(romstart))  /* 检查 romstart 指向的地址是否包含有效的ROM签名 */
            continue;
        /* 计算ROM的长度。它读取 romstart 指向地址中第三个字节的值（romstart[2]），
        并将这个值乘以 512。这是因为在许多ROM中，第三个字节指定了ROM大小的一部分（以512字节块为单位） */
        length = romstart[2] * 512;
        if (length)
        {
            unsigned int i;       /* 循环变量 */
            unsigned char chksum; /* 存储校验和 */

            chksum = 0;
            /* 计算整个ROM的校验和。它将 romstart 指向的每个字节加到 chksum 变量中。校验和用于验证ROM数据的完整性 */
            for (i = 0; i < length; i++)
                chksum += romstart[i];

            /* Good checksum? */
            if (!chksum) /* chksum 的结果为0，表示校验和有效，这意味着ROM数据没有错误 */
            {
                rom_resources[roms].start = base;            /* rom_resources对应空位的resource结构体记录rom的起始位置 */
                rom_resources[roms].end = base + length - 1; /* rom_resources对应空位的resource结构体记录记录rom的结束 */
                rom_resources[roms].name = "Extension ROM";  /* rom_resources对应空位的resource结构体记录记录rom的名称 */
                rom_resources[roms].flags = IORESOURCE_BUSY; /* rom_resources对应空位的resource结构体记录记录rom的类型 */
                /* 将该rom对应的rom_resources中的resource添加到iomem_resources资源树中 */
                request_resource(&iomem_resource, rom_resources + roms);
                roms++;              /* 自增，表示又发现了一个ROM */
                if (roms >= MAXROMS) /* 如果找到的ROM数量达到了 MAXROMS 的上限，函数将结束 */
                    return;
            }
        }
    }

    /* base 变量设置为0xE0000。这个地址通常是系统中主板扩展ROM的标准地址 */
    base = 0xE0000;
    romstart = bus_to_virt(base); /* 将物理地址（在这里是 base）转换为内核可以直接访问的虚拟地址 */

    if (romsignature(romstart)) /* 检查 romstart 指向的地址是否包含有效的ROM签名 */
    {
        rom_resources[roms].start = base;            /* rom_resources对应空位的resource结构体记录rom的起始位置 */
        rom_resources[roms].end = base + 65535;      /* rom_resources对应空位的resource结构体记录记录rom的结束，共64KB大小 */
        rom_resources[roms].name = "Extension ROM";  /* rom_resources对应空位的resource结构体记录记录rom的名称 */
        rom_resources[roms].flags = IORESOURCE_BUSY; /* rom_resources对应空位的resource结构体记录记录rom的类型 */
        /* 将该rom对应的rom_resources中的resource添加到iomem_resources资源树中 */
        request_resource(&iomem_resource, rom_resources + roms);
    }
}

/* 在链接脚本中使用 _text = .; _text 被定义为一个标签，它指向一个特定的内存地址。当声明 extern char _text;，
实际上是在告诉编译器：“存在一个名为 _text 的符号，它在其他地方定义了。” ，编译器并不关心 _text 是变量还是标签；它只是知道有这么个符号。
在链接脚本中定义标签，然后.c中extern char 标签，然后取标签地址。这是一个常见的做法，
尤其是在嵌入式系统或操作系统的内核代码中。这样做可以让 C 代码访问链接脚本中定义的特定内存地址。*/
extern char _text, _etext, _edata, _end;

/* 进行体系架构初始化工作，包括：建立内存区域映射，初始化init_mm描述内核代码与数据资源，
初始化引导内存分配器，进一步完善页面映射机制，并建立起内存页面管理机制。完成管理节点内存的pglist初始化。
添加资源进入iomem_resources，与ioport_resource树中。
传入参数是命令行。 */
void __init setup_arch(char **cmdline_p)
{

    unsigned long start_pfn;    /* 用于记录内存管理系统可管理的内存起始页面帧号 */
    unsigned long max_pfn;      /* 记录内存条可用最大内存地址对应的页面帧号 */
    unsigned long max_low_pfn;  /* 记录32位系统下内存管理系统最少可管理内存，也是可直接映射内存区与的边界，一般为896m */
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
/* 用于将一个地址向上舍入到最接近的页帧号 */
#define PFN_UP(x) (((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)

/* 这个宏用于将一个地址向下舍入到最接近的页帧号 */
#define PFN_DOWN(x) ((x) >> PAGE_SHIFT)

/* 于将页帧号转换回表示的虚拟地址 */
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

    /* 初始化引导内存分配器，并得到引导内存分配器的位图大小，且这个位图全部被置1 */
    bootmap_size = init_bootmem(start_pfn, max_low_pfn);

    /* 遍历e820映射，并将低于max_low_pfn（线性映射区域）的可用RAM内存区域注册到bootmem分配器中，
    也就是位图清0（位图在初始化时全部置1了），以供内核在早期阶段使用 */
    for (i = 0; i < e820.nr_map; i++)
    {
        /* curr_pfn记录需要注册的ram区域的起始页帧号，last_pfn记录需要注册的ram区域结束页帧号，size记录其大小 */
        unsigned long curr_pfn, last_pfn, size;
        if (e820.map[i].type != E820_RAM) /* 不是ram区域自然跳过 */
            continue;
        curr_pfn = PFN_UP(e820.map[i].addr); /* 得到当前需要添加的ram区域的起始地址的上边界页帧号 */
        if (curr_pfn >= max_low_pfn)         /* 如果需要添加的ram区域超过直接映射区域，那么就跳过 */
            continue;
        /* 得到当前需要添加的ram区域的起始地址的下边界页帧号 */
        last_pfn = PFN_DOWN(e820.map[i].addr + e820.map[i].size);

        if (last_pfn > max_low_pfn) /* 如果如果需要添加的ram区域超过直接映射区域，那么舍弃掉超过直接映射区域的内存 */
            last_pfn = max_low_pfn;

        if (last_pfn <= curr_pfn) /* 防止出现值溢出的情况 */
            continue;

        size = last_pfn - curr_pfn; /* 得到要添加的ram区域的页面数量 */
        /* 将可用RAM内存区域注册到bootmem分配器中，也就是位图清0（位图在初始化时全部置1了），以供内核在早期阶段使用 */
        free_bootmem(PFN_PHYS(curr_pfn), PFN_PHYS(size));
    }

    /* 将内核映像与引导内存位图自身占用内存保留 */
    reserve_bootmem(HIGH_MEMORY, (PFN_PHYS(start_pfn) + bootmap_size + PAGE_SIZE - 1) - (HIGH_MEMORY));

    /* 保留物理页0（这通常是因为物理地址0保存一些与引导以及BIOS本身有关的信息。
    实际上，物理地址0附近的内存经常被标记为E820_RESERVED，但由于其特殊性，Linux内核选择明确地保留它 */
    reserve_bootmem(0, PAGE_SIZE);

    /* 来进一步完善页面映射机制，并建立起内存页面管理机制。完成管理节点内存的pglist初始化 */
    paging_init();

    /* 探测系统中存在的ROM，并将其放入rom_resources对应的resource空位中，
    然后添加进入iomem_resources资源树中 */
    probe_roms();

    /* 遍历e820映射表，将其对应的内存区域信息添加进入iomem_resource中，
    其中0xf0000开始0x10000大小的区域已经在prob_roms中添加过了，这部分实际会添加失败 */
    for (i = 0; i < e820.nr_map; i++)
    {
        struct resource *res; /* 用于指向要设定的resource结构体 */
        /* 检查当前e820映射条目的地址加上大小是否超过了4GB的界限 */
        if (e820.map[i].addr + e820.map[i].size > 0x100000000ULL)
            continue;
        res = alloc_bootmem_low(sizeof(struct resource)); /* 为一个新的资源结构分配内存 */
        switch (e820.map[i].type)                         /* 根据e820映射条目的类型来设置资源的名称 */
        {
        /* e820条目的类型包括RAM、ACPI表、ACPI非易失性存储等 */
        case E820_RAM:
            res->name = "System RAM";
            break;
        case E820_ACPI:
            res->name = "ACPI Tables";
            break;
        case E820_NVS:
            res->name = "ACPI Non-volatile Storage";
            break;
        default:
            res->name = "reserved";
        }
        res->start = e820.map[i].addr; /* 设置资源的起始 */
        /* 设置资源的结束地址, 结束地址是起始地址加上大小减一，因为地址是从0开始计数的 */
        res->end = res->start + e820.map[i].size - 1;
        res->flags = IORESOURCE_MEM | IORESOURCE_BUSY; /* 标记为内存资源，并且是正在使用的 */
        request_resource(&iomem_resource, res);        /* 将其注册到系统的iomem_resource资源树中 */
        if (e820.map[i].type == E820_RAM)              /* 如果当前条目是RAM类型 */
        {

            /* 将其注册为代码资源和数据资源, 是为了标记内核代码和数据所在的内存区域。
            因为我们不知道哪块RAM包含了内核数据，更精确的定位需要后面进行 */
            request_resource(res, &code_resource);
            request_resource(res, &data_resource);
        }
    }
    /* 将显卡ram加入到iomem_resource树中 */
    request_resource(&iomem_resource, &vram_resource);

    /* 将所有的i[345]86机器会用到的io资源添加进入ioport_resource树中 */
    for (i = 0; i < STANDARD_IO_RESOURCES; i++)
        request_resource(&ioport_resource, standard_io_resources + i);
}

/* 用于表示每个cpu是否已经被初始化的位图 */
static unsigned long cpu_initialized __initdata = 0;

/*初始化每个 CPU 的状态。有些数据在引导过程中已经自然初始化了，
比如 GDT（全局描述符表）和 IDT（中断描述符表）。尽管如此，我们还是重新加载它们，
这个函数充当一个“CPU 状态屏障”，每个cpu都要执行。
包括：重新加载ldtr，idtr，清楚eflags中的NT位，swapper的task_struct的active_mm指向init_mm，
cpu的tss的esp 0 填入当前任务task_struct的thread.esp0，在GDT表中设置指向TSS的描述符，
并让TSS空闲，加载TR寄存器的值，加载LDTR指向LDT表，清除调试寄存器，当前任务的task_struct中
表示浮点使用与数学协处理器的字段清空，最后设置 CR0 寄存器中的任务切换标志（TS标志）*/
void __init cpu_init(void)
{
    int nr = smp_processor_id();          /* 获取当前 CPU 的编号 */
    struct tss_struct *t = &init_tss[nr]; /* 指向当前cpu的tss，其已初始化 */

    /* 检查当前 CPU 是否已经初始化 */
    if (test_and_set_bit(nr, &cpu_initialized))
    {
        /* 如果进入，说明该cpu已经被初始化，这是有问题的！ */
        printk("CPU#%d already initialized!\n", nr); /* 打印信息，提醒cpu已经被初始化 */
        for (;;)                                     /* 进入一个无限循环 */
            __sti();                                 /* 同时启用中断，所以能够参与调度 */
    }
    printk("Initializing CPU#%d\n", nr);

    /* 重新加载ldt，gdt_descr定义在 */
    __asm__ __volatile__("lgdt %0" : "=m"(gdt_descr));

    /* 重新加载idt，idt_descr定义在 */
    __asm__ __volatile__("lidt %0" : "=m"(idt_descr));

    /* 清除了标志寄存器中的 NT 位，它用于控制任务的嵌套 */
    __asm__("pushfl ; andl $0xffffbfff,(%esp) ; popfl");

    atomic_inc(&init_mm.mm_count); /* 增加 init_mm的引用计数 */
    /* 将当前正在运行的进程的活动内存管理结构设置为 init_mm,
    其实swapper 的 task_struct 的 active_mm 早就被指向了init_mm*/
    current->active_mm = &init_mm;
    if (current->mm) /* 检查当前进程的内存管理结构是否为非空，非空代表错误 */
        BUG();
    enter_lazy_tlb(&init_mm, current, nr); /* 用于处理转换后备缓冲器（TLB）的懒惰更新 */
    /* 设置 TSS 结构中的 esp0 字段，它是内核栈指针。当 CPU 从用户模式切换到内核模式时，会使用这个指针 */
    t->esp0 = current->thread.esp0;
    /* 在全局描述符表（GDT）中设置 TSS 描述符。这个描述符告诉 CPU TSS 的位置和属性 */
    set_tss_desc(nr, t);
    /* 它清除了tss对应的描述符的“忙”位，表示这个 TSS 现在是空闲的 */
    gdt_table[__TSS(nr)].b &= 0xfffffdff;
    load_TR(nr);        /* 加载任务寄存器（TR）的值。任务寄存器用于存储当前 TSS 的选择子 */
    load_LDT(&init_mm); /* 加载内核的LDT表到LDTR中 */

/* Clear Debug，用于清除调试寄存器。CD(1)会被替换成"movl %0,%%db" "1"，
然后拼接成"movl %0,%%db1" */
#define CD(register) __asm__("movl %0,%%db" #register ::"r"(0));
    /* 清除所有六个调试寄存器（db0, db1, db2, db3, db6, db7），没有 db4 和 db5*/
    CD(0);
    CD(1);
    CD(2);
    CD(3); /* 没有 db4 和 db5 */
    ;
    CD(6);
    CD(7);

#undef CD

    /* 强制 FPU 初始化 */
    current->flags &= ~PF_USEDFPU; /* 清除 PF_USEDFPU 标志 */
    current->used_math = 0;        /* 表示当前进程没有使用过数学协处理器 */
    stts();                        /* 设置 CR0 寄存器中的任务切换标志 */
}