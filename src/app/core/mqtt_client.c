#include "mqtt_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 全局客户端实例 */
static MQTTAsync client = NULL;
static mqtt_connection_state_t mqtt_state = MQTT_STATE_DISCONNECTED;
static mqtt_state_callback_t state_change_callback = NULL;

/**
 * @brief 更新连接状态并通知回调
 */
static void mqtt_update_state(mqtt_connection_state_t new_state)
{
	mqtt_state = new_state;
	printf("[MQTT] 状态变更: %s\n",
	       new_state == MQTT_STATE_DISCONNECTED ? "未连接" :
	       new_state == MQTT_STATE_CONNECTING ? "连接中" : "已连接");
	if (state_change_callback != NULL) {
		state_change_callback(new_state);
	}
}

/**
 * @brief 连接成功回调（异步）
 */
static void on_connect_success(void *context, MQTTAsync_successData *response)
{
	(void)context;
	(void)response;
	printf("[MQTT] ✅ 连接成功!\n");

	// 上报连接状态
	mqtt_update_state(MQTT_STATE_CONNECTED);
	
	// 发布设备在线状态
	mqtt_client_publish(MQTT_CLIENT_DEVICE_STATUS_TOPIC, "online");
	
	// 上报音量状态
	char volume_payload[16];
	snprintf(volume_payload, sizeof(volume_payload), "%d", audio_engine_get_volume());
	mqtt_client_publish(MQTT_CLIENT_VOLUME_TOPIC, volume_payload);
}

/**
 * @brief 连接失败回调（异步）
 */
static void on_connect_failure(void *context, MQTTAsync_failureData *response)
{
	(void)context;
	printf("[MQTT] ❌ 连接失败，错误码: %d\n", response ? response->code : 0);
	mqtt_update_state(MQTT_STATE_DISCONNECTED);
}

/**
 * @brief 连接断开回调
 */
static void on_connection_lost(void *context, char *cause)
{
	(void)context;
	printf("[MQTT] ⚠️ 连接断开，原因: %s\n", cause ? cause : "未知");
	mqtt_update_state(MQTT_STATE_DISCONNECTED);

	/* TODO: 可在此实现自动重连 */
}

/**
 * @brief 消息到达回调(当前无订阅，仅上报状态)
 */
static int on_message_arrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
	(void)context;
	(void)topicLen;

	char *payload = (char *)message->payload;
	int payload_len = message->payloadlen;

	/* 创建以 null 结尾的 payload 副本 */
	char *payload_str = (char *)malloc(payload_len + 1);
	if (payload_str != NULL) {
		memcpy(payload_str, payload, payload_len);
		payload_str[payload_len] = '\0';

		printf("[MQTT] 📩 收到消息: Topic=%s, Payload=%s\n", topicName, payload_str);
		/* 当前设计不接受控制命令，仅上报状态 */
		free(payload_str);
	}

	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);
	return 1;
}

/**
 * @brief 订阅成功回调
 */
static void on_subscribe_success(void *context, MQTTAsync_successData *response)
{
	(void)context;
	(void)response;
	printf("[MQTT] 订阅成功\n");
}

/**
 * @brief 订阅失败回调
 */
static void on_subscribe_failure(void *context, MQTTAsync_failureData *response)
{
	(void)context;
	printf("[MQTT] 订阅失败，错误码: %d\n", response ? response->code : 0);
}

/**
 * @brief 发起异步连接（不阻塞）
 */
static void mqtt_client_connect(void)
{
	int rc;

	if (client == NULL) {
		printf("[MQTT] ❌ 客户端未初始化\n");
		return;
	}

	if (mqtt_state == MQTT_STATE_CONNECTING) {
		printf("[MQTT] ⏳ 正在连接中，请稍候...\n");
		return;
	}

	if (mqtt_state == MQTT_STATE_CONNECTED) {
		printf("[MQTT] ✅ 已连接，无需重复连接\n");
		return;
	}

	/* 配置连接选项 */
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	conn_opts.username = MQTT_CLIENT_USERNAME;
	conn_opts.password = MQTT_CLIENT_PASSWORD;
	conn_opts.keepAliveInterval = 60;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = on_connect_success;
	conn_opts.onFailure = on_connect_failure;
	conn_opts.context = client;

	/* 配置 SSL 选项 */
	MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
	ssl_opts.trustStore = MQTT_CLIENT_CA_CERTIFICATE_FILE;
	ssl_opts.enableServerCertAuth = 1;
	conn_opts.ssl = &ssl_opts;

	/* 配置 Will Message (遗愿消息)，断线时自动发布 offline */
	MQTTAsync_willOptions will_opts = MQTTAsync_willOptions_initializer;
	will_opts.topicName = MQTT_CLIENT_DEVICE_STATUS_TOPIC;
	will_opts.message = "offline";
	will_opts.retained = 1;  /* 保留消息，新订阅者能立即获取状态 */
	will_opts.qos = 1;
	conn_opts.will = &will_opts;

	printf("[MQTT] 🚀 发起异步连接...\n");
	mqtt_update_state(MQTT_STATE_CONNECTING);

	rc = MQTTAsync_connect(client, &conn_opts);
	if (rc != MQTTASYNC_SUCCESS) {
		printf("[MQTT] ❌ 发起连接失败，错误码: %d\n", rc);
		mqtt_update_state(MQTT_STATE_DISCONNECTED);
	}
}

/**
 * @brief 初始化 MQTT 客户端（仅创建，不连接）
 */
void mqtt_client_init(void)
{
	int rc;

	printf("=== MQTT 异步客户端初始化 ===\n");
	printf("Broker: %s\n", MQTT_CLIENT_ADDRESS);
	printf("Client ID: %s\n", MQTT_CLIENT_ID);
	printf("Username: %s\n", MQTT_CLIENT_USERNAME);

	/* 检查 CA 证书文件是否存在 */
	FILE *fp = fopen(MQTT_CLIENT_CA_CERTIFICATE_FILE, "r");
	if (fp == NULL) {
		printf("[MQTT] ❌ CA 证书文件不存在: %s\n", MQTT_CLIENT_CA_CERTIFICATE_FILE);
		return;
	}
	fclose(fp);
	printf("CA Certificate: %s (OK)\n", MQTT_CLIENT_CA_CERTIFICATE_FILE);

	/* 创建异步客户端 */
	rc = MQTTAsync_create(&client, MQTT_CLIENT_ADDRESS, MQTT_CLIENT_ID,
	                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTASYNC_SUCCESS) {
		printf("[MQTT] ❌ 创建客户端失败，错误码: %d\n", rc);
		return;
	}

	/* 设置全局回调（断线、消息到达） */
	MQTTAsync_setCallbacks(client, NULL, on_connection_lost, on_message_arrived, NULL);

	/* 发起异步连接 */
	mqtt_client_connect();

	printf("[MQTT] 客户端初始化完成，异步连接中...\n");
}

/**
 * @brief 发布消息
 */
void mqtt_client_publish(char *topic, char *payload)
{
	if (mqtt_state != MQTT_STATE_CONNECTED) {
		printf("[MQTT] ⚠️ 未连接，无法发布消息\n");
		return;
	}

	MQTTAsync_message msg = MQTTAsync_message_initializer;
	msg.payload = payload;
	msg.payloadlen = strlen(payload);
	msg.qos = MQTT_CLIENT_QOS;
	msg.retained = 0;

	MQTTAsync_sendMessage(client, topic, &msg, NULL);
	printf("[MQTT] 📤 发送: Topic=%s, Payload=%s\n", topic, payload);
}

/**
 * @brief 订阅 Topic (当前设计不使用，保留接口)
 */
void mqtt_client_subscribe(char *topic, mqtt_client_callback_t callback)
{
	(void)callback;  /* 当前不使用回调 */

	/* 如果已连接，立即订阅 */
	if (mqtt_state == MQTT_STATE_CONNECTED && client != NULL) {
		MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
		opts.onSuccess = on_subscribe_success;
		opts.onFailure = on_subscribe_failure;
		MQTTAsync_subscribe(client, topic, MQTT_CLIENT_QOS, &opts);
	}
}

/**
 * @brief 取消订阅
 */
void mqtt_client_unsubscribe(char *topic)
{
	if (client != NULL) {
		MQTTAsync_unsubscribe(client, topic, NULL);
	}
}

/**
 * @brief 断开连接
 */
void mqtt_client_disconnect(void)
{
	if (client == NULL || mqtt_state != MQTT_STATE_CONNECTED) {
		printf("[MQTT] 未连接，无需断开\n");
		return;
	}

	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	opts.timeout = 1000;
	MQTTAsync_disconnect(client, &opts);
	mqtt_update_state(MQTT_STATE_DISCONNECTED);
	printf("[MQTT] 已断开连接\n");
}

/**
 * @brief 销毁客户端
 */
void mqtt_client_destroy(void)
{
	if (client != NULL) {
		mqtt_client_disconnect();
		MQTTAsync_destroy(&client);
		client = NULL;
		printf("[MQTT] 客户端已销毁\n");
	}
}

/**
 * @brief 获取当前连接状态
 */
mqtt_connection_state_t mqtt_client_get_state(void)
{
	return mqtt_state;
}

/**
 * @brief 注册状态变化回调
 */
void mqtt_client_set_state_callback(mqtt_state_callback_t callback)
{
	state_change_callback = callback;
}