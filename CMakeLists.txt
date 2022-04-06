cmake_minimum_required(VERSION 3.16)
project(timer)
set(CMAKE_CXX_STANDARD 20)
include_directories("src")

add_executable(timer src/main.cpp)
