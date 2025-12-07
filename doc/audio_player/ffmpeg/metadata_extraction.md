# 指南：提取媒体元数据、封面和歌词

本指南将说明如何使用 FFmpeg API 从媒体文件中提取常见的文本元数据（歌名、歌手）、内嵌的专辑封面图片以及歌词。

## 1. 提取通用元数据 (歌名, 歌手, 专辑等)

最常见的元数据以简单的“键-值”对形式存储在容器的头部。FFmpeg 在解封装时会读取这些信息，并存放在 `AVFormatContext` 的 `metadata` 成员中。

-   **核心 API**: `av_dict_get`
-   **数据结构**: `AVDictionary` (位于 `pFormatContext->metadata`)

`AVDictionary` 是一个键值对列表。我们可以通过循环调用 `av_dict_get` 来遍历其中的所有条目。

### 代码示例：遍历并打印所有元数据

```c
#include <libavformat/avformat.h>
#include <libavutil/dict.h>

void print_metadata(AVFormatContext *pFormatCtx) {
    const AVDictionaryEntry *tag = NULL;
    printf("--- Metadata ---
");
    while ((tag = av_dict_get(pFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        printf("Key: %s, Value: %s\n", tag->key, tag->value);
    }
    printf("----------------\n");
}
```

常见的键 (Key) 包括：
-   `title`: 歌名
-   `artist`: 歌手
-   `album`: 专辑
-   `genre`: 流派
-   `date`: 日期
-   `track`: 音轨号
-   `composer`: 作曲家

你可以在你的代码中调用 `av_dict_get` 并传入指定的键（如 "title"）来直接获取某个特定的值。

## 2. 提取专辑封面 (Attached Picture)

专辑封面通常不是作为简单的键值对元数据存储的，而是作为一个**特殊的附加视频流 (Attached Picture Stream)** 内嵌在文件中。

这个流的特点是：
1.  它的类型是 `AVMEDIA_TYPE_VIDEO`。
2.  它的 `disposition` 字段包含 `AV_DISPOSITION_ATTACHED_PIC` 标志位。

### 提取流程

1.  遍历 `pFormatContext->streams` 数组。
2.  找到那个同时满足 `codecpar->codec_type == AVMEDIA_TYPE_VIDEO` 和 `disposition & AV_DISPOSITION_ATTACHED_PIC` 的流。
3.  这个流 (`AVStream`) 的 `attached_pic` 成员是一个 `AVPacket`，它内部的 `data` 指针和 `size` 字段就包含了完整的图片文件数据（例如，整个 JPG 或 PNG 文件的二进制内容）。
4.  将这块内存数据写入一个文件（如 `cover.jpg`），就可以得到封面图片。

### 代码示例：查找并保存封面

```c
#include <stdio.h>
#include <libavformat/avformat.h>

void extract_album_art(AVFormatContext *pFormatCtx) {
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        AVStream *st = pFormatCtx->streams[i];
        if (st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket *pkt = &st->attached_pic;
            if (pkt->data) {
                printf("Found album art, size: %d bytes. Saving to cover.jpg\n", pkt->size);
                FILE *album_art_file = fopen("cover.jpg", "wb");
                if (album_art_file) {
                    fwrite(pkt->data, 1, pkt->size, album_art_file);
                    fclose(album_art_file);
                }
                // 通常只有一个封面，找到后即可退出循环
                break;
            }
        }
    }
}
```

## 3. 提取歌词

提取歌词比前两者要复杂，因为没有一个被普遍遵守的统一标准。主要有两种可能的方式：

### 方式一：作为普通元数据 (最常见)

对于很多音频文件（特别是 MP3），歌词被存储在一个特殊的元数据标签里。这种方式最简单，处理方法和获取歌名、歌手完全一样。

-   **常见键名**: `lyrics`, `lyrics-eng`, `USLT` (ID3v2 标准中的非同步歌词标签)。

You need to check for these keys when iterating through `pFormatContext->metadata`.

### 方式二：作为字幕流 (较少见)

对于一些更复杂的容器，特别是视频文件，或者需要支持卡拉OK式的同步歌词时，歌词可能会被编码成一个独立的**字幕流 (`AVMEDIA_TYPE_SUBTITLE`)**。

如果属于这种情况，你需要：
1.  在 `pFormatContext->streams` 中找到类型为 `AVMEDIA_TYPE_SUBTITLE` 的流。
2.  像处理音频/视频流一样，为它查找并打开一个解码器 (`avcodec_find_decoder`)。
3.  在主循环 (`av_read_frame`) 中，处理属于这个字幕流的 `AVPacket`。
4.  解码字幕包得到的是 `AVSubtitle` 结构体，而不是 `AVFrame`。你需要从 `AVSubtitle` 中提取文本信息。

**结论与建议**: 
对于音乐播放器，**首先应该尝试方式一**，即在 `metadata` 字典中查找 `lyrics` 相关的键。这覆盖了绝大多数场景。只有在需要支持复杂的同步歌词显示时，才需要考虑实现方式二的完整字幕解码流程。

## 4. 整合到你的代码中

将上述功能整合到你的 `step4_play_audio.c` 中，最佳的位置是在 `avformat_find_stream_info()` 调用成功之后，因为此时 `pFormatContext` 已经被完整地填充了所有信息。

```c
// 在你的 main 函数中:
// ...
avformat_find_stream_info(pFormatContext, NULL);

// 在这里调用元数据提取函数
print_metadata(pFormatContext);
extract_album_art(pFormatContext);

// 检查歌词 (简单方式)
const AVDictionaryEntry *lyrics_tag = av_dict_get(pFormatContext->metadata, "lyrics", NULL, 0);
if (lyrics_tag) {
    printf("Found Lyrics (simple metadata):\n%s\n", lyrics_tag->value);
}

// ... 接下来是查找音频流、打开解码器等代码
```