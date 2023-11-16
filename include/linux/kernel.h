#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

#include <stdarg.h>
#include <linux/linkage.h>

#define KERN_WARNING "<4>" /* 警告级别 */

/* ((format (printf, 1, 2)))属性。这个属性用于告诉编译器，该函数接受类似于printf、scanf、strftime或strfmon等标准库函数的格式字符串。
这可以让编译器检查函数调用中的格式字符串与提供的参数是否匹配，从而避免一些常见的错误。
(printf, 1, 2): 这部分指定了三个参数：
printf表示这个函数接受的格式字符串与printf函数的格式相同。
第一个1表示格式字符串在函数参数列表中的位置（在printk的情况下是第一个参数）。
第二个2表示格式字符串中第一个可变参数在参数列表中的位置（在这里是第二个参数，即...表示的部分）。 */
asmlinkage int printk(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
extern int vsprintf(char *buf, const char *, va_list);
extern void print_str(char *str);

#endif /* _LINUX_KERNEL_H */