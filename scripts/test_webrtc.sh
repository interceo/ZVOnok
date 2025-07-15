#!/bin/bash

# Скрипт для тестирования WebRTC функциональности
# Usage: ./test_webrtc.sh

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

echo "=== ZVOnok WebRTC Test Script ==="
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"

# Проверяем, что проект собран
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found. Building project..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake ..
    make -j$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
    cd "$PROJECT_ROOT"
fi

# Проверяем наличие исполняемых файлов
SIGNALING_SERVER="$BUILD_DIR/server/signaling_server"
CLIENT_WEBRTC="$BUILD_DIR/client/client_webrtc"

if [ ! -f "$SIGNALING_SERVER" ] || [ ! -f "$CLIENT_WEBRTC" ]; then
    echo "WebRTC executables not found. Make sure the project is built with WebRTC support."
    echo "Try: cd build && cmake .. -DBUILD_WEBRTC=ON && make"
    exit 1
fi

echo "Found executables:"
echo "  Signaling server: $SIGNALING_SERVER"
echo "  WebRTC client: $CLIENT_WEBRTC"
echo

# Функция для запуска сервера в фоне
start_server() {
    echo "Starting signaling server on port 12345..."
    "$SIGNALING_SERVER" 12345 &
    SERVER_PID=$!
    echo "Server PID: $SERVER_PID"
    
    # Ждем запуска сервера
    sleep 2
    
    # Проверяем, что сервер запустился
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo "Failed to start signaling server"
        exit 1
    fi
    
    echo "Signaling server started successfully"
}

# Функция для остановки сервера
stop_server() {
    if [ ! -z "$SERVER_PID" ]; then
        echo "Stopping signaling server (PID: $SERVER_PID)..."
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
        echo "Signaling server stopped"
    fi
}

# Обработчик сигналов для корректного завершения
cleanup() {
    echo
    echo "Cleaning up..."
    stop_server
    exit 0
}

trap cleanup SIGINT SIGTERM

# Запускаем сервер
start_server

echo
echo "=== Server is running ==="
echo "You can now connect clients manually:"
echo "  Terminal 1: $CLIENT_WEBRTC 127.0.0.1 12345 test_room"
echo "  Terminal 2: $CLIENT_WEBRTC 127.0.0.1 12345 test_room"
echo
echo "Or choose an option:"
echo "1) Start first client automatically"
echo "2) Stop server and exit"
echo "3) View server logs"

while true; do
    read -p "Enter choice (1-3): " choice
    case $choice in
        1)
            echo "Starting first client in test_room..."
            echo "After this client starts, open another terminal and run:"
            echo "  $CLIENT_WEBRTC 127.0.0.1 12345 test_room"
            echo
            "$CLIENT_WEBRTC" 127.0.0.1 12345 test_room
            ;;
        2)
            cleanup
            ;;
        3)
            echo "Server is running in background (PID: $SERVER_PID)"
            echo "Use 'ps aux | grep signaling_server' to see server status"
            echo "Server output is not captured in this script"
            ;;
        *)
            echo "Invalid choice. Please enter 1, 2, or 3."
            ;;
    esac
done 