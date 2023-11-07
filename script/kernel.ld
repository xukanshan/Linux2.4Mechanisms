                                                /*
                                                *      链接脚本是告诉计算机如何组合编译后的代码和数据。
                                                *	   首先，我们声明了程序的入口地址是符号 "grub_head_to_startup_32"，grub完成elf解析加载后，
                                                *      就会跳转这个函数执行，必须保证其编译地址与物理地址重合。
                                                *      然后，我们声明了第一个段名叫grub_head_data的数据段以及它的起始地址 0x100000（1MB）。
                                                *      对于已初始化数据段 .data 和 未初始化数据段 .bss 以及其他段采用4096的页对齐方式。
                                                *      Linux GCC 增加了额外的数据段 .rodata，这是一个只读的已初始化数据段，放置常量什么的。
                                                *      简单起见，我们把和 .data 段放在一起好了。
                                                */

ENTRY(grub_head)                                /* 设置入口点为grub_head_to_startup_32函数 */
SECTIONS                                        /* SECTIONS块定义了输出文件的段组织布局 */ 
{	
    . = 0x100000;                               /* 设置当前位置为0x100000。在链接脚本中.表示当前位置，它决定了下一个段在输出文件中的位置。*/
    PROVIDE( _start = . );                      /* 提供一个全局符号_start，其值为当前位置计数器的值 */
    .data.grub_head_data :                      /* 定义一个输出文件中的数据段.grub_head_data段，这个段将会从1MB开始 */
    {             
		*(.grub_head_data)                      /* 将输入文件中的所有名字有.grub_head_data的段的内容合并到输出文件中的一个.grub_head_data段中。*/
                                                /* *是一个通配符，它表示所有输入文件 */
	}
    .text.grub_head_code :               
	{
		*(.grub_head_code)
	}
    . = ALIGN(4096);                            /*当前位置向前增长到4096的倍数，所以下一个段就是4KB对齐了，相当于我们预留了4KB空间给grub引导头*/
    . += 0xC0000000;                            /* 当前位置计数器增加0xC0000000, 仿照linux2.4, 内核编译地址在3G以上 */
    .text : AT(ADDR(.text) - 0xC0000000)        /* 指定加载地址为链接地址 - 3G, 否则加载地址就是链接地址 */
	{
		*(.text)
		. = ALIGN(4096);                        
	}
    .data.init_task : AT(ADDR(.data.init_task) - 0xC0000000)    
    {
        *(.data.init_task)
                                                /* 这个段对应init_task_union变量，本就是8K大小，结束地址不对齐也可。 */
    }                                           /* 这里如果设定了结束地址8K对齐，那么这个这个段的起始地址也必须8K对齐，否则grub会报error: overlap detected */
                                                /* 换句话说，这个结束地址8K对齐决定了前一个段的结束地址也必须8K对齐 */
	.data : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data)
		*(.rodata)
		. = ALIGN(4096);
	}
    PROVIDE( __bss_start = . );
	.bss : AT(ADDR(.bss) - 0xC0000000)
	{
		*(.bss)
		. = ALIGN(4096);
	}
    PROVIDE( _end = . );
	                                            /* .stab和.stabstr是与调试相关的段。在UNIX和类UNIX系统中，*/
                                                /* 这两个段通常用于存储符号表(.stab)和与符号表条目关联的字符串(.stabstr)。*/
                                                /* 这是一个旧式的调试格式，特别是与GNU的gdb调试器配合使用。*/
                                                /* .stab：包含调试符号。 */
                                                /* .stabstr：包含与.stab中的符号相关联的字符串 */
    .stab : AT(ADDR(.stab) - 0xC0000000)
	{
		*(.stab)
		. = ALIGN(4096);
	}
    .stabstr : AT(ADDR(.stabstr) - 0xC0000000)
	{
		*(.stabstr)
	 	. = ALIGN(4096);
	}
	

	/DISCARD/ : { *(.comment) *(.eh_frame) }    /* 用于指定链接器应该从最终的输出文件中丢弃哪些段 */
}                                               /* .comment：这个段包含了编译器、链接器或其他工具可能添加的注释或版本信息。这些信息对于执行程序通常不是必要的 */
                                                /* .eh_frame：这个段包含异常处理信息，用于支持C++异常和其他语言特性。对于那些不使用C++异常的应用程序，这个段并不必要 */

