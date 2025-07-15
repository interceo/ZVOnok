#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <thread>
#include <mutex>
#include <json/json.h>

struct Client {
    std::string id;
    std::string room_id;
    int socket_fd;
    sockaddr_in address;
    
    Client(const std::string& client_id, int sock_fd, const sockaddr_in& addr) 
        : id(client_id), socket_fd(sock_fd), address(addr) {}
};

class SignalingServer {
public:
    SignalingServer(int port = 12345);
    ~SignalingServer();

    bool Start();
    void Stop();
    
private:
    int port_;
    int server_socket_;
    bool is_running_;
    std::thread server_thread_;
    
    // Клиенты и комнаты
    std::mutex clients_mutex_;
    std::unordered_map<std::string, std::shared_ptr<Client>> clients_;
    std::unordered_map<std::string, std::unordered_set<std::string>> rooms_;
    
    // Основной цикл сервера
    void ServerLoop();
    
    // Обработка сообщений
    void HandleMessage(const std::string& message, const sockaddr_in& client_addr, int socket_fd);
    void ProcessSignalingMessage(const Json::Value& msg, std::shared_ptr<Client> client);
    
    // Управление клиентами и комнатами
    std::string RegisterClient(int socket_fd, const sockaddr_in& address);
    void UnregisterClient(const std::string& client_id);
    void JoinRoom(const std::string& client_id, const std::string& room_id);
    void LeaveRoom(const std::string& client_id);
    
    // Пересылка сообщений
    void BroadcastToRoom(const std::string& room_id, const Json::Value& message, const std::string& sender_id = "");
    void SendToClient(const std::string& client_id, const Json::Value& message);
    
    // Обработка WebRTC сигналинга
    void HandleOffer(const Json::Value& message, std::shared_ptr<Client> sender);
    void HandleAnswer(const Json::Value& message, std::shared_ptr<Client> sender);
    void HandleIceCandidate(const Json::Value& message, std::shared_ptr<Client> sender);
    
    // Утилиты
    std::string GenerateClientId();
    Json::Value CreateMessage(const std::string& type, const Json::Value& data);
    void SendJsonMessage(int socket_fd, const sockaddr_in& address, const Json::Value& message);
}; 