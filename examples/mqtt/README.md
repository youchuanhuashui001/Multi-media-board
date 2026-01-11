# MQTT 测试指南

本目录包含 MQTT 客户端测试代码，用于验证开发板与 PC 之间的 MQTT 通信。

## 文件说明

| 文件 | 说明 |
|------|------|
| `mqtt_client_test.c` | 同步 API 测试程序（简单易懂） |
| `mqtt_async_test.c` | 异步 API 测试程序（适合集成） |
| `pc_mqtt_tool.py` | PC 端 Python 测试工具 |
| `Makefile` | 编译脚本 |

## 快速开始

### 1. 准备环境

**PC 端安装 MQTT Broker 和客户端库：**

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install mosquitto mosquitto-clients libpaho-mqtt-dev python3-pip
pip3 install paho-mqtt
```

### 2. 启动 MQTT Broker（PC 端）

```bash
# 启动 mosquitto 服务
sudo systemctl start mosquitto
# 停止 mosquitto 服务
sudo systemctl stop mosquitto

# 或前台运行查看日志
mosquitto -v
```

### 3. 编译测试程序

**PC 本地编译（用于调试）：**
```bash
cd examples/mqtt
make native
```

**交叉编译（用于开发板）：**
```bash
make cross
```

### 4. 运行测试

#### 方式一：使用 Python 工具测试

**终端 1 - 启动监听：**
```bash
python3 pc_mqtt_tool.py listen
```

**终端 2 - 运行 C 测试程序：**
```bash
./mqtt_client_test
# 或
./mqtt_async_test
```

**终端 3 - 发送控制命令：**
```bash
python3 pc_mqtt_tool.py control
# 进入交互模式后输入：play, pause, next, prev, vol 50, view audio 等
```

#### 方式二：使用 mosquitto_pub/sub 测试

**终端 1 - 订阅所有消息：**
```bash
mosquitto_sub -t "multimedia/#" -v
```

**终端 2 - 运行 C 测试程序：**
```bash
./mqtt_client_test
```

**终端 3 - 发送命令：**
```bash
# 发送播放命令
mosquitto_pub -t "multimedia/control/audio" -m '{"action": "play"}'

# 发送暂停命令
mosquitto_pub -t "multimedia/control/audio" -m '{"action": "pause"}'

# 切换视图
mosquitto_pub -t "multimedia/control/view" -m '{"action": "switch", "view": "audio"}'

# 发送测试消息
mosquitto_pub -t "multimedia/test" -m "Hello from PC!"
```

## 开发板部署

### 1. 确认 Buildroot 已启用 paho.mqtt.c

检查 SDK 中是否有库文件：
```bash
ls /home/book/100ask_imx6ull-sdk/Buildroot_2020.02.x/output/host/arm-buildroot-linux-gnueabihf/sysroot/usr/lib/libpaho*
```

如果没有，需要在 Buildroot 中启用：
```
make menuconfig
# Target packages -> Libraries -> Networking -> paho-mqtt-c
```

### 2. 交叉编译

```bash
make cross
```

### 3. 复制到开发板

```bash
scp mqtt_client_test_arm root@<开发板IP>:/tmp/
scp mqtt_async_test_arm root@<开发板IP>:/tmp/
```

### 4. 在开发板上运行

```bash
# 连接到 PC 的 MQTT Broker
./mqtt_client_test_arm tcp://<PC_IP>:1883
```

## 主题设计

```
multimedia/
├── control/           # PC -> 开发板
│   ├── audio          # 音频控制 {"action": "play/pause/next/prev/volume"}
│   ├── view           # 视图切换 {"action": "switch", "view": "main/audio/book"}
│   └── system         # 系统控制
└── status/            # 开发板 -> PC
    ├── online         # 在线状态 {"online": true/false}
    └── (其他状态)
```

## 服务器端详细操作指南

### 安装 MQTT Broker (Mosquitto)

**Ubuntu/Debian：**
```bash
# 安装 mosquitto broker 和 客户端工具
sudo apt update
sudo apt install mosquitto mosquitto-clients

# 验证安装
mosquitto -h
```

**CentOS/RHEL：**
```bash
sudo yum install epel-release
sudo yum install mosquitto
```

### 启动 MQTT 服务

**方式一：使用 systemd 服务（推荐）**
```bash
# 启动服务
sudo systemctl start mosquitto

# 设置开机自启
sudo systemctl enable mosquitto

# 查看服务状态
sudo systemctl status mosquitto
```

**方式二：前台运行（调试用）**
```bash
# 前台运行，显示详细日志
mosquitto -v
```

### 查看 MQTT 端口

**默认端口：**
- `1883` - 标准 MQTT 端口（无加密）
- `8883` - MQTT over TLS（加密）
- `9001` - WebSocket（用于浏览器）

**确认端口监听状态：**
```bash
# 方法1：使用 netstat
sudo netstat -tlnp | grep mosquitto

# 方法2：使用 ss
sudo ss -tlnp | grep 1883

# 方法3：使用 lsof
sudo lsof -i :1883
```

**输出示例：**
```
tcp        0      0 0.0.0.0:1883            0.0.0.0:*               LISTEN      1234/mosquitto
```

**测试端口连通性：**
```bash
# 本地测试
nc -zv localhost 1883

# 从开发板测试（替换为 PC 的 IP）
nc -zv 192.168.1.100 1883
```

### 防火墙配置

```bash
# Ubuntu (ufw)
sudo ufw allow 1883/tcp
sudo ufw allow 9001/tcp  # 如果使用 WebSocket

# CentOS (firewalld)
sudo firewall-cmd --permanent --add-port=1883/tcp
sudo firewall-cmd --permanent --add-port=9001/tcp
sudo firewall-cmd --reload
```

### 配置 Mosquitto

配置文件位置：`/etc/mosquitto/mosquitto.conf`

**基础配置示例：**
```bash
sudo tee /etc/mosquitto/conf.d/custom.conf << 'EOF'
# 监听所有网络接口（允许远程连接）
listener 1883
allow_anonymous true

# 启用 WebSocket（用于浏览器访问）
listener 9001
protocol websockets
allow_anonymous true

# 日志配置
log_type all
log_dest file /var/log/mosquitto/mosquitto.log
EOF

# 重启服务生效
sudo systemctl restart mosquitto
```

### 在浏览器中查看 MQTT

由于 MQTT 是二进制协议，无法直接在浏览器地址栏查看。需要使用专门的 Web 客户端：

#### 方法一：使用 MQTT Explorer（推荐，桌面应用）

```bash
# 下载安装 MQTT Explorer
# Ubuntu
sudo snap install mqtt-explorer

# 或从官网下载：https://mqtt-explorer.com/
```

**配置连接：**
1. 打开 MQTT Explorer
2. 点击 "+" 添加连接
3. 填写：
   - Host: `localhost`（或服务器 IP）
   - Port: `1883`
4. 点击 "Connect"

#### 方法二：使用在线 MQTT Web 客户端

1. 先确保 Mosquitto 启用了 WebSocket（端口 9001）
2. 打开以下任一网站：
   - http://www.hivemq.com/demos/websocket-client/
   - https://testclient-cloud.mqtt.cool/

3. 连接配置：
   - Host: `ws://你的PC_IP`
   - Port: `9001`
   - 点击 "Connect"

4. 订阅主题：
   - 输入 `multimedia/#`
   - 点击 "Subscribe"

#### 方法三：本地部署 MQTT Web 客户端

```bash
# 使用 Docker 部署 MQTT-Web-Client
docker run -d \
    --name mqtt-web-client \
    -p 8080:80 \
    eclipse-mosquitto/mqtt-web-client

# 浏览器访问 http://localhost:8080
```

### 命令行快速测试

**终端 1 - 订阅所有消息：**
```bash
mosquitto_sub -h localhost -t "multimedia/#" -v
```

**终端 2 - 发布测试消息：**
```bash
# 发布简单消息
mosquitto_pub -h localhost -t "multimedia/test" -m "Hello MQTT!"

# 发布 JSON 控制命令
mosquitto_pub -h localhost -t "multimedia/control/audio" -m '{"action":"play"}'
mosquitto_pub -h localhost -t "multimedia/control/audio" -m '{"action":"pause"}'
mosquitto_pub -h localhost -t "multimedia/control/view" -m '{"action":"switch","view":"audio"}'
```

### 查看 MQTT 日志

```bash
# 实时查看日志
sudo tail -f /var/log/mosquitto/mosquitto.log

# 或者如果配置了 systemd journal
sudo journalctl -u mosquitto -f
```

### 常见服务器问题排查

**问题：服务无法启动**
```bash
# 检查配置文件语法
mosquitto -c /etc/mosquitto/mosquitto.conf -v

# 查看错误日志
sudo journalctl -u mosquitto --no-pager -n 50
```

**问题：远程设备无法连接**
1. 检查防火墙：`sudo ufw status`
2. 检查是否监听所有接口：`sudo netstat -tlnp | grep 1883`
3. 检查配置是否允许匿名：`allow_anonymous true`

---

## 常见问题

### 连接失败

1. 检查 mosquitto 服务是否运行：
   ```bash
   sudo systemctl status mosquitto
   ```

2. 检查防火墙是否放行 1883 端口：
   ```bash
   sudo ufw allow 1883
   ```

3. 检查 IP 地址是否正确

### 编译错误：找不到 MQTTClient.h

安装 paho-mqtt 开发库：
```bash
sudo apt install libpaho-mqtt-dev
```

### Python 脚本报错

安装依赖：
```bash
pip3 install paho-mqtt
```
