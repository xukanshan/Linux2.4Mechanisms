#ifndef _LINUX_CTYPE_H
#define _LINUX_CTYPE_H
/* 含了一系列用于字符处理的函数和宏定义。
这个文件的名字和功能与C标准库中的ctype.h相似，
但它是专门为内核代码设计的。
在C语言标准库中，“ctype”代表“character type”，即字符类型。
Linux内核中的ctype.h文件沿用了这个命名习惯，
以表示它提供的是关于字符类型判断和转换的功能 */

#define _U 0x01  /* 表示大写字母（Upper case） */
#define _L 0x02  /* 表示小写字母（Lower case） */
#define _D 0x04  /* 表示数字（Digit） */
#define _C 0x08  /* 表示控制字符（Control character），比如换行、回车等 */
#define _P 0x10  /* 表示标点符号（Punctuation），如逗号、句号等 */
#define _S 0x20  /* 表示空白字符（white Space），包括空格、换行符、制表符等 (space/lf/tab) */
#define _X 0x40  /* 表示十六进制数字（Hexadecimal digit），a-f和A-F */
#define _SP 0x80 /* 表示硬空格（hard SPace），特指空格字符（ASCII码0x20） */

/* 引入字符属性表数组 */
extern unsigned char _ctype[];

/* 这个宏用字符的ascii码去查字符属性表 */
#define __ismask(x) (_ctype[(int)(unsigned char)(x)])

/* 用于判断一个字符是否为数字
它使用一个预定义的数组（_ctype数组），为每个可能的字符值分配一个特定的属性集。
每个数组元素代表一个字符的属性，如字母、数字、空格等。
isdigit函数通过查找给定字符在_ctype数组中对应的值，并使用位运算检查这个值是否包含数字属性（_D）。如果包含，则表示该字符是一个数字。
因此，isdigit实际上是通过查询一个字符属性表来判断一个字符是否是数字。这种方法允许高效且灵活地检查字符属性，因为属性表可以根据需要进行调整和扩展。
举例来说，如果我们想判断字符'3'是否为数字：
首先，'3'对应的ASCII值是51。
在_ctype数组中，索引51的值是_D（即0x04），表明它是一个数字。
isdigit('3')会计算(__ismask('3') & (_D))，即(0x04 & 0x04)，结果不为0，因此返回true。
这就是isdigit如何判断一个字符是否为数字的。它依赖于_ctype数组来获取字符的属性，并使用位运算来检测这些属性。 */
#define isdigit(c) ((__ismask(c) & (_D)) != 0)

#endif /* _LINUX_CTYPE_H */