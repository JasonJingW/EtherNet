# Validated compiler: Arm GNU Toolchain 15.2.Rel1, GCC 15.2.1.
# Target ABI: Cortex-M7 / Thumb / FPv5-D16 / hard float (set on the final target).
# Set ARM_GCC_BIN or put arm-none-eabi-gcc on PATH; no bundled compiler is used.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
find_program(CMAKE_C_COMPILER NAMES arm-none-eabi-gcc
    HINTS "${ARM_GCC_BIN}" "$ENV{ARM_GCC_BIN}" REQUIRED)
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")
get_filename_component(_gcc_bin "${CMAKE_C_COMPILER}" DIRECTORY)
find_program(CMAKE_OBJCOPY NAMES arm-none-eabi-objcopy HINTS "${_gcc_bin}" REQUIRED)
find_program(CMAKE_SIZE NAMES arm-none-eabi-size HINTS "${_gcc_bin}" REQUIRED)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
