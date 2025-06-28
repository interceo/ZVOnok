#include <arpa/inet.h>
#include <portaudio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>

#include "Audio.hpp"

#define PORT 12345

void sender(int sock, sockaddr_in serverAddr, Audio& audio_client) {
    SAMPLE buffer[BUF_SIZE];
    while (true) {
        audio_client.GetInputStreamBuffer(buffer);
        sendto(sock, buffer, sizeof(buffer), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
    }
}

void receiver(int sock, Audio& audio_client) {
    SAMPLE buffer[BUF_SIZE];
    while (true) {
        recv(sock, buffer, sizeof(buffer), 0);
        audio_client.SetOutputStreamBuffer(buffer);
    }
}

int main() {
    Audio audio_client;

    audio_client.CreateDefaultInputStream();
    audio_client.CreateDefaultOutputStream();

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    sendto(sock, "", 1, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::thread sendThread(sender, sock, serverAddr, std::ref(audio_client));
    std::thread recvThread(receiver, sock, std::ref(audio_client));

    sendThread.join();
    recvThread.join();

    close(sock);
    return 0;
}