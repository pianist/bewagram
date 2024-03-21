cmake_minimum_required(VERSION 2.8.12)
project(bewagram C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(HiSiliconSDK REQUIRED)
find_package(Threads REQUIRED)
find_library(CURL_LIBRARY NAMES curl)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${VENDOR_SDK_INCLUDE_DIRS})
include_directories(libev)

message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")

add_library(aux STATIC aux/logger.c aux/system.c)
add_library(cfg STATIC cfg/common.c cfg/sensor_config.c cfg/sensor_cfg_print.c)
add_library(libevx STATIC libev/ev.c)
add_library(evcurl STATIC evcurl/evcurl.c)
add_library(evgpio STATIC evgpio/evgpio.c)
add_library(hitiny STATIC hitiny/hitiny_sys.c hitiny/hitiny_aio.c hitiny/hitiny_ao.c hitiny/hitiny_venc_fd.c)

add_executable(test_evcurl tests/test_evcurl.c)
target_link_libraries(test_evcurl evcurl ${CURL_LIBRARY} libevx)

add_executable(test_watcher_gpio tests/test_watcher_gpio.c)
target_link_libraries(test_watcher_gpio evgpio libevx)

add_executable(bewagramd bewagramd/main.c bewagramd/cfg.c bewagramd/call_button.c bewagramd/snap.c)
target_link_libraries(bewagramd aux cfg hitiny evcurl ${CURL_LIBRARY} evgpio libevx)

