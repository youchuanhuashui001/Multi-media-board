#include <stdio.h>

// 引入 FFmpeg 的主要头文件
// libavformat 用于处理多种多媒体容器格式
#include <libavformat/avformat.h>
// libavutil 提供各种工具函数
#include <libavutil/log.h>

int main(int argc, char *argv[]) {
    // 1. 检查输入参数
    if (argc < 2) {
        printf("Usage: %s <media_file_path>\n", argv[0]);
        return -1;
    }
    const char *filepath = argv[1];

    // 2. 创建一个 AVFormatContext 结构体，这是 FFmpeg 操作的“句柄”
    AVFormatContext *pFormatContext = NULL;

    printf("Step 1: Opening input file '%s'\n", filepath);

    // 3. 打开媒体文件
    // avformat_open_input 会探测文件格式，并读取文件头信息
    if (avformat_open_input(&pFormatContext, filepath, NULL, NULL) != 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open input file '%s'\n", filepath);
        return -1;
    }

    printf("Step 2: Finding stream info\n");

    // 4. 查找流信息
    // avformat_find_stream_info 会读取一部分数据来填充流信息，如时长、码率等
    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to find stream information\n");
        avformat_close_input(&pFormatContext);
        return -1;
    }

    printf("Step 3: Dumping format info\n---\n");

    // 5. 打印媒体文件的详细信息
    // 这是一个非常有用的调试函数，它会将 pFormatContext 中的所有信息格式化输出
    av_dump_format(pFormatContext, 0, filepath, 0);

    printf("---\n");

    // 6. 清理和释放资源
    avformat_close_input(&pFormatContext);

    printf("Cleanup finished. Program exit.\n");

    return 0;
}
