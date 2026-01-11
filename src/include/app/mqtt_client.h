#ifndef __MQTT_CLIENT_H_
#define __MQTT_CLIENT_H_

#include "common.h"
#include "MQTTAsync.h"

#define USE_SSL 1

#define MQTT_CLIENT_ID "IMX6ULL_0"
#define MQTT_CLIENT_ADDRESS "ssl://ma41c02e.ala.cn-hangzhou.emqxsl.cn:8883"
#define MQTT_CLIENT_USERNAME "tanxzh"
#define MQTT_CLIENT_PASSWORD "Password001"
#define MQTT_CLIENT_CA_CERTIFICATE_FILE "./resources/mqtt/emqxsl-ca.crt"
/*
 * 播放控制：第三方可以控制播放器的播放、暂停、下一首、上一首，此时由第三方发布 TOPIC，开发板订阅并处理。
 * 播放状态：开发板可以上报播放器的播放状态、当前曲目、播放进度，此时由开发板发布 TOPIC，第三方订阅并处理。
 * 对于发布事件，UI 更新状态时调用 mqtt_client 的接口。
 * 对于订阅事件，mqtt_client 的回调函数中拿到了 topic 和 payload，需要根据 topic 和 payload 处理对应的事件。
*/
#define MQTT_CLIENT_STATUS_TOPIC     "states/player/status"  // 播放状态：播放、暂停
#define MQTT_CLIENT_TRACK_TOPIC      "states/player/track"   // 当前歌曲信息：歌曲名、歌手名
#define MQTT_CLIENT_VOLUME_TOPIC "states/player/volume" // 音量状态：0-100
#define MQTT_CLIENT_DEVICE_STATUS_TOPIC "states/device/status"  // 设备在线状态：在线、离线

#define MQTT_CLIENT_QOS         0
#define MQTT_CLIENT_TIMEOUT     10000L

/* 连接状态枚举 */
typedef enum {
	MQTT_STATE_DISCONNECTED = 0,   /* 未连接 */
	MQTT_STATE_CONNECTING,         /* 连接中 */
	MQTT_STATE_CONNECTED           /* 已连接 */
} mqtt_connection_state_t;

/* 消息回调类型 */
typedef void (*mqtt_client_callback_t)(char *topic, char *payload);

/* 连接状态变化回调类型，用于通知 UI 更新状态显示 */
typedef void (*mqtt_state_callback_t)(mqtt_connection_state_t state);

typedef struct {
	MQTTAsync client;               /* 异步客户端句柄 */
	mqtt_client_callback_t callback;
	mqtt_state_callback_t state_callback;  /* 状态变化回调 */
	mqtt_connection_state_t state;         /* 当前连接状态 */
} mqtt_client_t;

/* 初始化 MQTT 客户端（异步，不会阻塞） */
void mqtt_client_init(void);

/* 更新 UI 时调用，发布状态、track 等数据到 MQTT 服务器 */
void mqtt_client_publish(char *topic, char *payload);

/* 订阅事件，当有控制命令发布到 MQTT 服务器时，调用回调函数 */
void mqtt_client_subscribe(char *topic, mqtt_client_callback_t callback);

/* 取消订阅事件 */
void mqtt_client_unsubscribe(char *topic);

/* 断开与 MQTT 服务器的连接 */
void mqtt_client_disconnect(void);

/* 销毁 MQTT 客户端 */
void mqtt_client_destroy(void);

/* 获取当前连接状态 */
mqtt_connection_state_t mqtt_client_get_state(void);

/* 注册状态变化回调，用于 UI 更新 */
void mqtt_client_set_state_callback(mqtt_state_callback_t callback);

#endif /* __MQTT_CLIENT_H_ */