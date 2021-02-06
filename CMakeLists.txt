cmake_minimum_required(VERSION 3.10)


project(vcc VERSION 0.01)

add_executable(vulcanTest.out src/main.cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
