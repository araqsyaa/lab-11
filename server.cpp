#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>

#define PORT 8080
#define MAX_CLIENT 3

typedef struct info {
    struct sockaddr_in ip;
    int newfd;
    char name[20];
} info;

info Clients[MAX_CLIENT];
SOCKET sockfd;
std::ofstream logfile;

void LogMessage(const std::string& message) {
    if (logfile.is_open()) {
        logfile << message << std::endl;
    }
}

DWORD WINAPI FunctionThread(LPVOID arg) {
    info* ptr = (info*)arg;

    while (true) {
        char readbuf[1024];
        memset(readbuf, 0, sizeof(readbuf));

        int bytes_received = recv(ptr->newfd, readbuf, sizeof(readbuf), 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("%s disconnected.\n", ptr->name);
            } else {
                perror("recv");
            }

            closesocket(ptr->newfd);
            LogMessage(std::string(ptr->name) + " disconnected.");
            break;
        }

        std::string receivedMessage = std::string(ptr->name) + ": " + std::string(readbuf);
        printf("Received: %s\n", receivedMessage.c_str());
        LogMessage(receivedMessage);

        for (int i = 0; i < MAX_CLIENT; i++) {
            if (Clients[i].newfd != ptr->newfd && Clients[i].newfd > 0) {
                send(Clients[i].newfd, readbuf, bytes_received, 0);
            }
        }
    }

    return 0;
}

int main() {
    WSADATA wsaData;
    struct sockaddr_in serverAddr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        perror("socket");
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        perror("bind");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    if (listen(sockfd, MAX_CLIENT) == SOCKET_ERROR) {
        perror("listen");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    logfile.open("server_log.txt", std::ios::app);
    if (!logfile.is_open()) {
        std::cerr << "Failed to open server log file." << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    printf("Server is running and waiting for connections...\n");

    for (int i = 0; i < MAX_CLIENT; i++) {
        socklen_t clientSize = sizeof(serverAddr);
        Clients[i].newfd = accept(sockfd, (struct sockaddr*)&serverAddr, &clientSize);

        if (Clients[i].newfd == INVALID_SOCKET) {
            perror("accept");
            continue;
        }

        if (recv(Clients[i].newfd, Clients[i].name, sizeof(Clients[i].name), 0) == SOCKET_ERROR) {
            perror("recv");
            closesocket(Clients[i].newfd);
            continue;
        }

        printf("%s connected\n", Clients[i].name);
        LogMessage(std::string(Clients[i].name) + " connected.");

        CreateThread(NULL, 0, FunctionThread, (LPVOID)&Clients[i], 0, NULL);
    }

    while (true) {
        Sleep(1000);
    }

    closesocket(sockfd);
    WSACleanup();
    logfile.close();
    return 0;
}
