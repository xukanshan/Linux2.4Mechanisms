#ifndef _ASM_I386_TMP_PUT_CHAR_H
#define _ASM_I386_TMP_PUT_CHAR_H
/* 为了在早期实现printk函数所做的暂时支持，该头文件专门给tmp_put_char.S使用 */


#define TI_GDT 0
#define RPL0 0
#define SELECTOR_VIDEO (0x0006<<3)+TI_GDT+RPL0


#endif /* _ASM_I386_TMP_PUT_CHAR_H */