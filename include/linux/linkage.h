#ifndef _LINUX_LINKAGE_H
#define _LINUX_LINKAGE_H

#define __ALIGN .align 4,0x90
#define ALIGN __ALIGN
#define ENTRY(name) \
.globl name; \
ALIGN; \
name:

#endif
