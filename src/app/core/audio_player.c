#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h> // for usleep

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <alsa/asoundlib.h>

// --- ALSA & Player Defines ---
#define ALSA_DEVICE "default"
#define TARGET_SAMPLE_RATE 44100
#define TARGET_CHANNELS 2
#define TARGET_FORMAT SND_PCM_FORMAT_S16_LE
#define TARGET_SAMPLE_FMT AV_SAMPLE_FMT_S16

// --- 歌词处理部分 ---
typedef struct {
    long time_ms;
    char text[256];
} LyricLine;

// --- 标准播放器核心架构 ---

typedef enum {
    STATUS_STOPPED,
    STATUS_PLAYING,
    STATUS_PAUSED,
    STATUS_EXITING
} PlayerStatus;

typedef struct {
    // 线程与同步
    pthread_t thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    PlayerStatus status;

    // 媒体信息
    char media_filepath[1024];
    int64_t duration_ms;
    int64_t current_time_ms;

    // 内部播放状态
    long long total_frames_written;
} Player;

LyricLine *lyrics = NULL;
Player *player = NULL;

const char *media_filepath = NULL;
const char *lrc_filepath = NULL;

int lyric_count = 0;

pthread_t lrc_thread_id;

lv_obj_t *g_lrc_label;

static int parse_lrc_file(const char *filepath, LyricLine **lyrics_out, int *count_out) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Failed to open LRC file: %s\n", filepath);
        return -1;
    }
    char line[512];
    int capacity = 10;
    int count = 0;
    LyricLine *lyrics = malloc(capacity * sizeof(LyricLine));
    if (!lyrics) {
        fclose(file);
        return -1;
    }
    while (fgets(line, sizeof(line), file)) {
        int min, sec, cs;
        if (sscanf(line, "[%d:%d.%d]", &min, &sec, &cs) == 3) {
            if (count >= capacity) {
                capacity *= 2;
                LyricLine *temp = realloc(lyrics, capacity * sizeof(LyricLine));
                if (!temp) { free(lyrics); fclose(file); return -1; }
                lyrics = temp;
            }
            lyrics[count].time_ms = min * 60000 + sec * 1000 + cs * 10;
            char *lyric_text = strchr(line, ']');
            if (lyric_text && *(lyric_text + 1)) {
                lyric_text++;
                while (*lyric_text && isspace((unsigned char)*lyric_text)) lyric_text++;
                size_t len = strlen(lyric_text);
                while (len > 0 && isspace((unsigned char)lyric_text[len - 1])) len--;
                strncpy(lyrics[count].text, lyric_text, len);
                lyrics[count].text[len] = '\0';
            } else {
                lyrics[count].text[0] = '\0';
            }
            count++;
        }
    }
    fclose(file);
    *lyrics_out = lyrics;
    *count_out = count;
    return 0;
}

// 播放线程中的一个辅助函数，用于处理一帧解码后的音频
static void process_frame(AVFrame *frame, SwrContext *swr_ctx, snd_pcm_t *pcm_handle, Player *player, uint64_t out_ch_layout) {
    uint8_t *out_buffer;
    int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
    av_samples_alloc(&out_buffer, NULL, av_get_channel_layout_nb_channels(out_ch_layout), out_samples, TARGET_SAMPLE_FMT, 0);
    int converted_samples = swr_convert(swr_ctx, &out_buffer, out_samples, (const uint8_t **)frame->extended_data, frame->nb_samples);

    snd_pcm_sframes_t written_frames = snd_pcm_writei(pcm_handle, out_buffer, converted_samples);
    av_freep(&out_buffer);

    pthread_mutex_lock(&player->mutex);
    if (written_frames > 0) {
        player->total_frames_written += written_frames;
    }
    snd_pcm_sframes_t delay;
    if (snd_pcm_delay(pcm_handle, &delay) == 0) {
        long long played_frames = player->total_frames_written - delay;
        if (played_frames < 0) played_frames = 0;
        player->current_time_ms = (played_frames * 1000) / TARGET_SAMPLE_RATE;
    }
    pthread_mutex_unlock(&player->mutex);
}


// 播放线程函数
void* playback_thread_func(void *arg) {
    Player *player = (Player *)arg;
//    int ret;

    // FFmpeg & ALSA 变量
    AVFormatContext *pFormatContext = NULL;
    AVCodecContext *pCodecContext = NULL;
    SwrContext *swr_ctx = NULL;
    snd_pcm_t *pcm_handle = NULL;
    AVPacket *pPacket = NULL;
    AVFrame *pFrame = NULL;

    pPacket = av_packet_alloc();
    pFrame = av_frame_alloc();

    while (1) {
        pthread_mutex_lock(&player->mutex);

        while (player->status != STATUS_PLAYING && player->status != STATUS_EXITING) {
            pthread_cond_wait(&player->cond, &player->mutex);
        }

        if (player->status == STATUS_EXITING) {
            pthread_mutex_unlock(&player->mutex);
            break;
        }

        printf("[Playback Thread] Woke up, starting playback for: %s\n", player->media_filepath);
        
        pFormatContext = NULL;
        if (avformat_open_input(&pFormatContext, player->media_filepath, NULL, NULL) != 0) {
            fprintf(stderr, "[Playback Thread] Error: Could not open file\n");
            player->status = STATUS_STOPPED;
            pthread_mutex_unlock(&player->mutex);
            continue;
        }
        avformat_find_stream_info(pFormatContext, NULL);
        int audio_stream_index = av_find_best_stream(pFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        if (audio_stream_index < 0) { player->status = STATUS_STOPPED; pthread_mutex_unlock(&player->mutex); continue; }
        AVStream *pAudioStream = pFormatContext->streams[audio_stream_index];
        const AVCodec *pCodec = avcodec_find_decoder(pAudioStream->codecpar->codec_id);
        pCodecContext = avcodec_alloc_context3(pCodec);
        avcodec_parameters_to_context(pCodecContext, pAudioStream->codecpar);
        avcodec_open2(pCodecContext, pCodec, NULL);

        snd_pcm_open(&pcm_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
        snd_pcm_hw_params_t *hw_params;
        snd_pcm_hw_params_alloca(&hw_params);
        snd_pcm_hw_params_any(pcm_handle, hw_params);
        snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pcm_handle, hw_params, TARGET_FORMAT);
        snd_pcm_hw_params_set_channels(pcm_handle, hw_params, TARGET_CHANNELS);
        snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, (unsigned int[]){TARGET_SAMPLE_RATE}, 0);
        snd_pcm_hw_params(pcm_handle, hw_params);

        swr_ctx = NULL;
        uint64_t in_ch_layout = pCodecContext->channel_layout;
        uint64_t out_ch_layout = av_get_default_channel_layout(TARGET_CHANNELS);
        swr_ctx = swr_alloc_set_opts(NULL, out_ch_layout, TARGET_SAMPLE_FMT, TARGET_SAMPLE_RATE, in_ch_layout, pCodecContext->sample_fmt, pCodecContext->sample_rate, 0, NULL);
        swr_init(swr_ctx);

        player->total_frames_written = 0;
        player->current_time_ms = 0;
        player->duration_ms = pFormatContext->duration / 1000;

        pthread_mutex_unlock(&player->mutex);

        // --- 主解码播放循环 ---
        while (1) {
            pthread_mutex_lock(&player->mutex);
            if (player->status != STATUS_PLAYING) {
                pthread_mutex_unlock(&player->mutex);
                break;
            }
            pthread_mutex_unlock(&player->mutex);

            if (av_read_frame(pFormatContext, pPacket) < 0) {
                break; // 文件读取完毕
            }

            if (pPacket->stream_index == audio_stream_index) {
                if (avcodec_send_packet(pCodecContext, pPacket) == 0) {
                    while (avcodec_receive_frame(pCodecContext, pFrame) == 0) {
                        process_frame(pFrame, swr_ctx, pcm_handle, player, out_ch_layout);
                    }
                }
            }
            av_packet_unref(pPacket);
        }

        // --- Flush阶段: 刷空解码器和重采样器中剩余的帧 ---
        printf("[Playback Thread] Flushing remaining frames...\n");
        avcodec_send_packet(pCodecContext, NULL); // 发送NULL packet来启动flush
        while (avcodec_receive_frame(pCodecContext, pFrame) == 0) {
            process_frame(pFrame, swr_ctx, pcm_handle, player, out_ch_layout);
        }

        // --- Drain阶段: 等待ALSA缓冲区播放完毕 ---
        printf("[Playback Thread] Draining audio buffer...\n");
        while (1) {
            pthread_mutex_lock(&player->mutex);
            snd_pcm_state_t state = snd_pcm_state(pcm_handle);
            if (state != SND_PCM_STATE_RUNNING && state != SND_PCM_STATE_DRAINING) {
                pthread_mutex_unlock(&player->mutex);
                break;
            }
            
            snd_pcm_sframes_t delay;
            if (snd_pcm_delay(pcm_handle, &delay) == 0) {
                long long played_frames = player->total_frames_written - delay;
                if (played_frames < 0) played_frames = 0;
                player->current_time_ms = (played_frames * 1000) / TARGET_SAMPLE_RATE;
            }
            pthread_mutex_unlock(&player->mutex);
            usleep(20000); // 20ms
        }

        printf("[Playback Thread] Playback finished.\n");
        snd_pcm_close(pcm_handle); pcm_handle = NULL;
        swr_free(&swr_ctx); swr_ctx = NULL;
        avcodec_free_context(&pCodecContext); pCodecContext = NULL;
        avformat_close_input(&pFormatContext); pFormatContext = NULL;

        pthread_mutex_lock(&player->mutex);
        if (player->status != STATUS_EXITING) {
            player->status = STATUS_STOPPED;
        }
        pthread_mutex_unlock(&player->mutex);
    }

    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    printf("[Playback Thread] Exiting.\n");
    return NULL;
}

// --- 控制API ---

Player* player_create() {
    Player *player = (Player*)malloc(sizeof(Player));
    if (!player) return NULL;

    memset(player, 0, sizeof(Player));
    player->status = STATUS_STOPPED;

    pthread_mutex_init(&player->mutex, NULL);
    pthread_cond_init(&player->cond, NULL);

    pthread_create(&player->thread_id, NULL, playback_thread_func, player);

    return player;
}

void player_destroy(void) {
    if (!player) return;

    pthread_mutex_lock(&player->mutex);
    player->status = STATUS_EXITING;
    pthread_cond_signal(&player->cond);
    pthread_mutex_unlock(&player->mutex);

    pthread_join(player->thread_id, NULL);

    pthread_mutex_destroy(&player->mutex);
    pthread_cond_destroy(&player->cond);
    free(player);
}

//void player_start(Player *player, const char *media_path) {
void player_start(void)
{
    if (!player || !media_filepath) return;

    pthread_mutex_lock(&player->mutex);
    strncpy(player->media_filepath, media_filepath, sizeof(player->media_filepath) - 1);
    player->status = STATUS_PLAYING;
    pthread_cond_signal(&player->cond);
    pthread_mutex_unlock(&player->mutex);
}

//void player_stop(Player *player) {
void player_stop(void)
{
    if (!player) return;
    pthread_mutex_lock(&player->mutex);
    if (player->status == STATUS_PLAYING || player->status == STATUS_PAUSED) {
        player->status = STATUS_STOPPED;
    }
    pthread_mutex_unlock(&player->mutex);
}

int64_t player_get_time(Player *player) {
    if (!player) return 0;
    pthread_mutex_lock(&player->mutex);
    int64_t time = player->current_time_ms;
    pthread_mutex_unlock(&player->mutex);
    return time;
}

PlayerStatus player_get_status(Player *player) {
    if (!player) return STATUS_STOPPED;
    pthread_mutex_lock(&player->mutex);
    PlayerStatus status = player->status;
    pthread_mutex_unlock(&player->mutex);
    return status;
}


// --- Main函数 (模拟UI线程) ---
void* lrc_thread_func(void *arg) {
    int current_lyric_index = 0;
    char display_text[2048] = {0};  // 足够容纳 5 行歌词
    
    while (1) {
        PlayerStatus status = player_get_status(player);
        if (status == STATUS_STOPPED) {
            usleep(50000);
            continue;
        }

        int64_t current_time = player_get_time(player);

        if (lyrics && current_lyric_index < lyric_count) {
            while (current_lyric_index < lyric_count && current_time >= lyrics[current_lyric_index].time_ms) {
                // 拼接 5 句歌词：前两句 + 当前 + 后两句
                memset(display_text, 0, sizeof(display_text));
                
                // 前两句
                if (current_lyric_index >= 2) {
                    snprintf(display_text, sizeof(display_text), "%s\n", lyrics[current_lyric_index - 2].text);
                    snprintf(display_text + strlen(display_text), sizeof(display_text) - strlen(display_text), "%s\n", lyrics[current_lyric_index - 1].text);
                } else if (current_lyric_index == 1) {
                    snprintf(display_text, sizeof(display_text), "%s\n", lyrics[current_lyric_index - 1].text);
                }
                
                // 当前歌词
                snprintf(display_text + strlen(display_text), sizeof(display_text) - strlen(display_text), "%s\n", lyrics[current_lyric_index].text);
                
                // 后两句
                if (current_lyric_index + 1 < lyric_count) {
                    snprintf(display_text + strlen(display_text), sizeof(display_text) - strlen(display_text), "%s\n", lyrics[current_lyric_index + 1].text);
                }
                if (current_lyric_index + 2 < lyric_count) {
                    snprintf(display_text + strlen(display_text), sizeof(display_text) - strlen(display_text), "%s\n", lyrics[current_lyric_index + 2].text);
                }
                
                // 设置标签文本
                lv_label_set_text(g_lrc_label, display_text);
                current_lyric_index++;
            }
        }

        usleep(50000); // 50ms
    }
    return NULL;
}

#if 0
void* lrc_thread_func(void *arg) {
    int current_lyric_index = 0;
    while (1) {
        PlayerStatus status = player_get_status(player);
        if (status == STATUS_STOPPED) {
            continue;
//            // 检查是否还有最后一秒的歌词需要显示
//            int64_t current_time = player_get_time(player);
//            if (lyrics && current_lyric_index < lyric_count && current_time >= lyrics[current_lyric_index].time_ms) {
//                 // 再刷新一次歌词
//            } else {
//                break; // 播放已停止，退出UI循环
//            }
        }
//         if (status == STATUS_EXITING) break;


        int64_t current_time = player_get_time(player);

        if (lyrics && current_lyric_index < lyric_count) {
            while (current_lyric_index < lyric_count && current_time >= lyrics[current_lyric_index].time_ms) {
                lv_label_set_text(g_lrc_label, "%s\n%s\n%s\n", (current_lyric_index > 0) ? lyrics[current_lyric_index-1].text : "", lyrics[current_lyric_index].text, lyrics[current_lyric_index+1].text);
                current_lyric_index++;
            }
        }

        usleep(50000); // 50ms
    }
}
#endif


int audio_player_init(const char *audio_path, const char *lrc_path, lv_obj_t *lrc_label)
{
//    if (argc < 3) {
//        printf("Usage: %s <media_file_path> <lrc_file_path>\n", argv[0]);
//        return -1;
//    }
//    const char *media_filepath = audio_path;
//    const char *lrc_filepath = lrc_path;

    media_filepath = audio_path;
    lrc_filepath = lrc_path;
    g_lrc_label = lrc_label;

    if (parse_lrc_file(lrc_filepath, &lyrics, &lyric_count) != 0) {
        fprintf(stderr, "Could not load lyrics.\n");
        return -1;
    }
    printf("[Main Thread] Lyrics loaded.\n");

    player = player_create();
    if (!player) {
        fprintf(stderr, "Failed to create player.\n");
        free(lyrics);
        return -1;
    }
    printf("[Main Thread] Player created.\n");

    printf("[Main Thread] Starting playback...\n");
//    player_start(player, media_filepath);

    pthread_create(&lrc_thread_id, NULL, lrc_thread_func, player);

//    int current_lyric_index = 0;
//    while (1) {
//        PlayerStatus status = player_get_status(player);
//        if (status == STATUS_STOPPED) {
//            continue;
////            // 检查是否还有最后一秒的歌词需要显示
////            int64_t current_time = player_get_time(player);
////            if (lyrics && current_lyric_index < lyric_count && current_time >= lyrics[current_lyric_index].time_ms) {
////                 // 再刷新一次歌词
////            } else {
////                break; // 播放已停止，退出UI循环
////            }
//        }
////         if (status == STATUS_EXITING) break;
//
//
//        int64_t current_time = player_get_time(player);
//
//        if (lyrics && current_lyric_index < lyric_count) {
//            while (current_lyric_index < lyric_count && current_time >= lyrics[current_lyric_index].time_ms) {
//                printf(">>> %s\n", lyrics[current_lyric_index].text);
//                current_lyric_index++;
//            }
//        }
//
//        usleep(50000); // 50ms
//    }

//    printf("[Main Thread] Playback has stopped. Cleaning up.\n");
////    player_destroy(player);
//    player_destroy();
//    free(lyrics);
//
//    printf("[Main Thread] Program finished.\n");
    return 0;
}
