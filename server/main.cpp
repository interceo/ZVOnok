#include "SignalingServer.hpp"
#include <iostream>
#include <csignal>
#include <memory>
#include <thread>
#include <chrono>

std::unique_ptr<SignalingServer> server;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down server..." << std::endl;
    if (server) {
        server->Stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    // Устанавливаем обработчик сигналов для graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Порт по умолчанию
    int port = 12345;
    
    // Парсим аргументы командной строки
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port number: " << argv[1] << std::endl;
            return 1;
        }
    }
    
    // Создаем и запускаем сигналинг сервер
    server = std::make_unique<SignalingServer>(port);
    
    if (!server->Start()) {
        std::cerr << "Failed to start signaling server on port " << port << std::endl;
        return 1;
    }
    
    std::cout << "WebRTC Signaling Server running on port " << port << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;
    
    // Основной цикл - ждем сигнала завершения
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}