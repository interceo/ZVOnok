#include "SignalingServer.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <random>
#include <sstream>

SignalingServer::SignalingServer(int port) 
    : port_(port), server_socket_(-1), is_running_(false) {}

SignalingServer::~SignalingServer() {
    Stop();
}

bool SignalingServer::Start() {
    // Создаем UDP сокет
    server_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // Настраиваем адрес сервера
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Привязываем сокет
    if (bind(server_socket_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port_ << std::endl;
        close(server_socket_);
        return false;
    }
    
    is_running_ = true;
    server_thread_ = std::thread(&SignalingServer::ServerLoop, this);
    
    std::cout << "Signaling server started on port " << port_ << std::endl;
    return true;
}

void SignalingServer::Stop() {
    if (!is_running_) {
        return;
    }
    
    is_running_ = false;
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
    
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.clear();
    rooms_.clear();
    
    std::cout << "Signaling server stopped" << std::endl;
}

void SignalingServer::ServerLoop() {
    char buffer[4096];
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    
    while (is_running_) {
        int bytes_received = recvfrom(server_socket_, buffer, sizeof(buffer) - 1, 0, 
                                     (sockaddr*)&client_addr, &addr_len);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string message(buffer);
            HandleMessage(message, client_addr, server_socket_);
        }
    }
}

void SignalingServer::HandleMessage(const std::string& message, const sockaddr_in& client_addr, int socket_fd) {
    try {
        Json::Value root;
        Json::Reader reader;
        
        if (!reader.parse(message, root)) {
            std::cerr << "Failed to parse JSON message" << std::endl;
            return;
        }
        
        std::string type = root.get("type", "").asString();
        std::string client_id = root.get("client_id", "").asString();
        
        // Если клиент не зарегистрирован, регистрируем его
        std::shared_ptr<Client> client;
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            
            if (client_id.empty() || clients_.find(client_id) == clients_.end()) {
                client_id = RegisterClient(socket_fd, client_addr);
                client = clients_[client_id];
                
                // Отправляем клиенту его ID
                Json::Value response = CreateMessage("client_registered", Json::Value());
                response["client_id"] = client_id;
                SendJsonMessage(socket_fd, client_addr, response);
            } else {
                client = clients_[client_id];
            }
        }
        
        if (type == "join_room") {
            std::string room_id = root.get("room_id", "default").asString();
            JoinRoom(client_id, room_id);
        } else if (type == "leave_room") {
            LeaveRoom(client_id);
        } else {
            ProcessSignalingMessage(root, client);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void SignalingServer::ProcessSignalingMessage(const Json::Value& msg, std::shared_ptr<Client> client) {
    std::string type = msg.get("type", "").asString();
    
    if (type == "offer") {
        HandleOffer(msg, client);
    } else if (type == "answer") {
        HandleAnswer(msg, client);
    } else if (type == "ice_candidate") {
        HandleIceCandidate(msg, client);
    } else {
        std::cout << "Unknown message type: " << type << std::endl;
    }
}

std::string SignalingServer::RegisterClient(int socket_fd, const sockaddr_in& address) {
    std::string client_id = GenerateClientId();
    auto client = std::make_shared<Client>(client_id, socket_fd, address);
    clients_[client_id] = client;
    
    std::cout << "Client registered: " << client_id << std::endl;
    return client_id;
}

void SignalingServer::UnregisterClient(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    auto client_it = clients_.find(client_id);
    if (client_it != clients_.end()) {
        LeaveRoom(client_id);
        clients_.erase(client_it);
        std::cout << "Client unregistered: " << client_id << std::endl;
    }
}

void SignalingServer::JoinRoom(const std::string& client_id, const std::string& room_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    auto client_it = clients_.find(client_id);
    if (client_it == clients_.end()) {
        return;
    }
    
    // Покидаем предыдущую комнату
    if (!client_it->second->room_id.empty()) {
        LeaveRoom(client_id);
    }
    
    // Присоединяемся к новой комнате
    client_it->second->room_id = room_id;
    rooms_[room_id].insert(client_id);
    
    std::cout << "Client " << client_id << " joined room " << room_id << std::endl;
    
    // Уведомляем всех в комнате о новом участнике
    Json::Value notification = CreateMessage("user_joined", Json::Value());
    notification["user_id"] = client_id;
    BroadcastToRoom(room_id, notification, client_id);
    
    // Отправляем новому участнику список пользователей в комнате
    Json::Value users_list = CreateMessage("room_users", Json::Value());
    Json::Value users(Json::arrayValue);
    for (const auto& user_id : rooms_[room_id]) {
        if (user_id != client_id) {
            users.append(user_id);
        }
    }
    users_list["users"] = users;
    SendToClient(client_id, users_list);
}

void SignalingServer::LeaveRoom(const std::string& client_id) {
    auto client_it = clients_.find(client_id);
    if (client_it == clients_.end() || client_it->second->room_id.empty()) {
        return;
    }
    
    std::string room_id = client_it->second->room_id;
    
    // Удаляем из комнаты
    auto room_it = rooms_.find(room_id);
    if (room_it != rooms_.end()) {
        room_it->second.erase(client_id);
        
        // Если комната пустая, удаляем её
        if (room_it->second.empty()) {
            rooms_.erase(room_it);
        } else {
            // Уведомляем остальных участников
            Json::Value notification = CreateMessage("user_left", Json::Value());
            notification["user_id"] = client_id;
            BroadcastToRoom(room_id, notification, client_id);
        }
    }
    
    client_it->second->room_id.clear();
    std::cout << "Client " << client_id << " left room " << room_id << std::endl;
}

void SignalingServer::BroadcastToRoom(const std::string& room_id, const Json::Value& message, const std::string& sender_id) {
    auto room_it = rooms_.find(room_id);
    if (room_it == rooms_.end()) {
        return;
    }
    
    for (const auto& client_id : room_it->second) {
        if (client_id != sender_id) {
            SendToClient(client_id, message);
        }
    }
}

void SignalingServer::SendToClient(const std::string& client_id, const Json::Value& message) {
    auto client_it = clients_.find(client_id);
    if (client_it != clients_.end()) {
        SendJsonMessage(server_socket_, client_it->second->address, message);
    }
}

void SignalingServer::HandleOffer(const Json::Value& message, std::shared_ptr<Client> sender) {
    std::string target_id = message.get("target", "").asString();
    
    if (target_id.empty()) {
        // Broadcast offer to all in room
        Json::Value offer_msg = CreateMessage("offer", message["data"]);
        offer_msg["sender"] = sender->id;
        BroadcastToRoom(sender->room_id, offer_msg, sender->id);
    } else {
        // Send to specific client
        Json::Value offer_msg = CreateMessage("offer", message["data"]);
        offer_msg["sender"] = sender->id;
        SendToClient(target_id, offer_msg);
    }
}

void SignalingServer::HandleAnswer(const Json::Value& message, std::shared_ptr<Client> sender) {
    std::string target_id = message.get("target", "").asString();
    
    if (!target_id.empty()) {
        Json::Value answer_msg = CreateMessage("answer", message["data"]);
        answer_msg["sender"] = sender->id;
        SendToClient(target_id, answer_msg);
    }
}

void SignalingServer::HandleIceCandidate(const Json::Value& message, std::shared_ptr<Client> sender) {
    std::string target_id = message.get("target", "").asString();
    
    if (!target_id.empty()) {
        Json::Value ice_msg = CreateMessage("ice_candidate", message["data"]);
        ice_msg["sender"] = sender->id;
        SendToClient(target_id, ice_msg);
    }
}

std::string SignalingServer::GenerateClientId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    
    return "client_" + std::to_string(dis(gen));
}

Json::Value SignalingServer::CreateMessage(const std::string& type, const Json::Value& data) {
    Json::Value message;
    message["type"] = type;
    message["data"] = data;
    message["timestamp"] = static_cast<int64_t>(time(nullptr));
    return message;
}

void SignalingServer::SendJsonMessage(int socket_fd, const sockaddr_in& address, const Json::Value& message) {
    Json::StreamWriterBuilder builder;
    std::string json_string = Json::writeString(builder, message);
    
    sendto(socket_fd, json_string.c_str(), json_string.length(), 0, 
           (sockaddr*)&address, sizeof(address));
} 