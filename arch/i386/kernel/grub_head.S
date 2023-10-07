                                            /* 符合Multiboot规范的 OS 映象需要这样一个 magic Multiboot 头, 只有用这个开头, grub才能引导内核映像
                                            ; Multiboot 头的分布必须如下表所示：
                                            ; ----------------------------------------------------------
                                            ; 偏移量  类型  域名        备注
                                            ;
                                            ;   0     u32   magic       必需
                                            ;   4     u32   flags       必需
                                            ;   8     u32   checksum    必需
                                            ;
                                            ; 我们只使用到这些就够了，更多的详细说明请参阅 GNU 相关文档
                                            ;-----------------------------------------------------------
                                            */                                            
MBOOT_HEADER_MAGIC = 0x1BADB002 	        // Multiboot 魔数，由规范决定的

MBOOT_MEM_INFO = 1 << 1    	                //flags 1 号位表示告诉GRUB把内存空间的信息包含在Multiboot信息结构中

MBOOT_HEADER_FLAGS = MBOOT_MEM_INFO         //定义flags域

                                            // 域checksum是一个32位的无符号值, 当与其他的magic域(也就是magic和flags)相加时，
                                            // 要求其结果必须是32位的无符号值 0 (即magic + flags + checksum = 0)
MBOOT_CHECKSUM = -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

.section .data.grub_head_data               //定义一个数据段, 名字叫做grub_head_data, 用来设置符合 Multiboot 规范的开头
.long MBOOT_HEADER_MAGIC                    // GRUB 会通过这个魔数判断该映像是否支持
.long MBOOT_HEADER_FLAGS                    // GRUB 的一些加载时选项
.long MBOOT_CHECKSUM                        // 检测数值

.code32                                     //代码以 32-bit 的方式编译  	                                
.section .text.grub_head_code  	            //代码段, 并且这个代码段命名为grub_head_code
.globl grub_head_to_startup_32 		        // 此处提供该声明给 ld 链接器(链接脚本使用了这个符号来作为链接后的内核映像入口地址)
grub_head_to_startup_32:
    ljmp $0x10, $0x100013                   //通过长跳转进入startup_32, rip的值确定是查看kernel.map中的.text.init
                                            //这么做能既使用grub引导, 
                                            //又能让linux2.4内核代码(从startup_32开始)编译地址在3G以上(因为Linux2.4源码就是这样做的), 
                                            //还能在现在没有开启页机制的情况下进入startup_32(如果将内核映像入口指定为startup_32, 根本进入执行。 
                                            //虽然此后编译地址在3G以上, 但是由于startup_32就在物理地址0x1000013,
                                            // 所以能进入并执行。只要之后不call(可以jmp)符号, cpu寻址方式将会一直是
                                            //rip + 偏移，可以一直这么运行下去) 
