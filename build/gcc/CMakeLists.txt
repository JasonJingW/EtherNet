# Requirements: CMake >= 3.20, Python >= 3.9, Ninja, ARM bare-metal GCC (C11/ASM).
# Validated: CMake 4.4.5, Ninja 1.13.2, ARM GNU 15.2.Rel1 / GCC 15.2.1,
# Python 3.14.6. Tools are provided by the external environment, not this repo.
cmake_minimum_required(VERSION 3.20)
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/toolchain.cmake" CACHE FILEPATH "ARM GCC toolchain")
endif()
project(EtherNetFirmware LANGUAGES C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
get_filename_component(ETHERNET_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
include("${CMAKE_CURRENT_LIST_DIR}/options.cmake")

# Keep artifact names in sync with the firmware's public version header.
set(_version_header "${ETHERNET_PROJECT_ROOT}/board/config/version.h")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_version_header}")
file(READ "${_version_header}" _version_text)
if(NOT _version_text MATCHES "#[ \t]*define[ \t]+PROJECT_NAME[ \t]+\"([A-Za-z0-9_-]+)\"")
    message(FATAL_ERROR "version.h must define PROJECT_NAME as a file-safe string")
endif()
string(TOUPPER "${CMAKE_MATCH_1}" _artifact_project)
foreach(_part MAJOR MINOR PATCH)
    if(NOT _version_text MATCHES "#[ \t]*define[ \t]+PROJECT_VERSION_${_part}[ \t]+\\([ \t]*([0-9]+)[ \t]*\\)")
        message(FATAL_ERROR "version.h must define parenthesized numeric PROJECT_VERSION_${_part}")
    endif()
    set(_version_${_part} "${CMAKE_MATCH_1}")
endforeach()
if(_version_MINOR GREATER 9 OR _version_PATCH GREATER 9)
    message(FATAL_ERROR "Version uses decimal carry: MINOR/PATCH must be 0..9 (0.0.9 -> 0.1.0)")
endif()
set(_artifact_name "${_artifact_project}_${_version_MAJOR}.${_version_MINOR}.${_version_PATCH}")
set(_artifact_directory "${ETHERNET_PROJECT_ROOT}/build/build_file")

add_subdirectory("${ETHERNET_PROJECT_ROOT}/platform_common" components/common)
add_subdirectory("${ETHERNET_PROJECT_ROOT}/platform_application" components/application)
add_executable(firmware
    "${ETHERNET_PROJECT_ROOT}/board/bsp/bsp_gpio.c"
    "${ETHERNET_PROJECT_ROOT}/board/bsp/bsp_eth.c"
    "${ETHERNET_PROJECT_ROOT}/board/bsp/bsp_iic.c"
    "${ETHERNET_PROJECT_ROOT}/board/bsp/bsp_sai.c"
    "${ETHERNET_PROJECT_ROOT}/board/bsp/bsp_usart.c"
    runtime.c
)
set_target_properties(firmware PROPERTIES OUTPUT_NAME "${_artifact_name}" SUFFIX .elf)
find_package(Git QUIET)
set(_build_version_header "${CMAKE_CURRENT_BINARY_DIR}/version_build.h")
add_custom_target(firmware_build_version
    COMMAND "${CMAKE_COMMAND}" "-DPROJECT_ROOT=${ETHERNET_PROJECT_ROOT}"
        "-DGIT_EXECUTABLE=${GIT_EXECUTABLE}" "-DOUTPUT_FILE=${_build_version_header}"
        -P "${CMAKE_CURRENT_LIST_DIR}/write_build_version.cmake"
    BYPRODUCTS "${_build_version_header}"
    COMMENT "Generating firmware build version" VERBATIM)
add_dependencies(firmware firmware_build_version)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/firmware_path.txt"
    CONTENT "$<TARGET_FILE:firmware>\n")
target_include_directories(firmware PRIVATE "${ETHERNET_PROJECT_ROOT}/board/config"
    "${ETHERNET_PROJECT_ROOT}/board/bsp" "${ETHERNET_PROJECT_ROOT}/board/config/lwip"
    "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(firmware PRIVATE STM32F767xx USE_HAL_DRIVER
    PROJECT_USE_RTT=$<BOOL:${GCC_ENABLE_RTT}>
    PROJECT_USE_SOFTWARE_TIMERS=$<BOOL:${GCC_WITH_TIMERS}>)
set(_gcc_arch -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)
target_compile_options(firmware PRIVATE ${_gcc_arch} -ffunction-sections -fdata-sections
    "$<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra>"
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<BOOL:${GCC_WARNINGS_AS_ERRORS}>>:-Werror>
    "$<$<CONFIG:Debug>:-Og;-g3>")
target_link_libraries(firmware PRIVATE common::stm32f7xx application::main application::audio application::es8388
    common::stm32f7xx_eth common::stm32f7xx_i2c common::stm32f7xx_sai common::lwip application::network
    common::stm32f7xx_uart application::cli application::version
    common::freertos_core common::freertos_heap4 application::task_manage)
if(GCC_FREERTOS_PORT STREQUAL "CM7_R0P1")
    target_link_libraries(firmware PRIVATE common::freertos_port_cm7)
else()
    target_link_libraries(firmware PRIVATE common::freertos_port_cm4f)
endif()
if(GCC_WITH_TIMERS)
    target_link_libraries(firmware PRIVATE common::freertos_timers)
endif()
if(GCC_ENABLE_RTT)
    target_link_libraries(firmware PRIVATE common::rtt)
    target_compile_definitions(firmware PRIVATE
        SEGGER_RTT_SECTION=".noncache.rtt.control"
        SEGGER_RTT_BUFFER_SECTION=".noncache.rtt.buffers")
endif()
if(GCC_WITH_HEARTBEAT)
    target_link_libraries(firmware PRIVATE application::heartbeat)
endif()

# Reserve shared RAM at the start of SRAM1 and the main stack in DTCM.
configure_file(memory.ld.in "${CMAKE_CURRENT_BINARY_DIR}/memory.ld" @ONLY)
set_property(TARGET firmware APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/memory.ld")
target_link_options(firmware PRIVATE ${_gcc_arch} -nostartfiles --specs=nano.specs --specs=nosys.specs
    "-T${CMAKE_CURRENT_BINARY_DIR}/memory.ld" "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${_artifact_name}.map"
    -Wl,--gc-sections,--cref,--print-memory-usage)
target_link_libraries(firmware PRIVATE m c)
add_custom_command(TARGET firmware POST_BUILD
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/${_artifact_name}.hex"
        "${CMAKE_CURRENT_BINARY_DIR}/${_artifact_name}.bin" "${CMAKE_CURRENT_BINARY_DIR}/${_artifact_name}.map"
    COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:firmware>" "${CMAKE_CURRENT_BINARY_DIR}/${_artifact_name}.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:firmware>" "${CMAKE_CURRENT_BINARY_DIR}/${_artifact_name}.bin"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:firmware>" VERBATIM)
# Always publish the selected cache's outputs, including when switching profiles
# without recompiling. The shared directory represents the last successful build.
add_custom_target(firmware_files ALL
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_artifact_directory}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:firmware>"
        "${CMAKE_CURRENT_BINARY_DIR}/${_artifact_name}.bin"
        "${CMAKE_CURRENT_BINARY_DIR}/${_artifact_name}.hex"
        "${CMAKE_CURRENT_BINARY_DIR}/${_artifact_name}.map"
        "${_artifact_directory}"
    DEPENDS firmware
    BYPRODUCTS "${_artifact_directory}/${_artifact_name}.elf"
        "${_artifact_directory}/${_artifact_name}.bin"
        "${_artifact_directory}/${_artifact_name}.hex"
        "${_artifact_directory}/${_artifact_name}.map"
    COMMENT "Publishing ${_artifact_name} to build/build_file" VERBATIM)
