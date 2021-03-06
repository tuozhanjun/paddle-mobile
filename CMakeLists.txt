cmake_minimum_required(VERSION 3.0)
project(paddle-mobile)

# select the platform to build
option(CPU "armv7 with neon support" ON)
option(MALI_GPU "mali gpu support" OFF)
option(FPGA "fpga support" OFF)

option(USE_OPENMP "openmp support" OFF)
option(DEBUGING "enable debug mode" ON)
option(USE_EXCEPTION "use std exception" OFF)
option(LOG_PROFILE "log profile" OFF)

file(GLOB_RECURSE PADDLE_MOBILE_CC src/*.cc src/*.cpp src/*.c src/*.mm)
file(GLOB_RECURSE PADDLE_MOBILE_H src/*.h)
include_directories(src/)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-O3 -s -DNDEBUG ${CMAKE_CXX_FLAGS}")
if(IS_IOS)
    set(CMAKE_CXX_FLAGS "-mfpu=neon -marm -fobjc-abi-version=2 -fobjc-arc \
        -std=gnu++11 -stdlib=libc++ -isysroot ${CMAKE_OSX_SYSROOT} ${CMAKE_CXX_FLAGS}")
else()
    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
endif()

if(DEBUGING)
    message(STATUS "debugging mode")
    add_definitions(-DPADDLE_MOBILE_DEBUG)
else()
    add_definitions(-fvisibility=hidden -fvisibility-inlines-hidden)
endif()

if(USE_EXCEPTION)
    message(STATUS "use exception")
    add_definitions(-DENABLE_EXCEPTION -fexceptions)
else()
    add_definitions(-fno-exceptions)
endif()

if(LOG_PROFILE)
    add_definitions(-DPADDLE_MOBILE_PROFILE)
endif()

if(USE_OPENMP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
    add_definitions(-DPADDLE_MOBILE_USE_OPENMP)
endif()

# platform control
if(ARM_LINUX)
    include("${CMAKE_CURRENT_LIST_DIR}/tools/arm-platform.cmake")
endif()

if(CPU)
    add_definitions(-DPADDLE_MOBILE_CPU)
else()
    file(GLOB_RECURSE _tmp_list src/operators/kernel/arm/*.cpp src/operators/kernel/arm/*.cc)
    foreach(f ${_tmp_list})
        list(REMOVE_ITEM PADDLE_MOBILE_CC ${f})
    endforeach()

    file(GLOB_RECURSE _tmp_list_h src/operators/kernel/arm/*.h)
    foreach(f ${_tmp_list_h})
        list(REMOVE_ITEM PADDLE_MOBILE_H ${f})
    endforeach()
endif()

if(MALI_GPU)
    add_definitions(-DPADDLE_MOBILE_MALI_GPU)
    add_definitions(-DUSE_ACL=1)
    add_definitions(-DUSE_OPENCL)
    set(ACL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src/operators/kernel/mali/ACL_Android)
    include_directories(${ACL_ROOT} ${ACL_ROOT}/include)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${ACL_ROOT}/build")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -larm_compute")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -larm_compute_core")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -larm_compute_graph")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${ACL_ROOT}/build/opencl-1.2-stubs")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lOpenCL")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_ACL=1")
else()
    file(GLOB_RECURSE _tmp_list src/operators/kernel/mali/*.cpp src/operators/kernel/mali/*.cc)
    foreach(f ${_tmp_list})
        list(REMOVE_ITEM PADDLE_MOBILE_CC ${f})
    endforeach()

    file(GLOB_RECURSE _tmp_list_h src/operators/kernel/mali/*.h)
    foreach(f ${_tmp_list_h})
        list(REMOVE_ITEM PADDLE_MOBILE_H ${f})
    endforeach()
endif()

if(FPGA)
    set(DEBUGING ON)
    add_definitions(-DPADDLE_MOBILE_DEBUG)
    add_definitions(-DPADDLE_MOBILE_FPGA)
else()
    file(GLOB_RECURSE _tmp_list src/operators/kernel/fpga/*.cpp src/operators/kernel/fpga/*.cc)
    foreach(f ${_tmp_list})
        list(REMOVE_ITEM PADDLE_MOBILE_CC ${f})
    endforeach()

    file(GLOB_RECURSE _tmp_list_h src/operators/kernel/fpga/*.h)
    foreach(f ${_tmp_list_h})
        list(REMOVE_ITEM PADDLE_MOBILE_H ${f})
    endforeach()


    file(GLOB_RECURSE _tmp_list src/fpga/*.cpp src/fpga/*.cc)
    foreach(f ${_tmp_list})
        list(REMOVE_ITEM PADDLE_MOBILE_CC ${f})
    endforeach()

    file(GLOB_RECURSE _tmp_list_h src/fpga/*.h)
    foreach(f ${_tmp_list_h})
        list(REMOVE_ITEM PADDLE_MOBILE_H ${f})
    endforeach()
endif()

if(ANDROID_NDK_TOOLCHAIN_INCLUDED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -llog")
else()
    list(REMOVE_ITEM PADDLE_MOBILE_H ${CMAKE_CURRENT_SOURCE_DIR}/src/jni/paddle_mobile_jni.h)
    list(REMOVE_ITEM PADDLE_MOBILE_CC ${CMAKE_CURRENT_SOURCE_DIR}/src/jni/paddle_mobile_jni.cpp)
    list(REMOVE_ITEM PADDLE_MOBILE_H ${CMAKE_CURRENT_SOURCE_DIR}/src/operators/math/math_func_neon.h)
endif()

if(IS_IOS)
else()
    list(REMOVE_ITEM PADDLE_MOBILE_H ${CMAKE_CURRENT_SOURCE_DIR}/src/ios_io/PaddleMobileCPU.h)
    list(REMOVE_ITEM PADDLE_MOBILE_CC ${CMAKE_CURRENT_SOURCE_DIR}/src/ios_io/PaddleMobileCPU.mm)
    list(REMOVE_ITEM PADDLE_MOBILE_H ${CMAKE_CURRENT_SOURCE_DIR}/src/ios_io/op_symbols.h)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY build)

# NET default
if(FPGA)
    set(NET "FPGAnets" CACHE STRING "select net type")
else()
    set(NET "default" CACHE STRING "select net type")
endif()

set_property(CACHE NET PROPERTY STRINGS "default" "googlenet" "mobilenet" "yolo" "squeezenet" "FPGAnets" "NLP")
include("${CMAKE_CURRENT_LIST_DIR}/tools/op.cmake")


# build library
if(ANDROID_NDK_TOOLCHAIN_INCLUDED)
    list(REMOVE_DUPLICATES CMAKE_CXX_FLAGS)
    add_library(paddle-mobile SHARED ${PADDLE_MOBILE_CC} ${PADDLE_MOBILE_H})
elseif(IS_IOS)
    if(USE_OPENMP)
        add_library(paddle-mobile-stage0 STATIC ${PADDLE_MOBILE_CC} ${PADDLE_MOBILE_H})
        add_custom_target(paddle-mobile ALL
            COMMAND libtool -static -o ${CMAKE_BINARY_DIR}/libpaddle-mobile.a ${CMAKE_CURRENT_LIST_DIR}/tools/libomp.a $<TARGET_FILE:paddle-mobile-stage0>
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS paddle-mobile
        )
        add_dependencies(paddle-mobile paddle-mobile-stage0)
    else()
        add_library(paddle-mobile STATIC ${PADDLE_MOBILE_CC} ${PADDLE_MOBILE_H})
    endif()
else()
  add_library(paddle-mobile SHARED ${PADDLE_MOBILE_CC} ${PADDLE_MOBILE_H})
endif()

# unit test
if(DEBUGING)
    if(IS_IOS)
    else()
        add_subdirectory(test)
    endif()
endif()


