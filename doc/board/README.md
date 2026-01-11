# i.MX6ULL 开发板文档

本目录记录 i.MX6ULL 开发板相关的硬件信息、启动配置和系统参数。

## 目录结构

```
board/
├── README.md              # 本文件
├── boot/                  # 启动相关
│   ├── uboot_bootargs.md  # U-Boot 启动参数
│   ├── memory_layout.md   # 内存布局 (DTB/Image/存储地址)
│   └── boot_flow.md       # 启动流程
└── hardware/              # 硬件规格
    └── imx6ull_specs.md   # 芯片规格
```

## 快速链接

- [U-Boot 启动参数](boot/uboot_bootargs.md)
- [内存布局](boot/memory_layout.md)
- [启动流程](boot/boot_flow.md)
- [芯片规格](hardware/imx6ull_specs.md)

## 开发板信息

| 项目 | 描述 |
|------|------|
| 芯片型号 | i.MX6ULL |
| 开发板 | 100ASK_IMX6ULL_PRO |
| 启动介质 | EMMC / SD 卡 |
| 调试方式 | 串口 / JTAG |
