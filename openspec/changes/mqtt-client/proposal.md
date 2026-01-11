# Change: 集成 MQTT 音乐播放器远程控制

## Why

实现手机/PC 通过 MQTT 协议远程控制开发板上的音乐播放器，包括播放/暂停、切换歌曲、音量控制等功能，同时开发板上报播放状态供远程端显示。并在控制中心提供 MQTT 连接状态显示和开关控制。

## What Changes

- **新增** MQTT 客户端模块 (`src/app/core/mqtt_client.c/h`)
- **修改** 使用 MQTTAsync API 实现**异步连接**，避免阻塞 UI 线程
- **修改** 构建系统，添加 `paho-mqtt3as` 库依赖（异步版本）
- **修改** `main.c`，初始化 MQTT 客户端
- **新增** 控制中心 MQTT 状态显示和开关控制 (`src/app/ui/system_bar.c`)
- **新增** MQTT 音量控制功能
- **修改** 网页端添加音量控制滑块 (`tools/mqtt/mqtt.html`)

## Impact

- **Affected specs**: `control-center` (新增 MQTT 控制)
- **Affected code**:
  - `Makefile` - 添加 MQTT 异步库链接 (`paho-mqtt3as`)
  - `src/main.c` - 初始化 MQTT 客户端
  - `src/app/core/mqtt_client.c`:
    - 使用 `MQTTAsync` API 替代 `MQTTClient`
    - 实现异步连接回调 (`onConnectSuccess`, `onConnectFailure`)
    - 实现断线回调 (`onConnectionLost`)
    - 新增连接状态查询接口
  - `src/include/app/mqtt_client.h`:
    - 定义连接状态枚举 (`MQTT_DISCONNECTED`, `MQTT_CONNECTING`, `MQTT_CONNECTED`)
    - 新增状态回调类型和注册接口
  - `src/app/ui/system_bar.c` - 新增 MQTT 状态显示（含连接中状态）和开关控件
  - `src/app/ui/audio_view.c` - 新增音量控制 MQTT 回调
  - `tools/mqtt/mqtt.html` - 新增音量控制滑块
- **Dependencies**: paho.mqtt.c 库（异步 SSL 版本 `paho-mqtt3as`）

## References

- 设计文档: `examples/mqtt/MQTT_Specification.md`
- 参考代码: `/home/tanxzh/tanxzh/code/MQTT/mqtt_test/main_test.c`
