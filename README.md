# STM32F767IGT6 基础工程

当前包含芯片基础依赖、板级时钟、LED、RTT 输出及 FreeRTOS 应用入口。


固件构建入口为 `python platform_common/scripts/build/build.py build`。最终开关、工具链及链接规则位于 [build/gcc](build/gcc/readme.md)，子仓通过 CMake 源码接口提供各组件。最终产物位于 build/build_file，按大写项目名_版本号命名（如 ETHERNET_0.0.1.bin）；构建缓存位于 build/gcc/out/debug。

build 仅保留 gcc 构建定义和默认产物；CMake、Ninja 和终端依赖安装在工程外，由系统环境提供。工具配置与构建动作统一保存在 build/gcc/build.json。

- `platform_common/core/stm32f7xx/hal_driver/`：HAL 基础、GPIO、RCC、Cortex、PWR、FLASH 模块及许可证。
- `platform_common/core/stm32f7xx/cmsis/`：Cortex-M7 内核头文件、STM32F767 设备头文件、系统文件和 GCC 启动文件。
- `platform_common/core/stm32f7xx/config/stm32f7xx_hal_conf.h`：当前 HAL 模块开关、晶振频率等配置。
- `platform_common/RTT/`：统一使用此处的 SEGGER RTT 源码和配置，主仓库中的重复副本已删除。
- `platform_common/FreeRTOS/`：RTOS 内核、GCC/Cortex-M7 移植层、heap_4、配置模板和说明，当前选用 CM7/r0p1 移植层并已接入调度器。
- `board/bsp/board.c`、`board/bsp/board.h`：板级时钟初始化、LED 和错误处理；构建时将 `board/bsp` 加入包含路径。
- `board/config/project_config.h`：上层统一包含入口，汇总常用 C 标准库、STM32 HAL/CMSIS、硬件初始化、RTT 日志及 FreeRTOS 接口；板级接口另行包含 `board.h`。
- `board/config/FreeRTOSConfig.h`：1 kHz tick、32 KiB heap_4 堆、中断优先级和任务配置。构建时将 `board/config` 加入包含路径。
- `platform_common/core/stm32f7xx/config/stm32f767xgx_config.h`：芯片内存容量与 `CHIP_RTOS_*` 可调整参数；RTOS 配置逐项优先使用芯片参数，缺省时保留原值，设置 `FREERTOS_USE_CHIP_CONFIG=0` 可跳过芯片配置头。
- `platform_application/main.c`：初始化后调用任务管理入口；`task/task_manage.c/.h` 负责创建任务、启动调度器和 RTOS 错误钩子，心跳任务当前每 50 ms 翻转 LED0。SysTick/HardFault 位于 `platform_common/core/stm32f7xx/system_init.c`，SVC/PendSV 由 RTOS 移植层提供。
- `platform_application/task/task_heartbeat.c/.h`：心跳任务实现及其自身接口。
- `board/config/task_config.h`：仅集中保存任务优先级，其余配置和创建接口由各任务自身管理；任务管理模块不依赖各任务私有头文件。

芯片文件的包含路径、源文件及型号宏见 `platform_common/core/stm32f7xx/README.md`。HAL 文件直接加入手工工程，不依赖 CubeMX 生成代码。

以太网代码已移出仓库暂存，并保留修改前快照、ZIP 副本和 SHA256 清单；当前没有网络初始化与通信功能。暂存位置记录于工作区仓库外的 `ethernet_staging/latest.txt`。

已通过 ARM GCC 完整固件构建及 ELF 链接，输出 ELF、HEX、BIN 和 MAP，并验证了功能开关组合；尚未上板运行。工作区顶层的原始 HAL 驱动参考仓库及板卡手册保持原位置。

## 项目配置

直接编辑头文件，无需生成步骤：

- 芯片容量和 RTOS 内存预算：`platform_common/core/stm32f7xx/config/stm32f767xgx_config.h`。
- RTOS 调度及中断设置：`board/config/FreeRTOSConfig.h`。
- 任务优先级：`board/config/task_config.h`；名称、栈、周期和启动延时在任务自身头文件中。
- RTT 日志级别和颜色：`platform_common/RTT/Config/SEGGER_RTT_Conf.h`。
- 项目名称和版本：`board/config/version.h`。
- 最终编译开关：`build/gcc/options.cmake`，也可通过构建命令传入 `-D` 覆盖。

```powershell
python platform_common/scripts/build/build.py build
python platform_common/scripts/build/build.py rebuild --jobs 4
```

详细工具版本、功能开关和产物规则见 [构建说明](build/gcc/readme.md)。
