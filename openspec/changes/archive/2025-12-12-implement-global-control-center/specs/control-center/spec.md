# Spec: Global Control Center

## ADDED Requirements

### Requirement: Global Visibility
控制中心必须 (MUST) 可以通过从任意应用视图顶部的下滑手势访问。

#### Scenario: Access from any view
Given 用户在任意屏幕（主页、电子书、音频等）
When 用户从屏幕顶部向下滑动
Then 全局控制中心覆盖层应向下滑出或出现
And 它应覆盖屏幕的上半部分

### Requirement: Date and Time Display
控制中心必须 (MUST) 醒目地显示当前系统日期和时间。

#### Scenario: Check time
Given 控制中心已打开
Then 当前系统时间 (HH:MM) 应以大字体显示
And 当前日期 (YYYY-MM-DD) 应以较小字体显示

### Requirement: Mini Music Player
控制中心必须 (MUST) 提供后台音乐播放的控制功能。

#### Scenario: Control playback
Given 音乐正在后台播放
And 控制中心已打开
When 用户点击“下一首”按钮
Then 播放列表中的下一首歌曲应开始播放
And 控制中心里的歌曲标题应更新

#### Scenario: Play/Pause
Given 控制中心已打开
When 用户点击播放/暂停按钮
Then 音频播放状态应切换

### Requirement: System Controls
控制中心必须 (MUST) 提供对主页和音量等系统级功能的快速访问。

#### Scenario: Return to Home
Given 用户在电子书界面
And 控制中心已打开
When 用户点击“主页”按钮
Then 控制中心应关闭
And 视图应切换回主界面

#### Scenario: Adjust Volume
Given 控制中心已打开
When 用户拖动音量滑块
Then 系统音量应相应改变
