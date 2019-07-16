# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_STANDARD 11)
add_definitions(-Wno-int-to-void-pointer-cast -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast)

#设置生成的so动态库最后输出的路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs_export/${ANDROID_ABI})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs_export/${ANDROID_ABI})

#设置工程源码根目录
set(Mqtt_Root ${CMAKE_SOURCE_DIR}/../../../../../src/)

#设置头文件目录
INCLUDE_DIRECTORIES(${Mqtt_Root}/include)

#收集源代码
file(GLOB Mqtt_src_Root ${Mqtt_Root}/*/*.h ${Mqtt_Root}/*/*.c)

add_library(mqtt STATIC ${Mqtt_src_Root})
add_library(mqtt_jni SHARED  native-lib.cpp)
target_link_libraries(mqtt_jni mqtt log)
