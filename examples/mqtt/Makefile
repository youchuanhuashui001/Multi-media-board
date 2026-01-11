# MQTT 测试程序 Makefile
#
# 编译说明：
# 1. PC 本地编译（用于调试）：make native
# 2. 交叉编译（用于开发板）：make cross
# 3. 清理：make clean
#
# 依赖：
# - Eclipse Paho MQTT C Client（paho.mqtt.c）
# - 安装方法（Ubuntu）：sudo apt install libpaho-mqtt-dev
# - Buildroot：启用 BR2_PACKAGE_PAHO_MQTT_C

# 交叉编译工具链
CROSS_COMPILE ?= arm-buildroot-linux-gnueabihf-

# 编译器
CC_NATIVE = gcc
CC_CROSS = $(CROSS_COMPILE)gcc

# 编译选项
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = -lpaho-mqtt3c -lpaho-mqtt3a -lpthread

# SDK 路径（交叉编译时使用）
SDK_PATH ?= /home/book/100ask_imx6ull-sdk/Buildroot_2020.02.x/output
SDK_INCLUDE = $(SDK_PATH)/host/arm-buildroot-linux-gnueabihf/sysroot/usr/include
SDK_LIB = $(SDK_PATH)/host/arm-buildroot-linux-gnueabihf/sysroot/usr/lib

# 目标文件
TARGETS_NATIVE = mqtt_client_test mqtt_async_test
TARGETS_CROSS = mqtt_client_test_arm mqtt_async_test_arm

# 源文件
SRCS = mqtt_client_test.c mqtt_async_test.c

.PHONY: all native cross clean help

# 默认：本地编译
all: native

# 本地编译（PC）
native: $(TARGETS_NATIVE)

mqtt_client_test: mqtt_client_test.c
	$(CC_NATIVE) $(CFLAGS) -o $@ $< -lpaho-mqtt3c -lpthread
	@echo "[本地编译] $@ 完成"

mqtt_async_test: mqtt_async_test.c
	$(CC_NATIVE) $(CFLAGS) -o $@ $< -lpaho-mqtt3a -lpthread
	@echo "[本地编译] $@ 完成"

# 交叉编译（开发板）
cross: $(TARGETS_CROSS)

mqtt_client_test_arm: mqtt_client_test.c
	$(CC_CROSS) $(CFLAGS) -I$(SDK_INCLUDE) -L$(SDK_LIB) -o $@ $< -lpaho-mqtt3c -lpthread
	@echo "[交叉编译] $@ 完成"

mqtt_async_test_arm: mqtt_async_test.c
	$(CC_CROSS) $(CFLAGS) -I$(SDK_INCLUDE) -L$(SDK_LIB) -o $@ $< -lpaho-mqtt3a -lpthread
	@echo "[交叉编译] $@ 完成"

# 清理
clean:
	rm -f $(TARGETS_NATIVE) $(TARGETS_CROSS)
	@echo "已清理"

# 帮助
help:
	@echo "MQTT 测试程序 Makefile"
	@echo ""
	@echo "目标："
	@echo "  make native  - 本地编译（PC，用于调试）"
	@echo "  make cross   - 交叉编译（开发板）"
	@echo "  make clean   - 清理编译产物"
	@echo "  make help    - 显示此帮助"
	@echo ""
	@echo "环境变量："
	@echo "  CROSS_COMPILE - 交叉编译前缀（默认：arm-buildroot-linux-gnueabihf-）"
	@echo "  SDK_PATH      - SDK 路径（默认：/home/book/100ask_imx6ull-sdk/Buildroot_2020.02.x/output）"
	@echo ""
	@echo "依赖安装（PC）："
	@echo "  sudo apt install libpaho-mqtt-dev"
