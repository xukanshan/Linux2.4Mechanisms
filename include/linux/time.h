#ifndef _LINUX_TIME_H
#define _LINUX_TIME_H

#include <asm-i386/param.h>

/* 该结构体用于记录自Unix纪元（1970年1月1日）以来的秒数和微秒数部分，
这里的微秒数部分是在秒基础上多出来的，不是将秒换成微秒来表达 */
struct timeval
{
    time_t tv_sec;
    suseconds_t tv_usec;
};

#endif /* _LINUX_TIME_H */