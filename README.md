# STM32H750VBTx Bootloader

基于 STM32H750VBTx 的串口固件升级引导程序。通过 USART1 接收上位机固件包，写入外部 QSPI Flash（W25Q64），校验通过后跳转执行应用程序。

## 硬件规格

| 项目 | 参数 |
| ---- | ---- |
| 主控 | STM32H750VBTx (Cortex-M7, 480 MHz) |
| 内部 Flash | 128 KB（仅存放 Bootloader） |
| 外部 Flash | W25Q64 (QSPI, 8 MB，存放用户固件） |
| 调试接口 | SWD (PA13/PA14) |
| 串口 | USART1, 115200-8N1 |
| 晶振 | HSE 25 MHz, LSE 32.768 kHz |

## 内存布局

| 区域 | 地址 | 大小 | 用途 |
| ---- | ---- | ---- | ---- |
| ITCMRAM | `0x00000000` | 64 KB | 指令紧耦合内存（未使用） |
| FLASH | `0x08000000` | 128 KB | 内部 Flash，Bootloader 本体 |
| DTCMRAM | `0x20000000` | 128 KB | .data、.bss、堆、栈 |
| RAM_D1 | `0x24000000` | 512 KB | 通用 SRAM |
| RAM_D2 | `0x30000000` | 288 KB | 通用 SRAM |
| RAM_D3 | `0x38000000` | 64 KB | 通用 SRAM |
| QSPI Flash | `0x90000000` | 8 MB | 外部 Flash，内存映射访问 |

## 时钟配置

- **HSE** (25 MHz) → PLL1 (÷5, ×192, ÷2) → **SYSCLK 480 MHz**, **HCLK 240 MHz**, **APB 120 MHz**
- **HSE** → PLL2 (÷16, ×192, ÷2) → QSPI 150 MHz / USART1 75 MHz
- QSPI 实际时钟 = 150 MHz ÷ (prescaler+1) = **75 MHz**（prescaler=1, 半周期采样偏移）
- LSI (32.768 kHz) → RTC
- 电源模式：LDO, Voltage Scaling 0（最高性能）

## MPU 配置

采用"默认拒绝 + 白名单开放"策略：

| 区域 | 地址范围 | 大小 | 属性 | 说明 |
| ---- | -------- | ---- | ---- | ---- |
| Region 0 | `0x00000000`–`0xFFFFFFFF` | 4 GB | 背景拒绝 | SubRegionDisable=0x87，部分放行 |
| Region 1 | `0x90000000` | 8 MB | RWX, 可缓存 | QSPI Flash 映射区 |
| Region 2 | `0x20000000` | 1 MB | RW, 不可缓存 | DTCM（保证 DMA 一致性） |

I-Cache 和 D-Cache 在 MPU 配置之后、HAL 初始化之前使能。

## 架构

项目采用**分层架构**，依赖方向：Application → Device → Platform → HAL, Kernel → HAL。

```
Application/     业务逻辑层
  ├── app_bootloader.c    Bootloader 主循环：收帧 → 分发 → 回复 ACK
  ├── app_protocol.c      协议帧收发 + CRC-16 校验 + 命令分发
  └── app_firmware.c      固件写入/校验/跳转 + CRC32 校验

Device/          设备驱动层
  ├── dev_qspi_flash.c    QSPI Flash 芯片驱动 (W25Q64 ops 实现)
  └── dev_console.c       控制台 UART 收发 (DMA + 空闲中断)

Platform/        平台抽象层
  ├── pl_mpu.c            MPU 区域配置
  ├── pl_clock.c          系统时钟 + 外设共用时钟
  ├── pl_gpio.c           GPIO 端口时钟使能
  ├── pl_dma.c            DMA1 时钟 + IRQ 配置
  ├── pl_qspi.c           QSPI 外设初始化 + MSP
  ├── pl_uart.c           USART1 + DMA RX 初始化 + ISR
  ├── pl_crc.c            硬件 CRC 单元 (CRC16 + CRC32)
  └── pl_rtc.c            RTC 时钟 + 日历初始化

Kernel/          内核基础设施
  ├── initcall.c          类 Linux initcall 自动初始化框架
  └── ring_buffer.c       环形缓冲区（单生产者/单消费者，ISR 安全）

Core/            STM32CubeMX 生成入口文件
  ├── main.c              程序入口：board_init() → app_bootloader_run()
  ├── stm32h7xx_it.c      ISR 向量表 (SysTick, HardFault 等)
  ├── stm32h7xx_hal_msp.c 全局 HAL MSP (SYSCFG 时钟)
  └── system_stm32h7xx.c  系统初始化 (FPU, 向量表偏移配置)
```

### 初始化流程（`board_init()`）

**阶段 1 — 手动顺序（关键路径，不可重排）**

```
MPU_Config() → SCB_EnableICache() → SCB_EnableDCache()
→ HAL_Init() → SystemClock_Config() → PeriphCommonClock_Config()
```

**阶段 2 — initcall 自动发现**

Linker 按 `.initcall.N` 段优先级自动排序调用：

| 优先级 | 宏 | 模块 |
| ------ | -- | ---- |
| 1 | `arch_initcall` | GPIO 时钟使能 |
| 2 | `subsys_initcall` | DMA1 时钟 + IRQ 配置 |
| 3 | `device_initcall` | QSPI, UART, CRC, RTC 外设初始化 |
| 4 | `driver_initcall` | Flash 芯片驱动, Console 驱动 |

各模块通过宏注册自己的初始化函数：

```c
#include "initcall.h"

void my_init(void) { ... }
device_initcall(my_init);   // 自动在 device_initcall 阶段执行
```

## 通信协议

### 物理层

- USART1: 115200 baud, 8 data bits, no parity, 1 stop bit
- 接收：DMA1_Stream0，配合空闲中断实现不定长帧定界
- 发送：阻塞模式

### 帧格式（大端序）

```
┌──────┬──────┬─────────────┬────────────────────┬──────────────┐
│ SOF  │ CMD  │ Payload Len │    Payload          │   CRC16      │
│ 1 B  │ 1 B  │    2 B      │    0 – 256 B        │    2 B       │
│ 0xAA │      │  big-endian │                     │  big-endian   │
└──────┴──────┴─────────────┴────────────────────┴──────────────┘
```

- SOF: 帧头标识，固定 `0xAA`
- CMD: 命令码 / 应答码
- Payload Len: 负载长度（大端序，0–256）
- CRC16: CRC-16-CCITT (poly=`0x1021`, init=`0xFFFF`)，硬件 CRC 计算

### 命令码

| 值 | 名称 | 方向 | 说明 |
| -- | ---- | ---- | ---- |
| `0x01` | SYNC | 上位机 → 设备 | 握手，获取设备同步信息 |
| `0x02` | ERASE | 上位机 → 设备 | 擦除指定 Flash 扇区 |
| `0x03` | WRITE | 上位机 → 设备 | 写入固件数据块 |
| `0x04` | VERIFY | 上位机 → 设备 | 校验已写入数据的 CRC32 |
| `0x05` | LAUNCH | 上位机 → 设备 | 启动应用程序 |
| `0x06` | RESET | 上位机 → 设备 | 复位设备 |
| `0x07` | INFO | 上位机 → 设备 | 查询设备信息 |
| `0x80` | ACK_OK | 设备 → 上位机 | 操作成功 |
| `0x81` | ACK_ERROR | 设备 → 上位机 | 操作失败 |
| `0x82` | ACK_BUSY | 设备 → 上位机 | 设备忙 |
| `0x83` | ACK_CSUMERR | 设备 → 上位机 | CRC 校验错误 |
| `0x84` | ACK_INVALID | 设备 → 上位机 | 无效命令或参数 |

### 典型升级流程

```
上位机                              设备（Bootloader）
  │                                      │
  ├─── SYNC ───────────────────────────►│  握手，获取 Flash 信息
  │◄── ACK_OK (+ Flash 信息) ───────────┤
  │                                      │
  ├─── INFO ───────────────────────────►│  查询设备信息
  │◄── ACK_OK (+ 设备型号/版本) ────────┤
  │                                      │
  ├─── ERASE (扇区地址) ────────────────►│  擦除目标扇区
  │◄── ACK_OK ──────────────────────────┤
  │                                      │
  ├─── WRITE (块0: 头部) ───────────────►│  写入固件头部
  │◄── ACK_OK ──────────────────────────┤
  ├─── WRITE (块1: 数据) ───────────────►│  写入固件数据
  │◄── ACK_OK ──────────────────────────┤
  ├─── WRITE (块N: 数据) ───────────────►│
  │◄── ACK_OK ──────────────────────────┤
  │                                      │
  ├─── VERIFY ─────────────────────────►│  校验整包 CRC32
  │◄── ACK_OK ──────────────────────────┤
  │                                      │
  ├─── LAUNCH ─────────────────────────►│  跳转执行用户固件
  │◄── ACK_OK ──────────────────────────┤  （此后 Bootloader 退出）
```

## 固件镜像格式

上位机打包固件时，需在原始 bin 文件前附加 **28 字节头部**：

```c
typedef struct __attribute__((packed)) {
    uint32_t magic;          // 魔数: 0x0D000721
    uint32_t version;        // 固件版本，如 0x00010000 = v1.0.0
    uint32_t image_size;     // 固件大小（字节，不含头部）
    uint32_t entry_point;    // 入口地址（Vector Table 基址）
    uint32_t load_address;   // QSPI Flash 加载地址偏移
    uint32_t image_crc32;    // 固件本体 CRC32（不含头部）
    uint32_t header_crc;     // 头部自身 CRC32（计算时此字段填 0）
} firmware_header_t;
```

- CRC32 使用 STM32H7 硬件 CRC（Ethernet polynomial `0x04C11DB7`）
- `header_crc` 计算范围：`magic` 到 `image_crc32`（含），即头部前 24 字节
- 使用顺序：上位机先用 `header_crc` 校验头部合法性，Bootloader 收到后也先校验头部，再逐块校验固件本体

## 编译构建

### 依赖

- **arm-none-eabi-gcc** (ARM GNU Toolchain)
- **mingw32-make** (Windows/MSYS2) 或 **make** (Linux)
- STM32CubeMX v6.14.1（仅代码生成时需要）

### 构建命令

```bash
# 构建
make

# 或指定工具链路径
make GCC_PATH=/d/SoftWare/msys2/ucrt64/bin

# 清理
make clean
```

### 产物

| 文件 | 路径 | 说明 |
| ---- | ---- | ---- |
| `Bootloader.elf` | `build/` | 可执行文件（含调试信息） |
| `Bootloader.hex` | `build/` | Intel HEX 格式 |
| `Bootloader.bin` | `build/` | 原始二进制，烧录用 |

### 编译选项

- 优化：`-Og`（调试友好优化）
- 调试：`-g -gdwarf-2`
- C 标准库：`nano.specs`（精简版 newlib）
- 链接：`--gc-sections`（丢弃未引用的 section）
- 依赖生成：`-MMD -MP`（自动头文件依赖）

> **注意**：在 MSYS2 环境中，make 命令为 `mingw32-make`，不是 `make`。

## 目录结构

```
Bootloader/
├── Application/                应用层
│   ├── Inc/                    头文件
│   └── Src/                    源文件
├── Device/                     设备驱动层
│   ├── Inc/
│   └── Src/
├── Platform/                   平台抽象层
│   ├── Inc/
│   └── Src/
├── Kernel/                     内核基础设施
│   ├── Inc/
│   └── Src/
├── Core/                       STM32CubeMX 生成
│   ├── Inc/
│   └── Src/
├── Drivers/                    HAL 库（CubeMX 生成，禁止修改）
│   ├── CMSIS/
│   └── STM32H7xx_HAL_Driver/
├── build/                      编译产物目录
├── Makefile                    构建脚本
├── STM32H750XX_FLASH.ld        链接脚本
├── Bootloader.ioc          STM32CubeMX 项目文件
└── startup_stm32h750xx.s       启动汇编
```

## 二次开发注意事项

1. **Drivers/ 目录**是 CubeMX 自动生成的 HAL 库，**禁止手动修改**，重新生成 CubeMX 代码时会被覆盖。
2. **Core/ 目录**中仅 `main.c` 有用户代码区域（`USER CODE` 块），重新生成后需确保 `board_init()` + `app_bootloader_run()` 保持在 `USER CODE BEGIN 2` 区域内。
3. **USART1 DMA RX ISR** 在 `Platform/Src/pl_uart.c` 中实现：`DMA1_Stream0_IRQHandler` → `HAL_DMA_IRQHandler(&hdma_usart1_rx)` → 回调到 Device 层。
4. **新增外设驱动**的初始化函数应通过 `initcall` 宏注册，按照以下优先级选择：
   - `arch_initcall` — GPIO 时钟等架构级配置
   - `subsys_initcall` — DMA、IRQ 等子系统
   - `device_initcall` — UART、SPI、I²C 等外设
   - `driver_initcall` — 芯片级驱动
5. **Flash 驱动**使用类 Linux `file_operations` 的 ops 模式，换用不同 SPI Flash 芯片时只需实现新的 `dev_flash_ops_t` 并调用 `dev_flash_register()` 注册。
6. **环形缓冲区**支持单生产者/单消费者无锁操作，ISR 与主循环均可安全访问。容量固定 2KB，可通过 `RING_BUFFER_SIZE` 宏调整。
7. **CRC 外设**同时支持 CRC16-CCITT（协议帧校验）和 CRC32-MPEG2（固件校验），由 Platform 层统一管理时钟和初始化，上层通过 `app_fw_crc32()` 等 API 调用。
8. **固件跳转流程**（`app_fw_jump_to_app`）：
   - QSPI 切换为内存映射模式
   - 关闭全局中断、停止 SysTick
   - 清理 D-Cache
   - 关闭 USART1 / DMA1 时钟
   - 设置 VTOR → 加载 MSP → 跳转 Reset_Handler

## 引脚分配

| 功能 | 引脚 | 说明 |
| ---- | ---- | ---- |
| QSPI IO0 | PD11 | QSPI Bank1 IO0 |
| QSPI IO1 | PD12 | QSPI Bank1 IO1 |
| QSPI IO2 | PE2 | QSPI Bank1 IO2 |
| QSPI IO3 | PD13 | QSPI Bank1 IO3 |
| QSPI CLK | PB2 | QSPI 时钟 |
| QSPI NCS | PB6 | QSPI 片选 |
| USART1 TX | PB14 | 串口发送 |
| USART1 RX | PB15 | 串口接收 |
| SWCLK | PA14 | SWD 调试时钟 |
| SWDIO | PA13 | SWD 调试数据 |
| HSE_IN | PH0 | 25 MHz 外部晶振 |
| HSE_OUT | PH1 | 25 MHz 外部晶振 |
| LSE_IN | PC14 | 32.768 kHz RTC 晶振 |
| LSE_OUT | PC15 | 32.768 kHz RTC 晶振 |

## License

本项目自定义代码（Application / Device / Platform / Kernel / Core）采用 [MIT License](LICENSE)。

Drivers/ 目录下的 HAL 库为 STM32CubeMX 生成，版权归 STMicroelectronics 所有，详见其 [BSD 3-Clause License](https://github.com/STMicroelectronics/STM32CubeH7)。
