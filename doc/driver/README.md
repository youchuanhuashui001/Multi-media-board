# 驱动学习笔记

本目录记录 Linux 驱动开发学习笔记，包括各种 IP 驱动的分析和代码走读。

## 目录结构

```
driver/
├── README.md           # 本文件
├── sdhci/              # SDHCI 驱动
│   ├── overview.md     # 概述
│   ├── ip_analysis.md  # IP 分析
│   └── code_walkthrough.md  # 代码走读
├── i2s/                # I2S 驱动
│   └── ...
├── i2c/                # I2C 驱动
│   └── ...
└── ...
```

## 学习模板

每个驱动的学习建议按以下结构组织：

1. **概述** (`overview.md`)
   - 驱动功能介绍
   - 硬件原理简介
   - 相关设备树节点

2. **IP 分析** (`ip_analysis.md`)
   - 寄存器说明
   - 工作时序
   - 数据流向

3. **代码走读** (`code_walkthrough.md`)
   - 驱动框架分析
   - 关键函数解析
   - 调试技巧

## 驱动列表

| 驱动 | 状态 | 说明 |
|------|------|------|
| [SDHCI](sdhci/) | 📝 待学习 | SD/MMC 控制器驱动 |
| [I2S](i2s/) | 📝 待学习 | 音频接口驱动 |
| [I2C](i2c/) | 📝 待学习 | I2C 总线驱动 |

## 学习资源

- Linux 内核源码: `drivers/` 目录
- [Linux 设备驱动程序 (LDD3)](https://lwn.net/Kernel/LDD3/)
- [Linux 内核文档](https://www.kernel.org/doc/html/latest/)

## 备注

<!-- 在此记录其他相关信息 -->
