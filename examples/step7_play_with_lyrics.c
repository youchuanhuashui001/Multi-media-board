/*
 * FFmpeg 学习第7步：同步歌词显示 (基于step4)
 *
 * 简介:
 * 本程序在第4步（单线程播放）的基础上，增加了LRC歌词文件的解析和同步显示功能。
 * 1. 增加了一个用于存储歌词时间和文本的结构体 `LyricLine`。
 * 2. 实现了一个 `parse_lrc_file` 函数，用于读取并解析LRC文件。
 * 3. 主函数现在需要接收两个参数：媒体文件路径和LRC文件路径。
 * 4. 在解码循环中，通过解码后 `AVFrame` 的 `pts` (Presentation Timestamp)
 *    来计算当前播放的毫秒数。
 * 5. 将当前播放时间与歌词时间进行比较，在正确的时间点打印出对应的歌词行。
 *
 * 编译命令:
 * gcc -o step7_play_with_lyrics step7_play_with_lyrics.c \
 *     $(pkg-config --cflags libavformat libavcodec libswresample libavutil alsa) \
 *     $(pkg-config --libs libavformat libavcodec libswresample libavutil alsa)
 *
 * 使用方法:
 * ./step7_play_with_lyrics /path/to/your/media.mp3 /path/to/your/lyrics.lrc
 *
 * 预期行为:
 * 程序将播放指定的音频文件，并在命令行中随着歌曲的进度实时打印出歌词。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <alsa/asoundlib.h>

// ALSA 相关参数
#define ALSA_DEVICE "default"
#define TARGET_SAMPLE_RATE 44100
#define TARGET_CHANNELS 2
#define TARGET_FORMAT SND_PCM_FORMAT_S16_LE
#define TARGET_CHANNEL_LAYOUT AV_CH_LAYOUT_STEREO
#define TARGET_SAMPLE_FMT AV_SAMPLE_FMT_S16

// --- 新增歌词处理部分 ---

// 歌词行结构体
typedef struct {
    long time_ms;       // 时间戳 (毫秒)
    char text[256];     // 歌词内容
} LyricLine;

/**
 * @brief 解析LRC歌词文件
 * @param filepath LRC文件路径
 * @param lyrics_out 解析后的歌词数组指针
 * @param count_out 歌词行数指针
 * @return 0 on success, -1 on failure
 */
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
                if (!temp) {
                    free(lyrics);
                    fclose(file);
                    return -1;
                }
                lyrics = temp;
            }

            lyrics[count].time_ms = min * 60000 + sec * 1000 + cs * 10;
            
            char *lyric_text = strchr(line, ']');
            if (lyric_text && *(lyric_text + 1)) {
                lyric_text++; // Skip ']'
                while (*lyric_text && isspace((unsigned char)*lyric_text)) lyric_text++;
                
                size_t len = strlen(lyric_text);
                while (len > 0 && isspace((unsigned char)lyric_text[len - 1])) {
                    len--;
                }
                
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
    printf("[LRC Parser] Loaded %d lines of lyrics.\n", count);
    return 0;
}

static int decode_and_play(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame, 
                           SwrContext *swr_ctx, snd_pcm_t *pcm_handle,
                           AVStream *audio_stream, LyricLine *lyrics, int lyric_count, int *current_lyric_index) {
    int ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error sending a packet for decoding\n");
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error during decoding\n");
            return ret;
        }

        // --- 歌词同步点 ---
        if (lyrics && *current_lyric_index < lyric_count) {
            long long current_time_ms = frame->pts * 1000 * av_q2d(audio_stream->time_base);
            if (current_time_ms >= lyrics[*current_lyric_index].time_ms) {
                printf("\r\033[K> %s\n", lyrics[*current_lyric_index].text);
                (*current_lyric_index)++;
            }
        }

        // --- 音频重采样并播放 ---
        uint8_t *out_buffer;
        int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
        av_samples_alloc(&out_buffer, NULL, TARGET_CHANNELS, out_samples, TARGET_SAMPLE_FMT, 0);

        int converted_samples = swr_convert(swr_ctx, &out_buffer, out_samples, 
                                            (const uint8_t **)frame->extended_data, frame->nb_samples);
        
        snd_pcm_writei(pcm_handle, out_buffer, converted_samples);
        
        av_freep(&out_buffer);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <media_file_path> <lrc_file_path>\n", argv[0]);
        return -1;
    }
    const char *media_filepath = argv[1];
    const char *lrc_filepath = argv[2];

    // --- 加载歌词 ---
    LyricLine *lyrics = NULL;
    int lyric_count = 0;
    if (parse_lrc_file(lrc_filepath, &lyrics, &lyric_count) != 0) {
        fprintf(stderr, "Could not load lyrics, continuing without them.\n");
    }

    // --- FFmpeg 初始化 ---
    AVFormatContext *pFormatContext = NULL;
    avformat_open_input(&pFormatContext, media_filepath, NULL, NULL);
    avformat_find_stream_info(pFormatContext, NULL);

    int audio_stream_index = -1;
    AVStream *pAudioStream = NULL;
    AVCodecParameters *pCodecParams = NULL;
    for (int i = 0; i < pFormatContext->nb_streams; i++) {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            pAudioStream = pFormatContext->streams[i];
            pCodecParams = pAudioStream->codecpar;
            break;
        }
    }
    if (audio_stream_index == -1) {
        free(lyrics);
        return -1;
    }

    const AVCodec *pCodec = avcodec_find_decoder(pCodecParams->codec_id);
    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecContext, pCodecParams);
    avcodec_open2(pCodecContext, pCodec, NULL);
    printf("FFmpeg components initialized.\n");

    // --- ALSA 初始化 (来自 step4 的正确实现) ---
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_open(&pcm_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(pcm_handle, hw_params);
    snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, hw_params, TARGET_FORMAT);
    snd_pcm_hw_params_set_channels(pcm_handle, hw_params, TARGET_CHANNELS);
    snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, (unsigned int[]){TARGET_SAMPLE_RATE}, 0);
    snd_pcm_hw_params(pcm_handle, hw_params);
    printf("ALSA device initialized.\n");

    // --- SwrContext (重采样) 初始化 ---
    SwrContext *swr_ctx = swr_alloc_set_opts(NULL,
                                           TARGET_CHANNEL_LAYOUT, TARGET_SAMPLE_FMT, TARGET_SAMPLE_RATE,
                                           pCodecContext->channel_layout ? pCodecContext->channel_layout : av_get_default_channel_layout(pCodecContext->channels),
                                           pCodecContext->sample_fmt, pCodecContext->sample_rate,
                                           0, NULL);
    swr_init(swr_ctx);
    printf("SWR context initialized successfully.\n");

    // --- 解码与播放循环 ---
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    int current_lyric_index = 0;
    
    printf("\n--- Starting Playback ---\n\n");
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        if (pPacket->stream_index == audio_stream_index) {
            decode_and_play(pCodecContext, pPacket, pFrame, swr_ctx, pcm_handle, 
                            pAudioStream, lyrics, lyric_count, &current_lyric_index);
        }
        av_packet_unref(pPacket);
    }

    // Flush
    decode_and_play(pCodecContext, NULL, pFrame, swr_ctx, pcm_handle,
                    pAudioStream, lyrics, lyric_count, &current_lyric_index);
    printf("\n\n--- Playback Finished ---\n");

    // --- 清理工作 ---
    free(lyrics);
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    printf("ALSA device closed.\n");

    swr_free(&swr_ctx);
    printf("SWR context freed.\n");

    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    avcodec_close(pCodecContext);
    avcodec_free_context(&pCodecContext);
    avformat_close_input(&pFormatContext);
    printf("FFmpeg components freed.\n");

    return 0;
}
