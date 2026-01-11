#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PC 端 MQTT 测试工具

功能：
1. 启动 MQTT Broker（需要安装 mosquitto）
2. 发送控制命令到开发板
3. 接收开发板的状态消息

使用方法：
    python3 pc_mqtt_tool.py [command]

命令：
    listen    - 监听开发板消息
    control   - 发送控制命令（交互模式）
    play      - 发送播放命令
    pause     - 发送暂停命令
    next      - 发送下一曲命令
    prev      - 发送上一曲命令
    volume N  - 设置音量 (0-100)
    view NAME - 切换视图 (main/audio/book)

依赖：
    pip install paho-mqtt

2024 Multi-media-board Project
"""

import argparse
import json
import sys
import time
import signal
from datetime import datetime

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("错误: 请安装 paho-mqtt 库")
    print("运行: pip install paho-mqtt")
    sys.exit(1)

# MQTT 配置
BROKER_HOST = "localhost"
BROKER_PORT = 1883
CLIENT_ID = "pc_mqtt_tool"

# 主题定义
TOPIC_CONTROL_AUDIO = "multimedia/control/audio"
TOPIC_CONTROL_VIEW = "multimedia/control/view"
TOPIC_CONTROL_SYSTEM = "multimedia/control/system"
TOPIC_STATUS = "multimedia/status"
TOPIC_STATUS_ONLINE = "multimedia/status/online"

# 全局变量
running = True


def signal_handler(sig, frame):
    """信号处理"""
    global running
    print("\n正在退出...")
    running = False


def on_connect(client, userdata, flags, rc):
    """连接回调"""
    if rc == 0:
        print(f"[{datetime.now().strftime('%H:%M:%S')}] 已连接到 MQTT Broker")
        # 订阅所有状态主题
        client.subscribe("multimedia/status/#")
        client.subscribe("multimedia/test")
        print("已订阅: multimedia/status/# 和 multimedia/test")
    else:
        print(f"连接失败，返回码: {rc}")


def on_message(client, userdata, msg):
    """消息回调"""
    try:
        payload = msg.payload.decode('utf-8')
        timestamp = datetime.now().strftime('%H:%M:%S')
        print(f"\n[{timestamp}] 收到消息:")
        print(f"  主题: {msg.topic}")
        
        # 尝试解析 JSON
        try:
            data = json.loads(payload)
            print(f"  内容: {json.dumps(data, indent=4, ensure_ascii=False)}")
        except json.JSONDecodeError:
            print(f"  内容: {payload}")
    except Exception as e:
        print(f"处理消息错误: {e}")


def create_client():
    """创建 MQTT 客户端"""
    client = mqtt.Client(CLIENT_ID)
    client.on_connect = on_connect
    client.on_message = on_message
    return client


def listen_mode(client):
    """监听模式：持续接收开发板消息"""
    print("=== MQTT 监听模式 ===")
    print(f"Broker: {BROKER_HOST}:{BROKER_PORT}")
    print("按 Ctrl+C 退出\n")
    
    try:
        client.connect(BROKER_HOST, BROKER_PORT, 60)
        client.loop_start()
        
        while running:
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        pass
    finally:
        client.loop_stop()
        client.disconnect()


def send_command(client, topic, payload, description=""):
    """发送命令"""
    try:
        client.connect(BROKER_HOST, BROKER_PORT, 60)
        
        if isinstance(payload, dict):
            payload = json.dumps(payload)
        
        result = client.publish(topic, payload, qos=1)
        result.wait_for_publish()
        
        print(f"[已发送] {description if description else topic}")
        print(f"  主题: {topic}")
        print(f"  内容: {payload}")
        
        client.disconnect()
        return True
    except Exception as e:
        print(f"发送失败: {e}")
        return False


def control_mode(client):
    """交互控制模式"""
    print("=== MQTT 控制模式 ===")
    print(f"Broker: {BROKER_HOST}:{BROKER_PORT}")
    print("\n可用命令:")
    print("  play    - 播放")
    print("  pause   - 暂停")
    print("  next    - 下一曲")
    print("  prev    - 上一曲")
    print("  vol N   - 设置音量 (0-100)")
    print("  view X  - 切换视图 (main/audio/book)")
    print("  test M  - 发送测试消息")
    print("  quit    - 退出")
    print("")
    
    try:
        client.connect(BROKER_HOST, BROKER_PORT, 60)
        
        while running:
            try:
                cmd = input(">>> ").strip().lower()
                
                if not cmd:
                    continue
                elif cmd == "quit" or cmd == "q":
                    break
                elif cmd == "play":
                    send_command(client, TOPIC_CONTROL_AUDIO, 
                                {"action": "play"}, "播放")
                elif cmd == "pause":
                    send_command(client, TOPIC_CONTROL_AUDIO, 
                                {"action": "pause"}, "暂停")
                elif cmd == "next":
                    send_command(client, TOPIC_CONTROL_AUDIO, 
                                {"action": "next"}, "下一曲")
                elif cmd == "prev":
                    send_command(client, TOPIC_CONTROL_AUDIO, 
                                {"action": "prev"}, "上一曲")
                elif cmd.startswith("vol "):
                    try:
                        vol = int(cmd.split()[1])
                        vol = max(0, min(100, vol))
                        send_command(client, TOPIC_CONTROL_AUDIO, 
                                    {"action": "volume", "value": vol}, 
                                    f"音量 {vol}")
                    except (IndexError, ValueError):
                        print("用法: vol [0-100]")
                elif cmd.startswith("view "):
                    try:
                        view = cmd.split()[1]
                        if view in ["main", "audio", "book"]:
                            send_command(client, TOPIC_CONTROL_VIEW, 
                                        {"action": "switch", "view": view}, 
                                        f"切换到 {view}")
                        else:
                            print("可用视图: main, audio, book")
                    except IndexError:
                        print("用法: view [main/audio/book]")
                elif cmd.startswith("test "):
                    msg = cmd[5:]
                    send_command(client, "multimedia/test", msg, "测试消息")
                else:
                    print(f"未知命令: {cmd}")
                    
            except EOFError:
                break
                
    except KeyboardInterrupt:
        pass
    finally:
        client.disconnect()


def main():
    parser = argparse.ArgumentParser(description='PC 端 MQTT 测试工具')
    parser.add_argument('command', nargs='?', default='listen',
                       help='命令: listen, control, play, pause, next, prev, volume, view')
    parser.add_argument('args', nargs='*', help='命令参数')
    parser.add_argument('--host', default=BROKER_HOST, help='MQTT Broker 地址')
    parser.add_argument('--port', type=int, default=BROKER_PORT, help='MQTT Broker 端口')
    
    args = parser.parse_args()
    
    global BROKER_HOST, BROKER_PORT
    BROKER_HOST = args.host
    BROKER_PORT = args.port
    
    signal.signal(signal.SIGINT, signal_handler)
    
    client = create_client()
    
    if args.command == 'listen':
        listen_mode(client)
    elif args.command == 'control':
        control_mode(client)
    elif args.command == 'play':
        send_command(client, TOPIC_CONTROL_AUDIO, {"action": "play"}, "播放")
    elif args.command == 'pause':
        send_command(client, TOPIC_CONTROL_AUDIO, {"action": "pause"}, "暂停")
    elif args.command == 'next':
        send_command(client, TOPIC_CONTROL_AUDIO, {"action": "next"}, "下一曲")
    elif args.command == 'prev':
        send_command(client, TOPIC_CONTROL_AUDIO, {"action": "prev"}, "上一曲")
    elif args.command == 'volume':
        if args.args:
            vol = int(args.args[0])
            send_command(client, TOPIC_CONTROL_AUDIO, 
                        {"action": "volume", "value": vol}, f"音量 {vol}")
        else:
            print("用法: pc_mqtt_tool.py volume [0-100]")
    elif args.command == 'view':
        if args.args:
            view = args.args[0]
            send_command(client, TOPIC_CONTROL_VIEW, 
                        {"action": "switch", "view": view}, f"切换视图 {view}")
        else:
            print("用法: pc_mqtt_tool.py view [main/audio/book]")
    else:
        print(f"未知命令: {args.command}")
        parser.print_help()


if __name__ == '__main__':
    main()
