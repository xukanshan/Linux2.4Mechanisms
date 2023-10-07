	  				                        //此时经过grub引导,中断是关闭状态
		                                    //当grub引导结束, 会返回一个存储很多信息的结构体, 其地址会放入ebx中
                                            //grub引导会设定临时使用的内核代码段(GDT偏移1)、数据段段描述符(GDT偏移2), 并且设定好了所有段寄存器
.code32  	                                
.section .text.init
startup_32:
                                           
	movl $0xB8000, %eax                     //显示两个字符xx, 测试能够工作。
	movl $0x07580758, (%eax)            
stop:
	hlt 			                        //待机, 什么也不做, 可以降低 CPU 功耗
	jmp stop