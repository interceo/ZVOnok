cmake_minimum_required(VERSION 3.5)
project(client)

set(CMAKE_CXX_STANDARD 20)

# Создаем исполняемые файлы
add_executable(client main.cpp Audio.cpp)
add_executable(client_webrtc main_webrtc.cpp Audio.cpp WebRTCAudio.cpp)

# Подключаем библиотеки для обычного клиента
target_link_libraries(client PRIVATE portaudio trantor)

# Подключаем библиотеки для WebRTC клиента
target_link_libraries(client_webrtc PRIVATE 
    portaudio 
    trantor 
    datachannel 
    jsoncpp
)