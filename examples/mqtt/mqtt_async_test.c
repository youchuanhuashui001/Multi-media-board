/*
 * mqtt_async_test.c - MQTT 异步客户端测试程序
 *
 * 功能：演示 MQTT 异步连接模式，更适合嵌入式系统
 * 使用 Eclipse Paho MQTT C Client 异步 API
 *
 * 编译：make
 * 运行：./mqtt_async_test [broker_address]
 *
 * 2024 Multi-media-board Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "MQTTAsync.h"

/* MQTT 默认配置 */
#define DEFAULT_BROKER_ADDRESS	"tcp://localhost:1883"
#define CLIENT_ID		"imx6ull_async_test"
#define QOS			1
#define TIMEOUT			10000L

/* 主题定义 */
#define TOPIC_CONTROL		"multimedia/control/#"
#define TOPIC_STATUS		"multimedia/status"

/* 全局状态 */
static volatile int running = 1;
static volatile int connected = 0;
static volatile int disconnected = 0;
static MQTTAsync client;

/* 信号处理 */
void signal_handler(int sig)
{
	printf("\n收到信号 %d，正在退出...\n", sig);
	running = 0;
}

/* 连接成功回调 */
void on_connect(void *context, MQTTAsync_successData *response)
{
	int rc;

	printf("[成功] 已连接到 Broker\n");
	connected = 1;

	/* 订阅控制主题 */
	printf("[订阅] %s\n", TOPIC_CONTROL);
	rc = MQTTAsync_subscribe(client, TOPIC_CONTROL, QOS, NULL);
	if (rc != MQTTASYNC_SUCCESS) {
		printf("[错误] 订阅失败: %d\n", rc);
	}
}

/* 连接失败回调 */
void on_connect_failure(void *context, MQTTAsync_failureData *response)
{
	printf("[错误] 连接失败");
	if (response) {
		printf(", 错误码: %d", response->code);
		if (response->message) {
			printf(", 消息: %s", response->message);
		}
	}
	printf("\n");
	connected = 0;
}

/* 断开连接回调 */
void on_disconnect(void *context, MQTTAsync_successData *response)
{
	printf("[信息] 已断开连接\n");
	disconnected = 1;
}

/* 连接丢失回调 */
void connection_lost(void *context, char *cause)
{
	printf("[警告] 连接丢失: %s\n", cause ? cause : "未知原因");
	connected = 0;

	/* 自动重连 */
	printf("[信息] 5 秒后尝试重连...\n");
	sleep(5);

	if (running) {
		MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
		conn_opts.keepAliveInterval = 20;
		conn_opts.cleansession = 1;
		conn_opts.onSuccess = on_connect;
		conn_opts.onFailure = on_connect_failure;

		int rc = MQTTAsync_connect(client, &conn_opts);
		if (rc != MQTTASYNC_SUCCESS) {
			printf("[错误] 重连失败: %d\n", rc);
		}
	}
}

/* 消息到达回调 */
int message_arrived(void *context, char *topicName, int topicLen,
		    MQTTAsync_message *message)
{
	char payload[256];
	int len = message->payloadlen < 255 ? message->payloadlen : 255;

	memcpy(payload, message->payload, len);
	payload[len] = '\0';

	printf("\n[收到消息] 主题: %s\n", topicName);
	printf("           内容: %s\n", payload);

	/* 简单的命令解析示例 */
	if (strstr(topicName, "audio") != NULL) {
		if (strstr(payload, "play") != NULL) {
			printf("           -> 执行: 播放\n");
		} else if (strstr(payload, "pause") != NULL) {
			printf("           -> 执行: 暂停\n");
		} else if (strstr(payload, "next") != NULL) {
			printf("           -> 执行: 下一曲\n");
		} else if (strstr(payload, "prev") != NULL) {
			printf("           -> 执行: 上一曲\n");
		}
	} else if (strstr(topicName, "view") != NULL) {
		if (strstr(payload, "main") != NULL) {
			printf("           -> 切换到: 主页\n");
		} else if (strstr(payload, "audio") != NULL) {
			printf("           -> 切换到: 音乐\n");
		} else if (strstr(payload, "book") != NULL) {
			printf("           -> 切换到: 电子书\n");
		}
	}

	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);
	return 1;
}

/* 发布回调 */
void on_send(void *context, MQTTAsync_successData *response)
{
	/* 发布成功，可选：记录日志 */
}

/* 异步发布消息 */
int publish_async(const char *topic, const char *payload)
{
	MQTTAsync_message msg = MQTTAsync_message_initializer;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;

	if (!connected) {
		printf("[警告] 未连接，无法发布\n");
		return -1;
	}

	msg.payload = (void *)payload;
	msg.payloadlen = strlen(payload);
	msg.qos = QOS;
	msg.retained = 0;

	opts.onSuccess = on_send;
	opts.context = client;

	rc = MQTTAsync_sendMessage(client, topic, &msg, &opts);
	if (rc != MQTTASYNC_SUCCESS) {
		printf("[错误] 发布失败: %d\n", rc);
		return rc;
	}

	return MQTTASYNC_SUCCESS;
}

int main(int argc, char *argv[])
{
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	const char *broker_address;
	int rc;

	/* 解析命令行参数 */
	if (argc > 1) {
		broker_address = argv[1];
	} else {
		broker_address = DEFAULT_BROKER_ADDRESS;
	}

	printf("=== MQTT 异步客户端测试 ===\n");
	printf("Broker 地址: %s\n", broker_address);
	printf("\n");

	/* 注册信号处理 */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* 创建异步客户端 */
	rc = MQTTAsync_create(&client, broker_address, CLIENT_ID,
			      MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTASYNC_SUCCESS) {
		printf("[错误] 创建客户端失败: %d\n", rc);
		return EXIT_FAILURE;
	}

	/* 设置回调 */
	rc = MQTTAsync_setCallbacks(client, NULL, connection_lost,
				    message_arrived, NULL);
	if (rc != MQTTASYNC_SUCCESS) {
		printf("[错误] 设置回调失败: %d\n", rc);
		MQTTAsync_destroy(&client);
		return EXIT_FAILURE;
	}

	/* 配置连接选项 */
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = on_connect;
	conn_opts.onFailure = on_connect_failure;
	conn_opts.context = client;

	/* 遗嘱消息 */
	MQTTAsync_willOptions will_opts = MQTTAsync_willOptions_initializer;
	will_opts.topicName = "multimedia/status/online";
	will_opts.message = "{\"online\": false}";
	will_opts.qos = 1;
	will_opts.retained = 1;
	conn_opts.will = &will_opts;

	/* 异步连接 */
	printf("正在连接...\n");
	rc = MQTTAsync_connect(client, &conn_opts);
	if (rc != MQTTASYNC_SUCCESS) {
		printf("[错误] 连接启动失败: %d\n", rc);
		MQTTAsync_destroy(&client);
		return EXIT_FAILURE;
	}

	/* 等待连接完成 */
	int wait_count = 0;
	while (!connected && wait_count < 10) {
		usleep(500000);
		wait_count++;
	}

	if (!connected) {
		printf("[错误] 连接超时\n");
		MQTTAsync_destroy(&client);
		return EXIT_FAILURE;
	}

	/* 发布上线状态 */
	publish_async("multimedia/status/online", "{\"online\": true}");

	printf("\n等待消息中... (按 Ctrl+C 退出)\n");
	printf("每 5 秒发送一次心跳\n");
	printf("-----------------------------------\n");

	/* 主循环 */
	int heartbeat = 0;
	while (running) {
		sleep(1);
		heartbeat++;

		if (heartbeat >= 5 && connected) {
			char msg[64];
			snprintf(msg, sizeof(msg), "{\"heartbeat\": %d}", heartbeat / 5);
			publish_async(TOPIC_STATUS, msg);
			heartbeat = 0;
		}
	}

	/* 发布下线状态并断开 */
	if (connected) {
		publish_async("multimedia/status/online", "{\"online\": false}");
		usleep(500000);  /* 等待消息发送 */

		MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
		disc_opts.onSuccess = on_disconnect;
		disc_opts.context = client;

		MQTTAsync_disconnect(client, &disc_opts);

		/* 等待断开完成 */
		wait_count = 0;
		while (!disconnected && wait_count < 10) {
			usleep(100000);
			wait_count++;
		}
	}

	MQTTAsync_destroy(&client);
	printf("程序退出\n");

	return EXIT_SUCCESS;
}
