#include "WebRTCAudio.hpp"
#include <iostream>
#include <chrono>

WebRTCAudio::WebRTCAudio() 
    : is_capturing_(false) {
    audio_device_ = std::make_unique<Audio>();
}

WebRTCAudio::~WebRTCAudio() {
    Cleanup();
}

bool WebRTCAudio::Initialize() {
    try {
        // Настройка конфигурации WebRTC
        rtc::Configuration config;
        
        // Добавляем STUN серверы для NAT traversal
        config.iceServers.emplace_back("stun:stun.l.google.com:19302");
        config.iceServers.emplace_back("stun:stun1.l.google.com:19302");
        
        // Создаем PeerConnection
        peer_connection_ = std::make_shared<rtc::PeerConnection>(config);
        
        SetupPeerConnectionCallbacks();
        SetupMediaTracks();
        
        std::cout << "WebRTC initialized successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize WebRTC: " << e.what() << std::endl;
        return false;
    }
}

void WebRTCAudio::Cleanup() {
    StopAudioCapture();
    
    if (peer_connection_) {
        peer_connection_->close();
        peer_connection_.reset();
    }
    
    audio_track_.reset();
    audio_device_.reset();
}

void WebRTCAudio::CreatePeerConnection(const std::string& remote_id) {
    // PeerConnection уже создан в Initialize()
    std::cout << "Setting up connection for remote: " << remote_id << std::endl;
}

void WebRTCAudio::SetLocalDescription(const std::string& sdp) {
    try {
        rtc::Description desc(sdp, rtc::Description::Type::Offer);
        peer_connection_->setLocalDescription(desc);
    } catch (const std::exception& e) {
        std::cerr << "Failed to set local description: " << e.what() << std::endl;
    }
}

void WebRTCAudio::SetRemoteDescription(const std::string& sdp) {
    try {
        rtc::Description desc(sdp, rtc::Description::Type::Answer);
        peer_connection_->setRemoteDescription(desc);
    } catch (const std::exception& e) {
        std::cerr << "Failed to set remote description: " << e.what() << std::endl;
    }
}

void WebRTCAudio::AddIceCandidate(const std::string& candidate) {
    try {
        peer_connection_->addRemoteCandidate(rtc::Candidate(candidate));
    } catch (const std::exception& e) {
        std::cerr << "Failed to add ICE candidate: " << e.what() << std::endl;
    }
}

void WebRTCAudio::StartAudioCapture() {
    if (is_capturing_) {
        return;
    }
    
    audio_device_->CreateDefaultInputStream();
    audio_device_->CreateDefaultOutputStream();
    
    is_capturing_ = true;
    audio_capture_thread_ = std::thread(&WebRTCAudio::AudioCaptureLoop, this);
    
    std::cout << "Audio capture started" << std::endl;
}

void WebRTCAudio::StopAudioCapture() {
    if (!is_capturing_) {
        return;
    }
    
    is_capturing_ = false;
    
    if (audio_capture_thread_.joinable()) {
        audio_capture_thread_.join();
    }
    
    audio_device_->Clear();
    std::cout << "Audio capture stopped" << std::endl;
}

void WebRTCAudio::SetRemoteAudioCallback(OnRemoteAudioCallback callback) {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    remote_audio_callback_ = callback;
}

void WebRTCAudio::SetOnLocalDescription(std::function<void(std::string)> callback) {
    on_local_description_ = callback;
}

void WebRTCAudio::SetOnIceCandidate(std::function<void(std::string)> callback) {
    on_ice_candidate_ = callback;
}

void WebRTCAudio::AudioCaptureLoop() {
    SAMPLE buffer[BUF_SIZE];
    
    while (is_capturing_) {
        // Захватываем аудио с микрофона
        audio_device_->GetInputStreamBuffer(buffer);
        
        // Конвертируем в байты для отправки через WebRTC
        std::vector<uint8_t> audio_data(
            reinterpret_cast<uint8_t*>(buffer),
            reinterpret_cast<uint8_t*>(buffer) + sizeof(buffer)
        );
        
        // Отправляем через WebRTC track
        if (audio_track_) {
            try {
                audio_track_->send(audio_data);
            } catch (const std::exception& e) {
                std::cerr << "Failed to send audio data: " << e.what() << std::endl;
            }
        }
        
        // Небольшая задержка для контроля частоты кадров
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void WebRTCAudio::SetupMediaTracks() {
    try {
        // Создаем аудио трек
        audio_track_ = peer_connection_->addTrack(rtc::Description::Audio("audio"));
        
        // Настраиваем колбэк для получения удаленного аудио
        peer_connection_->onTrack([this](std::shared_ptr<rtc::Track> track) {
            std::cout << "Received remote audio track" << std::endl;
            
            track->onMessage([this](rtc::binary message) {
                std::vector<uint8_t> audio_data(message.begin(), message.end());
                ProcessAudioOutput(audio_data);
            });
        });
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to setup media tracks: " << e.what() << std::endl;
    }
}

void WebRTCAudio::SetupPeerConnectionCallbacks() {
    // Колбэк для локального SDP
    peer_connection_->onLocalDescription([this](rtc::Description description) {
        if (on_local_description_) {
            on_local_description_(std::string(description));
        }
    });
    
    // Колбэк для ICE кандидатов
    peer_connection_->onLocalCandidate([this](rtc::Candidate candidate) {
        if (on_ice_candidate_) {
            on_ice_candidate_(std::string(candidate));
        }
    });
    
    // Колбэк для состояния соединения
    peer_connection_->onStateChange([](rtc::PeerConnection::State state) {
        switch (state) {
            case rtc::PeerConnection::State::New:
                std::cout << "PeerConnection: New" << std::endl;
                break;
            case rtc::PeerConnection::State::Connecting:
                std::cout << "PeerConnection: Connecting" << std::endl;
                break;
            case rtc::PeerConnection::State::Connected:
                std::cout << "PeerConnection: Connected" << std::endl;
                break;
            case rtc::PeerConnection::State::Disconnected:
                std::cout << "PeerConnection: Disconnected" << std::endl;
                break;
            case rtc::PeerConnection::State::Failed:
                std::cout << "PeerConnection: Failed" << std::endl;
                break;
            case rtc::PeerConnection::State::Closed:
                std::cout << "PeerConnection: Closed" << std::endl;
                break;
        }
    });
}

void WebRTCAudio::ProcessAudioOutput(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    
    // Конвертируем байты обратно в SAMPLE формат
    if (data.size() >= sizeof(SAMPLE) * BUF_SIZE) {
        const SAMPLE* samples = reinterpret_cast<const SAMPLE*>(data.data());
        
        // Воспроизводим полученное аудио
        audio_device_->SetOutputStreamBuffer(samples);
        
        // Вызываем колбэк если установлен
        if (remote_audio_callback_) {
            remote_audio_callback_(data);
        }
    }
} 