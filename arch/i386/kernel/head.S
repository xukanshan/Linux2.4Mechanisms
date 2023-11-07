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

MBOOT_MEM_INFO = 1 << 1    	                /* flags 1 号位表示告诉GRUB把内存空间的信息包含在Multiboot信息结构中 */

MBOOT_HEADER_FLAGS = MBOOT_MEM_INFO         /* 定义flags域 */

                                            /* 域checksum是一个32位的无符号值, 当与其他的magic域(也就是magic和flags)相加时，
                                               要求其结果必须是32位的无符号值 0 (即magic + flags + checksum = 0) */
MBOOT_CHECKSUM = -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

.section .data.grub_head_data               /* 定义一个数据段, 名字叫做grub_head_data, 用来设置符合 Multiboot 规范的开头 */
.global stext
.global _stext
stext:                                
_stext:  	                                /* 仿照Linux，用_stext与stext记录整个内核的开始 */  
.long MBOOT_HEADER_MAGIC                    /* GRUB 会通过这个魔数判断该映像是否支持 */
.long MBOOT_HEADER_FLAGS                    /* GRUB 的一些加载时选项 */
.long MBOOT_CHECKSUM                        /* 检测数值 */

.code32                                     /* 代码以 32-bit 的方式编译 */  	                                
.section .text.grub_head_code  	            /* 代码段, 并且这个代码段命名为grub_head_code */
#include <linux/linkage.h> 

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

	  				                        /* 此时经过grub引导,中断是关闭状态 */
		                                    /* 当grub引导结束, 会返回一个存储很多信息的结构体, 其地址会放入ebx中 */
                                            /* grub引导会设定临时使用的内核代码段(GDT偏移2)、数据段段描述符(GDT偏移3), 并且设定好了所有段寄存器 */
.code32
.section .text
#include <asm-i386/segment.h>
#include <asm-i386/page.h>
#include <asm-i386/pagetable.h>
startup_32:
	cld                                     /* 设定df，但没有必要，因为grub引导已经设定好 */
	movl $(__KERNEL_DS), %eax               /* 设定除了cs以外所有段寄存器为内核数据段选择子，但没有必要，因为grub引导已经设定好 */
	movl %eax, %ds                          
	movl %eax, %es                        
	movl %eax, %fs                        
	movl %eax, %gs                                      

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
    
    movl $0xB8000, %eax                     /* 显示两个字符xx, 测试能够工作。 */
    movl $0x07580758, (%eax)            
stop:
	jmp stop

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
    //call SYMBOL_NAME(printk)              /* 暂未实现printk函数 */

	popl %eax                               /* 丢弃压入的字符串地址 */

	popl %ds
	popl %es
	popl %edx
	popl %ecx
	popl %eax
	iret



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