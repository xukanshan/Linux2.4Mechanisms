#include <linux/mm.h>
#include <linux/timex.h>
#include <linux/delay.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>

/* 时间轮中第二层到第五层每层vec的数量位数（用1左移表示） */
#define TVN_BITS 6

/* 时间轮第一层vec的数量位数（用1左移表示） */
#define TVR_BITS 8

/* 时间轮非第一层能容纳的vec数量，64个 */
#define TVN_SIZE (1 << TVN_BITS)

/* 时间轮第一层能够容纳的vec数量，256个 */
#define TVR_SIZE (1 << TVR_BITS)

/* 时间轮非第一层的数据结构 */
struct timer_vec
{
    int index;
    struct list_head vec[TVN_SIZE];
};

/* 时间轮第一层的数据结构 */
struct timer_vec_root
{
    int index;
    struct list_head vec[TVR_SIZE];
};

/* 时间轮的第五层，每一个vec之间的间隔是上一层的最大值，也就是256 * 64 * 64 * 64s，共64个vec */
static struct timer_vec tv5;

/* 时间轮的第四层，每一个vec之间的间隔是上一层的最大值，也就是256 * 64 * 64s，共64个vec */
static struct timer_vec tv4;

/* 时间轮的第三层，每一个vec之间的间隔是上一层的最大值，也就是256 * 64s，共64个vec */
static struct timer_vec tv3;

/* 时间轮的第二层，每一个vec之间的间隔是上一层的最大值，也就是256s，共64个vec */
static struct timer_vec tv2;

/* 时间轮的第一层，每一个vec之间的间隔表示1秒，共256个vec */
static struct timer_vec_root tv1;

/* 用于初始化时间轮，名字中的vecs” 是 “vectors” 的缩写，表示数组或向量，
时间轮在Linux内核的正式译名应该叫定时器向量 */
void init_timervecs(void)
{
    int i; /* 用作循环中的索引 */
    /* 初始化非第一层计时器向量内的vec成员 */
    for (i = 0; i < TVN_SIZE; i++)
    {
        INIT_LIST_HEAD(tv5.vec + i);
        INIT_LIST_HEAD(tv4.vec + i);
        INIT_LIST_HEAD(tv3.vec + i);
        INIT_LIST_HEAD(tv2.vec + i);
    }
    /* 初始化第一层计时器向量内的vec成员 */
    for (i = 0; i < TVR_SIZE; i++)
        INIT_LIST_HEAD(tv1.vec + i);
}

/* 定时器的下半部分处理函数，暂未实现 */
void timer_bh(void)
{
    // update_times();
    // run_timer_list();
}

/* 任务队列下半部分处理函数，暂未实现 */
void tqueue_bh(void)
{
    // run_task_queue(&tq_timer);
}

/* 立即执行的下半部分处理函数，暂未实现 */
void immediate_bh(void)
{
    // run_task_queue(&tq_immediate);
}

/* 用于记录自Unix纪元（1970年1月1日）以来的秒数和微秒数部分 */
volatile struct timeval xtime __attribute__((aligned(16)));
