#include <linux/linkage.h>
#include <linux/smp_lock.h>
#include <linux/init.h>

/* arch/i386/kernel/setup.c */
extern void setup_arch(char **);

asmlinkage void __init start_kernel(void)
{
    char *command_line;
    lock_kernel(); /* 只让主cpu进行后续初始化工作，其他cpu就被锁住，但是我们这是个用于单核系统，这函数什么都没有做 */
    setup_arch(&command_line);
    while (1)
        ;
}