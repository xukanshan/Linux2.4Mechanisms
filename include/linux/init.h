#ifndef _INIT_H
#define _INIT_H

/* 这个宏定义就是为了让编译器将函数放入.text.init段 */
#define __init __attribute__((__section__(".text.init")))

#endif /* #define _INIT_H */