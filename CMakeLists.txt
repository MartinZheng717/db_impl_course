cmake_minimum_required(VERSION 3.16)
project(linklist)

set(CMAKE_CXX_STANDARD 14)

add_executable(linklist main.cpp)

find_package(Threads REQUIRED)
target_link_libraries(linklist Threads::Threads)
