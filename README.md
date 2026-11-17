# STM32F767IGT6 基础工程

当前保留芯片基础依赖、板级时钟、LED、RTT 输出和应用入口。

- `platform_common/core/stm32f7xx/hal_driver/`：HAL 基础、GPIO、RCC、Cortex、PWR、FLASH 模块及许可证。
- `platform_common/core/stm32f7xx/cmsis/`：Cortex-M7 内核头文件、STM32F767 设备头文件、系统文件和 GCC 启动文件。
- `platform_common/core/stm32f7xx/config/stm32f7xx_hal_conf.h`：当前 HAL 模块开关、晶振频率等配置。
- `platform_common/RTT/`：统一使用此处的 SEGGER RTT 源码和配置，主仓库中的重复副本已删除。
- `platform_common/FreeRTOS/`：已复制 RTOS 内核、GCC/Cortex-M7 移植层、heap_4、配置模板和说明，尚未启用调度器。
- `board/bsp/board.c`、`board/bsp/board.h`：板级时钟初始化、LED 和错误处理；构建时将 `board/bsp` 加入包含路径。
- `platform_application/main.c`：基础启动、中断处理和 C 库系统调用适配；启动后通过 RTT 打印 CPU 频率并等待中断。

芯片文件的包含路径、源文件及型号宏见 `platform_common/core/stm32f7xx/README.md`。HAL 文件直接加入手工工程，不依赖 CubeMX 生成代码。

以太网代码已移出仓库暂存，并保留修改前快照、ZIP 副本和 SHA256 清单；当前没有网络初始化与通信功能。暂存位置记录于工作区仓库外的 `ethernet_staging/latest.txt`。

当前尚无完整构建、链接与下载配置，未验证完整固件链接或烧录。工作区顶层的原始 HAL 驱动参考仓库及板卡手册保持原位置。
