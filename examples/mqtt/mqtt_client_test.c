/*
 * mqtt_client_test.c - MQTT 客户端测试程序
 *
 * 功能：演示 MQTT 连接、发布、订阅基本功能
 * 使用 Eclipse Paho MQTT C Client 库
 *
 * 编译：make
 * 运行：./mqtt_client_test [broker_address]
 *
 * 2024 Multi-media-board Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "MQTTClient.h"

/* MQTT 默认配置 */
#define DEFAULT_BROKER_ADDRESS	"tcp://localhost:1883"
#define CLIENT_ID		"imx6ull_multimedia_board"
#define QOS			1
#define TIMEOUT			10000L

/* 主题定义 */
#define TOPIC_CONTROL_AUDIO	"multimedia/control/audio"
#define TOPIC_CONTROL_VIEW	"multimedia/control/view"
#define TOPIC_STATUS		"multimedia/status"
#define TOPIC_TEST		"multimedia/test"

/* 全局变量 */
static volatile int running = 1;
static MQTTClient client;

/* 信号处理：优雅退出 */
void signal_handler(int sig)
{
	printf("\n收到信号 %d，正在退出...\n", sig);
	running = 0;
}

/* 连接丢失回调 */
void connection_lost(void *context, char *cause)
{
	printf("[警告] 连接丢失: %s\n", cause ? cause : "未知原因");
	/* 这里可以实现自动重连逻辑 */
}

/* 消息到达回调 */
int message_arrived(void *context, char *topicName, int topicLen,
		    MQTTClient_message *message)
{
	char *payload = (char *)message->payload;

	printf("\n[收到消息]\n");
	printf("  主题: %s\n", topicName);
	printf("  内容: %.*s\n", message->payloadlen, payload);
	printf("  QoS: %d\n", message->qos);

	/* 根据主题处理消息 */
	if (strstr(topicName, "control/audio") != NULL) {
		printf("  -> 处理音频控制命令\n");
		/* TODO: 调用音频控制函数 */
	} else if (strstr(topicName, "control/view") != NULL) {
		printf("  -> 处理视图切换命令\n");
		/* TODO: 调用视图切换函数 */
	} else if (strstr(topicName, "test") != NULL) {
		printf("  -> 这是测试消息\n");
	}

	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	return 1;
}

/* 消息发送完成回调 */
void delivery_complete(void *context, MQTTClient_deliveryToken dt)
{
	printf("[发送完成] DeliveryToken: %d\n", dt);
}

/* 发布消息 */
int publish_message(const char *topic, const char *payload)
{
	MQTTClient_message msg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	int rc;

	msg.payload = (void *)payload;
	msg.payloadlen = strlen(payload);
	msg.qos = QOS;
	msg.retained = 0;

	rc = MQTTClient_publishMessage(client, topic, &msg, &token);
	if (rc != MQTTCLIENT_SUCCESS) {
		printf("[错误] 发布失败，返回码: %d\n", rc);
		return rc;
	}

	printf("[发布] 主题: %s, 内容: %s\n", topic, payload);

	/* 等待消息发送完成 */
	rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
	if (rc != MQTTCLIENT_SUCCESS) {
		printf("[错误] 等待发送完成超时\n");
		return rc;
	}

	return MQTTCLIENT_SUCCESS;
}

/* 订阅主题 */
int subscribe_topics(void)
{
	int rc;

	/* 订阅控制主题（使用通配符） */
	rc = MQTTClient_subscribe(client, "multimedia/control/#", QOS);
	if (rc != MQTTCLIENT_SUCCESS) {
		printf("[错误] 订阅 control 主题失败: %d\n", rc);
		return rc;
	}
	printf("[订阅] multimedia/control/#\n");

	/* 订阅测试主题 */
	rc = MQTTClient_subscribe(client, TOPIC_TEST, QOS);
	if (rc != MQTTCLIENT_SUCCESS) {
		printf("[错误] 订阅 test 主题失败: %d\n", rc);
		return rc;
	}
	printf("[订阅] %s\n", TOPIC_TEST);

	return MQTTCLIENT_SUCCESS;
}

/* 打印使用说明 */
void print_usage(const char *prog_name)
{
	printf("用法: %s [broker_address]\n", prog_name);
	printf("示例:\n");
	printf("  %s                         # 连接到 localhost:1883\n", prog_name);
	printf("  %s tcp://192.168.1.100:1883  # 连接到指定地址\n", prog_name);
	printf("\n");
	printf("测试方法:\n");
	printf("  1. 在 PC 端启动 MQTT Broker (如 mosquitto)\n");
	printf("  2. 运行此程序\n");
	printf("  3. 使用 mosquitto_pub 发送测试消息:\n");
	printf("     mosquitto_pub -t multimedia/control/audio -m '{\"action\":\"play\"}'\n");
	printf("     mosquitto_pub -t multimedia/test -m 'Hello MQTT!'\n");
}

int main(int argc, char *argv[])
{
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	const char *broker_address;
	int rc;

	/* 解析命令行参数 */
	if (argc > 1) {
		if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
			print_usage(argv[0]);
			return 0;
		}
		broker_address = argv[1];
	} else {
		broker_address = DEFAULT_BROKER_ADDRESS;
	}

	printf("=== MQTT 客户端测试程序 ===\n");
	printf("Broker 地址: %s\n", broker_address);
	printf("客户端 ID: %s\n", CLIENT_ID);
	printf("\n");

	/* 注册信号处理 */
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* 创建 MQTT 客户端 */
	rc = MQTTClient_create(&client, broker_address, CLIENT_ID,
			       MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTCLIENT_SUCCESS) {
		printf("[错误] 创建客户端失败: %d\n", rc);
		return EXIT_FAILURE;
	}

	/* 设置回调函数 */
	rc = MQTTClient_setCallbacks(client, NULL, connection_lost,
				     message_arrived, delivery_complete);
	if (rc != MQTTCLIENT_SUCCESS) {
		printf("[错误] 设置回调失败: %d\n", rc);
		MQTTClient_destroy(&client);
		return EXIT_FAILURE;
	}

	/* 配置连接选项 */
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	/* 配置遗嘱消息（LWT） */
	MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
	will_opts.topicName = "multimedia/status/online";
	will_opts.message = "{\"online\": false}";
	will_opts.qos = 1;
	will_opts.retained = 1;
	conn_opts.will = &will_opts;

	/* 连接到 Broker */
	printf("正在连接到 MQTT Broker...\n");
	rc = MQTTClient_connect(client, &conn_opts);
	if (rc != MQTTCLIENT_SUCCESS) {
		printf("[错误] 连接失败: %d\n", rc);
		printf("请确保 MQTT Broker 已启动\n");
		MQTTClient_destroy(&client);
		return EXIT_FAILURE;
	}
	printf("[成功] 已连接到 Broker\n\n");

	/* 发布上线状态 */
	publish_message("multimedia/status/online", "{\"online\": true}");

	/* 订阅主题 */
	if (subscribe_topics() != MQTTCLIENT_SUCCESS) {
		MQTTClient_disconnect(client, TIMEOUT);
		MQTTClient_destroy(&client);
		return EXIT_FAILURE;
	}

	printf("\n等待消息中... (按 Ctrl+C 退出)\n");
	printf("-----------------------------------\n");

	/* 主循环：等待消息 */
	int counter = 0;
	while (running) {
		/* 每 10 秒发送一次心跳/状态消息 */
		sleep(1);
		counter++;
		if (counter >= 10) {
			char status_msg[128];
			snprintf(status_msg, sizeof(status_msg),
				 "{\"type\": \"heartbeat\", \"count\": %d}",
				 counter / 10);
			publish_message(TOPIC_STATUS, status_msg);
			counter = 0;
		}
	}

	/* 发布下线状态 */
	publish_message("multimedia/status/online", "{\"online\": false}");

	/* 断开连接并清理 */
	printf("\n正在断开连接...\n");
	MQTTClient_disconnect(client, TIMEOUT);
	MQTTClient_destroy(&client);
	printf("已断开连接，程序退出\n");

	return EXIT_SUCCESS;
}
