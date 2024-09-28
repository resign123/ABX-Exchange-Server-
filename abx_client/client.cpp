#include <iostream>
#include <fstream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <map>
#include <set>
#include <chrono>
#include <thread>
#include "../json-develop/single_include/nlohmann/json.hpp"

#pragma comment(lib, "ws2_32.lib")

using json = nlohmann::json;

#define SERVER_PORT 3000
#define PACKET_SIZE 17
#define MAX_RETRIES 5
#define TIMEOUT_SEC 5

struct Packet {
    char symbol[5];
    char buySellIndicator;
    int32_t quantity;
    int32_t price;
    int32_t sequence;
};

void createRequestPayload(uint8_t* request, uint8_t callType, uint8_t resendSeq = 0) {
    request[0] = callType;
    request[1] = resendSeq;
}

Packet parsePacket(const uint8_t* buffer) {
    Packet packet;
    memcpy(packet.symbol, buffer, 4);
    packet.symbol[4] = '\0';
    packet.buySellIndicator = buffer[4];
    packet.quantity = ntohl(*(int32_t*)(buffer + 5));
    packet.price = ntohl(*(int32_t*)(buffer + 9));
    packet.sequence = ntohl(*(int32_t*)(buffer + 13));
    return packet;
}

bool isValidPacket(const Packet& packet) {
    return (packet.sequence > 0 && packet.sequence < 1000000) &&
           (packet.price >= 0 && packet.price < 1000000) &&
           (packet.quantity >= 0 && packet.quantity < 1000000);
}

json packetToJson(const Packet& packet) {
    json j;
    j["symbol"] = packet.symbol;
    j["buySellIndicator"] = std::string(1, packet.buySellIndicator);
    j["quantity"] = packet.quantity;
    j["price"] = packet.price;
    j["sequence"] = packet.sequence;
    return j;
}

SOCKET connectToServer() {
    SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "Error creating socket" << std::endl;
        return INVALID_SOCKET;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        closesocket(sockfd);
        return INVALID_SOCKET;
    }

    return sockfd;
}

bool receiveSinglePacket(SOCKET sockfd, std::map<int32_t, Packet>& receivedPackets, int32_t& maxSequence) {
    uint8_t buffer[PACKET_SIZE];
    int bytesReceived = recv(sockfd, reinterpret_cast<char*>(buffer), PACKET_SIZE, 0);
    if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
        std::cerr << "Error receiving packet or connection closed" << std::endl;
        return false;
    }

    if (bytesReceived == PACKET_SIZE) {
        Packet packet = parsePacket(buffer);
        if (isValidPacket(packet)) {
            receivedPackets[packet.sequence] = packet;
            maxSequence = std::max(maxSequence, packet.sequence);
            std::cout << "Received packet: " << packet.sequence << std::endl;
            return true;
        } else {
            std::cerr << "Invalid packet received" << std::endl;
        }
    }

    return false;
}

bool receivePackets(SOCKET sockfd, std::map<int32_t, Packet>& receivedPackets, int32_t& maxSequence) {
    fd_set readfds;
    struct timeval tv;
    bool connectionClosed = false;

    while (!connectionClosed) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        tv.tv_sec = TIMEOUT_SEC;
        tv.tv_usec = 0;

        int activity = select(0, &readfds, NULL, NULL, &tv);

        if (activity == SOCKET_ERROR) {
            std::cerr << "Select error: " << WSAGetLastError() << std::endl;
            return false;
        }

        if (activity == 0) {
            continue;  // Timeout occurred
        }

        if (!receiveSinglePacket(sockfd, receivedPackets, maxSequence)) {
            connectionClosed = true;
        }
    }

    return true;
}

bool requestMissingPackets(SOCKET sockfd, std::map<int32_t, Packet>& receivedPackets, int32_t maxSequence) {
    std::set<int32_t> missingSequences;
    for (int32_t seq = 1; seq <= maxSequence; ++seq) {
        if (receivedPackets.find(seq) == receivedPackets.end()) {
            missingSequences.insert(seq);
        }
    }

    if (missingSequences.empty()) {
        return true;
    }

    std::cout << "Requesting " << missingSequences.size() << " missing packets..." << std::endl;

    for (int32_t seq : missingSequences) {
        uint8_t request[2];
        createRequestPayload(request, 2, seq);
        if (send(sockfd, reinterpret_cast<const char*>(request), sizeof(request), 0) == SOCKET_ERROR) {
            std::cerr << "Failed to send request for packet " << seq << std::endl;
            continue;
        }

        // Wait for and receive the missing packet
        if (!receiveSinglePacket(sockfd, receivedPackets, maxSequence)) {
            std::cerr << "Failed to receive packet " << seq << std::endl;
        }
    }

    return true;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    std::map<int32_t, Packet> receivedPackets;
    int32_t maxSequence = 0;

    
    SOCKET sockfd = connectToServer();
    if (sockfd == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    uint8_t request[2];
    createRequestPayload(request, 1);
    send(sockfd, reinterpret_cast<const char*>(request), sizeof(request), 0);

    if (!receivePackets(sockfd, receivedPackets, maxSequence)) {
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    closesocket(sockfd);

    // Request missing packets
    sockfd = connectToServer(); // Reconnect to server for requesting missing packets
    if (sockfd == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    for (int i = 0; i < MAX_RETRIES; ++i) {
        if (requestMissingPackets(sockfd, receivedPackets, maxSequence)) {
            break;
        }
        std::cout << "Retry " << (i + 1) << " of " << MAX_RETRIES << std::endl;
    }

    closesocket(sockfd);
    WSACleanup();

  
    json output;
    output["packets"] = json::array();
    for (int32_t seq = 1; seq <= maxSequence; ++seq) {
        if (receivedPackets.find(seq) != receivedPackets.end()) {
            output["packets"].push_back(packetToJson(receivedPackets[seq]));
        }
    }

  
    std::ofstream outputFile("output.json");
    outputFile << output.dump(4);
    outputFile.close();

    std::cout << "Data received and written to output.json" << std::endl;
    std::cout << "Total packets received: " << receivedPackets.size() << std::endl;

    return 0;
}
