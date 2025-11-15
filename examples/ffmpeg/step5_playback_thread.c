/*
 * FFmpeg 学习第5步：线程封装与基础控制
 *
 * 简介:
 * 本程序是教学的最后一步，也是最接近最终方案的一步。
 * 1. 将第4步中所有的播放逻辑（FFmpeg初始化、ALSA初始化、解码循环、清理）
 *    全部移动到一个独立的函数 `playback_thread_func` 中。
 * 2. 使用 POSIX Threads (pthreads) API 创建一个专门的播放线程来执行该函数。
 * 3. 主线程(main)只负责创建并等待播放线程结束，不再处理任何播放业务。
 * 4. 这样做可以彻底将耗时的播放操作与主线程分离，避免未来UI线程被阻塞。
 *
 * 编译命令:
 * arm-buildroot-linux-gnueabihf-gcc -o step5_playback_thread step5_playback_thread.c \
 *     $(pkg-config --cflags libavformat libavcodec libswresample libavutil alsa) \
 *     $(pkg-config --libs libavformat libavcodec libswresample libavutil alsa) -pthread
 * 
 * (注意: 我们增加了 -pthread 链接选项来引入线程库)
 * 
 * 使用方法:
 * ./step5_playback_thread /path/to/your/media.mp3
 * 
 * 预期行为:
 * 程序行为与第4步完全相同——播放指定的音频文件。
 * 但其内部架构已经改变，播放操作在一个独立的后台线程中进行。
 */

#include <stdio.h>
#include <pthread.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libswresample/swresample.h>
#include <alsa/asoundlib.h>

// ALSA 相关参数
#define ALSA_DEVICE "default"
#define TARGET_SAMPLE_RATE 44100
#define TARGET_CHANNELS 2
#define TARGET_FORMAT SND_PCM_FORMAT_S16_LE
#define TARGET_CHANNEL_LAYOUT AV_CH_LAYOUT_STEREO
#define TARGET_SAMPLE_FMT AV_SAMPLE_FMT_S16

// 线程参数结构体
typedef struct {
    const char *filepath;
} ThreadArgs;

// 播放线程函数
void* playback_thread_func(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    const char *filepath = args->filepath;

    printf("[Playback Thread] Thread started for file: %s\n", filepath);

    // --- FFmpeg 初始化 ---
    AVFormatContext *pFormatContext = NULL;
    if (avformat_open_input(&pFormatContext, filepath, NULL, NULL) != 0) {
        av_log(NULL, AV_LOG_ERROR, "[Playback Thread] Failed to open input file\n");
        return NULL;
    }
    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "[Playback Thread] Failed to find stream info\n");
        avformat_close_input(&pFormatContext);
        return NULL;
    }

    int audio_stream_index = -1;
    AVCodecParameters *pCodecParams = NULL;
    for (int i = 0; i < pFormatContext->nb_streams; i++) {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            pCodecParams = pFormatContext->streams[i]->codecpar;
            break;
        }
    }
    if (audio_stream_index == -1) {
        av_log(NULL, AV_LOG_ERROR, "[Playback Thread] Could not find audio stream\n");
        avformat_close_input(&pFormatContext);
        return NULL;
    }

    const AVCodec *pCodec = avcodec_find_decoder(pCodecParams->codec_id);
    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecContext, pCodecParams);
    avcodec_open2(pCodecContext, pCodec, NULL);
    printf("[Playback Thread] FFmpeg components initialized.\n");

    // --- ALSA 初始化 ---
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_open(&pcm_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(pcm_handle, hw_params);
    snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, hw_params, TARGET_FORMAT);
    snd_pcm_hw_params_set_channels(pcm_handle, hw_params, TARGET_CHANNELS);
    unsigned int sample_rate = TARGET_SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &sample_rate, 0);
    snd_pcm_hw_params(pcm_handle, hw_params);
    printf("[Playback Thread] ALSA device initialized.\n");

    // --- SwrContext (重采样) 初始化 ---
    SwrContext *swr_ctx = swr_alloc_set_opts(NULL,
                                           TARGET_CHANNEL_LAYOUT, TARGET_SAMPLE_FMT, TARGET_SAMPLE_RATE,
                                           pCodecContext->channel_layout, pCodecContext->sample_fmt, pCodecContext->sample_rate,
                                           0, NULL);
    swr_init(swr_ctx);
    printf("[Playback Thread] SWR context initialized.\n");

    // --- 解码与播放循环 ---
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    uint8_t *out_buffer = av_malloc(TARGET_SAMPLE_RATE * TARGET_CHANNELS * 2); // 1 second buffer

    printf("[Playback Thread] --- Starting Playback ---\n");
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        if (pPacket->stream_index == audio_stream_index) {
            int ret = avcodec_send_packet(pCodecContext, pPacket);
            if (ret < 0) break;

            while (ret >= 0) {
                ret = avcodec_receive_frame(pCodecContext, pFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                if (ret < 0) goto cleanup;

                int converted_samples = swr_convert(swr_ctx, &out_buffer, pFrame->nb_samples, 
                                                    (const uint8_t **)pFrame->extended_data, pFrame->nb_samples);
                snd_pcm_writei(pcm_handle, out_buffer, converted_samples);
            }
        }
        av_packet_unref(pPacket);
    }

    // Flush
    avcodec_send_packet(pCodecContext, NULL);
    while (avcodec_receive_frame(pCodecContext, pFrame) == 0) {
         int converted_samples = swr_convert(swr_ctx, &out_buffer, pFrame->nb_samples, 
                                            (const uint8_t **)pFrame->extended_data, pFrame->nb_samples);
         snd_pcm_writei(pcm_handle, out_buffer, converted_samples);
    }
    printf("[Playback Thread] --- Playback Finished ---\n");

cleanup:
    // --- 清理工作 ---
    av_free(out_buffer);
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    printf("[Playback Thread] ALSA device closed.\n");

    swr_free(&swr_ctx);
    printf("[Playback Thread] SWR context freed.\n");

    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    avcodec_close(pCodecContext);
    avcodec_free_context(&pCodecContext);
    avformat_close_input(&pFormatContext);
    printf("[Playback Thread] FFmpeg components freed.\n");

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <media_file_path>\n", argv[0]);
        return -1;
    }

    pthread_t tid;
    ThreadArgs args;
    args.filepath = argv[1];

    printf("[Main Thread] Creating playback thread...\n");
    int ret = pthread_create(&tid, NULL, playback_thread_func, &args);
    if (ret != 0) {
        printf("[Main Thread] Failed to create thread\n");
        return -1;
    }

    printf("[Main Thread] Waiting for playback thread to finish...\n");
    pthread_join(tid, NULL);

    printf("[Main Thread] Playback thread finished. Program exit.\n");

    return 0;
}
