#include <linux/types.h>

/* 用于计算字符串 s 的长度，但最多计算到 count 个字符。
这个函数的名称由来可以从其功能理解：
strnlen 是由 "string"（字符串）、"n"（表示数量或上限）和"length"（长度）组合而成的。
在标准的 strlen 函数中，如果没有遇到空字符，函数将继续读取内存，
直到找到一个空字符，这可能会导致读取超出字符串分配的内存空间，从而引发安全漏洞（如缓冲区溢出）。
相比之下，strnlen 通过限制检查的最大字符数，为处理字符串提供了更高的安全性。
这在处理不确定长度或可能未正确空字符终止的字符串时尤其重要
参数解释：
const char *s：指向待测量的字符串的指针。
size_t count：最大检查长度，即函数最多只会检查这么多字符。 */
size_t strnlen(const char *s, size_t count)
{
    const char *sc;
    /* 初始化指针 sc 为指向字符串 s 的起始位置, 在每次循环迭代中，减少 count 的值（count--）。
    检查当前 sc 指向的字符是否是空字符（\0）。如果是，停止循环。
    将 sc 向前移动一个字符位置（++sc）。 */
    for (sc = s; count-- && *sc != '\0'; ++sc)
        ;
    return sc - s; /* 返回 sc - s，即从字符串 s 的起始位置到 sc 当前位置之间的字符数量，这代表了字符串的长度 */
}

/* 将s起始，大小为count字节的区域设置成c，常用于清零或者置1，返回清空区域的起始地址 */
void *memset(void *s, int c, size_t count)
{
    char *xs = (char *)s;

    while (count--)
        *xs++ = c;

    return s;
}