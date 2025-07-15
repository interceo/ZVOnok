cmake_minimum_required(VERSION 3.5)
project(server)

set(CMAKE_CXX_STANDARD 20)

# Создаем исполняемые файлы
add_executable(server main.cpp)
add_executable(signaling_server main.cpp SignalingServer.cpp)

# Подключаем библиотеки для обычного сервера
target_link_libraries(server PRIVATE trantor)

# Подключаем библиотеки для сигналинг сервера
target_link_libraries(signaling_server PRIVATE 
    trantor 
    jsoncpp
)