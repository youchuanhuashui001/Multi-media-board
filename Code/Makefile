# 1. 设置变量
CROSS_COMPILE ?= arm-buildroot-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

# 包含freetype2的头文件
CFLAGS := -Ihome/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/include
CFLAGS += -L/home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/lib
LDFLAGS := -lfreetype -lbrotlidec -lbrotlicommon

# 2. 推导变量
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
TARGET := main

# 3. 编译规则
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 4. 清理规则
clean:
	rm -f $(OBJS) $(TARGET)

# 5. 安装规则
install:
	@echo "Installing to /opt/nfs"
	@sudo cp $(TARGET) /opt/nfs

.PHONY: clean install all
