cmake_minimum_required(VERSION 3.10)
project(usb_control_lib)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 添加动态库目标
add_library(usb_control SHARED
    usb_api.c
    usb_control.c
)

# 设置包含目录
target_include_directories(usb_control PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 设置编译选项
if(MSVC)
    target_compile_definitions(usb_control PRIVATE WIN32 _WINDOWS _USRDLL)
endif()
