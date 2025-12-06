#include "audio_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ========== 全局状态 ==========
typedef struct {
	music_info_t *playlist_head; // 播放列表链表头
	int playlist_count;          // 歌曲总数

	music_info_t *current_music; // 当前播放的歌曲
	int current_index;           // 当前索引

	play_mode_t play_mode; // 播放模式
} audio_manager_t;

static audio_manager_t g_manager = {0};

// ========== 内部辅助函数 ==========

// 根据索引获取歌曲信息
static music_info_t *get_music_at_index(int index)
{
	if (index < 0 || index >= g_manager.playlist_count) {
		return NULL;
	}

	music_info_t *current = g_manager.playlist_head;
	for (int i = 0; i < index && current; i++) {
		current = current->next;
	}

	return current;
}

// 计算下一首索引（根据播放模式）
static int calculate_next_index(void)
{
	if (g_manager.playlist_count == 0) {
		return -1;
	}

	switch (g_manager.play_mode) {
	case PLAY_MODE_LOOP_SINGLE:
		return g_manager.current_index;

	case PLAY_MODE_SHUFFLE:
		// 简单随机（可优化为不重复随机）
		return rand() % g_manager.playlist_count;

	case PLAY_MODE_LOOP_LIST:
	default:
		return (g_manager.current_index + 1) % g_manager.playlist_count;
	}
}

// 计算上一首索引
static int calculate_prev_index(void)
{
	if (g_manager.playlist_count == 0) {
		return -1;
	}

	// 上一首不区分模式，统一按列表顺序
	int prev = g_manager.current_index - 1;
	if (prev < 0) {
		prev = g_manager.playlist_count - 1;
	}

	return prev;
}

// Engine 状态回调处理
static void engine_status_callback(player_status_t status, void *user_data)
{
	// 播放结束，自动播放下一首
	if (status == PLAYER_STATUS_FINISHED) {
		int next_index = calculate_next_index();
		if (next_index >= 0) {
			audio_manager_play_at_index(next_index);
		}
	}
}

// ========== 公开接口实现 ==========

int audio_manager_init(void)
{
	memset(&g_manager, 0, sizeof(g_manager));
	// TODO: 默认是循环播放，到时需要从上一次的配置获取
	g_manager.play_mode = PLAY_MODE_LOOP_LIST;
	g_manager.current_index = -1;

	// 初始化 Engine 并注册回调
	audio_engine_init();
	audio_engine_set_callback(engine_status_callback, NULL);

	// 初始化随机数种子
	srand(time(NULL));

	return 0;
}

int audio_manager_scan_dir(const char *path)
{
	// 释放旧列表
	if (g_manager.playlist_head) {
		audio_library_free_list(g_manager.playlist_head);
		g_manager.playlist_head = NULL;
		g_manager.playlist_count = 0;
		g_manager.current_music = NULL;
		g_manager.current_index = -1;
	}

	// 扫描新目录
	g_manager.playlist_head = audio_library_scan_dir(path);

	// 统计歌曲数量
	g_manager.playlist_count = 0;
	music_info_t *current = g_manager.playlist_head;
	while (current) {
		g_manager.playlist_count++;
		current = current->next;
	}

	printf("Audio Manager: Scanned %d songs\n", g_manager.playlist_count);

	return g_manager.playlist_count;
}

int audio_manager_play_at_index(int index)
{
	music_info_t *music = get_music_at_index(index);
	if (!music) {
		fprintf(stderr, "Audio Manager: Invalid index %d\n", index);
		return -1;
	}

	// 更新当前歌曲
	g_manager.current_music = music;
	g_manager.current_index = index;

	// 调用 Engine 播放
	return audio_engine_play(music->file_path);
}

int audio_manager_play(void)
{
	player_status_t status = audio_engine_get_status();

	switch (status) {
	case PLAYER_STATUS_STOPPED:
		// 从头播放（或播放第一首）
		if (g_manager.current_index < 0 && g_manager.playlist_count > 0) {
			return audio_manager_play_at_index(0);
		} else if (g_manager.current_music) {
			return audio_engine_play(g_manager.current_music->file_path);
		}
		break;

	case PLAYER_STATUS_PLAYING:
		// 切换为暂停
		audio_engine_pause();
		break;

	case PLAYER_STATUS_PAUSED:
		// 恢复播放
		audio_engine_resume();
		break;

	default:
		break;
	}

	return 0;
}

int audio_manager_pause(void)
{
	audio_engine_pause();
	return 0;
}

int audio_manager_stop(void)
{
	audio_engine_stop();
	return 0;
}

int audio_manager_play_next(void)
{
	int next_index = calculate_next_index();
	if (next_index < 0) {
		return -1;
	}

	return audio_manager_play_at_index(next_index);
}

int audio_manager_play_prev(void)
{
	int prev_index = calculate_prev_index();
	if (prev_index < 0) {
		return -1;
	}

	return audio_manager_play_at_index(prev_index);
}

int audio_manager_seek(int64_t time_ms)
{
	audio_engine_seek(time_ms);
	return 0;
}

int audio_manager_set_mode(play_mode_t mode)
{
	g_manager.play_mode = mode;
	return 0;
}

play_mode_t audio_manager_get_mode(void)
{
	return g_manager.play_mode;
}

music_info_t *audio_manager_get_current_info(void)
{
	return g_manager.current_music;
}

int64_t audio_manager_get_position(void)
{
	return audio_engine_get_position();
}

int64_t audio_manager_get_duration(void)
{
	return audio_engine_get_duration(); 
}

player_status_t audio_manager_get_status(void)
{
	return audio_engine_get_status();
}

int audio_manager_get_playlist_count(void)
{
	return g_manager.playlist_count;
}

music_info_t *audio_manager_get_playlist_head(void)
{
	return g_manager.playlist_head;
}
