#include "audio_engine.h"
#include "common.h"
#include <alsa/asoundlib.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ========== 配置常量 ==========
#define ALSA_DEVICE "default"
#define TARGET_SAMPLE_RATE 44100
#define TARGET_CHANNELS 2
#define TARGET_FORMAT SND_PCM_FORMAT_S16_LE
#define RING_BUFFER_SIZE (1024 * 512) // 512KB 环形缓冲区

// ========== 环形缓冲区 ==========
typedef struct {
	uint8_t data[RING_BUFFER_SIZE]; // 数据存储区
	int read_pos;                   // 读取位置索引
	int write_pos;                  // 写入位置索引
	int available;                  // 当前可读数据量(字节)
	pthread_mutex_t mutex;          // 保护缓冲区的互斥锁
	pthread_cond_t cond_not_empty;  // 条件变量：缓冲区非空 (唤醒消费者)
	pthread_cond_t cond_not_full;   // 条件变量：缓冲区非满 (唤醒生产者)
} ring_buffer_t;

// ========== 播放引擎状态 ==========
typedef struct {
	// --- 文件信息 ---
	char file_path[1024]; // 当前播放文件路径
	int64_t duration_ms;  // 总时长 (毫秒)
	int64_t position_ms;  // 当前播放位置 (毫秒)

	// --- 状态管理 ---
	player_status_t status;       // 当前播放器状态
	pthread_mutex_t status_mutex; // 保护状态和位置信息的互斥锁

	// --- 线程管理 ---
	pthread_t decode_thread;   // 解码线程 (生产者)
	pthread_t playback_thread; // 播放线程 (消费者)
	int thread_running;        // 线程运行标志

	// --- 控制标志 (volatile 确保线程可见性) ---
	volatile int should_stop;    // 停止信号：通知线程退出
	volatile int is_paused;      // 暂停信号：通知线程暂停
	volatile int seek_request;        // 跳转请求标志 (主线程设置，解码线程清除)
	volatile int seek_sequence;       // seek 序列号 (每次 seek 递增，播放线程对比)
	volatile int64_t seek_target_ms;  // 跳转目标时间 (毫秒)
	volatile int decode_finished;     // 解码完成标志
	volatile int alsa_released;       // ALSA 已释放标志 (播放线程设置，新播放前等待)

	// --- 暂停同步 ---
	pthread_mutex_t pause_mutex; // 暂停锁
	pthread_cond_t pause_cond;   // 暂停条件变量 (用于挂起和唤醒线程)

	// --- 精确进度计算 ---
	int64_t start_pts_ms;           // 起始帧的 PTS 时间戳 (毫秒)
	volatile int64_t bytes_played;  // 已写入 ALSA 的 PCM 字节总数
	int first_frame_received;       // 标记是否已接收到第一帧解码数据

	// --- 数据缓冲 ---
	ring_buffer_t ring_buffer; // 环形缓冲区实例

	// --- 回调通知 ---
	engine_status_cb_t status_callback; // 状态变更回调函数
	void *callback_user_data;           // 回调用户数据
} audio_engine_t;

static audio_engine_t g_engine = {0};

// ========== 环形缓冲区函数 ==========

static void ring_buffer_init(ring_buffer_t *rb)
{
	memset(rb, 0, sizeof(ring_buffer_t));
	pthread_mutex_init(&rb->mutex, NULL);
	pthread_cond_init(&rb->cond_not_empty, NULL);
	pthread_cond_init(&rb->cond_not_full, NULL);
}

static void ring_buffer_destroy(ring_buffer_t *rb)
{
	pthread_mutex_destroy(&rb->mutex);
	pthread_cond_destroy(&rb->cond_not_empty);
	pthread_cond_destroy(&rb->cond_not_full);
}

static int ring_buffer_write(ring_buffer_t *rb, const uint8_t *data, int size)
{
	pthread_mutex_lock(&rb->mutex);

	// ringbuffer 中的数据 + 要写的数据之和，不允许大于 RING_BUFFER_SIZE
	// 同时检查停止标志
	while ((rb->available + size) > RING_BUFFER_SIZE && !g_engine.should_stop) {
		pthread_cond_wait(&rb->cond_not_full, &rb->mutex);
	}

	// 检查是否需要退出
	if (g_engine.should_stop) {
		pthread_mutex_unlock(&rb->mutex);
		return 0;
	}

	int to_write = size;
	int part1 = RING_BUFFER_SIZE - rb->write_pos;

	// 这里在考虑绕圈的问题:
	// 1. 如果要写的数据小于等于 ringbuffer 的剩余空间，直接写入
	// 2. 如果要写的数据大于 ringbuffer 的剩余空间，分两段写入
	if (to_write <= part1) {
		memcpy(rb->data + rb->write_pos, data, to_write);
		rb->write_pos = (rb->write_pos + to_write) % RING_BUFFER_SIZE;
	} else {
		memcpy(rb->data + rb->write_pos, data, part1);
		memcpy(rb->data, data + part1, to_write - part1);
		// 写指针绕圈，要写的总长度 - 尾巴的部分，剩余的部分都是从 0 开始写的，
		// 所以这里计算没问题
		rb->write_pos = to_write - part1;
	}

	// 更新可用数据量，not_empty 条件变量会被唤醒
	rb->available += to_write;
	pthread_cond_signal(&rb->cond_not_empty);

	pthread_mutex_unlock(&rb->mutex);

	return to_write;
}

static int ring_buffer_read(ring_buffer_t *rb, uint8_t *data, int size)
{
	pthread_mutex_lock(&rb->mutex);

	// 如果 ringbuffer 中可用的数据小于要读取的数据，就等待
	// 同时检查停止标志和解码完成标志
	while (rb->available < size && !g_engine.should_stop && !g_engine.decode_finished) {
		pthread_cond_wait(&rb->cond_not_empty, &rb->mutex);
	}

	// 检查是否需要退出
	if (g_engine.should_stop) {
		pthread_mutex_unlock(&rb->mutex);
		return 0;
	}

	// 解码完成且 ringbuffer 为空，返回 0 表示播放结束
	if (g_engine.decode_finished && rb->available == 0) {
		pthread_mutex_unlock(&rb->mutex);
		return 0;
	}

	// 要读的数据量，取 ringbuffer 中可用的数据和要读取的数据的最小值
	// 当解码完成时，可能剩余数据不足 size，此时读取所有剩余数据
	int to_read = (rb->available < size) ? rb->available : size;
	int part1 = RING_BUFFER_SIZE - rb->read_pos;

	// 考虑绕圈的问题
	if (to_read <= part1) {
		memcpy(data, rb->data + rb->read_pos, to_read);
		rb->read_pos = (rb->read_pos + to_read) % RING_BUFFER_SIZE;
	} else {
		memcpy(data, rb->data + rb->read_pos, part1);
		memcpy(data + part1, rb->data, to_read - part1);
		rb->read_pos = to_read - part1;
	}

	// 更新可用数据量，not_full 条件变量会被唤醒
	rb->available -= to_read;
	pthread_cond_signal(&rb->cond_not_full);

	pthread_mutex_unlock(&rb->mutex);

	return to_read;
}

static void ring_buffer_clear(ring_buffer_t *rb)
{
	pthread_mutex_lock(&rb->mutex);
	rb->read_pos = 0;
	rb->write_pos = 0;
	rb->available = 0;
	pthread_cond_signal(&rb->cond_not_empty);
	pthread_cond_signal(&rb->cond_not_full);
	pthread_mutex_unlock(&rb->mutex);
}

// ========== ALSA 初始化 ==========
static int alsa_init(snd_pcm_t **handle)
{
	int err;
	snd_pcm_hw_params_t *hw_params;

	err = snd_pcm_open(handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		fprintf(stderr, "ALSA open error: %s\n", snd_strerror(err));
		return -1;
	}

	snd_pcm_hw_params_malloc(&hw_params);
	snd_pcm_hw_params_any(*handle, hw_params);
	snd_pcm_hw_params_set_access(*handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(*handle, hw_params, TARGET_FORMAT);
	snd_pcm_hw_params_set_channels(*handle, hw_params, TARGET_CHANNELS);
	snd_pcm_hw_params_set_rate_near(*handle, hw_params, &(unsigned int){TARGET_SAMPLE_RATE}, 0);

	err = snd_pcm_hw_params(*handle, hw_params);
	snd_pcm_hw_params_free(hw_params);

	if (err < 0) {
		fprintf(stderr, "ALSA hw_params error: %s\n", snd_strerror(err));
		snd_pcm_close(*handle);
		return -1;
	}

	snd_pcm_prepare(*handle);
	return 0;
}

// ========== 解码线程 ==========
static void *decode_thread_func(void *arg)
{
	AVFormatContext *fmt_ctx = NULL;
	AVCodecContext *codec_ctx = NULL;
	SwrContext *swr_ctx = NULL;
	AVPacket *packet = NULL;
	AVFrame *frame = NULL;
	uint8_t *pcm_buffer = NULL;
	int audio_stream_index = -1;

	// 打开文件
	if (avformat_open_input(&fmt_ctx, g_engine.file_path, NULL, NULL) != 0) {
		fprintf(stderr, "Could not open file: %s\n", g_engine.file_path);
		goto cleanup;
	}

	if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
		fprintf(stderr, "Could not find stream info\n");
		goto cleanup;
	}

	// 查找音频流
	for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
		if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_index = i;
			break;
		}
	}

	if (audio_stream_index == -1) {
		fprintf(stderr, "No audio stream found\n");
		goto cleanup;
	}

	// 获取解码器
	const AVCodec *codec = avcodec_find_decoder(
			fmt_ctx->streams[audio_stream_index]->codecpar->codec_id);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		goto cleanup;
	}

	codec_ctx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[audio_stream_index]->codecpar);

	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		goto cleanup;
	}

	// 初始化重采样
	swr_ctx = swr_alloc();
	av_opt_set_int(swr_ctx, "in_channel_layout", codec_ctx->channel_layout, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", codec_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codec_ctx->sample_fmt, 0);

	av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", TARGET_SAMPLE_RATE, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

	swr_init(swr_ctx);

	// 分配缓冲区
	packet = av_packet_alloc();
	frame = av_frame_alloc();
	pcm_buffer = (uint8_t *)malloc(192000 * 2);

	// 更新时长
	pthread_mutex_lock(&g_engine.status_mutex);
	g_engine.duration_ms = fmt_ctx->duration / 1000;
	pthread_mutex_unlock(&g_engine.status_mutex);

	// 解码循环
	while (!g_engine.should_stop) {
		// 处理 Seek 请求
		if (g_engine.seek_request) {
			// 将毫秒转换为流的 time_base 单位
			AVRational time_base = fmt_ctx->streams[audio_stream_index]->time_base;
			int64_t seek_ts = av_rescale_q(g_engine.seek_target_ms, (AVRational){1, 1000}, time_base);

			av_seek_frame(fmt_ctx, audio_stream_index, seek_ts, AVSEEK_FLAG_BACKWARD);
			avcodec_flush_buffers(codec_ctx);
			ring_buffer_clear(&g_engine.ring_buffer);

			pthread_mutex_lock(&g_engine.status_mutex);
			g_engine.position_ms = g_engine.seek_target_ms;
			g_engine.seek_request = 0;
			// 重置进度追踪状态
			g_engine.bytes_played = 0;
			g_engine.first_frame_received = 0;
			// 递增序列号，通知播放线程发生了 seek
			g_engine.seek_sequence++;
			pthread_mutex_unlock(&g_engine.status_mutex);
		}

		// 解码线程不处理暂停,继续解码直到 ringbuffer 填满

		if (av_read_frame(fmt_ctx, packet) < 0) {
			// 文件读取结束，设置解码完成标志并唤醒播放线程
			g_engine.decode_finished = 1;
			pthread_cond_signal(&g_engine.ring_buffer.cond_not_empty);
			break;
		}

		if (packet->stream_index != audio_stream_index) {
			av_packet_unref(packet);
			continue;
		}

		if (avcodec_send_packet(codec_ctx, packet) < 0) {
			av_packet_unref(packet);
			continue;
		}

		while (avcodec_receive_frame(codec_ctx, frame) == 0) {
			// 记录第一帧的 PTS 作为起始时间
			if (!g_engine.first_frame_received) {
				pthread_mutex_lock(&g_engine.status_mutex);
				g_engine.start_pts_ms = frame->pts * av_q2d(fmt_ctx->streams[audio_stream_index]->time_base) * 1000;
				g_engine.first_frame_received = 1;
				pthread_mutex_unlock(&g_engine.status_mutex);
			}

			// 重采样
			int out_samples = swr_convert(swr_ctx, &pcm_buffer, frame->nb_samples, (const uint8_t **)frame->data, frame->nb_samples);

			int pcm_size = out_samples * TARGET_CHANNELS * 2; // S16LE

			// 写入环形缓冲区
			ring_buffer_write(&g_engine.ring_buffer, pcm_buffer, pcm_size);
		}

		av_packet_unref(packet);
	}

	// 解码线程不再上报 FINISHED，由播放线程负责

cleanup:
	if (pcm_buffer)
		free(pcm_buffer);
	if (frame)
		av_frame_free(&frame);
	if (packet)
		av_packet_free(&packet);
	if (swr_ctx)
		swr_free(&swr_ctx);
	if (codec_ctx)
		avcodec_free_context(&codec_ctx);
	if (fmt_ctx)
		avformat_close_input(&fmt_ctx);

	return NULL;
}

// ========== 播放线程 ==========

static void *playback_thread_func(void *arg)
{
	uint8_t buffer[4096];
	snd_pcm_sframes_t frames;
	snd_pcm_t *alsa_handle = NULL;

	// 线程内创建独立的 ALSA 句柄
	if (alsa_init(&alsa_handle) < 0) {
		fprintf(stderr, "Playback thread: ALSA init failed\n");
		return NULL;
	}

	// seek 序列号，在循环开始前初始化为当前值
	int last_seek_seq = g_engine.seek_sequence;

	while (!g_engine.should_stop) {
		// 暂停处理:使用条件变量等待,释放 CPU
		if (g_engine.is_paused) {
			// 暂停 ALSA 播放
			int err = snd_pcm_pause(alsa_handle, 1);
			if (err < 0) {
				// 设备不支持硬件暂停,使用 drop
				snd_pcm_drop(alsa_handle);
			}

			// 使用条件变量等待恢复,避免忙等待
			pthread_mutex_lock(&g_engine.pause_mutex);
			while (g_engine.is_paused && !g_engine.should_stop) {
				pthread_cond_wait(&g_engine.pause_cond, &g_engine.pause_mutex);
			}
			pthread_mutex_unlock(&g_engine.pause_mutex);

			// 恢复 ALSA 播放
			if (err < 0) {
				// 需要重新 prepare
				snd_pcm_prepare(alsa_handle);
			} else {
				snd_pcm_pause(alsa_handle, 0);
			}
		}

		// seek 处理：对比序列号，检测是否发生了 seek
		// 注意：last_seek_seq 在循环外初始化，不能用 static
		if (g_engine.seek_sequence != last_seek_seq) {
			last_seek_seq = g_engine.seek_sequence;
			// 丢弃 ALSA 缓冲区中的旧数据
			snd_pcm_drop(alsa_handle);
			snd_pcm_prepare(alsa_handle);
			continue;  // 重新开始循环，读取新位置的数据
		}

		// 从环形缓冲区读取
		int bytes_read = ring_buffer_read(&g_engine.ring_buffer, buffer, sizeof(buffer));
		if (bytes_read == 0) {
			audio_view_printf("Playback thread: Ring buffer empty\n");
			break;
		}

		// 写入 ALSA
		frames = snd_pcm_writei(alsa_handle, buffer, bytes_read / 4);

		if (frames < 0) {
			frames = snd_pcm_recover(alsa_handle, frames, 0);
		}

		if (frames < 0) {
			fprintf(stderr, "ALSA write error\n");
			break;
		}

		// 累计已播放字节数
		if (frames > 0) {
			__sync_fetch_and_add(&g_engine.bytes_played, frames * 4);
		}

		// 计算精确播放位置
		snd_pcm_sframes_t delay = 0;
		if (snd_pcm_delay(alsa_handle, &delay) >= 0 && delay >= 0) {
			// 计算已播放时长(ms)
			int64_t bytes = __sync_fetch_and_add(&g_engine.bytes_played, 0);
			int64_t played_ms = (bytes * 1000) / (TARGET_SAMPLE_RATE * TARGET_CHANNELS * 2);

			// 计算硬件延迟(ms)
			int64_t delay_ms = (delay * 1000) / TARGET_SAMPLE_RATE;

			// 更新真实播放位置
			pthread_mutex_lock(&g_engine.status_mutex);
			g_engine.position_ms = g_engine.start_pts_ms + played_ms - delay_ms;
			if (g_engine.position_ms < 0) {
				g_engine.position_ms = 0;
			}
			pthread_mutex_unlock(&g_engine.status_mutex);
		}
	}

	// 释放线程独立的 ALSA 资源
	if (alsa_handle) {
		snd_pcm_drop(alsa_handle);   // 丢弃缓冲区，比 drain 更快
		snd_pcm_close(alsa_handle);
		alsa_handle = NULL;
	}
	// 标记 ALSA 已释放，允许新播放线程创建
	g_engine.alsa_released = 1;

	// 歌曲自然播放完成（非手动停止），上报 FINISHED 状态
	if (!g_engine.should_stop && g_engine.decode_finished) {
		pthread_mutex_lock(&g_engine.status_mutex);
		g_engine.status = PLAYER_STATUS_FINISHED;
		pthread_mutex_unlock(&g_engine.status_mutex);

		// 调用回调通知 Manager（回调中会创建新线程）
		if (g_engine.status_callback) {
			g_engine.status_callback(PLAYER_STATUS_FINISHED, g_engine.callback_user_data);
		}
	}

	return NULL;
}

// ========== 公开接口实现 ==========

int audio_engine_init(void)
{
	memset(&g_engine, 0, sizeof(g_engine));
	pthread_mutex_init(&g_engine.status_mutex, NULL);

	// 暂停时使用条件变量等待，并有互斥锁保护临界资源
	pthread_mutex_init(&g_engine.pause_mutex, NULL);
	pthread_cond_init(&g_engine.pause_cond, NULL);

	ring_buffer_init(&g_engine.ring_buffer);
	g_engine.status = PLAYER_STATUS_STOPPED;

	return 0;
}

// 每次播放创建新线程，线程使用 detach 模式自动回收
int audio_engine_play(const char *file_path)
{
	// 停止当前播放并等待 ALSA 释放
	if (g_engine.thread_running) {
		audio_engine_stop();
		// 等待播放线程释放 ALSA 资源
		while (!g_engine.alsa_released) {
			usleep(1000);  // 1ms
		}
	}

	// 重置状态
	strncpy(g_engine.file_path, file_path, sizeof(g_engine.file_path) - 1);
	g_engine.should_stop = 0;
	g_engine.is_paused = 0;
	g_engine.seek_request = 0;
	g_engine.position_ms = 0;
	g_engine.duration_ms = 0;
	g_engine.decode_finished = 0;  // 重置解码完成标志
	// 重置精确进度追踪状态
	g_engine.bytes_played = 0;
	g_engine.first_frame_received = 0;
	g_engine.start_pts_ms = 0;
	g_engine.alsa_released = 0;  // 重置 ALSA 释放标志

	ring_buffer_clear(&g_engine.ring_buffer);

	// ALSA 由播放线程独立管理，不在此处初始化

	// 创建线程并 detach（线程结束后自动回收资源）
	pthread_create(&g_engine.decode_thread, NULL, decode_thread_func, NULL);
	pthread_detach(g_engine.decode_thread);

	pthread_create(&g_engine.playback_thread, NULL, playback_thread_func, NULL);
	pthread_detach(g_engine.playback_thread);

	g_engine.thread_running = 1;

	pthread_mutex_lock(&g_engine.status_mutex);
	g_engine.status = PLAYER_STATUS_PLAYING;
	if (g_engine.status_callback) {
		g_engine.status_callback(PLAYER_STATUS_PLAYING, g_engine.callback_user_data);
	}
	pthread_mutex_unlock(&g_engine.status_mutex);

	return 0;
}

void audio_engine_pause(void)
{
	g_engine.is_paused = 1;

	pthread_mutex_lock(&g_engine.status_mutex);
	g_engine.status = PLAYER_STATUS_PAUSED;
	if (g_engine.status_callback) {
		g_engine.status_callback(PLAYER_STATUS_PAUSED, g_engine.callback_user_data);
	}
	pthread_mutex_unlock(&g_engine.status_mutex);
}

void audio_engine_resume(void)
{
	g_engine.is_paused = 0;

	// 唤醒播放线程
	pthread_mutex_lock(&g_engine.pause_mutex);
	pthread_cond_signal(&g_engine.pause_cond);
	pthread_mutex_unlock(&g_engine.pause_mutex);

	pthread_mutex_lock(&g_engine.status_mutex);
	g_engine.status = PLAYER_STATUS_PLAYING;
	if (g_engine.status_callback) {
		g_engine.status_callback(PLAYER_STATUS_PLAYING, g_engine.callback_user_data);
	}
	pthread_mutex_unlock(&g_engine.status_mutex);
}

// 功能1：退出播放的时候，相当于突然的停止播放，需要处理解码和播放线程
// 功能2：播放完歌曲后，先清掉相关的资源，再开始下一次播放
void audio_engine_stop(void)
{
	g_engine.should_stop = 1;

	// 唤醒所有等待的线程，让它们检测 should_stop 并自行退出
	pthread_cond_broadcast(&g_engine.ring_buffer.cond_not_empty);
	pthread_cond_broadcast(&g_engine.ring_buffer.cond_not_full);

	pthread_mutex_lock(&g_engine.pause_mutex);
	pthread_cond_broadcast(&g_engine.pause_cond);
	pthread_mutex_unlock(&g_engine.pause_mutex);

	// 不等待线程，线程会自行退出并清理资源
	g_engine.thread_running = 0;

	// ALSA 资源由播放线程独立管理，不在此处释放

	pthread_mutex_lock(&g_engine.status_mutex);
	g_engine.status = PLAYER_STATUS_STOPPED;
	pthread_mutex_unlock(&g_engine.status_mutex);
}

void audio_engine_seek(int64_t time_ms) {
	g_engine.seek_target_ms = time_ms;
	g_engine.seek_request = 1;
}

int64_t audio_engine_get_position(void)
{
	int64_t pos;
	pthread_mutex_lock(&g_engine.status_mutex);
	pos = g_engine.position_ms;
	pthread_mutex_unlock(&g_engine.status_mutex);
	return pos;
}

int64_t audio_engine_get_duration(void)
{
	int64_t dur;
	pthread_mutex_lock(&g_engine.status_mutex);
	dur = g_engine.duration_ms;
	pthread_mutex_unlock(&g_engine.status_mutex);
	return dur;
}

player_status_t audio_engine_get_status(void)
{
	player_status_t status;
	pthread_mutex_lock(&g_engine.status_mutex);
	status = g_engine.status;
	pthread_mutex_unlock(&g_engine.status_mutex);
	return status;
}

void audio_engine_set_callback(engine_status_cb_t cb, void *user_data)
{
	g_engine.status_callback = cb;
	g_engine.callback_user_data = user_data;
}
