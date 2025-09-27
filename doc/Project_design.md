# 基础环境搭建

## 开发板与主机挂载 nfs
- 使用 USB 网口一端连接到开发板的右侧网口 eth0，一端连接到主机的
- 配置开发板 ip:`ifconfig eth0 192.168.31.9`
- 配置 PC ip:`sudo ifconfig ethxxxxx 192.168.31.10`
- 开发板与 PC 互相能够 ping 通
- 开发板执行命令：`mount -t nfs -o nolock 192.168.31.10:/opt/nfs /tmp`

## 如何更新 kernel
- 开发板配置
- 编译 Linux4.9.88 后将 /arch/arm/boot/zImage 拷贝到主机的 nfs 目录
- 继续将 nfs 目录下的 zImage 拷贝到 /boot 目录
- 重启开发板

## 去掉 lvgl 以及让屏幕不再自动熄灭
- 去掉 lvgl：将 `/etc/init.d/S05lvg` 替换成其他的名称，例如：`bak_S05lvg`
- 让屏幕不再自动熄灭：`echo -e  "\033[9;0]"  > /dev/tty0`

## freetype

- 工具链缺少 brotli，因此需要先下载源码并编译
- 编译完成后将 lib 和 include 拷贝到工具链中去，并将 lib 拷贝到开发板上的 lib 目录下
- 编译 freetype 的时候

```shell


# build brotli

cmake . -DCMAKE_INSTALL_PREFIX=/home/tanxzh/sysroot/usr \
        -DCMAKE_C_COMPILER=arm-buildroot-linux-gnueabihf-gcc
make

# 这里目录可能是错误的，DESTDIR 用于指定向哪里安装 brotli
sudo make install  DESTDIR=/home/tanxzh/project/100ask/100ask_imx6ull_sdk/Buildroot_2020.02.x/output/host


# build freetype

./configure --host=arm-buildroot-linux-gnueabihf --prefix=$PWD/tmp --with-sysroot=/home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/lib/gcc/arm-buildroot-linux-gnueabihf/7.5.0/ --with-harfbuzz=no

make -j8
make install 

cp include/* -rf /home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/../lib/gcc/arm-buildroot-linux-gnueabihf/7.5.0/include

cp lib/* -rfd /home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/../lib/gcc/arm-buildroot-linux-gnueabihf/7.5.0/../../../../arm-buildroot-linux-gnueabihf/lib

arm-buildroot-linux-gnueabihf-gcc -o show_line show_line.c \
-L/home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/lib \
-I/home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/include \
-lfreetype -lbrotlidec -lbrotlicommon
```

## tslib

- 编译 tslib

```shell
mkdir tmp
./configure --host=arm-buildroot-linux-gnueabihf --prefix=$PWD/tmp

make -j8
make install

cp include/* -rf /home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/../lib/gcc/arm-buildroot-linux-gnueabihf/7.5.0/include 
cp lib/* -rfd /home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/../lib/gcc/arm-buildroot-linux-gnueabihf/7.5.0/../../../../arm-buildroot-linux-gnueabihf/lib
```

- 开发板执行`tslib`自带测试程序

```shell
export TSLIB_CONSOLEDEVICE=none
export TSLIB_FBDEVICE=/dev/fb0
export TSLIB_TSDEVICE=/dev/input/event1
export TSLIB_CONFFILE=/etc/ts.conf
export TSLIB_PLUGINDIR=/usr/lib/ts

ts_print
```

- 交叉编译测试程序

```shell
arm-buildroot-linux-gnueabihf-gcc -I /home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/../lib/gcc/arm-buildroot-linux-gnueabihf/7.5.0/include -L /home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/../lib/gcc/arm-buildroot-linux-gnueabihf/7.5.0/../../../../arm-buildroot-linux-gnueabihf/lib -lts  -o mt_cal_distance mt_cal_distance.c 
```


# 音视频
## 音频
- 录音：`arecord -v --format=cd --device=plughw:0,1 test.wav`
	- 需要耳机来录音
- mic 接口录音：还没测试
- 播放：`aplay -v --format=cd --device=plughw:0,0 music_zhou.wav`
- 测试：`speaker-test -t wav -c 2 -D plughw:0,0`
	- 0,0 表示声卡0，0或1是左声道或右声道
- 修改音量：`alsamixer` 后按下 F3 然后切换到对应的位置来设置音量大小


## 视频
### mplayer

#### alsa-lib
```shell
cd /usr/share/
sudo mkdir arm-alsa

./configure --host=arm-buildroot-linux-gnueabihf --prefix=/home/tanxzh/tools/lib/alsa-lib --with-configdir=/usr/share/arm-alsa

make
sudo make install
```
- 这时会出现编译失败的情况：
![[Pasted image 20250921164920.png]]
- 按照正点原子教程继续以下操作：
```shell
sudo -s  //切换到 root 用户
export ARCH=arm
export CROSS_COMPILE=arm-buildroot-linux-gnueabihf-
export PATH=$PATH:/home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin

cd 到刚刚编译的路径
make install

su tanxzh
```

#### alsa-utils

#### zlib
```shell
CC=arm-buildroot-linux-gnueabihf-gcc
LD=arm-buildroot-linux-gnueabihf-ld
AD=arm-buildroot-linux-gnueabihf-as ./configure --prefix=/home/tanxzh/tools/lib/zlib/

make
make install
```

#### mplayer
```shell
./configure --cc=arm-buildroot-linux-gnueabihf-gcc --host-cc=gcc --target=arm-buildroot-linux-gnueabihf --disable-ossaudio --enable-alsa --prefix=/home/tanxzh/tools/lib/MPlayer/ --extra-cflags="-I /home/tanxzh/tools/lib/zlib/include -I /home/tanxzh/tools/lib/alsa-lib/include" \
--extra-ldflags="-L/home/tanxzh/tools/zlib/lib -Iz -L/home/tanxzh/tools/alsa-lib/lib -lasound" --enable-fbdev --disable-mencoder
make -j8
```
- 修改`config.mak` 文件中 `INSTALLSTRIP=-s`为 `INSTALLSTRIP=`
- 继续安装：`make install`
- 拷贝 `mplayer` 到开发板，直接 `./mplayer`


##### 开发板直接执行 `./mplayer` 缺少库 libz.so.1
- 查看开发板 `/usr/lib` 目录下的库，发现是 64 位的，并且主机上的库也是 64 位的
```
[root@100ask:/tmp/module/mplayer]# ./mplayer 
./mplayer: error while loading shared libraries: libz.so.1: wrong ELF class: ELFCLASS64
```
![[Pasted image 20250921171653.png]]

- 之前配置 zlib 的时候，没有加 `\`，导致使用了主机的编译工具链，重新加上之后再编译一遍 zlib，并拷贝库到开发板之后可以正常运行了
```shell
CC=arm-buildroot-linux-gnueabihf-gcc \
LD=arm-buildroot-linux-gnueabihf-ld \
AD=arm-buildroot-linux-gnueabihf-as ./configure --prefix=/home/tanxzh/tools/lib/zlib/
```


#### 使用 mplayer 不能播放，使用 ffmpeg 可以播放

##### 使用 mplayer 不能正常播放，不知道为什么必须要关掉声音
```shell
./mplayer xxx.mp4 or xxx.avi
# 可以播放，但会闪屏
./mplayer -vo fbdev -framedrop -nosound -lavdopts lowres=1 file_example_AVI_640_800kB.avi  

# 可以播放
 ./mplayer -fs  -nosound file_example_AVI_640_800kB.avi
 
 # 可以播放
 ./mplayer -fs -afm ffmpeg -ac ffmpeg -ao -f^Cmat s16le file_example_AVI_640_800kB.avi
 
 mplayer -fs -afm ffmpeg -ac ffmpeg -ao alsa -format s16le file_example_AVI_640_800kB.avi
```


- 单独使用 mplayer 播放音频也有问题，会一直卡住，应该就是这个导致的
```shell
[root@100ask:/tmp/module/mplayer]# ./mplayer ../audio/file_example_WAV_2MG.wav 
MPlayer 1.4-7.5.0 (C) 2000-2019 MPlayer Team

Playing ../audio/file_example_WAV_2MG.wav.
libavformat version 58.27.102 (internal)
Audio only file format detected.
Load subtitles in ../audio/
==========================================================================
Opening audio decoder: [pcm] Uncompressed PCM audio decoder
AUDIO: 44100 Hz, 2 ch, s16le, 1411.2 kbit/100.00% (ratio: 176400->176400)
Selected audio codec: [pcm] afm: pcm (Uncompressed PCM)
==========================================================================
AO: [alsa] 44100Hz 2ch s16le (2 bytes per sample)
Video: no video
Starting playback...
A:   0.0 (unknown) of 11.0 (11.0) ??,?% $<50>


MPlayer interrupted by signal 2 in module: play_audio
 $<50>
Exiting... (Quit)
```

##### 使用 ffmpeg 不能正常播放：`ffmpeg xxx.avi` 失败
- 切换到下面的命令后正常了
```
[root@100ask:/tmp/module/mplayer]# ffmpeg -i file_example_AVI_640_800kB.avi -pix_fmt bgra -f fbdev /dev/fb0
ffmpeg version N-108120-g37a503ac87 Copyright (c) 2000-2022 the FFmpeg developers
  built with gcc 7.5.0 (Buildroot 2020.02-gee85cab)
  configuration: --cross-prefix=arm-buildroot-linux-gnueabihf- --enable-cross-compile --target-os=linux --cc=arm-buildroot-linux-gnueabihf-gcc --arch=arm --prefix=/home/tanxzh/tanxzh/linux/ffmpeg/ffmpeg/ffmpeg/_install --enable-shared --disable-st
atic --enable-gpl --enable-nonfree --disable-ffplay --enable-swscale --enable-pthreads --disable-armv5te --disable-armv6 --disable-armv6t2 --disable-x86asm --disable-stripping --enable-libx264 --extra-cflags=-I/home/tanxzh/tanxzh/linux/ffmpeg/x264
-master/_install/include --extra-ldflags=-L/home/tanxzh/tanxzh/linux/ffmpeg/x264-master/_install/lib --extra-libs=-ldl --pkg-config='pkg-config --static'
  libavutil      57. 36.101 / 57. 36.101
  libavcodec     59. 43.100 / 59. 43.100
  libavformat    59. 31.100 / 59. 31.100
  libavdevice    59.  8.101 / 59.  8.101
  libavfilter     8. 48.100 /  8. 48.100
  libswscale      6.  8.112 /  6.  8.112
  libswresample   4.  9.100 /  4.  9.100
  libpostproc    56.  7.100 / 56.  7.100
Input #0, avi, from 'file_example_AVI_640_800kB.avi':
  Metadata:
    software        : Lavf57.19.100
  Duration: 00:00:30.61, start: 0.000000, bitrate: 216 kb/s
  Stream #0:0: Video: h264 (High) (H264 / 0x34363248), yuv420p(progressive), 640x360 [SAR 1:1 DAR 16:9], 60 kb/s, 30 fps, 30 tbr, 30 tbn
  Stream #0:1: Audio: aac (LC) ([255][0][0][0] / 0x00FF), 48000 Hz, stereo, fltp, 139 kb/s
Stream mapping:
  Stream #0:0 -> #0:0 (h264 (native) -> rawvideo (native))
Press [q] to stop, [?] for help
[swscaler @ 0x1263a70] No accelerated colorspace conversion found from yuv420p to bgra.
Output #0, fbdev, to '/dev/fb0':
  Metadata:
    software        : Lavf57.19.100
    encoder         : Lavf59.31.100
  Stream #0:0: Video: rawvideo (BGRA / 0x41524742), bgra(pc, gbr/unknown/unknown, progressive), 640x360 [SAR 1:1 DAR 16:9], q=2-31, 221184 kb/s, 30 fps, 30 tbn
    Metadata:
      encoder         : Lavc59.43.100 rawvideo
frame=  901 fps= 50 q=-0.0 Lsize=N/A time=00:00:30.06 bitrate=N/A speed=1.66x     speed=2.29x    
video:810900kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: unknown
```



## 命令
- mplayer
```
# 有画面，有声音
./mplayer -ao alsa -zoom -x 1024 -y 600 trailer.mp4
```

- ffmpeg
```
# 有画面，有声音
ffmpeg -i trailer.mp4 -vf "scale=1024:600" -pix_fmt bgra -f fbdev /dev/fb0 -f alsa -ac 2 -ar 44100 -sample_fmt s16 hw:0,0
```


# lvgl

- 下载仓库： https://github.com/lvgl/lv_port_linux
- 参考 `README.md`
- 触摸屏需要在开发板中配置环境变量：`export LV_LINUX_EVDEV_POINTER_DEVICE=/dev/input/event1`








# 需求设计

## 电子阅读器

电子阅读器要求包括：
- 触摸屏：需要通过 input 子系统获取到触碰事件
- LCD 显示：需要显示文字

## 音乐播放器 + 视频播放器

使用 QT 或 LVGL 框架，实现音乐播放器 + 视频播放器



# 参考 100ask 设计


