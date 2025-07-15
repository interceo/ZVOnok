#pragma once

#include <rtc/rtc.hpp>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

#include "Audio.hpp"

class WebRTCAudio {
public:
    using OnAudioDataCallback = std::function<void(const std::vector<uint8_t>&)>;
    using OnRemoteAudioCallback = std::function<void(const std::vector<uint8_t>&)>;

    WebRTCAudio();
    ~WebRTCAudio();

    // Инициализация WebRTC компонентов
    bool Initialize();
    void Cleanup();

    // Настройка P2P соединения
    void CreatePeerConnection(const std::string& remote_id);
    void SetLocalDescription(const std::string& sdp);
    void SetRemoteDescription(const std::string& sdp);
    void AddIceCandidate(const std::string& candidate);

    // Аудио треки
    void StartAudioCapture();
    void StopAudioCapture();
    void SetRemoteAudioCallback(OnRemoteAudioCallback callback);

    // Сигналинг колбэки
    void SetOnLocalDescription(std::function<void(std::string)> callback);
    void SetOnIceCandidate(std::function<void(std::string)> callback);

private:
    // WebRTC компоненты
    std::shared_ptr<rtc::PeerConnection> peer_connection_;
    std::shared_ptr<rtc::Track> audio_track_;
    
    // Аудио компоненты
    std::unique_ptr<Audio> audio_device_;
    
    // Потоки и синхронизация
    std::thread audio_capture_thread_;
    std::atomic<bool> is_capturing_;
    std::mutex audio_mutex_;
    
    // Колбэки
    OnRemoteAudioCallback remote_audio_callback_;
    std::function<void(std::string)> on_local_description_;
    std::function<void(std::string)> on_ice_candidate_;

    // Приватные методы
    void AudioCaptureLoop();
    void SetupMediaTracks();
    void SetupPeerConnectionCallbacks();
    
    // Обработка аудио данных
    void ProcessAudioInput();
    void ProcessAudioOutput(const std::vector<uint8_t>& data);
}; 