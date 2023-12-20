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
    PROVIDE( _start = . );                      /* 提供一个全局符号_start(如果该符号没有定义)，其值为当前位置计数器的值 */
    .text.grub_head :                           /* 定义一个输出文件中的代码段.grub_head段，这个段将会从1MB开始 */
    {             
		*(.grub_head)                           /* 将输入文件中的所有名字有.grub_head的段的内容合并到输出文件中的一个.grub_head段中。*/
                                                /* *是一个通配符，它表示所有输入文件 */
	}
    . = ALIGN(4096);                            /* 当前位置向前增长到4096的倍数，所以下一个段就是4KB对齐了，相当于我们预留了4KB空间给grub引导头*/
    . += 0xC0000000;                            /* 当前位置计数器增加0xC0000000, 仿照linux2.4, 内核编译地址在3G以上 */
    _text = .;                                  /* 将当前位置赋值给 _text（无论有没有被定义） 标签，表示代码段起始 */ 
    .text.startup : AT(ADDR(.text.startup) - 0xC0000000)    /* 指定加载地址为链接地址 - 3G, 否则加载地址就是链接地址 */
	{                                           /* 这个段需要4K起始对齐，因为启动阶段临时页表在段内，它们依靠段内4K偏移来做到4K起始对齐 */
		*(.startup)                       
	}
    .text : AT(ADDR(.text) - 0xC0000000)
    {
        *(.text)
    }
    _etext = .;			                        /* 代码段结束，我也不知道为什么linux2.4放在了这个位置，没有包含诸如.text.init这样的代码段
                                                这个也是数据段的开始 */
	.data : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data)
		*(.rodata)
	}
    _edata = .;			                        /* 数据段结束，我也不知道为什么linux2.4放在了这个位置，没有包含诸如.data.init_task这样的数据段 */
    .data.init_task : AT(ADDR(.data.init_task) - 0xC0000000)    
    {
        . = ALIGN(8192);                        /* init_task_union作为swapper的task_struct，必须8K对齐起始 */
        *(.data.init_task)           
    }

    . = ALIGN(4096);		                    
    __init_begin = .;                           /* 以下是内核初始化代码段与数据段，直接从linux2.4链接脚本粘贴过来的，但是加入了加载地址指定 */
    .text.init : AT(ADDR(.text.init) - 0xC0000000)
    { 
        *(.text.init) 
    }
    .data.init : AT(ADDR(.data.init) - 0xC0000000)
    { 
        *(.data.init) 
    }
    . = ALIGN(16);
    __setup_start = .;
    .setup.init : AT(ADDR(.setup.init) - 0xC0000000)
    { 
        *(.setup.init) 
    }
    __setup_end = .;
    __initcall_start = .;
    .initcall.init : AT(ADDR(.data.init_task) - 0xC0000000)
    { 
        *(.initcall.init) 
    }
    __initcall_end = .;
    . = ALIGN(4096);
    __init_end = .;

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

