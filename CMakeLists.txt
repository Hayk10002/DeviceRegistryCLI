cmake_minimum_required(VERSION 3.22)

project(DeviceRegistry)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(CLI11 INTERFACE CLI11.hpp)
target_include_directories(CLI11 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(DeviceRegistry STATIC src/DeviceRegistry.cpp src/DeviceRegistry.hpp)
target_include_directories(DeviceRegistry PUBLIC src)

add_executable(device-registry src/main.cpp)
target_link_libraries(device-registry PRIVATE CLI11)
target_link_libraries(device-registry PRIVATE DeviceRegistry)