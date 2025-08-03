# 开发日记

## 2025-08-01
今日主要工作内容：
- 搭建开发环境，buildroot 编译成功，生成了各种镜像文件。开发板测试能够ping通主机，并使用mount挂载主机的NFS成功
- buildroot 编译报错，参考解决网页：
	- https://whycan.com/t_11252.html
	- https://blog.csdn.net/gz521125/article/details/135589423
	- https://forum.qt.io/topic/139626/unable-to-build-static-version-of-qt-5-15-2/18
	- https://github.com/mod-audio/u-boot-sunxi-mainline/issues/2
	- https://blog.csdn.net/qq_21697521/article/details/148743835
	- https://github.com/onnx/onnx-tensorrt/issues/474


- 考虑加载方法：
	- bootloader 使用 uboot，烧录到板子上，使用 EMMC 存储
	- linux && rootfs
		- linux 使用 tftp ?
		- rootfs 使用 root-nfs
- 思考一个问题：linux 有两份，一份在 linux4.9 目录中，一份在 Buildroot/output/ 下面，看起来是 Buildroot 这份比较新


## 2025-08-02
今日主要工作内容：
- 开发板使用 framebuffer 成功点亮屏幕，移植 libpng、libfreetype 到开发板显示字体


## 2025-08-03
今日主要工作内容：
- 熟悉 freetype 库
- 编译 100ask 平台的电子书阅读器并在开发板上运行，能够通过串口控制开发板翻页
- 基于 100ask 平台的代码结构，搭建自己的电子书阅读器项目