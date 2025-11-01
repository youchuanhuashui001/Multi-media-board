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

## 2025-08-08
今日主要工作内容：
- 搭建 display 框架，能够使用 framebuffer 画点了

## 2025-08-09
今日主要工作内容：
- 解码框架、显示字、显示 page 移植成功，能够成功显示单行。
- 手动实现一个 show_one_font 函数，可以打出来一行，还在排查原因。

## 2025-08-17
今日主要工作内容：
- 了解 utf-8 编码，并添加页面管理
- 目前能够实现通过串口控制页面翻页

## 2025-09-20
今日主要工作内容：
- 支持 tslib 控制触摸屏翻页:
	- 支持点击翻页(点击屏幕左边部分上一页，点击屏幕右边部分下一页)
	- 支持滑动翻页(往左滑动上一页，往右滑动下一页)

## 2025-09-21
今日主要工作内容：
- 使用 arecord、aplay 录音、播放音频
- 移植 mplayer 到开发板，并支持播放视频，不支持播放音频


## 2025-09-26
今日主要工作内容：
- 移植 lvglv9.x到开发板，显示屏正常，触摸屏正常


## 2025-09-27
今日主要工作内容：
- 参考 lv_port_linux_framber，移植到当前工程
下一步工作计划：
- 学习 lvgl 的组件，可以参考 lvgl/examples 目录下的代码
- 播放音视频等等都需要使用 api 接口来实现
	- alsa-api，ffmpeg-api




## 2025-10-12
今日主要工作内容：
- 学习 lvgl 的组件，参考 https://gitee.com/weidongshan/lvgl_100ask_course_v9/blob/master/part1/02_codes/lv_sim_codeblocks_win/lv_100ask_lesson_demos/src/lesson_3_24_1/lesson_3_24_1.c  https://docs.lvgl.io/master/details/main-modules/fs.html#use-drives-for-images


## 2025-10-25
今日主要工作内容：
- 单独起一个线程来播放音频，使用管道文件来控制 mplayer 播放音频，实现音视频播放和 lvgl 显示同时进行，参考：https://blog.csdn.net/hry_7419/article/details/151196795
- lvgl 的 music_demo 在 click 的 callback 中添加播放音频的代码，能够实现点击播放音频功能


## 2025-10-26
今日主要工作内容：
- 初步实现 lvgl 的代码框架，能够创建一个图标，点击之后播放音乐

## 2025-11-01
今日主要工作内容：
- 调通代码框架，能够实现页面切换，目前有三个页面，只实现了 main 页面的一些功能，其它页面简单填了一张图片