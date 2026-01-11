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


## 2025-11-02
今日主要工作内容：
- 使用 freetype 在 lvgl 中显示中文，读取 txt 文件并显示到屏幕上，目前显示 page 有问题，会丢数据，但是可以实现翻页功能


## 2025-11-8
今日主要工作内容：
- 能够实现电子书显示整页，并且支持翻页功能，支持了书架功能
- 下一步考虑添加返回书架的功能，将电子书的显示左边加一个返回按钮用于返回书架页面

## 2025-11-9
今日主要工作内容：
- 添加书架功能，保存和恢复阅读进度

## 2025-11-15
今日主要工作内容：
- 使用 ffmpeg api 播放音频，了解一些基本概念


## 2025-11-16
今日主要工作内容：
- 使用 ffmpeg api 播放音频，并且使用 lvgl 来驱动，还没有写完！！！！！
- 还有好多事情要干！！！！
- See you next week！！！！！


## 2025-11-30
今日主要工作内容：
- 音乐播放器：使用 ai 设计了一套框架，但是代码还没完全调通
- 播放音乐的通路整条链路通了，但是播放结束之后好像就挂了，并且进度条没走完

## 2025-12-6
今日主要工作内容：
- 音乐播放器：
	- 单首音乐播放完成后可以继续播放下一首
	- 可以暂停，恢复
	- 可以切歌


## 2025-12-7
今日主要工作内容：
- 音乐播放器功能正常，在修改 UI 界面的设计，还没有改完，记录在 audio_view.md 文件中

## 2025-12-14
今日主要工作内容：
- 完成 audio_view 的修改
- 修复添加 seek 功能后导致不能自动播放下一首歌曲的问题
- 添加音量调节功能
- 添加 mqtt 功能，使用 emqx 作为 mqtt 服务器，开发板和浏览器都连接上服务器，开发板发布状态，浏览器订阅状态(不可控制开发板)

## 2026-1-11
今日主要工作内容：
- 搞懂 imx6ull 的 uboot 启动 linux 过程，默认从 emmc 启动，现在也可以用 nfs 启动了
