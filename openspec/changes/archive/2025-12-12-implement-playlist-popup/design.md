# Design: Playlist Popup

## Architecture
- **UI Component**: 一个新的 `lv_obj` 容器，作为模态对话框。
- **List Widget**: 使用 `lv_table` 显示歌曲详情（序号、标题、歌手、时长）。
- **Data Source**: 访问 `audio_manager` 模块的播放列表数据。
- **Interaction**:
    - 打开：点击主屏幕上的"播放列表"按钮。
    - 关闭：点击遮罩层（列表外部区域）。
    - 播放：点击表格中一行的任意位置。

## UI Layout
- **Popup**: 全屏 (1024x600)，半透明黑色背景 (`LV_OPA_60`)。
- **List Container**: 居中，800x500，深灰色背景 (`#1e1e1e`)，圆角 10px。
- **Table Columns**:
    - 序号 (80px)
    - 标题 (380px)
    - 歌手 (220px)
    - 时长 (100px)

## Logic
- **Initialization**: 创建弹窗和表格，默认隐藏，注册事件回调。
- **Update**: 打开时刷新表格显示。
- **Event Handling**:
    - `playlist_popup_event_cb`: 点击遮罩层关闭弹窗。
    - `playlist_item_event_cb`: 处理 `LV_EVENT_VALUE_CHANGED` 检测行选择，调用 `audio_manager_play_at_index()`。

## Files Modified
- `audio_view.c`: 添加弹窗创建、事件回调、样式设置。
- `audio_manager.h/c`: 添加 `audio_manager_get_current_index()` 函数。
- `lv_conf.h`: 启用 `LV_CACHE_DEF_SIZE` (4MB) 用于图片缓存优化。
