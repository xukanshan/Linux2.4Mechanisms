#include <linux/mm.h>
#include <linux/kernel_stat.h>
#include <linux/interrupt.h>
#include <linux/smp_lock.h>
#include <linux/init.h>

/* 用于存储指向不同下半部处理函数的指针的数组，共32个 */
static void (*bh_base[32])(void);

/* 初始化底半部（bottom half）处理（应该叫做中断下半部分处理）参数：
nr 通常是下半部分处理的标识符，
routine 代表下半部分要执行的任务 */
void init_bh(int nr, void (*routine)(void))
{
    /* 将 routine 函数的地址赋值给 bh_base 数组的 nr 索引处，从而将指定的下半部处理函数与其标识符关联起来 */
    bh_base[nr] = routine;
    mb(); /* 确保之前的对 bh_base 数组的赋值在CPU上完成后，才会执行后续的操作 */
}