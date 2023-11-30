#ifndef _LINUX_STRING_H
#define _LINUX_STRING_H



/* 定义在lib/string.c中 */
extern __kernel_size_t strnlen(const char *, __kernel_size_t);

/* 定义在lib/string.c中 */
extern void *memset(void *, int, __kernel_size_t);

#endif /* _LINUX_STRING_H */