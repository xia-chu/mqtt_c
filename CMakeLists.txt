cmake_minimum_required(VERSION 2.8)
project(mqtt)

#使能c++11
#add_compile_options(-std=c++11)
set(CMAKE_CXX_STANDARD 11)

#加载自定义模块
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
#设置库文件路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
#设置可执行程序路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)


#设置工程源码根目录
set(ToolKit_Root ${CMAKE_SOURCE_DIR}/../ZLMediaKit/ZLToolKit/src)
set(Mqtt_Root ${CMAKE_SOURCE_DIR}/src)

#设置头文件目录
INCLUDE_DIRECTORIES(${ToolKit_Root})
INCLUDE_DIRECTORIES(${Mqtt_Root})

#收集源代码
file(GLOB ToolKit_src_list ${ToolKit_Root}/*/*.cpp ${ToolKit_Root}/*/*.h ${ToolKit_Root}/*/*.c)
file(GLOB Mqtt_src_Root ${Mqtt_Root}/*.cpp ${Mqtt_Root}/*.h ${Mqtt_Root}/*.c ${Mqtt_Root}/*.hpp)

#去除win32的适配代码
if (NOT WIN32)
    list(REMOVE_ITEM ToolKit_src_list ${ToolKit_Root}/win32/getopt.c)
endif ()

#添加库
add_library(zltoolkit STATIC ${ToolKit_src_list})
add_library(mqtt STATIC ${Mqtt_src_Root})
add_executable(client main.c)
target_link_libraries(client mqtt zltoolkit pthread)



