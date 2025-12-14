# Design: 音量控制滑动条

## Context

当前 `audio_view` 布局分析（基于 1024x600 屏幕）：

```
+------------------+--------------------------------+
|                  |                                |
|  封面图 (300x300)|       歌词区域 (500x450)       |
|  pos: 50,50      |       pos: 450,0               |
|                  |                                |
+------------------+--------------------------------+
|  歌名 (50,370)   |                                |
|  歌手 (50,415)   |  进度条 (450,460)              |
|                  |                                |
|  [空白区域]      |  [模式][<<][▶][>>] [播放列表]  |
|  y: 450~580      |  btn_y: 510                    |
+------------------+--------------------------------+
```

## Goals / Non-Goals

### Goals

- 提供直观的音量调节控件
- 音量图标根据音量大小动态变化
- 与现有 UI 风格保持一致

### Non-Goals

- 不实现静音按钮（可后续迭代）
- 不实现音量记忆持久化（使用系统默认值）

## Decisions

### 1. 滑动条位置

**决策**: 放置在左侧面板下方（歌手标签下方）

**位置**: `pos: 50, 480`，宽度约 250px

**理由**:
- 左侧面板下方有充足空间（y: 450~580）
- 与歌曲信息区域垂直对齐，视觉一致
- 不干扰右侧播放控制区域
- 用户视线从封面→歌曲信息→音量控制，符合阅读习惯

**布局示意**:

```
+------------------+
|  封面图          |
|                  |
+------------------+
|  歌名            |
|  歌手            |
|                  |
|  🔊 [====●===]   |  <- 音量图标 + 滑动条
+------------------+
```

### 2. 音量图标实现方案

**决策**: 使用 PNG 图片资源

**图标文件**: 存放在 `resources/image/audio/` 目录

| 音量范围 | 图标文件 | 描述 |
|---------|---------|------|
| 0       | `volume_mute.png` | 静音图标 |
| 1-33    | `volume_low.png`  | 低音量图标 |
| 34-66   | `volume_mid.png`  | 中音量图标 |
| 67-100  | `volume_high.png` | 高音量图标 |

**理由**:
- 项目已使用图片资源作为图标（参考 `main_view.c` 中的应用图标）
- PNG 图标可精确控制视觉效果
- 便于后续更换或调整图标样式

**备选方案**: 使用 Unicode 符号（🔇🔈🔉🔊），但字体支持可能不完整

### 3. 图标动态切换实现

**决策**: 创建 `update_volume_icon()` 函数

```c
static void update_volume_icon(int volume)
{
    const char *icon_path;
    
    if (volume == 0) {
        icon_path = VOLUME_ICON_MUTE;
    } else if (volume <= 33) {
        icon_path = VOLUME_ICON_LOW;
    } else if (volume <= 66) {
        icon_path = VOLUME_ICON_MID;
    } else {
        icon_path = VOLUME_ICON_HIGH;
    }
    
    lv_image_set_src(g_audio_view.volume_icon, icon_path);
}
```

**调用时机**:
- 界面初始化时
- 滑动条值变化时（`LV_EVENT_VALUE_CHANGED`）

## UI 组件结构

```c
// 在 audio_view_t 结构体中添加
typedef struct {
    // ... 现有字段 ...
    
    // --- 音量控制 ---
    lv_obj_t *volume_icon;    // 音量图标 (lv_image)
    lv_obj_t *volume_slider;  // 音量滑动条 (lv_slider)
} audio_view_t;
```

## Risks / Trade-offs

| 风险 | 影响 | 缓解措施 |
|-----|------|---------|
| 图标资源缺失 | 界面显示异常 | 添加默认回退到文本标签 |
| 滑动条响应延迟 | 用户体验差 | 使用 `LV_EVENT_VALUE_CHANGED` 实时响应 |

## Design Decisions (Confirmed)

1. **图标尺寸**: 32x32 像素
2. **音量百分比显示**: 不显示数值，仅通过图标和滑动条位置表示音量
