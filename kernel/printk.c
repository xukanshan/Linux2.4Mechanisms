/* 这个头文件定义了va_list宏，来源于c语言，Linux2.4也是这么处理的
printk.c包含了mm.h，其包含了schde.h，其包含了kernel.h，其包含了stdarg.h */

#include <linux/mm.h>
#include <linux/smp_lock.h>
#include <linux/init.h>

#define LOG_BUF_LEN (16384) /* 日志缓冲长度 */

#define LOG_BUF_MASK (LOG_BUF_LEN - 1) /* 14个2进制1，用于实现循环写入的掩码，你可以尝试用大于这个数来值来与这个值会得到什么 */

/* 默认日志等级，警告级别 */
#define DEFAULT_MESSAGE_LOGLEVEL 4

/* 输出到控制台的日志等级 */
#define DEFAULT_CONSOLE_LOGLEVEL 7

int console_loglevel = DEFAULT_CONSOLE_LOGLEVEL;         /* 输出到控制台的日志等级 */
int default_message_loglevel = DEFAULT_MESSAGE_LOGLEVEL; /* 设定默认默认日志等级 */

static char buf[1024];             /* 信息临时存储缓冲 */
static char log_buf[LOG_BUF_LEN];  /* 日志缓冲区 */
static unsigned long log_start;    /* 记录日志缓冲区内信息的起始 */
unsigned long log_size;            /* 日志缓冲区内现有信息的大小 */
static unsigned long logged_chars; /* 记录日志缓冲区内已经写入的字符数目 */
// struct console *console_drivers;   /* 指向当前的控制台，暂未实现 */

/* 这个函数的核心是将格式化的日志信息写入一个循环缓冲区，并且如果有控制台的话，就输出到控制台中。 */
asmlinkage int printk(const char *fmt, ...)
{
    va_list args;                      /* 用于处理可变参数，va_list 由c语言提供的头文件stdarg.h定义，实际类型就是char* */
    int i;                             /* 用于存储格式化字符串的长度 */
    char *msg, *p, *buf_end;           /* 定义三个字符指针，用于操作和追踪字符串 */
    int line_feed;                     /* 用于标记行结束 */
    static signed char msg_level = -1; /* 用于存储日志等级 */
    long flags;                        /* 用于存储旋转锁状态 */
    // spin_lock_irqsave(&console_lock, flags); /*  获取旋转锁console_lock，并保存中断状态到flags，暂未实现 */

    va_start(args, fmt);                /*  初始化args以指向fmt，其实现等同于 va_start(ap, v) ap = (va_list)&v */
    i = vsprintf(buf + 3, fmt, args);   /* 使用vsprintf将格式化的输出写入buf数组的第四个位置起的部分，前三个字符用于存储消息等级 */
    buf_end = buf + 3 + i;              /* 设置buf_end为格式化字符串的末尾 */
    va_end(args);                       /* 清理args，其实现等同于 va_end(ap) ap = NULL */
    for (p = buf + 3; p < buf_end; p++) /* 这部分代码遍历buf中的每个字符，根据日志等级和格式处理每一行日志 */
    {
        msg = p;           /*  将msg指针设置为当前遍历的字符 */
        if (msg_level < 0) /* 如果日志等级未设置（msg_level < 0），则尝试从消息中解析等级 */
        {
            /* 如果消息的开始不符合日志等级的格式（例如<4>，则将其设置为默认日志等级 */
            if (p[0] != '<' || p[1] < '0' || p[1] > '7' || p[2] != '>')
            {
                p -= 3;
                p[0] = '<';
                p[1] = default_message_loglevel + '0';
                p[2] = '>';
            }
            else /* 否则，跳过这三个字符 */
                msg += 3;
            msg_level = p[1] - '0'; /* 解析并设置消息的日志等级 */
        }
        line_feed = 0;           /* 初始化行结束标志。 */
        for (; p < buf_end; p++) /* 继续遍历，直到buf_end */
        {
            /* 将字符复制到循环日志缓冲区限。由于LOG_BUF_MASK是缓冲区大小减1，
            这里的与操作有效地将索引“回绕”到缓冲区的开始，如果它超过了缓冲区的末端。这是实现循环缓冲区的常见技术 */
            log_buf[(log_start + log_size) & LOG_BUF_MASK] = *p;
            if (log_size < LOG_BUF_LEN) /* 更新日志缓冲区的大小和开始位置 */
                log_size++;
            else
                log_start++;

            logged_chars++; /* 增加记录的字符数 */
            if (*p == '\n') /* 如果遇到换行符，设置行结束标志并跳出循环 */
            {
                line_feed = 1;
                break;
            }
        }
        /* 这里是判断是否输出到控制台，如果日志等级低于console_loglevel且存在控制台驱动，则输出到所有启用的控制台 */
        // if (msg_level < console_loglevel && console_drivers) /* 这里需要控制台，但暂未实现，因为还没有初始化 */
        // {
        //     struct console *c = console_drivers; /* 获取控制台驱动列表的头部 */
        //     while (c)                            /* 遍历所有控制台驱动 */
        //     {
        //         if ((c->flags & CON_ENABLED) && c->write) /* 如果控制台驱动启用并且有写函数，就调用它来输出消息 */
        //             c->write(c, msg, p - msg + line_feed);
        //         c = c->next; /* 移动到下一个控制台驱动 */
        //     }
        // }
        print_str(msg);
        if (line_feed) /* 如果处理了一整行（遇到了换行符），则重置日志等级，以便下一行可以重新解析等级 */
            msg_level = -1;
    }
    // spin_unlock_irqrestore(&console_lock, flags);   /* 释放旋转锁，并恢复之前保存的中断状态。暂未实现 */
    // wake_up_interruptible(&log_wait); /* 唤醒等待日志消息的进程，暂未实现 */
    return i; /* 返回写入的字符数 */
}