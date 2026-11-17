# GCC 固件构建

本目录拥有当前 STM32F767IGT6 工程的最终构建策略、工具链、链接布局和输出规则。子仓库只提供源码接口，最终是否编译由本目录决定。

build 中的 gcc/out/debug 保存构建缓存，build_file 保存最终固件产物；CMake、Ninja 和 Python 终端依赖由工程外的软件环境提供。本机统一安装于 E:/PorjectSoftware/tools，构建脚本不再携带或回退到工程内工具。

```text
build/
├── build_file/           最终 BIN、HEX、MAP、ELF
└── gcc/                  构建定义、开关、后端脚本
    └── out/debug/        中间产物和构建缓存
```

构建定义纳入版本控制，out 为可重新生成的本机构建产物。此前的验证输出目录已清理，可按下面命令重新生成。

最终文件名从 board/config/version.h 读取，格式为大写项目名_主版本.次版本.修订版本，例如 ETHERNET_0.0.1.bin、.hex、.map、.elf。版本从 0.0.1 开始，每次发布按十进制进位：0.0.9 → 0.1.0、0.9.9 → 1.0.0；次版本和修订版本取值为 0..9，普通编译不自动递增。修改版本头文件后重新构建即可更新名称。build_file 保存最后一次成功构建发布的产物，不区分 Debug/Release；各配置的中间文件保留在各自缓存目录中。clean 同时清理当前配置及其对应的发布文件。

## 使用

工具要求与当前验证版本：

| 工具 | 要求 | 当前验证版本 |
|---|---|---|
| Python | 3.9+ | 3.14.6 |
| CMake | 3.20+ | 4.4.5 |
| Ninja | 可用的 Ninja 构建后端 | 1.13.2 |
| ARM GCC | 支持 Cortex-M7、FPv5-D16、hard-float、C11/ASM | Arm GNU 15.2.Rel1 / GCC 15.2.1 |

除明确声明的最低版本外，上表列出实际验证基线，不代表其他 Ninja/GCC 版本已经测试。工具安装目录位于工程外，当前为 E:/PorjectSoftware/tools；工程不保留 tools 副本。

从主仓库根目录执行：

```powershell
python platform_common/scripts/build/build.py build
python platform_common/scripts/build/build.py build --build-type Release
python platform_common/scripts/build/build.py size
python platform_common/scripts/build/build.py clean
python platform_common/scripts/build/build.py rebuild --jobs 4

# 独立输出目录，避免不同配置共用 CMake 缓存。
python platform_common/scripts/build/build.py build --output out/no-heartbeat -DGCC_HEARTBEAT=OFF
python platform_common/scripts/build/build.py build --output out/baremetal -DGCC_ENABLE_FREERTOS=OFF -DGCC_ENABLE_RTT=OFF
python platform_common/scripts/build/build.py build --output out/no-timers -DGCC_TIMERS=OFF

```

Python 入口是 `python -B platform_common/scripts/build/build.py`。有多个构建后端时加 `--backend build/gcc`。`--output` 相对本目录，默认 `out/debug`；其他构建类型使用对应的小写目录名。额外 CMake 参数以 `-DKEY=VALUE` 传入。

## 功能开关

统一声明于 options.cmake：

| 参数 | 默认值 | 作用 |
|---|---|---|
| `GCC_ENABLE_FREERTOS` | ON | 是否链接 RTOS 内核、移植层及任务管理；OFF 时保留 HAL 和裸机等待循环 |
| `GCC_ENABLE_RTT` | ON | 是否链接 RTT；OFF 时日志接口编译为空操作 |
| `GCC_HEARTBEAT` | AUTO | AUTO 随 FreeRTOS 启用；ON/OFF 显式覆盖 |
| `GCC_TIMERS` | AUTO | AUTO 随 FreeRTOS 启用；ON/OFF 显式覆盖 |
| `GCC_FREERTOS_PORT` | CM7_R0P1 | CM7_R0P1 或 CM4F，最终只链接一个移植层 |
| `GCC_MAIN_STACK_SIZE` | 4096 | 主/中断栈预留字节数，至少 256、8 字节对齐并适配 DTCM |
| `GCC_WARNINGS_AS_ERRORS` | ON | C 编译警告视为错误 |

强制开启心跳或软件定时器时必须同时开启 FreeRTOS，非法组合在配置阶段报错。参数直接在各自配置头文件中维护，最终开关和源码选择位于此处。链接内存容量直接读取芯片配置头文件中的十进制字节宏，不再生成中间配置文件。已有输出目录会保留 CMake 缓存；更改开关时显式传 -D 参数，或选一个新的输出目录。

## 工具路径

build.json 的 tools 字段配置 CMake、Ninja、ARM GCC 的名称或路径，commands 字段提供构建动作。相对路径以本目录为基准，也可通过 `CMAKE`、`NINJA`、`ARM_GCC` 环境变量提供可执行文件路径；名称会在 PATH 中查找。工具只从 PATH、环境变量或 build.json 的 tools 字段查找。构建脚本不自动安装工具。

直接运行 CMake 时，工具链文件支持 `-DARM_GCC_BIN=编译器bin目录`，同时提供 Python 和 Ninja 路径即可：

```powershell
cmake -S build/gcc -B build/gcc/out/debug -G Ninja
cmake --build build/gcc/out/debug --target firmware
```

## 子仓接口与输出

- common::stm32f7xx：HAL、CMSIS、系统初始化及启动文件。
- common::freertos_core / freertos_heap4 / freertos_timers / freertos_port_cm7 / freertos_port_cm4f：按需组合的内核接口。
- common::rtt：RTT 基础和格式化输出。
- application::main / task_manage / heartbeat：应用入口、任务管理和心跳任务接口。

这些都是 INTERFACE 源码目标，不在子仓自行决定启用状态，也不把可选任务藏在普通静态库里。最终目标统一提供 board/config、板级接口及 PROJECT_USE_FREERTOS、PROJECT_USE_RTT、PROJECT_USE_SOFTWARE_TIMERS 的 0/1 定义。

build/build_file 包含 ETHERNET_0.0.1.elf、ETHERNET_0.0.1.hex、ETHERNET_0.0.1.bin、ETHERNET_0.0.1.map，名称随 version.h 更新；compile_commands.json 及中间产物位于 gcc/out/debug。主仓库保留本目录的构建源文件，忽略 out 中的构建产物。clean 使用 CMake 的 clean 目标，并检查缓存属于本后端。

## 内存与运行库

链接脚本由 memory.ld.in 按芯片容量生成：Flash 1 MiB；普通数据和 FreeRTOS heap_4 放到 SRAM1+SRAM2（384 KiB）；主/中断栈放到 DTCM 顶部，默认预留 4 KiB，剩余 DTCM 单独保留。链接器检查区域容量。

runtime.c 提供厂商 Reset_Handler 调用 C 库初始化时需要的 _init/_fini；标准库选用 nano.specs/nosys.specs，FreeRTOS heap_4 使用自己的内存池。当前 RTT 日志使用 SEGGER 接口，标准库 I/O 不自动重定向到 RTT。

已验证默认 RTOS+RTT+心跳、关闭心跳/RTT、裸机，以及 CM4F 移植层的 Release 构建。固件完成实际 ELF 链接和 HEX/BIN 导出，尚未上板运行。
