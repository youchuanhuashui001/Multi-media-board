/*
 * FFmpeg 垂习第2步：定位音频流并打开解码器
 *
 * 简介:
 * 这个程序会在第1步的基础上，增加以下功能：
 * 1. 遍历媒体文件中的所有“流”(Stream)。
 * 2. 找到类型为 AVMEDIA_TYPE_AUDIO 的音频流。
 * 3. 根据音频流的编码ID，查找对应的解码器 (如 mp3 解码器)。
 * 4. 打开该解码器，使其处于待命状态，准备接受数据。
 *
 * 编译命令:
 * arm-buildroot-linux-gnueabihf-gcc -o step2_open_decoder step2_open_decoder.c \
 *     $(pkg-config --cflags libavformat libavcodec libavutil) \
 *     $(pkg-config --libs libavformat libavcodec libavutil)
 * 
 * (注意: 我们在 pkg-config 中增加了 libavcodec，因为本步骤开始使用解码相关API)
 * 
 * 使用方法:
 * ./step2_open_decoder /path/to/your/media.mp3
 * 
 * 预期输出:
 * Successfully opened input file './test.mp3'
 * Found audio stream at index 0
 * Found decoder 'mp3' with id 86017
 * Successfully opened decoder.
 * Decoder closed.
 * Input file closed.
 * Program exit.
 */

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <media_file_path>\n", argv[0]);
        return -1;
    }
    const char *filepath = argv[1];

    AVFormatContext *pFormatContext = NULL;
    if (avformat_open_input(&pFormatContext, filepath, NULL, NULL) != 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open input file '%s'\n", filepath);
        return -1;
    }
    printf("Successfully opened input file '%s'\n", filepath);

    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to find stream information\n");
        avformat_close_input(&pFormatContext);
        return -1;
    }

    int audio_stream_index = -1;
    AVCodecParameters *pCodecParams = NULL;

    // 1. 遍历所有流，找到音频流
    for (int i = 0; i < pFormatContext->nb_streams; i++) {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            pCodecParams = pFormatContext->streams[i]->codecpar;
            printf("Found audio stream at index %d\n", audio_stream_index);
            break; // 找到第一个音频流就退出 
        }
    }

    if (audio_stream_index == -1) {
        av_log(NULL, AV_LOG_ERROR, "Could not find audio stream in the input file\n");
        avformat_close_input(&pFormatContext);
        return -1;
    }

    // 2. 根据编码器ID查找解码器
    const AVCodec *pCodec = avcodec_find_decoder(pCodecParams->codec_id);
    if (pCodec == NULL) {
        av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for codec id %d\n", pCodecParams->codec_id);
        avformat_close_input(&pFormatContext);
        return -1;
    }
    printf("Found decoder '%s' with id %d\n", pCodec->name, pCodec->id);

    // 3. 分配解码器上下文
    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate codec context\n");
        avformat_close_input(&pFormatContext);
        return -1;
    }

    // 4. 将流中的编码包参数复制到解码器上下文
    if (avcodec_parameters_to_context(pCodecContext, pCodecParams) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy codec parameters to context\n");
        avcodec_free_context(&pCodecContext);
        avformat_close_input(&pFormatContext);
        return -1;
    }

    // 5. 打开解码器
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open decoder\n");
        avcodec_free_context(&pCodecContext);
        avformat_close_input(&pFormatContext);
        return -1;
    }
    printf("Successfully opened decoder.\n");

    // --- 清理工作 ---
    avcodec_close(pCodecContext);
    avcodec_free_context(&pCodecContext);
    printf("Decoder closed.\n");

    avformat_close_input(&pFormatContext);
    printf("Input file closed.\n");

    printf("Program exit.\n");

    return 0;
}
