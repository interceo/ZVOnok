#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <unordered_set>
#include <vector>

#define PORT 12345
#define BUF_SIZE 4096

struct ClientAddrHash {
    std::size_t operator()(const sockaddr_in& addr) const {
        return std::hash<uint32_t>()(addr.sin_addr.s_addr) ^ std::hash<uint16_t>()(addr.sin_port);
    }
};
struct ClientAddrEqual {
    bool operator()(const sockaddr_in& a, const sockaddr_in& b) const {
        return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
    }
};

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in serverAddr{}, clientAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));

    std::unordered_set<sockaddr_in, ClientAddrHash, ClientAddrEqual> clients;
    char buffer[BUF_SIZE];

    std::cout << "Voice chat server started on port " << PORT << std::endl;

    while (true) {
        socklen_t len = sizeof(clientAddr);
        int n = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &len);

        if (n > 0) {
            clients.insert(clientAddr);

            for (const auto& c : clients) {
                if (!(c.sin_addr.s_addr == clientAddr.sin_addr.s_addr && c.sin_port == clientAddr.sin_port)) {
                    sendto(sock, buffer, n, 0, (sockaddr*)&c, sizeof(c));
                }
            }
        }
    }
    close(sock);
    return 0;
}