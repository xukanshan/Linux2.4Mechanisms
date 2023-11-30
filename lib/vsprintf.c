#include <stdarg.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <asm-i386/div64.h>

#define ZEROPAD 1  /* 用0填充 */
#define SIGN 2     /* 决定是unsigned long 还是 signed long */
#define PLUS 4     /* 数值前显示加号 */
#define SPACE 8    /* 在正数前应该加上一个空格 */
#define LEFT 16    /* 左对齐 */
#define SPECIAL 32 /* 特殊格式化（如在十六进制数前加0x） */
#define LARGE 64   /* 大写 */

/* 解析一个字符串中的连续数字字符序列，并将其转换为一个整数。
它通过遍历字符串中的字符，每次将累积的数乘以10再加上新的数字字符代表的数值，
这样可以将一个数字字符串（如"123"）转换为相应的整数值（123）。
同时，函数通过移动指针跳过已经处理的数字字符，为后续的解析工作准备。
skip指的是这个函数在转换数字的同时，还会移动（或“跳过”）字符串中已经处理的数字字符。
atoi是一个在C语言中广泛使用的标准库函数名，代表“ASCII to Integer”（ASCII转整数）。
函数接受一个指向字符指针的指针。这意味着它可以修改字符指针（比如移动它），但不能修改指针指向的字符内容。 */
static int skip_atoi(const char **s)
{
    int i = 0;                        /* 用于累积转换得到的数字 */
    while (isdigit(**s))              /* 持续检查s指向的指针所指向的字符是否为数字（0到9） */
        i = i * 10 + *((*s)++) - '0'; /* 将当前累积的数字i乘以10（这是因为我们可能正在解析一个多位数），然后加上当前字符代表的数字
                                       *((*s)++)首先取得当前字符，然后将s指向的指针向前移动一位，由于字符是以其ASCII码存储的，所以需要减去'0'的ASCII码来得到实际的数值
                                        (*s)++:这是表达式的核心。首先，(*s) 对指针 s 进行解引用操作，得到一级指针（直接指向了字符）。
                                        然后，++ 操作符作用于这个一级指针。这是后置增量操作符，它的意思是先返回指针的当前值，然后将指针移动到下一个位置。
                                        *((...)):最外层的 * 是对 (*s)++ 结果的再次解引用。由于 (*s)++ 返回的是一级指针，最外层的 * 则是取出这个位置上的字符值。 */
    return i;                         /* 返回累积的数字i */
}

/* 用于将长整型数字（long long num）转换成特定基数（base）的字符串表示，通常用于格式化输出。
这个函数处理数字的转换，并考虑了诸如填充、对齐、正负号等格式化选项。参数说明：
str: 输出缓冲区。
num: 要转换的数字。
base: 数字的基数（例如，10代表十进制，16代表十六进制）。
size: 输出字段的总大小。
precision: 数字部分的最小长度。
type: 控制标志（如填充字符、对齐方式等）。 */
static char *number(char *str, long long num, int base, int size, int precision, int type)
{
    /* c: 填充字符（空格或零）。sign: 正负号字符。tmp: 临时数组，用于存储转换后的数字字符。 */
    char c, sign, tmp[66];
    const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz"; /* 数字和字母的字符串，用于表示各种基数下的数字 */
    int i;                                                       /* 循环变量 */

    if (type & LARGE) /* 如果设置了LARGE标志，则使用大写字母 */
        digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    if (type & LEFT) /* 如果设置了左对齐（LEFT），则不使用零填充 */
        type &= ~ZEROPAD;

    if (base < 2 || base > 36) /* 确保基数在有效范围内（2到36） */
        return 0;

    c = (type & ZEROPAD) ? '0' : ' '; /* 根据是否零填充（ZEROPAD）来确定填充字符是'0'还是空格。 */

    sign = 0; /* sign初始化为0 */

    if (type & SIGN) /* 根据数字的正负以及是不是有符号的数字，设置sign变量，并相应地调整size */
    {
        if (num < 0)
        {
            sign = '-';
            num = -num;
            size--;
        }
        else if (type & PLUS)
        {
            sign = '+';
            size--;
        }
        else if (type & SPACE)
        {
            sign = ' ';
            size--;
        }
    }

    if (type & SPECIAL) /* 对于十六进制和八进制，size会根据需要的前缀字符进行调整 */
    {
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    }

    i = 0;
    /* 将数字num转换为字符串形式，存储在tmp数组中（逆序） */
    if (num == 0) /* 如果num为0，则直接在tmp数组中存储字符'0' */
        tmp[i++] = '0';
    else
        /* 如果num不为0，使用循环将num转换为对应基数的字符串表示。
        do_div负责执行除法（会将num改变）和模运算（返回的就是余数），返回的索引用于在digits字符串中找到相应的字符 */
        while (num != 0)
            tmp[i++] = digits[do_div(num, base)];

    if (i > precision) /* 如果实际数字长度（i）大于指定的精度（precision），则更新精度以匹配数字长度 */
        precision = i;

    size -= precision; /* 从总输出大小中减去数字长度（现在的精度），为前缀和填充留出空间 */

    if (!(type & (ZEROPAD + LEFT))) /* 如果没有设置零填充（ZEROPAD）和左对齐（LEFT），则在数字前添加空格填充到总输出大小 */
        while (size-- > 0)
            *str++ = ' ';

    if (sign) /* 如果存在正负号（非零sign），则将其添加到输出缓冲区 */
        *str++ = sign;

    if (type & SPECIAL) /* 如果设置了特殊标志（SPECIAL），则对八进制和十六进制数添加前缀。八进制前缀为'0'，十六进制前缀为"0x"或"0X" */
    {
        if (base == 8)
            *str++ = '0';
        else if (base == 16)
        {
            *str++ = '0';
            *str++ = digits[33];
        }
    }

    if (!(type & LEFT)) /* 如果没有设置左对齐（LEFT），则在数字前添加额外的填充字符（c），这可以是空格或零 */
        while (size-- > 0)
            *str++ = c;

    while (i < precision--) /* 在数字前添加额外的零来满足精度要求 */
        *str++ = '0';

    while (i-- > 0) /* 将tmp数组中的数字逆序复制到输出缓冲区，因为数字是逆序生成的 */
        *str++ = tmp[i];

    while (size-- > 0) /* 如果还有剩余空间，继续填充空格，通常用于左对齐的情况 */
        *str++ = ' ';

    return str; /* 返回输出缓冲区的新位置，即所有格式化内容之后的位置 */
}

/* 此函数通过解析格式字符串和相应的参数，将输出格式化为字符串。
它支持多种格式化选项，如左对齐、补零、字段宽度和精度设置等，
"v": 表示“可变参数列表”（varargs）。"sprintf": 是“string print formatted”的缩写，意味着这个函数将格式化的数据打印到一个字符串中。
参数：
buf: 输出缓冲区，用于存放格式化后的字符串。
fmt: 格式字符串，指定输出格式。
args: 可变参数列表，包含了与格式字符串相对应的参数。 */
int vsprintf(char *buf, const char *fmt, va_list args)
{
    int len;                /* 用于存储字符串长度 */
    unsigned long long num; /* 用于存储转换后的数字 */
    int i, base;            /* i 用于循环，base 表示数字的基数（如10进制、16进制） */
    char *str;              /* 用于操作buf */
    const char *s;          /* 用于存储从参数列表中获取的字符串 */
    int flags;              /* 存储格式化标志（如左对齐、补零等） */
    int field_width;        /* 字段宽度 */
    int precision;          /* 精度，用于整数和字符串 */
    int qualifier;          /* 修饰符，如h, l, L 或 Z */

    for (str = buf; *fmt; ++fmt) /* 循环遍历格式字符串fmt，str指向输出缓冲区 */
    {
        if (*fmt != '%') /* 如果当前字符不是%，则直接将其复制到输出缓冲区 */
        {
            *str++ = *fmt;
            continue;
        }

        /* 来到了这一句，说明已经遇到了% */
        /* 这一段代码是函数中处理格式化标志的部分 */
        flags = 0;    /* 格式化标志初始化 */
    repeat:           /* 标签，如果遇到了一个格式化标志，处理完毕后，它会跳转回到这个标签重新开始处理下一个字符 */
        ++fmt;        /* 指向格式字符串的下一个字符。在第一次循环时，这也跳过了格式字符串中的第一个'%'字符 */
        switch (*fmt) /* 处理格式化标志 (case '-', case '+', case ' ', case '#', case '0'): */
        {             /* 对于每个标志，设置相应的flags位，然后跳转回循环开始（goto repeat;） */
        case '-':     /* 如果当前字符是'-'，这表示左对齐标志 */
            flags |= LEFT;
            goto repeat;
        case '+': /* 如果当前字符是'+'，这表示数值前显示加号 */
            flags |= PLUS;
            goto repeat;
        case ' ': /* 如果当前字符是空格，这表示在正数前应该加上一个空格 */
            flags |= SPACE;
            goto repeat;
        case '#': /* 如果当前字符是'#'，这表示特殊格式化（如在十六进制数前加0x） */
            flags |= SPECIAL;
            goto repeat;
        case '0': /* 如果当前字符是'0'，这表示使用零填充而不是空格 */
            flags |= ZEROPAD;
            goto repeat;
        }

        /* 这段代码负责从格式字符串中解析出字段宽度。字段宽度是指定在输出结果中所占的字符数 */
        field_width = -1;                  /* 初始化field_width为-1。在这里，-1 表示没有指定字段宽度 */
        if (isdigit(*fmt))                 /* 这里检查格式字符串中当前的字符（指向的是*fmt）是否是数字。isdigit函数用于检测一个字符是否为数字字符（0-9） */
            field_width = skip_atoi(&fmt); /* 如果当前字符是数字，那么调用skip_atoi函数来解析这个数字。
                                            这个函数会将字符串中的数字部分转换为整数，并将fmt指针向前移动到数字之后的字符。 */
        else if (*fmt == '*')              /* 如果格式字符串中的当前字符是星号（*），这意味着字段宽度被指定为一个参数，而不是直接写在格式字符串中 */
        {
            ++fmt;                           /* 将fmt指针向前移动，跳过星号，以便处理下一个字符或参数 */
            field_width = va_arg(args, int); /* 通过va_arg宏从args（可变参数列表）中获取下一个参数，其实现等同于va_arg(ap, t) *((t*)(ap += 4))，这个ap是char*类型 */
            if (field_width < 0)             /* 检查字段宽度是否小于0。如果是，这意味着字段宽度被指定为负数 */
            {
                field_width = -field_width; /* 如果字段宽度是负数，那么将其转换为正数。负数表示结果应该左对齐（而不是正数的默认右对齐） */
                flags |= LEFT;              /* 设置LEFT标志，指示输出应该左对齐 */
            }
        }

        /* 这一段代码是函数中处理精度(precision)的部分，"精度"（precision）是一个指定如何精确表示数值或控制字符串输出长度的参数。
        它的含义和作用取决于它与哪种类型的格式说明符一起使用
        对于整数类型（如 %d, %i, %o, %u, %x, %X）:
        精度指定了要显示的最小数字位数。如果实际数字位数少于精度指定的位数，则结果会在左边填充零以达到指定的位数。
        例如，格式%.5d与数值123一起使用时，输出将是00123。
        对于浮点数类型（如 %f, %F, %e, %E, %g, %G）:
        精度指定了小数点后要显示的位数。默认情况下，通常是6位。
        例如，格式%.2f与数值123.456一起使用时，输出将是123.46。
        对于字符串类型（%s）:
        精度指定了最大字符数限制。如果字符串的长度超过这个限制，则只输出字符串的前n个字符，其中n是精度值。
        例如，格式%.3s与字符串"Hello"一起使用时，输出将是"Hel"。
        精度是通过格式字符串（如%.2f, %.5d, %.3s）或者在格式字符串中使用星号*并将精度作为额外的参数传递（如%.*f）来指定的。*/
        precision = -1;  /* 初始化precision变量为-1，这是一个标记值，表示精度尚未设置或无效 */
        if (*fmt == '.') /* 这个条件检查当前格式字符串的字符是否为.。在格式字符串中，.标志着精度说明的开始 */
        {
            ++fmt;                           /* 移动格式字符串的指针到下一个字符，跳过.，以解析精度值 */
            if (isdigit(*fmt))               /* 接下来的字符是否是数字。如果是，表示精度值将直接在格式字符串中指定，如%.2f中的2 */
                precision = skip_atoi(&fmt); /* 如果精度值在格式字符串中直接给出（如2在%.2f），skip_atoi函数会将这些数字字符转换为整数（即2），并更新fmt指针以跳过这些数字 */
            else if (*fmt == '*')            /* 如果接下来的字符是*，则表示精度值将从参数列表中获取，而不是直接在格式字符串中指定。 */
            {
                ++fmt; /* 跳过*字符，准备从参数列表中获取精度值 */
                precision = va_arg(args, int);
            }
            if (precision < 0) /* 如果精度是负数，则将其设置为0，表示没有特定的精度限制 */
                precision = 0;
        }

        /* 用于获取转换修饰符（conversion qualifier），它们用来修改相应的格式占位符（如 %d, %s 等）的行为，主要涉及数据类型的长度或大小
        下面是一些常见的转换修饰符及其含义：
        h:当与整数转换说明符（如d, i, o, u, x, X）一起使用时，h表示相应的参数是short int或unsigned short int类型的。例如，在printf("%hd", x);中，x应为short int类型。
        l:用于整数类型时，l表示long int或unsigned long int。
        在浮点数类型（如f, F, e, E, g, G）中，它表示double类型（虽然在这种情况下，l修饰符实际上是可选的，因为float参数在传递给函数时总是提升为double）。
        对于%c和%s，l修饰符表示宽字符（wchar_t）和宽字符串。
        L:主要用于浮点数，表示long double类型。
        Z:是Linux特有的，表示size_t类型。
        t:用于表示ptrdiff_t类型的参数，这是两个指针差值的类型。
        j:表示intmax_t或uintmax_t类型的整数，这些类型足以容纳任何类型的整数
        在格式化字符串中正确使用这些修饰符可以确保数据被正确地解释和处理。
        例如，在处理大文件的偏移时，可能需要使用%ld或%lld来格式化long或long long类型的变量。*/
        qualifier = -1; /* 初始化qualifier变量为-1。用于表示尚未设置或识别任何有效的转换修饰符 */
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z')
        {
            qualifier = *fmt;
            ++fmt; /* 跳过已经处理的转换修饰符 */
        }

        base = 10; /* 默认数字的基数，表示10进制 */

        switch (*fmt) /* 处理不同类型的格式化输出:如字符（'c'）、字符串（'s'）、指针（'p'）、整数（'d', 'i', 'u'）、16进制（'x', 'X'）等 */
        {
        case 'c':                /* 字符格式化 */
            if (!(flags & LEFT)) /* 如果没有左对齐标志(LEFT)，则在字符前填充空格，直到达到字段宽度 */
                while (--field_width > 0)
                    *str++ = ' ';
            *str++ = (unsigned char)va_arg(args, int); /* 从参数列表中获取一个字符（实际作为int传递）并将其添加到输出缓冲区 */
            while (--field_width > 0)                  /* 如果此时fileld_width仍然大于0，说明是左对齐，所以字符后填充空格直到字段宽度 */
                *str++ = ' ';
            continue;

        case 's':                     /* 字符串格式化 */
            s = va_arg(args, char *); /* 从参数列表中获取一个字符串 */
            if (!s)                   /* 如果字符串为空，则用"<NULL>"替代 */
                s = "<NULL>";

            len = strnlen(s, precision); /* 计算字符串长度，但不超过精度值，strnlen参数第二个是定义为unsigned int，即使精度是-1，也不会出问题 */

            if (!(flags & LEFT)) /* 如果没有左对齐标志，在字符串前填充空格直到字段宽度 */
                while (len < field_width--)
                    *str++ = ' ';
            for (i = 0; i < len; ++i) /* 将字符串复制到输出缓冲区，但不超过计算出的长度 */
                *str++ = *s++;
            while (len < field_width--) /* 如果此时fileld_width仍然大于0，说明是左对齐，所以字符后填充空格直到字段宽度 */
                *str++ = ' ';
            continue;

        case 'p':                  /* 指针格式化 */
            if (field_width == -1) /* 如果未指定字段宽度，则设为指针大小的两倍（也就是显示8个字符），并设置补零标志 */
            {
                field_width = 2 * sizeof(void *);
                flags |= ZEROPAD;
            }
            /* 将指针值转换为16进制字符串并添加到输出缓冲区 */
            str = number(str, (unsigned long)va_arg(args, void *), 16, field_width, precision, flags);
            continue;

        /* 根据修饰符(qualifier)获取正确的参数类型（long*, size_t*, int*），
        %n是一个特殊的格式指定符，它不输出任何内容，
        而是将到目前为止已写入字符串的字符数（不包括结尾的空字符）保存到给定的整数指针变量中 */
        case 'n':
            if (qualifier == 'l') /* 检查修饰符是否为l，这意味着预期的参数是一个long类型的指针 */
            {
                long *ip = va_arg(args, long *);
                *ip = (str - buf);
            }
            else if (qualifier == 'Z') /* 检查修饰符是否为Z。这是一个非标准修饰符，通常用于指示size_t类型 */
            {
                size_t *ip = va_arg(args, size_t *);
                *ip = (str - buf);
            }
            else /* 如果没有匹配到任何已知的修饰符，则默认处理为int类型的指针 */
            {
                int *ip = va_arg(args, int *);
                *ip = (str - buf);
            }
            continue;

        case '%': /* 直接在输出缓冲区添加%字符 */
            *str++ = '%';
            continue;

        /* 这些case处理不同的整数类型*/
        case 'o':
            base = 8;
            break;

        case 'X': /* 十六进制数（大写）格式化 */
            flags |= LARGE;
            // fall through
            /* 添加上面那个注释是因为编译器认为可能存在 "fall through" 的情况。
            "Fall through" 发生在 switch 语句中，当一个 case 语句块结束后没有 break 或 return 语句，
            导致控制流无条件地转移到下一个 case 语句块。这里我们这么写当然是有意为之，所以添加上面那个注释后，编译器就不会警告了 */
        case 'x':
            base = 16;
            break;

        /* 有符号十进制整数格式化 */
        case 'd':
        case 'i': /* %i与%d非常相似，在大多数情况下它们是可互换的，都用于格式化十进制整数。
        当用于scanf函数时，%i不仅可以读取十进制数，还可以读取八进制（以0开头）和十六进制（以0x或0X开头）的数。
        这是它与%d的一个主要区别，因为%d在输入时只识别十进制数 */
            flags |= SIGN;
        case 'u':
            break;

        /* 如果遇到未识别的格式化字符，则在输出缓冲区添加%和该字符 */
        default:
            *str++ = '%';
            if (*fmt)
                *str++ = *fmt;
            else
                --fmt;
            continue;
        }
        if (qualifier == 'L') /* 如果修饰符是'L'，则表示参数应当被解释为long long类型 */
            num = va_arg(args, long long);
        else if (qualifier == 'l') /* 如果修饰符是'l'，则参数被解释为long类型 */
        {
            num = va_arg(args, unsigned long);
            if (flags & SIGN) /* 如果设置了SIGN标志（表示有符号数），则进行类型转换 */
                num = (signed long)num;
        }
        else if (qualifier == 'Z') /* 如果修饰符是'Z'，则参数被解释为size_t类型 */
        {
            num = va_arg(args, size_t);
        }
        else if (qualifier == 'h') /* 如果修饰符是'h'，则参数被解释为short类型 */
        {
            num = (unsigned short)va_arg(args, int);
            if (flags & SIGN)
                num = (signed short)num;
        }
        else /* 如果没有匹配到任何修饰符，那么默认参数被解释为int类型 */
        {
            num = va_arg(args, unsigned int);
            if (flags & SIGN)
                num = (signed int)num;
        }
        str = number(str, num, base, field_width, precision, flags);
    }
    *str = '\0'; /* 在字符串的最后加上空字符（\0）并返回输出字符串的长度 */
    return str - buf;
}