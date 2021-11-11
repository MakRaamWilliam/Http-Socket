cmake_minimum_required(VERSION 3.20)
project(server)

set(CMAKE_CXX_STANDARD 14)

link_libraries(ws2_32 wsock32)

add_executable(server main.cpp)
