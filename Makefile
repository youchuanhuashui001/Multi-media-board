#
# Makefile
#
#-> % arm-buildroot-linux-gnueabihf-gcc mqtt_test/main_test.c -o mqtt_test_app \
#  -I paho.mqtt.c/src \
#  -L paho.mqtt.c/build_arm/src \
#  -l paho-mqtt3cs \
#  -lpthread -ldl

CROSS_COMPILE   = arm-buildroot-linux-gnueabihf-
CC              = $(CROSS_COMPILE)gcc
CXX             = $(CROSS_COMPILE)g++
LVGL_DIR_NAME   ?= lvgl
LVGL_DIR        ?= .

WARNINGS        := -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith \
                   -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers \
                   -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security \
                   -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wclobbered -Wdeprecated -Wempty-body \
                   -Wshift-negative-value -Wno-unused-value

CFLAGS          ?= -O3 -g0 -I$(LVGL_DIR)/ $(WARNINGS)
LDFLAGS         ?= -lm -pthread

CFLAGS          += -I./src/
CFLAGS          += -I./lvgl/
CFLAGS          += -I./src/include -I./src/include/app/ -I./src/include/common/

# freetype, tslib
CFLAGS          += -I/home/tanxzh/tools/lib/freetype/include
LDFLAGS         += -L/home/tanxzh/tools/lib/freetype/lib
# brotli
LDFLAGS         += -L/home/tanxzh/project/100ask/100ask_imx6ull_sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/lib
LDFLAGS         += -lfreetype -lbrotlidec -lbrotlicommon -lts

# ffmpeg
CFLAGS          += -I/home/tanxzh/project/100ask/100ask_imx6ull_sdk/Buildroot_2020.02.x/output/host/arm-buildroot-linux-gnueabihf/sysroot/home/tanxzh/tools/lib/ffmpeg/include
LDFLAGS         += -L/home/tanxzh/project/100ask/100ask_imx6ull_sdk/Buildroot_2020.02.x/output/host/arm-buildroot-linux-gnueabihf/sysroot/home/tanxzh/tools/lib/ffmpeg/lib
LDFLAGS         += -lavutil -lavformat -lavcodec -lavdevice -lswresample -lasound

# mqtt
CFLAGS          += -I/home/tanxzh/tanxzh/code/MQTT/paho.mqtt.c/src
LDFLAGS         += -L/home/tanxzh/tanxzh/code/MQTT/paho.mqtt.c/build_arm/src
LDFLAGS         += -lpaho-mqtt3as -ldl


BIN             = main
BUILD_DIR       = ./build
BUILD_OBJ_DIR   = $(BUILD_DIR)/obj
BUILD_BIN_DIR   = $(BUILD_DIR)/bin

prefix          ?= /usr
bindir          ?= /opt/nfs

# Collect source files recursively
CSRCS           := $(shell find src -type f -name '*.c')
CXXSRCS         := $(shell find src -type f -name '*.cpp')

# Include LVGL sources
include $(LVGL_DIR)/lvgl/lvgl.mk

OBJEXT          ?= .o

COBJS           = $(CSRCS:.c=$(OBJEXT))
CXXOBJS         = $(CXXSRCS:.cpp=$(OBJEXT))
AOBJS           = $(ASRCS:.S=$(OBJEXT))

SRCS            = $(ASRCS) $(CSRCS) $(CXXSRCS)
OBJS            = $(AOBJS) $(COBJS) $(CXXOBJS)
TARGET          = $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

all: default

$(BUILD_OBJ_DIR)/%.o: %.c lv_conf.h
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC  $<"

$(BUILD_OBJ_DIR)/%.o: %.cpp lv_conf.h
	@mkdir -p $(dir $@)
	@$(CXX)  $(CFLAGS) -c $< -o $@
	@echo "CXX $<"

$(BUILD_OBJ_DIR)/%.o: %.S lv_conf.h
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "AS  $<"

default: $(TARGET)
	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
	@$(CXX) -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

install:
	sudo cp $(BUILD_BIN_DIR)/$(BIN) $(bindir)/$(BIN)
#install:
#	install -d $(DESTDIR)$(bindir)
#	install $(BUILD_BIN_DIR)/$(BIN) $(DESTDIR)$(bindir)

uninstall:
	$(RM) -r $(addprefix $(DESTDIR)$(bindir)/,$(BIN))
