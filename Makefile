#请根据自己的环境，修改qemu与debug下的qemu执行路径

C_SOURCES = $(shell find . -name "*.c")
#find命令在当前目录和其子目录中查找所有扩展名为.c的C源文件。然后，将这些文件的完整路径存放在C_SOURCES变量中

C_OBJECTS = $(patsubst ./%, build/%, $(C_SOURCES:.c=.o))
#$(C_SOURCES:.c=.o) 遍历$(C_SOURCES)列表中的每个条目。对于每个条目，如果它的末尾是.c，则将.c替换为.o。
#patsubst是一个模式替换功能，它接受三个参数。前两个参数定义了要查找的模式和要替换的模式。
#第三个参数是输入列表。它遍历输入列表中的每个条目。
#对于每个条目，如果它匹配第一个模式（在这里是./%），那么它将该条目替换为第二个模式（在这里是build/%），
#其中%是通配符，它可以匹配任意字符串，并在替换中使用相同的值。
#其结果是将所有路径前的./替换为build/，这样我们就得到了正确的输出目录。

S_SOURCES = $(shell find . -name "*.S")
S_OBJECTS = $(patsubst ./%, build/%, $(S_SOURCES:.S=.o))

CC = gcc
LD = ld
ASM = nasm
LIB =-I ./include/

C_FLAGS = $(LIB) -Wall -W -Wstrict-prototypes -c -fno-builtin -m32 -fno-stack-protector -fno-pic -gdwarf-2
#-Wall:开启编译器的大多数常用警告。这是推荐的一个参数，因为它会帮助开发者识别出代码中的常见问题。
#需要注意的是，尽管名为“所有(all)”，但 -Wall 并不会开启所有的警告。
#-W:在某些GCC版本中，-W 与 -Wall 是等价的，都用于开启大多数常用警告。
#但在某些上下文中，-W 可用于开启特定的警告。例如，-Wformat 会开启与格式字符串相关的警告。
#-Wstrict-prototypes:这个选项会要求函数声明具有明确的参数类型。
#例如，对于C语言，一个像 int foo(); 这样的函数声明是不明确的，因为它意味着函数foo可以接受任意数量和类型的参数。
#使用 -Wstrict-prototypes 会产生一个警告，建议更明确地声明，如 int foo(void);。
#-Wmissing-prototypes:这个选项会在没有前置函数声明的情况下为全局函数生成警告。linux2.4中大量的全局函数都没有声明，所以不用这个参数
#这是为了确保所有全局函数都有一个头文件中的原型，这样可以在不同的源文件之间共享并检查这些函数的参数和返回类型。
#-I 参数指定包含文件的搜索路径。-c 这告诉GCC仅编译源文件但不链接。
#-fno-builtin 这意味着不使用任何内置的函数优化。默认情况下，GCC会尝试使用内置版本的某些函数（例如memcpy、memset等）
#-m32 指定生成32位x86代码。这对于编译32位的操作系统或应用程序是必要的。
#-fno-stack-protector 这将禁用栈保护，栈保护是一个安全特性，用于检测栈溢出攻击。但在某些低级环境，如内核代码中，它可能不需要或可能导致问题。
#-nostdinc 不在标准系统目录中搜索头文件。这在编写操作系统或其他不依赖于标准C库的代码时很有用。但是Linux2.4一些数据结构定义用到了
#c库，所以复现时不能加这个参数
#-fno-pic 不生成位置独立代码（Position-Independent Code）。在某些环境中，位置独立代码可能是不需要的或不被支持的。
#位置独立代码（Position-Independent Code，简称PIC）是一种特殊类型的机器代码，它可以在内存中的任何位置执行，而不需要进行任何修改。
#这与“位置相关代码”（Position-Dependent Code，非PIC）形成对比，位置相关代码在编译时会被固定到一个特定的内存地址，
#如果要在其他地址执行，需要对其进行重定位。但是，位置独立代码也有其开销。为了减少代码的大小和复杂性，或者为了最大化性能，开发人员可能会选择不使用PIC。
#-gdwarf-2 生成用于调试的DWARF 2格式的信息。这对于使用GDB或其他调试工具进行调试非常有用。DWARF 是一种用于表示程序调试信息的标准
#-fgnu89-inline  GCC 使用旧的 GNU89 内联语义，而不是 C99 的内联语义。
#对于extern inline 旧版gcc：我在头文件中定义一个d函数，并且加上了extern inline。假设a.c,b.c,c.c调用了这个头文件定义的d函数，
#那么a.c函数就会将这个d函数直接插入，而b,c函数就不会直接插入d函数，而是会去寻找d函数不加extern inline版本。新版gcc：a.c, b.c, c.c中的每个文件被编译时，
#它们都会看到 d 函数的 extern inline 定义。在 C99 标准中，inline 函数默认情况下不会在外部链接中提供其符号。
#因此，每个编译单元（a.c, b.c, c.c）都会尝试生成自己的 d 函数的本地副本。当这些编译单元最终链接成一个程序时，链接器会发现 d 函数在多个编译单元中都有定义，从而引发重复定义的错误。
#这种重复定义的问题是因为新版 GCC 遵循的 C99 行为，其中 inline 函数默认不提供外部链接的符号，除非显式指定。
#因此，在新版 GCC 中，如果不希望每个编译单元都有自己的 d 函数副本，就需要额外的处理，
#例如使用 static inline 替代 extern inline，或者在一个单独的 .c 文件中提供一个普通（非内联）的 d 函数定义供外部链接使用。
#猜测Linux2.4（遵循的是 GNU89 的内联语义）这么做，是因为inb，outb之类只会在底层驱动一次使用，这样就能避免符号冲突

ASM_FLAGS = $(LIB) -c -fno-builtin -m32 -gdwarf-2 -D__ASSEMBLY__ -fno-stack-protector -nostdinc -fno-pic  
#虽然编译汇编也是用的gcc编译器，但是相比于C_FLAGS，ASM_FLAGS少了-Wall（因为这些选项是为C编译器设计的） 
#少了-Wstrict-prototypes -Wmissing-prototypes （这两个是为C函数原型设计的）
#多了-D__ASSEMBLY__ -D：这个选项告诉GCC定义一个宏，其后紧跟宏的名称
#这个宏会被定义为1。它等同于在源代码的最开始添加了#define __ASSEMBLY__ 1这一行
#这么做是因为Linux2.4中，S代码和c代码使用了同样的头文件，但是头文件中有些特性（如结构体）定义S代码无法识别，
#所以这些定义需要用ifndf包裹起来，然后编译S与编译c使用不用的参数，就可以实现c识别这些头文件定义，但是S不识别

LD_FLAGS = -m elf_i386 -T ./script/kernel.ld -Map ./build/kernel.map -nostdlib
#-m elf_i386：这告诉链接器生成的目标代码格式是 ELF（可执行和链接格式），并且是针对 i386（32位Intel x86）架构的。
#-T ./script/kernel.ld：-T 选项允许你指定一个链接器脚本。链接器脚本用于控制输出二进制文件的布局和其他链接时的行为。
#在操作系统开发中，链接器脚本通常用于确保内核布局与引导加载程序的期望相匹配，并确保特定的符号（如内核的开始和结束地址）位于正确的位置。
#-Map ./build/kernel.map：-Map 选项告诉链接器生成一个映射文件，该文件列出了输出的二进制文件中的所有符号及其地址。
#这在调试和分析内核代码时非常有用，因为它可以帮助你确定函数和变量在内存中的确切位置。
#-nostdlib：这告诉链接器不使用标准系统库。在操作系统内核或其他需要完全控制其环境的代码中，
#你可能不想（也不应该）链接到常规的C库或其他系统库，因为它们通常依赖于运行时环境（如操作系统服务），而这些在内核中可能不可用。

all: $(S_OBJECTS) $(C_OBJECTS) link update_image

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) -o $@ $<
#build/%.o: %.c: 这定义了一个规则，它告诉make如何从.c文件（C源文件）生成build/.o文件（对象文件）。换句话说，这个规则描述了如何编译C源文件。
#mkdir: 用于创建目录。-p: 是 mkdir 的一个选项，它的功能是确保父目录存在。
#如果父目录已经存在，它不会报错。如果不存在，它会创建所需的父目录。
#例如，mkdir -p build/arch/i386 会创建 build, bulid/arch, 和 build/arch/i386 三个目录。
#$(dir ...)：是一个 Makefile 函数，它返回给定文件的目录部分。
#例如，$(dir build/arch/i386/grub_head.o) 返回 build/arch/i386/。
#$@: 是一个 Makefile 的自动变量，它代表当前目标的文件名。这里，它表示的是 build/.../*.o 文件的路径。
#$< 指代当前规则的第一个依赖，也就是当前要编译的.c文件。

build/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASM_FLAGS) -o $@ $<
#build/%.o: %.c 和 build/%.o: %.S这样的规则被称为模式规则。
#它们为构建过程提供了一种高效的方式，使得在常见的文件转换或构建任务中，这样就不必为每个文件明确写出详细的规则。

link:
	$(LD) $(LD_FLAGS) -o build/kernel.bin $(S_OBJECTS) $(C_OBJECTS) 

.PHONY:clean
clean:
	@sudo rm -r build

.PHONY:update_image
update_image:
	sudo cp ./build/kernel.bin ./hdisk/boot/
	@echo OK! ALL DONE!

.PHONY:mount_image
mount_image:
	sudo mount -o loop ./hd.img ./hdisk/

.PHONY:umount_image
umount_image:
	sudo umount ./hdisk

.PHONY:qemu
qemu:
	qemu-system-x86_64 -serial stdio -drive file=./hd.img,format=raw,index=0,media=disk -m 512 -device VGA
#-serial stdio：这告诉QEMU将虚拟机的串行端口输出重定向到主机的标准输入/输出（通常是你的终端或命令行窗口）。
#这允许你从虚拟机接收文本输出并向其发送输入。
#-drive file=./hd.img,format=raw,index=0,media=disk：这告诉QEMU你想连接一个虚拟硬盘，并给出了硬盘的详细信息。
#file=./hd.img：虚拟硬盘的文件路径是./hd.img。
#format=raw：图像文件的格式是原始的，即没有特定的文件系统或结构。
#index=0：这是该驱动器在模拟器中的索引号。通常，你的第一个硬盘或驱动器会有索引0。
#media=disk：这指明这是一个硬盘图像，而不是其他类型的存储介质，如光盘。
#-m 512：这为虚拟机分配了512MB的RAM。
#-device VGA 这告诉QEMU为虚拟机添加一个标准的VGA兼容的视频设备。VGA（Video Graphics Array）是计算机图形的旧标准，
#但在许多上下文中，它仍然被用作默认或基线显示配置。通过指定 -device VGA，QEMU会模拟一个VGA视频卡，
#这样运行在虚拟机上的操作系统或软件就可以使用标准的VGA驱动和模式来显示图形和文本输出，运行QEMU后，看到的黑色窗口是该虚拟VGA设备的输出显示

.PHONY:debug
debug:
	qemu-system-x86_64 -serial stdio -S -s -drive file=./hd.img,format=raw,index=0,media=disk -m 512
#-S：这使得QEMU在启动时不会自动开始模拟。它会停下来，等待一个调试器连接或接收到一个继续执行的命令。
#-s：这是一个便捷选项，等同于 -gdb tcp::1234。它告诉QEMU在TCP端口1234上监听GDB调试器的连接。这允许你使用GDB调试你在QEMU中运行的代码