cmake_minimum_required(VERSION 3.5)
project(ZVOnok_VoiceChat)

set(CMAKE_CXX_STANDARD 20)

# Опции сборки
option(BUILD_WEBRTC "Build WebRTC version" ON)
option(BUILD_ORIGINAL "Build original UDP version" ON)

# Поиск зависимостей
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# Поиск libdatachannel
find_package(datachannel QUIET)
if(NOT datachannel_FOUND)
    message(WARNING "libdatachannel not found. WebRTC functionality will be limited.")
    set(BUILD_WEBRTC OFF)
endif()

# Поиск jsoncpp
find_package(jsoncpp QUIET)
if(NOT jsoncpp_FOUND)
    pkg_check_modules(JSONCPP jsoncpp)
    if(NOT JSONCPP_FOUND)
        message(WARNING "jsoncpp not found. WebRTC functionality will be limited.")
        set(BUILD_WEBRTC OFF)
    endif()
endif()

# Включение подпроектов
if(BUILD_ORIGINAL)
    add_subdirectory(client)
    add_subdirectory(server)
    message(STATUS "Building original UDP version")
endif()

if(BUILD_WEBRTC)
    message(STATUS "Building WebRTC version")
endif()

# Установка
install(TARGETS client server
    RUNTIME DESTINATION bin
    OPTIONAL
)

if(BUILD_WEBRTC)
    install(TARGETS client_webrtc signaling_server
        RUNTIME DESTINATION bin
        OPTIONAL
    )
endif()

# Информация о сборке
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}") 