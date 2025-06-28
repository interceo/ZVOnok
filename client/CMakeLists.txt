cmake_minimum_required(VERSION 3.5)
project(client)

set(CMAKE_CXX_STANDARD 20)

add_executable(client main.cpp Audio.cpp)
target_link_libraries(client PRIVATE portaudio trantor datachannel)