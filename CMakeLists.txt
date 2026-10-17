# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

project(app LANGUAGES C CXX)



# 生成可执行文件
add_executable(app app/src/main.cpp)

target_include_directories(app PRIVATE app/include)
# 链接库
target_link_libraries(app wiringPi wiringPiDev)

# 编译选项
target_compile_options(app PRIVATE -Wall -O2)