#ifndef _LINUX_STRING_H
#define _LINUX_STRING_H

#include <asm-i386/posix_types.h>

extern __kernel_size_t strnlen(const char *, __kernel_size_t);
extern void *memset(void *, int, __kernel_size_t);

#endif /* _LINUX_STRING_H */