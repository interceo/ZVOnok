#include "WebRTCAudio.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <json/json.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

class SignalingClient {
public:
    SignalingClient(const std::string& server_ip, int server_port) 
        : server_ip_(server_ip), server_port_(server_port), socket_(-1), client_id_("") {}
    
    ~SignalingClient() {
        Disconnect();
    }
    
    bool Connect() {
        socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_ < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
        
        server_addr_.sin_family = AF_INET;
        server_addr_.sin_port = htons(server_port_);
        inet_pton(AF_INET, server_ip_.c_str(), &server_addr_.sin_addr);
        
        // Отправляем первое сообщение для регистрации
        Json::Value hello_msg;
        hello_msg["type"] = "hello";
        SendMessage(hello_msg);
        
        // Запускаем поток для получения сообщений
        is_running_ = true;
        receive_thread_ = std::thread(&SignalingClient::ReceiveLoop, this);
        
        return true;
    }
    
    void Disconnect() {
        is_running_ = false;
        
        if (receive_thread_.joinable()) {
            receive_thread_.join();
        }
        
        if (socket_ >= 0) {
            close(socket_);
            socket_ = -1;
        }
    }
    
    void JoinRoom(const std::string& room_id) {
        Json::Value join_msg;
        join_msg["type"] = "join_room";
        join_msg["room_id"] = room_id;
        join_msg["client_id"] = client_id_;
        SendMessage(join_msg);
    }
    
    void SendOffer(const std::string& sdp, const std::string& target = "") {
        Json::Value offer_msg;
        offer_msg["type"] = "offer";
        offer_msg["client_id"] = client_id_;
        offer_msg["data"]["sdp"] = sdp;
        if (!target.empty()) {
            offer_msg["target"] = target;
        }
        SendMessage(offer_msg);
    }
    
    void SendAnswer(const std::string& sdp, const std::string& target) {
        Json::Value answer_msg;
        answer_msg["type"] = "answer";
        answer_msg["client_id"] = client_id_;
        answer_msg["target"] = target;
        answer_msg["data"]["sdp"] = sdp;
        SendMessage(answer_msg);
    }
    
    void SendIceCandidate(const std::string& candidate, const std::string& target) {
        Json::Value ice_msg;
        ice_msg["type"] = "ice_candidate";
        ice_msg["client_id"] = client_id_;
        ice_msg["target"] = target;
        ice_msg["data"]["candidate"] = candidate;
        SendMessage(ice_msg);
    }
    
    // Колбэки для WebRTC событий
    std::function<void(std::string, std::string)> on_offer;
    std::function<void(std::string, std::string)> on_answer;
    std::function<void(std::string, std::string)> on_ice_candidate;
    std::function<void(std::string)> on_user_joined;
    std::function<void(std::string)> on_user_left;
    
private:
    std::string server_ip_;
    int server_port_;
    int socket_;
    sockaddr_in server_addr_;
    std::string client_id_;
    
    std::thread receive_thread_;
    std::atomic<bool> is_running_;
    
    void SendMessage(const Json::Value& message) {
        Json::StreamWriterBuilder builder;
        std::string json_string = Json::writeString(builder, message);
        
        sendto(socket_, json_string.c_str(), json_string.length(), 0,
               (sockaddr*)&server_addr_, sizeof(server_addr_));
    }
    
    void ReceiveLoop() {
        char buffer[4096];
        
        while (is_running_) {
            int bytes_received = recv(socket_, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                HandleMessage(std::string(buffer));
            }
        }
    }
    
    void HandleMessage(const std::string& message) {
        try {
            Json::Value root;
            Json::Reader reader;
            
            if (!reader.parse(message, root)) {
                std::cerr << "Failed to parse JSON message" << std::endl;
                return;
            }
            
            std::string type = root.get("type", "").asString();
            
            if (type == "client_registered") {
                client_id_ = root.get("client_id", "").asString();
                std::cout << "Registered with ID: " << client_id_ << std::endl;
            } else if (type == "offer" && on_offer) {
                std::string sender = root.get("sender", "").asString();
                std::string sdp = root["data"].get("sdp", "").asString();
                on_offer(sdp, sender);
            } else if (type == "answer" && on_answer) {
                std::string sender = root.get("sender", "").asString();
                std::string sdp = root["data"].get("sdp", "").asString();
                on_answer(sdp, sender);
            } else if (type == "ice_candidate" && on_ice_candidate) {
                std::string sender = root.get("sender", "").asString();
                std::string candidate = root["data"].get("candidate", "").asString();
                on_ice_candidate(candidate, sender);
            } else if (type == "user_joined" && on_user_joined) {
                std::string user_id = root.get("user_id", "").asString();
                on_user_joined(user_id);
            } else if (type == "user_left" && on_user_left) {
                std::string user_id = root.get("user_id", "").asString();
                on_user_left(user_id);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error handling message: " << e.what() << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    std::string server_ip = "127.0.0.1";
    int server_port = 12345;
    std::string room_id = "default";
    
    // Парсим аргументы командной строки
    if (argc > 1) server_ip = argv[1];
    if (argc > 2) server_port = std::atoi(argv[2]);
    if (argc > 3) room_id = argv[3];
    
    std::cout << "Connecting to signaling server at " << server_ip << ":" << server_port << std::endl;
    
    // Создаем сигналинг клиент
    SignalingClient signaling_client(server_ip, server_port);
    
    if (!signaling_client.Connect()) {
        std::cerr << "Failed to connect to signaling server" << std::endl;
        return 1;
    }
    
    // Создаем WebRTC аудио клиент
    WebRTCAudio webrtc_audio;
    
    if (!webrtc_audio.Initialize()) {
        std::cerr << "Failed to initialize WebRTC" << std::endl;
        return 1;
    }
    
    // Настраиваем колбэки между сигналинг клиентом и WebRTC
    signaling_client.on_offer = [&](const std::string& sdp, const std::string& sender) {
        std::cout << "Received offer from " << sender << std::endl;
        webrtc_audio.SetRemoteDescription(sdp);
        // Здесь должен быть код для создания answer
    };
    
    signaling_client.on_answer = [&](const std::string& sdp, const std::string& sender) {
        std::cout << "Received answer from " << sender << std::endl;
        webrtc_audio.SetRemoteDescription(sdp);
    };
    
    signaling_client.on_ice_candidate = [&](const std::string& candidate, const std::string& sender) {
        std::cout << "Received ICE candidate from " << sender << std::endl;
        webrtc_audio.AddIceCandidate(candidate);
    };
    
    signaling_client.on_user_joined = [&](const std::string& user_id) {
        std::cout << "User joined: " << user_id << std::endl;
        // Инициируем WebRTC соединение с новым пользователем
        webrtc_audio.CreatePeerConnection(user_id);
    };
    
    signaling_client.on_user_left = [&](const std::string& user_id) {
        std::cout << "User left: " << user_id << std::endl;
    };
    
    // Настраиваем WebRTC колбэки
    webrtc_audio.SetOnLocalDescription([&](const std::string& sdp) {
        std::cout << "Generated local SDP" << std::endl;
        signaling_client.SendOffer(sdp);
    });
    
    webrtc_audio.SetOnIceCandidate([&](const std::string& candidate) {
        std::cout << "Generated ICE candidate" << std::endl;
        // Здесь нужно определить target для отправки
        signaling_client.SendIceCandidate(candidate, ""); 
    });
    
    // Даем время на установку соединения
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Присоединяемся к комнате
    std::cout << "Joining room: " << room_id << std::endl;
    signaling_client.JoinRoom(room_id);
    
    // Запускаем захват аудио
    std::cout << "Starting audio capture..." << std::endl;
    webrtc_audio.StartAudioCapture();
    
    std::cout << "Voice chat client started. Press Enter to exit..." << std::endl;
    std::cin.get();
    
    // Очистка
    webrtc_audio.StopAudioCapture();
    webrtc_audio.Cleanup();
    signaling_client.Disconnect();
    
    return 0;
} 