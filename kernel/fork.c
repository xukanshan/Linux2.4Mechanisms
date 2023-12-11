#include <linux/malloc.h>
#include <linux/init.h>
#include <linux/smp_lock.h>
#include <asm-i386/pgtable.h>
#include <asm-i386/mmu_context.h>

/* PID 哈希表，这个表用于快速查找PCB。每个进程都有一个唯一的 PID，
哈希表使得根据 PID 快速定位到相应的 PCB 成为可能 */
struct task_struct *pidhash[PIDHASH_SZ];