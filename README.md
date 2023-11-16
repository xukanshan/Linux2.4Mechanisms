# **Linux2.4Mechanisms**

## 描述：

- 本仓库代码是自己学习并复现Linux2.4主要机制的成果。
- 系统采用grub2.06进行引导，未采用linux2.4原生引导方式。grub引导完毕之后通过`grub_head.S`的`grub_head_to_startup_32`函数跳转进入内核的`startup_32`函数。从此开始，便是Linux2.4主要机制代码。相较于Linux2.4，本系统只增加了`grub_head.S`文件。

## 复现路径：

1. 完成“参考与感谢”部分中的“环境搭建”与“环境搭建后-->运行起来”。
2. 之后学习《Linux内核源代码情景分析》第十章1492-1495页，然后去修改grub引导，链接脚本kernel.ld，使之适配Linux2.4内核映像的引导。
3. 继续学习《Linux内核源代码情景分析》第十章，然后递归复现Linux2.4主要机制代码，也就是从startup_32函数入手，然后依据对主要机制的理解去复现。期间《Linux内核源代码情景分析》也要从头开始看。换句话说，《Linux内核源代码情景分析》这本书需要从两个入口同时开始看，一个入口是第十章，一个入口是从头。

## 如何区分主要机制与非主要机制？

核心是依据对《操作系统真象还原》、《Linux内核源代码情节解析》、《Linux内核设计与实现》的理解而进行评判。

## 具体目标：

实现一个面向典型i386体系的32位单核环境的Linux2.4主要机制版。

## 参考与感谢

- 环境搭建：https://www.bilibili.com/video/BV1F84y1f7am/	

  注意：WSL环境搭建后，会遇到qemu启动没有图形界面问题，解决方法：https://www.bilibili.com/video/BV1fu4y1e7hG/
  （视频中02：08，安装gdk报错问题需自行解决）。在vmware中运行一个有图形化界面的Linux虚拟机来搭建环境就没有这个问题。

- 环境搭建后-->运行起来：https://www.bilibili.com/video/BV1fu4y1e7hG/ （视频中复用了胡庆伟自己写的操作系统XOS（https://github.com/huloves/XOS） 启动部分（XOS仓库doc目录下有文档））

- 主要参考书籍：《Linux内核源代码情景分析》，《操作系统真象还原》，《Linux内核设计与实现》

