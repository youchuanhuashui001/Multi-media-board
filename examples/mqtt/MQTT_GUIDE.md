# MQTT 协议学习指南

## 什么是 MQTT？

**MQTT** (Message Queuing Telemetry Transport) 是一种轻量级的**发布/订阅**消息传输协议，专为资源受限的设备和低带宽、高延迟的网络设计。

```
┌─────────────────────────────────────────────────────────────────┐
│                        核心特点                                  │
├─────────────────────────────────────────────────────────────────┤
│  • 轻量级：最小报文仅 2 字节                                      │
│  • 发布/订阅模式：发送方和接收方解耦                               │
│  • QoS 服务质量：确保消息可靠传递                                 │
│  • 遗嘱消息：检测异常断开                                         │
│  • 保留消息：新订阅者立即收到最新状态                              │
└─────────────────────────────────────────────────────────────────┘
```

---

## MQTT 架构：三个核心角色

```
┌──────────────┐                          ┌──────────────┐
│   Publisher  │                          │  Subscriber  │
│   (发布者)    │                          │  (订阅者)    │
│              │                          │              │
│  开发板/传感器 │                          │  PC/手机/云  │
└──────┬───────┘                          └──────┬───────┘
       │                                         │
       │  PUBLISH                      SUBSCRIBE │
       │  (发布消息)                   (订阅主题) │
       ▼                                         ▼
┌─────────────────────────────────────────────────────────┐
│                                                         │
│                    MQTT Broker                          │
│                    (消息代理)                            │
│                                                         │
│   职责：                                                │
│   1. 接收所有客户端的连接                                │
│   2. 接收发布的消息                                      │
│   3. 将消息路由给相应的订阅者                            │
│   4. 管理会话和消息队列                                  │
│                                                         │
│   例如：Mosquitto, EMQX, HiveMQ                         │
└─────────────────────────────────────────────────────────┘
```

### 为什么需要 Broker？

**传统模式（点对点）：**
```
设备A ──────► 设备B     # 直接连接，需要知道对方地址
设备A ──────► 设备C     # 每增加一个设备，连接数 ×N
设备A ──────► 设备D     # 复杂度爆炸！
```

**MQTT 模式（发布/订阅）：**
```
设备A ──┐                        ┌──► 设备B
设备B ──┼───► MQTT Broker ───────┼──► 设备C
设备C ──┤     (统一管理)         └──► 设备D
设备D ──┘

# 优势：
# 1. 设备之间互不知晓，只需知道 Broker 地址
# 2. 新增设备无需修改现有设备
# 3. 一对多、多对多通信变得简单
```

### 谁是客户端？

在 MQTT 中，**所有连接到 Broker 的设备/程序都是客户端**：

```
┌─────────────────────────────────────────────────────────────────┐
│                        MQTT 客户端                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  你的测试场景中：                                                 │
│                                                                 │
│  客户端 1: mqtt_client_test   ───┐                              │
│           (你编译的 C 程序)        │                              │
│                                  │                              │
│  客户端 2: MQTT Explorer      ───┼───► Mosquitto (Broker)       │
│           (桌面 GUI 程序)         │       监听 1883 端口          │
│                                  │                              │
│  客户端 3: mosquitto_sub      ───┤                              │
│           (命令行工具)            │                              │
│                                  │                              │
│  客户端 4: Python 脚本         ───┘                              │
│           (pc_mqtt_tool.py)                                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

关键点：
• Broker 只有一个（Mosquitto），是"服务器"
• 所有其他程序都是"客户端"，可以同时连接多个
• 客户端既可以发布消息，也可以订阅消息，或者两者都做
```

**各种客户端类型：**

| 客户端类型 | 例子 | 说明 |
|-----------|------|------|
| 嵌入式设备 | 开发板、传感器、ESP32 | 资源受限，使用 C/C++ |
| 桌面程序 | MQTT Explorer、自定义应用 | 功能丰富，用于调试 |
| 命令行工具 | mosquitto_pub/sub | 快速测试 |
| 脚本程序 | Python、Node.js 脚本 | 自动化测试/控制 |
| Web 应用 | 网页通过 WebSocket | 浏览器中运行 |
| 手机 App | iOS/Android 应用 | 移动端控制 |

### 客户端如何连接？

连接过程分为 **3 步**：

```
┌─────────────┐                           ┌─────────────┐
│   客户端    │                           │   Broker    │
│             │                           │ (Mosquitto) │
└──────┬──────┘                           └──────┬──────┘
       │                                         │
       │  1. TCP 连接                             │
       │ ────────────────────────────────────►   │
       │    连接到 Broker 的 IP:端口              │
       │    如: tcp://192.168.1.100:1883        │
       │                                         │
       │  2. MQTT CONNECT 报文                   │
       │ ────────────────────────────────────►   │
       │    包含:                                │
       │    • Client ID (客户端唯一标识)          │
       │    • 用户名/密码 (可选)                  │
       │    • 遗嘱消息 (可选)                     │
       │    • Keep Alive 心跳间隔                 │
       │                                         │
       │  3. CONNACK 确认                        │
       │ ◄────────────────────────────────────   │
       │    返回连接结果:                         │
       │    • 0 = 成功                           │
       │    • 其他 = 失败原因                     │
       │                                         │
       ▼                                         ▼
    连接建立，可以开始 PUBLISH/SUBSCRIBE
```

**连接参数详解：**

| 参数 | 说明 | 示例 |
|------|------|------|
| **Broker 地址** | Broker 的 IP 和端口 | `tcp://192.168.1.100:1883` |
| **Client ID** | 客户端唯一标识，不能重复！ | `imx6ull_board_001` |
| **Username** | 用户名（如果 Broker 开启认证） | `admin` |
| **Password** | 密码 | `123456` |
| **Keep Alive** | 心跳间隔（秒），超时无响应则断开 | `20` |
| **Clean Session** | `true`=不保留会话，`false`=恢复离线消息 | `true` |

### 不同语言的连接代码

**C 语言 (mqtt_client_test.c):**
```c
#include "MQTTClient.h"

// 1. 指定 Broker 地址和客户端 ID
#define BROKER_ADDRESS  "tcp://localhost:1883"  // Broker 的 IP:端口
#define CLIENT_ID       "my_device_001"          // 唯一标识这个客户端

int main() {
    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    
    // 2. 创建客户端
    MQTTClient_create(&client, BROKER_ADDRESS, CLIENT_ID, 
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    
    // 3. 配置连接选项
    opts.keepAliveInterval = 20;   // 每 20 秒发送心跳
    opts.cleansession = 1;         // 干净会话
    // opts.username = "user";     // 用户名（如果需要）
    // opts.password = "pass";     // 密码（如果需要）
    
    // 4. 发起连接
    int rc = MQTTClient_connect(client, &opts);
    if (rc == MQTTCLIENT_SUCCESS) {
        printf("连接成功！\n");
    } else {
        printf("连接失败，错误码: %d\n", rc);
    }
    
    // 5. 订阅主题
    MQTTClient_subscribe(client, "multimedia/control/#", 1);
    
    // ... 处理消息 ...
    
    // 6. 断开连接
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}
```

**Python (pc_mqtt_tool.py):**
```python
import paho.mqtt.client as mqtt

# 1. 创建客户端，指定 Client ID
client = mqtt.Client("my_python_client")

# 2. 设置回调函数
def on_connect(client, userdata, flags, rc):
    print(f"连接结果: {rc}")
    client.subscribe("multimedia/status/#")  # 订阅

def on_message(client, userdata, msg):
    print(f"收到: {msg.topic} -> {msg.payload}")

client.on_connect = on_connect
client.on_message = on_message

# 3. 连接到 Broker
client.connect("localhost", 1883, 60)
#              └─ IP地址   └─端口  └─心跳间隔(秒)

# 4. 开始循环接收消息
client.loop_forever()
```

**命令行 (mosquitto_sub):**
```bash
# 连接并订阅
mosquitto_sub -h localhost -p 1883 -t "multimedia/#" -v
#             └─ Broker IP  └─端口  └─ 订阅的主题

# 带用户名密码
mosquitto_sub -h 192.168.1.100 -p 1883 -u user -P pass -t "#"

# 连接并发布
mosquitto_pub -h localhost -t "multimedia/test" -m "Hello"
```

### 你的测试中发生了什么？

```
当你运行 ./mqtt_client_test 时，完整的流程如下：

┌────────────────────────────────────────────────────────────────────┐
│  1. 程序启动                                                        │
│     MQTTClient_create() 创建客户端实例                               │
│     Client ID = "imx6ull_multimedia_board"                         │
└────────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌────────────────────────────────────────────────────────────────────┐
│  2. TCP 连接                                                        │
│     程序向 localhost:1883 发起 TCP 三次握手                          │
│     Mosquitto 接受连接                                              │
└────────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌────────────────────────────────────────────────────────────────────┐
│  3. MQTT CONNECT                                                    │
│     程序发送 CONNECT 报文:                                           │
│       - Client ID: imx6ull_multimedia_board                        │
│       - Keep Alive: 20 秒                                          │
│       - 遗嘱主题: multimedia/status/online                          │
│       - 遗嘱内容: {"online": false}                                 │
│                                                                    │
│     Mosquitto 返回 CONNACK (code=0, 成功)                           │
└────────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌────────────────────────────────────────────────────────────────────┐
│  4. SUBSCRIBE 订阅                                                  │
│     程序发送 SUBSCRIBE 报文:                                         │
│       - 主题: "multimedia/control/#" (通配符，匹配所有子主题)          │
│       - QoS: 1                                                     │
│                                                                    │
│     Mosquitto 返回 SUBACK (订阅成功)                                │
│     Broker 记住: 这个客户端对 multimedia/control/# 感兴趣            │
└────────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌────────────────────────────────────────────────────────────────────┐
│  5. 等待消息                                                        │
│     程序进入循环，等待回调                                            │
│     同时每 20 秒自动发送 PINGREQ 心跳，Broker 回复 PINGRESP           │
└────────────────────────────────────────────────────────────────────┘
                            │
    此时，你在 MQTT Explorer 中发布消息
                            │
                            ▼
┌────────────────────────────────────────────────────────────────────┐
│  6. 消息路由                                                        │
│     MQTT Explorer 发布到 "multimedia/control/audio"                 │
│     Mosquitto 收到消息，检查订阅列表:                                 │
│       - mqtt_client_test 订阅了 "multimedia/control/#" ✓ 匹配！      │
│     Mosquitto 将消息转发给 mqtt_client_test                         │
└────────────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌────────────────────────────────────────────────────────────────────┐
│  7. 回调触发                                                        │
│     程序的 message_arrived() 函数被调用:                             │
│       - topicName = "multimedia/control/audio"                     │
│       - payload = {"action": "play"}                               │
│     程序打印收到的消息，执行相应操作                                   │
└────────────────────────────────────────────────────────────────────┘
```

---

## 核心概念详解

### 1. 主题 (Topic)

主题是消息的"地址"，使用层级结构（类似文件路径）：

```
多媒体开发板/
├── control/              # 控制命令
│   ├── audio/            # 音频控制
│   │   ├── play          # 播放
│   │   ├── pause         # 暂停
│   │   └── volume        # 音量
│   └── view/             # 视图控制
│       └── switch        # 切换视图
│
└── status/               # 状态上报
    ├── audio/            # 音频状态
    │   ├── playing       # 播放状态
    │   └── progress      # 进度
    └── system/           # 系统状态
        └── online        # 在线状态
```

**主题通配符：**
```bash
# + 匹配单层
control/audio/+          # 匹配 control/audio/play, control/audio/pause

# # 匹配多层（只能放末尾）
control/#                 # 匹配 control/ 下的所有主题
multimedia/#              # 匹配所有 multimedia 开头的主题
```

### 2. 发布 (Publish) 和 订阅 (Subscribe)

```
时间线 ─────────────────────────────────────────────────────────►

    开发板                    Broker                      PC
      │                         │                         │
      │                         │                         │
      │   ──── SUBSCRIBE ─────► │                         │
      │        "control/#"      │                         │
      │                         │                         │
      │                         │ ◄─── SUBSCRIBE ─────    │
      │                         │      "status/#"         │
      │                         │                         │
      │                         │ ◄─── PUBLISH ────────   │
      │                         │      "control/audio"    │
      │                         │      {"action":"play"}  │
      │                         │                         │
      │   ◄─── PUBLISH ──────   │                         │
      │        "control/audio"  │                         │
      │        {"action":"play"}│                         │
      │                         │                         │
      │   ──── PUBLISH ───────► │                         │
      │        "status/audio"   │                         │
      │        {"playing":true} │                         │
      │                         │                         │
      │                         │ ──── PUBLISH ────────►  │
      │                         │      "status/audio"     │
      │                         │      {"playing":true}   │
      ▼                         ▼                         ▼
```

### 3. QoS 服务质量等级

| 级别 | 名称 | 描述 | 适用场景 |
|------|------|------|----------|
| 0 | At most once | 最多一次，可能丢失 | 传感器周期上报（丢一条无所谓） |
| 1 | At least once | 至少一次，可能重复 | 大多数场景，重复可接受 |
| 2 | Exactly once | 恰好一次，可靠但慢 | 金融交易、计费（不能重复也不能丢） |

**QoS 0 流程：**
```
Publisher ──── PUBLISH ────► Broker ──── PUBLISH ────► Subscriber
               (发完就忘)              (转发就忘)
```

**QoS 1 流程：**
```
Publisher ──── PUBLISH ────► Broker ──── PUBLISH ────► Subscriber
               ◄──── PUBACK ────              ◄──── PUBACK ────
               (收到确认)                      (收到确认)
```

### 4. 遗嘱消息 (Last Will and Testament, LWT)

当客户端**异常断开**时，Broker 自动发布预设的消息：

```
1. 连接时设置遗嘱
┌──────────────┐    CONNECT + LWT                ┌──────────────┐
│   开发板     │ ──────────────────────────────► │    Broker    │
│              │    遗嘱主题: status/online      │              │
│              │    遗嘱内容: {"online":false}   │              │
└──────────────┘                                 └──────────────┘

2. 意外断开时 Broker 自动发布遗嘱
┌──────────────┐                                 ┌──────────────┐
│   开发板     │  ═══════ 断电/网络中断 ═══════   │    Broker    │
│              │         (没有发送 DISCONNECT)    │              │
└──────────────┘                                 └──────┬───────┘
                                                        │
                                                        ▼ 自动发布
                                         PUBLISH "status/online"
                                                {"online":false}
                                                        │
                                         ┌──────────────┘
                                         ▼
                                 ┌──────────────┐
                                 │      PC      │
                                 │  收到下线通知 │
                                 └──────────────┘
```

### 5. 保留消息 (Retained Message)

新订阅者会**立即收到**该主题的最新保留消息：

```
时间线 ─────────────────────────────────────────────────────────►

1. 设备发布保留消息
开发板 ──── PUBLISH (retained=true) ────► Broker
            "status/online"
            {"online":true}
                                          │
                                          ▼ 存储保留消息

2. 1小时后，新设备订阅
                                 PC ──── SUBSCRIBE ────► Broker
                                         "status/online"
                                                          │
                                                          ▼ 立即发送保留消息
                                 PC ◄──── PUBLISH ────────
                                         {"online":true}
                                         (不需要等开发板再发一次)
```

---

## 你的测试场景完整流程

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          你的测试场景                                    │
└─────────────────────────────────────────────────────────────────────────┘

步骤 1: 启动 Mosquitto Broker
┌────────────────────────────────────────────────────────────────────────┐
│ PC$ sudo systemctl start mosquitto                                     │
│                                                                        │
│   Mosquitto 开始监听 1883 端口，等待客户端连接                           │
└────────────────────────────────────────────────────────────────────────┘

步骤 2: 运行测试程序 (mqtt_client_test)
┌────────────────────────────────────────────────────────────────────────┐
│ PC$ ./mqtt_client_test                                                 │
│                                                                        │
│   1. 创建 MQTT 客户端                                                   │
│   2. 连接到 Broker (localhost:1883)                                    │
│   3. 设置遗嘱消息 (status/online → offline)                             │
│   4. 订阅 "multimedia/control/#" 主题                                   │
│   5. 进入循环，等待消息                                                  │
└────────────────────────────────────────────────────────────────────────┘

步骤 3: 使用 MQTT Explorer 发送消息
┌────────────────────────────────────────────────────────────────────────┐
│   MQTT Explorer:                                                       │
│   1. 连接到 localhost:1883                                             │
│   2. 发布消息到 "multimedia/control/audio"                              │
│   3. 内容: {"action": "play"}                                          │
│                                                                        │
│   数据流:                                                               │
│   MQTT Explorer ──PUBLISH──► Mosquitto ──PUBLISH──► mqtt_client_test   │
│                              (匹配订阅)                                  │
└────────────────────────────────────────────────────────────────────────┘

步骤 4: 测试程序收到消息
┌────────────────────────────────────────────────────────────────────────┐
│   mqtt_client_test 输出:                                               │
│   [收到消息]                                                            │
│     主题: multimedia/control/audio                                     │
│     内容: {"action": "play"}                                           │
│     -> 处理音频控制命令                                                  │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 代码对照理解

```c
// mqtt_client_test.c 中的关键代码

// 1. 创建客户端
MQTTClient_create(&client, "tcp://localhost:1883", "my_client_id", ...);
//                         └─── Broker 地址        └─── 客户端唯一标识

// 2. 设置回调函数
MQTTClient_setCallbacks(client, NULL, 
    connection_lost,     // 连接断开时调用
    message_arrived,     // 收到消息时调用  ◄── 核心！处理消息的地方
    delivery_complete);  // 发送完成时调用

// 3. 配置遗嘱消息
MQTTClient_willOptions will_opts;
will_opts.topicName = "status/online";
will_opts.message = "{\"online\": false}";
conn_opts.will = &will_opts;

// 4. 连接到 Broker
MQTTClient_connect(client, &conn_opts);

// 5. 订阅主题（使用通配符订阅所有控制主题）
MQTTClient_subscribe(client, "multimedia/control/#", QOS);

// 6. 消息到达回调
int message_arrived(void *context, char *topicName, int topicLen,
                    MQTTClient_message *message) 
{
    // topicName = "multimedia/control/audio"
    // message->payload = {"action": "play"}
    
    // 在这里处理收到的命令
    if (strstr(topicName, "audio")) {
        // 调用音频控制函数
    }
    return 1;  // 返回 1 表示消息已处理
}

// 7. 发布消息
MQTTClient_publish(client, "status/audio", payload, strlen(payload), QOS, 0, NULL);
//                         └─── 主题      └─── 消息内容                └─ retained
```

---

## MQTT 报文格式（进阶）

MQTT 使用二进制协议，每个报文结构：

```
┌─────────────────────────────────────────────────────────────────┐
│                        Fixed Header (固定头)                     │
├────────────────────────────────┬────────────────────────────────┤
│   Byte 1: 报文类型 + 标志位      │   Byte 2+: 剩余长度             │
├────────────────────────────────┴────────────────────────────────┤
│                      Variable Header (可变头)                    │
├─────────────────────────────────────────────────────────────────┤
│                        Payload (有效载荷)                        │
└─────────────────────────────────────────────────────────────────┘
```

**报文类型：**

| 类型 | 值 | 描述 |
|------|-----|------|
| CONNECT | 1 | 客户端请求连接 |
| CONNACK | 2 | 服务端连接确认 |
| PUBLISH | 3 | 发布消息 |
| PUBACK | 4 | 发布确认（QoS 1） |
| SUBSCRIBE | 8 | 订阅请求 |
| SUBACK | 9 | 订阅确认 |
| DISCONNECT | 14 | 断开连接 |

---

## 与 HTTP 对比

| 特性 | MQTT | HTTP |
|------|------|------|
| 模式 | 发布/订阅 | 请求/响应 |
| 连接 | 长连接 | 短连接（或 Keep-Alive） |
| 头部开销 | 2 字节起 | 数百字节 |
| 推送 | 原生支持 | 需要 WebSocket/SSE |
| 适用场景 | IoT、实时通信 | Web 应用、API |

---

## 学习资源

1. **官方规范**: https://mqtt.org/mqtt-specification/
2. **HiveMQ 教程**: https://www.hivemq.com/mqtt-essentials/
3. **Mosquitto 文档**: https://mosquitto.org/man/
4. **Paho C 文档**: https://www.eclipse.org/paho/files/mqttdoc/MQTTClient/html/index.html

---

## 练习建议

1. **尝试不同 QoS**：观察 QoS 0/1 的区别
2. **测试遗嘱消息**：强制杀掉客户端进程（Ctrl+C），观察遗嘱消息
3. **使用保留消息**：发布 retained=true 的消息，再新连接一个客户端
4. **主题通配符**：订阅 `#` 查看所有消息流

```bash
# 订阅所有消息
mosquitto_sub -t "#" -v

# 发布保留消息
mosquitto_pub -t "test/retained" -m "I am retained" -r

# 新客户端订阅（会立即收到保留消息）
mosquitto_sub -t "test/retained" -v
```
