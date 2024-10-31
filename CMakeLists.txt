cmake_minimum_required(VERSION 3.25)
project(toy_async)

set(CMAKE_CXX_STANDARD 20)

add_executable(toy_async main.cpp toy_async.cpp)

add_executable(bench bench_mark.cpp toy_async.cpp)