# MQTT Player Control Capability

## ADDED Requirements

### Requirement: MQTT 音乐播放器控制

系统 SHALL 支持通过 MQTT 协议远程控制音乐播放器。

#### Scenario: 处理播放控制命令

- **GIVEN** 系统已连接到 MQTT Broker 并订阅 `control/player/#`
- **WHEN** 收到控制命令：
  - `control/player/play` - Payload `"1"` 播放，`"0"` 暂停
  - `control/player/next` - Payload `"1"` 下一首
  - `control/player/prev` - Payload `"1"` 上一首
- **THEN** 系统 SHALL 调用对应的 `audio_manager` API

#### Scenario: 上报播放状态

- **GIVEN** 音乐播放状态发生变化
- **WHEN** 状态变为 playing/paused/stopped
- **THEN** 系统 SHALL 向 `states/player/status` 发布状态 (QoS=1, Retain=true)
- **AND** 系统 SHALL 向 `states/player/track` 发布曲目信息：

```json
{"title": "歌曲名", "artist": "歌手"}
```

#### Scenario: 上报播放进度

- **GIVEN** 音乐正在播放
- **WHEN** 每隔 1-2 秒
- **THEN** 系统 SHALL 向 `states/player/progress` 发布进度 (QoS=0, Retain=false)：

```json
{"current": 120, "total": 300}
```

---

### Requirement: MQTT 连接管理

系统 SHALL 支持 SSL/TLS 异步连接到 EMQX Cloud Broker，避免阻塞 UI 线程。

#### Scenario: 异步 SSL 连接

- **GIVEN** 配置了 Broker 地址和 CA 证书
- **WHEN** 系统启动或用户手动触发连接
- **THEN** 客户端 SHALL 使用 MQTTAsync API 发起非阻塞连接
- **AND** UI 线程 SHALL 不被阻塞，可正常响应用户操作
- **AND** 连接过程中状态显示为 "连接中..."

#### Scenario: 连接成功回调

- **GIVEN** 异步连接请求已发起
- **WHEN** SSL 握手和认证成功完成
- **THEN** 系统 SHALL 触发连接成功回调
- **AND** 更新内部连接状态为 "已连接"
- **AND** 开始订阅控制主题

#### Scenario: 连接失败回调

- **GIVEN** 异步连接请求已发起
- **WHEN** 连接超时或认证失败
- **THEN** 系统 SHALL 触发连接失败回调
- **AND** 更新内部连接状态为 "未连接"
- **AND** 可选：延迟后自动重试

#### Scenario: 自动重连

- **GIVEN** 网络断开
- **WHEN** 连接丢失
- **THEN** 客户端 SHALL 自动尝试重连
- **AND** 重连过程不阻塞 UI

---

### Requirement: MQTT 音量控制

系统 SHALL 支持通过 MQTT 协议远程控制音量大小。

#### Scenario: 接收音量控制命令

- **GIVEN** 系统已连接到 MQTT Broker 并订阅 `control/player/volume`
- **WHEN** 收到音量控制命令，Payload 为 0-100 的整数字符串（如 `"50"`）
- **THEN** 系统 SHALL 调用 `audio_engine_set_volume()` 设置音量
- **AND** 系统 SHALL 更新 UI 音量滑块显示

#### Scenario: 上报音量状态

- **GIVEN** 音量发生变化（本地或远程）
- **WHEN** 音量值改变
- **THEN** 系统 SHALL 向 `states/player/volume` 发布当前音量值（整数字符串）

---

### Requirement: 网页端音量控制

网页端 SHALL 支持音量控制功能。

#### Scenario: 网页端显示音量滑块

- **GIVEN** 网页已连接到 MQTT Broker
- **WHEN** 页面加载完成
- **THEN** 网页 SHALL 显示音量控制滑块（范围 0-100）
- **AND** 滑块应订阅 `states/player/volume` 同步显示当前音量

#### Scenario: 网页端控制音量

- **GIVEN** 用户拖动音量滑块
- **WHEN** 滑块值改变
- **THEN** 网页 SHALL 向 `control/player/volume` 发布新音量值
