# MQTT 客户端实现任务清单

> **参考代码**: `/home/tanxzh/tanxzh/code/MQTT/mqtt_test/main_test.c`
> **编译命令**: `arm-buildroot-linux-gnueabihf-gcc -lpaho-mqtt3as -lpthread -ldl`（异步 SSL 版本）

## 1. MQTT 客户端模块

- [x] 1.1 创建 `src/include/app/mqtt_client.h`
  - [x] 使用 `MQTTAsync` 替代 `MQTTClient`
  - [x] 定义连接状态枚举 (DISCONNECTED, CONNECTING, CONNECTED)
  - [x] 定义连接状态回调类型
- [x] 1.2 创建 `src/app/core/mqtt_client.c`
  - [x] 使用 MQTTAsync API 实现**异步非阻塞连接**
  - [x] 实现连接成功回调 `onConnectSuccess()`
  - [x] 实现连接失败回调 `onConnectFailure()`
  - [x] 实现断开连接回调 `onConnectionLost()`
  - [x] SSL 连接到 EMQX Cloud (8883)
  - [x] 订阅 `control/player/#`
  - [x] 消息回调处理

## 2. 音频控制命令处理

- [x] 2.1 处理 `control/player/play` → `audio_manager_play()`/`pause()`
- [x] 2.2 处理 `control/player/next` → `audio_manager_play_next()`
- [x] 2.3 处理 `control/player/prev` → `audio_manager_play_prev()`

## 3. 状态上报

- [x] 3.1 发布 `states/player/status` (playing/paused/stopped)
- [x] 3.2 发布 `states/player/track` (曲目信息 JSON)
- [ ] 3.3 发布 `states/player/progress` (播放进度 JSON)

## 4. 主程序集成

- [ ] 4.1 修改 `main.c` 初始化 MQTT
- [x] 4.2 修改 `Makefile` 添加库链接

## 5. 控制中心 MQTT 状态显示与开关

- [ ] 5.1 修改 `mqtt_client.h`，新增接口：
  - `mqtt_client_is_connected()` - 获取连接状态
  - `mqtt_client_connect()` - 手动连接
  - `mqtt_client_disconnect()` - 断开连接
- [ ] 5.2 修改 `mqtt_client.c`，实现上述接口
- [ ] 5.3 修改 `system_bar.c`：
  - 添加 MQTT 状态指示（绿点/红点）
  - 添加 MQTT 连接/断开开关按钮
  - 定时器轮询更新状态显示

## 6. MQTT 音量控制

- [x] 6.1 修改 `mqtt_client.h`：
  - 新增 `MQTT_CLIENT_VOLUME_TOPIC "control/player/volume"`
- [x] 6.2 修改 `mqtt_client.c`：
  - 新增音量回调变量和订阅处理
- [x] 6.3 修改 `audio_view.c`：
  - 订阅音量 Topic
  - 实现音量回调：解析 payload 并调用 `audio_engine_set_volume()`
  - 音量变化时上报 `states/player/volume`

## 7. 网页端音量控制

- [x] 7.1 修改 `tools/mqtt/mqtt.html`：
  - 添加音量控制滑块 UI
  - 订阅 `states/player/volume` 同步音量显示
  - 滑块拖动时发布 `control/player/volume`

## 8. 验证

- [ ] 8.1 编译通过
- [ ] 8.2 使用 MQTTX 测试控制命令
- [ ] 8.3 验证状态上报
- [ ] 8.4 验证控制中心 MQTT 状态显示与开关
- [ ] 8.5 验证网页端音量控制
