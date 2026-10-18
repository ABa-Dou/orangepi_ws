# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   /usr/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

set(CMAKE_SYSROOT /home/abadou/orangepi_ws/sysroot)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

project(app LANGUAGES C CXX)



# 生成可执行文件
add_executable(app
    app/src/main.cpp
    app/src/epoll_wrapper.cpp
    app/src/protocol_parser.cpp
)

target_include_directories(app PRIVATE
    app/include
    ${CMAKE_SYSROOT}/usr/include
)
# 链接库
target_link_libraries(app wiringPi wiringPiDev)

# 编译选项
target_compile_options(app PRIVATE -Wall -O2)