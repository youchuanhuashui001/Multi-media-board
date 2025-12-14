# Tasks: 添加音量控制滑动条

## 1. 资源准备（由用户提供）

- [x] 1.1 准备音量图标 PNG 文件（32x32 像素）
  - `resources/image/audio/volume_mute.png` - 静音图标
  - `resources/image/audio/volume_low.png` - 低音量图标
  - `resources/image/audio/volume_mid.png` - 中音量图标
  - `resources/image/audio/volume_high.png` - 高音量图标

## 2. Implementation

- [x] 2.1 在 `audio_view_t` 结构体中添加音量相关 UI 元素
  - `lv_obj_t *volume_icon` - 音量图标 (lv_image)
  - `lv_obj_t *volume_slider` - 音量滑动条 (lv_slider)
- [x] 2.2 定义图标路径宏
  - `VOLUME_ICON_MUTE`
  - `VOLUME_ICON_LOW`
  - `VOLUME_ICON_MID`
  - `VOLUME_ICON_HIGH`
- [x] 2.3 创建 `update_volume_icon()` 函数
  - 根据音量值 (0, 1-33, 34-66, 67-100) 切换图标
- [x] 2.4 创建 `create_volume_control()` 函数
  - 创建音量图标 `lv_image_create()`，位置: (50, 480)
  - 创建滑动条 `lv_slider_create()`，位置: (90, 488)，宽度 210px
  - 设置滑动条范围 0-100
  - 设置初始值为 `audio_engine_get_volume()`
  - 调用 `update_volume_icon()` 设置初始图标
- [x] 2.5 实现 `volume_slider_event_cb()` 事件回调
  - 监听 `LV_EVENT_VALUE_CHANGED` 事件
  - 调用 `audio_engine_set_volume()` 设置音量
  - 调用 `update_volume_icon()` 更新图标
- [x] 2.6 在 `audio_view_init()` 中调用 `create_volume_control()`

## 3. Testing

- [x] 3.1 验证滑动条拖动时音量实时变化
- [x] 3.2 验证音量图标根据音量值正确切换
  - 0: 静音图标
  - 1-33: 低音量图标
  - 34-66: 中音量图标
  - 67-100: 高音量图标
- [x] 3.3 验证界面加载时显示正确的初始音量和图标
