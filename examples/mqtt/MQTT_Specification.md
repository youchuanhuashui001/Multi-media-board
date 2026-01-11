# 基于 i.MX6ULL 与 LVGL 的 MQTT 远程控制系统设计文档

**版本:** 1.0  
**日期:** 2025-12-13  
**适用平台:** NXP i.MX6ULL (ARM A7) / Linux 4.9 / LVGL  
**云平台:** EMQX Cloud (Serverless)

---

## 1. 系统架构概述

本系统采用 **公网云端 Broker (Cloud Mode)** 模式，实现移动端/PC 端对嵌入式设备的远程控制与状态监控。

* **通信协议:** MQTT v3.1.1
* **传输层:** TCP/IP (端口 1883)
* **开发模式:** 异步事件驱动 (Asynchronous Event-driven)
* **网址:** https://cloud.emqx.com/console/deployments/ma41c02e/overview

### 数据流向

1. **下行控制 (Command):** 手机/PC -> `[MQTT Broker]` -> i.MX6ULL (接收指令)
2. **上行状态 (Telemetry):** i.MX6ULL -> `[MQTT Broker] `-> 手机/PC (更新 UI)

---

## 2. Broker 配置信息

| 配置项              | 值                                  | 说明                          |
| :--------------- | :--------------------------------- | :-------------------------- |
| **服务器地址 (Host)** | ma41c02e.ala.cn-hangzhou.emqxsl.cn | EMQX Cloud 概览页面获取           |
| **端口 (Port)**    | `8883/8884`                        | MQTT/WebSocket over TLS/SSL |
| **认证方式**         | Username / Password                | 必须配置，否则无法连接                 |

| 设备     | **客户端名称** | **客户端密码**   | 连接方式                 |
| :----- | --------- | ----------- | -------------------- |
| 开发板    | IMX6ULL_0 | Password001 | mqtt 连接到 Broker      |
| PC 浏览器 | Browser_0 | Password001 | websocket 连接到 Broker |
| 手机     | Android_0 | Password001 | 暂定                   |


---


## 测试部分

## Topic 规划

| 功能描述       | Topic 地址                          | 开发板角色        | QoS | Retain | Payload 格式                                      |
| :----------- | :---------------------------------- | :------------- | :-- | :----- | :----------------------------------------------- |
| **播放控制**   | `control/player/play`       | **订阅 (Sub)**  | 1   | ❌      | `"1"` 播放 / `"0"` 暂停                            |
| **下一首**     | `control/player/next`       | **订阅 (Sub)**  | 1   | ❌      | `"1"` (触发即执行)                                  |
| **上一首**     | `control/player/prev`       | **订阅 (Sub)**  | 1   | ❌      | `"1"` (触发即执行)                                  |
| **播放状态**   | `states/player/status`      | **发布 (Pub)**  | 1   | ✅      | `"playing"` / `"paused"` / `"stopped"`           |
| **当前曲目**   | `states/player/track`       | **发布 (Pub)**  | 1   | ✅      | JSON: `{"title":"歌曲名","artist":"歌手"}`          |
| **播放进度**   | `states/player/progress`    | **发布 (Pub)**  | 0   | ❌      | JSON: `{"current":120,"total":300}` (单位: 秒)     |












## 3. MQTT 协议定义 (Topic & Payload)

### 3.1 Topic 规划

采用 `类别/位置/设备/属性` 的层级结构。

| 功能描述         | Topic 地址                                 | 开发板角色        | QoS | Retain | Payload 格式           |
| :----------- | :--------------------------------------- | :----------- | :-- | :----- | :------------------- |
| **LED 开关控制** | `control/livingroom/ceiling_light/power` | **订阅 (Sub)** | 1   | ❌      | 字符串 `"ON"` 或 `"OFF"` |
| **设备真实状态**   | `states/livingroom/ceiling_light/status` | **发布 (Pub)** | 1   | ✅      | 字符串 `"ON"` 或 `"OFF"` |
| **温度传感器**    | `states/livingroom/sensor/temp`          | **发布 (Pub)** | 0   | ✅      | 字符串 (例如 `"25.5"`)    |

### 3.2 关键机制配置

* **Retain (保留消息):**
    * **启用 (True):** 用于 `states/...` 类型的主题。确保手机 App 刚打开时能立即看到当前灯的状态和温度，而不是显示空白。
    * **禁用 (False):** 用于 `control/...` 类型的主题。防止开发板重启后误执行之前的旧指令。
* **LWT (遗嘱消息):**
    * **Topic:** `states/livingroom/device/online`
    * **Payload:** `"Offline"`
    * **Retain:** True
    * **作用:** 当开发板意外断电/断网时，Broker 自动向所有客户端广播"离线"状态。

---

## 4. 嵌入式软件实现方案 (C 语言)

### 4.1 技术选型

* **MQTT 库:** Eclipse Paho MQTT C (Asynchronous 异步版 `libpaho-mqtt3a`)
* **编译工具:** 交叉编译器 `arm-linux-gnueabihf-gcc`
* **并发模型:** 多线程 + 互斥锁 (Mutex)

### 4.2 数据结构与线程安全

为了解决 MQTT 回调线程与 LVGL 主线程的冲突，使用 **带锁的全局共享内存** 模式。

```c
#include <pthread.h>
#include <stdbool.h>

// 1. 数据定义
struct control {
	bool led_power;     // 控制指令：开关
};

struct state {
	bool led_power;     // 真实状态：开关
	float temp;         // 真实状态：温度
};

struct mqtt_data {
	struct control con; // 存放收到的控制指令
	struct state sta;   // 存放采集的传感器数据
};

// 2. 全局实例
struct mqtt_data shared_data;

// 3. 互斥锁 (保护 shared_data)
pthread_mutex_t data_mutex;
```

### 4.3 核心逻辑流程

#### A. MQTT 接收线程 (后台)

由 Paho 库自动维护。当 `onMessageArrived` 回调触发时：

1. 解析 Payload (例如 `"ON"` -> `true`)。
2. `pthread_mutex_lock(&data_mutex)` **(上锁)**。
3. 写入 `shared_data.con.led_power`。
4. `pthread_mutex_unlock(&data_mutex)` **(解锁)**。

#### B. LVGL 主线程 (UI Loop)

在 `lv_task_handler()` 的主循环中：

1. `pthread_mutex_lock(&data_mutex)` **(上锁)**。
2. 读取 `shared_data.con` 和 `shared_data.sta`。
3. `pthread_mutex_unlock(&data_mutex)` **(解锁)**。
4. **UI 逻辑判断:**
    * 如果 `shared_data.con` 变了 -> 调用硬件驱动控制 LED，并更新 Switch 组件状态。
    * 如果 `shared_data.sta.temp` 变了 -> `lv_label_set_text` 更新温度显示。

---

## 5. 部署与环境准备

### 5.1 库文件部署

确保以下文件存在于开发板的 `/lib` 或 `/usr/lib` 目录下：

* `libpaho-mqtt3a.so` (核心异步库)
* `libpaho-mqtt3a.so.1`

### 5.2 网络配置

开发板必须配置 DNS 才能解析云端域名。

```bash
# 检查网络
ping broker.emqx.io

# 如果报错 bad address，执行：
echo "nameserver 8.8.8.8" > /etc/resolv.conf
```

---

## 6. 测试与验证工具

### 6.1 调试工具推荐

1. **PC 端:** MQTTX (跨平台客户端) - 用于查看所有 Log 数据流。
2. **手机端:** IoT MQTT Panel (Android) - 用于模拟真实用户操作。

### 6.2 验收测试用例

| ID | 测试项 | 操作步骤 | 预期结果 |
| :--- | :--- | :--- | :--- |
| **T01** | **连通性测试** | 启动开发板程序 | MQTTX 显示设备上线 (Online) |
| **T02** | **下行控制** | 手机 App 点击"开灯" | 开发板收到 Payload "ON"，LED 点亮 |
| **T03** | **上行同步** | 遮挡开发板温度传感器 | 手机 App 温度读数发生变化 |
| **T04** | **断网重连** | 拔掉开发板网线 10秒后插回 | 程序自动重连，无需人工干预 |
| **T05** | **Retain 测试** | 开发板断电 -> 手机 App 刷新 | App 依然显示最后一次的温度/开关状态 |

---

## 7. 下一步建议 (Next Steps)

1. **编码阶段:** 基于 `main_async.c`，引入 `shared_data` 和 `mutex`，编写具体的业务逻辑。
2. **UI 整合:** 将上述逻辑合并到包含 `lv_init()` 和 `lv_task_handler()` 的 LVGL 工程中。
3. **驱动对接:** 编写读取板载温度传感器和控制 GPIO LED 的代码，填充进 `shared_data`。
