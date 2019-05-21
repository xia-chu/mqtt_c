cmake_minimum_required(VERSION 2.8)
project(mqtt)

#使能c++11
#add_compile_options(-std=c++11)
set(CMAKE_CXX_STANDARD 11)
add_definitions(-Wno-int-to-void-pointer-cast -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast)


#加载自定义模块
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
#设置库文件路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
#设置可执行程序路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)


#设置工程源码根目录
set(Mqtt_Root ${CMAKE_SOURCE_DIR}/src)

#设置头文件目录
INCLUDE_DIRECTORIES(${Mqtt_Root}/include)

#收集源代码
file(GLOB Mqtt_src_Root ${Mqtt_Root}/*/*.h ${Mqtt_Root}/*/*.c)

add_library(mqtt STATIC ${Mqtt_src_Root})
add_executable(client main.c net.c)
add_executable(bench main_bench.c net.c)
target_link_libraries(client mqtt pthread)
target_link_libraries(bench mqtt pthread)




