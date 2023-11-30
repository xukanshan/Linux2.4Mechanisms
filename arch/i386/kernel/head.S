#include <linux/threads.h>
#include <linux/linkage.h>
#include <asm-i386/segment.h>
#include <asm-i386/page.h>					/* 引入__PAGE_OFFSET */
#include <asm-i386/pgtable.h>				/* 引入BOOT_USER_PGD_PTRS 和 BOOT_KERNEL_PGD_PTRS */
#include <asm-i386/desc.h>
                                         	/* 符合Multiboot规范的 OS 映象需要这样一个 magic Multiboot 头, 
                                            只有用这个开头, grub才能引导内核映像
                                            Multiboot 头的分布必须如下表所示：
                                            ----------------------------------------------------------
                                            偏移量  类型  域名        备注
                                            
                                              0     u32   magic       必需
                                              4     u32   flags       必需
                                              8     u32   checksum    必需
                                            
                                            我们只使用到这些就够了，更多的详细说明请参阅 GNU 相关文档
                                            ---------------------------------------------------------- */                                            
MBOOT_HEADER_MAGIC = 0x1BADB002 	        /* Multiboot 魔数，由规范决定的 */

											/* 设定flags以决定返回的multiboot结构体信息包含哪些信息，设定规则查看：
											https://blog.csdn.net/weixin_43836778/article/details/123234888 */
MBOOT_MEM_INFO = 1 << 1    	                /* flags 1 号位表示告诉GRUB把内存空间的信息包含在Multiboot信息结构中 */

MBOOT_HEADER_FLAGS = MBOOT_MEM_INFO         /* 定义flags域 */

                                            /* 域checksum是一个32位的无符号值, 当与其他的magic域(也就是magic和flags)相加时，
                                               要求其结果必须是32位的无符号值 0 (即magic + flags + checksum = 0) */
MBOOT_CHECKSUM = -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

.section .text.grub_head               		/* 定义一个代码, 名字叫做grub_head, 用来设置符合 Multiboot 规范的开头以及一些代码
											照理说mulbitoob开头与代码应该分属数据段与代码段，但是Linux2.4本身就有大量数据位于代码段中，
											所以这里直接模仿 */
.global stext
.global _stext
stext:                                
_stext:  	                                /* 仿照Linux，用_stext与stext记录整个内核的开始 */  
.long MBOOT_HEADER_MAGIC                    /* GRUB 会通过这个魔数判断该映像是否支持 */
.long MBOOT_HEADER_FLAGS                    /* GRUB 的一些加载时选项 */
.long MBOOT_CHECKSUM                        /* 检测数值 */
                                 
ENTRY(grub_head) 		                    /* 此处提供该声明给 ld 链接器(链接脚本使用了这个符号来作为链接后的内核映像入口地址) */
    ljmp $0x10, $0x101000                   /* 通过长跳转进入startup_32, rip的值确定是查看kernel.map中的.text的加载地址，实际是由我们的
                                            kernel.ld决定的 */
                                            /* 这么做能既使用grub引导, 
                                            又能让linux2.4内核代码(从startup_32开始)编译地址在3G以上(因为Linux2.4源码就是这样做的), 
                                            还能在现在没有开启页机制的情况下进入startup_32
                                            如果将内核映像入口直接指定为startup_32, 根本无法进入执行。
                                            虽然此后编译地址在3G以上, 但是由于startup_32就在物理地址0x10000xx,
                                            所以能进入并执行。只要之后不call(可以jmp 1f这样)符号, cpu寻址方式将会一直是
                                            rip + 偏移，可以一直这么运行下去  */
                                            
/* -----------------------------------------以上就是能让内核映像被grub引导的文件头---------------------------------------------------- */
											/*
											* grub启动后的机器状态：
											* 参考 Multiboot 规范中的 Machine state
											* 启动后，在32位内核进入点，机器状态如下：
											*   1. CS 指向基地址为 0x00000000，限长为4G – 1的代码段描述符。
											*   2. DS，SS，ES，FS 和 GS 指向基地址为0x00000000，限长为4G – 1的数据段描述符。
											*   3. A20 地址线已经打开。
											*   4. 页机制被禁止。
											*   5. 中断被禁止。
											*   6. EAX = 0x2BADB002，魔数，说明操作系统是由符合 Multiboot 的 boot loader 加载的
											*   7. 系统信息和启动信息块的线性地址保存在 EBX中（相当于一个指针）。
											*/
                                            /* grub引导会设定临时使用的内核代码段(GDT偏移2)、数据段段描述符(GDT偏移3) */
.section .text
startup_32:

                                            /* 将pg0至0页间的 8kb做成两个内核使用的临时页表，映射0-8m物理地址，为开启页机制做准备*/
    movl $pg0-__PAGE_OFFSET, %edi           /* 让edi中存上pg0的地址 */
    movl $007, %eax                         /* 设定页表项为 0地址 + PRESENT+RW+USER */
2:	stosl                                   /* Store String Long， 将 eax 的内容存储至 es:edi */
	add $0x1000, %eax                       /* eax中的页表项地址+4kb */
	cmp $empty_zero_page-__PAGE_OFFSET, %edi    /* 比较页表项的地址是否到了0页 */
	jne 2b                                  /* 没有到0页说明不是最后一个页表项，那么就继续循环设定 */

3:                                          /* 开启页机制 */
	movl $swapper_pg_dir-__PAGE_OFFSET,%eax
	movl %eax,%cr3		                    /* 让cr3指向页目录表 */
	movl %cr0,%eax                          /* 设定cr0寄存器的pg位，开启页机制 */
	orl $0x80000000,%eax
	movl %eax,%cr0

	jmp 1f			                        /* 用jmp抛弃流水线预取指令 */
1:
	movl $1f,%eax
	jmp *%eax		                        /* 通过直接取地址跳转，也就是现在eip的值为3G以上来正式用虚拟地址寻址 */
1:
    lss stack_start, %esp                   /* 加载stack_start处准备好的esp与ss，此指令会将从内存地址 stack_start 处取得的 32 位值加载到 esp，
                                            并将从 stack_start + 4 地址处取得的值加载到 ss。为创建第一个进程做准备 */
	
    xorl %eax, %eax                         /* 清空代码中的bss段（未初始化的全局变量），防止意外发生 */
	movl $SYMBOL_NAME(__bss_start), %edi    /* __bss_start由链接脚本提供 */
	movl $SYMBOL_NAME(_end), %ecx           /* _end由链接脚本提供 */
	subl %edi, %ecx
	rep
	stosb                                            

    call setup_idt                          /* 初始化idt表 */

	pushl $0								/* 初始化eflags */
	popfl           						/* 执行完毕之后，eflags并不会是0，因为其第二位固定为1，所以eflags整个的值是2 */
    
	lgdt gdt_descr							/* 重新加载gdtr。重新加载的意义在于，Linux2.4原来的gdt表在setup中，而这部分随着内核启动会被抛弃！
											所以要使用新的在内核中的gdt表。对于我们来说，原有的gdt表由grub提供，重新加载是为了让我们的内核来控制gdt表，
											使之随时可修改，更可控 */

	lidt idt_descr							/* 加载idtr */
	
	ljmp $(__KERNEL_CS), $1f				/* 用这种方式重新加载cs段寄存器的值 */
1:	movl $(__KERNEL_DS), %eax				/* 重新加载段寄存器的值 */
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	lss stack_start, %esp                   /* 用这种方式重新加载ss段寄存器 */

	xorl %eax, %eax
	lldt %ax								/* linux根本没有用ldt，所以直接加载0值到ldtr中去 */

	add $__PAGE_OFFSET, %ebx				/* ebx中存储着grub引导结束后返回的multiboot_t结构体地址，
											且地址是开启段机制后的线性地址（3G以下），为了优雅，使内核的一切都在3G以上，所以这里+3G */
	movl %ebx, (grub_multiboot_ptr - __PAGE_OFFSET)	/* 将 ebx 中存储的指针存入 grub_multiboot_ptr 变量，注意这个变量的地址在3G以上，
											现在还没有开启页机制，所以使用这个变量地址要-3G，而且这个赋值行为一定要在清空bss段之后*/

	call SYMBOL_NAME(start_kernel)			/* 前期基本准备工作已经完成，现在进入内核进步一初始化 */

L6:
	jmp L6									/* 永远不会回到这句 */
	
/* 设置idt表内所有256个中断门描述符指向同一个函数, 但现在并不会加载idt表，
只有在分页直接打开后，并且内核运行在3G以上以后才会加载。并且也不会去开中断*/
setup_idt:
                                            /* 开始拼凑中断门描述符 */
	lea ignore_int, %edx                    /* 准备默认中断处理函数的地址 */
	movl $(__KERNEL_CS << 16), %eax         /* 准备代码段的选择子 */
	movw %dx, %ax		                    /* 现在eax中就是中断门描述符的低4字节，包括代码段选择子（高2字节）与函数地址（低2字节） */
	movw $0x8E00, %dx	                    /* dx现在是中断门描述符高4字节的低2字节，存在，dpl = 0，类型111。edx高16位已经是函数地址的高2字节了 */
	lea SYMBOL_NAME(idt_table), %edi        /* 加载idt表的地址, 定义在trap.c中 */
	mov $256, %ecx                          /* 设定循环次数 */
rp_sidt:
	movl %eax, (%edi)                       /* 设定中断门描述符的低4字节 */
	movl %edx, 4(%edi)                      /* 设定中断门描述符的高4字节 */
	addl $8, %edi                           /* 设定下一个要设定的中断门描述符地址，一个中断门描述符是8B */
	dec %ecx                                /* 循环次数-1 */
	jne rp_sidt                             /* 没有循环完就继续循环 */
	ret

ENTRY(stack_start)                          
	.long SYMBOL_NAME(init_task_union)+8192 /* esp为init_task_union（swapper的pcb结构体） + 8K的位置 */
	.long __KERNEL_DS                       /* ss为内核数据段选择子 */
                                            /*尽管我们没有包含头文件或者链接脚本声明符号来显式地声明init_task_union，
                                            但在链接阶段，链接器会尝试解析所有未解析的符号引用，查找所有的目标文件和库来解决这些引用。*/

/* 以下是默认的中断处理函数，就是保存现场，然后压入字符串地址后去call prink打印出来 */
int_msg:
	.asciz "Unknown interrupt\n"
	ALIGN
ignore_int:
	cld
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl %es
	pushl %ds

	movl $(__KERNEL_DS), %eax
	movl %eax, %ds
	movl %eax, %es
	pushl $int_msg
    call SYMBOL_NAME(printk)              

	popl %eax                               /* 丢弃压入的字符串地址 */

	popl %ds
	popl %es
	popl %edx
	popl %ecx
	popl %eax
	iret

#define IDT_ENTRIES	256						/* 定义idt表项的数量 */
#define GDT_ENTRIES	(__TSS(NR_CPUS))		/* 定义gdt表项的数量，为12个基本段描述符 + 每个cpu 4个段描述符 */

.globl SYMBOL_NAME(idt)						/* 将表示idt表地址的标号全局化 */
.globl SYMBOL_NAME(gdt)						/* 将表示gdt表地址的标号全局化 */

ALIGN										
	.word 0									/* 填充2字节0作为分割 */
idt_descr:									/* 定义加载进入idtr的数据 */
	.word IDT_ENTRIES * 8 - 1				/* 前2字节，idt表界限，为idt表大小-1 */
SYMBOL_NAME(idt):
	.long SYMBOL_NAME(idt_table)			/* 后4字节，idt表的位置, idt表定义在trap.c中 */
	
	.word 0									/* 填充2字节0作为分割 */
gdt_descr:									/* 定义加载进入gdtr的数据 */
	.word GDT_ENTRIES * 8 - 1				/* 前2字节，gdt表界限，为gdt表大小-1 */
SYMBOL_NAME(gdt):							
	.long SYMBOL_NAME(gdt_table)			/* 后4字节，gdt表的位置，gdt表定义在此文件中 */



                                            /* 由于startup_32会被放到101000位置, 所以swapper_pg_dir(内核用页目录表)会被放到102000位置 */
.org 0x1000                                 /* 其实不应该.text内跟数据，但这种做法是为对内存布局精确控制 */
ENTRY(swapper_pg_dir)                       /* 设定内核用的页目录表，并利用预先设定数据的方式，让项0与1，项768与769指向pg0与pg1 */
	.long 0x00103007
	.long 0x00104007
	.fill BOOT_USER_PGD_PTRS-2,4,0          /* 用户空间剩下页目录项全部设定为0 */
	.long 0x00103007
	.long 0x00104007
	.fill BOOT_KERNEL_PGD_PTRS-2,4,0        /* 内核空间剩下页目录项全部设定为0 */

                                                      
.org 0x2000                                 /* 实际位置是103000 */
ENTRY(pg0)                                  /* 内核使用的临时页表，映射0-4M物理地址 */                       

.org 0x3000                                 /* 实际位置是104000 */
ENTRY(pg1)                                  /* 内核使用的临时页表，映射4-8M物理地址 */

.org 0x4000                                 /* 全0页 */        
ENTRY(empty_zero_page)

.data										/* 数据段从这里开始之前很多属于数据段的内容放到了代码段，是为了对内存布局精确控制 */
ALIGN
ENTRY(gdt_table)							/* gdt表，前12个是基本段描述符，后面是每个cpu都有的tss与ldt段描述符与2个空的段描述符 */
	.quad 0x0000000000000000				/* 防止段寄存器未设置时选用此段描述符，所以不设置，选用时会报错 */
	.quad 0x0000000000000000				/* 不知道Linux2.4为什么不用这个 */
	.quad 0x00cf9a000000ffff				/* 0x10 内核 4GB 代码 起始 0x00000000 */
	.quad 0x00cf92000000ffff				/* 0x18 内核 4GB 数据 起始 0x00000000 */
	.quad 0x00cffa000000ffff				/* 0x20 用户 4GB 代码 起始 0x00000000 */
	.quad 0x00cff2000000ffff				/* 0x28 用户 4GB 数据 起始 0x00000000 */
	.quad 0x00c0920b80000007				/* 0x30 显存段用的段描述符，为了在早期实现printk而暂时设定，原是全0 */
	.quad 0x0000000000000000				/* 未使用 */
											/*以下这4个在Linux2.4中是APM段描述符
											* APM段是指在高级电源管理（Advanced Power Management，简称APM）规范中定义的一段内存，
											* 这段内存用于BIOS和操作系统之间的通信，以实现电源管理功能。
											* 高级电源管理（APM）是一个早期的BIOS接口标准，
											* 用于使操作系统能够与系统固件（BIOS）一起协调电源使用和电源状态的变化
											* 随着技术的进步，APM标准已经被现代的电源管理接口，如ACPI（高级配置与电源接口）所取代。
											* ACPI提供了更为复杂和灵活的电源管理能力，与现代硬件的兼容性也更好。
											* 我这里留下它们占用的空间，但是值清空了，留下他们的空间是为了以防后续需要改动太多（比如宏__FIRST_TSS_ENTRY的值）。
											*/
	.quad 0x0000000000000000				/* 原值为0x0040920000000000 0x40 APM设置 用于处理不良BIOS的问题 */
	.quad 0x0000000000000000				/* 原值为0x00409a0000000000 0x48 APM代码段 */
	.quad 0x0000000000000000				/* 原值为0x00009a0000000000 0x50 APM 16位代码段（16位 */
	.quad 0x0000000000000000				/* 原值为0x0040920000000000 0x58 APM数据段 */

	.fill NR_CPUS * 4, 8, 0					/* 每个cpu都需要有自己的TSS与LDT段描述符，同时还预留了两个空的段描述符给他们 */