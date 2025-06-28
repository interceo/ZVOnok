cmake_minimum_required(VERSION 3.5)
project(server)

set(CMAKE_CXX_STANDARD 20)

add_executable(server main.cpp)

target_link_libraries(server PRIVATE trantor)