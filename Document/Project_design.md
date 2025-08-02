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

## freetype 编译方法

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

# 需求设计

## 电子阅读器

电子阅读器要求包括：
- 触摸屏：需要通过 input 子系统获取到触碰事件
- LCD 显示：需要显示文字