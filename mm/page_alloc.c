#include <linux/mmzone.h>

/* 抽象每个内存节点的pg_data_t结构体形成的单链表 */
pg_data_t *pgdat_list;