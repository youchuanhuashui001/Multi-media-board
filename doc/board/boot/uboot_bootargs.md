# U-Boot 启动参数 (bootargs)

## env
```
=> printenv
baudrate=115200
board_name=EVK
board_rev=14X14
boot_fdt=try
bootcmd=run updateset;run findfdt;run findtee;mmc dev ${mmcdev};mmc dev ${mmcdev}; if mmc rescan; then if run loadbootscript; then run bootscript; else if run loadimage; then run mmcboot; else run netboot; fi; fi; else run netboot; fi
bootcmd_mfg=run mfgtool_args; if test ${tee} = yes; then bootm ${tee_addr} ${initrd_addr} ${fdt_addr}; else bootz ${loadaddr} ${initrd_addr} ${fdt_addr}; fi;
bootdelay=3
bootdir=/boot
bootscript=echo Running bootscript from mmc ...; source
console=ttymxc0
eth1addr=00:01:3f:2d:3e:4d
ethact=ethernet@020b4000
ethaddr=00:01:1f:2d:3e:4d
ethprime=eth1
fdt_addr=0x83000000
fdt_file=100ask_imx6ull-14x14.dtb
fdt_high=0xffffffff
fdtcontroladdr=9ef40478
findfdt=if test $fdt_file = undefined; then if test $board_name = EVK && test $board_rev = 9X9; then setenv fdt_file imx6ull-9x9-evk.dtb; fi; if test $board_name = EVK && test $board_rev = 14X14; then setenv fdt_file imx6ull-14x14-evk.dtb; fi; if test $fdt_file = undefined; then setenv fdt_file imx6ull-14x14-alpha.dtb; fi; fi;
image=zImage
initrd_addr=0x83800000
initrd_high=0xffffffff
ip_dyn=yes
loadaddr=0x80800000
loadbootscript=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};
loadfdt=ext2load mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${bootdir}/${fdt_file}
loadimage=ext2load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${bootdir}/${image}
loadtee=fatload mmc ${mmcdev}:${mmcpart} ${tee_addr} ${tee_file}
mfgtool_args=setenv bootargs console=${console},${baudrate} rdinit=/linuxrc g_mass_storage.stall=0 g_mass_storage.removable=1 g_mass_storage.file=/fat g_mass_storage.ro=1 g_mass_storage.idVendor=0x066F g_mass_storage.idProduct=0x37FF g_mass_storage.iSerialNumber="" clk_ignore_unused 
mmcargs=setenv bootargs console=${console},${baudrate} root=${mmcroot}
mmcautodetect=no
mmcboot=echo Booting from mmc ...; run mmcargs; if test ${tee} = yes; then run loadfdt; run loadtee; bootm ${tee_addr} - ${fdt_addr}; else if test ${boot_fdt} = yes || test ${boot_fdt} = try; then if run loadfdt; then bootz ${loadaddr} - ${fdt_addr}; else if test ${boot_fdt} = try; then bootz; else echo WARN: Cannot load the DT; fi; fi; else bootz; fi; fi;
mmcdev=1
mmcpart=2
mmcroot=/dev/mmcblk1p2 rootwait rw
netargs=setenv bootargs console=${console},${baudrate} root=/dev/nfs ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp
netboot=echo Booting from net ...; run netargs; setenv get_cmd tftp; ${get_cmd} ${image}; ${get_cmd} ${fdt_addr} ${fdt_file};  bootz ${loadaddr} - ${fdt_addr};
panel=TFT7016
script=boot.scr
tee=no
tee_addr=0x84000000
tee_file=uTee-6ullevk
update=yes
updateset=if test $update = undefined; then setenv update yes; saveenv; fi;

Environment size: 2765/8188 bytes

```

## 环境变量详解

### 📌 基础配置

| 变量 | 值 | 说明 |
|------|-----|------|
| `baudrate` | `115200` | 串口波特率 |
| `board_name` | `EVK` | 开发板名称 (Evaluation Kit) |
| `board_rev` | `14X14` | 开发板版本 (14x14mm 封装) |
| `console` | `ttymxc0` | Linux 控制台设备 (i.MX UART1) |
| `panel` | `TFT7016` | LCD 屏幕型号 |

### 📌 内存地址配置

| 变量 | 值 | 说明 |
|------|-----|------|
| `loadaddr` | `0x80800000` | **zImage 加载地址** (DDR 起始 0x80000000 + 8MB 偏移) |
| `fdt_addr` | `0x83000000` | **DTB 加载地址** |
| `initrd_addr` | `0x83800000` | initrd (初始 ramdisk) 加载地址 |
| `tee_addr` | `0x84000000` | TEE (可信执行环境) 加载地址 |
| `fdt_high` | `0xffffffff` | DTB 可放置的最高地址 (不限制) |
| `initrd_high` | `0xffffffff` | initrd 可放置的最高地址 (不限制) |
| `fdtcontroladdr` | `0x9ef40478` | U-Boot 自用的 DTB 控制地址 |

**内存布局图：**
```
DDR (从 0x80000000 开始)
┌─────────────────┐ 0x80000000
│    保留区域      │
├─────────────────┤ 0x80800000  ← loadaddr (zImage)
│    zImage       │
│   (~6-8MB)      │
├─────────────────┤ 0x83000000  ← fdt_addr (DTB)
│    DTB          │
│   (~64KB)       │
├─────────────────┤ 0x83800000  ← initrd_addr
│   initrd        │
├─────────────────┤ 0x84000000  ← tee_addr
│   TEE (可选)    │
└─────────────────┘
```

### 📌 MMC/EMMC 相关

| 变量 | 值 | 说明 |
|------|-----|------|
| `mmcdev` | `1` | MMC 设备号 (**1 = EMMC**, 0 = SD 卡) |
| `mmcpart` | `2` | MMC 分区号 (第 2 分区包含 rootfs) |
| `mmcroot` | `/dev/mmcblk1p2 rootwait rw` | 根文件系统设备及挂载选项 |
| `mmcautodetect` | `no` | 是否自动检测 MMC 设备 |
| `bootdir` | `/boot` | 内核和 DTB 所在目录 |

### 📌 镜像文件名

| 变量 | 值 | 说明 |
|------|-----|------|
| `image` | `zImage` | Linux 内核镜像文件名 |
| `fdt_file` | `100ask_imx6ull-14x14.dtb` | **设备树文件名** (100ASK 定制) |
| `script` | `boot.scr` | 启动脚本文件名 |
| `tee_file` | `uTee-6ullevk` | TEE 镜像文件名 |

### 📌 网络相关

| 变量 | 值 | 说明 |
|------|-----|------|
| `ethaddr` | `00:01:1f:2d:3e:4d` | 主网卡 MAC 地址 |
| `eth1addr` | `00:01:3f:2d:3e:4d` | 第二网卡 MAC 地址 |
| `ethprime` | `eth1` | 默认使用的网卡 |
| `ip_dyn` | `yes` | 是否动态获取 IP (DHCP) |

### 📌 启动脚本/命令

| 变量 | 说明 |
|------|------|
| `bootcmd` | **主启动命令** - U-Boot 启动后自动执行的命令序列 |
| `bootcmd_mfg` | 工厂烧录模式启动命令 |
| `bootdelay` | 启动延迟秒数 (3秒)，期间可按键中断进入命令行 |
| `boot_fdt` | `try` - 尝试使用设备树启动 |

### 📌 加载命令 (脚本)

| 变量 | 说明 |
|------|------|
| `loadimage` | 从 MMC 加载内核: `ext2load mmc 1:2 0x80800000 /boot/zImage` |
| `loadfdt` | 从 MMC 加载 DTB: `ext2load mmc 1:2 0x83000000 /boot/100ask_imx6ull-14x14.dtb` |
| `loadbootscript` | 加载启动脚本 boot.scr |
| `loadtee` | 加载 TEE 镜像 |

### 📌 bootargs 生成命令

| 变量 | 说明 |
|------|------|
| `mmcargs` | 生成 EMMC 启动的 bootargs: `console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw` |
| `netargs` | 生成网络启动的 bootargs: `console=ttymxc0,115200 root=/dev/nfs ip=dhcp nfsroot=...` |
| `mfgtool_args` | 生成工厂模式的 bootargs |

### 📌 TEE (可信执行环境)

| 变量 | 值 | 说明 |
|------|-----|------|
| `tee` | `no` | 是否启用 TEE (**当前禁用**) |
| `tee_addr` | `0x84000000` | TEE 加载地址 |
| `tee_file` | `uTee-6ullevk` | TEE 镜像文件 |

### 📌 辅助脚本

| 变量 | 说明 |
|------|------|
| `findfdt` | 根据 board_name 和 board_rev 自动选择正确的 DTB 文件 |
| `updateset` | 检查并设置 update 变量 |
| `mmcboot` | 完整的 MMC 启动流程 (设置 args → 加载镜像 → 启动) |
| `netboot` | 完整的网络启动流程 (TFTP 下载 → 启动) |
| `bootscript` | 执行加载的启动脚本 |

---

## 启动流程解析

当 U-Boot 启动时，`bootcmd` 会依次执行：

run updateset: 执行名为 updateset 的变量脚本。根据你之前的 printenv 输出，它是用来初始化 update 状态的。

run findfdt: 自动检测并设置设备树文件名（fdt_file）。它会根据硬件版本（如 9x9 或 14x14）来决定加载哪个 .dtb 文件。

run findtee: 检查是否存在 TEE（可信执行环境/指纹加密等安全相关）映像。

mmc dev ${mmcdev}: 切换到指定的 MMC 设备（这里 ${mmcdev} 是 1，即 eMMC 或 SD 卡）。

if mmc rescan; then ...: 尝试扫描该存储设备。如果设备存在且响应，则进入下一步。

if run loadbootscript; then run bootscript;:

尝试从分区中加载名为 boot.scr 的脚本文件。

如果找到了这个脚本，就执行它（run bootscript）。这通常用于实现灵活的启动逻辑，而不需要修改 U-Boot 环境变量。

else if run loadimage; then run mmcboot; else run netboot; fi; fi; else run netboot; fi
意思就是：如果没有找到 boot.scr 脚本，就直接尝试加载内核镜像（zImage）并按照预设的 mmcboot 流程启动。


通过接下来的打印，可以看到 emmc 中并没有 boot.scr，所以会加载 zImage 并执行 mmcboot
```
=> boot
## Error: "findtee" not defined
switch to partitions #0, OK
mmc1(part 0) is current device
switch to partitions #0, OK
mmc1(part 0) is current device
** Unrecognized filesystem type **
7924872 bytes read in 411 ms (18.4 MiB/s)
Booting from mmc ...
38370 bytes read in 51 ms (734.4 KiB/s)
Kernel image @ 0x80800000 [ 0x000000 - 0x78ec88 ]
## Flattened Device Tree blob at 83000000
   Booting using the fdt blob at 0x83000000
   Using Device Tree in place at 83000000, end 8300c5e1
Modify /soc/aips-bus@02200000/epdc@0228c000:status disabled
ft_system_setup for mx6
```

### mmcboot 流程：

run loadimage:
`ext2load mmc 1:2 0x80800000 /boot/zImage`

run mmcboot:
`echo Booting from mmc ...; run mmcargs; if test ${tee} = yes; then run loadfdt; run loadtee; bootm ${tee_addr} - ${fdt_addr}; else if test ${boot_fdt} = yes || test ${boot_fdt} = try; then if run loadfdt; then bootz ${loadaddr} - ${fdt_addr}; else if test ${boot_fdt} = try; then bootz; else echo WARN: Cannot load the DT; fi; fi; else bootz; fi; fi;`

run mmcargs:
配置 bootargs = `console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw`

run loadfdt:
`ext2load mmc 1:2 0x83000000 /boot/100ask_imx6ull-14x14.dtb`

bootz:
`bootz 0x80800000 - 0x83000000`

接下来 linux 启动了，执行 sdhci 驱动程序的 probe 识别到了 mmc 卡，在挂载根文件系统时先尝试 ext3 格式，再尝试 ext4 格式，挂载根文件系统成功，执行 init 脚本
```
[ 3.374364] mmc1: SDHCI controller on 2194000.usdhc [2194000.usdhc] using ADMA
[ 3.489507] mmc1: new DDR MMC card at address 0001
[ 3.506379] mmcblk1: mmc1:0001 Q2J54A 3.64 GiB

[ 4.063182] EXT4-fs (mmcblk1p2): couldn't mount as ext3 due to feature incompatibilities
[ 4.099113] EXT4-fs (mmcblk1p2): mounted filesystem with ordered data mode. Opts: (null)
[ 4.107532] VFS: Mounted root (ext4 filesystem) on device 179:2.

[ 4.120551] Freeing unused kernel memory: 1024K
[ 4.375265] EXT4-fs (mmcblk1p2): re-mounted. Opts: data=ordered
```

### netboot 流程

```
netboot=echo Booting from net ...; run netargs; setenv get_cmd tftp; ${get_cmd} ${image}; ${get_cmd} ${fdt_addr} ${fdt_file};  bootz ${loadaddr} - ${fdt_addr};
```

run netargs:
配置 bootargs = `console=ttymxc0,115200 root=/dev/nfs ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp`

tftp zImage: tftp 0x80800000 zImage

在 U-Boot 中，tftp 命令的标准语法是：tftp [loadAddress] [[hostIPaddr:]bootfilename]。

如果你输入 tftp zImage 而没有手动写地址，U-Boot 会自动调用内置变量 ${loadaddr} 作为默认目标地址。

tftp 0x83000000 100ask_imx6ull-14x14.dtb
bootz 0x80800000 - 0x83000000

- nfs root 启动命令：(必须要关掉 eth0，否则默认会用 dhcp 申请 ip)
```
setenv ipaddr 192.168.31.9
setenv serverip 192.168.31.10
setenv bootargs console=ttymxc0,115200 root=/dev/nfs ip=192.168.31.9:192.168.31.10:192.168.31.1:255.255.255.0::eth0:off nfsroot=192.168.31.10:/opt/nfs/rootfs,v3,tcp rw

tftp zImage
tftp 0x83000000 100ask_imx6ull-14x14.dtb
bootz 0x80800000 - 0x83000000

setenv tanxzh_boot_nfs "setenv ipaddr 192.168.31.9; setenv serverip 192.168.31.10; setenv bootargs 'console=ttymxc0,115200 root=/dev/nfs ip=192.168.31.9:192.168.31.10:192.168.31.1:255.255.255.0::eth0:off nfsroot=192.168.31.10:/opt/nfs/rootfs,v3,tcp rw'; tftp 0x80800000 zImage; tftp 0x83000000 100ask_imx6ull-14x14.dtb; bootz 0x80800000 - 0x83000000"

run tanxzh_boot_nfs

如果卡在 init 启动过程中：
sudo vi /opt/nfs/rootfs/etc/network/interfaces
找到关于 eth0 的配置（通常有 auto eth0 和 iface eth0 inet dhcp），在它们前面加 # 注释掉：

如果 rootfs 不能登录用户，需要确保 pc 上的 rootfs 目录全部是 root 用户权限，否则使用命令切换：
sudo chown -R root:root .
然后在 pc 上需要使用 sudo 的方式访问这些文件夹
```

---

## bootargs 参数说明

| 参数 | 值 | 说明 |
|------|-----|------|
| `console` | `ttymxc0,115200` | 串口控制台，波特率 115200 |
| `root` | `/dev/mmcblk1p2` | 根文件系统设备 |
| `rootwait` | - | 等待根文件系统设备就绪 |
| `rw` | - | 以读写模式挂载根文件系统 |

## 常用 bootargs 配置

### 1. EMMC 启动

```bash
setenv bootargs 'console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw'
```

### 2. NFS 启动

```bash
setenv bootargs 'console=ttymxc0,115200 root=/dev/nfs nfsroot=192.168.1.100:/home/nfs/rootfs,proto=tcp rw ip=192.168.1.50:192.168.1.100:192.168.1.1:255.255.255.0::eth0:off'
```

## U-Boot 环境变量

```bash
# 查看所有环境变量
printenv

# 保存环境变量
saveenv

# 重置环境变量
env default -a
saveenv
```

## 备注

<!-- 在此记录其他相关信息 -->
