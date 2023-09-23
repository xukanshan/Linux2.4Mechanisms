BUILD_DIR = ./build
C_SOURCES = $(shell find . -name "*.c")
#find命令在当前目录和其子目录中查找所有扩展名为.c的C源文件。然后，将这些文件的完整路径存放在C_SOURCES变量中
C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))
#patsubst函数将C_SOURCES中所有的.c文件转换为.o对象文件。也就是说，它将每个.c文件的路径更改为对应的.o文件路径
S_SOURCES = $(shell find . -name "*.S")
S_OBJECTS = $(patsubst %.S, %.o, $(S_SOURCES))

CC = gcc
LD = ld
ASM = nasm
LIB = -I ./include/ -I ./arch/i386/include

C_FLAGS = $(LIB) -Wall -W -Wstrict-prototypes -Wmissing-prototypes -c -fno-builtin -m32 -fno-stack-protector -nostdinc -fno-pic -gdwarf-2
#-Wall:开启编译器的大多数常用警告。这是推荐的一个参数，因为它会帮助开发者识别出代码中的常见问题。
#需要注意的是，尽管名为“所有(all)”，但 -Wall 并不会开启所有的警告。
#-W:在某些GCC版本中，-W 与 -Wall 是等价的，都用于开启大多数常用警告。
#但在某些上下文中，-W 可用于开启特定的警告。例如，-Wformat 会开启与格式字符串相关的警告。
#-Wstrict-prototypes:这个选项会要求函数声明具有明确的参数类型。
#例如，对于C语言，一个像 int foo(); 这样的函数声明是不明确的，因为它意味着函数foo可以接受任意数量和类型的参数。
#使用 -Wstrict-prototypes 会产生一个警告，建议更明确地声明，如 int foo(void);。
#-Wmissing-prototypes:这个选项会在没有前置函数声明的情况下为全局函数生成警告。
#这是为了确保所有全局函数都有一个头文件中的原型，这样可以在不同的源文件之间共享并检查这些函数的参数和返回类型。
#-I 参数指定包含文件的搜索路径。-c 这告诉GCC仅编译源文件但不链接。
#-fno-builtin 这意味着不使用任何内置的函数优化。默认情况下，GCC会尝试使用内置版本的某些函数（例如memcpy、memset等）
#-m32 指定生成32位x86代码。这对于编译32位的操作系统或应用程序是必要的。
#-fno-stack-protector 这将禁用栈保护，栈保护是一个安全特性，用于检测栈溢出攻击。但在某些低级环境，如内核代码中，它可能不需要或可能导致问题。
#-nostdinc 不在标准系统目录中搜索头文件。这在编写操作系统或其他不依赖于标准C库的代码时很有用。
#-fno-pic 不生成位置独立代码（Position-Independent Code）。在某些环境中，位置独立代码可能是不需要的或不被支持的。
#位置独立代码（Position-Independent Code，简称PIC）是一种特殊类型的机器代码，它可以在内存中的任何位置执行，而不需要进行任何修改。
#这与“位置相关代码”（Position-Dependent Code，非PIC）形成对比，位置相关代码在编译时会被固定到一个特定的内存地址，
#如果要在其他地址执行，需要对其进行重定位。但是，位置独立代码也有其开销。为了减少代码的大小和复杂性，或者为了最大化性能，开发人员可能会选择不使用PIC。
#-gdwarf-2 生成用于调试的DWARF 2格式的信息。这对于使用GDB或其他调试工具进行调试非常有用。DWARF 是一种用于表示程序调试信息的标准

LD_FLAGS = -m elf_i386 -T ./script/kernel.ld -Map ./build/kernel.map -nostdlib
#-m elf_i386：这告诉链接器生成的目标代码格式是 ELF（可执行和链接格式），并且是针对 i386（32位Intel x86）架构的。
#-T ./script/kernel.ld：-T 选项允许你指定一个链接器脚本。链接器脚本用于控制输出二进制文件的布局和其他链接时的行为。
#在操作系统开发中，链接器脚本通常用于确保内核布局与引导加载程序的期望相匹配，并确保特定的符号（如内核的开始和结束地址）位于正确的位置。
#-Map ./build/kernel.map：-Map 选项告诉链接器生成一个映射文件，该文件列出了输出的二进制文件中的所有符号及其地址。
#这在调试和分析内核代码时非常有用，因为它可以帮助你确定函数和变量在内存中的确切位置。
#-nostdlib：这告诉链接器不使用标准系统库。在操作系统内核或其他需要完全控制其环境的代码中，
#你可能不想（也不应该）链接到常规的C库或其他系统库，因为它们通常依赖于运行时环境（如操作系统服务），而这些在内核中可能不可用。

all: $(S_OBJECTS) $(C_OBJECTS) link update_image

.c.o:
	$(CC) $(C_FLAGS) -o $@ $<
#.c.o: 这定义了一个规则，它告诉make如何从.c文件（C源文件）生成.o文件（对象文件）。换句话说，这个规则描述了如何编译C源文件。
#@echo 编译代码文件 $< ... 这是一个命令，它会在终端中打印消息“编译代码文件”后面跟着当前处理的.c文件的名称。
#在make规则中，特殊的变量 $< 指代当前规则的第一个依赖，也就是当前要编译的.c文件。这里的...是一个视觉效果，就是显示三个点


.S.o:
	$(CC) $(C_FLAGS) -o $@ $<
#.c.o 和 .S.o 这样的规则被称为“隐含规则”或“模式规则”。
#它们为构建过程提供了一种高效的方式，使得在常见的文件转换或构建任务中，这样就不必为每个文件明确写出详细的规则。

link:
	$(LD) $(LD_FLAGS) -o kernel.bin $(S_OBJECTS) $(C_OBJECTS) 

.PHONY:clean
clean:
	$(RM) $(S_OBJECTS) $(C_OBJECTS) kernel.bin

.PHONY:update_image
update_image:
	sudo cp kernel.bin ./hdisk/boot/
	@echo OK! ALL DONE!

.PHONY:mount_image
mount_image:
	sudo mount -o loop ./hd.img ./hdisk/

.PHONY:umount_image
umount_image:
	sudo umount ./hdisk

.PHONY:qemu
qemu:
	../installment/qemu-8.1.0/build/qemu-system-x86_64 -serial stdio -drive file=./hd.img,format=raw,index=0,media=disk -m 512 -device VGA
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
	../installment/qemu-8.1.0/build/qemu-system-x86_64 -serial stdio -S -s -drive file=./hd.img,format=raw,index=0,media=disk -m 512
#-S：这使得QEMU在启动时不会自动开始模拟。它会暂停，等待一个调试器连接或接收到一个继续执行的命令。
#-s：这是一个便捷选项，等同于 -gdb tcp::1234。它告诉QEMU在TCP端口1234上监听GDB调试器的连接。这允许你使用GDB调试你在QEMU中运行的代码
