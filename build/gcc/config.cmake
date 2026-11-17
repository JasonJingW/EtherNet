include_guard(GLOBAL)
get_filename_component(ETHERNET_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
set(_config_script "${ETHERNET_PROJECT_ROOT}/config.py")
set(_config_header "${ETHERNET_PROJECT_ROOT}/board/config/generated/project_autoconf.h")
set(_config_cmake "${ETHERNET_PROJECT_ROOT}/board/config/generated/project_config.cmake")

file(GLOB_RECURSE _config_menus CONFIGURE_DEPENDS "${ETHERNET_PROJECT_ROOT}/*/kconfig")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${ETHERNET_PROJECT_ROOT}/board/config/kconfig"
    "${ETHERNET_PROJECT_ROOT}/board/config/project.conf"
    "${_config_script}" ${_config_menus})

execute_process(COMMAND "${Python3_EXECUTABLE}" -B "${_config_script}" generate
    RESULT_VARIABLE _config_result OUTPUT_VARIABLE _config_output ERROR_VARIABLE _config_error)
if(NOT _config_result EQUAL 0)
    message(FATAL_ERROR "Project configuration failed:\n${_config_output}${_config_error}")
endif()
include("${_config_cmake}")

add_custom_target(config_generate
    COMMAND "${Python3_EXECUTABLE}" -B "${_config_script}" generate
    COMMENT "Generating project configuration" VERBATIM)
add_custom_target(config_check
    COMMAND "${Python3_EXECUTABLE}" -B "${_config_script}" check
    COMMENT "Checking project configuration" VERBATIM)
add_custom_target(guiconfig
    COMMAND "${Python3_EXECUTABLE}" -B "${_config_script}" gui
    USES_TERMINAL COMMENT "Opening graphical configuration" VERBATIM)
add_custom_target(menuconfig
    COMMAND "${Python3_EXECUTABLE}" -B "${_config_script}" menu
    USES_TERMINAL COMMENT "Opening terminal configuration" VERBATIM)

# Generated values are inputs only. Final source/feature selection lives in
# the root repository's build backend (currently build/gcc).
