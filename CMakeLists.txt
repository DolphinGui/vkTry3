cmake_minimum_required(VERSION 3.10)

project(vcc VERSION 0.3)
include_directories(${CMAKE_SOURCE_DIR}/src)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(SAMPLES_BUILD ON)

cmake_policy(SET CMP0072 NEW)
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

add_subdirectory(src/)
add_subdirectory(shaders/)
