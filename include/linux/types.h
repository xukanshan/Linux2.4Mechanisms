#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H
/* Linux内核的设计遵循了抽象的软件工程原则，通过在不同层次之间提供抽象，
确保了代码的可移植性和模块性。linux/types.h提供了一个硬件无关的基本类型定义，
它可以根据不同的架构通过asm/types.h或其他类似的文件来具体实现。
这样，内核代码的大部分可以编写得与硬件无关，只有很小一部分需要针对具体的硬件架构进行修改 */

/* 以下是一些Linux内核代码中常用抽象层的例子：
架构无关层 (Generic layer): 这些代码对于所有硬件平台都是一样的。例如，linux/types.h和大多数内核子系统代码都是这样的。
架构相关层 (Architecture-specific layer): 这些代码针对特定的硬件架构。例如，asm/types.h通常会根据具体的CPU架构提供类型定义。
驱动和子系统层 (Driver and subsystem layer): 这些代码通常针对具体的设备或子系统，但仍然会尽可能地使用通用的接口和数据类型。 */

#include <asm-i386/types.h>

#endif /* _LINUX_TYPES_H */