/* 这个文件是用于临时的put_str实现，因为要打印些信息用于调试，但目前还没有初始化控制台驱动，没有对应的write实现
代码来源：修改的真象还原代码直接的 */

extern void put_char(char c);

void print_str(char *str)
{
    while (*str) {
        put_char(*str);
        str++;
    }
}