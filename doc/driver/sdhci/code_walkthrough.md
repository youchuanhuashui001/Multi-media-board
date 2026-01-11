# SDHCI 代码走读

## 驱动框架

```
┌─────────────────────────────────────────┐
│              Block Layer                 │
├─────────────────────────────────────────┤
│              MMC Core                    │
│         (drivers/mmc/core/)             │
├─────────────────────────────────────────┤
│              SDHCI Core                  │
│         (drivers/mmc/host/sdhci.c)      │
├─────────────────────────────────────────┤
│          SDHCI Platform                  │
│    (drivers/mmc/host/sdhci-esdhc-imx.c) │
├─────────────────────────────────────────┤
│              Hardware                    │
└─────────────────────────────────────────┘
```

## 关键数据结构

### struct sdhci_host

```c
struct sdhci_host {
	const char *hw_name;     /* 硬件名称 */
	void __iomem *ioaddr;    /* 寄存器基地址 */
	struct mmc_host *mmc;    /* MMC 主机结构 */
	struct sdhci_ops *ops;   /* 操作函数 */
	/* ... */
};
```

### struct sdhci_ops

```c
struct sdhci_ops {
	void (*reset)(struct sdhci_host *host, u8 mask);
	void (*set_clock)(struct sdhci_host *host, unsigned int clock);
	void (*set_uhs_signaling)(struct sdhci_host *host, unsigned int uhs);
	/* ... */
};
```

## 关键函数

### 初始化流程

```c
// 平台驱动探测
sdhci_esdhc_imx_probe()
    └── sdhci_pltfm_init()
            └── sdhci_add_host()
                    └── mmc_add_host()
```

### 数据传输

```c
// 读写请求处理
sdhci_request()
    └── sdhci_send_command()
            └── sdhci_prepare_data()
                    └── sdhci_adma_table_pre()  // ADMA
```

### 中断处理

```c
sdhci_irq()
    ├── sdhci_cmd_irq()      // 命令完成
    └── sdhci_data_irq()     // 数据完成
```

## 调试技巧

### 打开调试日志

```bash
# 动态调试
echo 'file sdhci.c +p' > /sys/kernel/debug/dynamic_debug/control
echo 'file sdhci-esdhc-imx.c +p' > /sys/kernel/debug/dynamic_debug/control
```

### 查看 MMC 信息

```bash
# 查看 SD 卡信息
cat /sys/class/mmc_host/mmc0/mmc0:*/cid
cat /sys/class/mmc_host/mmc0/mmc0:*/csd
```

## 备注

<!-- 在此记录其他相关信息 -->
