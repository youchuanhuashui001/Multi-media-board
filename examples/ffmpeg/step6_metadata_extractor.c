/*
 * FFmpeg 学习第6步：提取元数据与封面
 *
 * 简介:
 * 本程序演示如何从媒体文件中读取元数据（如标题、艺术家）和内嵌的专辑封面。
 * 1. 打开媒体文件并读取流信息。
 * 2. 遍历 AVFormatContext 中的 metadata 字典，打印所有的键值对。
 * 3. 遍历所有流，查找类型为附加图片（Attached Picture）的视频流。
 * 4. 如果找到，将其数据包（AVPacket）的内容保存为 "cover.jpg" 文件。
 *
 * 编译命令 (PC):
 * gcc -o step6_metadata_extractor step6_metadata_extractor.c \
 *     $(pkg-config --cflags libavformat libavutil) \
 *     $(pkg-config --libs libavformat libavutil)
 *
 * (注意: 本程序不涉及解码，因此只需要 libavformat 和 libavutil)
 *
 * 使用方法:
 * ./step6_metadata_extractor /path/to/your/media.mp3
 *
 * 预期行为:
 * 程序运行后，会打印出文件中包含的元数据，
 * 如果有内嵌封面，会提示找到并保存为 cover.jpg。
 */

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/log.h>

/**
 * @brief 遍历并打印 AVFormatContext 中的所有元数据条目。
 * @param pFormatCtx 包含元数据的格式上下文。
 */
static void print_all_metadata(AVFormatContext *pFormatCtx) {
    const AVDictionaryEntry *tag = NULL;
    printf("--- Media Metadata ---\n");

    if (!pFormatCtx->metadata) {
        printf("No metadata found.\n");
        printf("----------------------\n");
        return;
    }

    // av_dict_get 的第三个参数是前一个条目，传入 NULL 或之前的 tag
    // 第四个参数是标志位，AV_DICT_IGNORE_SUFFIX 表示不区分大小写地匹配键
    while ((tag = av_dict_get(pFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        printf("%-20s: %s\n", tag->key, tag->value);
    }

    // 尝试直接获取歌词
    tag = av_dict_get(pFormatCtx->metadata, "lyrics", NULL, AV_DICT_IGNORE_SUFFIX);
    if (tag) {
        printf("\n--- Found Lyrics ---\n%s\n", tag->value);
    }

    printf("----------------------\n");
}

/**
 * @brief 在流中查找附加的封面图片并将其保存到文件。
 * @param pFormatCtx 要搜索的格式上下文。
 */
static void extract_album_art(AVFormatContext *pFormatCtx) {
    printf("\n--- Searching for Album Art ---\n");
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        AVStream *st = pFormatCtx->streams[i];
        // 检查流的 disposition 标志位是否包含 AV_DISPOSITION_ATTACHED_PIC
        if (st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket *pkt = &st->attached_pic;
            if (pkt && pkt->data) {
                printf("Found album art in stream #%d, size: %d bytes.\n", i, pkt->size);
                
                FILE *album_art_file = fopen("cover.jpg", "wb");
                if (album_art_file) {
                    fwrite(pkt->data, 1, pkt->size, album_art_file);
                    fclose(album_art_file);
                    printf("Successfully saved album art to 'cover.jpg'.\n");
                } else {
                    fprintf(stderr, "Error: Could not open 'cover.jpg' for writing.\n");
                }
                // 通常只有一个封面，找到后即可返回
                return;
            }
        }
    }
    printf("No album art found in this file.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <media_file_path>\n", argv[0]);
        return -1;
    }
    const char *filepath = argv[1];

    AVFormatContext *pFormatContext = NULL;

    // 1. 打开媒体文件
    if (avformat_open_input(&pFormatContext, filepath, NULL, NULL) != 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open input file '%s'\n", filepath);
        return -1;
    }

    // 2. 查找流信息
    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to find stream information\n");
        avformat_close_input(&pFormatContext);
        return -1;
    }

    // 3. 打印所有元数据
    print_all_metadata(pFormatContext);

    // 4. 提取封面
    extract_album_art(pFormatContext);

    // 5. 清理
    avformat_close_input(&pFormatContext);

    printf("\nExtraction finished. Program exit.\n");

    return 0;
}
