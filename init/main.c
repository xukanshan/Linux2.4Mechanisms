#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/smp_lock.h>
#include <linux/bootmem.h>
#include <asm-i386/io.h>


/* arch/i386/kernel/setup.c */
extern void setup_arch(char **);

asmlinkage void __init start_kernel(void)
{
    char *command_line;
    lock_kernel();             /* 只让主cpu进行后续初始化工作，其他cpu就被锁住，但是我们这是个用于单核系统，这函数什么都没有做 */
    setup_arch(&command_line); /* 进行体系化初始工作，主要是内存资源管理与寻址相关 */
    trap_init();               /* 设定intel cpu的保留中断与系统调用中断，为每个cpu设置必要的状态和环境 */

    while (1)
        ;
}