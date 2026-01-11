# SDHCI IP 分析

## 寄存器概述

uSDHC 控制器基地址 (以 i.MX6ULL 为例):
- uSDHC1: `0x02190000`
- uSDHC2: `0x02194000`

## 关键寄存器

| 偏移 | 寄存器名 | 说明 |
|------|----------|------|
| 0x00 | DS_ADDR | DMA 系统地址 |
| 0x04 | BLK_ATT | 块属性 |
| 0x08 | CMD_ARG | 命令参数 |
| 0x0C | CMD_XFR_TYP | 命令传输类型 |
| 0x10-0x1C | CMD_RSP0-3 | 命令响应 |
| 0x20 | DATA_BUFF_ACC_PORT | 数据缓冲区访问端口 |
| 0x24 | PRES_STATE | 当前状态 |
| 0x28 | PROT_CTRL | 协议控制 |
| 0x2C | SYS_CTRL | 系统控制 |
| 0x30 | INT_STATUS | 中断状态 |
| 0x34 | INT_STATUS_EN | 中断状态使能 |
| 0x38 | INT_SIGNAL_EN | 中断信号使能 |

## 命令类型

| 类型 | 说明 |
|------|------|
| CMD0 | GO_IDLE_STATE |
| CMD2 | ALL_SEND_CID |
| CMD3 | SEND_RELATIVE_ADDR |
| CMD7 | SELECT_CARD |
| CMD17 | READ_SINGLE_BLOCK |
| CMD18 | READ_MULTIPLE_BLOCK |
| CMD24 | WRITE_BLOCK |
| CMD25 | WRITE_MULTIPLE_BLOCK |

## 数据传输流程

```
1. 设置 BLK_ATT (块大小、块数量)
2. 设置 DS_ADDR (DMA 地址)
3. 设置 CMD_ARG (命令参数)
4. 设置 CMD_XFR_TYP (发送命令)
5. 等待中断 (数据传输完成)
6. 检查 INT_STATUS
```

## 时钟配置

```
SD 时钟频率 = 基础时钟 / (SDCLKFS × DVS)

SDCLKFS: 时钟分频选择
DVS: 除数值选择
```

## 备注

<!-- 在此记录其他相关信息 -->
