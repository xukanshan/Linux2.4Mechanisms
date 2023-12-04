#include <linux/linkage.h>
#include <asm-i386/segment.h>

int_msg:
	.asciz "Please perfect the interruption exit!\n"
    ALIGN
ENTRY(ret_from_intr)                        /* 暂未实现中断退出 */
    pushl $int_msg
	call SYMBOL_NAME(printk)
L6:
	jmp L6